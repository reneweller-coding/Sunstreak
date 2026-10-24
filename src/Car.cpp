/**
 * @file Car.cpp
 * @brief Arcade car physics.
 * @author reneweller
 */
#include "Car.h"

#include "Local.h"

#include <fstream>

namespace sunstreak
{

namespace
{

constexpr float GearShare[GearCount] = { 0.25f, 0.41f, 0.56f, 0.71f, 0.86f, 1.0f };      ///< gear top / top speed
constexpr float GearPull[GearCount] = { 1.0f, 0.86f, 0.73f, 0.61f, 0.51f, 0.43f };      ///< drive per gear
constexpr float BoostTopShare = 1.14f;      ///< boost lets the engine run past the top speed by this much
constexpr float ShiftTime = 0.16f;          ///< seconds without drive per shift

/**
 * @brief Engine torque over the rev range.
 * @param[in] r  revs, 0..1 of the redline
 * @return 0.55..1
 */
float torque( float r )
{
    const float k = 2.0f * r - 1.15f;
    return std::max( 0.55f, 0.72f + 0.28f * ( 1.0f - k * k ) );
}

} // namespace

Tuning &tuning()
{
    static Tuning t;
    return t;
}

Tuning applyDifficulty( const Tuning &base, int difficulty )
{
    Tuning t = base;
    if( difficulty == static_cast<int>( DifficultyE::EASY ) )
    {
        t.m_centrifugal *= 0.85f;
        t.m_grip *= 1.08f;
        t.m_crashKeep = std::max( t.m_crashKeep, 0.4f );
        t.m_time *= 1.25f;
        t.m_traffic *= 0.7f;
        t.m_rivals *= 0.93f;
    }
    else if( difficulty == static_cast<int>( DifficultyE::HARD ) )
    {
        t.m_centrifugal *= 1.1f;
        t.m_grip *= 0.95f;
        t.m_crashKeep *= 0.7f;
        t.m_time *= 0.88f;
        t.m_traffic *= 1.3f;
        t.m_rivals *= 1.04f;
    }
    return t;
}

Tuning loadTuning( const std::string &path )
{
    Tuning t;
    std::ifstream in( path );
    if( !in )
    {
        std::ofstream out( path );
        out << "# Sunstreak handling. Change a value, save, restart the game. Delete the file to get the defaults back.\n"
            << "# The difficulty (Settings) scales some of these further.\n"
            << "steer=" << t.m_steer << "            # sideways speed at full lock (m/s); higher turns sharper\n"
            << "centrifugal=" << t.m_centrifugal << "    # how hard bends push outwards; lower is easier\n"
            << "accel=" << t.m_accel << "           # acceleration (m/s^2 in first gear)\n"
            << "brake=" << t.m_brake << "             # braking (m/s^2)\n"
            << "boost=" << t.m_boost << "            # extra acceleration while boosting\n"
            << "grip=" << t.m_grip << "               # grip factor on every surface\n"
            << "crash_keep=" << t.m_crashKeep << "      # share of the speed left after a crash\n"
            << "time=" << t.m_time << "               # time bonus factor in endless runs\n"
            << "traffic=" << t.m_traffic << "            # traffic density factor\n"
            << "rivals=" << t.m_rivals << "             # championship rivals' pace\n";
        return t;
    }
    std::string line;
    while( std::getline( in, line ) )
    {
        const auto hash = line.find( '#' );
        if( hash != std::string::npos ) line = line.substr( 0, hash );
        const auto eq = line.find( '=' );
        if( eq == std::string::npos ) continue;
        const std::string key = line.substr( 0, eq );
        float             v = 0.0f;
        try
        {
            v = std::stof( line.substr( eq + 1 ) );
        }
        catch( ... )
        {
            continue;
        }
        if( key == "steer" ) t.m_steer = clampf( v, 4.0f, 40.0f );
        else if( key == "centrifugal" ) t.m_centrifugal = clampf( v, 0.0f, 2.0f );
        else if( key == "accel" ) t.m_accel = clampf( v, 2.0f, 40.0f );
        else if( key == "brake" ) t.m_brake = clampf( v, 4.0f, 80.0f );
        else if( key == "boost" ) t.m_boost = clampf( v, 0.0f, 30.0f );
        else if( key == "grip" ) t.m_grip = clampf( v, 0.3f, 3.0f );
        else if( key == "crash_keep" ) t.m_crashKeep = clampf( v, 0.0f, 1.0f );
        else if( key == "time" ) t.m_time = clampf( v, 0.3f, 5.0f );
        else if( key == "traffic" ) t.m_traffic = clampf( v, 0.0f, 4.0f );
        else if( key == "rivals" ) t.m_rivals = clampf( v, 0.5f, 1.5f );
    }
    return t;
}

Pose roadPose( const Track &track, double s, float x, float yaw )
{
    const TrackFrame f = track.frameAt( s );
    Pose             p;
    p.m_pos = track.pointAt( s, x, 0.0f );
    const float   c = std::cos( yaw ), sn = std::sin( yaw );
    const Vector3 fwd = Vector3Add( Vector3Scale( f.m_forward, c ), Vector3Scale( f.m_right, sn ) );
    const Vector3 right = Vector3Normalize( Vector3CrossProduct( fwd, f.m_up ) );
    p.m_forward = Vector3Normalize( fwd );
    p.m_up = Vector3CrossProduct( right, p.m_forward );
    p.m_left = Vector3Negate( right );
    return p;
}

Pose PlayerCar::pose( const Track &track ) const
{
    const TrackFrame f = track.frameAt( m_s );
    Pose             p;
    p.m_pos = track.pointAt( m_s, m_x, 0.0f );
    p.m_pos.m_y = m_y;
    const float yaw = m_yawVisual + m_spin;
    Vector3     fwd = Vector3Add( Vector3Scale( f.m_forward, std::cos( yaw ) ), Vector3Scale( f.m_right, std::sin( yaw ) ) );
    Vector3     right = Vector3Normalize( Vector3CrossProduct( fwd, f.m_up ) );
    Vector3     up = f.m_up;
    if( m_airborne )
    {
        // Nose follows the flight path, a little.
        const float pitch = clampf( std::atan2( m_vy, std::max( m_speed, 10.0f ) ) - std::atan( f.m_grade ), -0.35f, 0.35f ) * 0.6f;
        const Vector3 nf = Vector3Add( Vector3Scale( fwd, std::cos( pitch ) ), Vector3Scale( up, std::sin( pitch ) ) );
        up = Vector3Subtract( Vector3Scale( up, std::cos( pitch ) ), Vector3Scale( fwd, std::sin( pitch ) ) );
        fwd = nf;
    }
    // Body roll towards the outside of the turn.
    const float roll = clampf( ( m_vx - m_drift ) * 0.01f, -0.07f, 0.07f );
    up = Vector3Normalize( Vector3Subtract( Vector3Scale( up, std::cos( roll ) ), Vector3Scale( right, std::sin( roll ) ) ) );
    right = Vector3Normalize( Vector3CrossProduct( fwd, up ) );
    p.m_forward = Vector3Normalize( Vector3CrossProduct( up, right ) );
    p.m_up = up;
    p.m_left = Vector3Negate( right );
    return p;
}

float PlayerCar::gearTop( int gear ) const
{
    return GearShare[std::clamp( gear, 0, GearCount - 1 )] * m_spec.m_topSpeed;
}

void PlayerCar::reset( const CarSpec &spec, double s, float x, const Track &track )
{
    const CarSpec keep = spec;
    *this = PlayerCar();
    m_spec = keep;
    m_s = s;
    m_x = x;
    m_y = track.surfaceHeight( s, x );
    m_prevRoadY = m_y;
}

void PlayerCar::crash( float keep, float push )
{
    if( m_crashTimer > 0.0f ) return;
    m_speed *= keep;
    m_x += push;
    m_vx = 0.0f;
    m_crashTimer = 1.1f;
    m_spin = 0.0f;
    m_shake = 1.0f;
}

void PlayerCar::update( const CarInput &rawIn, const Track &track, float dt, bool manual )
{
    m_eventShift = false;
    m_eventLanded = false;
    m_eventBackfire = false;
    CarInput in = rawIn;
    if( m_crashTimer > 0.0f || m_coasting )
    {
        in.m_throttle = 0.0f;
        in.m_boost = false;
        if( m_crashTimer > 0.0f ) in.m_steer *= 0.2f;
    }

    const TrackFrame f = track.frameAt( m_s );
    const Segment   &seg = track.segment( static_cast<long>( m_s / SegmentLength ) );
    const BiomeDef  &biome = biomeDef( static_cast<BiomeE>( seg.m_blend < 0.5f ? seg.m_biome : seg.m_biomeTo ) );
    const float      g = Gravity * biome.m_gravity;

    // Surface: kerb, verge, puddles.
    const float ax = std::abs( m_x );
    m_rumble = ( ax > f.m_halfWidth - 0.6f && ax < f.m_halfWidth + RumbleWidth + 0.4f && !m_airborne ) ? 1.0f : 0.0f;
    if( biome.m_cobbles && ax < f.m_halfWidth && !m_airborne ) m_rumble = std::max( m_rumble, 0.35f );   // cobbles shake
    m_offroad = m_airborne ? 0.0f : smoothstepf( f.m_halfWidth + RumbleWidth, f.m_halfWidth + RumbleWidth + 1.5f, ax );
    // Gravel and sand hold less, except for a car built for them.
    m_unpaved = biome.m_unpaved;
    if( !m_unpaved && biome.m_gravel > 0.0f && gravelAt( track, m_s ) > 0.5f ) m_unpaved = 1;
    float grip = biome.m_grip * m_weatherGrip * ( m_unpaved ? m_spec.m_dirt * ( m_unpaved == 2 ? 0.94f : 1.0f ) : 1.0f );
    const long segIndex = static_cast<long>( m_s / SegmentLength );
    // A pothole under a wheel: a thump, and a little of the speed.
    float holeX = 0.0f, holeR = 0.0f;
    if( !m_airborne && segIndex != m_lastPothole && potholeAt( track, segIndex, holeX, holeR ) && std::abs( m_x - holeX ) < holeR + 0.8f )
    {
        m_lastPothole = segIndex;
        m_speed *= 0.97f;
        m_rumble = 1.0f;
        m_eventLanded = true;
        m_landImpact = 2.5f;
        m_shake = std::max( m_shake, 0.2f );
    }
    if( ( seg.m_flags & SEG_PUDDLE ) && std::abs( m_x - puddleLateral( track, segIndex ) ) < 1.8f ) grip *= 0.3f;
    m_wetGrip = grip;
    grip *= m_spec.m_grip * tuning().m_grip * ( 1.0f - 0.35f * m_offroad );

    // Engine and gears.
    m_throttle = approach( m_throttle, in.m_throttle, 12.0f, dt );
    if( m_shiftTimer > 0.0f ) m_shiftTimer -= dt;
    if( manual )
    {
        if( in.m_shiftUp && m_gear < GearCount - 1 )
        {
            ++m_gear;
            m_shiftTimer = ShiftTime;
            m_eventShift = true;
        }
        if( in.m_shiftDown && m_gear > 0 )
        {
            --m_gear;
            m_shiftTimer = ShiftTime * 0.6f;
            m_eventShift = true;
        }
    }
    else if( !m_frozen )
    {
        const float r = m_speed / gearTop( m_gear );
        if( r > 0.97f && m_gear < GearCount - 1 && in.m_throttle > 0.1f && !m_airborne )
        {
            ++m_gear;
            m_shiftTimer = ShiftTime;
            m_eventShift = true;
        }
        else if( m_gear > 0 && m_speed < gearTop( m_gear - 1 ) * 0.68f )
        {
            --m_gear;
            m_eventShift = true;
        }
    }
    const float topLimit = ( m_boostTank > 0.0f && in.m_boost && m_gear == GearCount - 1 ) ? BoostTopShare : 1.0f;
    const float wheelRpm = m_speed / gearTop( m_gear );
    float       targetRpm = wheelRpm;
    if( m_frozen || m_speed < 9.0f ) targetRpm = std::max( wheelRpm, 0.12f + 0.8f * m_throttle * ( m_frozen ? 1.0f : 1.0f - m_speed / 9.0f ) );
    targetRpm = std::max( targetRpm, 0.12f );
    if( m_airborne ) targetRpm = std::max( targetRpm, 0.12f + 0.95f * m_throttle );
    m_rpm = approach( m_rpm, std::min( targetRpm, topLimit ), m_shiftTimer > 0.0f ? 10.0f : 22.0f, dt );
    if( m_lastThrottle > 0.8f && in.m_throttle < 0.2f && m_rpm > 0.7f ) m_eventBackfire = true;
    m_lastThrottle = in.m_throttle;

    // Longitudinal forces.
    float accel = 0.0f;
    m_boosting = false;
    if( !m_frozen && !m_airborne )
    {
        const bool limiter = wheelRpm >= topLimit;
        if( m_shiftTimer <= 0.0f && !limiter )
            accel += tuning().m_accel * m_spec.m_accel * GearPull[m_gear] * torque( std::min( wheelRpm, 1.0f ) ) * in.m_throttle;
        if( in.m_boost && m_boostTank > 0.0f && in.m_throttle > 0.1f )
        {
            m_boosting = true;
            if( !limiter ) accel += tuning().m_boost;
        }
        accel -= tuning().m_brake * in.m_brake * ( 0.6f + 0.4f * std::min( 1.0f, grip ) );
    }
    if( m_boosting ) m_boostTank = std::max( 0.0f, m_boostTank - 0.22f * dt );
    else m_boostTank = std::min( 1.0f, m_boostTank + 0.015f * dt );

    // Air drag tuned so the top gear tops out at the car's top speed; engine braking when coasting.
    const float topAccel = tuning().m_accel * m_spec.m_accel * GearPull[GearCount - 1] * torque( 1.0f );
    const float dragK = topAccel / ( m_spec.m_topSpeed * m_spec.m_topSpeed * 1.03f );
    accel -= dragK * m_speed * m_speed + 0.25f;
    if( in.m_throttle < 0.05f && !m_airborne ) accel -= 1.2f + 2.0f * m_rpm;
    accel -= 9.0f * m_offroad;
    const float offCap = lerpf( m_spec.m_topSpeed, 0.42f * m_spec.m_topSpeed, m_offroad );
    if( m_speed > offCap ) accel -= 14.0f * m_offroad;
    if( m_crashTimer > 0.0f ) accel -= 8.0f;
    if( !m_airborne ) accel -= f.m_grade * 9.81f * 0.5f;
    if( m_frozen ) accel = 0.0f;
    m_speed = std::max( 0.0f, m_speed + accel * dt );

    // Steering against the bend.
    const float steerTarget = clampf( in.m_steer, -1.0f, 1.0f );
    const bool  returning = std::abs( steerTarget ) < std::abs( m_steer );
    m_steer = approach( m_steer, steerTarget, returning ? 14.0f : 9.0f, dt );
    const float steerMax = steerLimit( m_speed, m_spec.m_topSpeed );
    const float authority = m_airborne ? 0.3f * grip : grip;
    m_vx = approach( m_vx, m_steer * steerMax * authority, 3.0f + 4.0f * std::min( authority, 1.2f ), dt );
    m_drift = m_airborne ? m_drift : f.m_curve * m_speed * m_speed * tuning().m_centrifugal;
    const float lateral = m_vx - m_drift;
    if( !m_frozen ) m_x += lateral * dt;

    const float demand = std::abs( lateral ) / std::max( 1.0f, steerMax * std::max( authority, 0.2f ) );
    float       slip = smoothstepf( 0.6f, 1.05f, std::abs( m_drift ) / std::max( 1.0f, steerMax * std::max( authority, 0.2f ) ) );
    slip = std::max( slip, smoothstepf( 0.75f, 1.0f, std::abs( m_steer ) ) * smoothstepf( 0.6f, 1.0f, demand + 0.4f ) * 0.6f );
    if( m_speed < 10.0f && in.m_throttle > 0.8f && !m_frozen ) slip = std::max( slip, 0.55f );
    if( in.m_brake > 0.7f && m_speed > 15.0f ) slip = std::max( slip, 0.5f );
    m_slip = ( m_airborne || m_offroad > 0.5f ) ? 0.0f : slip * smoothstepf( 2.0f, 14.0f, m_speed );

    // The visible yaw follows the sideways motion; a crash spins the car.
    const float yawTarget = std::atan2( lateral, std::max( m_speed, 6.0f ) ) * 1.5f + m_steer * 0.06f;
    m_yawVisual = approach( m_yawVisual, clampf( yawTarget, -0.6f, 0.6f ), 8.0f, dt );
    if( m_crashTimer > 0.0f )
    {
        m_crashTimer -= dt;
        m_spin += dt * Tau * 1.1f * std::max( 0.2f, m_crashTimer );
        if( m_crashTimer <= 0.0f ) m_spin = 0.0f;
    }

    // Keep the car within reach of the road.
    const float limit = f.m_halfWidth + 60.0f;
    if( ax > limit )
    {
        m_x = std::copysign( limit, m_x );
        m_vx = 0.0f;
    }

    // Advance and follow the road's height; leave it where it falls away faster than gravity.
    if( !m_frozen ) m_s += static_cast<double>( m_speed ) * dt;
    const double roadY = track.surfaceHeight( m_s, m_x );
    if( !m_airborne )
    {
        const float vRoad = static_cast<float>( ( roadY - m_prevRoadY ) / dt );
        if( vRoad < m_vy - g * dt * 1.15f && m_speed > 25.0f )
        {
            m_airborne = true;
            m_airTime = 0.0f;
        }
        else
        {
            m_y = roadY;
            m_vy = vRoad;
        }
    }
    if( m_airborne )
    {
        m_vy -= g * dt;
        m_y += static_cast<double>( m_vy ) * dt;
        m_airTime += dt;
        if( m_y <= roadY )
        {
            const float vRoad = static_cast<float>( ( roadY - m_prevRoadY ) / dt );
            m_landImpact = std::max( 0.0f, vRoad - m_vy );
            m_eventLanded = m_airTime > 0.12f;
            m_airborne = false;
            m_y = roadY;
            m_vy = vRoad;
            if( m_landImpact > 6.0f ) m_speed *= 0.985f;
            m_shake = std::max( m_shake, std::min( 1.0f, m_landImpact / 14.0f ) );
        }
    }
    m_prevRoadY = roadY;

    float shake = 0.0f;
    if( m_rumble > 0.0f ) shake = 0.25f * smoothstepf( 5.0f, 40.0f, m_speed );
    shake = std::max( shake, m_offroad * 0.6f * smoothstepf( 3.0f, 30.0f, m_speed ) );
    m_shake = std::max( approach( m_shake, 0.0f, 3.0f, dt ), shake );
}

} // namespace sunstreak
