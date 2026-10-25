/**
 * @file Race.cpp
 * @brief Rival drivers, the grid, contact, results and standings.
 * @author Rene Weller
 */
#include "Race.h"

#include <algorithm>

namespace sunstreak
{

namespace
{

const char *Names[] = {
    "K. VOSS",   "M. TANAKA", "L. FERRAND", "S. OKAFOR", "J. MORENO",  "A. LINDQVIST",
    "R. COSTA",  "D. KOVAC",  "T. HALE",    "N. ROSSI",  "E. MARLOWE", "P. ADEYEMI",
    "C. BRANDT", "Y. SATO",   "I. NOVAK",   "F. DUARTE", "G. MILLER",  "H. AALTO"
};                                                                                    ///< fictional drivers
constexpr int NameCount = static_cast<int>( sizeof( Names ) / sizeof( Names[0] ) );   ///< how many

}   // namespace

int pointsFor( int place )
{
    static const int points[8] = { 10, 8, 6, 5, 4, 3, 2, 1 };
    return place >= 1 && place <= 8 ? points[place - 1] : 0;
}

void Race::newChampionship( uint64_t seed )
{
    m_seed = seed;
    m_rng = Rng( seed ^ 0x9A1Bull );
    int order[NameCount];
    for ( int i = 0; i < NameCount; ++i ) order[i] = i;
    for ( int i = NameCount - 1; i > 0; --i ) std::swap( order[i], order[m_rng.rangeInt( 0, i )] );
    for ( int i = 0; i < RivalCount; ++i )
    {
        Driver &d = m_drivers[i];
        d.m_name = Names[order[i]];
        d.m_car = m_rng.rangeInt( 0, CarCount - 1 );
        d.m_paint = m_rng.rangeInt( 0, PaintCount - 1 );
        d.m_skill = 0.88f + 0.12f * static_cast<float>( i ) / static_cast<float>( RivalCount - 1 ) +
                    m_rng.range( -0.01f, 0.01f );
        d.m_points = 0;
    }
    m_playerPoints = 0;
    m_round = 0;
}

uint64_t Race::roundSeed( int round ) const
{
    return ( hash64( m_seed * 131 + static_cast<uint64_t>( round ) ) & 0x1FFFFFFFull ) | 1ull;
}

TrackParams Race::roundParams( int round ) const
{
    static const double lengths[RoundCount] = { 8000.0, 9000.0, 10000.0, 11000.0, 12500.0 };
    // Five different biomes, drawn from the championship's seed.
    Rng              order( m_seed * 7 + 3 );
    std::vector<int> biomes( static_cast<size_t>( biomeCount() ) );
    for ( int i = 0; i < biomeCount(); ++i ) biomes[static_cast<size_t>( i )] = i;
    for ( int i = biomeCount() - 1; i > 0; --i )
        std::swap( biomes[static_cast<size_t>( i )], biomes[static_cast<size_t>( order.rangeInt( 0, i ) )] );
    TrackParams p;
    p.m_biome = biomes[static_cast<size_t>( std::clamp( round, 0, RoundCount - 1 ) )];
    p.m_length = lengths[std::clamp( round, 0, RoundCount - 1 )];
    p.m_startLine = 100.0;
    p.m_traffic = 0;
    p.m_obstacles = 0;
    return p;
}

void Race::grid( const Track &track, PlayerCar &player, float pace )
{
    m_pace = pace;
    m_finishS = track.params().m_length;
    m_rivals.clear();
    // The field by pace, the fastest at the front; the player last.
    int order[RivalCount];
    for ( int i = 0; i < RivalCount; ++i ) order[i] = i;
    std::sort( order, order + RivalCount, [&]( int a, int b ) { return m_drivers[a].m_skill > m_drivers[b].m_skill; } );
    const double start = track.params().m_startLine;
    auto         slot = [&]( int place, double &s, float &x )
    {
        const Segment &seg = track.segment( static_cast<long>( start / SegmentLength ) );
        s = start - 6.0 - 8.0 * static_cast<double>( place / 2 );
        x = ( place % 2 ? 0.45f : -0.45f ) * seg.m_halfWidth;
    };
    for ( int place = 0; place < RivalCount; ++place )
    {
        Rival r;
        r.m_driver = order[place];
        slot( place, r.m_s, r.m_x );
        const Segment &seg = track.segment( static_cast<long>( r.m_s / SegmentLength ) );
        r.m_lane = laneOf( r.m_x, seg.m_lanes, seg.m_halfWidth );
        r.m_launch = m_rng.range( 0.1f, 0.6f );
        m_rivals.push_back( r );
    }
    double s = 0.0;
    float  x = 0.0f;
    slot( RivalCount, s, x );
    player.reset( player.m_spec, s, x, track );
}

float Race::cornerSpeed( const Track &track, const Rival &r ) const
{
    const CarSpec  &spec = carSpec( m_drivers[r.m_driver].m_car );
    const Segment  &seg = track.segment( static_cast<long>( r.m_s / SegmentLength ) );
    const BiomeDef &biome = biomeDef( static_cast<BiomeE>( seg.m_blend < 0.5f ? seg.m_biome : seg.m_biomeTo ) );
    float           maxK = 0.0f;
    const float     look = 40.0f + r.m_speed * 2.6f;
    for ( float d = 0.0f; d < look; d += SegmentLength )
        maxK =
            std::max( maxK, std::abs( track.segment( static_cast<long>( ( r.m_s + d ) / SegmentLength ) ).m_curve ) );
    const float limit = steerLimit( std::max( r.m_speed, 25.0f ), spec.m_topSpeed ) * spec.m_grip * biome.m_grip *
                        tuning().m_grip * 0.82f;
    return std::sqrt( limit / ( tuning().m_centrifugal * maxK + 1e-6f ) );
}

void Race::update( float dt, float raceTime, const Track &track, PlayerCar &player, RaceEvents &ev, bool started )
{
    const float pl = player.m_spec.m_length, pw = player.m_spec.m_width;
    for ( Rival &r : m_rivals )
    {
        const Driver  &d = m_drivers[r.m_driver];
        const CarSpec &spec = carSpec( d.m_car );
        const Segment &seg = track.segment( static_cast<long>( r.m_s / SegmentLength ) );
        const int      lanes = seg.m_lanes;
        if ( r.m_lane >= lanes ) r.m_lane = lanes - 1;
        const float skill = d.m_skill * m_pace;
        float       target = 0.0f;
        if ( started && raceTime > r.m_launch )
        {
            target = std::min( spec.m_topSpeed * std::min( skill, 1.04f ),
                               cornerSpeed( track, r ) * ( 0.94f + 0.06f * skill ) );
            if ( r.m_finished ) target = std::min( target, spec.m_topSpeed * 0.6f );
            // A gentle rubber band keeps the field together.
            const float gap = static_cast<float>( player.m_s - r.m_s );
            target *= 1.0f + clampf( gap / 700.0f, -0.05f, 0.07f );
            // Slipstream and blocking: the car ahead in the lane.
            auto ahead = [&]( double s, float x, float speed )
            {
                const double g = s - r.m_s;
                if ( g <= 0.0 || g > 30.0 || std::abs( x - r.m_x ) > 1.6f ) return;
                target *= 1.025f;
                if ( g < 6.0 + r.m_speed * 0.35f && speed < target )
                {
                    const int   l = r.m_lane + ( m_rng.chance( 0.5f ) ? -1 : 1 );
                    bool        free = l >= 0 && l < lanes;
                    const float lx = free ? laneCenter( l, lanes, seg.m_halfWidth ) : 0.0f;
                    for ( const Rival &o : m_rivals )
                        if ( free && &o != &r && std::abs( o.m_s - r.m_s ) < 14.0 && std::abs( o.m_x - lx ) < 1.6f )
                            free = false;
                    if ( free && std::abs( player.m_s - r.m_s ) < 14.0 && std::abs( player.m_x - lx ) < 1.6f )
                        free = false;
                    if ( free )
                        r.m_lane = l;
                    else
                        target = std::min( target, speed - 0.5f );
                }
            };
            for ( const Rival &o : m_rivals )
                if ( &o != &r ) ahead( o.m_s, o.m_x, o.m_speed );
            ahead( player.m_s, player.m_x, player.m_speed );
        }
        if ( r.m_speed < target )
            r.m_speed +=
                ( 10.0f * spec.m_accel * std::max( 0.15f, 1.0f - r.m_speed / ( spec.m_topSpeed * 1.05f ) ) + 0.5f ) *
                dt;
        else
            r.m_speed = approach( r.m_speed, std::max( 0.0f, target ), started ? 2.2f : 10.0f, dt );
        const float laneX = laneCenter( r.m_lane, lanes, seg.m_halfWidth );
        r.m_vx = clampf( ( laneX - r.m_x ) * 1.5f, -3.5f, 3.5f );
        r.m_x += r.m_vx * dt;
        r.m_yaw = std::atan2( r.m_vx, std::max( r.m_speed, 6.0f ) );
        r.m_s += static_cast<double>( r.m_speed ) * dt;
        if ( r.m_hitTimer > 0.0f ) r.m_hitTimer -= dt;
        if ( !r.m_finished && m_finishS > 0.0 && r.m_s >= m_finishS )
        {
            r.m_finished = true;
            r.m_finishTime = raceTime - static_cast<float>( ( r.m_s - m_finishS ) / std::max( 1.0f, r.m_speed ) );
        }
    }

    // Rivals do not drive through each other.
    for ( Rival &a : m_rivals )
        for ( Rival &b : m_rivals )
        {
            if ( &a == &b || a.m_s > b.m_s ) continue;
            const float len =
                ( carSpec( m_drivers[a.m_driver].m_car ).m_length + carSpec( m_drivers[b.m_driver].m_car ).m_length ) *
                0.5f;
            if ( b.m_s - a.m_s < len && std::abs( a.m_x - b.m_x ) < 1.7f )
            {
                a.m_s = b.m_s - len;
                a.m_speed = std::min( a.m_speed, b.m_speed );
            }
        }

    // Contact with the player and passes.
    for ( Rival &r : m_rivals )
    {
        const CarSpec &spec = carSpec( m_drivers[r.m_driver].m_car );
        const double   ds = player.m_s - r.m_s;
        const float    dx = player.m_x - r.m_x;
        const float    halfL = ( pl + spec.m_length ) * 0.5f;
        const float    halfW = ( pw + spec.m_width ) * 0.5f * 0.92f;
        if ( std::abs( ds ) < halfL && std::abs( dx ) < halfW && !player.m_airborne )
        {
            const float side = dx >= 0.0f ? 1.0f : -1.0f;
            const float rel = player.m_speed - r.m_speed;
            if ( std::abs( ds ) < halfL * 0.6f )
            {
                player.m_x += side * ( halfW - std::abs( dx ) ) * 0.6f;
                r.m_x -= side * ( halfW - std::abs( dx ) ) * 0.4f;
                player.m_vx = side * 2.5f;
                player.m_speed *= 0.985f;
                ev.m_hit = std::max( ev.m_hit, 0.3f );
            }
            else if ( ds < 0.0 )
            {
                player.m_speed = std::max( 0.0f, r.m_speed - 1.0f );
                player.m_s = r.m_s - halfL;
                r.m_speed += std::max( 0.0f, rel ) * 0.3f;
                ev.m_hit = std::max( ev.m_hit, clampf( rel / 30.0f, 0.25f, 0.8f ) );
            }
            else
            {
                r.m_speed = std::max( 0.0f, player.m_speed - 1.0f );
                r.m_s = player.m_s - halfL;
                ev.m_hit = std::max( ev.m_hit, 0.25f );
            }
            r.m_hitTimer = 1.2f;
            player.m_shake = std::max( player.m_shake, ev.m_hit );
        }
        const float rel = player.m_speed - r.m_speed;
        if ( std::abs( ds ) < 1.0 && std::abs( dx ) < 8.0f && std::abs( rel ) > 8.0f )
        {
            const float strength = clampf( std::abs( rel ) / 40.0f, 0.2f, 1.0f ) * ( 1.0f - std::abs( dx ) / 10.0f );
            if ( strength > ev.m_whoosh )
            {
                ev.m_whoosh = strength;
                ev.m_whooshPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
            }
        }
    }
}

int Race::position( const PlayerCar &player ) const
{
    int place = 1;
    for ( const Rival &r : m_rivals )
        if ( r.m_s >= player.m_s ) ++place;
    return place;
}

std::vector<ResultLine> Race::finish( float playerTime )
{
    std::vector<ResultLine> lines;
    lines.push_back( { -1, playerTime, 0 } );
    for ( const Rival &r : m_rivals )
    {
        const float t = r.m_finished
                            ? r.m_finishTime
                            : playerTime + static_cast<float>( ( m_finishS - r.m_s ) / std::max( 25.0f, r.m_speed ) );
        lines.push_back( { r.m_driver, t, 0 } );
    }
    std::stable_sort( lines.begin(), lines.end(),
                      []( const ResultLine &a, const ResultLine &b ) { return a.m_time < b.m_time; } );
    for ( size_t i = 0; i < lines.size(); ++i )
    {
        lines[i].m_points = pointsFor( static_cast<int>( i ) + 1 );
        if ( lines[i].m_driver < 0 )
            m_playerPoints += lines[i].m_points;
        else
            m_drivers[lines[i].m_driver].m_points += lines[i].m_points;
    }
    return lines;
}

std::vector<std::pair<int, int>> Race::standings() const
{
    std::vector<std::pair<int, int>> table;
    table.emplace_back( -1, m_playerPoints );
    for ( int i = 0; i < RivalCount; ++i ) table.emplace_back( i, m_drivers[i].m_points );
    std::stable_sort( table.begin(), table.end(), []( const auto &a, const auto &b ) { return a.second > b.second; } );
    return table;
}

}   // namespace sunstreak
