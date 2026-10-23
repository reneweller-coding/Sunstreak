/**
 * @file World.cpp
 * @brief Chunk baking: road surface, land, water, furniture and props.
 * @author reneweller
 */
#include "World.h"

#include "Noise.h"

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
 * @brief How deep the valley under a bridge is at a segment (1 under the bridge, rising at its ends).
 * @param[in] track  the road
 * @param[in] index  the segment
 * @return 0..1
 */
float valleyDepth( const Track &track, long index )
{
    float best = 0.0f;
    for( long d = -RampSegments; d <= RampSegments; ++d )
        if( track.segment( index + d ).m_flags & SEG_BRIDGE )
            best = std::max( best, 1.0f - static_cast<float>( std::abs( d ) ) / static_cast<float>( RampSegments + 1 ) );
    return smoothstepf( 0.0f, 1.0f, best );
}

} // namespace

float World::terrainHeight( const Track &track, long index, float lateral, const Terrain &terrain, float edge )
{
    const float    a = std::max( 0.0f, std::abs( lateral ) - edge );
    const bool     left = lateral < 0.0f;
    const double   s = static_cast<double>( index ) * SegmentLength;
    const uint64_t seed = track.seed();
    float          h = -0.35f * smoothstepf( 0.0f, 4.0f, a );
    const float    wall = left ? terrain.m_wallLeft : terrain.m_wallRight;
    const float    n1 = valueNoise( s * 0.012, lateral * 0.02, seed + 11 );
    h += wall * smoothstepf( 8.0f, 70.0f, a ) * ( 0.75f + 0.45f * n1 );
    const float hills = fbm( s * 0.0035, lateral * 0.005, seed + 23 ) * 0.5f + 0.5f;
    h += terrain.m_hills * smoothstepf( 30.0f, 220.0f, a ) * hills * 1.6f;
    const float water = left ? terrain.m_waterLeft : terrain.m_waterRight;
    if( water > 0.0f ) h = lerpf( h, terrain.m_seaLevel - 4.5f, water * smoothstepf( 4.0f, 22.0f, a ) );
    const float cover = tunnelCover( track, index );
    if( cover > 0.0f ) h += TunnelLift * cover * smoothstepf( 0.0f, 6.0f, a ) * ( 1.0f - smoothstepf( 60.0f, 140.0f, a ) );
    const float valley = valleyDepth( track, index );
    if( valley > 0.0f )
    {
        const bool under = track.segment( index ).m_flags & SEG_BRIDGE;
        h -= ValleyDepth * valley * ( under ? 1.0f : smoothstepf( 0.0f, 12.0f, a ) ) * ( 1.0f - smoothstepf( 80.0f, 200.0f, a ) );
    }
    return h;
}

void World::clear()
{
    for( Chunk &c : m_chunks )
    {
        UnloadMesh( c.m_ground );
        UnloadMesh( c.m_props );
    }
    m_chunks.clear();
}

