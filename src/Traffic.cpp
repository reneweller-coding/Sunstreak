/**
 * @file Traffic.cpp
 * @brief Traffic driving, overtakes, near misses, collisions and flying cones.
 * @author reneweller
 */
#include "Traffic.h"

#include <algorithm>

namespace sunstreak
{

namespace
{

/**
 * @brief Whether a lane is closed somewhere ahead.
 * @param[in] track     the road
 * @param[in] s         from here
 * @param[in] lane      the lane
 * @param[in] distance  how far to look
 * @return true if roadworks close it
 */
bool laneClosedAhead( const Track &track, double s, int lane, double distance )
{
    const long first = static_cast<long>( s / SegmentLength );
    const long last = static_cast<long>( ( s + distance ) / SegmentLength );
    for( long i = first; i <= last; ++i )
        if( track.segment( i ).m_blockedLanes & ( 1u << lane ) ) return true;
    return false;
}

} // namespace

void Traffic::reset( uint64_t seed, double startS )
{
    m_cars.clear();
    m_obstacles.clear();
    m_rng = Rng( seed ^ 0x7AFF1Cull );
    m_spawnS = startS + 120.0;
    m_scanned = 0;
}

bool Traffic::laneFree( int lane, double s, const TrafficCar *self ) const
{
    for( const TrafficCar &c : m_cars )
    {
        if( &c == self || c.m_lane != lane ) continue;
        if( c.m_s > s - 18.0 && c.m_s < s + 28.0 ) return false;
    }
    return true;
}

void Traffic::scanRoadworks( const Track &track, double upTo )
{
    const long last = std::min( static_cast<long>( upTo / SegmentLength ), track.endIndex() - 2 );
    m_scanned = std::max( m_scanned, track.firstIndex() + 1 );
    for( ; m_scanned <= last; ++m_scanned )
    {
        const Segment &seg = track.segment( m_scanned );
        if( !( seg.m_flags & SEG_ROADWORKS ) || seg.m_blockedLanes == 0 ) continue;
        const int   lanes = seg.m_lanes;
        const bool  leftClosed = seg.m_blockedLanes & 1u;
        const float w = 2.0f * seg.m_halfWidth / static_cast<float>( lanes );
        const float boundary = leftClosed ? -seg.m_halfWidth + w : seg.m_halfWidth - w;
        const double s = static_cast<double>( m_scanned ) * SegmentLength;
        const bool  zoneStart = !( track.segment( m_scanned - 1 ).m_flags & SEG_ROADWORKS );
        if( zoneStart )
        {
            // A taper of cones from the road's edge to the lane line, the arrow board in the closed lane.
            const float edge = leftClosed ? -seg.m_halfWidth + 0.4f : seg.m_halfWidth - 0.4f;
            for( int k = 0; k < 5; ++k )
            {
                Obstacle o;
                o.m_s = s - 24.0 + 6.0 * k;
                o.m_x = lerpf( edge, boundary, static_cast<float>( k ) / 4.0f );
                m_obstacles.push_back( o );
            }
            Obstacle board;
            board.m_s = s - 30.0;
            board.m_x = leftClosed ? -seg.m_halfWidth + w * 0.5f : seg.m_halfWidth - w * 0.5f;
            board.m_kind = leftClosed ? 2 : 1;
            m_obstacles.push_back( board );
        }
        if( m_scanned % 2 == 0 )
        {
            Obstacle o;
            o.m_s = s;
            o.m_x = boundary;
            m_obstacles.push_back( o );
        }
    }
}

void Traffic::update( float dt, const Track &track, PlayerCar &player, const Models &models, TrafficEvents &ev )
{
    // Spawn ahead, beyond the fog.
    const BiomeMix  mix = track.biomeMix( player.m_s + 900.0 );
    const float     density = lerpf( biomeDef( mix.m_from ).m_traffic, biomeDef( mix.m_to ).m_traffic, mix.m_t ) *
                          std::min( 1.8f, 0.85f + 0.12f * static_cast<float>( player.m_s / StageLength ) ) * tuning().m_traffic;
    if( m_spawnS < player.m_s + 220.0 ) m_spawnS = player.m_s + 220.0;
    while( m_spawnS < player.m_s + 1050.0 )
    {
        m_spawnS += static_cast<double>( lerpf( 200.0f, 60.0f, std::min( 1.0f, density / 1.8f ) ) * m_rng.range( 0.55f, 1.45f ) );
        const Segment &seg = track.segment( static_cast<long>( m_spawnS / SegmentLength ) );
        if( m_spawnS < StartLine + 150.0 ) continue;
        TrafficCar c;
        c.m_s = m_spawnS;
        int lane = m_rng.rangeInt( 0, seg.m_lanes - 1 );
        for( int tries = 0; tries < seg.m_lanes && laneClosedAhead( track, c.m_s - 40.0, lane, 80.0 ); ++tries )
            lane = ( lane + 1 ) % seg.m_lanes;
        if( laneClosedAhead( track, c.m_s - 40.0, lane, 80.0 ) || !laneFree( lane, c.m_s, nullptr ) ) continue;
        c.m_lane = lane;
        c.m_x = laneCenter( lane, seg.m_lanes, seg.m_halfWidth );
        const int truckCount = 4;
        const int carCount = static_cast<int>( models.m_traffic.size() ) - truckCount;
        const bool truck = m_rng.chance( 0.14f );
        c.m_model = truck ? carCount + m_rng.rangeInt( 0, truckCount - 1 ) : m_rng.rangeInt( 0, carCount - 1 );
        c.m_length = models.m_traffic[c.m_model].m_length;
        c.m_width = models.m_traffic[c.m_model].m_width;
        const float laneBonus = static_cast<float>( seg.m_lanes - 1 - lane ) * 2.0f;
        c.m_cruise = truck ? m_rng.range( 15.0f, 20.0f ) : m_rng.range( 16.0f, 26.0f ) + laneBonus;
        c.m_speed = c.m_cruise;
        m_cars.push_back( c );
    }
    scanRoadworks( track, player.m_s + 900.0 );

    // Drive.
    for( TrafficCar &c : m_cars )
    {
        const Segment &seg = track.segment( static_cast<long>( c.m_s / SegmentLength ) );
        const int      lanes = seg.m_lanes;
        if( c.m_lane >= lanes ) c.m_lane = lanes - 1;
        float want = c.m_cruise;
        // Closed lane ahead: move over.
        if( laneClosedAhead( track, c.m_s, c.m_lane, 120.0 ) )
        {
            for( int d : { -1, 1 } )
            {
                const int l = c.m_lane + d;
                if( l >= 0 && l < lanes && !laneClosedAhead( track, c.m_s, l, 140.0 ) && laneFree( l, c.m_s, &c ) )
                {
                    c.m_lane = l;
                    break;
                }
            }
            if( laneClosedAhead( track, c.m_s, c.m_lane, 50.0 ) ) want = 6.0f;
        }
        // A slower car ahead: overtake or follow.
        for( const TrafficCar &o : m_cars )
        {
            if( &o == &c || o.m_lane != c.m_lane ) continue;
            const double gap = o.m_s - c.m_s;
            if( gap > 0.0 && gap < 20.0 + c.m_speed * 1.2 && o.m_speed < want )
            {
                const int l = c.m_lane + ( m_rng.chance( 0.5f ) ? -1 : 1 );
                if( l >= 0 && l < lanes && laneFree( l, c.m_s, &c ) && !laneClosedAhead( track, c.m_s, l, 100.0 ) )
                    c.m_lane = l;
                else
                    want = std::min( want, o.m_speed - ( gap < 12.0 ? 3.0f : 0.0f ) );
            }
        }
        c.m_speed = approach( c.m_speed, std::max( 0.0f, want ), want < c.m_speed ? 1.6f : 0.5f, dt );
        const float target = laneCenter( c.m_lane, lanes, seg.m_halfWidth );
        c.m_vx = clampf( ( target - c.m_x ) * 1.3f, -2.6f, 2.6f );
        c.m_x += c.m_vx * dt;
        c.m_yaw = std::atan2( c.m_vx, std::max( c.m_speed, 5.0f ) );
        c.m_s += static_cast<double>( c.m_speed ) * dt;
        if( c.m_hitTimer > 0.0f ) c.m_hitTimer -= dt;
    }

    // Contact with the player, overtakes, near misses.
    const float pl = player.m_spec.m_length, pw = player.m_spec.m_width;
    for( TrafficCar &c : m_cars )
    {
        const double ds = player.m_s - c.m_s;
        const float  dx = player.m_x - c.m_x;
        const float  halfL = ( pl + c.m_length ) * 0.5f;
        const float  halfW = ( pw + c.m_width ) * 0.5f * 0.92f;
        const bool   lowEnough = !player.m_airborne || player.m_y - track.surfaceHeight( player.m_s, player.m_x ) < 1.2;
        if( std::abs( ds ) < halfL && std::abs( dx ) < halfW && lowEnough )
        {
            const float rel = player.m_speed - c.m_speed;
            const float side = dx >= 0.0f ? 1.0f : -1.0f;
            if( std::abs( ds ) < halfL * 0.6f )
            {
                // Side by side: shove apart.
                player.m_x += side * ( halfW - std::abs( dx ) ) * 0.9f;
                player.m_vx = side * 3.0f;
                player.m_speed *= 0.97f;
                c.m_x -= side * 0.25f;
                ev.m_hit = std::max( ev.m_hit, 0.35f );
            }
            else if( ds < 0.0 )
            {
                // Rear-ended it.
                if( rel > 18.0f )
                    player.crash( 0.4f, side * 1.4f );
                else
                    player.m_speed = std::max( 0.0f, c.m_speed - 2.0f );
                player.m_s = c.m_s - halfL;
                c.m_speed += std::max( 0.0f, rel ) * 0.4f;
                ev.m_hit = std::max( ev.m_hit, clampf( rel / 30.0f, 0.3f, 1.0f ) );
            }
            else
            {
                // It ran into the player.
                c.m_speed = std::max( 0.0f, player.m_speed - 1.0f );
                c.m_s = player.m_s - halfL;
                player.m_speed += 1.5f;
                ev.m_hit = std::max( ev.m_hit, 0.3f );
            }
            c.m_hitTimer = 1.5f;
            player.m_shake = std::max( player.m_shake, ev.m_hit );
        }
        if( !c.m_passed && ds > halfL )
        {
            c.m_passed = true;
            ++ev.m_overtakes;
            if( std::abs( dx ) < halfW + 1.4f && player.m_speed > 30.0f && c.m_hitTimer <= 0.0f ) ++ev.m_nearMisses;
            const float rel = player.m_speed - c.m_speed;
            if( std::abs( dx ) < 9.0f && rel > 8.0f )
            {
                const float strength = clampf( rel / 50.0f, 0.2f, 1.0f ) * ( 1.0f - std::abs( dx ) / 10.0f );
                if( strength > ev.m_whoosh )
                {
                    ev.m_whoosh = strength;
                    ev.m_whooshPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
                }
            }
        }
        else if( c.m_passed && ds < -halfL )
        {
            c.m_passed = false;
        }
    }
    std::erase_if( m_cars, [&]( const TrafficCar &c ) { return c.m_s < player.m_s - 140.0 || c.m_s > player.m_s + 1700.0; } );

    // Cones and boards.
    const TrackFrame pf = track.frameAt( player.m_s );
    for( Obstacle &o : m_obstacles )
    {
        if( o.m_knocked )
        {
            o.m_life += dt;
            o.m_vel.y -= 9.81f * dt;
            o.m_pos.m_x += static_cast<double>( o.m_vel.x * dt );
            o.m_pos.m_y += static_cast<double>( o.m_vel.y * dt );
            o.m_pos.m_z += static_cast<double>( o.m_vel.z * dt );
            o.m_angle += o.m_spinRate * dt;
            if( o.m_pos.m_y < o.m_groundY && o.m_vel.y < 0.0f )
            {
                o.m_pos.m_y = o.m_groundY;
                o.m_vel = Vector3{ o.m_vel.x * 0.6f, -o.m_vel.y * 0.35f, o.m_vel.z * 0.6f };
                o.m_spinRate *= 0.6f;
            }
            continue;
        }
        const double ds = player.m_s - o.m_s;
        const float  dx = player.m_x - o.m_x;
        const float  reachL = pl * 0.5f + ( o.m_kind ? 0.3f : 0.25f );
        const float  reachW = pw * 0.5f + ( o.m_kind ? 1.0f : 0.25f );
        if( std::abs( ds ) < reachL && std::abs( dx ) < reachW && !player.m_airborne )
        {
            o.m_knocked = true;
            o.m_pos = track.pointAt( o.m_s, o.m_x, 0.0f );
            o.m_groundY = static_cast<float>( o.m_pos.m_y );
            const float side = dx >= 0.0f ? -1.0f : 1.0f;
            o.m_vel = Vector3Add( Vector3Scale( pf.m_forward, player.m_speed * m_rng.range( 0.7f, 1.0f ) ),
                                  Vector3Add( Vector3{ 0, m_rng.range( 3.0f, 7.0f ), 0 },
                                              Vector3Scale( pf.m_right, side * m_rng.range( 1.0f, 5.0f ) ) ) );
            o.m_axis = Vector3Normalize( Vector3{ m_rng.range( -1, 1 ), m_rng.range( -0.3f, 0.3f ), m_rng.range( -1, 1 ) } );
            o.m_spinRate = m_rng.range( 8.0f, 16.0f );
            player.m_speed *= o.m_kind ? 0.82f : 0.97f;
            player.m_shake = std::max( player.m_shake, o.m_kind ? 0.5f : 0.18f );
            ++ev.m_cones;
        }
    }
    std::erase_if( m_obstacles, [&]( const Obstacle &o ) { return o.m_life > 3.0f || ( !o.m_knocked && o.m_s < player.m_s - 60.0 ); } );
}

} // namespace sunstreak
