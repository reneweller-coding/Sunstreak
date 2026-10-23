/**
 * @file Game.cpp
 * @brief Game flow, rules, camera, effects and the screens.
 * @author reneweller
 */
#include "Game.h"

#include "VectorFont.h"

#include <rlgl.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>

namespace sunstreak
{

namespace
{

constexpr float PhysicsStep = 1.0f / 120.0f;    ///< fixed physics step
constexpr float StartTime = 55.0f;              ///< seconds on the clock at the start

constexpr Color Accent = { 255, 150, 40, 255 };   ///< sunset orange
constexpr Color Cyan = { 70, 220, 255, 255 };     ///< boost, near misses
constexpr Color Green = { 90, 255, 150, 255 };    ///< checkpoints
constexpr Color Red = { 255, 60, 60, 255 };       ///< warnings
constexpr Color Ink = { 245, 246, 250, 255 };     ///< text
constexpr Color Dim = { 190, 196, 210, 255 };     ///< secondary text

/**
 * @brief printf into a std::string.
 * @param[in] f  the format
 * @return the text
 */
std::string format( const char *f, ... )
{
    char    buf[256];
    va_list args;
    va_start( args, f );
    std::vsnprintf( buf, sizeof( buf ), f, args );
    va_end( args );
    return buf;
}

/**
 * @brief The time a checkpoint adds.
 * @param[in] k  the checkpoint's number (1 first)
 * @return seconds
 */
float checkpointBonus( int k )
{
    return static_cast<float>( CheckpointSpacing ) / std::min( 64.0f, 48.0f + 2.0f * static_cast<float>( k ) );
}

/**
 * @brief A fresh random seed (40 bits, never zero).
 * @return the seed
 */
uint64_t randomSeed()
{
    const auto t = static_cast<uint64_t>( std::chrono::high_resolution_clock::now().time_since_epoch().count() );
    return ( hash64( t ) & 0xFFFFFFFFFFull ) | 1ull;
}

/**
 * @brief The blended environment at a distance.
 * @param[in] track  the road
 * @param[in] s      the distance
 * @return light and air
 */
Environment environmentAt( const Track &track, double s )
{
    const BiomeMix m = track.biomeMix( s );
    return blendEnvironment( biomeDef( m.m_from ).m_env, biomeDef( m.m_to ).m_env, m.m_t );
}

/**
 * @brief The blended surface at a distance.
 * @param[in] track  the road
 * @param[in] s      the distance
 * @return colours
 */
Surface surfaceAt( const Track &track, double s )
{
    const BiomeMix m = track.biomeMix( s );
    return blendSurface( biomeDef( m.m_from ).m_surface, biomeDef( m.m_to ).m_surface, m.m_t );
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
 * @brief Draws a rounded translucent panel.
 * @param[in] x      left
 * @param[in] y      top
 * @param[in] w      width
 * @param[in] h      height
 * @param[in] alpha  opacity 0..255
 */
void panel( float x, float y, float w, float h, unsigned char alpha = 170 )
{
    DrawRectangleRounded( Rectangle{ x, y, w, h }, 0.08f, 8, Color{ 8, 10, 22, alpha } );
    DrawRectangleRoundedLinesEx( Rectangle{ x, y, w, h }, 0.08f, 8, 1.5f, Color{ 255, 255, 255, 40 } );
}

} // namespace

std::string trackCode( uint64_t seed )
{
    static const char *alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    std::string        s;
    for( int i = 7; i >= 0; --i )
    {
        s += alphabet[( seed >> ( i * 5 ) ) & 31];
        if( i == 4 ) s += '-';
    }
    return s;
}

Game::Game( Settings &settings, const LaunchOptions &options ) : m_settings( settings ), m_options( options ) {}

void Game::init()
{
    m_models.build();
    m_renderer.init();
    m_world.init( &m_models );
    const std::vector<std::string> dirs = { std::string( GetApplicationDirectory() ) + "music", configDirectory() + "/music" };
    m_audioOk = !m_options.m_mute && m_audio.init( dirs );
    m_audio.setVolumes( m_settings.m_musicVolume, m_settings.m_sfxVolume );
    if( m_options.m_skipMenu ) startRun( m_options.m_seed ? m_options.m_seed : randomSeed() );
    else startDemo();
}

void Game::shutdown()
{
    m_audio.stopMusic();
    m_audio.shutdown();
    m_world.clear();
    m_renderer.unload();
    m_models.unload();
}

void Game::setupTrack( uint64_t seed, double startS )
{
    m_seed = seed;
    m_world.clear();
    m_track = std::make_unique<Track>( seed, m_options.m_startBiome );
    m_track->generateTo( startS + ViewDistance + 600.0 );
    const Segment &seg = m_track->segment( static_cast<long>( startS / SegmentLength ) );
    m_car.reset( carSpec( m_carIndex ), startS, laneCenter( seg.m_lanes / 2, seg.m_lanes, seg.m_halfWidth ), *m_track );
    m_traffic.reset( seed, startS );
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
    m_state = StateE::TITLE;
    m_stateTime = 0.0f;
    m_menuIndex = 0;
    const uint64_t seed = randomSeed();
    setupTrack( seed, 600.0 );
    m_car.m_speed = 42.0f;
    m_car.m_gear = 3;
    m_musicStage = static_cast<int>( m_car.m_s / StageLength );
    playMusic( hash64( seed ), wantedStation() );
}

void Game::startRun( uint64_t seed )
{
    m_demo = false;
    double startS = StartLine - 9.0;
    if( m_options.m_startDistance > 0.0f )
    {
        startS = m_options.m_startDistance;
        m_options.m_startDistance = 0.0f;
    }
    setupTrack( seed, startS );
    m_car.m_frozen = true;
    m_timeLeft = StartTime;
    m_timeUp = false;
    m_timeUpFor = 0.0f;
    m_score = 0.0;
    m_checkpoints = static_cast<int>( startS / CheckpointSpacing );
    m_nextCheckpoint = ( m_checkpoints + 1 ) * CheckpointSpacing;
    m_stage = static_cast<int>( startS / StageLength );
    m_overtakes = 0;
    m_nearMisses = 0;
    m_combo = 1;
    m_comboTimer = 0.0f;
    m_topSpeed = 0.0f;
    m_newBest = false;
    m_lastWarning = 99;
    m_lastCount = 4;
    m_state = StateE::COUNTDOWN;
    m_stateTime = 0.0f;
    m_musicStage = m_stage;
    playMusic( hash64( seed + 7 ), wantedStation() );
    popup( format( "STAGE %d", m_stage + 1 ), biomeDef( m_track->stageBiome( m_stage ) ).m_name, Accent, 1.0f, 3.0f );
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
        menus( mi );
        if( m_state == StateE::TITLE || m_state == StateE::SETTINGS ) simulate( dt, true );
        break;
    case StateE::SETTINGS:
        menus( mi );
        if( m_settingsReturn == StateE::TITLE ) simulate( dt, true );
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
        simulate( dt, m_options.m_autopilot );
        if( m_timeUp && ( m_car.m_speed < 0.5f || m_timeUpFor > 5.0f ) )
        {
            const auto score = static_cast<int64_t>( m_score );
            const double dist = m_car.m_s - m_startS;
            m_newBest = score > m_settings.m_bestScore;
            m_settings.m_bestScore = std::max( m_settings.m_bestScore, score );
            m_settings.m_bestDistance = std::max( m_settings.m_bestDistance, dist );
            saveSettings( m_settings );
            m_state = StateE::GAMEOVER;
            m_stateTime = 0.0f;
            m_menuIndex = 0;
        }
        break;
    case StateE::PAUSED: menus( mi ); break;
    case StateE::GAMEOVER:
        menus( mi );
        if( m_state == StateE::GAMEOVER ) simulate( dt, true );
        break;
    }
    if( m_input.m_nextStation ) nextStation();
    if( m_input.m_nextTrack ) playMusic( hash64( m_trackSeed + 31 ), m_playingStation );
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

void Game::simulate( float dt, bool autopilotOn )
{
    Track &track = *m_track;
    track.generateTo( m_car.m_s + ViewDistance + 600.0 );
    track.discardBefore( m_car.m_s - 400.0 );
    m_world.update( track, m_car.m_s, 2 );

    m_ev = TrafficEvents{};
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
                m_car.crash( 0.22f, c->m_x + toRoad * ( rw + 0.3f ) - m_car.m_x );
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

    // The rules of a run.
    if( m_state != StateE::RACE || m_demo ) return;
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
    m_topSpeed = std::max( m_topSpeed, m_car.m_speed );
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
    const float vMax = std::max( 15.0f, std::sqrt( limit / ( CentrifugalK * maxK + 1e-6f ) ) );

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
    const bool scoring = m_state == StateE::RACE && !m_demo && !m_timeUp;
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
    if( m_shifted && m_state != StateE::TITLE ) m_audio.play( SfxE::SHIFT, 0.5f );
    if( m_backfire && m_state != StateE::TITLE )
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
    if( m_car.m_boosting && !m_wasBoosting && m_state != StateE::TITLE ) m_audio.play( SfxE::BOOST, 0.8f );
    m_wasBoosting = m_car.m_boosting;
    if( m_state == StateE::RACE && !m_timeUp && m_timeLeft < 10.0f )
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
    const int    mode = m_state == StateE::TITLE || ( m_state == StateE::SETTINGS && m_settingsReturn == StateE::TITLE ) ? 1
                                                                                                                    : m_settings.m_camera;
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
        if( m_state == StateE::TITLE ) back += 2.0f * std::sin( m_time * 0.15f );
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
    const bool title = m_state == StateE::TITLE || ( m_state == StateE::SETTINGS && m_settingsReturn == StateE::TITLE );
    s.m_active = !title && m_state != StateE::PAUSED && !( m_state == StateE::SETTINGS );
    s.m_rpm = m_car.m_rpm;
    s.m_throttle = m_car.m_throttle;
    s.m_speed = s.m_active ? m_car.m_speed : 0.0f;
    s.m_slip = s.m_active ? m_car.m_slip : 0.0f;
    s.m_offroad = s.m_active ? m_car.m_offroad : 0.0f;
    s.m_rumble = s.m_active ? m_car.m_rumble : 0.0f;
    s.m_boost = s.m_active && m_car.m_boosting;
    s.m_limiter = s.m_active && m_car.m_rpm > 0.985f && m_car.m_throttle > 0.5f && !m_car.m_boosting;
    s.m_shifting = m_car.m_shiftTimer > 0.0f;
    m_audio.setEngine( s );
}

void Game::menus( const MenuInput &m )
{
    auto move = [&]( int count )
    {
        if( m.m_up )
        {
            m_menuIndex = ( m_menuIndex + count - 1 ) % count;
            m_audio.play( SfxE::TICK );
        }
        if( m.m_down )
        {
            m_menuIndex = ( m_menuIndex + 1 ) % count;
            m_audio.play( SfxE::TICK );
        }
    };
    const int change = ( m.m_right ? 1 : 0 ) - ( m.m_left ? 1 : 0 );

    switch( m_state )
    {
    case StateE::TITLE:
        move( 5 );
        if( m_menuIndex == 1 && change != 0 )
        {
            m_carIndex = ( m_carIndex + CarCount + change ) % CarCount;
            const CarSpec spec = carSpec( m_carIndex );
            m_car.m_spec = spec;
            m_audio.play( SfxE::TICK );
        }
        if( m_menuIndex == 2 && change != 0 )
        {
            m_paintIndex = ( m_paintIndex + PaintCount + change ) % PaintCount;
            m_audio.play( SfxE::TICK );
        }
        if( m.m_accept )
        {
            m_audio.play( SfxE::SELECT );
            switch( m_menuIndex )
            {
            case 0: startRun( m_options.m_seed ? m_options.m_seed : randomSeed() ); break;
            case 3:
                m_settingsReturn = StateE::TITLE;
                m_returnIndex = 3;
                m_state = StateE::SETTINGS;
                m_menuIndex = 0;
                break;
            case 4: m_quit = true; break;
            default: break;
            }
        }
        break;
    case StateE::SETTINGS:
    {
        move( 11 );
        bool changed = false;
        if( change != 0 || ( m.m_accept && m_menuIndex < 10 ) )
        {
            const int dir = change != 0 ? change : 1;
            changed = true;
            switch( m_menuIndex )
            {
            case 0: toggleFullscreen(); break;
            case 1:
            {
                static const float scales[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
                int idx = 2;
                for( int i = 0; i < 6; ++i )
                    if( std::abs( scales[i] - m_settings.m_renderScale ) < 0.01f ) idx = i;
                m_settings.m_renderScale = scales[std::clamp( idx + dir, 0, 5 )];
                break;
            }
            case 2:
                m_settings.m_vsync = !m_settings.m_vsync;
                if( m_settings.m_vsync ) SetWindowState( FLAG_VSYNC_HINT );
                else ClearWindowState( FLAG_VSYNC_HINT );
                break;
            case 3: m_settings.m_manualGears = !m_settings.m_manualGears; break;
            case 4: m_settings.m_units = m_settings.m_units == UnitsE::KMH ? UnitsE::MPH : UnitsE::KMH; break;
            case 5: m_settings.m_camera = ( m_settings.m_camera + 3 + dir ) % 3; break;
            case 6: m_settings.m_speedBlur = !m_settings.m_speedBlur; break;
            case 7: m_settings.m_musicVolume = clampf( m_settings.m_musicVolume + 0.1f * dir, 0.0f, 1.0f ); break;
            case 8: m_settings.m_sfxVolume = clampf( m_settings.m_sfxVolume + 0.1f * dir, 0.0f, 1.0f ); break;
            case 9:
            {
                const int last = m_audio.hasFileMusic() ? StationCount : StationCount - 1;
                int       s = m_settings.m_station + dir;
                if( s > last ) s = -1;
                if( s < -1 ) s = last;
                m_settings.m_station = s;
                playMusic( hash64( m_trackSeed + 101 ), wantedStation() );
                break;
            }
            default: changed = false; break;
            }
        }
        if( changed )
        {
            settingsChanged();
            m_audio.play( SfxE::TICK );
        }
        if( m.m_back || ( m.m_accept && m_menuIndex == 10 ) )
        {
            m_audio.play( SfxE::SELECT );
            m_state = m_settingsReturn;
            m_menuIndex = m_returnIndex;
        }
        break;
    }
    case StateE::PAUSED:
        move( 5 );
        if( m.m_back || m_input.m_pause )
        {
            m_state = m_pauseReturn;
            break;
        }
        if( m.m_accept )
        {
            m_audio.play( SfxE::SELECT );
            switch( m_menuIndex )
            {
            case 0: m_state = m_pauseReturn; break;
            case 1: startRun( m_seed ); break;
            case 2: playMusic( hash64( m_trackSeed + 31 ), m_playingStation ); break;
            case 3:
                m_settingsReturn = StateE::PAUSED;
                m_returnIndex = 3;
                m_state = StateE::SETTINGS;
                m_menuIndex = 0;
                break;
            default: startDemo(); break;
            }
        }
        break;
    case StateE::GAMEOVER:
        move( 3 );
        if( m.m_accept && m_stateTime > 1.0f )
        {
            m_audio.play( SfxE::SELECT );
            switch( m_menuIndex )
            {
            case 0: startRun( m_seed ); break;
            case 1: startRun( randomSeed() ); break;
            default: startDemo(); break;
            }
        }
        break;
    default: break;
    }
}

void Game::draw()
{
    const bool lastFrame = m_options.m_quitAfter > 0.0f && m_time >= m_options.m_quitAfter;
    BeginDrawing();
    ClearBackground( BLACK );

    SceneInput in;
    in.m_track = m_track.get();
    in.m_world = &m_world;
    in.m_models = &m_models;
    in.m_player = &m_car;
    in.m_car = m_carIndex;
    in.m_paint = m_paintIndex;
    in.m_traffic = &m_traffic;
    in.m_particles = &m_particles;
    in.m_env = environmentAt( *m_track, m_car.m_s + 30.0 );
    in.m_surface = surfaceAt( *m_track, m_car.m_s + 30.0 );
    in.m_time = m_time;
    in.m_dt = m_state == StateE::PAUSED ? 0.0f : m_dt;
    in.m_flash = m_flash;
    in.m_speedBlur = m_blur;
    in.m_aberration = m_aberration;
    in.m_windStreaks = smoothstepf( 0.62f, 0.95f, m_car.m_speed / m_car.m_spec.m_topSpeed ) + ( m_car.m_boosting ? 0.5f : 0.0f );
    in.m_drawPlayer = !( m_settings.m_camera == 2 && ( m_state == StateE::RACE || m_state == StateE::COUNTDOWN || m_state == StateE::PAUSED ||
                                                       m_state == StateE::GAMEOVER || ( m_state == StateE::SETTINGS && m_settingsReturn != StateE::TITLE ) ) );
    m_renderer.render( in, m_cam, m_settings.m_renderScale );

    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = std::min( H / 720.0f, W / 1150.0f );
    switch( m_state )
    {
    case StateE::TITLE: drawTitle(); break;
    case StateE::SETTINGS:
    {
        if( m_settingsReturn != StateE::TITLE ) drawHud();
        DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 110 } );
        static const char *cams[] = { "CHASE", "FAR CHASE", "BUMPER" };
        auto bar = []( float v )
        {
            std::string s;
            const int n = static_cast<int>( std::round( v * 10.0f ) );
            for( int i = 0; i < 10; ++i ) s += i < n ? '#' : '-';
            return s;
        };
        drawMenu( "SETTINGS",
                  { "DISPLAY", "3D RESOLUTION", "VSYNC", "GEARS", "SPEEDOMETER", "CAMERA", "SPEED BLUR", "MUSIC", "EFFECTS", "RADIO",
                    "BACK" },
                  { m_settings.m_display == DisplayModeE::FULLSCREEN ? "FULLSCREEN" : "WINDOW",
                    format( "%d%%", static_cast<int>( m_settings.m_renderScale * 100.0f + 0.5f ) ), m_settings.m_vsync ? "ON" : "OFF",
                    m_settings.m_manualGears ? "MANUAL" : "AUTOMATIC", m_settings.m_units == UnitsE::MPH ? "MPH" : "KM/H",
                    cams[m_settings.m_camera], m_settings.m_speedBlur ? "ON" : "OFF", bar( m_settings.m_musicVolume ),
                    bar( m_settings.m_sfxVolume ), radioName(), "" },
                  H * 0.18f );
        break;
    }
    case StateE::COUNTDOWN:
    {
        drawHud();
        const int n = 3 - static_cast<int>( m_stateTime );
        if( n >= 1 && n <= 3 )
        {
            const float k = 1.0f - std::fmod( m_stateTime, 1.0f );
            drawTextFx( std::to_string( n ), W * 0.5f, H * 0.32f - 30.0f * u * k, ( 120.0f + 60.0f * k ) * u, Accent, AlignE::CENTER, 1.0f );
        }
        break;
    }
    case StateE::RACE: drawHud(); break;
    case StateE::PAUSED:
        drawHud();
        DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 120 } );
        drawMenu( "PAUSE", { "RESUME", "RESTART TRACK", "NEXT SONG", "SETTINGS", "QUIT TO TITLE" }, {}, H * 0.28f );
        break;
    case StateE::GAMEOVER: drawResults(); break;
    }

    if( m_nowTimer < 4.5f && !m_nowPlaying.empty() )
    {
        const float a = std::min( 1.0f, m_nowTimer * 4.0f ) * std::min( 1.0f, ( 4.5f - m_nowTimer ) * 1.5f );
        const float y = H - ( m_state == StateE::TITLE ? 76.0f : 40.0f ) * u;
        Color       lab = Accent;
        lab.a = static_cast<unsigned char>( 255.0f * a );
        Color txt = Ink;
        txt.a = lab.a;
        drawTextFx( "RADIO  " + radioName(), W * 0.5f, y - 22.0f * u, 11.0f * u, lab, AlignE::CENTER, 0.0f );
        drawTextFx( m_nowPlaying, W * 0.5f, y, 15.0f * u, txt, AlignE::CENTER, 0.2f * a );
    }

    if( lastFrame )
    {
        if( !m_options.m_screenshot.empty() )
        {
            // LoadImageFromScreen() scales by the desktop's DPI even when the framebuffer is not scaled.
            rlDrawRenderBatchActive();
            const int w = GetRenderWidth(), h = GetRenderHeight();
            Image     img = { rlReadScreenPixels( w, h ), w, h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
            ExportImage( img, m_options.m_screenshot.c_str() );
            UnloadImage( img );
        }
        m_quit = true;
    }
    EndDrawing();
}

