/**
 * @file Game.cpp
 * @brief Game flow, rules, camera and effects. The menus, screens and the HUD are in GameUi.cpp.
 * @author reneweller
 */
#include "Game.h"

#include "UiCommon.h"
#include "VectorFont.h"

#include <rlgl.h>

#include <filesystem>
#include <future>

namespace sunstreak
{

using namespace ui;

namespace
{

constexpr float PhysicsStep = 1.0f / 120.0f;    ///< fixed physics step
constexpr float StartTime = 55.0f;              ///< seconds on the clock at the start

/**
 * @brief The time a checkpoint adds.
 * @param[in] k  the checkpoint's number (1 first)
 * @return seconds
 */
float checkpointBonus( int k )
{
    return static_cast<float>( CheckpointSpacing ) / std::min( 64.0f, 48.0f + 2.0f * static_cast<float>( k ) ) * tuning().m_time;
}

/**
 * @brief A point in a pose's frame.
 * @param[in] p  the pose
 * @param[in] x  metres to the left
 * @param[in] y  metres up
 * @param[in] z  metres forward
 * @return the world point
 */
Vec3d poseLocal( const Pose &p, float x, float y, float z )
{
    Vec3d r = p.m_pos;
    r.m_x += static_cast<double>( p.m_left.x * x + p.m_up.x * y + p.m_forward.x * z );
    r.m_y += static_cast<double>( p.m_left.y * x + p.m_up.y * y + p.m_forward.y * z );
    r.m_z += static_cast<double>( p.m_left.z * x + p.m_up.z * y + p.m_forward.z * z );
    return r;
}

/**
 * @brief A random count for a rate, rounded stochastically.
 * @param[in]     rate  events per second
 * @param[in]     dt    seconds
 * @param[in,out] r     generator
 * @return how many events this frame
 */
int emitCount( float rate, float dt, Rng &r )
{
    return static_cast<int>( rate * dt + r.uniform() );
}

/**
 * @brief Where ghosts are kept.
 * @return the folder
 */
std::string ghostDir()
{
    return configDirectory() + "/ghosts";
}

} // namespace

Game::Game( Settings &settings, const LaunchOptions &options ) : m_settings( settings ), m_options( options ) {}

void Game::init()
{
    if( m_options.m_camera >= 0 ) m_settings.m_camera = m_options.m_camera;
    if( m_options.m_timeOfDay >= 0 ) m_settings.m_timeOfDay = m_options.m_timeOfDay;
    setColourBlind( m_settings.m_colourBlind );
    setTextScale( 1.0f + 0.15f * static_cast<float>( m_settings.m_textSize ) );
    m_tuningBase = loadTuning( configDirectory() + "/tuning.ini" );
    tuning() = applyDifficulty( m_tuningBase, m_settings.m_difficulty );
    m_records.load( configDirectory() + "/records.txt" );
    m_models.build();
    m_carIndex = std::clamp( m_settings.m_car, 0, CarCount - 1 );
    m_paintIndex = std::clamp( m_settings.m_paint, 0, PaintCount - 1 );
    m_car.m_spec = carSpec( m_carIndex );
    applyLivery();
    m_renderer.init();
    m_world.init( &m_models );
    const std::vector<std::string> dirs = { std::string( GetApplicationDirectory() ) + "music", configDirectory() + "/music" };
    m_audioOk = !m_options.m_mute && m_audio.init( dirs );
    m_audio.setVolumes( m_settings.m_musicVolume, m_settings.m_sfxVolume );
    m_haptics.init();
    m_lab.m_seed = randomSeed();
    m_lab.m_params.m_biome = SceneryWorld;
    TrackSetup fromCode;
    if( !m_options.m_code.empty() && decodeTrack( m_options.m_code, fromCode ) )
    {
        startTrack( fromCode, ModeE::CUSTOM );
    }
    else if( m_options.m_championship )
    {
        startChampionship();
    }
    else if( m_options.m_skipMenu )
    {
        TrackSetup endless;
        endless.m_seed = m_options.m_seed ? ( ( m_options.m_seed & 0x1FFFFFFFull ) | 1ull ) : randomSeed();
        endless.m_params = endlessParams();
        startTrack( endless, ModeE::ENDLESS );
    }
    else
    {
        startDemo();
        if( m_options.m_screen == "lab" ) m_state = StateE::LAB;
        if( m_options.m_screen == "records" ) m_state = StateE::RECORDS;
        if( m_options.m_screen == "garage" ) m_state = StateE::GARAGE;
        if( m_options.m_screen == "passport" ) m_state = StateE::PASSPORT;
        if( m_options.m_screen == "stamps" || m_options.m_screen == "postcards" )
        {
            m_state = StateE::PASSPORT;
            m_passportPage = static_cast<int>( m_options.m_screen == "stamps" ? PassportE::STAMPS : PassportE::POSTCARDS );
        }
        if( m_options.m_screen == "code" )
        {
            m_codeText = encodeTrack( m_lab );
            std::erase( m_codeText, '-' );
            m_state = StateE::CODE_ENTRY;
        }
    }
}

void Game::shutdown()
{
    saveRecords();
    if( m_postcardJob.valid() ) m_postcardJob.wait();
    unloadPostcards();
    if( m_world.m_buildCount > 0 )
        TraceLog( LOG_INFO, "SUNSTREAK: %d chunks built, %.2f ms on average, %.2f ms the slowest, %d on the main thread, frame part peak %.2f ms",
                  m_world.m_buildCount, m_world.m_buildTotalMs / m_world.m_buildCount, m_world.m_buildMaxMs, m_world.m_syncBuilds,
                  m_world.m_buildPeakMs );
    m_audio.stopMusic();
    m_audio.shutdown();
    m_world.clear();
    m_renderer.unload();
    m_models.unload();
}

void Game::saveRecords() const
{
    m_records.save( configDirectory() + "/records.txt" );
}

float Game::frontAt( double s, WeatherE &type ) const
{
    type = WeatherE::NONE;
    if( m_options.m_weather >= 0 )
    {
        type = static_cast<WeatherE>( m_options.m_weather );
        return type == WeatherE::NONE ? 0.0f : 1.0f;
    }
    if( m_settings.m_weather != 0 || !m_track ) return 0.0f;
    const long stage = static_cast<long>( s / StageLength );
    float      best = 0.0f;
    for( long st = std::max( 0L, stage - 1 ); st <= stage + 1; ++st )
    {
        const uint64_t h = hash64( m_seed * 977 + static_cast<uint64_t>( st ) * 131 + 17 );
        if( h % 100 >= 34 ) continue;
        const ClimateE climate = biomeDef( m_track->stageBiome( static_cast<int>( st ) ) ).m_climate;
        if( climate == ClimateE::NONE ) continue;
        const double centre = ( static_cast<double>( st ) + 0.25 + 0.5 * static_cast<double>( ( h >> 8 ) % 1000 ) / 1000.0 ) * StageLength;
        const double half = 700.0 + static_cast<double>( ( h >> 20 ) % 900 );
        const float  k = 1.0f - smoothstepf( static_cast<float>( half * 0.5 ), static_cast<float>( half ), static_cast<float>( std::abs( s - centre ) ) );
        if( k <= best ) continue;
        best = k;
        static const WeatherE temperate[4] = { WeatherE::RAIN, WeatherE::RAIN, WeatherE::FOG, WeatherE::STORM };
        switch( climate )
        {
        case ClimateE::ARID: type = WeatherE::SANDSTORM; break;
        case ClimateE::COLD: type = WeatherE::SNOW; break;
        case ClimateE::TROPICAL: type = ( h >> 32 ) % 3 == 0 ? WeatherE::RAIN : WeatherE::STORM; break;
        default: type = temperate[( h >> 32 ) % 4]; break;
        }
    }
    return best;
}

float Game::wantedHour() const
{
    static const float fixed[4] = { 6.6f, 12.5f, 19.6f, 23.0f };
    if( m_settings.m_timeOfDay >= 2 ) return fixed[std::clamp( m_settings.m_timeOfDay - 2, 0, 3 )];
    if( m_settings.m_timeOfDay != 1 || !m_track ) return -1.0f;
    // Local solar time: the universal hour (an hour per ten kilometres) plus the place's longitude.
    GeoPoint where;
    if( !placeLocation( m_track->biomeMix( m_car.m_s ).m_from, where ) ) return -1.0f;
    const double utc = m_clockStart + ( m_car.m_s - m_startS ) / 10000.0;
    return static_cast<float>( std::fmod( utc + where.m_lon / 15.0 + 48.0, 24.0 ) );
}

Environment Game::sceneEnvironment( double s ) const
{
    Environment e = environmentAt( *m_track, s );
    if( m_hour >= 0.0f ) e = environmentAtHour( e, m_hour );
    WeatherE    type = WeatherE::NONE;
    const float k = frontAt( s, type );
    if( k <= 0.0f ) return e;
    auto toward = [k]( Color a, Color b, float amount ) { return lerpColor( a, b, k * amount ); };
    switch( type )
    {
    case WeatherE::RAIN:
    case WeatherE::STORM:
    {
        const bool storm = type == WeatherE::STORM;
        e.m_rain = std::max( e.m_rain, k * ( storm ? 0.95f : 0.7f ) );
        e.m_lightning = std::max( e.m_lightning, storm ? k * 0.6f : 0.0f );
        e.m_cloudAmount = std::max( e.m_cloudAmount, 0.6f + 0.4f * k );
        e.m_cloudColor = toward( e.m_cloudColor, Color{ 110, 116, 126, 255 }, storm ? 0.9f : 0.6f );
        e.m_sunIntensity *= 1.0f - 0.45f * k;
        e.m_skyZenith = toward( e.m_skyZenith, Color{ 96, 104, 116, 255 }, storm ? 0.8f : 0.55f );
        e.m_skyHorizon = toward( e.m_skyHorizon, Color{ 150, 156, 164, 255 }, 0.6f );
        e.m_fogColor = toward( e.m_fogColor, Color{ 150, 156, 164, 255 }, 0.6f );
        e.m_fogDensity *= 1.0f + 0.9f * k;
        e.m_headlights = std::max( e.m_headlights, 0.35f * k );
        e.m_saturation = lerpf( e.m_saturation, 0.9f, k );
        break;
    }
    case WeatherE::FOG:
        e.m_fogColor = toward( e.m_fogColor, Color{ 206, 212, 218, 255 }, 0.8f );
        e.m_skyHorizon = toward( e.m_skyHorizon, Color{ 206, 212, 218, 255 }, 0.8f );
        e.m_fogDensity = lerpf( e.m_fogDensity, 0.009f, k );
        e.m_sunIntensity *= 1.0f - 0.35f * k;
        e.m_headlights = std::max( e.m_headlights, 0.5f * k );
        break;
    case WeatherE::SNOW:
        e.m_snow = std::max( e.m_snow, 0.85f * k );
        e.m_cloudAmount = std::max( e.m_cloudAmount, 0.7f + 0.3f * k );
        e.m_cloudColor = toward( e.m_cloudColor, Color{ 214, 218, 226, 255 }, 0.8f );
        e.m_skyZenith = toward( e.m_skyZenith, Color{ 170, 178, 192, 255 }, 0.7f );
        e.m_skyHorizon = toward( e.m_skyHorizon, Color{ 214, 220, 230, 255 }, 0.7f );
        e.m_fogColor = toward( e.m_fogColor, Color{ 214, 220, 230, 255 }, 0.7f );
        e.m_fogDensity *= 1.0f + 1.6f * k;
        e.m_sunIntensity *= 1.0f - 0.4f * k;
        break;
    case WeatherE::SANDSTORM:
    {
        const Color sand = { 206, 160, 104, 255 };
        e.m_dust = std::max( e.m_dust, k );
        e.m_fogColor = toward( e.m_fogColor, sand, 0.9f );
        e.m_skyHorizon = toward( e.m_skyHorizon, sand, 0.9f );
        e.m_skyZenith = toward( e.m_skyZenith, Color{ 170, 140, 104, 255 }, 0.7f );
        e.m_ambientTop = toward( e.m_ambientTop, sand, 0.5f );
        e.m_sunColor = toward( e.m_sunColor, Color{ 255, 190, 120, 255 }, 0.6f );
        e.m_fogDensity = lerpf( e.m_fogDensity, 0.0075f, k );
        e.m_sunIntensity *= 1.0f - 0.5f * k;
        e.m_cloudAmount *= 1.0f - 0.7f * k;
        e.m_headlights = std::max( e.m_headlights, 0.45f * k );
        break;
    }
    default: break;
    }
    return e;
}

void Game::applyLivery()
{
    m_settings.m_car = m_carIndex;
    m_settings.m_paint = m_paintIndex;
    m_settings.m_rims = std::clamp( m_settings.m_rims, 0, RimCount - 1 );
    m_settings.m_decal = std::clamp( m_settings.m_decal, 0, DecalCount - 1 );
    m_settings.m_decalColor = std::clamp( m_settings.m_decalColor, 0, PaintCount - 1 );
    m_models.buildLivery( Livery{ m_carIndex, m_paintIndex, m_settings.m_rims, m_settings.m_decal, m_settings.m_decalColor } );
}

void Game::earn( int64_t points )
{
    if( points <= 0 || m_demo ) return;
    m_earned += points;
    m_records.m_points += points;
}

void Game::takePostcard( BiomeE biome )
{
    m_postcardWanted = -1;
    m_records.m_postcards.set( static_cast<size_t>( biome ) );
    // Read what the renderer drew (the HUD comes after); scale and save it on a worker.
    rlDrawRenderBatchActive();
    const int         w = GetRenderWidth(), h = GetRenderHeight();
    Image             img = { rlReadScreenPixels( w, h ), w, h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    const std::string dir = configDirectory() + "/postcards";
    const std::string file = dir + "/" + std::to_string( static_cast<int>( biome ) ) + ".png";
    if( m_postcardJob.valid() ) m_postcardJob.wait();
    m_postcardJob = std::async( std::launch::async,
                                [img, dir, file]() mutable
                                {
                                    const int ch = std::min( img.height, img.width * 9 / 16 ), cw = ch * 16 / 9;
                                    ImageCrop( &img, Rectangle{ static_cast<float>( img.width - cw ) * 0.5f, static_cast<float>( img.height - ch ) * 0.5f,
                                                                static_cast<float>( cw ), static_cast<float>( ch ) } );
                                    ImageResize( &img, 480, 270 );
                                    std::error_code ec;
                                    std::filesystem::create_directories( dir, ec );
                                    ExportImage( img, file.c_str() );
                                    UnloadImage( img );
                                } );
    const auto old = m_postcardTex.find( static_cast<int>( biome ) );
    if( old != m_postcardTex.end() )
    {
        UnloadTexture( old->second );
        m_postcardTex.erase( old );
    }
    popup( "POSTCARD", placeName( biome ), Cyan, 0.7f, 2.2f );
}

void Game::unloadPostcards()
{
    for( auto &[biome, tex] : m_postcardTex ) UnloadTexture( tex );
    m_postcardTex.clear();
}

void Game::setupTrack( uint64_t seed, double startS, const TrackParams &params )
{
    m_seed = seed;
    m_world.clear();
    m_track = std::make_unique<Track>( seed, params, m_options.m_startBiome );
    m_track->generateTo( startS + ViewDistance + 600.0 );
    const Segment &seg = m_track->segment( static_cast<long>( startS / SegmentLength ) );
    // Start in the slow lane's neighbour: right of the middle, or left of it where they drive on the left.
    const bool keepLeft = countryInfo( biomeDef( m_track->stageBiome( static_cast<int>( startS / StageLength ) ) ).m_country ).m_left;
    m_car.reset( carSpec( m_carIndex ), startS, laneCenter( keepLeft ? ( seg.m_lanes - 1 ) / 2 : seg.m_lanes / 2, seg.m_lanes, seg.m_halfWidth ), *m_track );
    m_traffic.reset( seed, startS );
    m_traffic.m_density = params.m_traffic < 0 ? 1.0f : static_cast<float>( params.m_traffic ) / 4.5f;
    m_race.m_rivals.clear();
    m_particles.clear();
    m_world.update( *m_track, startS, 1000 );
    m_camInit = false;
    m_popups.clear();
    m_lastS = startS;
    m_startS = startS;
}

TrackParams Game::endlessParams() const
{
    TrackParams p;
    p.m_biome = m_settings.m_scenery ? SceneryMixed : SceneryWorld;
    if( m_options.m_scenery >= SceneryWorld && m_options.m_scenery < static_cast<int>( BiomeE::COUNT ) ) p.m_biome = m_options.m_scenery;
    return p;
}

void Game::startDemo()
{
    m_demo = true;
    m_mode = ModeE::ENDLESS;
    m_setup = TrackSetup();
    m_state = StateE::TITLE;
    m_stateTime = 0.0f;
    m_menuIndex = 0;
    const uint64_t seed = randomSeed();
    m_setup.m_seed = seed;
    m_setup.m_params = endlessParams();
    setupTrack( seed, 600.0, m_setup.m_params );
    m_car.m_speed = 42.0f;
    m_car.m_gear = 3;
    m_musicStage = static_cast<int>( m_car.m_s / StageLength );
    playMusic( hash64( seed ), wantedStation() );
}

void Game::startTrack( const TrackSetup &setup, ModeE mode )
{
    m_demo = false;
    m_mode = mode;
    m_setup = setup;
    m_code = encodeTrack( setup );
    TrackParams params = setup.m_params;
    if( setup.m_race )
    {
        params.m_startLine = 100.0;
        if( params.m_traffic < 0 ) params.m_traffic = 0;
        if( params.m_length <= 0.0 ) params.m_length = 10000.0;
        m_setup.m_params = params;
    }
    double startS = params.m_startLine - 9.0;
    if( m_options.m_startDistance > 0.0f )
    {
        startS = m_options.m_startDistance;
        m_options.m_startDistance = 0.0f;
    }
    if( m_options.m_find == 3 )
    {
        // Testing: start shortly before the first stage's first landmark.
        Track           probe( setup.m_seed, params, m_options.m_startBiome );
        const BiomeDef &def = biomeDef( probe.stageBiome( 0 ) );
        for( size_t r = 0; r < def.m_props.size(); ++r )
            if( def.m_props[r].m_perStage > 0 )
            {
                bool       right = false;
                const long local = landmarkSpot( setup.m_seed, r, 0, 0, right );
                startS = std::max( params.m_startLine - 9.0, static_cast<double>( local ) * SegmentLength - 260.0 );
                break;
            }
        m_options.m_find = 0;
    }
    if( m_options.m_find != 0 )
    {
        // Testing: start shortly before the first tunnel or bridge.
        Track          probe( setup.m_seed, params, m_options.m_startBiome );
        const uint16_t want = m_options.m_find == 1 ? SEG_TUNNEL : SEG_BRIDGE;
        for( double s = 0.0; s < 60000.0; s += 1000.0 )
        {
            probe.generateTo( s + 1000.0 );
            for( long i = static_cast<long>( s / SegmentLength ); i < static_cast<long>( ( s + 1000.0 ) / SegmentLength ); ++i )
                if( probe.segment( i ).m_flags & want )
                {
                    startS = std::max( params.m_startLine - 9.0, static_cast<double>( i ) * SegmentLength - 160.0 );
                    s = 1e9;
                    break;
                }
            probe.discardBefore( s - 2000.0 );
        }
        m_options.m_find = 0;
    }
    setupTrack( setup.m_seed, startS, params );
    if( setup.m_race )
    {
        m_race.newChampionship( setup.m_seed );
        m_race.grid( *m_track, m_car, tuning().m_rivals );
    }
    beginCountdown();
    if( setup.m_race )
        popup( "RACE", placeName( m_track->stageBiome( 0 ) ), Accent, 1.0f, 3.0f );
    else
        popup( format( "STAGE %d", m_stage + 1 ), placeName( m_track->stageBiome( m_stage ) ), Accent, 1.0f, 3.0f );
}

void Game::startChampionship()
{
    m_race.newChampionship( randomSeed() );
    startRound( 0 );
}

void Game::startRound( int round )
{
    m_demo = false;
    m_mode = ModeE::CHAMPIONSHIP;
    m_race.m_round = round;
    TrackSetup setup;
    setup.m_seed = m_race.roundSeed( round );
    setup.m_params = m_race.roundParams( round );
    setup.m_race = true;
    m_setup = setup;
    m_code = encodeTrack( setup );
    setupTrack( setup.m_seed, setup.m_params.m_startLine - 9.0, setup.m_params );
    m_race.grid( *m_track, m_car, tuning().m_rivals );
    beginCountdown();
    popup( format( "RACE %d OF %d", round + 1, RoundCount ), placeName( m_track->stageBiome( 0 ) ), Accent, 1.0f, 3.0f );
}

void Game::restart()
{
    if( m_mode == ModeE::CHAMPIONSHIP ) startRound( m_race.m_round );
    else startTrack( m_setup, m_mode );
}

void Game::beginCountdown()
{
    m_car.m_frozen = true;
    m_timeLeft = StartTime * tuning().m_time;
    m_timeUp = false;
    m_timeUpFor = 0.0f;
    m_score = 0.0;
    m_checkpoints = static_cast<int>( m_car.m_s / CheckpointSpacing );
    m_nextCheckpoint = ( m_checkpoints + 1 ) * CheckpointSpacing;
    m_stage = static_cast<int>( m_car.m_s / StageLength );
    m_overtakes = 0;
    m_nearMisses = 0;
    m_combo = 1;
    m_comboTimer = 0.0f;
    m_topSpeed = 0.0f;
    m_newBest = false;
    m_newGhost = false;
    m_rank = -1;
    m_finishedTrack = false;
    m_lastWarning = 99;
    m_lastCount = 4;
    m_raceTime = 0.0f;
    m_finished = false;
    m_finishedFor = 0.0f;
    m_place = 0;
    m_results.clear();
    m_resultPage = 0;
    m_ghost.load( ghostDir(), m_code );
    m_ghost.startRecording();
    m_earned = 0;
    m_postcardWanted = -1;
    m_frontShown = -1;
    // The tour clock: the morning, give or take, wherever the run starts.
    GeoPoint where;
    m_clockStart = placeLocation( m_track->biomeMix( m_car.m_s ).m_from, where )
                       ? 7.5 + static_cast<double>( hash64( m_seed + 3 ) % 40 ) * 0.1 - static_cast<double>( where.m_lon ) / 15.0
                       : 9.0;
    m_hour = -1.0f;
    m_state = StateE::COUNTDOWN;
    m_stateTime = 0.0f;
    m_musicStage = m_stage;
    playMusic( hash64( m_seed + 7 ), wantedStation() );
}

void Game::popup( const std::string &text, const std::string &sub, Color color, float size, float life )
{
    if( m_popups.size() >= 4 ) m_popups.erase( m_popups.begin() );
    Popup p;
    p.m_text = text;
    p.m_sub = sub;
    p.m_color = color;
    p.m_size = size;
    p.m_life = life;
    m_popups.push_back( p );
}

void Game::toggleFullscreen()
{
    ToggleBorderlessWindowed();
    m_settings.m_display = IsWindowState( FLAG_BORDERLESS_WINDOWED_MODE ) ? DisplayModeE::FULLSCREEN : DisplayModeE::WINDOWED;
    settingsChanged();
}

int Game::wantedStation() const
{
    if( m_settings.m_station >= StationCount ) return m_audio.hasFileMusic() ? StationCount : 0;
    if( m_settings.m_station >= 0 ) return m_settings.m_station;
    if( !m_track ) return 0;
    return biomeDef( m_track->stageBiome( static_cast<int>( m_car.m_s / StageLength ) ) ).m_station;
}

void Game::playMusic( uint64_t seed, int station )
{
    m_audio.startMusic( seed, station );
    m_playingStation = station;
    m_trackSeed = seed;
    m_nowTimer = 0.0f;
    m_nowPlaying = station >= StationCount ? std::string( "MY MUSIC" ) : std::string( stationName( station ) ) + "  -  " + trackTitle( seed );
}

std::string Game::radioName() const
{
    if( m_settings.m_station < 0 ) return "AUTO";
    if( m_settings.m_station >= StationCount ) return "MY MUSIC";
    return stationName( m_settings.m_station );
}

void Game::nextStation()
{
    const int last = m_audio.hasFileMusic() ? StationCount : StationCount - 1;
    m_settings.m_station = m_settings.m_station >= last ? -1 : m_settings.m_station + 1;
    settingsChanged();
    playMusic( hash64( m_trackSeed + 101 ), wantedStation() );
}

void Game::settingsChanged()
{
    m_audio.setVolumes( m_settings.m_musicVolume, m_settings.m_sfxVolume );
    saveSettings( m_settings );
}

void Game::progress( double ds )
{
    if( ds <= 0.0 || ds > 500.0 ) return;
    bool cars[CarCount], paints[PaintCount];
    for( int i = 0; i < CarCount; ++i ) cars[i] = m_records.carUnlocked( i );
    for( int i = 0; i < PaintCount; ++i ) paints[i] = m_records.paintUnlocked( i );
    m_records.m_totalKm += ds / 1000.0;
    const BiomeMix mix = m_track->biomeMix( m_car.m_s );
    const BiomeE   here = mix.m_t < 0.5f ? mix.m_from : mix.m_to;
    const size_t   bit = static_cast<size_t>( here );
    if( !m_records.m_seen.test( bit ) )
    {
        // A new place: points, and a stamp for a new country.
        const int  countries = m_records.countriesSeen();
        const bool round = m_records.circumnavigated();
        const bool globe = m_records.decalUnlocked( DecalCount - 1 );
        m_records.m_seen.set( bit );
        earn( 2000 );
        if( m_records.countriesSeen() > countries )
        {
            earn( 5000 );
            const char *country = biomeDef( here ).m_country;
            popup( "NEW STAMP", country ? country : "", Gold, 0.8f, 2.6f );
        }
        if( !round && m_records.circumnavigated() ) popup( "ROUND THE WORLD", "NEW PAINT  GLOBETROTTER", Gold, 1.2f, 4.5f );
        if( !globe && m_records.decalUnlocked( DecalCount - 1 ) ) popup( "NEW DECAL", "GLOBE", Gold, 0.9f, 3.0f );
    }
    if( !m_records.m_postcards.test( bit ) && m_postcardWanted != static_cast<int>( here ) && mix.m_from == mix.m_to )
    {
        // A postcard of a new place: in sight of its landmark, or a stretch into its stage.
        const long      stage = static_cast<long>( m_car.m_s / StageLength );
        const BiomeDef &def = biomeDef( here );
        double          at = static_cast<double>( stage ) * StageLength + 1400.0;
        for( size_t r = 0; r < def.m_props.size(); ++r )
            if( def.m_props[r].m_perStage > 0 )
            {
                bool          right = false;
                const PropRule &pr = def.m_props[r];
                const double  spot = ( static_cast<double>( stage ) * ( StageLength / SegmentLength ) + landmarkSpot( m_seed, r, 0, stage, right ) ) * SegmentLength;
                at = spot - ( ( pr.m_nearest + pr.m_farthest ) * 0.75 + 40.0 );
                break;
            }
        m_postcardWanted = static_cast<int>( here );
        m_postcardAt = std::max( at, m_car.m_s + 60.0 );
    }
    for( int i = 0; i < CarCount; ++i )
        if( !cars[i] && m_records.carUnlocked( i ) ) popup( "NEW CAR", carSpec( i ).m_name, Gold, 0.9f, 3.0f );
    for( int i = 0; i < PaintCount; ++i )
        if( !paints[i] && m_records.paintUnlocked( i ) ) popup( "NEW PAINT", paintName( i ), Gold, 0.8f, 3.0f );
}

void Game::frame( float dt )
{
    m_dt = dt;
    m_time += dt;
    m_stateTime += dt;
    m_input.update();
    if( m_input.m_fullscreen ) toggleFullscreen();
    if( m_input.m_stats ) m_showStats = !m_showStats;
    m_frameMsPeak = std::max( dt * 1000.0f, m_frameMsPeak * 0.995f );
    const MenuInput mi = m_input.menu();

    switch( m_state )
    {
    case StateE::TITLE:
    case StateE::LAB:
    case StateE::CODE_ENTRY:
    case StateE::RECORDS:
    case StateE::GARAGE:
    case StateE::PASSPORT:
    {
        const StateE before = m_state;
        menus( mi );
        if( m_state == before || m_state == StateE::SETTINGS || m_state == StateE::LAB || m_state == StateE::TITLE ||
            m_state == StateE::RECORDS || m_state == StateE::CODE_ENTRY || m_state == StateE::GARAGE || m_state == StateE::PASSPORT )
            if( m_demo ) simulate( dt, true );
        break;
    }
    case StateE::SETTINGS:
        menus( mi );
        if( m_demo ) simulate( dt, true );
        break;
    case StateE::COUNTDOWN:
    {
        if( m_input.m_pause )
        {
            m_pauseReturn = StateE::COUNTDOWN;
            m_state = StateE::PAUSED;
            m_menuIndex = 0;
            break;
        }
        const int n = 3 - static_cast<int>( m_stateTime );
        if( n != m_lastCount && n >= 1 && n <= 3 )
        {
            m_lastCount = n;
            m_audio.play( SfxE::BEEP );
        }
        if( m_stateTime >= 3.0f )
        {
            m_state = StateE::RACE;
            m_stateTime = 0.0f;
            m_car.m_frozen = false;
            m_audio.play( SfxE::GO );
            popup( "GO!", "", Green, 1.6f, 1.2f );
        }
        simulate( dt, m_options.m_autopilot );
        break;
    }
    case StateE::RACE:
        if( m_input.m_pause )
        {
            m_pauseReturn = StateE::RACE;
            m_state = StateE::PAUSED;
            m_menuIndex = 0;
            m_audio.play( SfxE::TICK );
            break;
        }
        simulate( dt, m_options.m_autopilot || m_finished || m_finishedTrack );
        if( m_setup.m_race )
        {
            if( m_finished && m_finishedFor > 2.5f )
            {
                m_state = StateE::RESULTS;
                m_stateTime = 0.0f;
                m_menuIndex = 0;
                m_resultPage = 0;
            }
        }
        else if( ( m_timeUp && ( m_car.m_speed < 0.5f || m_timeUpFor > 5.0f ) ) || ( m_finishedTrack && m_finishedFor > 2.5f ) )
        {
            endTimeAttack();
        }
        break;
    case StateE::PAUSED: menus( mi ); break;
    case StateE::GAMEOVER:
    case StateE::RESULTS:
    case StateE::INITIALS:
    {
        const StateE before = m_state;
        menus( mi );
        if( m_state == before ) simulate( dt, true );
        break;
    }
    }
    if( m_input.m_nextStation ) nextStation();
    if( m_input.m_nextTrack && m_state != StateE::CODE_ENTRY && m_state != StateE::INITIALS )
        playMusic( hash64( m_trackSeed + 31 ), m_playingStation );
    m_nowTimer += dt;
    if( m_input.m_camera && ( m_state == StateE::RACE || m_state == StateE::COUNTDOWN ) )
    {
        m_settings.m_camera = ( m_settings.m_camera + 1 ) % 3;
        settingsChanged();
    }
    for( Popup &p : m_popups ) p.m_age += dt;
    std::erase_if( m_popups, []( const Popup &p ) { return p.m_age > p.m_life; } );
    updateAudio();
    m_audio.update();
    m_haptics.update( dt, m_settings.m_vibration && !m_demo && m_state == StateE::RACE );
    draw();
}

void Game::endTimeAttack()
{
    const auto   score = static_cast<int64_t>( m_score );
    const double dist = m_car.m_s - m_startS;
    m_newBest = score > m_settings.m_bestScore;
    m_settings.m_bestScore = std::max( m_settings.m_bestScore, score );
    m_settings.m_bestDistance = std::max( m_settings.m_bestDistance, dist );
    m_newGhost = m_ghost.saveIfBest( ghostDir(), m_code, static_cast<double>( score ), m_carIndex, m_paintIndex );
    m_rank = m_mode == ModeE::ENDLESS ? m_records.rankOf( score ) : -1;
    earn( score );
    saveSettings( m_settings );
    saveRecords();
    m_state = m_rank >= 0 ? StateE::INITIALS : StateE::GAMEOVER;
    m_initialCursor = 0;
    m_stateTime = 0.0f;
    m_menuIndex = 0;
}

void Game::crossFinish()
{
    m_finished = true;
    m_finishedFor = 0.0f;
    m_results = m_race.finish( m_raceTime );
    for( size_t i = 0; i < m_results.size(); ++i )
        if( m_results[i].m_driver < 0 ) m_place = static_cast<int>( i ) + 1;
    m_newGhost = m_ghost.saveIfBest( ghostDir(), m_code, -static_cast<double>( m_raceTime ), m_carIndex, m_paintIndex );
    static const int64_t byPlace[8] = { 12000, 9000, 7000, 5500, 4500, 3500, 2500, 1500 };
    earn( m_place >= 1 && m_place <= 8 ? byPlace[m_place - 1] : 800 );
    popup( m_place == 1 ? "YOU WIN!" : format( "P%d", m_place ), "FINISH  " + clockText( m_raceTime ), m_place <= 3 ? Gold : Ink, 1.4f, 3.5f );
    m_audio.playOnBeat( m_place <= 3 ? SfxE::VICTORY : SfxE::STAGE );
    saveRecords();
}

void Game::simulate( float dt, bool autopilotOn )
{
    Track &track = *m_track;
    track.generateTo( m_car.m_s + ViewDistance + 600.0 );
    track.discardBefore( m_car.m_s - 400.0 );
    m_world.update( track, m_car.m_s, 1 );   // one 80 m chunk a frame outruns any car; spikes stay one chunk long

    m_ev = TrafficEvents{};
    m_raceEv = RaceEvents{};
    const double before = m_car.m_s;
    m_crash = 0.0f;
    m_scraping = false;
    m_landed = false;
    m_landImpact = 0.0f;
    m_shifted = false;
    m_backfire = false;
    CarInput in = autopilotOn ? autopilot() : m_input.car();
    if( !autopilotOn && !m_demo && m_state == StateE::RACE && ( m_settings.m_steerAssist > 0 || m_settings.m_brakeAssist ) )
    {
        // The assists borrow the demo driver: steering where the hands are idle, braking for what comes too fast.
        const CarInput ai = autopilot();
        if( m_settings.m_steerAssist > 0 )
        {
            const float k = m_settings.m_steerAssist == 1 ? 0.35f : 0.7f;
            const float idle = 1.0f - std::min( 1.0f, std::abs( in.m_steer ) * 2.0f );
            in.m_steer = clampf( in.m_steer + ai.m_steer * k * idle, -1.0f, 1.0f );
        }
        if( m_settings.m_brakeAssist && ai.m_brake > 0.3f && in.m_throttle > 0.0f )
        {
            in.m_brake = std::max( in.m_brake, ai.m_brake * 0.85f );
            in.m_throttle = std::min( in.m_throttle, 1.0f - ai.m_brake );
        }
    }
    float    remaining = dt;
    while( remaining > 1e-6f )
    {
        const float h = std::min( remaining, PhysicsStep );
        step( h, in );
        in.m_shiftUp = false;
        in.m_shiftDown = false;
        remaining -= h;
    }
    handleEvents();
    if( !m_demo ) progress( m_car.m_s - before );
    const int musicStage = static_cast<int>( m_car.m_s / StageLength );
    if( musicStage != m_musicStage )
    {
        m_musicStage = musicStage;
        if( m_settings.m_station < 0 && wantedStation() != m_playingStation ) playMusic( hash64( m_seed * 31 + musicStage ), wantedStation() );
    }
    emitParticles( dt );
    m_particles.update( dt );
    updateCamera( dt );

    // The hour the sky shows follows the time zones smoothly; fronts are announced as they come.
    const float hour = wantedHour();
    if( hour < 0.0f ) m_hour = -1.0f;
    else if( m_hour < 0.0f ) m_hour = hour;
    else
    {
        const float d = std::remainder( hour - m_hour, 24.0f );
        m_hour = std::fmod( m_hour + d * std::min( 1.0f, dt * 0.25f ) + 24.0f, 24.0f );
    }
    WeatherE    front = WeatherE::NONE;
    const float frontAhead = frontAt( m_car.m_s + 450.0, front );
    const long  frontStage = static_cast<long>( ( m_car.m_s + 450.0 ) / StageLength );
    if( !m_demo && frontAhead > 0.3f && frontStage != m_frontShown && m_state == StateE::RACE )
    {
        m_frontShown = frontStage;
        static const char *names[] = { "", "RAIN AHEAD", "THUNDERSTORM", "FOG BANK", "SNOW SHOWERS", "SANDSTORM" };
        popup( names[static_cast<int>( front )], "", Cyan, 0.7f, 2.2f );
    }
    WeatherE    here = WeatherE::NONE;
    const float wet = frontAt( m_car.m_s, here );
    const float lose = here == WeatherE::SNOW ? 0.22f : here == WeatherE::SANDSTORM ? 0.08f : here == WeatherE::FOG || here == WeatherE::NONE ? 0.0f : 0.12f;
    m_car.m_weatherGrip = 1.0f - lose * wet;

    const Environment env = sceneEnvironment( m_car.m_s + 30.0 );
    Rng              &r = m_particles.m_rng;
    if( env.m_lightning > 0.05f && r.uniform() < dt * 0.12f * env.m_lightning )
    {
        m_flash = 1.0f;
        m_thunderIn = r.range( 0.4f, 1.8f );
    }
    m_flash = approach( m_flash, 0.0f, 7.0f, dt );
    if( m_thunderIn > 0.0f )
    {
        m_thunderIn -= dt;
        if( m_thunderIn <= 0.0f )
        {
            m_audio.play( SfxE::CRASH, 0.4f, r.range( -0.6f, 0.6f ) );
            m_thunderIn = -1.0f;
        }
    }
}

void Game::step( float h, const CarInput &in )
{
    Track     &track = *m_track;
    const bool manual = m_settings.m_manualGears && !m_demo && !m_options.m_autopilot;
    m_car.update( in, track, h, manual );
    m_shifted = m_shifted || m_car.m_eventShift;
    m_backfire = m_backfire || m_car.m_eventBackfire;
    if( m_car.m_eventLanded )
    {
        m_landed = true;
        m_landImpact = std::max( m_landImpact, m_car.m_landImpact );
    }
    m_traffic.update( h, track, m_car, m_models, m_ev );
    const bool racing = m_setup.m_race && !m_demo;
    if( racing ) m_race.update( h, m_raceTime, track, m_car, m_raceEv, m_state == StateE::RACE );

    // Solid things beside the road.
    const double height = m_car.m_y - track.surfaceHeight( m_car.m_s, m_car.m_x );
    if( m_car.m_crashTimer <= 0.0f && height < 1.5 )
    {
        m_colliders.clear();
        m_world.collidersNear( m_car.m_s, 10.0, m_colliders );
        for( const Collider *c : m_colliders )
        {
            const double ds = m_car.m_s - c->m_s;
            const float  dx = m_car.m_x - c->m_x;
            const float  rl = m_car.m_spec.m_length * 0.5f + c->m_radius * 0.8f;
            const float  rw = m_car.m_spec.m_width * 0.5f + c->m_radius * 0.8f;
            if( std::abs( ds ) < rl && std::abs( dx ) < rw )
            {
                const float toRoad = c->m_x > 0.0f ? -1.0f : 1.0f;
                m_car.crash( tuning().m_crashKeep, c->m_x + toRoad * ( rw + 0.3f ) - m_car.m_x );
                m_crash = std::max( m_crash, clampf( m_car.m_speed / 12.0f, 0.3f, 1.0f ) );
                break;
            }
        }
    }

    // Guard rails.
    const Segment   &seg = track.segment( static_cast<long>( m_car.m_s / SegmentLength ) );
    const TrackFrame f = track.frameAt( m_car.m_s );
    const float      railX = f.m_halfWidth + RumbleWidth + 0.45f - m_car.m_spec.m_width * 0.5f;
    for( float side : { -1.0f, 1.0f } )
    {
        if( !( seg.m_flags & ( side < 0 ? SEG_RAIL_L : SEG_RAIL_R ) ) || m_car.m_x * side <= railX ) continue;
        const float into = ( m_car.m_vx - m_car.m_drift ) * side;
        m_car.m_x = side * railX;
        if( into > 0.0f ) m_car.m_vx = m_car.m_drift - side * into * 0.35f;
        if( !m_wasScraping && into > 3.0f )
        {
            m_crash = std::max( m_crash, std::min( 1.0f, into / 15.0f ) * 0.6f );
            m_car.m_speed *= 0.95f;
        }
        m_car.m_speed *= std::exp( -0.35f * h );
        m_scraping = true;
        m_scrapeSide = side;
    }

    if( m_state != StateE::RACE || m_demo ) return;
    m_raceTime += h;
    if( !m_finished && !m_finishedTrack ) m_ghost.record( m_raceTime, m_car );
    m_topSpeed = std::max( m_topSpeed, m_car.m_speed );
    if( racing )
    {
        if( !m_finished && m_race.m_finishS > 0.0 && m_car.m_s >= m_race.m_finishS ) crossFinish();
        if( m_finished ) m_finishedFor += h;
        return;
    }
    timeAttackRules( h );
}

void Game::timeAttackRules( float h )
{
    const Track &track = *m_track;
    if( m_finishedTrack )
    {
        m_finishedFor += h;
        return;
    }
    const double length = m_setup.m_params.m_length;
    if( length > 0.0 && m_car.m_s >= length )
    {
        m_finishedTrack = true;
        m_finishedFor = 0.0f;
        m_score += static_cast<double>( m_timeLeft ) * 100.0;
        m_car.m_coasting = true;
        popup( "FINISH", format( "%.1f SEC LEFT  +%d", m_timeLeft, static_cast<int>( m_timeLeft * 100.0f ) ), Gold, 1.4f, 3.5f );
        m_audio.playOnBeat( SfxE::CHECKPOINT );
        return;
    }
    if( !m_timeUp )
    {
        m_timeLeft -= h;
        if( m_timeLeft <= 0.0f )
        {
            m_timeLeft = 0.0f;
            m_timeUp = true;
            m_car.m_coasting = true;
            popup( "TIME UP", "", Red, 1.5f, 4.0f );
            m_audio.play( SfxE::GAMEOVER );
        }
    }
    else
    {
        m_timeUpFor += h;
    }
    while( m_car.m_s >= m_nextCheckpoint )
    {
        ++m_checkpoints;
        if( !m_timeUp )
        {
            const float bonus = checkpointBonus( m_checkpoints );
            m_timeLeft += bonus;
            m_score += 1000.0;
            popup( "CHECKPOINT", format( "+%.1f SEC", bonus ), Green, 1.1f, 2.4f );
            m_audio.playOnBeat( SfxE::CHECKPOINT );
        }
        m_nextCheckpoint += CheckpointSpacing;
    }
    const int stage = static_cast<int>( m_car.m_s / StageLength );
    if( stage != m_stage )
    {
        m_stage = stage;
        popup( format( "STAGE %d", stage + 1 ), placeName( track.stageBiome( stage ) ), Accent, 1.2f, 3.2f );
        m_audio.playOnBeat( SfxE::STAGE );
    }
    const double ds = m_car.m_s - m_lastS;
    m_lastS = m_car.m_s;
    const float sn = m_car.m_speed / m_car.m_spec.m_topSpeed;
    if( !m_timeUp ) m_score += ds * ( 0.5 + 1.5 * static_cast<double>( sn * sn ) ) * m_combo;
    m_comboTimer -= h;
    if( m_comboTimer <= 0.0f ) m_combo = 1;
}

CarInput Game::autopilot() const
{
    const Track   &track = *m_track;
    const Segment &seg = track.segment( static_cast<long>( m_car.m_s / SegmentLength ) );
    const int      lanes = seg.m_lanes;
    const int      current = laneOf( m_car.m_x, lanes, seg.m_halfWidth );

    // Pick the lane with the most room ahead.
    int   best = current;
    float bestCost = 1e9f;
    for( int lane = 0; lane < lanes; ++lane )
    {
        float cost = std::abs( static_cast<float>( lane - current ) ) * 6.0f + std::abs( static_cast<float>( lane ) - ( lanes - 1 ) * 0.5f );
        for( long i = 0; i < 30; ++i )
            if( track.segment( static_cast<long>( m_car.m_s / SegmentLength ) + i ).m_blockedLanes & ( 1u << lane ) ) cost += 200.0f;
        for( const TrafficCar &c : m_traffic.m_cars )
        {
            const double gap = c.m_s - m_car.m_s;
            if( gap < -6.0 || gap > 90.0 ) continue;
            if( laneOf( c.m_x, lanes, seg.m_halfWidth ) == lane ) cost += 120.0f * static_cast<float>( 1.0 - gap / 100.0 );
        }
        for( const Rival &r : m_race.m_rivals )
        {
            const double gap = r.m_s - m_car.m_s;
            if( gap < -6.0 || gap > 60.0 || r.m_speed > m_car.m_speed + 2.0f ) continue;
            if( laneOf( r.m_x, lanes, seg.m_halfWidth ) == lane ) cost += 90.0f * static_cast<float>( 1.0 - gap / 70.0 );
        }
        if( cost < bestCost )
        {
            bestCost = cost;
            best = lane;
        }
    }
    const float targetX = laneCenter( best, lanes, seg.m_halfWidth );

    // Brake for the bends ahead.
    const float grip = m_car.m_spec.m_grip * m_car.m_wetGrip;
    float       maxK = 0.0f;
    const float look = 40.0f + m_car.m_speed * 2.6f;
    for( float d = 0.0f; d < look; d += SegmentLength )
        maxK = std::max( maxK, std::abs( track.segment( static_cast<long>( ( m_car.m_s + d ) / SegmentLength ) ).m_curve ) );
    const float limit = steerLimit( std::max( m_car.m_speed, 25.0f ), m_car.m_spec.m_topSpeed ) * grip * 0.8f;
    const float vMax = std::max( 15.0f, std::sqrt( limit / ( tuning().m_centrifugal * maxK + 1e-6f ) ) );

    // Follow a car that blocks the lane until there is room to pass.
    float follow = 1e9f;
    for( const TrafficCar &c : m_traffic.m_cars )
    {
        const double gap = c.m_s - m_car.m_s - ( c.m_length + m_car.m_spec.m_length ) * 0.5;
        if( gap < 0.0 || gap > 25.0 + m_car.m_speed * 1.2 ) continue;
        const float dx = std::abs( c.m_x - m_car.m_x );
        const float tx = std::abs( c.m_x - targetX );
        if( dx < ( c.m_width + m_car.m_spec.m_width ) * 0.6f || tx < 1.5f )
            follow = std::min( follow, c.m_speed + static_cast<float>( gap ) * 0.35f - 3.0f );
    }
    for( const Rival &r : m_race.m_rivals )
    {
        const double gap = r.m_s - m_car.m_s - m_car.m_spec.m_length;
        if( gap < 0.0 || gap > 15.0 + m_car.m_speed * 0.8 ) continue;
        if( std::abs( r.m_x - m_car.m_x ) < 2.0f || std::abs( r.m_x - targetX ) < 1.5f )
            follow = std::min( follow, r.m_speed + static_cast<float>( gap ) * 0.4f - 1.0f );
    }
    const float want = std::min( vMax, std::max( 0.0f, follow ) );

    CarInput in;
    in.m_throttle = m_car.m_speed < want ? 1.0f : 0.0f;
    in.m_brake = m_car.m_speed > want + 3.0f ? std::min( 1.0f, ( m_car.m_speed - want ) / 10.0f ) : 0.0f;
    const float steerMax = std::max( 1.0f, steerLimit( m_car.m_speed, m_car.m_spec.m_topSpeed ) * grip );
    const float feed = m_car.m_drift / steerMax;
    in.m_steer = clampf( feed + ( targetX - m_car.m_x ) * 0.22f - ( m_car.m_vx - m_car.m_drift ) * 0.04f, -1.0f, 1.0f );
    in.m_boost = maxK < 0.0015f && m_car.m_boostTank > 0.35f && m_car.m_speed > 45.0f;
    return in;
}

void Game::sparks( const Vec3d &at, int count, float speed )
{
    Rng &r = m_particles.m_rng;
    const TrackFrame f = m_track->frameAt( m_car.m_s );
    for( int i = 0; i < count; ++i )
    {
        Particle p;
        p.m_pos = at;
        p.m_vel = Vector3Add( Vector3Scale( f.m_forward, m_car.m_speed * r.range( 0.3f, 0.9f ) ),
                              Vector3{ r.range( -speed, speed ), r.range( 1.0f, speed ), r.range( -speed, speed ) } );
        p.m_maxLife = r.range( 0.25f, 0.6f );
        p.m_size = r.range( 0.03f, 0.06f );
        p.m_color = Color{ 255, static_cast<unsigned char>( r.rangeInt( 140, 220 ) ), 60, 255 };
        p.m_glow = 3.0f;
        p.m_gravity = 9.8f;
        p.m_drag = 0.8f;
        p.m_streak = true;
        m_particles.emit( p );
    }
}

void Game::handleEvents()
{
    const bool scoring = m_state == StateE::RACE && !m_demo && !m_timeUp && !m_setup.m_race && !m_finishedTrack;
    const Pose pose = m_car.pose( *m_track );
    if( m_ev.m_overtakes > 0 )
    {
        if( scoring )
        {
            m_overtakes += m_ev.m_overtakes;
            m_score += 100.0 * m_combo * m_ev.m_overtakes;
        }
        m_car.m_boostTank = std::min( 1.0f, m_car.m_boostTank + 0.03f * static_cast<float>( m_ev.m_overtakes ) );
    }
    for( int i = 0; i < m_ev.m_nearMisses; ++i )
    {
        m_car.m_boostTank = std::min( 1.0f, m_car.m_boostTank + 0.12f );
        if( !scoring ) continue;
        ++m_nearMisses;
        m_combo = std::min( m_combo + 1, 8 );
        m_comboTimer = 4.0f;
        m_score += 500.0 * m_combo;
        popup( "NEAR MISS", format( "+%d  X%d", 500 * m_combo, m_combo ), Cyan, 0.7f, 1.3f );
        m_audio.play( SfxE::NEAR_MISS, 0.8f );
    }
    if( m_ev.m_whoosh > 0.0f ) m_audio.play( SfxE::WHOOSH, m_ev.m_whoosh, m_ev.m_whooshPan );
    if( m_raceEv.m_whoosh > 0.0f ) m_audio.play( SfxE::WHOOSH, m_raceEv.m_whoosh, m_raceEv.m_whooshPan );
    if( m_raceEv.m_hit > 0.0f )
    {
        m_audio.play( m_raceEv.m_hit > 0.6f ? SfxE::CRASH : SfxE::BUMP, m_raceEv.m_hit );
        m_haptics.pulse( 0.5f * m_raceEv.m_hit, 0.35f * m_raceEv.m_hit, 0.25f );
        sparks( poseLocal( pose, 0.0f, 0.5f, 1.0f ), 10, 5.0f );
    }
    if( m_ev.m_animal >= 0 && !m_demo && m_state == StateE::RACE )
    {
        static const std::pair<PropKindE, const char *> names[] = {
            { PropKindE::COW, "CATTLE ON THE ROAD" },  { PropKindE::SHEEP, "SHEEP ON THE ROAD" }, { PropKindE::HORSE, "HORSES CROSSING" },
            { PropKindE::CAMEL, "CAMELS CROSSING" },   { PropKindE::ELEPHANT, "ELEPHANTS!" },     { PropKindE::GIRAFFE, "GIRAFFES CROSSING" },
            { PropKindE::ZEBRA, "ZEBRAS CROSSING" },   { PropKindE::KANGAROO, "KANGAROOS!" },     { PropKindE::LLAMA, "LLAMAS CROSSING" },
            { PropKindE::BISON, "BISON ON THE ROAD" }, { PropKindE::MOOSE, "MOOSE CROSSING" },    { PropKindE::REINDEER, "REINDEER CROSSING" },
            { PropKindE::YAK, "YAKS ON THE ROAD" } };
        for( const auto &[kind, text] : names )
            if( static_cast<int>( kind ) == m_ev.m_animal ) popup( text, "", Gold, 0.75f, 2.2f );
    }
    if( m_ev.m_hit > 0.0f )
    {
        m_audio.play( m_ev.m_hit > 0.6f ? SfxE::CRASH : SfxE::BUMP, m_ev.m_hit );
        m_haptics.pulse( 0.7f * m_ev.m_hit, 0.4f * m_ev.m_hit, 0.3f );
        sparks( poseLocal( pose, 0.0f, 0.5f, 2.0f ), 14, 6.0f );
        m_combo = 1;
    }
    for( int i = 0; i < std::min( m_ev.m_cones, 3 ); ++i ) m_audio.play( SfxE::CONE, 0.9f, m_particles.m_rng.range( -0.4f, 0.4f ) );
    if( m_ev.m_cones > 0 ) m_haptics.pulse( 0.15f, 0.4f, 0.12f );
    if( m_crash > 0.0f )
    {
        m_audio.play( SfxE::CRASH, m_crash );
        m_haptics.pulse( std::min( 1.0f, 0.6f + m_crash ), 0.6f, 0.6f );
        sparks( poseLocal( pose, 0.0f, 0.6f, 1.5f ), 36, 9.0f );
        m_combo = 1;
    }
    if( m_scraping )
    {
        sparks( poseLocal( pose, -m_scrapeSide * m_car.m_spec.m_width * 0.5f, 0.4f, 0.5f ), 2, 4.0f );
        if( !m_wasScraping ) m_audio.play( SfxE::BUMP, 0.5f, m_scrapeSide );
        m_haptics.steady( 0.12f, 0.45f );
    }
    m_wasScraping = m_scraping;
    if( m_landed && m_landImpact > 3.0f )
    {
        m_audio.play( SfxE::LAND, std::min( 1.0f, m_landImpact / 12.0f ) );
        m_haptics.pulse( std::min( 1.0f, m_landImpact / 12.0f ) * 0.8f, 0.2f, 0.25f );
    }
    if( m_shifted && !m_demo )
    {
        m_audio.play( SfxE::SHIFT, 0.5f );
        m_haptics.pulse( 0.18f, 0.0f, 0.08f );
    }
    if( m_backfire && !m_demo )
    {
        m_audio.play( SfxE::BACKFIRE, 0.8f );
        Rng &r = m_particles.m_rng;
        for( int i = 0; i < 6; ++i )
        {
            Particle p;
            p.m_pos = poseLocal( pose, r.range( -0.4f, 0.4f ), 0.35f, -m_car.m_spec.m_length * 0.5f - 0.1f );
            p.m_vel = Vector3Add( Vector3Scale( pose.m_forward, m_car.m_speed - r.range( 3.0f, 8.0f ) ), Vector3{ 0, r.range( 0, 1 ), 0 } );
            p.m_maxLife = r.range( 0.06f, 0.14f );
            p.m_size = r.range( 0.15f, 0.3f );
            p.m_color = Color{ 255, 140, 40, 255 };
            p.m_glow = 3.0f;
            m_particles.emit( p );
        }
    }
    if( m_car.m_boosting && !m_wasBoosting && !m_demo )
    {
        m_audio.play( SfxE::BOOST, 0.8f );
        m_haptics.pulse( 0.3f, 0.15f, 0.45f );
    }
    m_wasBoosting = m_car.m_boosting;
    if( m_state == StateE::RACE && !m_timeUp && !m_setup.m_race && !m_finishedTrack && m_timeLeft < 10.0f )
    {
        const int sec = static_cast<int>( std::ceil( m_timeLeft ) );
        if( sec != m_lastWarning )
        {
            m_lastWarning = sec;
            m_audio.play( SfxE::WARNING, 0.8f );
        }
    }
    else if( m_timeLeft >= 10.0f )
    {
        m_lastWarning = 99;
    }
}

void Game::emitParticles( float dt )
{
    const Track      &track = *m_track;
    const Pose        pose = m_car.pose( track );
    Rng              &r = m_particles.m_rng;
    const Environment env = sceneEnvironment( m_car.m_s );
    const Surface     surf = surfaceAt( track, m_car.m_s );
    const Vector3     carVel = Vector3Scale( pose.m_forward, m_car.m_speed );
    const float       rearZ = -m_car.m_spec.m_length * 0.3f;

    if( m_car.m_slip > 0.25f )
    {
        for( int i = emitCount( m_car.m_slip * 70.0f, dt, r ); i > 0; --i )
        {
            Particle p;
            p.m_pos = poseLocal( pose, ( i % 2 ? 0.8f : -0.8f ), 0.25f, rearZ );
            p.m_vel = Vector3Add( Vector3Scale( carVel, 0.3f ), Vector3{ r.range( -1.5f, 1.5f ), r.range( 0.4f, 1.4f ), r.range( -1.5f, 1.5f ) } );
            p.m_maxLife = r.range( 0.8f, 1.4f );
            p.m_size = 0.4f;
            p.m_grow = 2.2f;
            p.m_color = Color{ 214, 214, 220, 110 };
            p.m_drag = 1.5f;
            m_particles.emit( p );
        }
    }
    const float dirt = std::max( m_car.m_offroad, m_car.m_unpaved && !m_car.m_airborne ? 0.75f : 0.0f );
    if( dirt > 0.3f && m_car.m_speed > 5.0f )
    {
        // Off the road, or on gravel and sand: a cloud of the ground's colour (the road's own on a dirt track).
        const bool  snowy = env.m_snow > 0.5f;
        const Color ground = m_car.m_offroad > 0.3f ? surf.m_groundA : surf.m_road;
        const Color dust = snowy ? Color{ 240, 244, 250, 150 } : lerpColor( ground, Color{ 200, 180, 150, 255 }, 0.4f );
        for( int i = emitCount( dirt * m_car.m_speed * 1.2f, dt, r ); i > 0; --i )
        {
            Particle p;
            p.m_pos = poseLocal( pose, r.range( -0.9f, 0.9f ), 0.2f, rearZ - 0.3f );
            p.m_vel = Vector3Add( Vector3Scale( carVel, 0.4f ), Vector3{ r.range( -2.0f, 2.0f ), r.range( 1.0f, 3.0f ), r.range( -2.0f, 2.0f ) } );
            p.m_maxLife = r.range( 0.9f, 1.6f );
            p.m_size = 0.5f;
            p.m_grow = 3.0f;
            p.m_color = Color{ dust.r, dust.g, dust.b, static_cast<unsigned char>( snowy ? 170 : 120 ) };
            p.m_drag = 1.2f;
            p.m_gravity = 0.5f;
            m_particles.emit( p );
        }
    }
    if( env.m_rain > 0.4f && m_car.m_speed > 15.0f && !m_car.m_airborne )
    {
        for( int i = emitCount( m_car.m_speed * 0.7f * env.m_rain, dt, r ); i > 0; --i )
        {
            Particle p;
            p.m_pos = poseLocal( pose, ( i % 2 ? 0.8f : -0.8f ), 0.2f, rearZ - 0.2f );
            p.m_vel = Vector3Add( Vector3Scale( carVel, 0.55f ), Vector3{ r.range( -1.0f, 1.0f ), r.range( 0.5f, 1.5f ), r.range( -1.0f, 1.0f ) } );
            p.m_maxLife = r.range( 0.5f, 0.8f );
            p.m_size = 0.45f;
            p.m_grow = 4.0f;
            p.m_color = Color{ 196, 202, 212, 70 };
            p.m_drag = 2.0f;
            m_particles.emit( p );
        }
    }
    if( m_car.m_boosting )
    {
        for( int i = emitCount( 90.0f, dt, r ); i > 0; --i )
        {
            Particle p;
            p.m_pos = poseLocal( pose, ( i % 2 ? 0.35f : -0.35f ), 0.32f, -m_car.m_spec.m_length * 0.5f - 0.05f );
            p.m_vel = Vector3Add( Vector3Scale( pose.m_forward, m_car.m_speed - 7.0f ), Vector3{ r.range( -0.4f, 0.4f ), r.range( -0.2f, 0.4f ), 0.0f } );
            p.m_maxLife = r.range( 0.07f, 0.13f );
            p.m_size = r.range( 0.12f, 0.22f );
            p.m_color = Color{ 110, 170, 255, 255 };
            p.m_glow = 2.5f;
            m_particles.emit( p );
        }
    }
    if( m_landed && m_landImpact > 4.0f )
    {
        for( int i = 0; i < 16; ++i )
        {
            Particle p;
            p.m_pos = poseLocal( pose, r.range( -1.0f, 1.0f ), 0.1f, r.range( -2.0f, 2.0f ) );
            p.m_vel = Vector3Add( Vector3Scale( carVel, 0.5f ), Vector3{ r.range( -3.0f, 3.0f ), r.range( 0.5f, 2.0f ), r.range( -3.0f, 3.0f ) } );
            p.m_maxLife = 1.0f;
            p.m_size = 0.5f;
            p.m_grow = 2.5f;
            p.m_color = Color{ 200, 196, 186, 110 };
            p.m_drag = 2.0f;
            m_particles.emit( p );
        }
    }
}

void Game::updateCamera( float dt )
{
    const Track &track = *m_track;
    const int    mode = m_demo ? 1 : m_settings.m_camera;
    const float  sn = m_car.m_speed / m_car.m_spec.m_topSpeed;
    const float  fovTarget = ( mode == 2 ? 66.0f : 58.0f ) + 20.0f * std::pow( sn, 1.4f ) + ( m_car.m_boosting ? 9.0f : 0.0f );
    m_fov = m_camInit ? approach( m_fov, fovTarget, 3.0f, dt ) : fovTarget;
    const TrackFrame f = track.frameAt( m_car.m_s );
    const double surfaceAtCar = track.surfaceHeight( m_car.m_s, m_car.m_x );
    const float  lift = static_cast<float>( m_car.m_y - surfaceAtCar );

    if( mode == 2 )
    {
        const Pose p = m_car.pose( track );
        m_cam.m_pos = poseLocal( p, 0.0f, 1.02f, 0.3f );
        const Vector3 fwd = Vector3Normalize( Vector3Add( Vector3Scale( f.m_forward, 0.6f ), Vector3Scale( p.m_forward, 0.4f ) ) );
        m_cam.m_forward = fwd;
        m_cam.m_up = p.m_up;
    }
    else
    {
        float back = ( mode == 0 ? 6.4f : 9.2f ) + sn * 0.9f;
        float height = mode == 0 ? 2.1f : 3.1f;
        if( m_demo ) back += 2.0f * std::sin( m_time * 0.15f );
        m_camX = m_camInit ? approach( m_camX, m_car.m_x * 0.82f, 5.0f, dt ) : m_car.m_x * 0.82f;
        const double camS = m_car.m_s - back;
        Vec3d        pos = track.pointAt( camS, m_camX, 0.0f );
        const double want = std::max( pos.m_y + height, m_car.m_y + height * 0.75 );
        m_camY = m_camInit ? m_camY + ( want - m_camY ) * ( 1.0 - std::exp( -8.0 * dt ) ) : want;
        pos.m_y = m_camY;
        Vec3d look = track.pointAt( m_car.m_s + 16.0 + sn * 8.0f, m_car.m_x * 0.55f, 1.0f );
        look.m_y += lift * 0.6f;
        m_cam.m_pos = pos;
        m_cam.m_forward = Vector3Normalize( relativeTo( look, pos ) );
        const Vector3 up = Vector3Normalize( Vector3Lerp( Vector3{ 0, 1, 0 }, f.m_up, 0.5f ) );
        const float   roll = -m_car.m_steer * 0.025f - clampf( m_car.m_drift * 0.002f, -0.03f, 0.03f );
        const Vector3 right = Vector3Normalize( Vector3CrossProduct( m_cam.m_forward, up ) );
        m_cam.m_up = Vector3Normalize( Vector3Add( Vector3Scale( up, std::cos( roll ) ), Vector3Scale( right, std::sin( roll ) ) ) );
    }
    if( m_state == StateE::GARAGE )
    { // The showroom: round and round the car.
        const Pose  p = m_car.pose( track );
        const float a = m_time * 0.3f;
        m_cam.m_pos = poseLocal( p, std::sin( a ) * 6.5f, 1.7f + 0.4f * std::sin( m_time * 0.21f ), std::cos( a ) * 6.5f );
        m_cam.m_forward = Vector3Normalize( relativeTo( poseLocal( p, 0.0f, 0.55f, 0.0f ), m_cam.m_pos ) );
        m_cam.m_up = p.m_up;
        m_cam.m_fovY = 50.0f;
        m_blur = 0.0f;
        return;
    }
    // Shake: kerbs, verge, landings, hits, and a faint tremor at top speed.
    const float shake = m_car.m_shake + std::pow( sn, 3.0f ) * 0.15f;
    if( shake > 0.001f )
    {
        const float t = m_time;
        const float sx = ( std::sin( t * 37.1f ) * 0.6f + std::sin( t * 61.7f ) * 0.4f ) * shake * 0.07f;
        const float sy = ( std::sin( t * 43.3f ) * 0.6f + std::sin( t * 71.9f ) * 0.4f ) * shake * 0.06f;
        m_cam.m_pos.m_y += static_cast<double>( sy );
        const Vector3 right = Vector3Normalize( Vector3CrossProduct( m_cam.m_forward, m_cam.m_up ) );
        m_cam.m_pos.m_x += static_cast<double>( right.x * sx );
        m_cam.m_pos.m_z += static_cast<double>( right.z * sx );
    }
    m_cam.m_fovY = m_fov;
    m_camInit = true;
    const float blurTarget = m_settings.m_speedBlur ? smoothstepf( 0.55f, 1.0f, sn ) * 0.9f + ( m_car.m_boosting ? 0.45f : 0.0f ) : 0.0f;
    m_blur = approach( m_blur, blurTarget, 4.0f, dt );
    m_aberration = 0.0f;   // colour fringes turned bright lamps and stars green; left off
}

void Game::updateAudio()
{
    EngineState s;
    s.m_active = !m_demo && m_state != StateE::PAUSED && m_state != StateE::SETTINGS;
    s.m_rpm = m_car.m_rpm;
    s.m_throttle = m_car.m_throttle;
    s.m_speed = s.m_active ? m_car.m_speed : 0.0f;
    s.m_slip = s.m_active ? m_car.m_slip : 0.0f;
    s.m_offroad = s.m_active ? std::max( m_car.m_offroad, m_car.m_unpaved ? 0.5f : 0.0f ) : 0.0f;
    s.m_rumble = s.m_active ? m_car.m_rumble : 0.0f;
    m_haptics.steady( s.m_rumble * 0.25f + s.m_offroad * std::min( 1.0f, s.m_speed / 30.0f ) * 0.2f, s.m_rumble * 0.35f );
    s.m_boost = s.m_active && m_car.m_boosting;
    s.m_limiter = s.m_active && m_car.m_rpm > 0.985f && m_car.m_throttle > 0.5f && !m_car.m_boosting;
    s.m_shifting = m_car.m_shiftTimer > 0.0f;
    s.m_cylinders = m_car.m_spec.m_cylinders;
    s.m_idle = m_car.m_spec.m_idle;
    s.m_redline = m_car.m_spec.m_redline;
    s.m_bright = m_car.m_spec.m_bright;
    m_audio.setEngine( s );

    // The place's own sounds: the sea, the wind, the jungle, the city, rain, bridges and tunnels.
    Ambience a;
    if( !m_demo && ( m_state == StateE::RACE || m_state == StateE::COUNTDOWN ) )
    {
        const BiomeMix  mix = m_track->biomeMix( m_car.m_s );
        const BiomeDef &def = biomeDef( mix.m_t < 0.5f ? mix.m_from : mix.m_to );
        const Terrain  &land = def.m_terrain;
        const uint16_t  flags = m_track->segment( static_cast<long>( m_car.m_s / SegmentLength ) ).m_flags;
        const bool      city = def.m_lanes >= 3;
        WeatherE        front = WeatherE::NONE;
        const float     k = frontAt( m_car.m_s, front );
        a.m_surf = std::max( land.m_waterLeft, land.m_waterRight ) > 0.5f ? ( land.m_seaLevel > -12.0f ? 1.0f : 0.55f ) : 0.0f;
        a.m_surfPan = clampf( land.m_waterRight - land.m_waterLeft, -1.0f, 1.0f );
        a.m_city = city ? 0.85f : def.m_lampSpacing > 0.0f ? 0.3f : 0.0f;
        a.m_jungle = city ? 0.0f : def.m_climate == ClimateE::TROPICAL ? 0.8f : def.m_climate == ClimateE::TEMPERATE ? 0.2f : 0.0f;
        a.m_wind = ( def.m_climate == ClimateE::ARID ? 0.7f : def.m_climate == ClimateE::COLD ? 0.55f : 0.22f ) +
                   ( front == WeatherE::SANDSTORM || front == WeatherE::SNOW || front == WeatherE::STORM ? 0.6f * k : 0.0f );
        a.m_rain = sceneEnvironment( m_car.m_s ).m_rain;
        a.m_bridge = flags & SEG_BRIDGE ? 1.0f : 0.0f;
        if( flags & SEG_TUNNEL ) a = Ambience{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    }
    m_audio.setAmbience( a );

    // The soundtrack's mood: tension when time runs short or the finish nears, drive with the speed.
    float tension = 0.0f;
    if( !m_demo && m_state == StateE::RACE )
    {
        if( m_setup.m_race ) tension = m_finished ? 0.0f : smoothstepf( 1200.0f, 300.0f, static_cast<float>( m_race.m_finishS - m_car.m_s ) );
        else if( !m_timeUp && !m_finishedTrack ) tension = smoothstepf( 12.0f, 4.0f, m_timeLeft );
    }
    m_audio.setMood( tension, m_demo ? 0.6f : m_car.m_speed / m_car.m_spec.m_topSpeed );
}

} // namespace sunstreak
