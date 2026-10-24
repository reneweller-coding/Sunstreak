/**
 * @file Track.cpp
 * @brief The track generator.
 * @author reneweller
 */
#include "Track.h"

#include <algorithm>
#include <array>

namespace sunstreak
{

namespace
{

constexpr float MaxHeading = 1.35f;   ///< the road never turns further than this from north (rad)

} // namespace

Track::Track( uint64_t seed, const TrackParams &params, int startBiome )
    : m_seed( seed ), m_params( params ), m_rng( seed ^ 0x5EED5EEDull )
{
    for( int i = 0; i < 3; ++i ) m_hillPhase[i] = hashFloat( seed, 101 + i ) * Tau;
    if( m_params.m_biome == SceneryWorld )
    {
        // The world tour starts somewhere on the route, or at the requested place; one off the map goes first.
        const bool wanted = startBiome >= 0 && startBiome < biomeCount();
        const int  at = wanted ? routeIndex( static_cast<BiomeE>( startBiome ) ) : -1;
        m_routeStart = at >= 0 ? at : static_cast<int>( hash64( seed * 131 + 17 ) % worldRoute().size() );
        if( wanted && at < 0 ) m_stageBiomes.push_back( static_cast<uint16_t>( startBiome ) );
        m_tour = std::make_shared<Tour>();
        m_tour->m_pos.push_back( m_routeStart );
        m_tour->m_sea.push_back( -1 );
    }
    else
    {
        // The first bag of stages decides the start; a requested biome is swapped to the front.
        stageBiome( 0 );
    }
    if( m_params.m_biome != SceneryWorld && startBiome >= 0 && startBiome < biomeCount() )
    {
        const auto want = static_cast<uint16_t>( startBiome );
        const auto end = m_stageBiomes.begin() + static_cast<long>( biomeCount() );
        auto       it = std::find( m_stageBiomes.begin(), end, want );
        if( it != end ) std::iter_swap( m_stageBiomes.begin(), it );
    }
    generateTo( 1000.0 );
}

BiomeE Track::stageBiome( int stage ) const
{
    const int Count = biomeCount();
    if( !m_params.m_stages.empty() )
        return static_cast<BiomeE>( m_params.m_stages[static_cast<size_t>( std::clamp( stage, 0, static_cast<int>( m_params.m_stages.size() ) - 1 ) )] );
    if( m_params.m_biome >= 0 && m_params.m_biome < Count ) return static_cast<BiomeE>( m_params.m_biome );
    if( m_params.m_biome == SceneryWorld )
    {
        const auto &route = worldRoute();
        const int   detour = static_cast<int>( m_stageBiomes.size() );
        if( stage < detour ) return static_cast<BiomeE>( m_stageBiomes[static_cast<size_t>( std::max( 0, stage ) )] );
        const int pos = tourPos( stage - detour );
        int       sea = -1;
        {
            std::lock_guard<std::mutex> lock( m_tour->m_mutex );
            const size_t                k = static_cast<size_t>( std::max( 0, stage - detour ) );
            if( k < m_tour->m_sea.size() ) sea = m_tour->m_sea[k];
        }
        if( sea >= 0 ) return static_cast<BiomeE>( sea );
        return route[static_cast<size_t>( pos ) % route.size()];
    }
    while( static_cast<int>( m_stageBiomes.size() ) <= stage )
    {
        // One shuffled bag of all biomes after another, never the same biome twice in a row.
        const int               bag = static_cast<int>( m_stageBiomes.size() ) / Count;
        Rng                  r( hash64( m_seed * 31 + static_cast<uint64_t>( bag ) * 977 + 7 ) );
        std::vector<uint16_t> order( Count );
        for( int i = 0; i < Count; ++i ) order[i] = static_cast<uint16_t>( i );
        for( int i = Count - 1; i > 0; --i ) std::swap( order[i], order[r.rangeInt( 0, i )] );
        if( !m_stageBiomes.empty() && order[0] == m_stageBiomes.back() ) std::swap( order[0], order[1] );
        m_stageBiomes.insert( m_stageBiomes.end(), order.begin(), order.end() );
    }
    return static_cast<BiomeE>( m_stageBiomes[stage] );
}

int Track::tourPos( int stage ) const
{
    if( !m_tour ) return m_routeStart + stage;
    std::lock_guard<std::mutex> lock( m_tour->m_mutex );
    std::vector<int>    &pos = m_tour->m_pos;
    std::vector<int>    &sea = m_tour->m_sea;
    std::vector<int8_t> &choice = m_tour->m_choice;
    while( static_cast<int>( pos.size() ) <= stage )
    {
        // The next stage: along the route, or where the other carriageway of the last fork led; across the sea a
        // stage on the sea floor comes first.
        const size_t last = pos.size() - 1;
        if( sea[last] >= 0 )
        {
            pos.push_back( pos[last] );
            sea.push_back( -1 );
            continue;
        }
        const RouteFork f = routeFork( pos[last] );
        const int8_t    c = last < choice.size() ? choice[last] : 0;
        const bool      other = f.m_other >= 0 && c != 0 && ( c < 0 ) == f.m_otherLeft;
        const int       next = other ? f.m_other : f.m_main;
        const auto     &route = worldRoute();
        const int       crossing = seaCrossing( route[static_cast<size_t>( pos[last] ) % route.size()], route[static_cast<size_t>( next ) % route.size()] );
        pos.push_back( next );
        sea.push_back( crossing );
    }
    return pos[static_cast<size_t>( std::max( 0, stage ) )];
}

Fork Track::fork( int stage ) const
{
    Fork f;
    const int detour = static_cast<int>( m_stageBiomes.size() );
    if( !m_tour || stage < detour ) return f;
    const int       pos = tourPos( stage - detour );
    {
        std::lock_guard<std::mutex> lock( m_tour->m_mutex );
        if( m_tour->m_sea[static_cast<size_t>( stage - detour )] >= 0 ) return f;   // no forks on the sea floor
    }
    const RouteFork r = routeFork( pos );
    if( r.m_other < 0 ) return f;
    const auto &route = worldRoute();
    const BiomeE main = route[static_cast<size_t>( r.m_main ) % route.size()], other = route[static_cast<size_t>( r.m_other ) % route.size()];
    f.m_exists = true;
    f.m_at = static_cast<double>( stage ) * StageLength + ForkAt;
    f.m_left = r.m_otherLeft ? other : main;
    f.m_right = r.m_otherLeft ? main : other;
    std::lock_guard<std::mutex> lock( m_tour->m_mutex );
    const size_t k = static_cast<size_t>( stage - detour );
    f.m_chosen = k < m_tour->m_choice.size() ? m_tour->m_choice[k] : 0;
    return f;
}

void Track::choose( int stage, int side )
{
    const int detour = static_cast<int>( m_stageBiomes.size() );
    if( !m_tour || stage < detour || !fork( stage ).m_exists ) return;
    std::lock_guard<std::mutex> lock( m_tour->m_mutex );
    const size_t k = static_cast<size_t>( stage - detour );
    if( m_tour->m_choice.size() <= k ) m_tour->m_choice.resize( k + 1, 0 );
    if( m_tour->m_choice[k] != 0 ) return;
    m_tour->m_choice[k] = static_cast<int8_t>( side < 0 ? -1 : 1 );
    // The stages after this one were only guesses.
    if( m_tour->m_pos.size() > k + 1 )
    {
        m_tour->m_pos.resize( k + 1 );
        m_tour->m_sea.resize( k + 1 );
    }
}

std::vector<int8_t> Track::tourChoices() const
{
    if( !m_tour ) return {};
    std::lock_guard<std::mutex> lock( m_tour->m_mutex );
    return m_tour->m_choice;
}

void Track::setTourChoices( const std::vector<int8_t> &choices )
{
    if( !m_tour ) return;
    std::lock_guard<std::mutex> lock( m_tour->m_mutex );
    m_tour->m_choice = choices;
    m_tour->m_pos.resize( 1 );
    m_tour->m_sea.resize( 1 );
}

float Track::seaDip( double s, float *trench ) const
{
    if( trench ) *trench = 0.0f;
    if( !m_tour || s < 0.0 ) return 0.0f;
    auto        sea = [this]( int stage ) { return stage >= 0 && biomeDef( stageBiome( stage ) ).m_underwater; };
    const int   stage = static_cast<int>( s / StageLength );
    if( sea( stage ) ) return SeaDipDepth;
    // Down the ramp to the next stage's portal, up the ramp from the last one's. The next stage is asked only on the
    // ramp: it is known for sure once the fork before it is behind.
    const double b0 = static_cast<double>( stage ) * StageLength, b1 = b0 + StageLength;
    float        dip = 0.0f;
    if( s > b1 - SeaRamp && sea( stage + 1 ) )
        dip = SeaDipDepth * smoothstepf( static_cast<float>( -SeaRamp ), 0.0f, static_cast<float>( s - b1 ) );
    if( s < b0 + SeaRamp && sea( stage - 1 ) )
        dip = std::max( dip, SeaDipDepth * ( 1.0f - smoothstepf( 0.0f, static_cast<float>( SeaRamp ), static_cast<float>( s - b0 ) ) ) );
    if( trench ) *trench = dip;
    return dip;
}

bool Track::nearSeaPortal( double s, double margin ) const
{
    if( !m_tour ) return false;
    const double k = std::floor( s / StageLength + 0.5 ), boundary = k * StageLength;
    if( k < 1.0 || std::abs( s - boundary ) > SeaRamp + margin ) return false;
    return biomeDef( stageBiome( static_cast<int>( k ) - 1 ) ).m_underwater != biomeDef( stageBiome( static_cast<int>( k ) ) ).m_underwater;
}

bool Track::inForkZone( double s, double margin ) const
{
    if( !m_tour ) return false;
    const int stage = static_cast<int>( std::max( 0.0, s ) / StageLength );
    const Fork f = fork( stage );
    if( !f.m_exists ) return false;
    // Round the gore, and where the other carriageway peels away; in between the road is free to bend and climb.
    const double u = s - f.m_at;
    return ( u > -ForkWiden - margin && u < 250.0 + margin ) || ( u > ForkMedian - 150.0 - margin && u < ForkMedian + ForkPeel + 150.0 + margin );
}

float Track::difficulty( int stage )
{
    return std::min( 1.0f, static_cast<float>( stage ) / 7.0f );
}

BiomeMix Track::biomeMix( double s ) const
{
    BiomeMix     m;
    const double k = std::floor( s / StageLength + 0.5 );
    const double boundary = k * StageLength;
    if( k >= 1.0 && std::abs( s - boundary ) < BlendLength * 0.5 )
    {
        m.m_from = stageBiome( static_cast<int>( k ) - 1 );
        m.m_to = stageBiome( static_cast<int>( k ) );
        m.m_t = smoothstepf( static_cast<float>( -BlendLength * 0.5 ), static_cast<float>( BlendLength * 0.5 ),
                             static_cast<float>( s - boundary ) );
    }
    else
    {
        m.m_from = m.m_to = stageBiome( static_cast<int>( std::max( 0.0, s ) / StageLength ) );
        m.m_t = 0.0f;
    }
    return m;
}

void Track::styleAt( double s, float &curvy, float &sharp, float &hilly ) const
{
    const BiomeMix  m = biomeMix( s );
    const BiomeDef &a = biomeDef( m.m_from );
    const BiomeDef &b = biomeDef( m.m_to );
    const float     d = difficulty( static_cast<int>( s / StageLength ) );
    curvy = clampf( lerpf( a.m_curviness, b.m_curviness, m.m_t ) * ( 0.75f + 0.45f * d ), 0.0f, 1.0f );
    sharp = clampf( lerpf( a.m_sharpness, b.m_sharpness, m.m_t ) * ( 0.7f + 0.5f * d ), 0.0f, 1.0f );
    hilly = clampf( lerpf( a.m_hilliness, b.m_hilliness, m.m_t ) * ( 0.8f + 0.3f * d ), 0.0f, 1.0f );
    if( m_params.m_curvy >= 0 ) curvy = static_cast<float>( m_params.m_curvy ) / 9.0f;
    if( m_params.m_sharp >= 0 ) sharp = static_cast<float>( m_params.m_sharp ) / 9.0f;
    if( m_params.m_hilly >= 0 ) hilly = static_cast<float>( m_params.m_hilly ) / 9.0f;
}

float Track::hillGrade( double s, float hilliness ) const
{
    const double w = static_cast<double>( Tau );
    const double g = 0.060 * std::sin( w * s / 1100.0 + m_hillPhase[0] ) +
                     0.035 * std::sin( w * s / 420.0 + m_hillPhase[1] ) +
                     0.018 * std::sin( w * s / 170.0 + m_hillPhase[2] );
    return static_cast<float>( g ) * hilliness;
}

void Track::planFeature()
{
    const size_t from = m_pendingFlags.size();
    planShape();
    decorate( from );
}

void Track::decorate( size_t from )
{
    const size_t n = m_pendingFlags.size() - from;
    const long   first = endIndex() + static_cast<long>( from );
    const double s = static_cast<double>( first ) * SegmentLength;
    if( s < m_params.m_startLine + 600.0 || n < 16 || first - m_lastSpecial < 80 ) return;
    if( inForkZone( s, 150.0 ) || inForkZone( s + static_cast<double>( n ) * SegmentLength, 150.0 ) ) return;
    if( nearSeaPortal( s, 150.0 ) || nearSeaPortal( s + static_cast<double>( n ) * SegmentLength, 150.0 ) ) return;
    float maxCurve = 0.0f;
    for( size_t i = from; i < from + n; ++i )
    {
        if( m_pendingFlags[i] & ( SEG_JUMP | SEG_ROADWORKS ) ) return;
        maxCurve = std::max( maxCurve, std::abs( m_pendingCurve[i] ) );
    }
    if( m_params.m_length > 0.0 && s + static_cast<double>( n ) * SegmentLength > m_params.m_length - 100.0 ) return;
    const BiomeMix  mix = biomeMix( s + 100.0 );
    const BiomeDef &here = biomeDef( mix.m_t < 0.5f ? mix.m_from : mix.m_to );
    const float     length = static_cast<float>( n ) * SegmentLength;
    if( length >= 120.0f && maxCurve < 1.0f / 220.0f && m_rng.chance( here.m_tunnels ) )
    {
        const size_t last = from + std::min<size_t>( n - 1, 140 );
        for( size_t i = from + 1; i < last; ++i ) m_pendingFlags[i] |= SEG_TUNNEL;
        m_lastSpecial = first;
    }
    else if( here.m_bridges > 0.0f && length >= 80.0f && maxCurve < 1.0f / 700.0f )
    {
        // Long causeways over the sea, short covered bridges, the big spans not too short; one style per bridge.
        const BridgeE style = here.m_bridge;
        const float   longest = style == BridgeE::CAUSEWAY ? 1500.0f : style == BridgeE::COVERED ? 140.0f : 450.0f;
        const float   shortest = style == BridgeE::SUSPENSION || style == BridgeE::CABLE_STAYED ? 160.0f : 80.0f;
        const size_t  count = std::min( n, static_cast<size_t>( longest / SegmentLength ) );
        const double  e = s + static_cast<double>( count ) * SegmentLength;
        const BiomeMix a = biomeMix( s - 40.0 ), b = biomeMix( e + 40.0 );
        const BiomeE   ba = a.m_t < 0.5f ? a.m_from : a.m_to, bb = b.m_t < 0.5f ? b.m_from : b.m_to;
        if( static_cast<float>( count ) * SegmentLength >= shortest && ba == bb && m_rng.chance( here.m_bridges ) )
        {
            for( size_t i = from + 1; i + 1 < from + count; ++i ) m_pendingFlags[i] |= SEG_BRIDGE;
            m_lastSpecial = first;
        }
    }
}

void Track::planShape()
{
    const long  startIndex = endIndex() + static_cast<long>( m_pendingFlags.size() );
    const double s = static_cast<double>( startIndex ) * SegmentLength;
    float        curvy = 0, sharp = 0, hilly = 0;
    styleAt( s, curvy, sharp, hilly );
    const float     d = difficulty( static_cast<int>( s / StageLength ) );
    const BiomeMix  mix = biomeMix( s + 200.0 );
    const BiomeDef &here = biomeDef( mix.m_t < 0.5f ? mix.m_from : mix.m_to );
    const float     obstacles = m_params.m_obstacles >= 0 ? static_cast<float>( m_params.m_obstacles ) / 4.5f : 1.0f;
    const bool      wet = here.m_env.m_rain > 0.5f || obstacles > 1.4f;

    auto push = [&]( float curve, float grade, uint16_t flags, uint8_t blocked )
    {
        if( wet && m_rng.chance( 0.012f * obstacles ) ) flags |= SEG_PUDDLE;
        m_pendingCurve.push_back( curve );
        m_pendingGrade.push_back( grade );
        m_pendingFlags.push_back( flags );
        m_pendingBlocked.push_back( blocked );
    };

    auto straight = [&]( float length, bool allowSpecials )
    {
        const int n = std::max( 2, static_cast<int>( length / SegmentLength ) );
        if( inForkZone( s, 100.0 ) || inForkZone( s + static_cast<double>( n ) * SegmentLength, 100.0 ) ) allowSpecials = false;
        if( nearSeaPortal( s, 100.0 ) || nearSeaPortal( s + static_cast<double>( n ) * SegmentLength, 100.0 ) ) allowSpecials = false;
        // A crest that throws the car, in hilly country, now and then.
        int jumpAt = -1;
        if( allowSpecials && n >= 40 && hilly > 0.45f && startIndex - m_lastJump > 300 && m_rng.chance( 0.25f * hilly ) )
            jumpAt = n / 2 - 8;
        // Roadworks closing the outer lane.
        int     worksFrom = -1, worksTo = -1;
        uint8_t worksSide = 0;
        if( allowSpecials && n >= 50 && startIndex - m_lastWorks > 400 && m_rng.chance( 0.22f * ( 0.5f + d ) * obstacles ) )
        {
            worksFrom = m_rng.rangeInt( 8, 14 );
            worksTo = std::min( n - 4, worksFrom + m_rng.rangeInt( 28, 50 ) );
            worksSide = m_rng.chance( 0.5f ) ? 1 : 2;
            m_lastWorks = startIndex;
        }
        if( jumpAt >= 0 ) m_lastJump = startIndex;
        const float amp = 0.10f + 0.05f * hilly;
        for( int j = 0; j < n; ++j )
        {
            float    grade = 0.0f;
            uint16_t flags = 0;
            uint8_t  blocked = 0;
            if( jumpAt >= 0 && j >= jumpAt && j < jumpAt + 16 )
            {
                const float u = ( static_cast<float>( j - jumpAt ) + 0.5f ) / 16.0f;
                grade = amp * std::sin( Tau * u );
            }
            if( jumpAt >= 0 && j == jumpAt - 14 ) flags |= SEG_JUMP;
            if( j >= worksFrom && j < worksTo )
            {
                flags |= SEG_ROADWORKS;
                blocked = worksSide;
            }
            push( 0.0f, grade, flags, blocked );
        }
    };

    auto curveFeature = [&]( float dir, float radius, float angle )
    {
        const float arc = angle * radius;
        const int   n = std::max( 8, static_cast<int>( arc / SegmentLength ) );
        const float ease = 0.32f;
        std::vector<float> w( n );
        float              sum = 0.0f;
        for( int j = 0; j < n; ++j )
        {
            const float u = ( static_cast<float>( j ) + 0.5f ) / static_cast<float>( n );
            w[j] = smoothstepf( 0.0f, 1.0f, std::min( u, 1.0f - u ) / ease );
            sum += w[j];
        }
        const bool sharpBend = radius < 330.0f;
        for( int j = 0; j < n; ++j )
        {
            const float curve = dir * angle * w[j] / ( sum * SegmentLength );
            uint16_t    flags = 0;
            if( sharpBend && w[j] > 0.25f ) flags |= dir > 0 ? SEG_CHEVRON_L : SEG_CHEVRON_R;
            push( curve, 0.0f, flags, 0 );
        }
    };

    // The run-up to the start and the first few hundred metres are straight.
    if( s < m_params.m_startLine + 360.0 )
    {
        straight( static_cast<float>( m_params.m_startLine + 360.0 - s ), false );
        return;
    }
    // Round a fork the road runs straight or in long sweeps, so the carriageways part cleanly.
    if( inForkZone( s, 250.0 ) )
    {
        if( m_rng.chance( 0.5f ) ) straight( m_rng.range( 150.0f, 300.0f ), false );
        else
        {
            const float dir = std::abs( m_genHeading ) > 0.6f ? ( m_genHeading > 0 ? -1.0f : 1.0f ) : m_rng.sign();
            curveFeature( dir, m_rng.range( 1600.0f, 2600.0f ), m_rng.range( 0.08f, 0.2f ) );
        }
        return;
    }

    const float h = m_genHeading;
    auto chooseDir = [&]()
    {
        const float back = h > 0 ? -1.0f : 1.0f;
        return m_rng.chance( smoothstepf( 0.25f, 1.1f, std::abs( h ) ) ) ? back : m_rng.sign();
    };
    auto chooseRadius = [&]()
    {
        const float rMin = lerpf( 420.0f, 75.0f, sharp );
        const float rMax = lerpf( 1500.0f, 380.0f, sharp );
        return lerpf( rMax, rMin, std::pow( m_rng.uniform(), 1.3f - 0.6f * sharp ) );
    };
    auto limitAngle = [&]( float dir, float angle, float heading )
    {
        // Turning further away from north is allowed only up to MaxHeading.
        angle = std::min( angle, MaxHeading - dir * heading );
        return angle;
    };

    const float pStraight = 0.55f - 0.35f * curvy;
    const float r = m_rng.uniform();
    if( r < pStraight )
    {
        straight( m_rng.range( 80.0f, 420.0f ) * ( 1.3f - 0.6f * curvy ), true );
    }
    else if( r < pStraight + 0.22f )
    {
        // S-bend: two bends in opposite directions.
        float       dir = chooseDir();
        const float radius = std::max( 120.0f, chooseRadius() );
        float       angle = limitAngle( dir, m_rng.range( 0.3f, 0.8f ), h );
        if( angle < 0.15f )
        {
            dir = -dir;
            angle = limitAngle( dir, m_rng.range( 0.3f, 0.8f ), h );
        }
        curveFeature( dir, radius, angle );
        if( m_rng.chance( 0.5f ) ) straight( m_rng.range( 10.0f, 50.0f ), false );
        const float after = h + dir * angle;
        curveFeature( -dir, radius * m_rng.range( 0.8f, 1.25f ), limitAngle( -dir, angle * m_rng.range( 0.8f, 1.3f ), after ) );
    }
    else
    {
        float       dir = chooseDir();
        const float radius = chooseRadius();
        float       angle = m_rng.range( 0.35f, radius < 160.0f ? 1.9f : 1.4f );
        angle = limitAngle( dir, angle, h );
        if( angle < 0.15f )
        {
            dir = -dir;
            angle = limitAngle( dir, m_rng.range( 0.35f, 1.2f ), h );
        }
        curveFeature( dir, radius, angle );
    }
    if( m_rng.chance( 0.35f ) ) straight( m_rng.range( 20.0f, 90.0f ), false );
}

void Track::appendSegment()
{
    if( m_pendingCurve.empty() ) planFeature();
    const long   index = endIndex();
    const double s = static_cast<double>( index ) * SegmentLength;

    Segment seg;
    seg.m_curve = m_pendingCurve.front();
    const float extraGrade = m_pendingGrade.front();
    seg.m_flags = m_pendingFlags.front();
    const uint8_t worksSide = m_pendingBlocked.front();
    m_pendingCurve.pop_front();
    m_pendingGrade.pop_front();
    m_pendingFlags.pop_front();
    m_pendingBlocked.pop_front();

    float curvy = 0, sharp = 0, hilly = 0;
    styleAt( s, curvy, sharp, hilly );
    const float pull = clampf( static_cast<float>( m_genPos.m_y ) / 2500.0f, -0.03f, 0.03f );
    seg.m_grade = hillGrade( s, hilly ) + extraGrade - pull;
    if( s < m_params.m_startLine + 60.0 ) seg.m_grade *= static_cast<float>( s / ( m_params.m_startLine + 60.0 ) );
    if( m_tour )
    {
        // Down into an undersea tunnel's portal and up out of the other one; the land beside stays where it was.
        float       trench = 0.0f;
        const float dip = seaDip( s, &trench ), next = seaDip( s + SegmentLength );
        seg.m_grade -= ( next - dip ) / SegmentLength;
        seg.m_trench = trench;
    }

    const BiomeMix  mix = biomeMix( s );
    const BiomeDef &a = biomeDef( mix.m_from );
    const BiomeDef &b = biomeDef( mix.m_to );
    seg.m_biome = static_cast<uint16_t>( mix.m_from );
    seg.m_biomeTo = static_cast<uint16_t>( mix.m_to );
    seg.m_blend = mix.m_t;
    const float lanesT = smoothstepf( 0.2f, 0.8f, mix.m_t );
    seg.m_halfWidth = lerpf( static_cast<float>( a.m_lanes ), static_cast<float>( b.m_lanes ), lanesT ) * LaneWidth * 0.5f;
    seg.m_lanes = static_cast<uint8_t>( mix.m_t < 0.5f ? a.m_lanes : b.m_lanes );
    if( a.m_lanes != b.m_lanes && mix.m_t > 0.05f && mix.m_t < 0.95f ) seg.m_flags |= SEG_NO_DASH;
    const BiomeDef &rails = mix.m_t < 0.5f ? a : b;
    if( rails.m_railLeft ) seg.m_flags |= SEG_RAIL_L;
    if( rails.m_railRight ) seg.m_flags |= SEG_RAIL_R;
    if( worksSide != 0 && seg.m_lanes >= 2 )
        seg.m_blockedLanes = static_cast<uint8_t>( worksSide == 1 ? 1u : 1u << ( seg.m_lanes - 1 ) );
    else
        seg.m_flags &= static_cast<uint16_t>( ~SEG_ROADWORKS );
    if( index > 0 && index % static_cast<long>( CheckpointSpacing / SegmentLength ) == 0 ) seg.m_flags |= SEG_CHECKPOINT;
    if( index == static_cast<long>( m_params.m_startLine / SegmentLength ) ) seg.m_flags |= SEG_START;
    if( m_params.m_length > 0.0 && index == static_cast<long>( m_params.m_length / SegmentLength ) ) seg.m_flags |= SEG_FINISH;
    if( m_tour )
    {
        const Fork fork = this->fork( static_cast<int>( s / StageLength ) );
        if( fork.m_exists && s > fork.m_at - ForkWiden - 200.0 && s < fork.m_at + ForkMedian + ForkPeel + 200.0 )
        {
            // Two carriageways of the road's lanes each: widening before the barrier, narrowing after it.
            const double u = s - fork.m_at;
            const float  wide = smoothstepf( static_cast<float>( -ForkWiden ), -60.0f, static_cast<float>( u ) ) *
                               ( 1.0f - smoothstepf( static_cast<float>( ForkMedian ), static_cast<float>( ForkMedian + ForkPeel * 0.7 ), static_cast<float>( u ) ) );
            const int lanes = std::min( 3, static_cast<int>( seg.m_lanes ) );
            seg.m_halfWidth = lerpf( seg.m_halfWidth, static_cast<float>( lanes ) * LaneWidth + 0.6f, wide );
            seg.m_lanes = static_cast<uint8_t>( wide > 0.5f ? lanes * 2 : seg.m_lanes );
            if( wide > 0.02f && wide < 0.98f ) seg.m_flags |= SEG_NO_DASH;
            seg.m_flags |= SEG_FORK;
            if( inForkZone( s, 0.0 ) )
            {
                seg.m_flags &= static_cast<uint16_t>( ~( SEG_TUNNEL | SEG_BRIDGE | SEG_JUMP | SEG_ROADWORKS | SEG_PUDDLE | SEG_RAIL_L | SEG_RAIL_R |
                                                         SEG_CHEVRON_L | SEG_CHEVRON_R ) );
                seg.m_blockedLanes = 0;
            }
            seg.m_flags &= static_cast<uint16_t>( ~( SEG_ROADWORKS | SEG_PUDDLE ) );
            seg.m_blockedLanes = 0;
            if( u >= 0.0 && u < ForkMedian ) seg.m_flags |= SEG_MEDIAN;
            // Flat land only where the other carriageway turns away into it.
            seg.m_flat = smoothstepf( static_cast<float>( ForkMedian - 350.0 ), static_cast<float>( ForkMedian - 150.0 ), static_cast<float>( u ) ) *
                         ( 1.0f - smoothstepf( static_cast<float>( ForkMedian + ForkPeel ), static_cast<float>( ForkMedian + ForkPeel + 200.0 ), static_cast<float>( u ) ) );
        }
    }
    if( seg.m_flags & ( SEG_TUNNEL | SEG_BRIDGE ) ) seg.m_flags |= SEG_RAIL_L | SEG_RAIL_R;
    seg.m_bank = clampf( seg.m_curve * 28.0f, -0.09f, 0.09f );
    seg.m_stage = static_cast<int>( s / StageLength );
    seg.m_pos = m_genPos;
    seg.m_heading = m_genHeading;
    m_segments.push_back( seg );

    // Advance along an arc: the chord follows the heading at the segment's middle.
    const Vector3 f = headingForward( m_genHeading + seg.m_curve * SegmentLength * 0.5f );
    m_genPos.m_x += static_cast<double>( f.x ) * SegmentLength;
    m_genPos.m_z += static_cast<double>( f.z ) * SegmentLength;
    m_genPos.m_y += static_cast<double>( seg.m_grade ) * SegmentLength;
    m_genHeading += seg.m_curve * SegmentLength;
}

void Track::generateTo( double s )
{
    while( static_cast<double>( endIndex() ) * SegmentLength < s + 2.0 * SegmentLength ) appendSegment();
}

void Track::discardBefore( double s )
{
    while( m_segments.size() > 4 && static_cast<double>( m_first + 1 ) * SegmentLength < s )
    {
        m_segments.pop_front();
        ++m_first;
    }
}

const Segment &Track::segment( long index ) const
{
    const long i = std::clamp( index, m_first, endIndex() - 1 );
    return m_segments[static_cast<size_t>( i - m_first )];
}

TrackFrame Track::frameAt( double s ) const
{
    long i = static_cast<long>( std::floor( s / SegmentLength ) );
    i = std::clamp( i, m_first, endIndex() - 2 );
    const float    t = clampf( static_cast<float>( ( s - static_cast<double>( i ) * SegmentLength ) / SegmentLength ), 0.0f, 1.0f );
    const Segment &a = segment( i );
    const Segment &b = segment( i + 1 );

    TrackFrame f;
    f.m_heading = a.m_heading + a.m_curve * SegmentLength * t;
    const Vector3 chord = headingForward( a.m_heading + a.m_curve * SegmentLength * t * 0.5f );
    f.m_pos.m_x = a.m_pos.m_x + static_cast<double>( chord.x ) * SegmentLength * t;
    f.m_pos.m_z = a.m_pos.m_z + static_cast<double>( chord.z ) * SegmentLength * t;
    f.m_pos.m_y = a.m_pos.m_y + static_cast<double>( a.m_grade ) * SegmentLength * t;
    f.m_grade = lerpf( a.m_grade, b.m_grade, t );
    f.m_bank = lerpf( a.m_bank, b.m_bank, t );
    f.m_curve = a.m_curve;
    f.m_halfWidth = lerpf( a.m_halfWidth, b.m_halfWidth, t );
    f.m_forward = Vector3Normalize( Vector3Add( headingForward( f.m_heading ), Vector3{ 0.0f, f.m_grade, 0.0f } ) );
    f.m_right = headingRight( f.m_heading );
    const Vector3 tilted = Vector3Normalize( Vector3Add( f.m_right, Vector3{ 0.0f, -std::tan( f.m_bank ), 0.0f } ) );
    f.m_up = Vector3Normalize( Vector3CrossProduct( tilted, f.m_forward ) );
    return f;
}

Vec3d Track::pointAt( double s, float lateral, float height ) const
{
    const TrackFrame f = frameAt( s );
    Vec3d            p = f.m_pos;
    p.m_x += static_cast<double>( f.m_right.x * lateral + f.m_up.x * height );
    p.m_z += static_cast<double>( f.m_right.z * lateral + f.m_up.z * height );
    p.m_y += static_cast<double>( -lateral * std::tan( f.m_bank ) + f.m_up.y * height );
    return p;
}

float puddleLateral( const Track &track, long index )
{
    const Segment &seg = track.segment( index );
    const int      lane = static_cast<int>( hash64( track.seed() ^ ( static_cast<uint64_t>( index ) * 7717 ) ) % seg.m_lanes );
    return laneCenter( lane, seg.m_lanes, seg.m_halfWidth ) + ( hashFloat( track.seed() + 3, static_cast<uint64_t>( index ) ) - 0.5f );
}

double Track::surfaceHeight( double s, float lateral ) const
{
    const TrackFrame f = frameAt( s );
    return f.m_pos.m_y - static_cast<double>( lateral * std::tan( f.m_bank ) );
}

} // namespace sunstreak