void Game::drawMenu( const char *title, const std::vector<std::string> &items, const std::vector<std::string> &values, float y )
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = std::min( H / 720.0f, W / 1150.0f );
    const float rowH = 40.0f * u;
    const float w = ( values.empty() ? 460.0f : 720.0f ) * u;
    const float x = W * 0.5f - w * 0.5f;
    const float h = 90.0f * u + rowH * static_cast<float>( items.size() );
    panel( x, y, w, h );
    drawTextFx( title, W * 0.5f, y + 22.0f * u, 30.0f * u, Accent, AlignE::CENTER, 0.8f );
    for( size_t i = 0; i < items.size(); ++i )
    {
        const float ry = y + 76.0f * u + rowH * static_cast<float>( i );
        const bool  sel = static_cast<int>( i ) == m_menuIndex;
        if( sel )
        {
            const float pulse = 0.6f + 0.4f * std::sin( m_time * 6.0f );
            DrawRectangleRounded( Rectangle{ x + 16.0f * u, ry - 8.0f * u, w - 32.0f * u, rowH - 4.0f * u }, 0.3f, 6,
                                  Color{ 255, 150, 40, static_cast<unsigned char>( 50 + 40 * pulse ) } );
        }
        const Color c = sel ? Ink : Dim;
        if( values.empty() || values[i].empty() )
        {
            drawText( items[i], W * 0.5f, ry, 20.0f * u, c, AlignE::CENTER );
        }
        else
        {
            drawText( items[i], x + 40.0f * u, ry, 18.0f * u, c, AlignE::LEFT );
            const std::string v = sel ? "< " + values[i] + " >" : values[i];
            drawText( v, x + w - 40.0f * u, ry, 18.0f * u, sel ? Accent : c, AlignE::RIGHT );
        }
    }
    const char *hint = m_input.m_gamepad ? "A  SELECT     B  BACK     D-PAD  CHANGE" : "ENTER  SELECT     ESC  BACK     ARROWS  CHANGE";
    drawText( hint, W * 0.5f, y + h + 18.0f * u, 13.0f * u, Color{ 220, 224, 235, 200 }, AlignE::CENTER );
}

