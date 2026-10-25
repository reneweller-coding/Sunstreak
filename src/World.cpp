/**
 * @file World.cpp
 * @brief Chunk baking: road surface, land, water, furniture and props.
 * @author reneweller
 */
#include "World.h"

#include "Festival.h"
#include "Local.h"

#include "Noise.h"
#include "Train.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iterator>

namespace sunstreak
{

namespace
{

/** @brief One cross-section of the road, relative to the chunk's origin. */
struct Cross
{
    long           m_index = 0;     ///< segment index
    Vector3        m_center{};      ///< centre line
    Vector3        m_right{};       ///< horizontal right
    Vector3        m_forward{};     ///< horizontal forward
    float          m_heading = 0;   ///< yaw
    float          m_tanBank = 0;   ///< tan of the roll
    float          m_hw = 0;        ///< half paved width
    float          m_edge = 0;      ///< centre to the shoulder's outer edge
    Surface        m_surface{};     ///< blended colours
    Terrain        m_terrain{};     ///< blended land
    const Segment *m_seg = nullptr; ///< the segment
    SeasonLook     m_season{};      ///< what the season does here
};

/** @brief Kinds of strips across the road. */
enum class StripE
{
    ROAD,   ///< asphalt
    LINE,   ///< solid edge line
    DASH,   ///< lane divider, dashed
};

constexpr Color WarmLight = { 255, 214, 160, 255 };   ///< street lamp light
constexpr Mat   GroundMat = { -1.0f, 0.0f };           ///< marks land for the shader's neon grid

/** @brief Paints for rows of houses whose rule asks for a different colour each (PropRule::m_pastel). */
constexpr Color Pastels[] = { { 236, 170, 180, 255 }, { 170, 206, 230, 255 }, { 240, 220, 140, 255 }, { 180, 220, 170, 255 },
                              { 240, 180, 130, 255 }, { 200, 180, 230, 255 }, { 240, 240, 232, 255 }, { 230, 130, 110, 255 },
                              { 140, 200, 200, 255 }, { 250, 200, 90, 255 } };

/**
 * @brief A point on a cross-section.
 * @param[in] x  the cross-section
 * @param[in] l  metres to the right
 * @param[in] h  metres above the road's height there
 * @return the point
 */
Vector3 at( const Cross &x, float l, float h )
{
    const float bankL = clampf( l, -x.m_hw - RumbleWidth, x.m_hw + RumbleWidth );
    return Vector3{ x.m_center.x + x.m_right.x * l, x.m_center.y - bankL * x.m_tanBank + h,
                    x.m_center.z + x.m_right.z * l };
}

/**
 * @brief Whether a kind is a building that stands on a plot in a town, a city or a village.
 * @param[in] k     the kind
 * @param[out] tall  a high-rise
 * @return true for houses and high-rises
 */
bool plotted( PropKindE k, bool &tall )
{
    using K = PropKindE;
    tall = k == K::BUILDING || k == K::TOWER || k == K::NEON_TOWER || k == K::GLASS_TOWER || k == K::PANEL_BLOCK;
    return tall || k == K::HOUSE || k == K::SNOW_HOUSE || k == K::CABIN || k == K::HUT || k == K::COTTAGE || k == K::FLAT_HOUSE ||
           k == K::CHALET || k == K::HALF_TIMBER || k == K::ROW_HOUSES || k == K::ADOBE || k == K::RONDAVEL || k == K::FAVELA ||
           k == K::HANOK || k == K::GASSHO || k == K::HUIZHOU_HOUSE || k == K::CAPE_DUTCH || k == K::YURT || k == K::REED_HOUSE ||
           k == K::TRULLO;
}

/** @brief The plots of a place: its buildings in rows back from the road instead of scattered. */
struct Plots
{
    std::array<size_t, 16> m_rule{};       ///< the building rules
    std::array<float, 16>  m_upTo{};       ///< their chances summed up to each (to pick one by weight)
    int                    m_count = 0;    ///< how many
    float                  m_along = 0.0f;    ///< a plot's length along the road (m)
    float                  m_across = 0.0f;   ///< a row's depth
    float                  m_nearest = 0.0f;  ///< the first row's front from the road's edge
    float                  m_half = 0.0f;     ///< half the widest building
    int                    m_rows = 0;        ///< rows back from the road
    bool                   m_city = false;    ///< a town or a city, built all along (else villages here and there)

    /**
     * @brief Whether a rule stands on the plots.
     * @param[in] r  the rule's index
     * @return true if it does
     */
    bool has( size_t r ) const { return std::find( m_rule.begin(), m_rule.begin() + m_count, r ) != m_rule.begin() + m_count; }
};

/**
 * @brief Lays out a place's plots: as wide as its widest building, a city's rows some 300 m back (the skyline behind
 *        the street), a village's two or three.
 * @param[in] rule    the place
 * @param[in] models  the footprints
 * @return the plots (m_count 0: no buildings)
 */
Plots plotsOf( const BiomeDef &rule, const Models &models )
{
    Plots p;
    float tallChance = 0.0f, houseChance = 0.0f, nearest = 1e9f, farthest = 0.0f, widest = 0.0f;
    for( size_t r = 0; r < rule.m_props.size() && p.m_count < static_cast<int>( p.m_rule.size() ); ++r )
    {
        const PropRule &pr = rule.m_props[r];
        bool            tall = false;
        if( pr.m_perStage > 0 || pr.m_onWater || pr.m_soft || !plotted( pr.m_kind, tall ) ) continue;
        ( tall ? tallChance : houseChance ) += pr.m_chance;
        p.m_rule[static_cast<size_t>( p.m_count )] = r;
        p.m_upTo[static_cast<size_t>( p.m_count )] = tallChance + houseChance;
        ++p.m_count;
        nearest = std::min( nearest, pr.m_nearest );
        farthest = std::max( farthest, pr.m_farthest );
        widest = std::max( widest, models.propFootprint( pr.m_kind ) * pr.m_scaleMax );
    }
    if( p.m_count == 0 ) return p;
    const bool townish = rule.m_lampSpacing > 0.0f || rule.m_cobbles || rule.m_lanes >= 3;
    p.m_city = tallChance >= 0.05f || ( townish && houseChance >= 0.05f );
    p.m_along = std::max( 8.0f, widest ) + ( p.m_city ? 4.0f : 7.0f );
    p.m_across = p.m_along + 3.0f;
    p.m_nearest = nearest;
    p.m_half = std::max( 4.0f, widest * 0.5f );
    const float deepest = p.m_city ? std::max( farthest, tallChance >= 0.05f ? 320.0f : 160.0f ) : nearest + 3.0f * p.m_across;
    p.m_rows = std::clamp( static_cast<int>( ( deepest - nearest ) / p.m_across ), 2, p.m_city ? 10 : 3 );
    return p;
}

/**
 * @brief Whether a segment lies in a village: one stretch of 250 to 550 m in roughly every other 800 m of the country.
 * @param[in] seed  the track's seed
 * @param[in] j     the segment
 * @return true in a village
 */
bool inVillage( uint64_t seed, long j )
{
    const long     block = 160, b = j / block, at = j % block;
    const uint64_t key = seed ^ 0x5157A6Eull;
    if( hashFloat( key, static_cast<uint64_t>( b ) ) >= 0.45f ) return false;
    const long len = 50 + static_cast<long>( 60.0f * hashFloat( key + 1, static_cast<uint64_t>( b ) ) );
    const long first = 5 + static_cast<long>( static_cast<float>( block - 10 - len ) * hashFloat( key + 2, static_cast<uint64_t>( b ) ) );
    return at >= first && at < first + len;
}

/**
 * @brief The biome definition that rules a segment's discrete choices (rails, lamps, props).
 * @param[in] seg  the segment
 * @param[in] h    a random number 0..1 (props pick either side of a crossfade)
 * @return the definition
 */
const BiomeDef &ruling( const Segment &seg, float h )
{
    return biomeDef( static_cast<BiomeE>( h < seg.m_blend ? seg.m_biomeTo : seg.m_biome ) );
}

constexpr float TunnelLift = 18.0f;    ///< height of the hill over a tunnel
constexpr float ValleyDepth = 22.0f;   ///< depth of the valley under a bridge
constexpr int   RampSegments = 7;      ///< segments over which the hill rises and the valley falls

/**
 * @brief How much of the hill over a tunnel stands at a segment (1 in the tunnel, falling off before and after).
 * @param[in] track  the road
 * @param[in] index  the segment
 * @return 0..1
 */
float tunnelCover( const Track &track, long index )
{
    float best = 0.0f;
    for( long d = -RampSegments; d <= RampSegments; ++d )
        if( track.segment( index + d ).m_flags & SEG_TUNNEL )
            best = std::max( best, 1.0f - static_cast<float>( std::abs( d ) ) / static_cast<float>( RampSegments + 1 ) );
    return best;
}

/**
 * @brief The biome a bridge segment belongs to (the one ruling its middle, so a whole bridge has one style).
 * @param[in] seg  the segment
 * @return the definition
 */
const BiomeDef &bridgeBiome( const Segment &seg )
{
    return biomeDef( static_cast<BiomeE>( seg.m_blend < 0.5f ? seg.m_biome : seg.m_biomeTo ) );
}

/**
 * @brief How deep the valley under a bridge of a style is.
 * @param[in] style  how the bridge is built
 * @return metres below the road
 */
float styleDepth( BridgeE style )
{
    switch( style )
    {
    case BridgeE::VIADUCT: return 55.0f;
    case BridgeE::ARCH: return 24.0f;
    case BridgeE::TRUSS: return 20.0f;
    case BridgeE::SUSPENSION: return 34.0f;
    case BridgeE::CABLE_STAYED: return 30.0f;
    case BridgeE::COVERED: return 9.0f;
    case BridgeE::CAUSEWAY: return 3.2f;
    default: return ValleyDepth;
    }
}

/**
 * @brief How deep the valley under a bridge is at a segment (1 under the bridge, rising at its ends).
 * @param[in]  track   the road
 * @param[in]  index   the segment
 * @param[out] metres  the full depth for the nearest bridge's style (if not null)
 * @return 0..1
 */
float valleyDepth( const Track &track, long index, float *metres = nullptr )
{
    float best = 0.0f;
    for( long d = -RampSegments; d <= RampSegments; ++d )
    {
        const Segment &s = track.segment( index + d );
        if( !( s.m_flags & SEG_BRIDGE ) ) continue;
        const float v = 1.0f - static_cast<float>( std::abs( d ) ) / static_cast<float>( RampSegments + 1 );
        if( v > best )
        {
            best = v;
            if( metres ) *metres = styleDepth( bridgeBiome( s ).m_bridge );
        }
    }
    return smoothstepf( 0.0f, 1.0f, best );
}

/**
 * @brief A box turned to the road: centre, the road's right and forward, half sizes across, up and along.
 * @param[out] m        where the triangles go
 * @param[in]  c        centre
 * @param[in]  right    the road's right
 * @param[in]  forward  the road's forward
 * @param[in]  hx       half size across
 * @param[in]  hy       half height
 * @param[in]  hz       half size along
 * @param[in]  col      colour
 */
void obox( MeshBuilder &m, Vector3 c, Vector3 right, Vector3 forward, float hx, float hy, float hz, Color col )
{
    auto p = [&]( float x, float y, float z )
    { return Vector3{ c.x + right.x * x + forward.x * z, c.y + y, c.z + right.z * x + forward.z * z }; };
    const Vector3 v[8] = { p( -hx, -hy, -hz ), p( hx, -hy, -hz ), p( hx, hy, -hz ), p( -hx, hy, -hz ),
                           p( -hx, -hy, hz ),  p( hx, -hy, hz ),  p( hx, hy, hz ),  p( -hx, hy, hz ) };
    m.quad( v[0], v[1], v[2], v[3], col );
    m.quad( v[5], v[4], v[7], v[6], col );
    m.quad( v[4], v[0], v[3], v[7], scaleColor( col, 0.9f ) );
    m.quad( v[1], v[5], v[6], v[2], scaleColor( col, 0.9f ) );
    m.quad( v[3], v[2], v[6], v[7], scaleColor( col, 1.05f ) );
    m.quad( v[4], v[5], v[1], v[0], scaleColor( col, 0.7f ) );
}

} // namespace

float World::terrainHeight( const Track &track, long index, float lateral, const Terrain &terrain, float edge )
{
    const float    a = std::max( 0.0f, std::abs( lateral ) - edge );
    const bool     left = lateral < 0.0f;
    const double   s = static_cast<double>( index ) * SegmentLength;
    const uint64_t seed = track.seed();
    if( const Serpentine *sp = track.serpentineAt( index ) )
    {
        // A pass in hairpins: one mountainside for all its legs (Track.h, Serpentine), so the land at a point lies as
        // high whichever leg it is seen from; near the road it comes down to the shoulder.
        const Segment &seg = track.segment( index );
        const Vector3  r = headingRight( seg.m_heading );
        const double   x = seg.m_pos.m_x + static_cast<double>( r.x * lateral ), z = seg.m_pos.m_z + static_cast<double>( r.z * lateral );
        const float    bankL = clampf( lateral, -seg.m_halfWidth - RumbleWidth, seg.m_halfWidth + RumbleWidth );
        const float    road = static_cast<float>( seg.m_pos.m_y ) - bankL * std::tan( seg.m_bank );
        const float    rough = 6.0f * ( valueNoise( x * 0.012, z * 0.012, seed + 31 ) - 0.5f ) * smoothstepf( 15.0f, 70.0f, a );
        const float    side = static_cast<float>( sp->height( x, z ) ) - road + rough;
        return lerpf( -0.35f * smoothstepf( 0.0f, 4.0f, a ), side, smoothstepf( 2.0f, 9.0f, a ) );
    }
    // Round a fork the land lies flat and level with the road, far out to where the other carriageway goes.
    const float    keep = 1.0f - track.segment( index ).m_flat * ( 1.0f - smoothstepf( 160.0f, 260.0f, a ) );
    float          h = -0.35f * smoothstepf( 0.0f, 4.0f, a ) * keep;
    const float    wall = ( left ? terrain.m_wallLeft : terrain.m_wallRight ) * keep;
    const float    n1 = valueNoise( s * 0.012, lateral * 0.02, seed + 11 );
    h += wall * smoothstepf( 8.0f, 70.0f, a ) * ( 0.75f + 0.45f * n1 );
    const float hills = fbm( s * 0.0035, lateral * 0.005, seed + 23 ) * 0.5f + 0.5f;
    h += terrain.m_hills * keep * smoothstepf( 30.0f, 220.0f, a ) * hills * 1.6f;
    const float water = ( left ? terrain.m_waterLeft : terrain.m_waterRight ) * keep;
    if( water > 0.0f ) h = lerpf( h, terrain.m_seaLevel - 4.5f, water * smoothstepf( 4.0f, 22.0f, a ) );
    const float cover = tunnelCover( track, index );
    if( cover > 0.0f ) h += TunnelLift * cover * smoothstepf( 0.0f, 6.0f, a ) * ( 1.0f - smoothstepf( 60.0f, 140.0f, a ) );
    float       metres = ValleyDepth;
    const float valley = valleyDepth( track, index, &metres );
    if( valley > 0.0f )
    {
        const bool under = track.segment( index ).m_flags & SEG_BRIDGE;
        h -= metres * valley * ( under ? 1.0f : smoothstepf( 0.0f, 12.0f, a ) ) * ( 1.0f - smoothstepf( 80.0f, 200.0f, a ) );
    }
    // The cutting to an undersea tunnel: the land stays up behind its walls while the road sinks.
    h += track.segment( index ).m_trench * smoothstepf( 2.0f, 3.0f, a );
    return h;
}

long landmarkSpot( uint64_t seed, size_t rule, int k, long stage, bool &right )
{
    const long     stageSegs = static_cast<long>( StageLength / SegmentLength );
    const uint64_t h = hash64( seed ^ ( rule * 131 + static_cast<uint64_t>( k ) * 977 + static_cast<uint64_t>( stage ) * 7919 ) );
    right = ( ( h >> 20 ) & 1u ) != 0;
    return 60 + static_cast<long>( h % static_cast<uint64_t>( stageSegs - 120 ) );
}

World::~World()
{
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_quit = true;
    }
    m_wake.notify_all();
    if( m_thread.joinable() ) m_thread.join();
}

