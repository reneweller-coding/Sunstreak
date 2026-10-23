/**
 * @file Game.cpp
 * @brief Game flow, rules, camera and effects. The menus, screens and the HUD are in GameUi.cpp.
 * @author reneweller
 */
#include "Game.h"

#include "UiCommon.h"

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
    m_tuningBase = loadTuning( configDirectory() + "/tuning.ini" );
    tuning() = applyDifficulty( m_tuningBase, m_settings.m_difficulty );
    m_records.load( configDirectory() + "/records.txt" );
    m_models.build();
    m_renderer.init();
    m_world.init( &m_models );
    const std::vector<std::string> dirs = { std::string( GetApplicationDirectory() ) + "music", configDirectory() + "/music" };
    m_audioOk = !m_options.m_mute && m_audio.init( dirs );
    m_audio.setVolumes( m_settings.m_musicVolume, m_settings.m_sfxVolume );
    m_lab.m_seed = randomSeed();
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
        startTrack( endless, ModeE::ENDLESS );
    }
    else
    {
        startDemo();
        if( m_options.m_screen == "lab" ) m_state = StateE::LAB;
        if( m_options.m_screen == "records" ) m_state = StateE::RECORDS;
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

void Game::setupTrack( uint64_t seed, double startS, const TrackParams &params )
{
    m_seed = seed;
    m_world.clear();
    m_track = std::make_unique<Track>( seed, params, m_options.m_startBiome );
    m_track->generateTo( startS + ViewDistance + 600.0 );
    const Segment &seg = m_track->segment( static_cast<long>( startS / SegmentLength ) );
    m_car.reset( carSpec( m_carIndex ), startS, laneCenter( seg.m_lanes / 2, seg.m_lanes, seg.m_halfWidth ), *m_track );
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
    setupTrack( seed, 600.0, TrackParams() );
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
        popup( "RACE", biomeDef( m_track->stageBiome( 0 ) ).m_name, Accent, 1.0f, 3.0f );
    else
        popup( format( "STAGE %d", m_stage + 1 ), biomeDef( m_track->stageBiome( m_stage ) ).m_name, Accent, 1.0f, 3.0f );
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
    popup( format( "RACE %d OF %d", round + 1, RoundCount ), biomeDef( m_track->stageBiome( 0 ) ).m_name, Accent, 1.0f, 3.0f );
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
    m_records.m_seenBiomes |= 1ull << static_cast<int>( mix.m_t < 0.5f ? mix.m_from : mix.m_to );
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
    const MenuInput mi = m_input.menu();

    switch( m_state )
    {
    case StateE::TITLE:
    case StateE::LAB:
    case StateE::CODE_ENTRY:
    case StateE::RECORDS:
    {
        const StateE before = m_state;
        menus( mi );
        if( m_state == before || m_state == StateE::SETTINGS || m_state == StateE::LAB || m_state == StateE::TITLE ||
            m_state == StateE::RECORDS || m_state == StateE::CODE_ENTRY )
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
    popup( m_place == 1 ? "YOU WIN!" : format( "P%d", m_place ), "FINISH  " + clockText( m_raceTime ), m_place <= 3 ? Gold : Ink, 1.4f, 3.5f );
    m_audio.play( m_place <= 3 ? SfxE::CHECKPOINT : SfxE::STAGE );
    saveRecords();
}

void Game::simulate( float dt, bool autopilotOn )
{
    Track &track = *m_track;
    track.generateTo( m_car.m_s + ViewDistance + 600.0 );
    track.discardBefore( m_car.m_s - 400.0 );
    m_world.update( track, m_car.m_s, 2 );

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

    const Environment env = environmentAt( track, m_car.m_s + 30.0 );
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
        m_audio.play( SfxE::CHECKPOINT );
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
            m_audio.play( SfxE::CHECKPOINT );
        }
        m_nextCheckpoint += CheckpointSpacing;
    }
    const int stage = static_cast<int>( m_car.m_s / StageLength );
    if( stage != m_stage )
    {
        m_stage = stage;
        popup( format( "STAGE %d", stage + 1 ), biomeDef( track.stageBiome( stage ) ).m_name, Accent, 1.2f, 3.2f );
        m_audio.play( SfxE::STAGE );
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
        sparks( poseLocal( pose, 0.0f, 0.5f, 1.0f ), 10, 5.0f );
    }
    if( m_ev.m_hit > 0.0f )
    {
        m_audio.play( m_ev.m_hit > 0.6f ? SfxE::CRASH : SfxE::BUMP, m_ev.m_hit );
        sparks( poseLocal( pose, 0.0f, 0.5f, 2.0f ), 14, 6.0f );
        m_combo = 1;
    }
    for( int i = 0; i < std::min( m_ev.m_cones, 3 ); ++i ) m_audio.play( SfxE::CONE, 0.9f, m_particles.m_rng.range( -0.4f, 0.4f ) );
    if( m_crash > 0.0f )
    {
        m_audio.play( SfxE::CRASH, m_crash );
        sparks( poseLocal( pose, 0.0f, 0.6f, 1.5f ), 36, 9.0f );
        m_combo = 1;
    }
    if( m_scraping )
    {
        sparks( poseLocal( pose, -m_scrapeSide * m_car.m_spec.m_width * 0.5f, 0.4f, 0.5f ), 2, 4.0f );
        if( !m_wasScraping ) m_audio.play( SfxE::BUMP, 0.5f, m_scrapeSide );
    }
    m_wasScraping = m_scraping;
    if( m_landed && m_landImpact > 3.0f ) m_audio.play( SfxE::LAND, std::min( 1.0f, m_landImpact / 12.0f ) );
    if( m_shifted && !m_demo ) m_audio.play( SfxE::SHIFT, 0.5f );
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
    if( m_car.m_boosting && !m_wasBoosting && !m_demo ) m_audio.play( SfxE::BOOST, 0.8f );
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
    const Environment env = environmentAt( track, m_car.m_s );
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
    if( m_car.m_offroad > 0.3f && m_car.m_speed > 5.0f )
    {
        const bool  snowy = env.m_snow > 0.5f;
        const Color dust = snowy ? Color{ 240, 244, 250, 150 } : lerpColor( surf.m_groundA, Color{ 200, 180, 150, 255 }, 0.5f );
        for( int i = emitCount( m_car.m_offroad * m_car.m_speed * 1.2f, dt, r ); i > 0; --i )
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
    s.m_offroad = s.m_active ? m_car.m_offroad : 0.0f;
    s.m_rumble = s.m_active ? m_car.m_rumble : 0.0f;
    s.m_boost = s.m_active && m_car.m_boosting;
    s.m_limiter = s.m_active && m_car.m_rpm > 0.985f && m_car.m_throttle > 0.5f && !m_car.m_boosting;
    s.m_shifting = m_car.m_shiftTimer > 0.0f;
    s.m_cylinders = m_car.m_spec.m_cylinders;
    s.m_idle = m_car.m_spec.m_idle;
    s.m_redline = m_car.m_spec.m_redline;
    s.m_bright = m_car.m_spec.m_bright;
    m_audio.setEngine( s );
}

} // namespace sunstreak