void Game::drawTitle()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = std::min( H / 720.0f, W / 1150.0f );
    DrawRectangleGradientV( 0, 0, static_cast<int>( W ), static_cast<int>( H * 0.45f ), Color{ 0, 0, 0, 150 }, Color{ 0, 0, 0, 0 } );

    // The emblem: a striped sun behind the name.
    const Vector2 c = { W * 0.5f, 150.0f * u };
    const float   r = 74.0f * u;
    for( int i = 0; i < 24; ++i )
    {
        const float y0 = -r + 2.0f * r * static_cast<float>( i ) / 24.0f;
        const float y1 = y0 + 2.0f * r / 24.0f;
        const float t = static_cast<float>( i ) / 23.0f;
        if( t > 0.5f && ( i % 3 ) == 0 ) continue;
        const float yc = ( y0 + y1 ) * 0.5f;
        const float hw = std::sqrt( std::max( 0.0f, r * r - yc * yc ) );
        const Color col = lerpColor( Color{ 255, 214, 80, 230 }, Color{ 255, 40, 140, 230 }, t );
        DrawRectangle( static_cast<int>( c.x - hw ), static_cast<int>( c.y + y0 ), static_cast<int>( hw * 2.0f ),
                       static_cast<int>( y1 - y0 + 1.0f ), col );
    }
    drawTextFx( "SUNSTREAK", W * 0.5f, 118.0f * u, 76.0f * u, Ink, AlignE::CENTER, 1.0f );
    drawText( "ENDLESS ARCADE RACING", W * 0.5f, 214.0f * u, 16.0f * u, Accent, AlignE::CENTER );

    const CarSpec &spec = carSpec( m_carIndex );
    drawMenu( "", { "START ENDLESS RUN", "CAR", "PAINT", "SETTINGS", "QUIT" },
              { "", spec.m_name, paintName( m_paintIndex ), "", "" }, H * 0.42f );

    // The car's figures.
    const float px = W * 0.5f + 380.0f * u;
    if( px + 220.0f * u < W )
    {
        panel( px, H * 0.42f, 210.0f * u, 150.0f * u );
        const float vals[3] = { ( spec.m_topSpeed - 60.0f ) / 32.0f, ( spec.m_accel - 0.8f ) / 0.45f, ( spec.m_grip - 0.8f ) / 0.4f };
        const char *names[3] = { "TOP SPEED", "ACCELERATION", "GRIP" };
        for( int i = 0; i < 3; ++i )
        {
            const float y = H * 0.42f + 20.0f * u + 42.0f * u * static_cast<float>( i );
            drawText( names[i], px + 16.0f * u, y, 12.0f * u, Dim );
            DrawRectangle( static_cast<int>( px + 16.0f * u ), static_cast<int>( y + 20.0f * u ), static_cast<int>( 178.0f * u ),
                           static_cast<int>( 8.0f * u ), Color{ 255, 255, 255, 40 } );
            DrawRectangle( static_cast<int>( px + 16.0f * u ), static_cast<int>( y + 20.0f * u ),
                           static_cast<int>( 178.0f * u * clampf( vals[i], 0.08f, 1.0f ) ), static_cast<int>( 8.0f * u ), Accent );
        }
    }
    if( m_settings.m_bestScore > 0 )
        drawText( format( "BEST  %lld   %.1f KM", static_cast<long long>( m_settings.m_bestScore ), m_settings.m_bestDistance / 1000.0 ),
                  W * 0.5f, H - 40.0f * u, 14.0f * u, Dim, AlignE::CENTER );
}