void World::clear()
{
    {
        // The worker may be baking on its track: drop the queues, wait until it is idle, then forget the track.
        std::unique_lock<std::mutex> lock( m_mutex );
        m_jobs.clear();
        m_done.clear();
        ++m_generation;
        m_idle.wait( lock, [this] { return !m_busy; } );
        m_workerTrack.reset();
        m_requested = -1;
    }
    m_ready.clear();
    for( Chunk &c : m_chunks )
    {
        UnloadMesh( c.m_ground );
        UnloadMesh( c.m_props );
        UnloadMesh( c.m_scenery );
        UnloadMesh( c.m_sceneryFar );
        UnloadMesh( c.m_glass );
    }
    m_chunks.clear();
}

void World::work()
{
    std::unique_lock<std::mutex> lock( m_mutex );
    for( ;; )
    {
        m_wake.wait( lock, [this] { return m_quit || ( !m_jobs.empty() && m_workerTrack ); } );
        if( m_quit ) return;
        BakedChunk baked;
        baked.m_index = m_jobs.front();
        m_jobs.pop_front();
        const uint64_t generation = m_generation;
        Track         &track = *m_workerTrack;
        m_busy = true;
        lock.unlock();

        const auto t0 = std::chrono::steady_clock::now();
        buildChunk( track, baked );
        baked.m_ms = std::chrono::duration<float, std::milli>( std::chrono::steady_clock::now() - t0 ).count();
        track.discardBefore( static_cast<double>( baked.m_index ) * ChunkLength - 2500.0 );

        lock.lock();
        m_busy = false;
        if( generation == m_generation ) m_done.push_back( std::move( baked ) );
        m_idle.notify_all();
    }
}

void World::upload( BakedChunk &baked )
{
    Chunk c;
    c.m_index = baked.m_index;
    c.m_origin = baked.m_origin;
    c.m_ground = baked.m_ground.upload();
    c.m_props = baked.m_props.upload();
    c.m_scenery = baked.m_scenery.upload();
    c.m_sceneryFar = baked.m_sceneryFar.upload();
    c.m_glass = baked.m_glass.upload();
    c.m_colliders = std::move( baked.m_colliders );
    m_chunks.push_back( std::move( c ) );
    m_buildMaxMs = std::max( m_buildMaxMs, baked.m_ms );
    m_buildTotalMs += baked.m_ms;
    ++m_buildCount;
}

double World::keptBehind( const Track &track, double s )
{
    for( double back : { 0.0, 600.0 } )
        if( const Serpentine *sp = track.serpentineAt( static_cast<long>( std::max( 0.0, s - back ) / SegmentLength ) ) )
            return std::max( KeepBehind, std::min( 1500.0, s - static_cast<double>( sp->m_first ) * SegmentLength ) );
    return KeepBehind;
}

void World::update( Track &track, double s, int maxBuilds )
{
    const auto t0 = std::chrono::steady_clock::now();
    const long firstWanted = std::max( 0L, static_cast<long>( std::floor( ( s - keptBehind( track, s ) ) / ChunkLength ) ) );
    const long lastWanted = static_cast<long>( std::floor( ( s + ViewDistance ) / ChunkLength ) );
    while( !m_chunks.empty() && m_chunks.front().m_index < firstWanted )
    {
        UnloadMesh( m_chunks.front().m_ground );
        UnloadMesh( m_chunks.front().m_props );
        UnloadMesh( m_chunks.front().m_scenery );
        UnloadMesh( m_chunks.front().m_sceneryFar );
        UnloadMesh( m_chunks.front().m_glass );
        m_chunks.pop_front();
    }
    long next = m_chunks.empty() ? firstWanted : m_chunks.back().m_index + 1;
    // A chunk the car is about to reach is baked here if the worker has not delivered it (never in practice).
    auto bakeHere = [&]( long index )
    {
        BakedChunk baked;
        baked.m_index = index;
        const auto b0 = std::chrono::steady_clock::now();
        buildChunk( track, baked );
        baked.m_ms = std::chrono::duration<float, std::milli>( std::chrono::steady_clock::now() - b0 ).count();
        upload( baked );
    };

    bool hasWorker;
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        hasWorker = m_workerTrack != nullptr;
    }
    if( !hasWorker )
    {
        // A new track: bake what is wanted now, then give the worker its copy.
        for( int built = 0; next <= lastWanted && built < maxBuilds; ++built, ++next ) bakeHere( next );
        std::lock_guard<std::mutex> lock( m_mutex );
        m_workerTrack = std::make_unique<Track>( track );
        m_requested = next - 1;
        if( !m_thread.joinable() ) m_thread = std::thread( [this] { work(); } );
    }
    else
    {
        std::vector<BakedChunk> arrived;
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            if( m_requested < next - 1 ) m_requested = next - 1;
            while( m_requested < lastWanted && m_jobs.size() < 6 ) m_jobs.push_back( ++m_requested );
            while( !m_done.empty() )
            {
                arrived.push_back( std::move( m_done.front() ) );
                m_done.pop_front();
            }
        }
        m_wake.notify_one();
        for( BakedChunk &b : arrived ) m_ready[b.m_index] = std::move( b );
        while( !m_ready.empty() && m_ready.begin()->first < next ) m_ready.erase( m_ready.begin() );
        for( int uploaded = 0; uploaded < std::max( 1, maxBuilds ) && next <= lastWanted; ++next )
        {
            auto it = m_ready.find( next );
            if( it != m_ready.end() )
            {
                upload( it->second );
                m_ready.erase( it );
                ++uploaded;
                continue;
            }
            // Not there yet: wait for it unless the car is close to the end of the world.
            if( static_cast<double>( next ) * ChunkLength - s > 250.0 ) break;
            bakeHere( next );
            ++m_syncBuilds;
            ++uploaded;
        }
    }
    m_buildMs = std::chrono::duration<float, std::milli>( std::chrono::steady_clock::now() - t0 ).count();
    m_buildPeakMs = std::max( m_buildMs, m_buildPeakMs * 0.995f );
}

void World::collidersNear( double s, double range, std::vector<const Collider *> &out ) const
{
    for( const Chunk &c : m_chunks )
    {
        const double c0 = static_cast<double>( c.m_index ) * ChunkLength;
        if( c0 > s + range || c0 + ChunkLength < s - range ) continue;
        for( const Collider &k : c.m_colliders )
            if( std::abs( k.m_s - s ) < range ) out.push_back( &k );
    }
}

