/**
 * @file Hero.cpp
 * @brief The local hero: where they turn up, how they drive, who wins.
 * @author Rene Weller
 */
#include "Hero.h"

#include "Local.h"
#include "Models.h"
#include "Phrases.h"
#include "Rng.h"
#include "Traffic.h"

#include <algorithm>
#include <cmath>

namespace sunstreak
{

namespace
{

/**
 * @brief How fast a car may take the bends ahead.
 * @param[in] track  the road
 * @param[in] s      where it is
 * @param[in] speed  how fast it goes (the look ahead grows with it)
 * @param[in] spec   the car
 * @return m/s
 */
float cornerSpeed( const Track &track, double s, float speed, const CarSpec &spec )
{
    const Segment  &seg = track.segment( static_cast<long>( s / SegmentLength ) );
    const BiomeDef &biome = biomeDef( static_cast<BiomeE>( seg.m_blend < 0.5f ? seg.m_biome : seg.m_biomeTo ) );
    float           maxK = 0.0f;
    const float     look = 40.0f + speed * 2.6f;
    for ( float d = 0.0f; d < look; d += SegmentLength )
        maxK = std::max( maxK, std::abs( track.segment( static_cast<long>( ( s + d ) / SegmentLength ) ).m_curve ) );
    const float limit =
        steerLimit( std::max( speed, 25.0f ), spec.m_topSpeed ) * spec.m_grip * biome.m_grip * tuning().m_grip * 0.82f;
    return std::sqrt( limit / ( tuning().m_centrifugal * maxK + 1e-6f ) );
}

}   // namespace

bool stageHero( const Track &track, int stage, double &checkpoint )
{
    const BiomeDef &place = biomeDef( track.stageBiome( stage ) );
    if ( stage < 0 || !place.m_country || place.m_underwater || place.m_surface.m_lineGlow > 0.0f ) return false;
    const uint64_t h = hash64( track.seed() * 4099 + static_cast<uint64_t>( stage ) * 17 + 11 );
    if ( h % 3 != 1 ) return false;   // one stage in three
    // A checkpoint well inside the stage, with the road to the next one free of crossings and forks.
    const double first =
        std::ceil( ( static_cast<double>( stage ) * StageLength + 2000.0 ) / CheckpointSpacing ) * CheckpointSpacing;
    const int count = static_cast<int>( ( StageLength - 2000.0 - CheckpointSpacing - 1500.0 ) / CheckpointSpacing );
    if ( count <= 0 ) return false;
    for ( int k = 0; k < count; ++k )
    {
        checkpoint =
            first + CheckpointSpacing * static_cast<double>( ( static_cast<int>( ( h >> 8 ) % 97 ) + k ) % count );
        bool clear = !track.inForkZone( checkpoint + CheckpointSpacing * 0.5, CheckpointSpacing * 0.5 + 200.0 );
        for ( double d = -HeroApproach; d <= CheckpointSpacing && clear; d += 250.0 )
            clear = track.crossingAt( checkpoint + d ).m_type == CrossingE::NONE;
        if ( clear ) return true;
    }
    return false;
}

void LocalHero::reset()
{
    *this = LocalHero{};
}

void LocalHero::startThief( const Track &track, const PlayerCar &player, double goal )
{
    *this = LocalHero{};
    m_thief = true;
    m_state = HeroStateE::DUEL;
    m_name = "THE THIEF";
    const uint64_t h = hash64( track.seed() * 37 + static_cast<uint64_t>( player.m_s ) );
    m_car = static_cast<int>( h % CarCount );
    m_paint = static_cast<int>( ( h >> 8 ) % PaintCount );
    const Segment &seg = track.segment( static_cast<long>( ( player.m_s + 120.0 ) / SegmentLength ) );
    m_lane = static_cast<int>( ( h >> 16 ) % static_cast<uint64_t>( std::max( 1, static_cast<int>( seg.m_lanes ) ) ) );
    m_s = player.m_s + 120.0;
    m_x = laneCenter( m_lane, seg.m_lanes, seg.m_halfWidth );
    m_speed = std::max( player.m_speed, 25.0f );
    m_start = player.m_s;
    m_goal = goal;
    m_lastStage = static_cast<int>( player.m_s / StageLength );
}

void LocalHero::update( float dt, const Track &track, PlayerCar &player, const Traffic &traffic, bool enabled,
                        HeroEvents &ev )
{
    ev = HeroEvents{};
    if ( m_state == HeroStateE::NONE )
    {
        if ( !enabled ) return;
        const int stage = static_cast<int>( player.m_s / StageLength );
        for ( int st = stage; st <= stage + 1; ++st )
        {
            double cp = 0.0;
            if ( st <= m_lastStage || !stageHero( track, st, cp ) || cp - player.m_s < 150.0 ||
                 cp - player.m_s > HeroApproach )
                continue;
            // Who they are: a name of the country, a car and a paint of their own.
            const BiomeDef &place = biomeDef( track.stageBiome( st ) );
            const Phrases  &words = phrasesOf( place.m_country );
            const uint64_t  h = hash64( track.seed() * 131 + static_cast<uint64_t>( st ) );
            m_lastStage = st;
            m_state = HeroStateE::CLOSING;
            m_country = place.m_country ? place.m_country : "";
            m_name = words.m_names[static_cast<size_t>( h % words.m_names.size() )];
            m_car = static_cast<int>( ( h >> 8 ) % CarCount );
            m_paint = static_cast<int>( ( h >> 16 ) % PaintCount );
            m_start = cp;
            m_goal = cp + CheckpointSpacing;
            m_heroFirst = false;
            // Behind the car in the lane beside it, a little faster.
            const Segment &seg = track.segment( static_cast<long>( player.m_s / SegmentLength ) );
            const int      mine = laneOf( player.m_x, seg.m_lanes, seg.m_halfWidth );
            m_lane = seg.m_lanes < 2 ? mine : mine > 0 ? mine - 1 : mine + 1;
            m_s = player.m_s - 70.0;
            m_x = laneCenter( m_lane, seg.m_lanes, seg.m_halfWidth );
            m_speed = player.m_speed + 6.0f;
            break;
        }
        if ( m_state == HeroStateE::NONE ) return;
    }

    const CarSpec &spec = player.m_spec;   // the same class of car: a fair race
    const Segment &seg = track.segment( static_cast<long>( m_s / SegmentLength ) );
    const int      lanes = seg.m_lanes;
    if ( m_lane >= lanes ) m_lane = lanes - 1;
    const double gap = player.m_s - m_s;   // > 0: the car is ahead
    float        target = 0.0f;
    switch ( m_state )
    {
    case HeroStateE::CLOSING:
        // Up beside the car, then along with it to the checkpoint.
        target = gap > 4.0 ? player.m_speed + 8.0f : gap < -4.0 ? player.m_speed - 3.0f : player.m_speed;
        if ( player.m_s >= m_start )
        {
            m_state = HeroStateE::DUEL;
            ev.m_start = true;
        }
        break;
    case HeroStateE::DUEL:
    {
        if ( m_thief )
        {
            // A little slower than the car; caught when the car stays close for a moment, away at the goal.
            target = std::min( spec.m_topSpeed * 0.9f, cornerSpeed( track, m_s, m_speed, spec ) ) *
                     ( 1.0f + clampf( static_cast<float>( gap ) / 500.0f, -0.1f, 0.03f ) );
            m_catch = std::abs( gap ) < 9.0 && std::abs( player.m_x - m_x ) < 3.2f
                          ? m_catch + dt
                          : std::max( 0.0f, m_catch - dt * 0.5f );
            if ( m_catch >= 1.2f || m_s >= m_goal )
            {
                m_heroFirst = m_catch < 1.2f;
                ( m_heroFirst ? ev.m_lost : ev.m_won ) = true;
                m_state = HeroStateE::AFTER;
            }
            break;
        }
        target = std::min( spec.m_topSpeed * 0.99f, cornerSpeed( track, m_s, m_speed, spec ) );
        target *= 1.0f + clampf( static_cast<float>( gap ) / 600.0f, -0.06f, 0.08f );   // a little rubber band
        const bool carFirst = player.m_s >= m_goal, heroFirst = m_s >= m_goal;
        if ( carFirst || heroFirst )
        {
            m_heroFirst = heroFirst && !carFirst;
            ( m_heroFirst ? ev.m_lost : ev.m_won ) = true;
            m_state = HeroStateE::AFTER;
        }
        break;
    }
    case HeroStateE::AFTER:
        target = m_heroFirst ? spec.m_topSpeed * 0.85f : m_thief ? 0.0f : 22.0f;
        if ( std::abs( gap ) > HeroGone ) m_state = HeroStateE::NONE;
        break;
    default: break;
    }
    if ( m_state == HeroStateE::NONE ) return;

    // Past the traffic: another lane if it is free, else behind the car ahead.
    auto blocked = [&]( double s, float x, float speed )
    {
        const double g = s - m_s;
        if ( g <= 0.0 || g > 8.0 + m_speed * 0.45f || std::abs( x - m_x ) > 1.7f || speed >= target ) return;
        for ( int d : { -1, 1 } )
        {
            const int l = m_lane + d;
            if ( l < 0 || l >= lanes ) continue;
            const float lx = laneCenter( l, lanes, seg.m_halfWidth );
            bool        free = !( std::abs( player.m_s - m_s ) < 14.0 && std::abs( player.m_x - lx ) < 1.7f );
            for ( const TrafficCar &c : traffic.m_cars )
                if ( free && !c.m_swim && std::abs( c.m_s - m_s ) < 16.0 + c.m_length && std::abs( c.m_x - lx ) < 1.7f )
                    free = false;
            if ( free )
            {
                m_lane = l;
                return;
            }
        }
        target = std::min( target, speed - 0.5f );
    };
    for ( const TrafficCar &c : traffic.m_cars )
        if ( !c.m_swim && !c.m_animal ) blocked( c.m_s, c.m_x, c.m_speed );
    if ( m_state != HeroStateE::CLOSING ) blocked( player.m_s, player.m_x, player.m_speed );

    if ( m_speed < target )
        m_speed +=
            ( 10.0f * spec.m_accel * std::max( 0.15f, 1.0f - m_speed / ( spec.m_topSpeed * 1.05f ) ) + 0.5f ) * dt;
    else
        m_speed = approach( m_speed, std::max( 0.0f, target ), 2.2f, dt );
    const float vx = clampf( ( laneCenter( m_lane, lanes, seg.m_halfWidth ) - m_x ) * 1.5f, -3.5f, 3.5f );
    m_x += vx * dt;
    m_yaw = std::atan2( vx, std::max( m_speed, 6.0f ) );
    m_s += static_cast<double>( m_speed ) * dt;

    // Contact: side by side they push apart; from behind the one behind gives way.
    if ( m_hitTimer > 0.0f ) m_hitTimer -= dt;
    const double ds = player.m_s - m_s;
    const float  dx = player.m_x - m_x;
    const float  halfL = spec.m_length, halfW = spec.m_width * 0.92f;
    if ( std::abs( ds ) < halfL && std::abs( dx ) < halfW && !player.m_airborne )
    {
        const float side = dx >= 0.0f ? 1.0f : -1.0f;
        if ( std::abs( ds ) < halfL * 0.6f )
        {
            player.m_x += side * ( halfW - std::abs( dx ) ) * 0.6f;
            m_x -= side * ( halfW - std::abs( dx ) ) * 0.4f;
            if ( m_hitTimer <= 0.0f ) player.m_shake = std::max( player.m_shake, 0.3f );
        }
        else if ( ds > 0.0 )
        {
            m_s = player.m_s - halfL;
            m_speed = std::min( m_speed, player.m_speed );
        }
        else
        {
            player.m_s = m_s - halfL;
            player.m_speed = std::min( player.m_speed, m_speed );
        }
        m_hitTimer = 0.8f;
    }
}

}   // namespace sunstreak