void Game::drawResults()
{
    drawHud();
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = std::min( H / 720.0f, W / 1150.0f );
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 130 } );
    const float w = 560.0f * u, h = 300.0f * u;
    const float x = W * 0.5f - w * 0.5f, y = H * 0.12f;
    panel( x, y, w, h );
    drawTextFx( "TIME UP", W * 0.5f, y + 22.0f * u, 40.0f * u, Red, AlignE::CENTER, 1.0f );
    const bool  mph = m_settings.m_units == UnitsE::MPH;
    const std::string rows[6][2] = {
        { "DISTANCE", format( "%.2f KM", ( m_car.m_s - m_startS ) / 1000.0 ) },
        { "SCORE", format( "%lld", static_cast<long long>( m_score ) ) },
        { "STAGE", format( "%d", m_stage + 1 ) },
        { "OVERTAKES", format( "%d", m_overtakes ) },
        { "NEAR MISSES", format( "%d", m_nearMisses ) },
        { "TOP SPEED", format( "%d %s", static_cast<int>( m_topSpeed * ( mph ? 2.23694f : 3.6f ) ), mph ? "MPH" : "KM/H" ) },
    };
    for( int i = 0; i < 6; ++i )
    {
        const float ry = y + 86.0f * u + 30.0f * u * static_cast<float>( i );
        drawText( rows[i][0], x + 50.0f * u, ry, 16.0f * u, Dim );
        drawText( rows[i][1], x + w - 50.0f * u, ry, 16.0f * u, i == 1 && m_newBest ? Accent : Ink, AlignE::RIGHT );
    }
    if( m_newBest ) drawTextFx( "NEW BEST", W * 0.5f, y + h - 34.0f * u, 18.0f * u, Accent, AlignE::CENTER, 1.0f );
    drawText( "TRACK CODE  " + trackCode( m_seed ), W * 0.5f, y + h + 14.0f * u, 14.0f * u, Dim, AlignE::CENTER );
    drawMenu( "", { "RETRY SAME TRACK", "NEW TRACK", "TITLE" }, {}, y + h + 50.0f * u );
}