void World::update( Track &track, double s, int maxBuilds )
{
    const long firstWanted = std::max( 0L, static_cast<long>( std::floor( ( s - 40.0 ) / ChunkLength ) ) );
    const long lastWanted = static_cast<long>( std::floor( ( s + ViewDistance ) / ChunkLength ) );
    while( !m_chunks.empty() && m_chunks.front().m_index < firstWanted )
    {
        UnloadMesh( m_chunks.front().m_ground );
        UnloadMesh( m_chunks.front().m_props );
        m_chunks.pop_front();
    }
    long next = m_chunks.empty() ? firstWanted : m_chunks.back().m_index + 1;
    for( int built = 0; next <= lastWanted && built < maxBuilds; ++built, ++next )
    {
        Chunk c;
        c.m_index = next;
        buildChunk( track, c );
        m_chunks.push_back( std::move( c ) );
    }
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

void World::buildChunk( Track &track, Chunk &c )
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
        const BiomeDef &a = biomeDef( static_cast<BiomeE>( seg.m_biome ) );
        const BiomeDef &b = biomeDef( static_cast<BiomeE>( seg.m_biomeTo ) );
        x.m_surface = blendSurface( a.m_surface, b.m_surface, seg.m_blend );
        x.m_terrain = blendTerrain( a.m_terrain, b.m_terrain, seg.m_blend );
        return x;
    };

    MeshBuilder ground;
    MeshBuilder props;

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
        const bool     band = ( j / 2 ) % 2 == 0;
        const int      lanes = seg.m_lanes;

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
            cols.push_back( { f, -0.08f, StripE::DASH } );
            cols.push_back( { f, 0.08f, StripE::ROAD } );
        }
        cols.push_back( { 1.0f, -0.45f, StripE::LINE } );
        cols.push_back( { 1.0f, -0.25f, StripE::ROAD } );
        cols.push_back( { 1.0f, 0.0f, StripE::ROAD } );
        const bool  dashOn = band && !( seg.m_flags & SEG_NO_DASH );
        const Color roadCol = band ? sf.m_road : sf.m_roadAlt;
        for( size_t k = 0; k + 1 < cols.size(); ++k )
        {
            const float la0 = cols[k].m_frac * A.m_hw + cols[k].m_off, la1 = cols[k + 1].m_frac * A.m_hw + cols[k + 1].m_off;
            const float lb0 = cols[k].m_frac * B.m_hw + cols[k].m_off, lb1 = cols[k + 1].m_frac * B.m_hw + cols[k + 1].m_off;
            Color       base = roadCol;
            float       glow = 0.0f;
            float       gloss = 0.18f;
            if( cols[k].m_kind == StripE::LINE || ( cols[k].m_kind == StripE::DASH && dashOn ) )
            {
                base = sf.m_line;
                glow = sf.m_lineGlow * 0.45f;
                gloss = 0.3f;
            }
            const Vector3 p[4] = { at( A, la0, 0 ), at( A, la1, 0 ), at( B, lb1, 0 ), at( B, lb0, 0 ) };
            const float   ls[4] = { lampLight( j, la0, 0 ), lampLight( j, la1, 0 ), lampLight( j, lb1, 1 ), lampLight( j, lb0, 1 ) };
            Color         col[4];
            Mat           mat[4];
            for( int v = 0; v < 4; ++v )
            {
                col[v] = lerpColor( base, WarmLight, ls[v] * 0.5f );
                mat[v] = Mat{ glow + ls[v] * 0.22f, gloss };
            }
            ground.quadV( p, col, mat );
        }

        // Kerbs and shoulders on both sides.
        for( float side : { -1.0f, 1.0f } )
        {
            const Color kerb = j % 2 == 0 ? sf.m_rumbleA : sf.m_rumbleB;
            const float l0a = side * A.m_hw, l1a = side * ( A.m_hw + RumbleWidth ), l2a = side * A.m_edge;
            const float l0b = side * B.m_hw, l1b = side * ( B.m_hw + RumbleWidth ), l2b = side * B.m_edge;
            {
                const Vector3 p[4] = { at( A, l0a, 0.02f ), at( A, l1a, 0.02f ), at( B, l1b, 0.02f ), at( B, l0b, 0.02f ) };
                const Color   col[4] = { kerb, kerb, kerb, kerb };
                const Mat     m = { sf.m_lineGlow * 0.35f, 0.2f };
                const Mat     mat[4] = { m, m, m, m };
                ground.quadV( p, col, mat );
            }
            if( bridge )
            {
                const Color concrete = { 150, 150, 154, 255 };
                ground.quad( at( A, l1a, 0.0f ), at( B, l1b, 0.0f ), at( B, l1b, -1.6f ), at( A, l1a, -1.6f ), concrete );
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
            for( int k = 0; k + 1 < offsetCount; ++k )
            {
                const float la0 = side * ( A.m_edge + offsets[k] ), la1 = side * ( A.m_edge + offsets[k + 1] );
                const float lb0 = side * ( B.m_edge + offsets[k] ), lb1 = side * ( B.m_edge + offsets[k + 1] );
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
            const float y = A.m_terrain.m_seaLevel - ( 1.0f - w ) * 20.0f;
            ground.quad( at( A, la0, y ), at( A, la1, y ), at( B, lb1, y ), at( B, lb0, y ), sf.m_water,
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
            const float   da = A.m_hw + RumbleWidth, db = B.m_hw + RumbleWidth;
            const Color   concrete = { 150, 150, 154, 255 };
            const float   depth = ValleyDepth * valleyDepth( track, j );
            props.quad( at( A, -da, -1.6f ), at( B, -db, -1.6f ), at( B, db, -1.6f ), at( A, da, -1.6f ), scaleColor( concrete, 0.7f ) );
            if( j % 6 == 0 )
            {
                const float half = ( depth + 2.4f ) * 0.5f;
                for( float side : { -1.0f, 1.0f } )
                {
                    const Vector3 p = at( A, side * ( da - 1.2f ), -1.6f - half );
                    props.box( p, Vector3{ 0.75f, half, 0.75f }, concrete );
                }
                const Vector3 c0 = at( A, 0.0f, -2.0f );
                props.box( c0, Vector3{ 0.5f, 0.4f, 0.5f }, concrete );
                props.quad( at( A, -da, -1.6f ), at( A, da, -1.6f ), at( A, da, -2.4f ), at( A, -da, -2.4f ), concrete );
            }
            const float y = -depth + 1.2f;
            ground.quad( at( A, -160.0f, y ), at( B, -160.0f, y ), at( B, 160.0f, y ), at( A, 160.0f, y ), sf.m_water,
                         Mat{ sf.m_waterGlow, sf.m_waterGloss } );
        }

        // Guard rails, or tall sound walls.
        const BiomeDef &here = ruling( seg, 0.5f );
        for( float side : { -1.0f, 1.0f } )
        {
            if( !( seg.m_flags & ( side < 0 ? SEG_RAIL_L : SEG_RAIL_R ) ) ) continue;
            const float   la = side * ( A.m_hw + RumbleWidth + 0.55f ), lb = side * ( B.m_hw + RumbleWidth + 0.55f );
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

        // Furniture and props, per side.
        for( float side : { -1.0f, 1.0f } )
        {
            const uint64_t  sideKey = side < 0 ? 0x1111ull : 0x2222ull;
            const BiomeDef &rule = ruling( seg, hashFloat( seed ^ sideKey, static_cast<uint64_t>( j ) ) );
            const bool      railed = seg.m_flags & ( side < 0 ? SEG_RAIL_L : SEG_RAIL_R );

            if( rule.m_postSpacing > 0.0f && !railed && !tunnel && !bridge )
            {
                const long k = std::max( 2L, static_cast<long>( rule.m_postSpacing / SegmentLength + 0.5f ) );
                if( ( j + ( side < 0 ? k / 2 : 0 ) ) % k == 0 )
                    props.append( models.m_post, at( A, side * ( A.m_hw + RumbleWidth + 0.7f ), 0.0f ), A.m_heading, 1.0f );
            }
            if( rule.m_lampSpacing > 0.0f && !tunnel && !bridge )
            {
                const long k = std::max( 2L, static_cast<long>( rule.m_lampSpacing / SegmentLength + 0.5f ) );
                if( j % k == ( side > 0 ? 0 : k / 2 ) )
                {
                    const float l = side * ( A.m_hw + RumbleWidth + 1.0f );
                    props.append( models.m_lamp, at( A, l, 0.0f ), A.m_heading + ( side > 0 ? -0.5f : 0.5f ) * Tau * 0.5f, 1.0f );
                    c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.35f } );
                }
            }
            const bool chevron = seg.m_flags & ( side < 0 ? SEG_CHEVRON_L : SEG_CHEVRON_R );
            if( chevron && j % 2 == 0 && !tunnel && !bridge )
            {
                const float l = side * ( A.m_edge + 1.4f );
                props.append( side < 0 ? models.m_chevronRight : models.m_chevronLeft, at( A, l, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.5f } );
            }
            if( ( seg.m_flags & SEG_JUMP ) && !tunnel )
                props.append( models.m_jumpSign, at( A, side * ( A.m_edge + 1.2f ), 0.0f ), A.m_heading, 1.0f );

            // Scattered props.
            for( size_t r = 0; r < rule.m_props.size(); ++r )
            {
                const PropRule &pr = rule.m_props[r];
                const uint64_t  key = seed ^ ( sideKey * 31 + r * 7919 );
                const float     h0 = hashFloat( key, static_cast<uint64_t>( j ) );
                if( h0 >= pr.m_chance ) continue;
                const float h1 = hashFloat( key + 1, static_cast<uint64_t>( j ) );
                const float h2 = hashFloat( key + 2, static_cast<uint64_t>( j ) );
                const float h3 = hashFloat( key + 3, static_cast<uint64_t>( j ) );
                const float h4 = hashFloat( key + 4, static_cast<uint64_t>( j ) );
                const float dist = lerpf( pr.m_nearest, pr.m_farthest, h1 );
                const float waterSide = side < 0 ? A.m_terrain.m_waterLeft : A.m_terrain.m_waterRight;
                if( pr.m_onWater )
                {
                    if( waterSide < 0.5f || dist < 35.0f ) continue;
                }
                else if( waterSide > 0.5f && dist > 12.0f )
                {
                    continue;
                }
                if( railed && dist < 2.0f ) continue;
                if( ( tunnel && dist < 7.0f ) || ( bridge && dist < 4.0f ) ) continue;
                const float  lateral = side * ( A.m_edge + dist );
                const double ps = static_cast<double>( j ) * SegmentLength + h2 * SegmentLength;
                const float  ht = pr.m_onWater ? A.m_terrain.m_seaLevel : terrainHeight( track, j, lateral, A.m_terrain, A.m_edge );
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
                default: break;
                }
                const float scale = lerpf( pr.m_scaleMin, pr.m_scaleMax, h4 );
                const float bright = 0.88f + 0.24f * hashFloat( key + 5, static_cast<uint64_t>( j ) );
                props.append( models.prop( pr.m_kind, hash64( key + 6 + static_cast<uint64_t>( j ) ) ), pos, yaw, scale, bright );
                const float radius = models.propRadius( pr.m_kind );
                if( radius > 0.0f && dist < 45.0f ) c.m_colliders.push_back( { ps, lateral, radius * scale } );
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
    }

    c.m_ground = ground.upload();
    c.m_props = props.upload();
}

} // namespace sunstreak