void World::buildChunk( Track &track, BakedChunk &c ) const
{
    const long i0 = c.m_index * ChunkSegments;
    track.generateTo( static_cast<double>( i0 + ChunkSegments + 2 ) * SegmentLength );
    c.m_origin = track.segment( i0 ).m_pos;
    const uint64_t seed = track.seed();
    const Models  &models = *m_models;

    auto cross = [&]( long j )
    {
        Cross          x;
        const Segment &seg = track.segment( j );
        x.m_index = j;
        x.m_seg = &seg;
        x.m_center = relativeTo( seg.m_pos, c.m_origin );
        x.m_heading = seg.m_heading;
        x.m_right = headingRight( seg.m_heading );
        x.m_forward = headingForward( seg.m_heading );
        x.m_tanBank = std::tan( seg.m_bank );
        x.m_hw = seg.m_halfWidth;
        x.m_edge = seg.m_halfWidth + RumbleWidth + ShoulderWidth;
        const BiomeDef  &a = biomeDef( static_cast<BiomeE>( seg.m_biome ) );
        const BiomeDef  &b = biomeDef( static_cast<BiomeE>( seg.m_biomeTo ) );
        // The season of either place, before they blend.
        const float      phase = m_seasons.phaseAt( static_cast<double>( j ) * SegmentLength );
        const SeasonLook la = seasonAt( static_cast<BiomeE>( seg.m_biome ), phase ), lb = seasonAt( static_cast<BiomeE>( seg.m_biomeTo ), phase );
        Surface          sa = a.m_surface, sb = b.m_surface;
        seasonSurface( sa, la );
        seasonSurface( sb, lb );
        x.m_surface = blendSurface( sa, sb, seg.m_blend );
        x.m_season = SeasonLook{ lerpf( la.m_autumn, lb.m_autumn, seg.m_blend ), lerpf( la.m_winter, lb.m_winter, seg.m_blend ),
                                 lerpf( la.m_bare, lb.m_bare, seg.m_blend ), lerpf( la.m_spring, lb.m_spring, seg.m_blend ) };
        x.m_terrain = blendTerrain( a.m_terrain, b.m_terrain, seg.m_blend );
        return x;
    };

    MeshBuilder ground;
    MeshBuilder props;
    MeshBuilder scenery, sceneryFar;
    MeshBuilder glass;

    // Light pools of the street lamps: lamps stand every k segments, right side at j % k == 0, left at k/2.
    auto lampLight = [&]( long j, float l, float t )
    {
        float light = 0.0f;
        for( long m = j - 12; m <= j + 12; ++m )
        {
            const Segment  &seg = track.segment( m );
            const BiomeDef &d = ruling( seg, 0.5f );
            if( d.m_lampSpacing <= 0.0f ) continue;
            const long k = std::max( 2L, static_cast<long>( d.m_lampSpacing / SegmentLength + 0.5f ) );
            float      side = 0.0f;
            if( m % k == 0 ) side = 1.0f;
            else if( m % k == k / 2 ) side = -1.0f;
            else continue;
            const float headL = side * ( seg.m_halfWidth + RumbleWidth + 1.0f - 2.9f );
            const float ds = ( static_cast<float>( j - m ) + t ) * SegmentLength;
            const float dl = l - headL;
            light += std::exp( -( ds * ds / 55.0f + dl * dl / 40.0f ) );
        }
        return std::min( light, 1.2f );
    };

    for( long j = i0; j < i0 + ChunkSegments; ++j )
    {
        const Cross    A = cross( j );
        const Cross    B = cross( j + 1 );
        const Segment &seg = *A.m_seg;
        const Surface &sf = A.m_surface;
        const bool     tunnel = seg.m_flags & SEG_TUNNEL;
        const bool     bridge = seg.m_flags & SEG_BRIDGE;
        ground.setShade( tunnel ? 0.12f : 1.0f );

        // A ferry or a car train over the sea: from where the car drives aboard to where it drives off, the vessels'
        // decks, open water to the horizon (or the bank the train goes into) and the vessels themselves.
        const double   sj = static_cast<double>( j ) * SegmentLength;
        // A fast train's viaduct beside the road (Train.h), from a stone portal to a stone portal; props keep clear of it.
        TrainStretch   railway;
        const bool     hasRailway = !tunnel && !bridge && stageTrain( track, static_cast<int>( sj / StageLength ), railway );
        const double   viaFrom = railway.m_from - RailBefore, viaTo = railway.m_to + RailAfter;
        const bool     viaduct = hasRailway && sj >= viaFrom - PortalLength && sj < viaTo + PortalLength;
        const float    railL = railway.m_side * ( A.m_edge + RailOffset );
        const Crossing xc = track.crossingAt( sj );
        if( xc.m_type != CrossingE::NONE && sj >= xc.m_board && sj < xc.m_land )
        {
            const bool      ferry = xc.m_type == CrossingE::FERRY;
            const bool      out = sj < xc.m_board + xc.m_length, in = sj >= xc.m_land - xc.m_length;
            const BiomeDef &shore = biomeDef( track.stageBiome( static_cast<int>( ( sj < ( xc.m_board + xc.m_land ) * 0.5 ? xc.m_board : xc.m_land ) / StageLength ) ) );
            const Surface  &sh = shore.m_surface;
            if( ferry )
            {
                ground.quad( at( A, -1500.0f, -3.0f ), at( A, 1500.0f, -3.0f ), at( B, 1500.0f, -3.0f ), at( B, -1500.0f, -3.0f ), sh.m_water,
                             Mat{ sh.m_waterGlow, sh.m_waterGloss } );
            }
            else if( out || in )
            {
                ground.quad( at( A, -40.0f, -0.25f ), at( A, 40.0f, -0.25f ), at( B, 40.0f, -0.25f ), at( B, -40.0f, -0.25f ), sh.m_shoulder );
                for( float side : { -1.0f, 1.0f } )
                    ground.quad( at( A, side * 40.0f, -0.25f ), at( A, side * 900.0f, 0.0f ), at( B, side * 900.0f, 0.0f ), at( B, side * 40.0f, -0.25f ),
                                 sh.m_groundFar );
            }
            else
            {
                // The bank over the tunnel: its top, and its face where the trains go in and come out.
                ground.quad( at( A, -900.0f, 22.0f ), at( A, 900.0f, 22.0f ), at( B, 900.0f, 22.0f ), at( B, -900.0f, 22.0f ), sh.m_groundFar );
                const double ua = sj - xc.m_board - xc.m_length, ub = xc.m_land - xc.m_length - sj - SegmentLength;
                if( ua >= 0.0 && ua < SegmentLength )
                    ground.quad( at( A, -900.0f, -0.3f ), at( A, 900.0f, -0.3f ), at( A, 900.0f, 22.0f ), at( A, -900.0f, 22.0f ), scaleColor( sh.m_groundFar, 0.8f ) );
                if( ub >= 0.0 && ub < SegmentLength )
                    ground.quad( at( B, -900.0f, -0.3f ), at( B, 900.0f, -0.3f ), at( B, 900.0f, 22.0f ), at( B, -900.0f, 22.0f ), scaleColor( sh.m_groundFar, 0.8f ) );
            }
            if( out || in )
                ground.quad( at( A, -A.m_hw, 0.0f ), at( A, A.m_hw, 0.0f ), at( B, B.m_hw, 0.0f ), at( B, -B.m_hw, 0.0f ), Color{ 70, 74, 80, 255 } );
            // The vessels, open towards the land; beyond the trains the mouth of their tunnel.
            const int    vessel = landmarkByName( ferry ? "FERRY_SHIP" : "CAR_TRAIN" ), mouth = landmarkByName( "TUNNEL_PORTAL" );
            const double ua = sj - xc.m_board, ub = xc.m_land - sj - SegmentLength;
            if( vessel >= 0 && ua < SegmentLength ) props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( vessel ) ), at( A, 0.0f, 0.0f ), A.m_heading, 1.0f );
            if( vessel >= 0 && ub >= 0.0 && ub < SegmentLength )
                props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( vessel ) ), at( B, 0.0f, 0.0f ), B.m_heading + Tau * 0.5f, 1.0f );
            if( !ferry && mouth >= 0 )
            {
                const double ma = ua - xc.m_length - 5.0, mb = ub - xc.m_length - 5.0;
                if( ma >= 0.0 && ma < SegmentLength ) props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( mouth ) ), at( A, 0.0f, 0.0f ), A.m_heading, 1.0f );
                if( mb >= 0.0 && mb < SegmentLength )
                    props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( mouth ) ), at( B, 0.0f, 0.0f ), B.m_heading + Tau * 0.5f, 1.0f );
            }
            continue;
        }
        const bool     band = ( j / 2 ) % 2 == 0;
        const int      lanes = seg.m_lanes;
        // The country's markings: yellow between the directions (the Americas, Scandinavia, East Asia; on a road of an
        // odd number of lanes its centre-side edge), on the centre-side edge (Arabia) or on the outer edge (southern
        // Africa, Ireland), mirrored where they drive on the left.
        const BiomeDef    &here = ruling( seg, 0.5f );
        const CountryInfo &land = countryInfo( here.m_country );
        // A fork of the world tour: where its barrier begins (u = 0) and where the other carriageway goes.
        const Fork   fork = track.fork( seg.m_stage );
        const double u = static_cast<double>( j ) * SegmentLength - fork.m_at;
        const int    half = lanes / 2;
        const float  carriage = static_cast<float>( std::max( 1, half ) ) * here.m_laneWidth * 0.5f;   // half a carriageway
        auto         branchX = [&]( double v ) -> float
        {
            const double w = std::clamp( v - ForkMedian, 0.0, ForkPeel );
            return static_cast<float>( -fork.m_chosen ) * ( carriage + 0.6f + static_cast<float>( 130.0 * ( w / ForkPeel ) * ( w / ForkPeel ) ) );
        };
        const bool peeling = fork.m_exists && fork.m_chosen != 0 && u >= ForkMedian && u < ForkMedian + ForkPeel;
        const Color roadCol = band ? sf.m_road : sf.m_roadAlt;
        const bool  marked = here.m_country && sf.m_lineGlow <= 0.0f && !here.m_unpaved;
        const bool  between = marked && land.m_lines == 1 && lanes % 2 == 0;   // yellow down the middle
        Color       lineL = sf.m_line, lineR = sf.m_line, centre = sf.m_line;
        if( marked )
        {
            const Color yellow = { 242, 188, 34, 255 };
            if( land.m_lines == 3 || ( land.m_lines == 1 && !between ) ) ( land.m_left ? lineR : lineL ) = yellow;
            if( land.m_lines == 2 ) ( land.m_left ? lineL : lineR ) = yellow;
            if( between ) centre = yellow;
            // Worn roads: the paint fades into the asphalt.
            const float fade = 0.55f * here.m_wear;
            lineL = lerpColor( lineL, roadCol, fade );
            lineR = lerpColor( lineR, roadCol, fade );
            centre = lerpColor( centre, roadCol, fade );
        }
        const Color dash = marked ? lerpColor( sf.m_line, roadCol, 0.55f * here.m_wear ) : sf.m_line;
        // Gravel where a poor country's asphalt ends, at either end of the segment; its colour that of the verge.
        const float gA = here.m_gravel > 0.0f ? gravelAt( track, static_cast<double>( j ) * SegmentLength ) : 0.0f;
        const float gB = here.m_gravel > 0.0f ? gravelAt( track, static_cast<double>( j + 1 ) * SegmentLength ) : 0.0f;
        Color       dirt = sf.m_shoulder;
        if( dirt.g > dirt.r ) dirt = lerpColor( dirt, Color{ 150, 128, 96, 255 }, 0.65f );
        if( !band ) dirt = scaleColor( dirt, 0.95f );

        // Road surface: strips between the edge lines and lane dividers, each with per-corner lamp light. A column
        // sits at a fraction of the half width plus fixed metres, resolved per cross-section.
        struct Column
        {
            float  m_frac;    ///< fraction of the half width (-1..1)
            float  m_off;     ///< plus metres
            StripE m_kind;    ///< the strip that starts here
        };
        std::vector<Column> cols;
        cols.push_back( { -1.0f, 0.0f, StripE::ROAD } );
        cols.push_back( { -1.0f, 0.25f, StripE::LINE } );
        cols.push_back( { -1.0f, 0.45f, StripE::ROAD } );
        for( int k = 1; k < lanes; ++k )
        {
            const float f = -1.0f + 2.0f * static_cast<float>( k ) / static_cast<float>( lanes );
            if( fork.m_exists && k * 2 == lanes && u > -120.0 && u < ForkMedian )
            {
                // The gore before the barrier: a hatched wedge between the carriageways.
                const float g = u < 0.0 ? 0.6f * smoothstepf( -120.0f, 0.0f, static_cast<float>( u ) ) : 0.6f;
                cols.push_back( { 0.0f, -g - 0.15f, StripE::LINE } );
                cols.push_back( { 0.0f, -g, StripE::DASH } );
                cols.push_back( { 0.0f, g, StripE::LINE } );
                cols.push_back( { 0.0f, g + 0.15f, StripE::ROAD } );
                continue;
            }
            if( between && k * 2 == lanes && lanes >= 4 )
            {
                cols.push_back( { f, -0.22f, StripE::LINE } );
                cols.push_back( { f, -0.08f, StripE::ROAD } );
                cols.push_back( { f, 0.08f, StripE::LINE } );
                cols.push_back( { f, 0.22f, StripE::ROAD } );
                continue;
            }
            cols.push_back( { f, -0.08f, StripE::DASH } );
            cols.push_back( { f, 0.08f, StripE::ROAD } );
        }
        cols.push_back( { 1.0f, -0.45f, StripE::LINE } );
        cols.push_back( { 1.0f, -0.25f, StripE::ROAD } );
        cols.push_back( { 1.0f, 0.0f, StripE::ROAD } );
        const bool dashOn = band && !( seg.m_flags & SEG_NO_DASH );
        const float wear = marked ? here.m_wear : 0.0f;
        for( size_t k = 0; k + 1 < cols.size(); ++k )
        {
            const float la0 = cols[k].m_frac * A.m_hw + cols[k].m_off, la1 = cols[k + 1].m_frac * A.m_hw + cols[k + 1].m_off;
            const float lb0 = cols[k].m_frac * B.m_hw + cols[k].m_off, lb1 = cols[k + 1].m_frac * B.m_hw + cols[k + 1].m_off;
            const bool  middle = std::abs( cols[k].m_frac ) < 0.001f && between;
            Color       base = roadCol;
            float       glow = 0.0f;
            float       gloss = 0.18f;
            const bool  cobbled = here.m_cobbles && cols[k].m_kind != StripE::LINE;
            // A worn road loses dashes, and stretches of its edge lines.
            const bool dashGone = wear > 0.0f && hashFloat( seed * 23 + k, static_cast<uint64_t>( j / 4 ) ) < 0.45f * wear;
            const bool lineGone = wear > 0.45f && !middle && hashFloat( seed * 29 + k, static_cast<uint64_t>( j / 3 ) ) < 1.2f * ( wear - 0.45f );
            if( ( cols[k].m_kind == StripE::LINE && !lineGone ) || ( cols[k].m_kind == StripE::DASH && dashOn && !dashGone ) )
            {
                base = middle ? centre : cols[k].m_kind == StripE::LINE ? ( cols[k].m_frac < 0.0f ? lineL : lineR ) : dash;
                glow = sf.m_lineGlow * 0.45f;
                gloss = 0.3f;
            }
            else if( wear > 0.0f && cols[k].m_kind == StripE::ROAD )
            {
                // Patches: fresh black tar, or old asphalt bleached grey.
                const float h = hashFloat( seed * 19 + k, static_cast<uint64_t>( j / 2 ) );
                if( h < 0.22f * wear ) base = scaleColor( base, h < 0.11f * wear ? 0.78f : 1.12f );
            }
            const Vector3 p[4] = { at( A, la0, 0 ), at( A, la1, 0 ), at( B, lb1, 0 ), at( B, lb0, 0 ) };
            const float   ls[4] = { lampLight( j, la0, 0 ), lampLight( j, la1, 0 ), lampLight( j, lb1, 1 ), lampLight( j, lb0, 1 ) };
            Color         col[4];
            Mat           mat[4];
            for( int v = 0; v < 4; ++v )
            {
                const float g = v < 2 ? gA : gB;
                col[v] = lerpColor( lerpColor( base, dirt, g ), WarmLight, ls[v] * 0.5f );
                mat[v] = cobbled ? Mat{ -2.0f, 0.12f } : Mat{ glow * ( 1.0f - g ) + ls[v] * 0.22f, lerpf( gloss, 0.04f, g ) };
            }
            ground.quadV( p, col, mat );
        }

        // On the road: the joints of concrete slabs, potholes, and the studs along the lines.
        auto along = [&]( float t, float l, float h )
        {
            const Vector3 a = at( A, l, h ), b = at( B, l, h );
            return Vector3{ a.x + ( b.x - a.x ) * t, a.y + ( b.y - a.y ) * t, a.z + ( b.z - a.z ) * t };
        };
        if( here.m_concrete && gA <= 0.0f && !bridge )
        {
            const Color joint = scaleColor( roadCol, 0.72f );
            ground.quad( along( 0.0f, -A.m_hw, 0.008f ), along( 0.0f, A.m_hw, 0.008f ), along( 0.03f, A.m_hw, 0.008f ), along( 0.03f, -A.m_hw, 0.008f ), joint );
        }
        float holeX = 0.0f, holeR = 0.0f;
        if( potholeAt( track, j, holeX, holeR ) )
        {
            const Color  hole = scaleColor( roadCol, 0.42f );
            const float  rt = holeR / SegmentLength, d = 0.72f;
            ground.quad( along( 0.5f - rt, holeX, 0.012f ), along( 0.5f, holeX + holeR, 0.012f ), along( 0.5f + rt, holeX, 0.012f ), along( 0.5f, holeX - holeR, 0.012f ),
                         hole );
            ground.quad( along( 0.5f - rt * d, holeX - holeR * d, 0.012f ), along( 0.5f - rt * d, holeX + holeR * d, 0.012f ),
                         along( 0.5f + rt * d, holeX + holeR * d, 0.012f ), along( 0.5f + rt * d, holeX - holeR * d, 0.012f ), hole );
        }
        if( here.m_studs && marked && gA <= 0.0f && j % 2 == 0 )
        {
            // White between the lanes, red along the outer edge, amber along the centre side (and down a yellow middle).
            const Color white = { 236, 236, 230, 255 }, red = { 220, 40, 30, 255 }, amber = { 240, 160, 30, 255 };
            const float outer = land.m_left ? -1.0f : 1.0f;
            auto        stud = [&]( float l, Color col )
            {
                ground.quad( along( 0.0f, l - 0.07f, 0.02f ), along( 0.0f, l + 0.07f, 0.02f ), along( 0.03f, l + 0.07f, 0.02f ), along( 0.03f, l - 0.07f, 0.02f ),
                             col, Mat{ 0.7f, 0.5f } );
            };
            stud( outer * ( A.m_hw - 0.35f ), red );
            stud( -outer * ( A.m_hw - 0.35f ), amber );
            for( int k = 1; k < lanes; ++k )
            {
                if( fork.m_exists && k * 2 == lanes && u > -120.0 && u < ForkMedian ) continue;
                stud( ( -1.0f + 2.0f * static_cast<float>( k ) / static_cast<float>( lanes ) ) * A.m_hw, between && k * 2 == lanes ? amber : white );
            }
        }

        // Kerbs and shoulders on both sides.
        for( float side : { -1.0f, 1.0f } )
        {
            Color kerb = j % 2 == 0 ? sf.m_rumbleA : sf.m_rumbleB;
            if( wear > 0.6f && !here.m_kerbs && hashFloat( seed * 41 + ( side > 0.0f ), static_cast<uint64_t>( j ) ) < wear - 0.5f ) kerb = scaleColor( roadCol, 0.9f );
            const float l0a = side * A.m_hw, l1a = side * ( A.m_hw + RumbleWidth ), l2a = side * A.m_edge;
            const float l0b = side * B.m_hw, l1b = side * ( B.m_hw + RumbleWidth ), l2b = side * B.m_edge;
            {
                const Vector3 p[4] = { at( A, l0a, 0.02f ), at( A, l1a, 0.02f ), at( B, l1b, 0.02f ), at( B, l0b, 0.02f ) };
                const Color   ka = lerpColor( kerb, dirt, gA ), kb = lerpColor( kerb, dirt, gB );
                const Color   col[4] = { ka, ka, kb, kb };
                const Mat     m = { sf.m_lineGlow * 0.35f, 0.2f };
                const Mat     mat[4] = { m, m, m, m };
                ground.quadV( p, col, mat );
            }
            if( bridge )
            {
                const Color fascia = scaleColor( bridgeBiome( seg ).m_bridgeColor, 0.9f );
                ground.quad( at( A, l1a, 0.0f ), at( B, l1b, 0.0f ), at( B, l1b, -1.6f ), at( A, l1a, -1.6f ), fascia );
                continue;
            }
            {
                const float   ha = terrainHeight( track, j, l2a, A.m_terrain, A.m_edge );
                const float   hb = terrainHeight( track, j + 1, l2b, B.m_terrain, B.m_edge );
                const Vector3 p[4] = { at( A, l1a, 0.0f ), at( A, l2a, ha ), at( B, l2b, hb ), at( B, l1b, 0.0f ) };
                const float   ls[4] = { lampLight( j, l1a, 0 ), lampLight( j, l2a, 0 ), lampLight( j, l2b, 1 ), lampLight( j, l1b, 1 ) };
                Color         col[4];
                Mat           mat[4];
                for( int v = 0; v < 4; ++v )
                {
                    col[v] = lerpColor( sf.m_shoulder, WarmLight, ls[v] * 0.4f );
                    mat[v] = Mat{ ls[v] * 0.15f, 0.0f };
                }
                ground.quadV( p, col, mat );
            }
        }

        // The land: stripes near the road, patchy colour further out, ponds in the marsh.
        static const float offsets[] = { 0.0f, 6.0f, 16.0f, 32.0f, 60.0f, 100.0f, 170.0f, 280.0f, 450.0f, 700.0f };
        constexpr int      offsetCount = static_cast<int>( sizeof( offsets ) / sizeof( offsets[0] ) );
        for( float side : { -1.0f, 1.0f } )
        {
            // In a serpentine the land ends where the next leg's begins.
            auto squeeze = [&]( const Cross &x )
            {
                const float reach = side < 0.0f ? x.m_seg->m_reachL : x.m_seg->m_reachR;
                return reach > 0.0f ? clampf( ( reach - x.m_edge ) / offsets[offsetCount - 1], 0.0f, 1.0f ) : 1.0f;
            };
            const float fa = squeeze( A ), fb = squeeze( B );
            for( int k = 0; k + 1 < offsetCount; ++k )
            {
                const float la0 = side * ( A.m_edge + offsets[k] * fa ), la1 = side * ( A.m_edge + offsets[k + 1] * fa );
                const float lb0 = side * ( B.m_edge + offsets[k] * fb ), lb1 = side * ( B.m_edge + offsets[k + 1] * fb );
                const float h[4] = { terrainHeight( track, j, la0, A.m_terrain, A.m_edge ),
                                     terrainHeight( track, j, la1, A.m_terrain, A.m_edge ),
                                     terrainHeight( track, j + 1, lb1, B.m_terrain, B.m_edge ),
                                     terrainHeight( track, j + 1, lb0, B.m_terrain, B.m_edge ) };
                const float mid = ( offsets[k] + offsets[k + 1] ) * 0.5f;
                const Color stripe = band ? sf.m_groundA : sf.m_groundB;
                const float patch = hashFloat( seed + static_cast<uint64_t>( k * 2 + ( side > 0 ) ), static_cast<uint64_t>( j / 3 ) );
                Color       col = lerpColor( stripe, sf.m_groundFar, smoothstepf( 20.0f, 110.0f, mid ) );
                col = scaleColor( col, 0.92f + 0.16f * patch );
                if( sf.m_fields > 0.0f && k >= 1 && k <= 6 )
                {
                    // Fields in blocks of 50 m, rows every 10 m.
                    const uint64_t cell = static_cast<uint64_t>( k + ( side > 0 ) * 11 );
                    if( hashFloat( seed + 333 + cell, static_cast<uint64_t>( j / 10 ) ) < sf.m_fields )
                    {
                        const float pick = hashFloat( seed + 444 + cell, static_cast<uint64_t>( j / 10 ) );
                        Color       field = pick < 0.4f ? sf.m_fieldA : pick < 0.75f ? sf.m_fieldB : sf.m_fieldC;
                        if( ( j / 2 ) % 2 ) field = scaleColor( field, 0.86f );
                        col = lerpColor( col, field, 0.9f );
                    }
                }
                // Steep faces show bare rock.
                const float run = std::max( 1.0f, std::abs( la1 - la0 ) );
                const float slope = std::max( std::abs( h[1] - h[0] ), std::abs( h[2] - h[3] ) ) / run;
                if( slope > 0.7f ) col = lerpColor( col, sf.m_shoulder, smoothstepf( 0.7f, 1.4f, slope ) * 0.85f );
                const bool pond = A.m_terrain.m_ponds > 0.0f && k >= 1 && k <= 4 &&
                                  hashFloat( seed + 99 + static_cast<uint64_t>( k + ( side > 0 ) * 7 ), static_cast<uint64_t>( j / 2 ) ) <
                                      A.m_terrain.m_ponds * 0.3f;
                if( pond )
                {
                    const float   y = std::min( { h[0], h[1], h[2], h[3] } ) - 0.15f;
                    const Vector3 p[4] = { at( A, la0, y ), at( A, la1, y ), at( B, lb1, y ), at( B, lb0, y ) };
                    ground.quad( p[0], p[1], p[2], p[3], sf.m_water, Mat{ sf.m_waterGlow, sf.m_waterGloss } );
                    continue;
                }
                const Vector3 p[4] = { at( A, la0, h[0] ), at( A, la1, h[1] ), at( B, lb1, h[2] ), at( B, lb0, h[3] ) };
                ground.quad( p[0], p[1], p[2], p[3], col, Mat{ GroundMat.m_emissive, sf.m_groundGloss } );
            }
        }
        // The sea: left, right or both (sinking away while a crossfade removes it).
        for( float side : { -1.0f, 1.0f } )
        {
            const float w = side < 0 ? A.m_terrain.m_waterLeft : A.m_terrain.m_waterRight;
            if( w <= 0.05f ) continue;
            const float la0 = side * ( A.m_edge + 14.0f ), la1 = side * ( A.m_edge + 1200.0f );
            const float lb0 = side * ( B.m_edge + 14.0f ), lb1 = side * ( B.m_edge + 1200.0f );
            const float y = A.m_terrain.m_seaLevel - ( 1.0f - w ) * 20.0f, ya = y + A.m_seg->m_trench, yb = y + B.m_seg->m_trench;
            ground.quad( at( A, la0, ya ), at( A, la1, ya ), at( B, lb1, yb ), at( B, lb0, yb ), sf.m_water,
                         Mat{ sf.m_waterGlow, sf.m_waterGloss } );
        }

        if( tunnel )
        {
            const float wa = A.m_hw + RumbleWidth + 0.6f, wb = B.m_hw + RumbleWidth + 0.6f;
            const Color concrete = { 140, 138, 132, 255 };
            const Color skirt = { 90, 92, 96, 255 };
            props.setShade( 0.1f );
            const Vector3 a[6] = { at( A, -wa, 0.0f ), at( A, -wa, 5.0f ), at( A, -wa * 0.55f, 7.2f ), at( A, wa * 0.55f, 7.2f ),
                                   at( A, wa, 5.0f ), at( A, wa, 0.0f ) };
            const Vector3 b[6] = { at( B, -wb, 0.0f ), at( B, -wb, 5.0f ), at( B, -wb * 0.55f, 7.2f ), at( B, wb * 0.55f, 7.2f ),
                                   at( B, wb, 5.0f ), at( B, wb, 0.0f ) };
            for( int k = 0; k < 5; ++k ) props.quad( a[k], b[k], b[k + 1], a[k + 1], k == 2 ? scaleColor( concrete, 0.8f ) : concrete );
            for( float side : { -1.0f, 1.0f } )
                props.quad( at( A, side * wa * 0.99f, 0.0f ), at( B, side * wb * 0.99f, 0.0f ), at( B, side * wb * 0.99f, 1.0f ),
                            at( A, side * wa * 0.99f, 1.0f ), skirt );
            // Lights every 10 m on both slopes: they rush past.
            if( j % 2 == 0 )
            {
                for( float side : { -1.0f, 1.0f } )
                {
                    const Vector3 p0 = Vector3Lerp( at( A, side * wa, 5.0f ), at( A, side * wa * 0.55f, 7.2f ), 0.5f );
                    const Vector3 p1 = Vector3Lerp( at( B, side * wb, 5.0f ), at( B, side * wb * 0.55f, 7.2f ), 0.5f );
                    const Vector3 q0 = Vector3Lerp( p0, p1, 0.1f ), q1 = Vector3Lerp( p0, p1, 0.4f );
                    const Vector3 up = { 0.0f, 0.18f, 0.0f };
                    props.quad( Vector3Subtract( q0, up ), Vector3Subtract( q1, up ), Vector3Add( q1, up ), Vector3Add( q0, up ),
                                Color{ 255, 196, 120, 255 }, Mat{ 1.0f, 0.0f } );
                }
            }
            props.setShade( 1.0f );
            // The hill's lid over the tube, and the portals.
            const float ea = A.m_edge + 6.0f, eb = B.m_edge + 6.0f;
            const Color lid = sf.m_groundFar;
            props.quad( at( A, -ea, TunnelLift ), at( B, -eb, TunnelLift ), at( B, eb, TunnelLift ), at( A, ea, TunnelLift ), lid );
            const bool entry = !( track.segment( j - 1 ).m_flags & SEG_TUNNEL );
            const bool exit = !( track.segment( j + 1 ).m_flags & SEG_TUNNEL );
            for( int end = 0; end < 2; ++end )
            {
                if( ( end == 0 && !entry ) || ( end == 1 && !exit ) ) continue;
                const Cross &X = end == 0 ? A : B;
                const float  wx = X.m_hw + RumbleWidth + 0.6f, ex = X.m_edge + 6.0f;
                const Color  face = { 172, 170, 162, 255 };
                props.quad( at( X, -ex, 0.0f ), at( X, -wx, 0.0f ), at( X, -wx, TunnelLift ), at( X, -ex, TunnelLift ), face );
                props.quad( at( X, wx, 0.0f ), at( X, ex, 0.0f ), at( X, ex, TunnelLift ), at( X, wx, TunnelLift ), face );
                props.quad( at( X, -wx, 7.2f ), at( X, wx, 7.2f ), at( X, wx, TunnelLift ), at( X, -wx, TunnelLift ), face );
                props.tri( at( X, -wx, 5.0f ), at( X, -wx * 0.55f, 7.2f ), at( X, -wx, 7.2f ), face );
                props.tri( at( X, wx, 5.0f ), at( X, wx, 7.2f ), at( X, wx * 0.55f, 7.2f ), face );
                props.quad( at( X, -wx - 0.6f, 7.2f ), at( X, wx + 0.6f, 7.2f ), at( X, wx + 0.6f, 8.2f ), at( X, -wx - 0.6f, 8.2f ),
                            Color{ 120, 118, 112, 255 } );
            }
        }
        if( bridge )
        {
            const BiomeDef &bd = bridgeBiome( seg );
            const Color     paint = bd.m_bridgeColor;
            const float     da = A.m_hw + RumbleWidth, db = B.m_hw + RumbleWidth;
            float           metres = ValleyDepth;
            const float     depth = valleyDepth( track, j, &metres ) * metres;
            const Mat       metal = { 0.0f, 0.45f };
            props.quad( at( A, -da, -1.6f ), at( B, -db, -1.6f ), at( B, db, -1.6f ), at( A, da, -1.6f ), scaleColor( paint, 0.7f ) );
            // Where the bridge begins and ends (the towers and pylons stand by those).
            long b0 = j, b1 = j;
            track.generateTo( static_cast<double>( j + 420 ) * SegmentLength );
            while( b0 > std::max( track.firstIndex(), j - 400 ) && ( track.segment( b0 - 1 ).m_flags & SEG_BRIDGE ) ) --b0;
            while( b1 < std::min( track.endIndex() - 2, j + 400 ) && ( track.segment( b1 + 1 ).m_flags & SEG_BRIDGE ) ) ++b1;
            auto piers = [&]( float half, float inset, Color col )
            {
                const float h = ( depth + 2.4f ) * 0.5f;
                for( float side : { -1.0f, 1.0f } ) obox( props, at( A, side * ( da - inset ), -1.6f - h ), A.m_right, A.m_forward, half, h, half, col );
            };
            switch( bd.m_bridge )
            {
            case BridgeE::VIADUCT:
                if( j % 10 == 0 )
                { // One tall slim pier with a hammerhead.
                    const float h = ( depth + 1.0f ) * 0.5f;
                    obox( props, at( A, 0.0f, -2.4f - h ), A.m_right, A.m_forward, 2.2f, h, 1.6f, paint );
                    obox( props, at( A, 0.0f, -2.0f ), A.m_right, A.m_forward, da, 0.4f, 1.8f, scaleColor( paint, 0.9f ) );
                }
                break;
            case BridgeE::ARCH:
            { // Stone arches of 20 m on piers, solid spandrels.
                const int   k = static_cast<int>( ( ( j % 4 ) + 4 ) % 4 );
                const float rise = 7.0f;
                auto        arch = [&]( float t ) { return -2.4f - rise * ( 1.0f - std::sin( 3.14159f * t ) ); };
                const float y0 = arch( k / 4.0f ), y1 = arch( ( k + 1 ) / 4.0f );
                for( float side : { -1.0f, 1.0f } )
                    props.quad( at( A, side * da, -1.6f ), at( B, side * db, -1.6f ), at( B, side * db, y1 ), at( A, side * da, y0 ), paint );
                props.quad( at( A, -da, y0 ), at( A, da, y0 ), at( B, db, y1 ), at( B, -db, y1 ), scaleColor( paint, 0.55f ) );
                if( k == 0 )
                {
                    const float top = -2.4f - rise, h = ( top + depth + 2.0f ) * 0.5f;
                    obox( props, at( A, 0.0f, top - h ), A.m_right, A.m_forward, da + 0.3f, h, 1.6f, scaleColor( paint, 0.92f ) );
                }
                break;
            }
            case BridgeE::TRUSS:
            { // A through truss on both sides, braced overhead.
                if( j % 8 == 0 ) piers( 0.9f, 1.0f, scaleColor( paint, 0.8f ) );
                for( float side : { -1.0f, 1.0f } )
                {
                    const float xa = side * ( da + 0.9f ), xb = side * ( db + 0.9f );
                    props.tube( at( A, xa, 6.5f ), at( B, xb, 6.5f ), 0.25f, 4, paint, metal );
                    props.tube( at( A, xa, 0.2f ), at( B, xb, 0.2f ), 0.22f, 4, paint, metal );
                    props.tube( at( A, xa, 0.2f ), at( A, xa, 6.5f ), 0.16f, 4, paint, metal );
                    if( j % 2 == 0 ) props.tube( at( A, xa, 0.2f ), at( B, xb, 6.5f ), 0.12f, 4, paint, metal );
                    else props.tube( at( A, xa, 6.5f ), at( B, xb, 0.2f ), 0.12f, 4, paint, metal );
                }
                if( j % 2 == 0 )
                {
                    props.tube( at( A, -da - 0.9f, 6.5f ), at( A, da + 0.9f, 6.5f ), 0.14f, 4, paint, metal );
                    props.tube( at( A, -da - 0.9f, 6.5f ), at( B, db + 0.9f, 6.5f ), 0.08f, 4, paint, metal );
                }
                break;
            }
            case BridgeE::SUSPENSION:
            { // Two towers, the main cables in a curve between them, hangers.
                const long  n = b1 - b0 + 1;
                const long  t1 = b0 + std::max( 2L, n / 5 ), t2 = b1 - std::max( 2L, n / 5 );
                const float top = 42.0f;
                auto        cable = [&]( long i )
                {
                    if( i <= t1 ) return lerpf( 1.2f, top, static_cast<float>( i - b0 ) / static_cast<float>( std::max( 1L, t1 - b0 ) ) );
                    if( i >= t2 ) return lerpf( top, 1.2f, static_cast<float>( i - t2 ) / static_cast<float>( std::max( 1L, b1 + 1 - t2 ) ) );
                    const float mid = 0.5f * static_cast<float>( t1 + t2 ), half = 0.5f * static_cast<float>( t2 - t1 );
                    const float u = ( static_cast<float>( i ) - mid ) / half;
                    return 2.5f + ( top - 2.5f ) * u * u;
                };
                for( float side : { -1.0f, 1.0f } )
                {
                    const float xa = side * ( da + 1.3f ), xb = side * ( db + 1.3f );
                    props.tube( at( A, xa, cable( j ) ), at( B, xb, cable( j + 1 ) ), 0.3f, 5, paint, metal );
                    if( cable( j ) > 2.6f ) props.tube( at( A, xa, 0.3f ), at( A, xa, cable( j ) ), 0.05f, 3, paint, metal );
                    props.quad( at( A, side * da, -1.6f ), at( B, side * db, -1.6f ), at( B, side * db, -3.4f ), at( A, side * da, -3.4f ),
                                scaleColor( paint, 0.75f ) );
                }
                if( j == t1 || j == t2 )
                {
                    const float bottom = -depth - 2.0f, h = ( top + 3.0f - bottom ) * 0.5f;
                    for( float side : { -1.0f, 1.0f } ) obox( props, at( A, side * ( da + 1.3f ), bottom + h ), A.m_right, A.m_forward, 1.1f, h, 1.5f, paint );
                    for( float y : { top * 0.55f, top + 1.5f } ) obox( props, at( A, 0.0f, y ), A.m_right, A.m_forward, da + 1.3f, 0.9f, 1.0f, paint );
                    obox( props, at( A, 0.0f, -3.0f ), A.m_right, A.m_forward, da + 1.3f, 0.9f, 1.2f, scaleColor( paint, 0.85f ) );
                }
                break;
            }
            case BridgeE::CABLE_STAYED:
            { // An A-shaped pylon every 180 m, a fan of stays to the deck edges.
                const long  n = b1 - b0 + 1, period = 36;
                const long  pylon = n < period ? ( b0 + b1 ) / 2 : std::min( b1 - 4, b0 + ( ( j - b0 ) / period ) * period + period / 2 );
                const float top = 38.0f;
                const Cross P = cross( pylon );
                if( j == pylon )
                {
                    const float bottom = -depth - 2.0f;
                    for( float side : { -1.0f, 1.0f } )
                        props.tube( at( A, side * ( da + 2.0f ), bottom ), at( A, side * 0.7f, top ), 1.0f, 6, paint );
                    props.tube( at( A, -da - 1.4f, -2.0f ), at( A, da + 1.4f, -2.0f ), 0.8f, 6, paint );
                    props.tube( at( A, -1.0f, top - 1.0f ), at( A, 1.0f, top - 1.0f ), 0.9f, 6, paint );
                }
                const long d = std::abs( j - pylon );
                if( d >= 2 && j % 2 == 0 )
                    for( float side : { -1.0f, 1.0f } )
                        props.tube( at( P, side * 0.9f, top - 3.0f - 0.25f * static_cast<float>( std::min( d, 30L ) ) ), at( A, side * ( da + 0.6f ), 0.4f ),
                                    0.07f, 3, scaleColor( paint, 1.05f ), metal );
                break;
            }
            case BridgeE::COVERED:
            { // Plank walls and a shingled roof; stone abutments at the ends.
                const Color roof = { 70, 58, 52, 255 };
                for( float side : { -1.0f, 1.0f } )
                {
                    const float xa = side * ( da + 0.7f ), xb = side * ( db + 0.7f );
                    props.quad( at( A, xa, 0.0f ), at( B, xb, 0.0f ), at( B, xb, 4.4f ), at( A, xa, 4.4f ), paint );
                    props.quad( at( A, xa, 4.4f ), at( B, xb, 4.4f ), at( B, 0.0f, 6.6f ), at( A, 0.0f, 6.6f ), roof );
                }
                for( long end : { b0, b1 } )
                    if( j == end )
                    {
                        const Cross &E = end == b0 ? A : B;
                        props.tri( at( E, -da - 0.7f, 4.4f ), at( E, da + 0.7f, 4.4f ), at( E, 0.0f, 6.6f ), scaleColor( paint, 0.85f ) );
                        const float h = ( depth + 2.0f ) * 0.5f;
                        obox( props, at( E, 0.0f, -1.6f - h ), E.m_right, E.m_forward, da + 1.0f, h, 1.4f, Color{ 140, 134, 124, 255 } );
                    }
                break;
            }
            case BridgeE::CAUSEWAY:
                if( j % 3 == 0 ) piers( 0.45f, 1.2f, scaleColor( paint, 0.85f ) );
                break;
            default:
                if( j % 6 == 0 )
                {
                    piers( 0.75f, 1.2f, paint );
                    obox( props, at( A, 0.0f, -2.0f ), A.m_right, A.m_forward, da, 0.4f, 0.6f, paint );
                }
                break;
            }
            const float y = -depth + 1.2f;
            ground.quad( at( A, -160.0f, y ), at( B, -160.0f, y ), at( B, 160.0f, y ), at( A, 160.0f, y ), sf.m_water,
                         Mat{ sf.m_waterGlow, sf.m_waterGloss } );
        }

        // Guard rails, or tall sound walls.
        for( float side : { -1.0f, 1.0f } )
        {
            if( !( seg.m_flags & ( side < 0 ? SEG_RAIL_L : SEG_RAIL_R ) ) ) continue;
            const float   la = side * ( A.m_hw + RumbleWidth + 0.55f ), lb = side * ( B.m_hw + RumbleWidth + 0.55f );
            const BridgeE kind = bridge ? bridgeBiome( seg ).m_bridge : BridgeE::BEAM;
            if( bridge && kind == BridgeE::COVERED ) continue;
            if( bridge && kind == BridgeE::ARCH )
            {
                const Color stone = bridgeBiome( seg ).m_bridgeColor;
                const float lo = side * 0.5f;
                props.quad( at( A, la, 0.0f ), at( B, lb, 0.0f ), at( B, lb, 1.0f ), at( A, la, 1.0f ), scaleColor( stone, 0.9f ) );
                props.quad( at( A, la, 1.0f ), at( B, lb, 1.0f ), at( B, lb + lo, 1.0f ), at( A, la + lo, 1.0f ), scaleColor( stone, 1.05f ) );
                props.quad( at( A, la + lo, -1.6f ), at( B, lb + lo, -1.6f ), at( B, lb + lo, 1.0f ), at( A, la + lo, 1.0f ), stone );
                continue;
            }
            if( here.m_walls )
            {
                const Color concrete = { 172, 170, 162, 255 };
                const float lo = side * 0.3f;
                props.quad( at( A, la, 0.0f ), at( B, lb, 0.0f ), at( B, lb, 3.0f ), at( A, la, 3.0f ), concrete );
                props.quad( at( A, la, 3.0f ), at( B, lb, 3.0f ), at( B, lb + lo, 3.0f ), at( A, la + lo, 3.0f ), scaleColor( concrete, 1.1f ) );
                props.quad( at( A, la + lo, 0.0f ), at( B, lb + lo, 0.0f ), at( B, lb + lo, 3.0f ), at( A, la + lo, 3.0f ),
                            scaleColor( concrete, 0.8f ) );
                if( j % 2 == 0 )
                {
                    const Vector3 base = at( A, la - side * 0.02f, 0.0f );
                    props.box( Vector3{ base.x, base.y + 1.5f, base.z }, Vector3{ 0.06f, 1.5f, 0.06f }, scaleColor( concrete, 0.7f ) );
                }
                continue;
            }
            const Color   steel = { 176, 180, 190, 255 };
            props.quad( at( A, la, 0.45f ), at( B, lb, 0.45f ), at( B, lb, 0.82f ), at( A, la, 0.82f ), steel, Mat{ 0.0f, 0.6f } );
            props.quad( at( A, la + side * 0.12f, 0.82f ), at( B, lb + side * 0.12f, 0.82f ), at( B, lb, 0.82f ),
                        at( A, la, 0.82f ), scaleColor( steel, 0.8f ) );
            const Vector3 base = at( A, la + side * 0.15f, 0.0f );
            props.box( Vector3{ base.x, base.y + 0.4f, base.z }, Vector3{ 0.07f, 0.4f, 0.07f }, Color{ 120, 124, 132, 255 } );
        }

        // On the sea floor: steel ribs of a glass tube over the road, rails along it, a light in every crown, a portal
        // where it begins and ends.
        {
            const bool under = ruling( seg, 0.5f ).m_underwater;
            const bool before = ruling( track.segment( j - 1 ), 0.5f ).m_underwater, after = ruling( track.segment( j + 1 ), 0.5f ).m_underwater;
            if( under )
            {
                const float   radius = A.m_hw + RumbleWidth + 2.6f;
                const Color   steel = { 70, 86, 96, 255 }, rail = { 110, 128, 138, 255 };
                const int     n = 10;
                auto          arc = [&]( const Cross &x, int k )
                {
                    const float th = Tau * 0.5f * static_cast<float>( k ) / static_cast<float>( n );
                    return at( x, -std::cos( th ) * radius, 0.2f + std::sin( th ) * radius * 0.82f );
                };
                if( j % 3 == 0 )
                {
                    for( int k = 0; k < n; ++k ) props.tube( arc( A, k ), arc( A, k + 1 ), 0.16f, 4, steel );
                    const Vector3 top = arc( A, n / 2 );
                    props.box( Vector3{ top.x, top.y - 0.25f, top.z }, Vector3{ 0.35f, 0.08f, 0.35f }, Color{ 200, 240, 255, 255 }, Mat{ 1.0f, 0.0f } );
                }
                for( int k : { 1, 3, 5, 7, 9 } ) props.tube( arc( A, k ), arc( B, k ), 0.06f, 3, rail );
                // Tinted glass between the ribs, drawn see-through after everything solid.
                const Color tint = { 46, 110, 124, 255 };
                for( int k = 0; k < n; ++k ) glass.quad( arc( A, k ), arc( B, k ), arc( B, k + 1 ), arc( A, k + 1 ), tint, Mat{ 0.0f, 0.9f } );
                if( !before || !after )
                {
                    // The portal: a thick concrete ring, set in a wall up to the land above the cutting.
                    const Color concrete = { 168, 170, 168, 255 };
                    const Cross &P = before ? B : A;
                    for( int k = 0; k < n; ++k ) props.tube( arc( P, k ), arc( P, k + 1 ), 0.9f, 5, concrete );
                    const float top = SeaDipDepth + 1.5f, wide = radius + 7.0f;
                    auto        outer = [&]( int k )
                    {
                        const float th = Tau * 0.5f * static_cast<float>( k ) / static_cast<float>( n );
                        return at( P, -std::cos( th ) * wide, 0.2f + std::sin( th ) * top );
                    };
                    for( int k = 0; k < n; ++k )
                    {
                        props.quad( arc( P, k ), outer( k ), outer( k + 1 ), arc( P, k + 1 ), scaleColor( concrete, 0.9f ) );
                        props.quad( arc( P, k + 1 ), outer( k + 1 ), outer( k ), arc( P, k ), scaleColor( concrete, 0.9f ) );
                    }
                }
            }
        }

        // The cutting down to an undersea tunnel: concrete walls hold the land back.
        {
            const float ta = A.m_seg->m_trench, tb = B.m_seg->m_trench;
            if( ta > 0.05f || tb > 0.05f )
            {
                const Color wall = { 172, 172, 166, 255 };
                for( float side : { -1.0f, 1.0f } )
                {
                    const float   la = side * ( A.m_edge + 2.5f ), lb = side * ( B.m_edge + 2.5f );
                    const Vector3 a0 = at( A, la, -0.5f ), a1 = at( A, la, ta + 0.7f ), b0 = at( B, lb, -0.5f ), b1 = at( B, lb, tb + 0.7f );
                    const Vector3 a2 = at( A, la + side * 0.6f, ta + 0.7f ), b2 = at( B, lb + side * 0.6f, tb + 0.7f );
                    props.quad( a0, b0, b1, a1, wall );
                    props.quad( a1, b1, b0, a0, wall );
                    props.quad( a1, b1, b2, a2, scaleColor( wall, 1.1f ) );
                    props.quad( a2, b2, b1, a1, scaleColor( wall, 1.1f ) );
                }
            }
        }

        // The port before a crossing: signs to the ferry (or the car train), the check-in, the quay wall; after the
        // crossing the far quay.
        if( xc.m_type != CrossingE::NONE )
        {
            const bool         ferry = xc.m_type == CrossingE::FERRY;
            const double       before = xc.m_board - sj, after = sj - xc.m_land;
            const BiomeDef    &dest = biomeDef( track.stageBiome( static_cast<int>( xc.m_land / StageLength ) ) );
            const CountryInfo &nation = countryInfo( here.m_country );
            const float        driver = nation.m_left ? -1.0f : 1.0f;
            for( double at0 : { 600.0, 260.0 } )
                if( before > at0 - SegmentLength && before <= at0 )
                {
                    MeshBuilder sign;
                    buildPlaceSign( sign, std::string( ferry ? "FERRY TO " : "CAR TRAIN TO " ) + ( dest.m_name ? dest.m_name : "" ), 3, 0.5f, 2.2f );
                    const float l = driver * ( A.m_edge + 3.0f );
                    props.append( sign, at( A, l, 0.0f ), A.m_heading, 1.0f );
                    c.m_colliders.push_back( { sj, l, 0.6f } );
                }
            const int canopy = landmarkByName( "TERMINAL_CANOPY" );
            if( canopy >= 0 && before > 180.0 - SegmentLength && before <= 180.0 )
                props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( canopy ) ), at( A, 0.0f, 0.0f ), A.m_heading, 1.0f );
            if( ferry && before > 0.0 && before <= SegmentLength )
                ground.quad( at( B, -1500.0f, -6.0f ), at( B, 1500.0f, -6.0f ), at( B, 1500.0f, 0.0f ), at( B, -1500.0f, 0.0f ), Color{ 150, 150, 146, 255 } );
            if( ferry && after >= 0.0 && after < SegmentLength )
                ground.quad( at( A, -1500.0f, -6.0f ), at( A, 1500.0f, -6.0f ), at( A, 1500.0f, 0.0f ), at( A, -1500.0f, 0.0f ), Color{ 150, 150, 146, 255 } );
        }

        // The country's road customs: its speed limit after the place sign and before a speed camera, the camera, a
        // toll plaza across the road.
        {
            const long         stageSegs = static_cast<long>( StageLength / SegmentLength ), local = j % stageSegs;
            const int          stage = static_cast<int>( j / stageSegs );
            const BiomeDef    &place = biomeDef( track.stageBiome( stage ) );
            const RoadCustoms &roads = roadCustoms( place.m_country );
            const float        driver = countryInfo( place.m_country ).m_left ? -1.0f : 1.0f;
            double             cs = 0.0, ts = 0.0;
            int                limit = 0;
            bool               mph = false;
            const bool         camera = local < stageSegs / 2 && stageCamera( track, stage, cs, limit, mph );
            const bool         sign = ( local == 40 && place.m_country && !place.m_underwater && !tunnel && !bridge && xc.m_type == CrossingE::NONE ) ||
                                     ( camera && j == static_cast<long>( ( cs - LimitAt ) / SegmentLength ) );
            if( sign )
            {
                MeshBuilder m;
                buildSpeedSign( m, camera && j == static_cast<long>( ( cs - LimitAt ) / SegmentLength ) ? limit : speedLimit( seg, roads.m_mph ), roads.m_board );
                const float l = driver * ( A.m_edge + 1.6f );
                props.append( m, at( A, l, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { sj, l, 0.3f } );
            }
            if( camera && j == static_cast<long>( cs / SegmentLength ) )
            {
                const float l = driver * ( A.m_edge + 1.8f );
                props.append( models.m_camera, at( A, l, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { sj, l, 0.4f } );
            }
            if( local < stageSegs / 2 && stageToll( track, stage, ts ) && j == static_cast<long>( ts / SegmentLength ) )
            {
                // A canopy over the road, booths on islands between the lanes, their barriers up.
                const Color white = { 236, 236, 230, 255 }, stripe = { 30, 90, 170, 255 }, concrete = { 176, 174, 166, 255 };
                const Vector3 mid = at( A, 0.0f, 0.0f );
                obox( props, Vector3{ mid.x, mid.y + 5.8f, mid.z }, A.m_right, A.m_forward, A.m_hw + 3.0f, 0.3f, 6.0f, white );
                obox( props, Vector3{ mid.x, mid.y + 6.15f, mid.z }, A.m_right, A.m_forward, A.m_hw + 3.05f, 0.06f, 6.05f, stripe );
                for( int k = 0; k <= lanes; ++k )
                {
                    const float   x = -A.m_hw + 2.0f * A.m_hw * static_cast<float>( k ) / static_cast<float>( lanes );
                    const Vector3 p = at( A, x, 0.0f );
                    obox( props, Vector3{ p.x, p.y + 0.12f, p.z }, A.m_right, A.m_forward, 0.45f, 0.12f, 4.5f, concrete );
                    obox( props, Vector3{ p.x, p.y + 1.35f, p.z }, A.m_right, A.m_forward, 0.36f, 1.15f, 1.1f, white );
                    obox( props, Vector3{ p.x, p.y + 3.6f, p.z + 0.0f }, A.m_right, A.m_forward, 0.1f, 2.2f, 0.1f, Color{ 150, 150, 156, 255 } );
                    if( k < lanes ) props.tube( at( A, x + 0.4f, 1.1f ), at( A, x + 0.7f, 3.4f ), 0.05f, 4, Color{ 220, 40, 40, 255 } );
                    c.m_colliders.push_back( { sj, x, 0.5f } );
                }
                MeshBuilder board;
                buildPlaceSign( board, "TOLL", 3, 0.7f, 4.2f, false );
                props.append( board, Vector3{ mid.x - A.m_forward.x * 6.1f, mid.y, mid.z - A.m_forward.z * 6.1f }, A.m_heading, 1.0f );
            }
        }

        // A festival: strings of flags, lanterns or lights across the road, little lamps along it, stalls, tents and
        // Christmas trees beside it.
        const int fest = tunnel || bridge || fork.m_exists || xc.m_type != CrossingE::NONE
                             ? -1
                             : festivalAt( static_cast<BiomeE>( seg.m_blend < 0.5f ? seg.m_biome : seg.m_biomeTo ), m_seasons.dateAt( sj ) );
        if( fest >= 0 )
        {
            const Festival           &f = festivals()[static_cast<size_t>( fest )];
            const std::vector<Color> &string = !f.m_lanterns.empty() ? f.m_lanterns : !f.m_bunting.empty() ? f.m_bunting : f.m_lights;
            if( !string.empty() && j % 14 == 0 )
            {
                const float w = A.m_edge + 1.5f;
                for( float side : { -1.0f, 1.0f } ) props.cylinder( at( A, side * w, 0.0f ), 0.09f, 0.07f, 7.0f, 5, Color{ 92, 92, 98, 255 } );
                const int n = 26;
                Vector3   last = at( A, -w, 6.8f );
                for( int k = 1; k <= n; ++k )
                {
                    const float   x = -w + 2.0f * w * static_cast<float>( k ) / static_cast<float>( n ), t = x / w;
                    const Vector3 p = at( A, x, 6.8f - 1.5f * ( 1.0f - t * t ) );
                    const Color   col = string[static_cast<size_t>( k ) % string.size()];
                    props.tube( last, p, 0.025f, 3, Color{ 40, 40, 44, 255 } );
                    if( !f.m_lanterns.empty() )
                        props.blob( Vector3{ p.x, p.y - 0.5f, p.z }, Vector3{ 0.28f, 0.36f, 0.28f }, col, static_cast<unsigned>( k ), 0.0f, Mat{ 0.8f, 0.0f } );
                    else if( !f.m_bunting.empty() )
                        props.tri( last, p, Vector3{ ( p.x + last.x ) * 0.5f, ( p.y + last.y ) * 0.5f - 0.6f, ( p.z + last.z ) * 0.5f }, col );
                    else
                        props.blob( Vector3{ p.x, p.y - 0.12f, p.z }, Vector3{ 0.08f, 0.08f, 0.08f }, col, static_cast<unsigned>( k ), 0.0f, Mat{ 1.0f, 0.0f } );
                    last = p;
                }
            }
            if( !f.m_lamps.empty() && j % 2 == 0 )
                for( float side : { -1.0f, 1.0f } )
                    props.blob( at( A, side * ( A.m_edge + 0.4f ), 0.12f ), Vector3{ 0.13f, 0.1f, 0.13f }, f.m_lamps[0], static_cast<unsigned>( j ), 0.0f,
                                Mat{ 1.0f, 0.0f } );
            // Beside the road, facing it, on alternate sides.
            auto beside = [&]( const char *shape, const std::vector<Color> &paint, long every, float away, float radius )
            {
                const int v = landmarkByName( shape );
                if( v < 0 || j % every != 0 ) return;
                const float side = ( j / every ) % 2 ? 1.0f : -1.0f, l = side * ( A.m_edge + away );
                const Color keys[3] = { paint.size() > 0 ? paint[0] : Color{ 176, 32, 32, 255 }, paint.size() > 1 ? paint[1] : Color{ 242, 238, 228, 255 },
                                        !f.m_lights.empty() ? f.m_lights[0] : Color{ 255, 224, 160, 255 } };
                const float h = terrainHeight( track, j, l, A.m_terrain, A.m_edge );
                SeasonLook  snow = A.m_season;
                snow.m_autumn = snow.m_bare = snow.m_spring = 0.0f;
                props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( v ) ), at( A, l, h ), A.m_heading + ( side > 0.0f ? 0.0f : Tau * 0.5f ), 1.0f,
                              1.0f, keys, &snow );
                c.m_colliders.push_back( { sj, l, radius } );
            };
            if( !f.m_stalls.empty() ) beside( "MARKET_STALL", f.m_stalls, 7, 5.0f, 2.0f );
            if( !f.m_tents.empty() ) beside( "BEER_TENT", f.m_tents, 31, 16.0f, 8.0f );
            if( f.m_trees ) beside( "CHRISTMAS_TREE", {}, 23, 9.0f, 1.5f );
        }

        // The fork: a barrier between the carriageways, a cushion at its nose, the gantry with both places; after it
        // the other carriageway turns away into the fields.
        if( fork.m_exists )
        {
            const Color concrete = { 196, 194, 186, 255 };
            if( seg.m_flags & SEG_MEDIAN )
            {
                const Vector3 a0 = at( A, 0.0f, 0.0f ), b0 = at( B, 0.0f, 0.0f );
                const Vector3 mid = Vector3Lerp( a0, b0, 0.5f );
                obox( props, Vector3{ mid.x, mid.y + 0.42f, mid.z }, A.m_right, A.m_forward, 0.28f, 0.42f, SegmentLength * 0.52f, concrete );
                if( u < SegmentLength )
                {
                    const Color yellow = { 250, 200, 30, 255 }, black = { 24, 24, 26, 255 };
                    const Vector3 nose = Vector3Add( a0, Vector3Scale( A.m_forward, 1.6f ) );
                    obox( props, Vector3{ nose.x, nose.y + 0.6f, nose.z }, A.m_right, A.m_forward, 0.7f, 0.6f, 1.6f, yellow );
                    for( int k = 0; k < 3; ++k )
                        obox( props, Vector3{ a0.x - A.m_forward.x * 0.02f, a0.y + 0.3f + 0.4f * k, a0.z - A.m_forward.z * 0.02f }, A.m_right, A.m_forward, 0.72f, 0.08f, 0.05f, black );
                }
            }
            if( ( u >= -285.0 && u < -285.0 + SegmentLength ) || ( u >= -780.0 && u < -780.0 + SegmentLength ) )
            {
                std::string left, right;
                forkNames( fork.m_left, fork.m_right, track.stageBiome( seg.m_stage ), left, right );
                MeshBuilder sign;
                buildForkSign( sign, left, right, A.m_hw );
                props.append( sign, at( A, 0.0f, 0.0f ), A.m_heading, 1.0f );
            }
            if( peeling )
            {
                // The other carriageway, a little above the land, its kerbs along it.
                const double  ua = u, ub = u + SegmentLength;
                const float   xa = branchX( ua ), xb = branchX( ub );
                const Color   kerb = j % 2 == 0 ? sf.m_rumbleA : sf.m_rumbleB;
                const float   lift = 0.04f;
                auto          strip = [&]( float a0, float a1, float b0, float b1, Color col )
                { ground.quad( at( A, a0, lift ), at( A, a1, lift ), at( B, b1, lift ), at( B, b0, lift ), col ); };
                strip( xa - carriage, xa + carriage, xb - carriage, xb + carriage, band ? sf.m_road : sf.m_roadAlt );
                strip( xa - carriage - RumbleWidth, xa - carriage, xb - carriage - RumbleWidth, xb - carriage, kerb );
                strip( xa + carriage, xa + carriage + RumbleWidth, xb + carriage, xb + carriage + RumbleWidth, kerb );
                if( band ) strip( xa - 0.08f, xa + 0.08f, xb - 0.08f, xb + 0.08f, sf.m_line );
            }
        }

        // Furniture and props, per side.
        for( float side : { -1.0f, 1.0f } )
        {
            const uint64_t  sideKey = side < 0 ? 0x1111ull : 0x2222ull;
            const BiomeDef &rule = ruling( seg, hashFloat( seed ^ sideKey, static_cast<uint64_t>( j ) ) );
            const bool      railed = seg.m_flags & ( side < 0 ? SEG_RAIL_L : SEG_RAIL_R );
            const CountryInfo &signs = countryInfo( rule.m_country );

            // The place's name at the start of its stage, a border post where the country changes.
            const long stageSegs = static_cast<long>( StageLength / SegmentLength ), stage = j / stageSegs, local = j % stageSegs;
            if( local >= 3 && local <= 7 && !tunnel && !bridge )
            {
                const BiomeDef    &place = biomeDef( track.stageBiome( static_cast<int>( stage ) ) );
                const CountryInfo &nation = countryInfo( place.m_country );
                const float        driver = nation.m_left ? -1.0f : 1.0f;
                const BiomeDef    *last = stage > 0 ? &biomeDef( track.stageBiome( static_cast<int>( stage - 1 ) ) ) : nullptr;
                const char        *before = last ? last->m_country : nullptr;
                const bool border = place.m_country && before && std::strcmp( place.m_country, before ) != 0;
                // A state or province line inside a country: its name on a board.
                const bool regionBorder = !border && place.m_region && last &&
                                          ( !last->m_region || std::strcmp( place.m_region, last->m_region ) != 0 );
                if( regionBorder && local == 5 && side == driver )
                {
                    MeshBuilder sign;
                    buildPlaceSign( sign, place.m_region, 4, 0.5f, 2.0f );
                    props.append( sign, at( A, side * ( A.m_edge + 3.0f ), 0.0f ), A.m_heading, 1.0f );
                    c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, side * ( A.m_edge + 3.0f ), 0.6f } );
                }
                if( local == 7 && side == driver && place.m_name )
                {
                    MeshBuilder sign;
                    buildPlaceSign( sign, place.m_name, nation.m_townSign );
                    const float l = side * ( A.m_edge + 2.4f );
                    props.append( sign, at( A, l, 0.0f ), A.m_heading, 1.0f );
                    c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.6f } );
                }
                if( border && local == 3 && side == driver )
                {
                    const float l = side * ( A.m_edge + 2.0f );
                    props.append( models.m_borderPost, at( A, l, 0.0f ), A.m_heading + ( side < 0 ? Tau * 0.5f : 0.0f ), 1.0f );
                    c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 1.4f } );
                }
                if( border && local == 5 )
                {
                    // The new country's flag and name on the driver's side, the old country's flag across the road.
                    const int   flag = countryIndex( side == driver ? place.m_country : before );
                    const float l = side * ( A.m_edge + 6.0f );
                    if( flag >= 0 ) props.append( models.m_flags[static_cast<size_t>( flag )], at( A, l, 0.0f ), A.m_heading, 1.0f );
                    if( side == driver )
                    {
                        MeshBuilder sign;
                        buildPlaceSign( sign, place.m_country, 3, 0.6f, 2.4f );
                        props.append( sign, at( A, side * ( A.m_edge + 3.0f ), 0.0f ), A.m_heading, 1.0f );
                        c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, side * ( A.m_edge + 3.0f ), 0.6f } );
                    }
                }
            }

            // The hairpins of a famous pass are numbered on the outside, counting down to the top, where its name and
            // height stand.
            if( seg.m_hairpin > 0 && side * track.segment( j + 4 ).m_curve < 0.0f )
            {
                MeshBuilder sign;
                buildPlaceSign( sign, ( rule.m_hairpinWord ? std::string( rule.m_hairpinWord ) + " " : std::string() ) + std::to_string( seg.m_hairpin ), 0,
                                0.42f, 1.2f );
                const float l = side * ( A.m_edge + 1.8f );
                props.append( sign, at( A, l, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.5f } );
            }
            if( ( seg.m_flags & SEG_SUMMIT ) && rule.m_summit && side == ( signs.m_left ? -1.0f : 1.0f ) )
            {
                MeshBuilder sign;
                buildPlaceSign( sign, std::string( rule.m_summit ) + "  " + std::to_string( rule.m_summitHeight ) + " M", signs.m_townSign, 0.5f, 1.8f );
                const float l = side * ( A.m_edge + 2.6f );
                props.append( sign, at( A, l, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.6f } );
            }

            const bool forkClear = ( seg.m_flags & SEG_FORK ) && ( u < 250.0 || u > ForkMedian - 150.0 );   // the gore and the peel
            if( rule.m_postSpacing > 0.0f && !railed && !tunnel && !bridge && !forkClear )
            {
                const long k = std::max( 2L, static_cast<long>( rule.m_postSpacing / SegmentLength + 0.5f ) );
                if( ( j + ( side < 0 ? k / 2 : 0 ) ) % k == 0 )
                {
                    // The reflectors differ between the traffic's own side and the other.
                    const bool   keepLeft = rule.m_country && countryInfo( rule.m_country ).m_left;
                    const size_t far = ( side < 0 ) == keepLeft ? 0 : 1;
                    props.append( models.m_post[std::min<size_t>( rule.m_postStyle, 3 ) * 2 + far], at( A, side * ( A.m_hw + RumbleWidth + 0.7f ), 0.0f ),
                                  A.m_heading, 1.0f );
                }
            }
            if( rule.m_lampSpacing > 0.0f && !tunnel && !bridge && !forkClear )
            {
                const long k = std::max( 2L, static_cast<long>( rule.m_lampSpacing / SegmentLength + 0.5f ) );
                if( j % k == ( side > 0 ? 0 : k / 2 ) )
                {
                    const float l = side * ( A.m_hw + RumbleWidth + 1.0f );
                    props.append( models.m_lamp[std::min<size_t>( rule.m_lampTone, 2 )], at( A, l, 0.0f ), A.m_heading + ( side > 0 ? -0.5f : 0.5f ) * Tau * 0.5f, 1.0f );
                    c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.35f } );
                }
            }
            // The poles of the overhead lines beside the country road (data/roadside.txt), every 60 m on the right.
            if( rule.m_poleKind > 0 && side > 0 && !tunnel && !bridge && !forkClear && j % std::max( 2L, static_cast<long>( 60.0f / SegmentLength + 0.5f ) ) == 0 )
            {
                const float l = A.m_hw + RumbleWidth + 5.5f;
                props.append( models.m_utilityPole[std::min<size_t>( rule.m_poleKind - 1, 2 )], at( A, l, terrainHeight( track, j, l, A.m_terrain, A.m_edge ) ),
                              A.m_heading, 1.0f );
            }
            const bool chevron = seg.m_flags & ( side < 0 ? SEG_CHEVRON_L : SEG_CHEVRON_R );
            if( chevron && j % 2 == 0 && !tunnel && !bridge )
            {
                const float l = side * ( A.m_edge + 1.4f );
                const size_t style = std::min<size_t>( signs.m_chevrons, 2 );
                props.append( side < 0 ? models.m_chevronRight[style] : models.m_chevronLeft[style], at( A, l, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.5f } );
            }
            if( ( seg.m_flags & SEG_JUMP ) && !tunnel )
                props.append( models.m_jumpSign[std::min<size_t>( signs.m_warn, 3 )], at( A, side * ( A.m_edge + 1.2f ), 0.0f ), A.m_heading, 1.0f );

            // Scattered props; the buildings of a town or a city on plots in rows back from the road, in the country in a
            // village here and there.
            auto place = [&]( const PropRule &pr, uint64_t key, float dist, bool onPlot )
            {
                const float h2 = hashFloat( key + 2, static_cast<uint64_t>( j ) );
                const float h3 = hashFloat( key + 3, static_cast<uint64_t>( j ) );
                const float h4 = hashFloat( key + 4, static_cast<uint64_t>( j ) );
                const float waterSide = side < 0 ? A.m_terrain.m_waterLeft : A.m_terrain.m_waterRight;
                if( pr.m_onWater )
                {
                    if( waterSide < 0.5f || dist < 35.0f ) return;
                }
                else if( waterSide > 0.5f && dist > 12.0f )
                {
                    return;
                }
                if( railed && dist < 2.0f ) return;
                if( ( tunnel && dist < 7.0f ) || ( bridge && dist < 4.0f ) ) return;
                const float  lateral = side * ( A.m_edge + dist );
                if( viaduct && std::abs( lateral - railL ) < 7.0f ) return;
                const float reach = side < 0.0f ? seg.m_reachL : seg.m_reachR;
                if( reach > 0.0f && std::abs( lateral ) > reach - 3.0f ) return;   // the next leg's land
                const double ps = static_cast<double>( j ) * SegmentLength + h2 * SegmentLength;
                // Nothing where the other carriageway of a fork turns away (it is decided before this is baked).
                if( fork.m_exists && u > ForkMedian - 60.0 && u < ForkMedian + ForkPeel + 40.0 )
                {
                    if( fork.m_chosen == 0 || side * static_cast<float>( -fork.m_chosen ) > 0.0f )
                    {
                        const float bx = branchX( std::clamp( u, ForkMedian, ForkMedian + ForkPeel ) );
                        if( std::abs( lateral - bx ) < carriage + 14.0f || std::abs( lateral ) < std::abs( bx ) + 14.0f ) return;
                    }
                }
                const float  ht = pr.m_onWater ? A.m_terrain.m_seaLevel + A.m_seg->m_trench : terrainHeight( track, j, lateral, A.m_terrain, A.m_edge );
                // Place along the chord inside the segment.
                Vector3 pos = at( A, lateral, ht );
                pos = Vector3Add( pos, Vector3Scale( A.m_forward, h2 * SegmentLength ) );
                float yaw = A.m_heading + h3 * Tau;
                switch( pr.m_kind )
                {
                case PropKindE::FENCE:
                case PropKindE::STONE_WALL:
                case PropKindE::TORII: yaw = A.m_heading; break;
                case PropKindE::PUMPKIN:
                case PropKindE::GRAVESTONE: yaw = A.m_heading + ( h3 - 0.5f ) * 0.8f; break;
                case PropKindE::BILLBOARD: yaw = A.m_heading - side * 0.35f; break;
                case PropKindE::MAILBOX: yaw = A.m_heading + side * Tau * 0.25f; break;
                case PropKindE::CRANE: yaw = side < 0 ? A.m_heading : A.m_heading + Tau * 0.5f; break;
                case PropKindE::NEON_TOWER:
                case PropKindE::CABIN:
                case PropKindE::BARN:
                case PropKindE::SNOW_HOUSE:
                case PropKindE::HUT:
                case PropKindE::HOUSE: yaw = A.m_heading + side * Tau * 0.25f + ( h3 - 0.5f ) * 0.4f; break;
                case PropKindE::WINDMILL: yaw = A.m_heading + Tau * 0.5f + ( h3 - 0.5f ) * 0.8f; break;
                default:
                    // Fronts (+x) to the road: the road lies to the left of things on the right.
                    if( models.facing( pr.m_kind ) == FacingE::ROAD ) yaw = A.m_heading + ( side < 0 ? Tau * 0.5f : 0.0f ) + ( h3 - 0.5f ) * 0.3f;
                    else if( models.facing( pr.m_kind ) == FacingE::ALONG ) yaw = A.m_heading;
                    else if( onPlot ) yaw = A.m_heading + Tau * 0.25f * std::floor( h3 * 4.0f ) + ( h3 - 0.5f ) * 0.1f;
                    break;
                }
                const float scale = lerpf( pr.m_scaleMin, pr.m_scaleMax, h4 );
                const float bright = 0.88f + 0.24f * hashFloat( key + 5, static_cast<uint64_t>( j ) );
                const Color *defaults = models.keys( pr.m_kind );
                Color        keys[3] = {};
                if( defaults )
                {
                    keys[0] = pr.m_a.a ? pr.m_a : defaults[0];
                    keys[1] = pr.m_b.a ? pr.m_b : defaults[1];
                    keys[2] = pr.m_c.a ? pr.m_c : defaults[2];
                    if( pr.m_pastel ) keys[0] = Pastels[hash64( key + 7 + static_cast<uint64_t>( j ) ) % std::size( Pastels )];
                }
                uint64_t variant = pr.m_variant >= 0 ? static_cast<uint64_t>( pr.m_variant ) : hash64( key + 6 + static_cast<uint64_t>( j ) );
                // Warning signs in the country's shape: a yellow diamond, a white or a yellow triangle bordered red, a yellow
                // one bordered black.
                static const uint64_t warnShape[] = { 5, 10, 0, 15 };
                if( pr.m_kind == PropKindE::WARNING_SIGN ) variant = variant % 5 + warnShape[std::min<size_t>( signs.m_warn, 3 )];
                // Deciduous trees, bushes, hedges and vines turn, lose their leaves and some blossom; the rest only takes the
                // snow.
                using K = PropKindE;
                const K    kind = pr.m_kind;
                const bool turns = kind == K::OAK || kind == K::BUSH || kind == K::BIRCH || kind == K::POPLAR || kind == K::BROADLEAF ||
                                   kind == K::SHRUB || kind == K::HEDGE || kind == K::VINE_ROW || kind == K::FERN || kind == K::FLOWER_BUSH;
                const bool blooms = kind == K::BROADLEAF || kind == K::BUSH || kind == K::SHRUB;
                SeasonLook look = A.m_season;
                if( !turns ) look.m_autumn = look.m_bare = look.m_spring = 0.0f;
                const float h = hashFloat( key + 13, static_cast<uint64_t>( j ) );
                // Up close in full; from afar (and deep beside the road) lighter; landmarks always in full.
                const float        pick = blooms ? h : 0.3f + 0.7f * h;
                const MeshBuilder &full = models.prop( pr.m_kind, variant );
                const MeshBuilder &far = pr.m_perStage > 0 ? full : models.propFar( pr.m_kind, variant );
                const MeshBuilder &near = dist > LodLateral ? far : full;
                if( near.triangleCount() ) scenery.append( near, pos, yaw, scale, bright, defaults ? keys : nullptr, &look, pick );
                if( far.triangleCount() ) sceneryFar.append( far, pos, yaw, scale, bright, defaults ? keys : nullptr, &look, pick );
                const float radius = models.propRadius( pr.m_kind );
                if( radius > 0.0f && dist < 45.0f && !pr.m_soft ) c.m_colliders.push_back( { ps, lateral, radius * scale } );
            };
            const Plots plots = plotsOf( rule, models );
            const bool  built = plots.m_count > 0 && ( plots.m_city || inVillage( seed, j ) );
            for( size_t r = 0; r < rule.m_props.size(); ++r )
            {
                const PropRule &pr = rule.m_props[r];
                if( built && plots.has( r ) ) continue;
                const uint64_t key = seed ^ ( sideKey * 31 + r * 7919 );
                const float    h0 = hashFloat( key, static_cast<uint64_t>( j ) );
                if( pr.m_perStage > 0 )
                { // A landmark stands at a few fixed spots per stage, not by chance.
                    bool spot = false;
                    for( int k = 0; k < pr.m_perStage && !spot; ++k )
                    {
                        bool       right = false;
                        const long where = landmarkSpot( seed, r, k, stage, right );
                        // On the dry side, or on the water for a monument that stands in it.
                        const bool wetL = A.m_terrain.m_waterLeft > 0.5f, wetR = A.m_terrain.m_waterRight > 0.5f;
                        if( ( right ? wetR : wetL ) != pr.m_onWater && ( right ? wetL : wetR ) == pr.m_onWater ) right = !right;
                        spot = !tunnel && local == where && right == ( side > 0 );
                    }
                    if( !spot ) continue;
                }
                else if( h0 >= pr.m_chance )
                {
                    continue;
                }
                place( pr, key, lerpf( pr.m_nearest, pr.m_farthest, hashFloat( key + 1, static_cast<uint64_t>( j ) ) ), false );
            }
            // One plot per few segments and row; a city leaves every fourth free for a cross street, a village thins out
            // towards its back.
            const long plotSegs = std::max( 2L, std::lround( plots.m_along / SegmentLength ) ), plot = j / plotSegs;
            const bool wet = ( side < 0.0f ? A.m_terrain.m_waterLeft : A.m_terrain.m_waterRight ) > 0.5f;   // no plots in the sea
            if( built && !wet && j % plotSegs == plotSegs / 2 && !( plots.m_city && plot % 4 == 0 ) )
                for( int row = 0; row < plots.m_rows; ++row )
                {
                    const uint64_t key = seed ^ ( sideKey * 37 + static_cast<uint64_t>( row ) * 104729 + 0x9107ull );
                    const float    fill = plots.m_city ? 0.94f : 0.8f - 0.18f * static_cast<float>( row );
                    if( hashFloat( key, static_cast<uint64_t>( plot ) ) >= fill ) continue;
                    const float pick = hashFloat( key + 14, static_cast<uint64_t>( plot ) ) * plots.m_upTo[static_cast<size_t>( plots.m_count - 1 )];
                    int         i = 0;
                    while( i + 1 < plots.m_count && pick >= plots.m_upTo[static_cast<size_t>( i )] ) ++i;
                    const float dist = plots.m_nearest + plots.m_half + plots.m_across * static_cast<float>( row ) +
                                       ( hashFloat( key + 15, static_cast<uint64_t>( plot ) ) - 0.5f ) * 3.0f;
                    place( rule.m_props[plots.m_rule[static_cast<size_t>( i )]], key, dist, true );
                }
        }

        // Overhead sign gantries.
        if( here.m_gantrySpacing > 0.0f && !tunnel )
        {
            const long k = std::max( 4L, static_cast<long>( here.m_gantrySpacing / SegmentLength + 0.5f ) );
            if( j % k == k / 3 && !( seg.m_flags & ( SEG_CHECKPOINT | SEG_START ) ) )
            {
                MeshBuilder g;
                const float span = A.m_hw + RumbleWidth + 0.3f;
                const Color steel = { 150, 154, 162, 255 };
                const Color white = { 240, 240, 240, 255 };
                const Mat   paint = { 0.35f, 0.0f };
                for( float x : { -span, span } ) g.box( Vector3{ x, 3.8f, 0 }, Vector3{ 0.25f, 3.8f, 0.25f }, steel );
                g.box( Vector3{ 0, 7.4f, 0 }, Vector3{ span + 0.3f, 0.3f, 0.3f }, steel );
                for( float sx : { -0.5f, 0.5f } )
                {
                    const float cx = sx * A.m_hw, hw = A.m_hw * 0.42f;
                    g.box( Vector3{ cx, 8.7f, -0.1f }, Vector3{ hw, 1.05f, 0.08f }, Color{ 20, 110, 64, 255 }, Mat{ 0.08f, 0.0f } );
                    g.box( Vector3{ cx, 9.65f, -0.19f }, Vector3{ hw - 0.1f, 0.04f, 0.01f }, white, paint );
                    g.box( Vector3{ cx, 7.75f, -0.19f }, Vector3{ hw - 0.1f, 0.04f, 0.01f }, white, paint );
                    for( int line = 0; line < 2; ++line )
                    {
                        const uint64_t key = static_cast<uint64_t>( j * 4 + line + ( sx > 0 ? 2 : 0 ) );
                        const float    len = hw * ( 0.5f + 0.3f * hashFloat( seed + 55, key ) );
                        g.box( Vector3{ cx - hw + 0.4f + len * 0.5f, 9.15f - 0.55f * static_cast<float>( line ), -0.19f },
                               Vector3{ len * 0.5f, 0.14f, 0.01f }, white, paint );
                    }
                    g.tri( Vector3{ cx + hw - 1.0f, 8.0f, -0.19f }, Vector3{ cx + hw - 0.4f, 8.0f, -0.19f },
                           Vector3{ cx + hw - 0.7f, 8.5f, -0.19f }, white, paint );
                }
                props.appendBasis( g, Vector3Negate( A.m_right ), Vector3{ 0, 1, 0 }, A.m_forward, at( A, 0.0f, 0.0f ) );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, -span, 0.5f } );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, span, 0.5f } );
            }
        }

        // Checkpoint gates and the start gantry.
        if( ( seg.m_flags & ( SEG_CHECKPOINT | SEG_START | SEG_FINISH ) ) && !tunnel )
        {
            const bool  start = seg.m_flags & ( SEG_START | SEG_FINISH );
            MeshBuilder gate;
            const float span = A.m_edge + 1.0f;
            const Color pillar = start ? Color{ 60, 62, 70, 255 } : Color{ 230, 232, 240, 255 };
            for( float x : { -span, span } ) gate.box( Vector3{ x, 4.6f, 0 }, Vector3{ 0.6f, 4.6f, 0.6f }, pillar );
            gate.box( Vector3{ 0, 9.0f, 0 }, Vector3{ span + 0.6f, 0.9f, 0.45f }, Color{ 30, 30, 34, 255 } );
            const int   squares = static_cast<int>( ( span + 0.6f ) * 2.0f / 0.9f );
            const float w = ( span + 0.6f ) * 2.0f / static_cast<float>( squares );
            for( int row = 0; row < 2; ++row )
            {
                for( int q = 0; q < squares; ++q )
                {
                    if( ( q + row ) % 2 ) continue;
                    const float x0 = -span - 0.6f + w * static_cast<float>( q );
                    const float y0 = 8.1f + 0.9f * static_cast<float>( row );
                    gate.quad( Vector3{ x0, y0, -0.47f }, Vector3{ x0 + w, y0, -0.47f }, Vector3{ x0 + w, y0 + 0.9f, -0.47f },
                               Vector3{ x0, y0 + 0.9f, -0.47f }, Color{ 245, 245, 245, 255 }, Mat{ 0.55f, 0.0f } );
                }
            }
            const Color strip = start ? Color{ 255, 40, 40, 255 } : Color{ 60, 255, 140, 255 };
            gate.box( Vector3{ 0, 7.95f, -0.2f }, Vector3{ span, 0.08f, 0.08f }, strip, Mat{ 1.0f, 0.0f } );
            if( start )
            {
                for( int q = -2; q <= 2; ++q )
                    gate.box( Vector3{ static_cast<float>( q ) * 1.2f, 7.4f, -0.3f }, Vector3{ 0.35f, 0.35f, 0.1f },
                              Color{ 255, 30, 30, 255 }, Mat{ 1.0f, 0.0f } );
            }
            props.appendBasis( gate, Vector3Negate( A.m_right ), Vector3{ 0, 1, 0 }, A.m_forward, at( A, 0.0f, 0.0f ) );
            c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, -span, 0.8f } );
            c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, span, 0.8f } );
            // A chequered line across the road.
            MeshBuilder line;
            const int   n = static_cast<int>( A.m_hw * 2.0f / 0.8f );
            const float sq = A.m_hw * 2.0f / static_cast<float>( n );
            for( int row = 0; row < 2; ++row )
                for( int q = 0; q < n; ++q )
                {
                    const Color col = ( q + row ) % 2 ? Color{ 20, 20, 24, 255 } : Color{ 240, 240, 240, 255 };
                    const float x0 = -A.m_hw + sq * static_cast<float>( q );
                    const float z0 = sq * static_cast<float>( row );
                    line.quad( Vector3{ x0, 0.03f, z0 }, Vector3{ x0 + sq, 0.03f, z0 }, Vector3{ x0 + sq, 0.03f, z0 + sq },
                               Vector3{ x0, 0.03f, z0 + sq }, col, Mat{ 0.0f, 0.2f } );
                }
            const Vector3 tiltedLeft = Vector3Normalize( Vector3{ -A.m_right.x, A.m_tanBank, -A.m_right.z } );
            ground.appendBasis( line, tiltedLeft, Vector3{ 0, 1, 0 }, A.m_forward, at( A, 0.0f, 0.0f ) );
        }

        // Puddles: dark mirrors on the road.
        if( seg.m_flags & SEG_PUDDLE )
        {
            MeshBuilder pud;
            const Color dark = { 26, 30, 40, 255 };
            for( int q = 0; q < 10; ++q )
            {
                const float a0 = Tau * static_cast<float>( q ) / 10.0f, a1 = Tau * static_cast<float>( q + 1 ) / 10.0f;
                pud.tri( Vector3{ 0, 0.025f, 0 }, Vector3{ std::cos( a0 ) * 1.4f, 0.025f, std::sin( a0 ) * 2.3f },
                         Vector3{ std::cos( a1 ) * 1.4f, 0.025f, std::sin( a1 ) * 2.3f }, dark, Mat{ 0.0f, 1.0f } );
            }
            const float   l = puddleLateral( track, j );
            const Vector3 p = Vector3Add( at( A, l, 0.0f ), Vector3Scale( A.m_forward, SegmentLength * 0.5f ) );
            const Vector3 tiltedLeft = Vector3Normalize( Vector3{ -A.m_right.x, A.m_tanBank, -A.m_right.z } );
            ground.appendBasis( pud, tiltedLeft, Vector3{ 0, 1, 0 }, A.m_forward, p );
        }

        if( viaduct )
        {
            // Both windings: the viaduct is seen from below, beside and above.
            auto face = [&]( Vector3 a, Vector3 b, Vector3 cc, Vector3 d, Color col )
            {
                props.quad( a, b, cc, d, col );
                props.quad( d, cc, b, a, col );
            };
            const float  l = railL, w = 2.6f, h = RailHeight;
            const Color  concrete = { 178, 174, 166, 255 }, shade = { 128, 124, 118, 255 }, steel = { 64, 60, 58, 255 }, stone = { 132, 124, 112, 255 };
            const float  groundA = terrainHeight( track, j, l, A.m_terrain, A.m_edge );
            if( sj >= viaFrom && sj < viaTo )
            {
                // The deck, its parapets and the rails.
                face( at( A, l - w, h - 0.15f ), at( A, l + w, h - 0.15f ), at( B, l + w, h - 0.15f ), at( B, l - w, h - 0.15f ), concrete );
                face( at( A, l - w, h - 1.4f ), at( A, l + w, h - 1.4f ), at( B, l + w, h - 1.4f ), at( B, l - w, h - 1.4f ), shade );
                for( float edge : { -w, w } )
                    face( at( A, l + edge, h + 0.55f ), at( B, l + edge, h + 0.55f ), at( B, l + edge, h - 1.4f ), at( A, l + edge, h - 1.4f ), concrete );
                for( float r : { -0.75f, 0.75f } )
                    face( at( A, l + r - 0.07f, h ), at( A, l + r + 0.07f, h ), at( B, l + r + 0.07f, h ), at( B, l + r - 0.07f, h ), steel );
                // A pillar down to the ground every 25 m, a mast for the wire every 50.
                if( j % 5 == 0 && groundA < h - 1.4f )
                {
                    props.box( at( A, l, ( groundA + h - 1.4f ) * 0.5f ), Vector3{ 0.9f, ( h - 1.4f - groundA ) * 0.5f, 0.9f }, shade );
                    c.m_colliders.push_back( { sj, l, 1.1f } );
                }
                if( j % 10 == 5 )
                {
                    props.box( at( A, l + railway.m_side * ( w - 0.3f ), h + 3.0f ), Vector3{ 0.12f, 3.0f, 0.12f }, steel );
                    face( at( A, l - w, h + 5.9f ), at( A, l + w, h + 5.9f ), at( A, l + w, h + 5.75f ), at( A, l - w, h + 5.75f ), steel );
                }
            }
            else
            {
                // A portal: a block of stone the rails run into, its mouth dark.
                const float bw = w + 1.6f, top = h + 5.0f;
                face( at( A, l - bw, top ), at( A, l + bw, top ), at( B, l + bw, top ), at( B, l - bw, top ), stone );
                for( float edge : { -bw, bw } ) face( at( A, l + edge, top ), at( B, l + edge, top ), at( B, l + edge, groundA ), at( A, l + edge, groundA ), stone );
                const bool startMouth = sj < viaFrom && sj + SegmentLength >= viaFrom, endMouth = sj >= viaTo && sj - SegmentLength < viaTo;
                if( startMouth || endMouth )
                {
                    const Cross &M = startMouth ? B : A;
                    const float  push = startMouth ? 0.06f : -0.06f;
                    auto         out = [&]( Vector3 p ) { return Vector3Add( p, Vector3Scale( M.m_forward, push ) ); };
                    face( at( M, l - bw, top ), at( M, l + bw, top ), at( M, l + bw, groundA ), at( M, l - bw, groundA ), stone );
                    face( out( at( M, l - w, h + 4.2f ) ), out( at( M, l + w, h + 4.2f ) ), out( at( M, l + w, h - 1.4f ) ), out( at( M, l - w, h - 1.4f ) ),
                          Color{ 22, 20, 18, 255 } );
                }
            }
        }
    }

    c.m_ground = std::move( ground );
    c.m_props = std::move( props );
    c.m_scenery = std::move( scenery );
    c.m_sceneryFar = std::move( sceneryFar );
    c.m_glass = std::move( glass );
}

} // namespace sunstreak