void Game::drawMinimap( float x, float y, float w, float h )
{
    const Track &track = *m_track;
    panel( x, y, w, h, 140 );
    const TrackFrame cf = track.frameAt( m_car.m_s );
    const Vector3    fwd = headingForward( cf.m_heading );
    const Vector3    right = headingRight( cf.m_heading );
    const Vec3d      c = cf.m_pos;
    const float      scale = h * 0.8f / 1300.0f;
    auto map = [&]( const Vec3d &p )
    {
        const float rx = static_cast<float>( p.m_x - c.m_x ), rz = static_cast<float>( p.m_z - c.m_z );
        const float f = rx * fwd.x + rz * fwd.z;
        const float r = rx * right.x + rz * right.z;
        return Vector2{ x + w * 0.5f + r * scale, y + h * 0.88f - f * scale };
    };
    BeginScissorMode( static_cast<int>( x ), static_cast<int>( y ), static_cast<int>( w ), static_cast<int>( h ) );
    Vector2 prev = map( track.pointAt( m_car.m_s - 80.0, 0.0f, 0.0f ) );
    const float u = h / 150.0f;
    for( double d = -55.0; d <= 1250.0; d += 25.0 )
    {
        const Vector2 p = map( track.pointAt( m_car.m_s + d, 0.0f, 0.0f ) );
        DrawLineEx( prev, p, 5.0f * u, Color{ 235, 238, 245, 200 } );
        prev = p;
    }
    if( m_nextCheckpoint - m_car.m_s < 1250.0 && !m_demo )
        DrawCircleV( map( track.pointAt( m_nextCheckpoint, 0.0f, 0.0f ) ), 5.0f * u, Green );
    for( const TrafficCar &t : m_traffic.m_cars )
    {
        if( t.m_s - m_car.m_s > 1250.0 ) continue;
        DrawCircleV( map( track.pointAt( t.m_s, t.m_x, 0.0f ) ), 2.6f * u, Color{ 255, 200, 80, 255 } );
    }
    const Vector2 me = map( track.pointAt( m_car.m_s, m_car.m_x, 0.0f ) );
    DrawTriangle( Vector2{ me.x, me.y - 7.0f * u }, Vector2{ me.x - 5.0f * u, me.y + 5.0f * u }, Vector2{ me.x + 5.0f * u, me.y + 5.0f * u }, Accent );
    EndScissorMode();
}

void Game::drawHud()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = std::min( H / 720.0f, W / 1150.0f );
    const bool  mph = m_settings.m_units == UnitsE::MPH;
    auto        txt = []( std::string_view t, float x, float y, float s, Color c, AlignE a = AlignE::LEFT ) { drawTextFx( t, x, y, s, c, a, 0.0f ); };

    // The clock.
    const bool  low = m_timeLeft < 10.0f && !m_timeUp;
    const float pulse = low ? 0.5f + 0.5f * std::sin( m_time * 12.0f ) : 0.0f;
    const Color tc = low ? lerpColor( Ink, Red, 0.5f + 0.5f * pulse ) : Ink;
    txt( "TIME", W * 0.5f, 12.0f * u, 13.0f * u, Dim, AlignE::CENTER );
    drawTextFx( format( "%d", static_cast<int>( std::ceil( m_timeLeft ) ) ), W * 0.5f, 32.0f * u, ( 54.0f + 6.0f * pulse ) * u, tc,
                AlignE::CENTER, 0.6f );

    // Score, distance, stage.
    txt( "SCORE", 28.0f * u, 14.0f * u, 13.0f * u, Dim );
    drawTextFx( format( "%lld", static_cast<long long>( m_score ) ), 28.0f * u, 34.0f * u, 28.0f * u, Ink, AlignE::LEFT, 0.3f );
    txt( format( "%.2f KM", ( m_car.m_s - m_startS ) / 1000.0 ), 28.0f * u, 74.0f * u, 15.0f * u, Ink );
    const BiomeE biome = m_track->stageBiome( static_cast<int>( m_car.m_s / StageLength ) );
    txt( format( "STAGE %d  %s", static_cast<int>( m_car.m_s / StageLength ) + 1, biomeDef( biome ).m_name ), 28.0f * u, 98.0f * u,
              12.0f * u, Accent );

    // Records and combo.
    txt( format( "BEST %lld", static_cast<long long>( std::max<int64_t>( m_settings.m_bestScore, 0 ) ) ), W - 28.0f * u, 14.0f * u,
              13.0f * u, Dim, AlignE::RIGHT );
    txt( format( "OVERTAKES %d", m_overtakes ), W - 28.0f * u, 38.0f * u, 13.0f * u, Ink, AlignE::RIGHT );
    if( m_combo > 1 ) drawTextFx( format( "COMBO X%d", m_combo ), W - 28.0f * u, 62.0f * u, 20.0f * u, Cyan, AlignE::RIGHT, 0.8f );

    // Speed, revs, gear, boost.
    const float speed = m_car.displaySpeed( mph );
    const float right = W - 34.0f * u;
    drawTextFx( format( "%d", static_cast<int>( speed ) ), right, H - 120.0f * u, 64.0f * u, Ink, AlignE::RIGHT, 0.4f );
    txt( mph ? "MPH" : "KM/H", right, H - 44.0f * u, 13.0f * u, Dim, AlignE::RIGHT );
    const int   segs = 22;
    const float barW = 300.0f * u;
    const float x0 = right - barW;
    for( int i = 0; i < segs; ++i )
    {
        const float t = static_cast<float>( i ) / static_cast<float>( segs - 1 );
        const float sw = barW / static_cast<float>( segs ) - 3.0f * u;
        const float sh = ( 8.0f + 14.0f * t ) * u;
        const bool  lit = t <= m_car.m_rpm / 1.0f;
        Color       col = t < 0.6f ? Color{ 90, 255, 150, 255 } : t < 0.85f ? Color{ 255, 220, 60, 255 } : Red;
        if( !lit ) col = Color{ 255, 255, 255, 35 };
        DrawRectangle( static_cast<int>( x0 + static_cast<float>( i ) * barW / static_cast<float>( segs ) ),
                       static_cast<int>( H - 142.0f * u - sh ), static_cast<int>( sw ), static_cast<int>( sh ), col );
    }
    const float gx = x0 - 6.0f * u;
    DrawRectangleRounded( Rectangle{ gx - 56.0f * u, H - 120.0f * u, 54.0f * u, 64.0f * u }, 0.2f, 6, Color{ 8, 10, 22, 170 } );
    drawTextFx( format( "%d", m_car.m_gear + 1 ), gx - 29.0f * u, H - 108.0f * u, 40.0f * u, m_settings.m_manualGears ? Accent : Ink,
                AlignE::CENTER, 0.3f );
    txt( "BOOST", x0, H - 30.0f * u, 11.0f * u, Cyan );
    const float bx = x0 + 70.0f * u, bw = right - bx;
    DrawRectangle( static_cast<int>( bx ), static_cast<int>( H - 30.0f * u ), static_cast<int>( bw ), static_cast<int>( 10.0f * u ),
                   Color{ 255, 255, 255, 35 } );
    const Color boostCol = m_car.m_boosting ? Color{ 160, 240, 255, 255 } : Cyan;
    DrawRectangle( static_cast<int>( bx ), static_cast<int>( H - 30.0f * u ), static_cast<int>( bw * m_car.m_boostTank ),
                   static_cast<int>( 10.0f * u ), boostCol );

    // The road ahead and the next checkpoint.
    const float mw = 210.0f * u, mh = 150.0f * u;
    drawMinimap( 24.0f * u, H - mh - 24.0f * u, mw, mh );
    if( !m_demo )
        txt( format( "CHECKPOINT %.1f KM", std::max( 0.0, m_nextCheckpoint - m_car.m_s ) / 1000.0 ), 24.0f * u,
                  H - mh - 46.0f * u, 12.0f * u, Green );

    // Messages.
    float py = H * 0.3f;
    for( const Popup &p : m_popups )
    {
        const float in = std::min( 1.0f, p.m_age * 6.0f );
        const float out = std::min( 1.0f, ( p.m_life - p.m_age ) * 3.0f );
        const float k = in * out;
        const float size = 40.0f * u * p.m_size * ( 1.3f - 0.3f * in );
        Color       c = p.m_color;
        c.a = static_cast<unsigned char>( 255.0f * k );
        drawTextFx( p.m_text, W * 0.5f, py, size, c, AlignE::CENTER, 0.9f * k );
        py += size * 1.35f;
        if( !p.m_sub.empty() )
        {
            Color s = Ink;
            s.a = c.a;
            txt( p.m_sub, W * 0.5f, py, size * 0.5f, s, AlignE::CENTER );
            py += size * 0.9f;
        }
    }
}

} // namespace sunstreak
