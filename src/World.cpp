/**
 * @file World.cpp
 * @brief Chunk baking: road surface, land, water, furniture and props.
 * @author Rene Weller
 */
#include "World.h"

#include "ScriptNames.h"
#include "Spectacle.h"

#include "Festival.h"
#include "Local.h"

#include "Noise.h"
#include "Train.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iterator>
#include <unordered_map>

namespace sunstreak
{

namespace
{

/** @brief One cross-section of the road, relative to the chunk's origin. */
struct Cross
{
    long           m_index = 0;       ///< segment index
    Vector3        m_center{};        ///< centre line
    Vector3        m_right{};         ///< horizontal right
    Vector3        m_forward{};       ///< horizontal forward
    float          m_heading = 0;     ///< yaw
    float          m_tanBank = 0;     ///< tan of the roll
    float          m_hw = 0;          ///< half paved width
    float          m_edge = 0;        ///< centre to the shoulder's outer edge
    Surface        m_surface{};       ///< blended colours
    Terrain        m_terrain{};       ///< blended land
    const Segment *m_seg = nullptr;   ///< the segment
    SeasonLook     m_season{};        ///< what the season does here
};

/** @brief Kinds of strips across the road. */
enum class StripE
{
    ROAD,   ///< asphalt
    LINE,   ///< solid edge line
    DASH,   ///< lane divider, dashed
};

constexpr Color WarmLight = { 255, 214, 160, 255 };   ///< street lamp light
constexpr Mat   GroundMat = { -1.0f, 0.0f };          ///< marks land for the shader's neon grid

/** @brief The land's rows: metres beyond the shoulder's edge, closer where the slopes bend, the last on the horizon. */
constexpr float LandOffsets[] = { 0.0f, 6.0f, 15.0f, 28.0f, 44.0f, 64.0f, 95.0f, 150.0f, 240.0f, 420.0f, 700.0f };
constexpr int   LandRows = static_cast<int>( std::size( LandOffsets ) );   ///< how many
constexpr long  GroundPad = 8;          ///< segments beyond a chunk's ends whose ground is known (for what reaches across)
constexpr float MaxSpread = 3.0f;       ///< the most the ground may climb under a building's walls (m); steeper, it moves or stays away
constexpr Color Snow = { 236, 240, 246, 255 };         ///< snow on the land above the snow line
constexpr Color BareRock = { 126, 122, 116, 255 };     ///< rock above the tree line

/**
 * @brief Where snow lies on a place's mountains all year: about 4900 m in the tropics, 80 m lower for each degree of
 *        latitude beyond 20, down to the sea near the poles; in winter far lower.
 * @param[in] place   the place
 * @param[in] winter  0..1 how deep in winter it is
 * @return metres above the sea (very high for the places off the map)
 */
float placeSnowLine( BiomeE place, float winter )
{
    GeoPoint where;
    if ( biomeDef( place ).m_climate == ClimateE::NONE || !placeLocation( place, where ) ) return 1e9f;
    const float lat = std::abs( where.m_lat );
    return std::max( 0.0f, 4900.0f - 80.0f * std::max( 0.0f, lat - 20.0f ) ) - 1400.0f * winter;
}

/**
 * @brief The snow line at a segment, blended between its two places.
 * @param[in] seg   the segment
 * @param[in] look  the season there
 * @return metres above the sea
 */
float snowLine( const Segment &seg, const SeasonLook &look )
{
    const float a = placeSnowLine( static_cast<BiomeE>( seg.m_biome ), look.m_winter );
    const float b = placeSnowLine( static_cast<BiomeE>( seg.m_biomeTo ), look.m_winter );
    if ( a > 1e8f || b > 1e8f ) return seg.m_blend < 0.5f ? a : b;
    return lerpf( a, b, seg.m_blend );
}

/**
 * @brief Whether a prop is a tree (it casts a round soft patch on the ground and stays below the tree line).
 * @param[in] k  the kind
 * @return true for trees
 */
bool isTree( PropKindE k )
{
    using KindT = PropKindE;
    switch ( k )
    {
    case KindT::PINE:
    case KindT::SNOW_PINE:
    case KindT::OAK:
    case KindT::PALM:
    case KindT::DEAD_TREE:
    case KindT::AUTUMN_TREE:
    case KindT::BIRCH:
    case KindT::CHERRY:
    case KindT::JUNGLE_TREE:
    case KindT::ACACIA:
    case KindT::CYPRESS:
    case KindT::MOSS_TREE:
    case KindT::OLIVE:
    case KindT::POPLAR:
    case KindT::UMBRELLA_PINE:
    case KindT::SPRUCE:
    case KindT::BAOBAB:
    case KindT::EUCALYPTUS:
    case KindT::BANANA:
    case KindT::CEDAR:
    case KindT::REDWOOD:
    case KindT::WAX_PALM:
    case KindT::DRAGON_TREE:
    case KindT::TREE_FERN:
    case KindT::BROADLEAF:
    case KindT::DATE_PALM: return true;
    default: return false;
    }
}

/**
 * @brief Whether a tree is a conifer (a forest of them stands dark and pointed).
 * @param[in] k  the kind
 * @return true for conifers
 */
bool isConifer( PropKindE k )
{
    using KindT = PropKindE;
    return k == KindT::PINE || k == KindT::SNOW_PINE || k == KindT::SPRUCE || k == KindT::CEDAR ||
           k == KindT::REDWOOD || k == KindT::CYPRESS;
}

/**
 * @brief Whether a prop stands in a line along the road and should follow the ground's slope (fences, walls, rows).
 * @param[in] k  the kind
 * @return true for those
 */
bool followsSlope( PropKindE k )
{
    using KindT = PropKindE;
    return k == KindT::FENCE || k == KindT::STONE_WALL || k == KindT::RANCH_FENCE || k == KindT::HEDGE ||
           k == KindT::VINE_ROW || k == KindT::TEA_ROW || k == KindT::PIPELINE;
}

/** @brief Paints for rows of houses whose rule asks for a different colour each (PropRule::m_pastel). */
constexpr Color Pastels[] = { { 236, 170, 180, 255 }, { 170, 206, 230, 255 }, { 240, 220, 140, 255 },
                              { 180, 220, 170, 255 }, { 240, 180, 130, 255 }, { 200, 180, 230, 255 },
                              { 240, 240, 232, 255 }, { 230, 130, 110, 255 }, { 140, 200, 200, 255 },
                              { 250, 200, 90, 255 } };

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
 * @brief The season a place's look shows, as a bit of PropRule::m_seasons (the tropics and the deserts: summer).
 * @param[in] s  the look
 * @return 1 spring, 2 summer, 4 autumn, 8 winter
 */
uint8_t seasonBit( const SeasonLook &s )
{
    if ( s.m_winter > 0.4f ) return 8;
    if ( s.m_autumn > 0.3f || s.m_bare > 0.4f ) return 4;
    return s.m_spring > 0.3f ? 1 : 2;
}

/**
 * @brief Whether a kind is a building that stands on a plot in a town, a city or a village.
 * @param[in] k     the kind
 * @param[out] tall  a high-rise
 * @return true for houses and high-rises
 */
bool plotted( PropKindE k, bool &tall )
{
    using KindT = PropKindE;
    tall = k == KindT::BUILDING || k == KindT::TOWER || k == KindT::NEON_TOWER || k == KindT::GLASS_TOWER ||
           k == KindT::PANEL_BLOCK;
    return tall || k == KindT::HOUSE || k == KindT::SNOW_HOUSE || k == KindT::CABIN || k == KindT::HUT ||
           k == KindT::COTTAGE || k == KindT::FLAT_HOUSE || k == KindT::CHALET || k == KindT::HALF_TIMBER ||
           k == KindT::ROW_HOUSES || k == KindT::ADOBE || k == KindT::RONDAVEL || k == KindT::FAVELA ||
           k == KindT::HANOK || k == KindT::GASSHO || k == KindT::HUIZHOU_HOUSE || k == KindT::CAPE_DUTCH ||
           k == KindT::YURT || k == KindT::REED_HOUSE || k == KindT::TRULLO;
}

/**
 * @brief Whether a prop is a building that stands on its own (a barn, a church, a castle): like the houses on the plots
 *        it needs flat enough ground under its footprint, and a plinth where the ground falls away.
 * @param[in] k  the kind
 * @return true for those
 */
bool standsOnFootprint( PropKindE k )
{
    using KindT = PropKindE;
    switch ( k )
    {
    case KindT::BARN:
    case KindT::SILO:
    case KindT::WINDMILL:
    case KindT::DUTCH_MILL:
    case KindT::ROUND_MILL:
    case KindT::PAGODA:
    case KindT::LIGHTHOUSE:
    case KindT::CHURCH:
    case KindT::ONION_CHURCH:
    case KindT::MOSQUE:
    case KindT::STUPA:
    case KindT::WAT:
    case KindT::TEMPLE_TOWER:
    case KindT::KASBAH:
    case KindT::CASTLE:
    case KindT::WATCHTOWER:
    case KindT::FORT:
    case KindT::GRAIN_ELEVATOR:
    case KindT::DINER:
    case KindT::GAS_STATION:
    case KindT::CASINO:
    case KindT::SUPERTALL:
    case KindT::CONTAINERS: return true;
    default: return false;
    }
}

/**
 * @brief A shop sign's colour as its part of the world paints them: muted in Europe, loud in the Americas, neon in East
 *        Asia, every colour at once in South Asia and Africa, green, white and blue in Arabia.
 * @param[in]  region  the country's fleet (its part of the world)
 * @param[in]  pick    which of the region's colours
 * @param[out] glow    how brightly its signs are lit, 0..1
 * @return the colour
 */
Color shopSign( FleetE region, uint64_t pick, float &glow )
{
    static const Color muted[6] = { { 36, 84, 60, 255 },    { 120, 30, 40, 255 }, { 28, 44, 90, 255 },
                                    { 232, 224, 200, 255 }, { 190, 150, 60, 255 }, { 60, 60, 64, 255 } };
    static const Color loud[6] = { { 230, 40, 40, 255 },   { 250, 200, 40, 255 }, { 40, 130, 235, 255 },
                                   { 40, 196, 96, 255 },   { 245, 245, 240, 255 }, { 250, 124, 30, 255 } };
    static const Color neon[6] = { { 240, 40, 40, 255 },   { 250, 210, 40, 255 }, { 250, 250, 245, 255 },
                                   { 40, 200, 120, 255 },  { 255, 90, 160, 255 }, { 60, 160, 255, 255 } };
    static const Color bazaar[6] = { { 250, 120, 20, 255 }, { 230, 30, 90, 255 },  { 30, 160, 200, 255 },
                                     { 250, 210, 40, 255 }, { 60, 180, 70, 255 },  { 140, 60, 200, 255 } };
    static const Color oasis[6] = { { 20, 130, 80, 255 },  { 245, 245, 235, 255 }, { 30, 110, 190, 255 },
                                    { 200, 160, 60, 255 }, { 20, 130, 80, 255 },   { 245, 245, 235, 255 } };
    static const Color plain[6] = { { 200, 40, 40, 255 },  { 40, 90, 170, 255 },   { 240, 240, 235, 255 },
                                    { 60, 140, 80, 255 },  { 230, 180, 50, 255 },  { 90, 90, 96, 255 } };
    const Color *set = loud;
    glow = 0.9f;
    switch ( region )
    {
    case FleetE::EUROPE:
    case FleetE::BRITISH:
    case FleetE::OCEANIA:
        set = muted;
        glow = 0.45f;
        break;
    case FleetE::EAST_ASIA:
        set = neon;
        glow = 1.0f;
        break;
    case FleetE::SOUTH_ASIA:
    case FleetE::SOUTHEAST_ASIA:
    case FleetE::AFRICA:
        set = bazaar;
        glow = 0.8f;
        break;
    case FleetE::ARABIA:
        set = oasis;
        glow = 0.8f;
        break;
    case FleetE::EASTERN:
        set = plain;
        glow = 0.6f;
        break;
    default: break;
    }
    return set[pick % 6];
}

/**
 * @brief Whether a prop is a structure that stands where it is put and never gives way to a slope: like a building it
 *        stands on its base (on a terrace where the ground climbs under it), but it neither moves nor stays away.
 * @param[in] k  the kind
 * @return true for the places' landmarks, towers, pylons, pyramids, ruins and the like
 */
bool keepsItsPlace( PropKindE k )
{
    using KindT = PropKindE;
    switch ( k )
    {
    case KindT::LANDMARK:
    case KindT::RUIN:
    case KindT::DOME:
    case KindT::CHIMNEY:
    case KindT::WATER_TOWER:
    case KindT::PUMPJACK:
    case KindT::WIND_PUMP:
    case KindT::COLUMNS:
    case KindT::AQUEDUCT:
    case KindT::PYRAMID:
    case KindT::STEP_PYRAMID:
    case KindT::STONE_PYRAMID:
    case KindT::TEMPLE_GATE:
    case KindT::TV_TOWER:
    case KindT::LATTICE_TOWER:
    case KindT::POWER_PYLON:
    case KindT::CRANE:
    case KindT::ANTENNA: return true;
    default: return false;
    }
}

/** @brief What a building stands on where the ground climbs under it. */
enum class UnderE
{
    EARTH,   ///< a terrace of earth in the land's colour
    STONE,   ///< a base of stone or plaster under its walls (and earth below it where the ground falls further)
    STILTS   ///< posts: the house stands over the ground
};

/** @brief The plots of a place: its buildings in rows back from the road instead of scattered. */
struct Plots
{
    std::array<size_t, 16> m_rule{};           ///< the building rules
    std::array<float, 16>  m_upTo{};           ///< their chances summed up to each (to pick one by weight)
    int                    m_count = 0;        ///< how many
    float                  m_along = 0.0f;     ///< a plot's length along the road (m)
    float                  m_across = 0.0f;    ///< a row's depth
    float                  m_nearest = 0.0f;   ///< the first row's front from the road's edge
    float                  m_half = 0.0f;      ///< half the widest building
    int                    m_rows = 0;         ///< rows back from the road
    bool                   m_city = false;     ///< a town or a city, built all along (else villages here and there)

    /**
     * @brief Whether a rule stands on the plots.
     * @param[in] r  the rule's index
     * @return true if it does
     */
    bool has( size_t r ) const
    {
        return std::find( m_rule.begin(), m_rule.begin() + m_count, r ) != m_rule.begin() + m_count;
    }
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
    for ( size_t r = 0; r < rule.m_props.size() && p.m_count < static_cast<int>( p.m_rule.size() ); ++r )
    {
        const PropRule &pr = rule.m_props[r];
        bool            tall = false;
        if ( pr.m_perStage > 0 || pr.m_onWater || pr.m_soft || !plotted( pr.m_kind, tall ) ) continue;
        ( tall ? tallChance : houseChance ) += pr.m_chance;
        p.m_rule[static_cast<size_t>( p.m_count )] = r;
        p.m_upTo[static_cast<size_t>( p.m_count )] = tallChance + houseChance;
        ++p.m_count;
        nearest = std::min( nearest, pr.m_nearest );
        farthest = std::max( farthest, pr.m_farthest );
        widest = std::max( widest, models.propFootprint( pr.m_kind ) * pr.m_scaleMax );
    }
    if ( p.m_count == 0 ) return p;
    const bool townish = rule.m_lampSpacing > 0.0f || rule.m_cobbles || rule.m_lanes >= 3;
    p.m_city = tallChance >= 0.05f || ( townish && houseChance >= 0.05f );
    p.m_along = std::max( 8.0f, widest ) + ( p.m_city ? 4.0f : 7.0f );
    p.m_across = p.m_along + 3.0f;
    p.m_nearest = nearest;
    p.m_half = std::max( 4.0f, widest * 0.5f );
    const float deepest =
        p.m_city ? std::max( farthest, tallChance >= 0.05f ? 320.0f : 160.0f ) : nearest + 3.0f * p.m_across;
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
    if ( hashFloat( key, static_cast<uint64_t>( b ) ) >= 0.45f ) return false;
    const long len = 50 + static_cast<long>( 60.0f * hashFloat( key + 1, static_cast<uint64_t>( b ) ) );
    const long first = 5 + static_cast<long>( static_cast<float>( block - 10 - len ) *
                                              hashFloat( key + 2, static_cast<uint64_t>( b ) ) );
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

/** @brief A town's street on one side of a segment. */
struct Pavement
{
    float m_width = 0.0f;       ///< the pavement's width beyond the parking lane (0: no town here)
    bool  m_crossing = false;   ///< a cross street meets the road here: its lights, a zebra, no pavement
};

/**
 * @brief Whether a segment's side is a town's street (its plots are a city's), and where its cross streets meet it.
 * @param[in] track   the road
 * @param[in] models  the props (their footprints set the plots)
 * @param[in] seed    the track's seed
 * @param[in] j       the segment
 * @param[in] side    -1 left, 1 right
 * @return the pavement there
 */
Pavement pavementAt( const Track &track, const Models &models, uint64_t seed, long j, float side )
{
    Pavement        p;
    const uint64_t  sideKey = side < 0 ? 0x1111ull : 0x2222ull;
    const BiomeDef &rule = ruling( track.segment( j ), hashFloat( seed ^ sideKey, static_cast<uint64_t>( j ) ) );
    const Plots     plots = plotsOf( rule, models );
    if ( !plots.m_city ) return p;
    p.m_width = clampf( plots.m_nearest - 2.0f, 2.5f, 6.0f );
    // A city leaves every fourth plot free for a cross street (the scattered props' loop does the same).
    const long segs = std::max( 2L, std::lround( plots.m_along / SegmentLength ) );
    p.m_crossing = ( j / segs ) % 4 == 0 && j % segs == segs / 2;
    return p;
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
    for ( long d = -RampSegments; d <= RampSegments; ++d )
        if ( track.segment( index + d ).m_flags & SEG_TUNNEL )
            best =
                std::max( best, 1.0f - static_cast<float>( std::abs( d ) ) / static_cast<float>( RampSegments + 1 ) );
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
    switch ( style )
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
    for ( long d = -RampSegments; d <= RampSegments; ++d )
    {
        const Segment &s = track.segment( index + d );
        if ( !( s.m_flags & SEG_BRIDGE ) ) continue;
        const float v = 1.0f - static_cast<float>( std::abs( d ) ) / static_cast<float>( RampSegments + 1 );
        if ( v > best )
        {
            best = v;
            if ( metres ) *metres = styleDepth( bridgeBiome( s ).m_bridge );
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

}   // namespace

float World::terrainHeight( const Track &track, long index, float lateral, const Terrain &terrain, float edge )
{
    const float    a = std::max( 0.0f, std::abs( lateral ) - edge );
    const bool     left = lateral < 0.0f;
    const double   s = static_cast<double>( index ) * SegmentLength;
    const uint64_t seed = track.seed();
    if ( const Serpentine *sp = track.serpentineAt( index ) )
    {
        // A pass in hairpins: one mountainside for all its legs (Track.h, Serpentine), so the land at a point lies as
        // high whichever leg it is seen from; near the road it comes down to the shoulder.
        const Segment &seg = track.segment( index );
        const Vector3  r = headingRight( seg.m_heading );
        const double   x = seg.m_pos.m_x + static_cast<double>( r.x * lateral ),
                       z = seg.m_pos.m_z + static_cast<double>( r.z * lateral );
        const float    bankL = clampf( lateral, -seg.m_halfWidth - RumbleWidth, seg.m_halfWidth + RumbleWidth );
        const float    road = static_cast<float>( seg.m_pos.m_y ) - bankL * std::tan( seg.m_bank );
        const float    rough =
            6.0f * ( valueNoise( x * 0.012, z * 0.012, seed + 31 ) - 0.5f ) * smoothstepf( 15.0f, 70.0f, a );
        const float side = static_cast<float>( sp->height( x, z ) ) - road + rough;
        return lerpf( -0.35f * smoothstepf( 0.0f, 4.0f, a ), side, smoothstepf( 2.0f, 9.0f, a ) );
    }
    // Round a fork the land lies flat and level with the road, far out to where the other carriageway goes; so does the
    // land of a landing plane's runway, on its side.
    float       keep = 1.0f - track.segment( index ).m_flat * ( 1.0f - smoothstepf( 160.0f, 260.0f, a ) );
    const float runway = track.segment( index ).m_runway;
    if ( runway != 0.0f && ( runway > 0.0f ) != left ) keep *= 1.0f - std::abs( runway );
    float       h = -0.35f * smoothstepf( 0.0f, 4.0f, a ) * keep;
    const float wall = ( left ? terrain.m_wallLeft : terrain.m_wallRight ) * keep;
    const float n1 = valueNoise( s * 0.012, lateral * 0.02, seed + 11 );
    h += wall * smoothstepf( 8.0f, 70.0f, a ) * ( 0.75f + 0.45f * n1 );
    const float hills = fbm( s * 0.0035, lateral * 0.005, seed + 23 ) * 0.5f + 0.5f;
    h += terrain.m_hills * keep * smoothstepf( 30.0f, 220.0f, a ) * hills * 1.6f;
    const float water = ( left ? terrain.m_waterLeft : terrain.m_waterRight ) * keep;
    if ( water > 0.0f ) h = lerpf( h, terrain.m_seaLevel - 4.5f, water * smoothstepf( 4.0f, 22.0f, a ) );
    const float cover = tunnelCover( track, index );
    if ( cover > 0.0f )
        h += TunnelLift * cover * smoothstepf( 0.0f, 6.0f, a ) * ( 1.0f - smoothstepf( 60.0f, 140.0f, a ) );
    float       metres = ValleyDepth;
    const float valley = valleyDepth( track, index, &metres );
    if ( valley > 0.0f )
    {
        const bool under = track.segment( index ).m_flags & SEG_BRIDGE;
        h -= metres * valley * ( under ? 1.0f : smoothstepf( 0.0f, 12.0f, a ) ) *
             ( 1.0f - smoothstepf( 80.0f, 200.0f, a ) );
    }
    // The cutting to an undersea tunnel: the land stays up behind its walls while the road sinks.
    h += track.segment( index ).m_trench * smoothstepf( 2.0f, 3.0f, a );
    return h;
}

long landmarkSpot( uint64_t seed, size_t rule, int k, long stage, long segments, bool &right )
{
    const uint64_t h =
        hash64( seed ^ ( rule * 131 + static_cast<uint64_t>( k ) * 977 + static_cast<uint64_t>( stage ) * 7919 ) );
    right = ( ( h >> 20 ) & 1u ) != 0;
    return 60 + static_cast<long>( h % static_cast<uint64_t>( std::max( 1L, segments - 120 ) ) );
}

World::~World()
{
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_quit = true;
    }
    m_wake.notify_all();
    if ( m_thread.joinable() ) m_thread.join();
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
    for ( Chunk &c : m_chunks )
    {
        UnloadMesh( c.m_ground );
        UnloadMesh( c.m_props );
        UnloadMesh( c.m_scenery );
        UnloadMesh( c.m_sceneryFar );
        UnloadMesh( c.m_glass );
        UnloadMesh( c.m_decals );
    }
    m_chunks.clear();
}

void World::work()
{
    std::unique_lock<std::mutex> lock( m_mutex );
    for ( ;; )
    {
        m_wake.wait( lock, [this] { return m_quit || ( !m_jobs.empty() && m_workerTrack ); } );
        if ( m_quit ) return;
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
        if ( generation == m_generation ) m_done.push_back( std::move( baked ) );
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
    c.m_decals = baked.m_decals.upload();
    c.m_chimneys = std::move( baked.m_chimneys );
    c.m_rotors = std::move( baked.m_rotors );
    c.m_colliders = std::move( baked.m_colliders );
    m_chunks.push_back( std::move( c ) );
    m_buildMaxMs = std::max( m_buildMaxMs, baked.m_ms );
    m_buildTotalMs += baked.m_ms;
    ++m_buildCount;
}

double World::keptBehind( const Track &track, double s )
{
    for ( double back : { 0.0, 600.0 } )
        if ( const Serpentine *sp =
                 track.serpentineAt( static_cast<long>( std::max( 0.0, s - back ) / SegmentLength ) ) )
            return std::max( KeepBehind, std::min( 1500.0, s - static_cast<double>( sp->m_first ) * SegmentLength ) );
    return KeepBehind;
}

void World::update( Track &track, double s, int maxBuilds )
{
    const auto t0 = std::chrono::steady_clock::now();
    const long firstWanted =
        std::max( 0L, static_cast<long>( std::floor( ( s - keptBehind( track, s ) ) / ChunkLength ) ) );
    const long lastWanted = static_cast<long>( std::floor( ( s + ViewDistance ) / ChunkLength ) );
    while ( !m_chunks.empty() && m_chunks.front().m_index < firstWanted )
    {
        UnloadMesh( m_chunks.front().m_ground );
        UnloadMesh( m_chunks.front().m_props );
        UnloadMesh( m_chunks.front().m_scenery );
        UnloadMesh( m_chunks.front().m_sceneryFar );
        UnloadMesh( m_chunks.front().m_glass );
        UnloadMesh( m_chunks.front().m_decals );
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
    if ( !hasWorker )
    {
        // A new track: bake what is wanted now, then give the worker its copy.
        for ( int built = 0; next <= lastWanted && built < maxBuilds; ++built, ++next ) bakeHere( next );
        std::lock_guard<std::mutex> lock( m_mutex );
        m_workerTrack = std::make_unique<Track>( track );
        m_requested = next - 1;
        if ( !m_thread.joinable() ) m_thread = std::thread( [this] { work(); } );
    }
    else
    {
        std::vector<BakedChunk> arrived;
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            if ( m_requested < next - 1 ) m_requested = next - 1;
            while ( m_requested < lastWanted && m_jobs.size() < 6 ) m_jobs.push_back( ++m_requested );
            while ( !m_done.empty() )
            {
                arrived.push_back( std::move( m_done.front() ) );
                m_done.pop_front();
            }
        }
        m_wake.notify_one();
        for ( BakedChunk &b : arrived ) m_ready[b.m_index] = std::move( b );
        while ( !m_ready.empty() && m_ready.begin()->first < next ) m_ready.erase( m_ready.begin() );
        for ( int uploaded = 0; uploaded < std::max( 1, maxBuilds ) && next <= lastWanted; ++next )
        {
            auto it = m_ready.find( next );
            if ( it != m_ready.end() )
            {
                upload( it->second );
                m_ready.erase( it );
                ++uploaded;
                continue;
            }
            // Not there yet: wait for it unless the car is close to the end of the world.
            if ( static_cast<double>( next ) * ChunkLength - s > 250.0 ) break;
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
    for ( const Chunk &c : m_chunks )
    {
        const double c0 = static_cast<double>( c.m_index ) * ChunkLength;
        if ( c0 > s + range || c0 + ChunkLength < s - range ) continue;
        for ( const Collider &k : c.m_colliders )
            if ( std::abs( k.m_s - s ) < range ) out.push_back( &k );
    }
}

void World::buildChunk( Track &track, BakedChunk &c ) const
{
    const long i0 = c.m_index * ChunkSegments;
    track.generateTo( static_cast<double>( i0 + ChunkSegments + GroundPad + 2 ) * SegmentLength );
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
        // The season of either place, before they blend.
        const float      phase = m_seasons.phaseAt( static_cast<double>( j ) * SegmentLength );
        const SeasonLook la = seasonAt( static_cast<BiomeE>( seg.m_biome ), phase ),
                         lb = seasonAt( static_cast<BiomeE>( seg.m_biomeTo ), phase );
        Surface          sa = a.m_surface, sb = b.m_surface;
        seasonSurface( sa, la );
        seasonSurface( sb, lb );
        x.m_surface = blendSurface( sa, sb, seg.m_blend );
        x.m_season =
            SeasonLook{ lerpf( la.m_autumn, lb.m_autumn, seg.m_blend ), lerpf( la.m_winter, lb.m_winter, seg.m_blend ),
                        lerpf( la.m_bare, lb.m_bare, seg.m_blend ), lerpf( la.m_spring, lb.m_spring, seg.m_blend ) };
        x.m_terrain = blendTerrain( a.m_terrain, b.m_terrain, seg.m_blend );
        return x;
    };

    // The land's rows as its mesh draws them, from some segments before the chunk to some after it: what stands on the
    // land takes its height from them, so it stands on the ground as drawn (between the rows the formula strays by metres).
    struct LandRow
    {
        Cross m_cross;                       ///< the cross-section
        float m_lateral[2][LandRows] = {};   ///< the rows' lateral offsets, left and right
        float m_height[2][LandRows] = {};    ///< their heights above the road there
        float m_snowLine = 1e9f;             ///< where snow lies in this season (m above the sea; very high: nowhere)
        float m_treeLine = 1e9f;             ///< where the trees end, whatever the season
        float m_pave[2] = {};                ///< a town's pavement beyond the parking lane, left and right (m; 0: none)
        float m_snowCover = 0.0f;            ///< 0..1 the land lies under snow (where the road runs between banks of it)
    };
    std::vector<LandRow> landRows( static_cast<size_t>( ChunkSegments + 2 * GroundPad + 2 ) );
    for ( size_t r = 0; r < landRows.size(); ++r )
    {
        LandRow &w = landRows[r];
        w.m_cross = cross( i0 - GroundPad + static_cast<long>( r ) );
        const Cross   &X = w.m_cross;
        const Segment &xs = *X.m_seg;
        for ( int s = 0; s < 2; ++s )
        {
            // In a serpentine the land ends where the next leg's begins.
            const float reach = s == 0 ? xs.m_reachL : xs.m_reachR;
            const float squeeze =
                reach > 0.0f ? clampf( ( reach - X.m_edge ) / LandOffsets[LandRows - 1], 0.0f, 1.0f ) : 1.0f;
            for ( int k = 0; k < LandRows; ++k )
            {
                w.m_lateral[s][k] = ( s == 0 ? -1.0f : 1.0f ) * ( X.m_edge + LandOffsets[k] * squeeze );
                w.m_height[s][k] = terrainHeight( track, X.m_index, w.m_lateral[s][k], X.m_terrain, X.m_edge );
            }
            // A town's pavement: not in the sea, a tunnel or a gallery, on a bridge, in a pass's hairpins, or where a
            // fork's other carriageway turns away.
            const Pavement pave = pavementAt( track, models, seed, X.m_index, s == 0 ? -1.0f : 1.0f );
            const float    water = s == 0 ? X.m_terrain.m_waterLeft : X.m_terrain.m_waterRight;
            const Fork     split = track.fork( xs.m_stage );
            const double   along = static_cast<double>( X.m_index ) * SegmentLength - split.m_at;
            const bool     peel = split.m_exists && along > ForkMedian - 60.0 && along < ForkMedian + ForkPeel + 40.0;
            if ( pave.m_width > 0.0f && !pave.m_crossing && water < 0.5f && xs.m_gallery == 0 && reach <= 0.0f &&
                 !peel && !( xs.m_flags & ( SEG_TUNNEL | SEG_BRIDGE ) ) )
                w.m_pave[s] = pave.m_width;
        }
        w.m_snowLine = snowLine( xs, X.m_season );
        const float summer = snowLine( xs, SeasonLook{} );
        w.m_treeLine = summer < 1e8f ? summer - 700.0f : 1e9f;
        // Where the place clears its road through deep snow, the land beside it lies under snow as well.
        if ( bridgeBiome( xs ).m_snowWalls > 0.0f && ( X.m_season.m_winter > 0.25f || X.m_season.m_spring > 0.35f ) )
            w.m_snowCover = 1.0f;
    }
    auto landRow = [&]( long j ) -> const LandRow &
    {
        const long last = static_cast<long>( landRows.size() ) - 1;
        return landRows[static_cast<size_t>( std::clamp( j - ( i0 - GroundPad ), 0L, last ) )];
    };
    // The land's height on a row (relative to the chunk's origin): flat on the road, the shoulder sloping to the land's
    // first row, then straight from row to row.
    auto rowY = [&]( const LandRow &w, float l )
    {
        const Cross &X = w.m_cross;
        const int    s = l < 0.0f ? 0 : 1;
        const float  a = std::abs( l ), kerb = X.m_hw + RumbleWidth;
        float        h = 0.0f;
        if ( a > X.m_edge )
        {
            int k = 0;
            while ( k + 2 < LandRows && std::abs( w.m_lateral[s][k + 1] ) < a ) ++k;
            const float a0 = std::abs( w.m_lateral[s][k] ), a1 = std::abs( w.m_lateral[s][k + 1] );
            h = lerpf( w.m_height[s][k], w.m_height[s][k + 1], clampf( ( a - a0 ) / std::max( 0.01f, a1 - a0 ), 0.0f, 1.0f ) );
        }
        else if ( a > kerb )
        {
            h = w.m_height[s][0] * ( a - kerb ) / std::max( 0.01f, X.m_edge - kerb );
        }
        return at( X, l, h ).y;
    };
    // A town's pavement on a row: a kerb's height above the road, or the land where that lies higher.
    auto slabY = [&]( const LandRow &w, float l ) { return std::max( rowY( w, l ), at( w.m_cross, l, 0.0f ).y ) + 0.15f; };
    // The ground's height a share t of the way from segment j to the next, at a lateral offset: on the very triangles the
    // ground is drawn with (each strip's quad is split from its inner corner at j to its outer corner at j + 1).
    auto groundY = [&]( long j, float t, float l )
    {
        const LandRow &ra = landRow( j ), &rb = landRow( j + 1 );
        const Cross   &XA = ra.m_cross, &XB = rb.m_cross;
        const int      s = l < 0.0f ? 0 : 1;
        const float    sign = l < 0.0f ? -1.0f : 1.0f, a = std::abs( l );
        const float    kerbA = XA.m_hw + RumbleWidth, kerbB = XB.m_hw + RumbleWidth;
        const float    edge = lerpf( XA.m_edge, XB.m_edge, t );
        if ( a <= lerpf( kerbA, kerbB, t ) ) return lerpf( at( XA, l, 0.0f ).y, at( XB, l, 0.0f ).y, t );
        if ( ra.m_pave[s] > 0.0f && a <= edge + ra.m_pave[s] )
        {
            // A town's street: the parking lane at the road's level, the pavement beyond it.
            if ( a <= edge ) return lerpf( at( XA, l, 0.0f ).y, at( XB, l, 0.0f ).y, t );
            return lerpf( slabY( ra, l ), slabY( rb, l ), t );
        }
        // The strip the point lies in: its inner and outer edges at either end.
        float inA = kerbA, outA = XA.m_edge, inB = kerbB, outB = XB.m_edge;
        float hInA = 0.0f, hOutA = ra.m_height[s][0], hInB = 0.0f, hOutB = rb.m_height[s][0];
        if ( a > edge )
        {
            int k = 0;
            while ( k + 2 < LandRows &&
                    lerpf( std::abs( ra.m_lateral[s][k + 1] ), std::abs( rb.m_lateral[s][k + 1] ), t ) < a )
                ++k;
            inA = std::abs( ra.m_lateral[s][k] );
            outA = std::abs( ra.m_lateral[s][k + 1] );
            inB = std::abs( rb.m_lateral[s][k] );
            outB = std::abs( rb.m_lateral[s][k + 1] );
            hInA = ra.m_height[s][k];
            hOutA = ra.m_height[s][k + 1];
            hInB = rb.m_height[s][k];
            hOutB = rb.m_height[s][k + 1];
        }
        const float in = lerpf( inA, inB, t ), out = lerpf( outA, outB, t );
        const float u = clampf( ( a - in ) / std::max( 0.01f, out - in ), 0.0f, 1.0f );
        const Vector3 pInA = at( XA, sign * inA, hInA ), pOutA = at( XA, sign * outA, hOutA );
        const Vector3 pInB = at( XB, sign * inB, hInB ), pOutB = at( XB, sign * outB, hOutB );
        // Where the point lies on the map, and the drawn triangle over that spot: in a hairpin the strips fan out, and a
        // point's shares along and across them are not its place in the triangle.
        const Vector3 q = Vector3Lerp( at( XA, l, 0.0f ), at( XB, l, 0.0f ), t );
        auto          over = [&]( Vector3 p0, Vector3 p1, Vector3 p2, float &y )
        {
            const float d = ( p1.z - p2.z ) * ( p0.x - p2.x ) + ( p2.x - p1.x ) * ( p0.z - p2.z );
            if ( std::abs( d ) < 1e-5f ) return false;
            const float w0 = ( ( p1.z - p2.z ) * ( q.x - p2.x ) + ( p2.x - p1.x ) * ( q.z - p2.z ) ) / d;
            const float w1 = ( ( p2.z - p0.z ) * ( q.x - p2.x ) + ( p0.x - p2.x ) * ( q.z - p2.z ) ) / d;
            const float w2 = 1.0f - w0 - w1;
            y = w0 * p0.y + w1 * p1.y + w2 * p2.y;
            return w0 >= -0.02f && w1 >= -0.02f && w2 >= -0.02f;
        };
        float y = 0.0f;
        if ( over( pInA, pOutA, pOutB, y ) || over( pInA, pOutB, pInB, y ) ) return y;
        return u >= t ? pInA.y + u * ( pOutA.y - pInA.y ) + t * ( pOutB.y - pOutA.y )
                      : pInA.y + t * ( pInB.y - pInA.y ) + u * ( pOutB.y - pInB.y );
    };
    // The marsh's ponds: in the land's near rows, here and there.
    auto pondAt = [&]( long j, int s, int k )
    {
        const Cross &X = landRow( j ).m_cross;
        return X.m_terrain.m_ponds > 0.0f && k >= 1 && k <= 4 &&
               hashFloat( seed + 99 + static_cast<uint64_t>( k + s * 7 ), static_cast<uint64_t>( j / 2 ) ) <
                   X.m_terrain.m_ponds * 0.3f;
    };
    // What height and snow do to the land's colour: above the tree line rock; over the snow line (lower in winter) snow
    // where it is not too steep; and all of it under snow where the road runs between banks of snow.
    auto highTone = [&]( Color col, long j, float alt, float slope )
    {
        const LandRow &ra = landRow( j ), &rb = landRow( j + 1 );
        const float    line = lerpf( ra.m_snowLine, rb.m_snowLine, 0.5f );
        const float    lies = 1.0f - smoothstepf( 0.9f, 1.7f, slope );
        if ( line < 1e8f )
        {
            const float trees = lerpf( ra.m_treeLine, rb.m_treeLine, 0.5f );
            col = lerpColor( col, BareRock, 0.8f * smoothstepf( trees + 150.0f, trees + 550.0f, alt ) );
            col = lerpColor( col, Snow, smoothstepf( line - 80.0f, line + 120.0f, alt ) * lies );
        }
        const float cover = 0.5f * ( ra.m_snowCover + rb.m_snowCover );
        if ( cover > 0.0f ) col = lerpColor( col, Snow, 0.94f * cover * ( 0.45f + 0.55f * lies ) );
        return col;
    };
    // The land's colour some way from the road at a height (relative to the chunk), without its fields and patches.
    auto landTone = [&]( long j, float dist, float y )
    {
        const Surface &surface = landRow( j ).m_cross.m_surface;
        const Color    base = lerpColor( ( j / 2 ) % 2 == 0 ? surface.m_groundA : surface.m_groundB, surface.m_groundFar,
                                         smoothstepf( 20.0f, 110.0f, dist ) );
        return highTone( base, j, static_cast<float>( c.m_origin.m_y ) + y, 0.0f );
    };
    // The same for a point given by its offset from where segment j's centre line lies a share t of the way along.
    auto groundAt = [&]( long j, float t, float l, Vector3 offset )
    {
        const Cross &X = landRow( j ).m_cross;
        const float  along = t * SegmentLength + Vector3DotProduct( offset, X.m_forward );
        const float  steps = std::floor( along / SegmentLength );
        return groundY( j + static_cast<long>( steps ), along / SegmentLength - steps,
                        l + Vector3DotProduct( offset, X.m_right ) );
    };

    // A point beside the road: at a lateral offset, a metres along the road from where segment j lies a share t of the way
    // (it follows the road's bends), on the ground as drawn and lifted a little.
    auto onGround = [&]( long j, float t, float l, float a, float lift )
    {
        const float along = t * SegmentLength + a;
        const float steps = std::floor( along / SegmentLength ), tt = along / SegmentLength - steps;
        const long  jj = j + static_cast<long>( steps );
        Vector3     p = Vector3Lerp( at( landRow( jj ).m_cross, l, 0.0f ), at( landRow( jj + 1 ).m_cross, l, 0.0f ), tt );
        p.y = groundY( jj, tt, l ) + lift;
        return p;
    };
    // How far a model reaches from its origin (walking its vertices once per model, not per prop).
    std::unordered_map<const MeshBuilder *, Vector3> extents;
    auto                                             extentOf = [&]( const MeshBuilder &m )
    {
        const auto it = extents.find( &m );
        if ( it != extents.end() ) return it->second;
        const Vector3 e = m.extent();
        extents.emplace( &m, e );
        return e;
    };
    // Where a building's walls meet the ground: the middle of their box and its half sizes (its roof may reach further).
    std::unordered_map<const MeshBuilder *, std::pair<Vector3, Vector3>> bases;
    auto baseOf = [&]( const MeshBuilder &m ) -> const std::pair<Vector3, Vector3> &
    {
        const auto it = bases.find( &m );
        if ( it != bases.end() ) return it->second;
        const Vector3 e = extentOf( m );
        Vector3       lo{}, hi{};
        if ( !m.baseBox( 0.3f, lo, hi ) || hi.x - lo.x < 1.0f || hi.z - lo.z < 1.0f )
        {
            lo = Vector3{ -e.x, 0.0f, -e.z };
            hi = Vector3{ e.x, 0.0f, e.z };
        }
        return bases
            .emplace( &m, std::make_pair( Vector3{ ( lo.x + hi.x ) * 0.5f, 0.0f, ( lo.z + hi.z ) * 0.5f },
                                          Vector3{ ( hi.x - lo.x ) * 0.5f, 0.0f, ( hi.z - lo.z ) * 0.5f } ) )
            .first->second;
    };
    // A strip lying on the ground (a drive, a cross street): across from one lateral offset to another, w wide, centred
    // a metres along from segment j's point at share t.
    auto groundStrip = [&]( MeshBuilder &m, long j, float t, float a, float l0, float l1, float w, Color col, Mat mat )
    {
        const int n = std::max( 1, static_cast<int>( std::ceil( std::abs( l1 - l0 ) / 4.0f ) ) );
        for ( int i = 0; i < n; ++i )
        {
            const float la = lerpf( l0, l1, static_cast<float>( i ) / static_cast<float>( n ) );
            const float lb = lerpf( l0, l1, static_cast<float>( i + 1 ) / static_cast<float>( n ) );
            m.quad( onGround( j, t, la, a - w * 0.5f, 0.07f ), onGround( j, t, lb, a - w * 0.5f, 0.07f ),
                    onGround( j, t, lb, a + w * 0.5f, 0.07f ), onGround( j, t, la, a + w * 0.5f, 0.07f ), col, mat );
        }
    };
    // A low wall, a fence or a hedge on the ground from (l0, a0) to (l1, a1) (lateral offsets, metres along from segment
    // j's point at share t), following the slope.
    auto groundWall =
        [&]( MeshBuilder &m, long j, float t, float l0, float a0, float l1, float a1, float height, float thick, Color col )
    {
        const Vector3 p0 = onGround( j, t, l0, a0, 0.0f ), p1 = onGround( j, t, l1, a1, 0.0f );
        const Vector3 dir = Vector3Normalize( Vector3{ p1.x - p0.x, 0.0f, p1.z - p0.z } );
        const Vector3 across = { -dir.z * thick * 0.5f, 0.0f, dir.x * thick * 0.5f };
        const int     n = std::max( 1, static_cast<int>( std::ceil( std::hypot( l1 - l0, a1 - a0 ) / 3.0f ) ) );
        const Color   side = scaleColor( col, 0.88f ), top = scaleColor( col, 1.08f ), foot = scaleColor( col, 0.62f );
        for ( int i = 0; i < n; ++i )
        {
            const float   u0 = static_cast<float>( i ) / static_cast<float>( n ), u1 = static_cast<float>( i + 1 ) / n;
            const Vector3 g0 = onGround( j, t, lerpf( l0, l1, u0 ), lerpf( a0, a1, u0 ), 0.0f );
            const Vector3 g1 = onGround( j, t, lerpf( l0, l1, u1 ), lerpf( a0, a1, u1 ), 0.0f );
            const Vector3 lo0 = { g0.x, g0.y - 0.25f, g0.z }, lo1 = { g1.x, g1.y - 0.25f, g1.z };
            const Vector3 hi0 = { g0.x, g0.y + height, g0.z }, hi1 = { g1.x, g1.y + height, g1.z };
            for ( float s : { -1.0f, 1.0f } )
            {
                const Vector3 o = Vector3Scale( across, s );
                const Vector3 q[4] = { Vector3Add( lo0, o ), Vector3Add( lo1, o ), Vector3Add( hi1, o ), Vector3Add( hi0, o ) };
                const Color   c4[4] = { foot, foot, side, side };
                const Mat     m4[4] = {};
                m.quadV( q, c4, m4 );
            }
            m.quad( Vector3Add( hi0, across ), Vector3Add( hi1, across ), Vector3Subtract( hi1, across ),
                    Vector3Subtract( hi0, across ), top );
            if ( i == 0 )
                m.quad( Vector3Add( lo0, across ), Vector3Add( hi0, across ), Vector3Subtract( hi0, across ),
                        Vector3Subtract( lo0, across ), side );
            if ( i == n - 1 )
                m.quad( Vector3Add( lo1, across ), Vector3Add( hi1, across ), Vector3Subtract( hi1, across ),
                        Vector3Subtract( lo1, across ), side );
        }
    };

    MeshBuilder ground;
    MeshBuilder props;
    MeshBuilder scenery, sceneryFar;
    MeshBuilder glass;
    MeshBuilder decals;

    // Soft dark patches where something stands on the ground: a ring round a footprint (darkest at its walls), or a disc
    // under a tree's crown. Up close only (the chunk's decals).
    constexpr float OnTheGround = -1e9f;   // contactRing's level for a ring that follows the ground
    auto contactRing = [&]( long j, float t, float l, float yaw, Vector3 mid, float hx, float hz, float grow,
                            unsigned char alpha, float level )
    {
        const Vector3 L = Vector3Negate( headingRight( yaw ) ), F = headingForward( yaw );
        const float   cx[4] = { -1.0f, 1.0f, 1.0f, -1.0f }, cz[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
        const Color   dark = { 0, 0, 0, alpha }, clear = { 0, 0, 0, 0 };
        const Mat     m4[4] = {};
        const Color   c4[4] = { dark, dark, clear, clear };
        for ( int e = 0; e < 4; ++e )
        {
            const int   f = ( e + 1 ) % 4;
            const float len = e % 2 == 0 ? hx * 2.0f : hz * 2.0f;
            const int   n = std::max( 1, static_cast<int>( std::ceil( len / 3.0f ) ) );
            for ( int i = 0; i < n; ++i )
            {
                Vector3 q[4];
                for ( int v = 0; v < 4; ++v )
                {
                    const float u = static_cast<float>( i + ( v == 1 || v == 2 ? 1 : 0 ) ) / static_cast<float>( n );
                    const float g = v >= 2 ? grow : 0.0f;
                    const float x = mid.x + lerpf( cx[e], cx[f], u ) * ( hx + g ),
                                z = mid.z + lerpf( cz[e], cz[f], u ) * ( hz + g );
                    const Vector3 off = Vector3Add( Vector3Scale( L, x ), Vector3Scale( F, z ) );
                    q[v] = onGround( j, t, l + Vector3DotProduct( off, landRow( j ).m_cross.m_right ),
                                     Vector3DotProduct( off, landRow( j ).m_cross.m_forward ), 0.07f );
                    if ( level > OnTheGround ) q[v].y = level;
                }
                decals.quadV( q, c4, m4 );
            }
        }
    };
    auto contactDisc = [&]( long j, float t, float l, float radius, unsigned char alpha )
    {
        const Vector3 c0 = onGround( j, t, l, 0.0f, 0.07f );
        Vector3       prev{};
        for ( int i = 0; i <= 8; ++i )
        {
            const float   a = Tau * static_cast<float>( i ) / 8.0f;
            const Vector3 p = onGround( j, t, l + std::cos( a ) * radius, std::sin( a ) * radius, 0.07f );
            if ( i > 0 ) decals.tri3( c0, prev, p, Color{ 0, 0, 0, alpha }, Color{ 0, 0, 0, 0 }, Color{ 0, 0, 0, 0 } );
            prev = p;
        }
    };

    // Light pools of the street lamps: lamps stand every k segments, right side at j % k == 0, left at k/2.
    auto lampLight = [&]( long j, float l, float t )
    {
        float light = 0.0f;
        for ( long m = j - 12; m <= j + 12; ++m )
        {
            const Segment  &seg = track.segment( m );
            const BiomeDef &d = ruling( seg, 0.5f );
            if ( d.m_lampSpacing <= 0.0f ) continue;
            const long k = std::max( 2L, static_cast<long>( d.m_lampSpacing / SegmentLength + 0.5f ) );
            float      side = 0.0f;
            if ( m % k == 0 )
                side = 1.0f;
            else if ( m % k == k / 2 )
                side = -1.0f;
            else
                continue;
            const float headL = side * ( seg.m_halfWidth + RumbleWidth + 1.0f - 2.9f );
            const float ds = ( static_cast<float>( j - m ) + t ) * SegmentLength;
            const float dl = l - headL;
            light += std::exp( -( ds * ds / 55.0f + dl * dl / 40.0f ) );
        }
        return std::min( light, 1.2f );
    };

    for ( long j = i0; j < i0 + ChunkSegments; ++j )
    {
        const Cross   &A = landRow( j ).m_cross;
        const Cross   &B = landRow( j + 1 ).m_cross;
        const Segment &seg = *A.m_seg;
        const Surface &sf = A.m_surface;
        const bool     tunnel = seg.m_flags & SEG_TUNNEL;
        const bool     bridge = seg.m_flags & SEG_BRIDGE;
        double         runwayS = 0.0;
        float          runwaySide = 0.0f;
        const bool     hasRunway =
            stageRunway( track, track.stageAt( static_cast<double>( j ) * SegmentLength ), runwayS, runwaySide );
        const bool runwayHere = hasRunway && static_cast<long>( runwayS / SegmentLength ) == j;
        ground.setShade( tunnel ? 0.12f : seg.m_gallery != 0 ? 0.5f : 1.0f );

        // A ferry or a car train over the sea: from where the car drives aboard to where it drives off, the vessels'
        // decks, open water to the horizon (or the bank the train goes into) and the vessels themselves.
        const double sj = static_cast<double>( j ) * SegmentLength;
        // A fast train's viaduct beside the road (Train.h), from a stone portal to a stone portal; props keep clear of it.
        TrainStretch   railway;
        const bool     hasRailway = !tunnel && !bridge && stageTrain( track, track.stageAt( sj ), railway );
        const double   viaFrom = railway.m_from - RailBefore, viaTo = railway.m_to + RailAfter;
        const bool     viaduct = hasRailway && sj >= viaFrom - PortalLength && sj < viaTo + PortalLength;
        const float    railL = railway.m_side * ( A.m_edge + RailOffset );
        const Crossing xc = track.crossingAt( sj );
        if ( xc.m_type != CrossingE::NONE && sj >= xc.m_board && sj < xc.m_land )
        {
            const bool      ferry = xc.m_type == CrossingE::FERRY;
            const bool      out = sj < xc.m_board + xc.m_length, in = sj >= xc.m_land - xc.m_length;
            const BiomeDef &shore = biomeDef(
                track.stageBiome( track.stageAt( sj < ( xc.m_board + xc.m_land ) * 0.5 ? xc.m_board : xc.m_land ) ) );
            const Surface &sh = shore.m_surface;
            if ( ferry )
            {
                ground.quad( at( A, -1500.0f, -3.0f ), at( A, 1500.0f, -3.0f ), at( B, 1500.0f, -3.0f ),
                             at( B, -1500.0f, -3.0f ), sh.m_water, Mat{ sh.m_waterGlow, sh.m_waterGloss } );
            }
            else if ( out || in )
            {
                ground.quad( at( A, -40.0f, -0.25f ), at( A, 40.0f, -0.25f ), at( B, 40.0f, -0.25f ),
                             at( B, -40.0f, -0.25f ), sh.m_shoulder );
                for ( float side : { -1.0f, 1.0f } )
                    ground.quad( at( A, side * 40.0f, -0.25f ), at( A, side * 900.0f, 0.0f ),
                                 at( B, side * 900.0f, 0.0f ), at( B, side * 40.0f, -0.25f ), sh.m_groundFar );
            }
            else
            {
                // The bank over the tunnel: its top, and its face where the trains go in and come out.
                ground.quad( at( A, -900.0f, 22.0f ), at( A, 900.0f, 22.0f ), at( B, 900.0f, 22.0f ),
                             at( B, -900.0f, 22.0f ), sh.m_groundFar );
                const double ua = sj - xc.m_board - xc.m_length, ub = xc.m_land - xc.m_length - sj - SegmentLength;
                if ( ua >= 0.0 && ua < SegmentLength )
                    ground.quad( at( A, -900.0f, -0.3f ), at( A, 900.0f, -0.3f ), at( A, 900.0f, 22.0f ),
                                 at( A, -900.0f, 22.0f ), scaleColor( sh.m_groundFar, 0.8f ) );
                if ( ub >= 0.0 && ub < SegmentLength )
                    ground.quad( at( B, -900.0f, -0.3f ), at( B, 900.0f, -0.3f ), at( B, 900.0f, 22.0f ),
                                 at( B, -900.0f, 22.0f ), scaleColor( sh.m_groundFar, 0.8f ) );
            }
            if ( out || in )
                ground.quad( at( A, -A.m_hw, 0.0f ), at( A, A.m_hw, 0.0f ), at( B, B.m_hw, 0.0f ),
                             at( B, -B.m_hw, 0.0f ), Color{ 70, 74, 80, 255 } );
            // The vessels, open towards the land; beyond the trains the mouth of their tunnel.
            const int    vessel = landmarkByName( ferry ? "FERRY_SHIP" : "CAR_TRAIN" ),
                         mouth = landmarkByName( "TUNNEL_PORTAL" );
            const double ua = sj - xc.m_board, ub = xc.m_land - sj - SegmentLength;
            if ( vessel >= 0 && ua < SegmentLength )
                props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( vessel ) ), onGround( j, 0.0f, 0.0f, 0.0f, 0.0f ),
                              A.m_heading, 1.0f );
            if ( vessel >= 0 && ub >= 0.0 && ub < SegmentLength )
                props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( vessel ) ), at( B, 0.0f, 0.0f ),
                              B.m_heading + Tau * 0.5f, 1.0f );
            if ( !ferry && mouth >= 0 )
            {
                const double ma = ua - xc.m_length - 5.0, mb = ub - xc.m_length - 5.0;
                if ( ma >= 0.0 && ma < SegmentLength )
                    props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( mouth ) ),
                                  onGround( j, 0.0f, 0.0f, 0.0f, 0.0f ), A.m_heading, 1.0f );
                if ( mb >= 0.0 && mb < SegmentLength )
                    props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( mouth ) ),
                                  at( B, 0.0f, 0.0f ), B.m_heading + Tau * 0.5f, 1.0f );
            }
            continue;
        }
        const bool band = ( j / 2 ) % 2 == 0;
        const int  lanes = seg.m_lanes;
        // The country's markings: yellow between the directions (the Americas, Scandinavia, East Asia; on a road of an
        // odd number of lanes its centre-side edge), on the centre-side edge (Arabia) or on the outer edge (southern
        // Africa, Ireland), mirrored where they drive on the left.
        const BiomeDef    &here = ruling( seg, 0.5f );
        const CountryInfo &land = countryInfo( here.m_country );
        // A fork of the world tour: where its barrier begins (u = 0) and where the other carriageway goes.
        const Fork   fork = track.fork( seg.m_stage );
        const double u = static_cast<double>( j ) * SegmentLength - fork.m_at;
        const int    half = lanes / 2;
        const float  carriage =
            static_cast<float>( std::max( 1, half ) ) * here.m_laneWidth * 0.5f;   // half a carriageway
        auto branchX = [&]( double v ) -> float
        {
            const double w = std::clamp( v - ForkMedian, 0.0, ForkPeel );
            return static_cast<float>( -fork.m_chosen ) *
                   ( carriage + 0.6f + static_cast<float>( 130.0 * ( w / ForkPeel ) * ( w / ForkPeel ) ) );
        };
        const bool  peeling = fork.m_exists && fork.m_chosen != 0 && u >= ForkMedian && u < ForkMedian + ForkPeel;
        const Color roadCol = band ? sf.m_road : sf.m_roadAlt;
        const bool  marked = here.m_country && sf.m_lineGlow <= 0.0f && !here.m_unpaved;
        const bool  between = marked && land.m_lines == 1 && lanes % 2 == 0;   // yellow down the middle
        Color       lineL = sf.m_line, lineR = sf.m_line, centre = sf.m_line;
        if ( marked )
        {
            const Color yellow = { 242, 188, 34, 255 };
            if ( land.m_lines == 3 || ( land.m_lines == 1 && !between ) ) ( land.m_left ? lineR : lineL ) = yellow;
            if ( land.m_lines == 2 ) ( land.m_left ? lineL : lineR ) = yellow;
            if ( between ) centre = yellow;
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
        if ( dirt.g > dirt.r ) dirt = lerpColor( dirt, Color{ 150, 128, 96, 255 }, 0.65f );
        if ( !band ) dirt = scaleColor( dirt, 0.95f );

        // Road surface: strips between the edge lines and lane dividers, each with per-corner lamp light. A column
        // sits at a fraction of the half width plus fixed metres, resolved per cross-section.
        struct Column
        {
            float  m_frac;   ///< fraction of the half width (-1..1)
            float  m_off;    ///< plus metres
            StripE m_kind;   ///< the strip that starts here
        };
        std::vector<Column> cols;
        cols.push_back( { -1.0f, 0.0f, StripE::ROAD } );
        cols.push_back( { -1.0f, 0.25f, StripE::LINE } );
        cols.push_back( { -1.0f, 0.45f, StripE::ROAD } );
        for ( int k = 1; k < lanes; ++k )
        {
            const float f = -1.0f + 2.0f * static_cast<float>( k ) / static_cast<float>( lanes );
            if ( fork.m_exists && k * 2 == lanes && u > -120.0 && u < ForkMedian )
            {
                // The gore before the barrier: a hatched wedge between the carriageways.
                const float g = u < 0.0 ? 0.6f * smoothstepf( -120.0f, 0.0f, static_cast<float>( u ) ) : 0.6f;
                cols.push_back( { 0.0f, -g - 0.15f, StripE::LINE } );
                cols.push_back( { 0.0f, -g, StripE::DASH } );
                cols.push_back( { 0.0f, g, StripE::LINE } );
                cols.push_back( { 0.0f, g + 0.15f, StripE::ROAD } );
                continue;
            }
            if ( between && k * 2 == lanes && lanes >= 4 )
            {
                cols.push_back( { f, -0.22f, StripE::LINE } );
                cols.push_back( { f, -0.08f, StripE::ROAD } );
                cols.push_back( { f, 0.08f, StripE::LINE } );
                cols.push_back( { f, 0.22f, StripE::ROAD } );
                continue;
            }
            if ( k * 2 == lanes && track.noPassing( j ) )
            {
                // The middle of a road of two directions: solid on a wide road, and where nobody sees far enough to
                // overtake (around a tight bend or a crest).
                cols.push_back( { f, -0.08f, StripE::LINE } );
                cols.push_back( { f, 0.08f, StripE::ROAD } );
                continue;
            }
            cols.push_back( { f, -0.08f, StripE::DASH } );
            cols.push_back( { f, 0.08f, StripE::ROAD } );
        }
        cols.push_back( { 1.0f, -0.45f, StripE::LINE } );
        cols.push_back( { 1.0f, -0.25f, StripE::ROAD } );
        cols.push_back( { 1.0f, 0.0f, StripE::ROAD } );
        const bool  dashOn = band && !( seg.m_flags & SEG_NO_DASH );
        const float wear = marked ? here.m_wear : 0.0f;
        for ( size_t k = 0; k + 1 < cols.size(); ++k )
        {
            const float la0 = cols[k].m_frac * A.m_hw + cols[k].m_off,
                        la1 = cols[k + 1].m_frac * A.m_hw + cols[k + 1].m_off;
            const float lb0 = cols[k].m_frac * B.m_hw + cols[k].m_off,
                        lb1 = cols[k + 1].m_frac * B.m_hw + cols[k + 1].m_off;
            // The line down the middle in its own colour (yellow where it parts the directions), never worn away.
            const bool middle = std::abs( cols[k].m_frac ) < 0.001f && ( between || lanes % 2 == 0 );
            Color       base = roadCol;
            float       glow = 0.0f;
            float       gloss = 0.18f;
            const bool  cobbled = here.m_cobbles && cols[k].m_kind != StripE::LINE;
            // A worn road loses dashes, and stretches of its edge lines.
            const bool dashGone =
                wear > 0.0f && hashFloat( seed * 23 + k, static_cast<uint64_t>( j / 4 ) ) < 0.45f * wear;
            const bool lineGone = wear > 0.45f && !middle &&
                                  hashFloat( seed * 29 + k, static_cast<uint64_t>( j / 3 ) ) < 1.2f * ( wear - 0.45f );
            if ( ( cols[k].m_kind == StripE::LINE && !lineGone ) ||
                 ( cols[k].m_kind == StripE::DASH && dashOn && !dashGone ) )
            {
                base = middle                           ? centre
                       : cols[k].m_kind == StripE::LINE ? ( cols[k].m_frac < 0.0f ? lineL : lineR )
                                                        : dash;
                glow = sf.m_lineGlow * 0.45f;
                gloss = 0.3f;
            }
            else if ( wear > 0.0f && cols[k].m_kind == StripE::ROAD )
            {
                // Patches: fresh black tar, or old asphalt bleached grey.
                const float h = hashFloat( seed * 19 + k, static_cast<uint64_t>( j / 2 ) );
                if ( h < 0.22f * wear ) base = scaleColor( base, h < 0.11f * wear ? 0.78f : 1.12f );
            }
            const Vector3 p[4] = { at( A, la0, 0 ), at( A, la1, 0 ), at( B, lb1, 0 ), at( B, lb0, 0 ) };
            const float   ls[4] = { lampLight( j, la0, 0 ), lampLight( j, la1, 0 ), lampLight( j, lb1, 1 ),
                                    lampLight( j, lb0, 1 ) };
            Color         col[4];
            Mat           mat[4];
            for ( int v = 0; v < 4; ++v )
            {
                const float g = v < 2 ? gA : gB;
                col[v] = lerpColor( lerpColor( base, dirt, g ), WarmLight, ls[v] * 0.5f );
                mat[v] = cobbled ? Mat{ -2.0f, 0.12f }
                                 : Mat{ glow * ( 1.0f - g ) + ls[v] * 0.22f, lerpf( gloss, 0.04f, g ) };
            }
            ground.quadV( p, col, mat );
        }

        // On the road: the joints of concrete slabs, potholes, and the studs along the lines.
        auto along = [&]( float t, float l, float h )
        {
            const Vector3 a = at( A, l, h ), b = at( B, l, h );
            return Vector3{ a.x + ( b.x - a.x ) * t, a.y + ( b.y - a.y ) * t, a.z + ( b.z - a.z ) * t };
        };
        if ( here.m_concrete && gA <= 0.0f && !bridge )
        {
            const Color joint = scaleColor( roadCol, 0.72f );
            ground.quad( along( 0.0f, -A.m_hw, 0.008f ), along( 0.0f, A.m_hw, 0.008f ), along( 0.03f, A.m_hw, 0.008f ),
                         along( 0.03f, -A.m_hw, 0.008f ), joint );
        }
        float holeX = 0.0f, holeR = 0.0f;
        if ( potholeAt( track, j, holeX, holeR ) )
        {
            const Color hole = scaleColor( roadCol, 0.42f );
            const float rt = holeR / SegmentLength, d = 0.72f;
            ground.quad( along( 0.5f - rt, holeX, 0.012f ), along( 0.5f, holeX + holeR, 0.012f ),
                         along( 0.5f + rt, holeX, 0.012f ), along( 0.5f, holeX - holeR, 0.012f ), hole );
            ground.quad( along( 0.5f - rt * d, holeX - holeR * d, 0.012f ),
                         along( 0.5f - rt * d, holeX + holeR * d, 0.012f ),
                         along( 0.5f + rt * d, holeX + holeR * d, 0.012f ),
                         along( 0.5f + rt * d, holeX - holeR * d, 0.012f ), hole );
        }
        if ( here.m_studs && marked && gA <= 0.0f && j % 2 == 0 )
        {
            // White between the lanes, red along the outer edge, amber along the centre side (and down a yellow middle).
            const Color white = { 236, 236, 230, 255 }, red = { 220, 40, 30, 255 }, amber = { 240, 160, 30, 255 };
            const float outer = land.m_left ? -1.0f : 1.0f;
            auto        stud = [&]( float l, Color col )
            {
                ground.quad( along( 0.0f, l - 0.07f, 0.02f ), along( 0.0f, l + 0.07f, 0.02f ),
                             along( 0.03f, l + 0.07f, 0.02f ), along( 0.03f, l - 0.07f, 0.02f ), col,
                             Mat{ 0.7f, 0.5f } );
            };
            stud( outer * ( A.m_hw - 0.35f ), red );
            stud( -outer * ( A.m_hw - 0.35f ), amber );
            for ( int k = 1; k < lanes; ++k )
            {
                if ( fork.m_exists && k * 2 == lanes && u > -120.0 && u < ForkMedian ) continue;
                stud( ( -1.0f + 2.0f * static_cast<float>( k ) / static_cast<float>( lanes ) ) * A.m_hw,
                      between && k * 2 == lanes ? amber : white );
            }
        }

        // Kerbs and shoulders on both sides.
        for ( float side : { -1.0f, 1.0f } )
        {
            Color kerb = j % 2 == 0 ? sf.m_rumbleA : sf.m_rumbleB;
            if ( wear > 0.6f && !here.m_kerbs &&
                 hashFloat( seed * 41 + ( side > 0.0f ), static_cast<uint64_t>( j ) ) < wear - 0.5f )
                kerb = scaleColor( roadCol, 0.9f );
            const float l0a = side * A.m_hw, l1a = side * ( A.m_hw + RumbleWidth ), l2a = side * A.m_edge;
            const float l0b = side * B.m_hw, l1b = side * ( B.m_hw + RumbleWidth ), l2b = side * B.m_edge;
            {
                const Vector3 p[4] = { at( A, l0a, 0.02f ), at( A, l1a, 0.02f ), at( B, l1b, 0.02f ),
                                       at( B, l0b, 0.02f ) };
                const Color   ka = lerpColor( kerb, dirt, gA ), kb = lerpColor( kerb, dirt, gB );
                const Color   col[4] = { ka, ka, kb, kb };
                const Mat     m = { sf.m_lineGlow * 0.35f, 0.2f };
                const Mat     mat[4] = { m, m, m, m };
                ground.quadV( p, col, mat );
            }
            if ( bridge )
            {
                const Color fascia = scaleColor( bridgeBiome( seg ).m_bridgeColor, 0.9f );
                ground.quad( at( A, l1a, 0.0f ), at( B, l1b, 0.0f ), at( B, l1b, -1.6f ), at( A, l1a, -1.6f ), fascia );
                continue;
            }
            const int   s = side < 0.0f ? 0 : 1;
            const float paveWidth = landRow( j ).m_pave[s];
            if ( paveWidth > 0.0f )
            {
                // A town's street: a parking lane at the road's level, a kerb, and a pavement up to the houses.
                const float   ls[4] = { lampLight( j, l1a, 0 ), lampLight( j, l2a, 0 ), lampLight( j, l2b, 1 ),
                                        lampLight( j, l1b, 1 ) };
                const Vector3 lane[4] = { at( A, l1a, 0.0f ), at( A, l2a, 0.0f ), at( B, l2b, 0.0f ), at( B, l1b, 0.0f ) };
                Color         col[4];
                Mat           mat[4];
                for ( int v = 0; v < 4; ++v )
                {
                    col[v] = lerpColor( scaleColor( roadCol, 1.05f ), WarmLight, ls[v] * 0.5f );
                    mat[v] = Mat{ ls[v] * 0.22f, 0.16f };
                }
                ground.quadV( lane, col, mat );
                ground.quad( along( 0.0f, l1a, 0.02f ), along( 0.0f, l2a, 0.02f ), along( 0.025f, l2a, 0.02f ),
                             along( 0.025f, l1a, 0.02f ), Color{ 226, 226, 220, 255 }, Mat{ 0.0f, 0.3f } );
                const float l3a = l2a + side * paveWidth, l3b = l2b + side * paveWidth;
                auto        top = [&]( const Cross &X, long jj, float l )
                {
                    Vector3 p = at( X, l, 0.0f );
                    p.y = slabY( landRow( jj ), l );
                    return p;
                };
                auto foot = [&]( const Cross &X, long jj, float l )
                {
                    Vector3 p = at( X, l, 0.0f );
                    p.y = std::min( rowY( landRow( jj ), l ), p.y ) - 0.05f;
                    return p;
                };
                // The kerb in the country's paint: black and yellow in South Asia and Arabia, black and white in
                // Southeast Asia and Africa, yellow in Latin America, bare stone elsewhere.
                const FleetE kerbRegion = countryInfo( ruling( seg, 0.5f ).m_country ).m_fleet;
                const bool   kerbCountry = ruling( seg, 0.5f ).m_country != nullptr;
                Color        kerbStone = { 168, 166, 160, 255 };
                if ( kerbCountry && ( kerbRegion == FleetE::SOUTH_ASIA || kerbRegion == FleetE::ARABIA ) )
                    kerbStone = j % 2 ? Color{ 236, 196, 40, 255 } : Color{ 30, 30, 32, 255 };
                else if ( kerbCountry && ( kerbRegion == FleetE::SOUTHEAST_ASIA || kerbRegion == FleetE::AFRICA ) )
                    kerbStone = j % 2 ? Color{ 236, 236, 230, 255 } : Color{ 30, 30, 32, 255 };
                else if ( kerbCountry && kerbRegion == FleetE::LATIN )
                    kerbStone = Color{ 232, 196, 60, 255 };
                const Color paving = scaleColor( Color{ 184, 180, 172, 255 }, band ? 1.0f : 0.95f );
                ground.quad( foot( A, j, l2a ), foot( B, j + 1, l2b ), top( B, j + 1, l2b ), top( A, j, l2a ), kerbStone );
                const Vector3 slab[4] = { top( A, j, l2a ), top( A, j, l3a ), top( B, j + 1, l3b ), top( B, j + 1, l2b ) };
                const float   lp[4] = { lampLight( j, l2a, 0 ), lampLight( j, l3a, 0 ), lampLight( j, l3b, 1 ),
                                        lampLight( j, l2b, 1 ) };
                for ( int v = 0; v < 4; ++v )
                {
                    col[v] = lerpColor( paving, WarmLight, lp[v] * 0.4f );
                    mat[v] = Mat{ lp[v] * 0.15f, 0.05f };
                }
                ground.quadV( slab, col, mat );
                ground.quad( top( A, j, l3a ), top( B, j + 1, l3b ), foot( B, j + 1, l3b ), foot( A, j, l3a ),
                             scaleColor( kerbStone, 0.85f ) );
                continue;
            }
            {
                const float   ha = landRow( j ).m_height[s][0];
                const float   hb = landRow( j + 1 ).m_height[s][0];
                const Vector3 p[4] = { at( A, l1a, 0.0f ), at( A, l2a, ha ), at( B, l2b, hb ), at( B, l1b, 0.0f ) };
                const float   ls[4] = { lampLight( j, l1a, 0 ), lampLight( j, l2a, 0 ), lampLight( j, l2b, 1 ),
                                        lampLight( j, l1b, 1 ) };
                Color         col[4];
                Mat           mat[4];
                for ( int v = 0; v < 4; ++v )
                {
                    col[v] = lerpColor( sf.m_shoulder, WarmLight, ls[v] * 0.4f );
                    mat[v] = Mat{ ls[v] * 0.15f, 0.0f };
                }
                ground.quadV( p, col, mat );
            }
        }

        // The land: stripes near the road, patchy colour further out, ponds in the marsh; above the tree line rock, above
        // the snow line snow, where the slope lets it lie.
        for ( float side : { -1.0f, 1.0f } )
        {
            const int      s = side < 0.0f ? 0 : 1;
            const LandRow &ra = landRow( j ), &rb = landRow( j + 1 );
            for ( int k = 0; k + 1 < LandRows; ++k )
            {
                const float la0 = ra.m_lateral[s][k], la1 = ra.m_lateral[s][k + 1];
                const float lb0 = rb.m_lateral[s][k], lb1 = rb.m_lateral[s][k + 1];
                const float h[4] = { ra.m_height[s][k], ra.m_height[s][k + 1], rb.m_height[s][k + 1],
                                     rb.m_height[s][k] };
                const float mid = ( LandOffsets[k] + LandOffsets[k + 1] ) * 0.5f;
                const Color stripe = band ? sf.m_groundA : sf.m_groundB;
                const float patch =
                    hashFloat( seed + static_cast<uint64_t>( k * 2 + ( side > 0 ) ), static_cast<uint64_t>( j / 3 ) );
                Color col = lerpColor( stripe, sf.m_groundFar, smoothstepf( 20.0f, 110.0f, mid ) );
                col = scaleColor( col, 0.92f + 0.16f * patch );
                if ( sf.m_fields > 0.0f && k >= 1 && k <= 6 )
                {
                    // Fields in blocks of 50 m, rows every 10 m.
                    const uint64_t cell = static_cast<uint64_t>( k + ( side > 0 ) * 11 );
                    if ( hashFloat( seed + 333 + cell, static_cast<uint64_t>( j / 10 ) ) < sf.m_fields )
                    {
                        const float pick = hashFloat( seed + 444 + cell, static_cast<uint64_t>( j / 10 ) );
                        Color       field = pick < 0.4f ? sf.m_fieldA : pick < 0.75f ? sf.m_fieldB : sf.m_fieldC;
                        if ( ( j / 2 ) % 2 ) field = scaleColor( field, 0.86f );
                        col = lerpColor( col, field, 0.9f );
                    }
                }
                // Steep faces show bare rock.
                const float run = std::max( 1.0f, std::abs( la1 - la0 ) );
                const float slope = std::max( std::abs( h[1] - h[0] ), std::abs( h[2] - h[3] ) ) / run;
                if ( slope > 0.7f ) col = lerpColor( col, sf.m_shoulder, smoothstepf( 0.7f, 1.4f, slope ) * 0.85f );
                // High up rock and snow, and snow where the road runs between banks of it.
                col = highTone( col, j,
                                static_cast<float>( c.m_origin.m_y ) + ( at( A, la0, h[0] ).y + at( A, la1, h[1] ).y +
                                                                          at( B, lb1, h[2] ).y + at( B, lb0, h[3] ).y ) *
                                                                            0.25f,
                                slope );
                if ( pondAt( j, s, k ) )
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
        // The sea: left, right or both (sinking away while a crossfade removes it). Pale and green over the shallows,
        // deepening away from the shore; where it meets the land, a line of surf.
        for ( float side : { -1.0f, 1.0f } )
        {
            const float w = side < 0 ? A.m_terrain.m_waterLeft : A.m_terrain.m_waterRight;
            if ( w <= 0.05f ) continue;
            const int   s = side < 0.0f ? 0 : 1;
            const float y = A.m_terrain.m_seaLevel - ( 1.0f - w ) * 20.0f, ya = y + A.m_seg->m_trench,
                        yb = y + B.m_seg->m_trench;
            // Where the land as drawn goes under the water, on either row (14 m out where it does not).
            auto shore = [&]( const LandRow &row, float level )
            {
                for ( int k = 0; k + 1 < LandRows; ++k )
                    if ( row.m_height[s][k] > level && row.m_height[s][k + 1] <= level )
                        return lerpf( std::abs( row.m_lateral[s][k] ), std::abs( row.m_lateral[s][k + 1] ),
                                      ( row.m_height[s][k] - level ) /
                                          std::max( 0.01f, row.m_height[s][k] - row.m_height[s][k + 1] ) );
                return row.m_cross.m_edge + 14.0f;
            };
            const float wa = shore( landRow( j ), ya ), wb = shore( landRow( j + 1 ), yb );
            const Color shallow = lerpColor( sf.m_water, Color{ 190, 240, 228, 255 }, 0.4f );
            const Color deep = lerpColor( scaleColor( sf.m_water, 0.62f ), Color{ 16, 40, 96, 255 }, 0.25f );
            const float outs[4] = { 0.0f, 30.0f, 140.0f, 1200.0f };
            const Color tones[4] = { shallow, lerpColor( shallow, sf.m_water, 0.6f ), sf.m_water, deep };
            const Mat   water = { sf.m_waterGlow, sf.m_waterGloss };
            const Mat   waters[4] = { water, water, water, water };
            for ( int k = 0; k < 3; ++k )
            {
                const float a1 = A.m_edge + 14.0f + outs[k + 1], b1 = B.m_edge + 14.0f + outs[k + 1];
                const float a0 = k == 0 ? std::min( wa, a1 - 1.0f ) : A.m_edge + 14.0f + outs[k];
                const float b0 = k == 0 ? std::min( wb, b1 - 1.0f ) : B.m_edge + 14.0f + outs[k];
                const Vector3 q[4] = { at( A, side * a0, ya ), at( A, side * a1, ya ), at( B, side * b1, yb ),
                                       at( B, side * b0, yb ) };
                const Color   col[4] = { tones[k], tones[k + 1], tones[k + 1], tones[k] };
                ground.quadV( q, col, waters );
            }
            if ( w > 0.95f && !tunnel )
            {
                // The surf: white on the beach's edge, thinning out over the first metres of water; broken here and there.
                const float broken = hashFloat( seed + 47 + static_cast<uint64_t>( s ), static_cast<uint64_t>( j ) );
                const Color foam = { 244, 248, 250, 255 };
                const float in = -0.7f, mid = 0.6f + 0.8f * broken, out = 2.2f + 1.6f * broken;
                const Mat   froth = { 0.0f, 0.25f };
                const Mat   froths[4] = { froth, froth, froth, froth };
                const Vector3 q0[4] = { at( A, side * ( wa + in ), ya + 0.04f ), at( A, side * ( wa + mid ), ya + 0.04f ),
                                        at( B, side * ( wb + mid ), yb + 0.04f ), at( B, side * ( wb + in ), yb + 0.04f ) };
                const Color   c0[4] = { foam, foam, foam, foam };
                ground.quadV( q0, c0, froths );
                const Vector3 q1[4] = { at( A, side * ( wa + mid ), ya + 0.04f ), at( A, side * ( wa + out ), ya + 0.04f ),
                                        at( B, side * ( wb + out ), yb + 0.04f ), at( B, side * ( wb + mid ), yb + 0.04f ) };
                const Color   c1[4] = { foam, shallow, shallow, foam };
                ground.quadV( q1, c1, froths );
            }
        }

        if ( tunnel )
        {
            const float wa = A.m_hw + RumbleWidth + 0.6f, wb = B.m_hw + RumbleWidth + 0.6f;
            const Color concrete = { 140, 138, 132, 255 };
            const Color skirt = { 90, 92, 96, 255 };
            props.setShade( 0.1f );
            const Vector3 a[6] = { at( A, -wa, 0.0f ),        at( A, -wa, 5.0f ), at( A, -wa * 0.55f, 7.2f ),
                                   at( A, wa * 0.55f, 7.2f ), at( A, wa, 5.0f ),  at( A, wa, 0.0f ) };
            const Vector3 b[6] = { at( B, -wb, 0.0f ),        at( B, -wb, 5.0f ), at( B, -wb * 0.55f, 7.2f ),
                                   at( B, wb * 0.55f, 7.2f ), at( B, wb, 5.0f ),  at( B, wb, 0.0f ) };
            for ( int k = 0; k < 5; ++k )
                props.quad( a[k], b[k], b[k + 1], a[k + 1], k == 2 ? scaleColor( concrete, 0.8f ) : concrete );
            for ( float side : { -1.0f, 1.0f } )
                props.quad( at( A, side * wa * 0.99f, 0.0f ), at( B, side * wb * 0.99f, 0.0f ),
                            at( B, side * wb * 0.99f, 1.0f ), at( A, side * wa * 0.99f, 1.0f ), skirt );
            // Lights every 10 m on both slopes: they rush past.
            if ( j % 2 == 0 )
            {
                for ( float side : { -1.0f, 1.0f } )
                {
                    const Vector3 p0 = Vector3Lerp( at( A, side * wa, 5.0f ), at( A, side * wa * 0.55f, 7.2f ), 0.5f );
                    const Vector3 p1 = Vector3Lerp( at( B, side * wb, 5.0f ), at( B, side * wb * 0.55f, 7.2f ), 0.5f );
                    const Vector3 q0 = Vector3Lerp( p0, p1, 0.1f ), q1 = Vector3Lerp( p0, p1, 0.4f );
                    const Vector3 up = { 0.0f, 0.18f, 0.0f };
                    props.quad( Vector3Subtract( q0, up ), Vector3Subtract( q1, up ), Vector3Add( q1, up ),
                                Vector3Add( q0, up ), Color{ 255, 196, 120, 255 }, Mat{ 1.0f, 0.0f } );
                }
            }
            props.setShade( 1.0f );
            // The hill's lid over the tube, and the portals.
            const float ea = A.m_edge + 6.0f, eb = B.m_edge + 6.0f;
            const Color lid = sf.m_groundFar;
            props.quad( at( A, -ea, TunnelLift ), at( B, -eb, TunnelLift ), at( B, eb, TunnelLift ),
                        at( A, ea, TunnelLift ), lid );
            const bool entry = !( track.segment( j - 1 ).m_flags & SEG_TUNNEL );
            const bool exit = !( track.segment( j + 1 ).m_flags & SEG_TUNNEL );
            for ( int end = 0; end < 2; ++end )
            {
                if ( ( end == 0 && !entry ) || ( end == 1 && !exit ) ) continue;
                const Cross &X = end == 0 ? A : B;
                const float  wx = X.m_hw + RumbleWidth + 0.6f, ex = X.m_edge + 6.0f;
                const Color  face = { 172, 170, 162, 255 };
                props.quad( at( X, -ex, 0.0f ), at( X, -wx, 0.0f ), at( X, -wx, TunnelLift ), at( X, -ex, TunnelLift ),
                            face );
                props.quad( at( X, wx, 0.0f ), at( X, ex, 0.0f ), at( X, ex, TunnelLift ), at( X, wx, TunnelLift ),
                            face );
                props.quad( at( X, -wx, 7.2f ), at( X, wx, 7.2f ), at( X, wx, TunnelLift ), at( X, -wx, TunnelLift ),
                            face );
                props.tri( at( X, -wx, 5.0f ), at( X, -wx * 0.55f, 7.2f ), at( X, -wx, 7.2f ), face );
                props.tri( at( X, wx, 5.0f ), at( X, wx, 7.2f ), at( X, wx * 0.55f, 7.2f ), face );
                props.quad( at( X, -wx - 0.6f, 7.2f ), at( X, wx + 0.6f, 7.2f ), at( X, wx + 0.6f, 8.2f ),
                            at( X, -wx - 0.6f, 8.2f ), Color{ 120, 118, 112, 255 } );
            }
        }
        if ( seg.m_gallery != 0 && !tunnel && !bridge )
        {
            // An avalanche gallery: a concrete roof from the mountain's wall over the road onto pillars on the valley
            // side, the slope going on over it.
            const float m = static_cast<float>( seg.m_gallery );
            const float wa = A.m_hw + RumbleWidth + 0.8f, wb = B.m_hw + RumbleWidth + 0.8f;
            const Color concrete = { 152, 150, 142, 255 }, under = { 110, 108, 102, 255 };
            props.setShade( 0.45f );
            props.quad( at( A, m * wa, 0.0f ), at( B, m * wb, 0.0f ), at( B, m * wb, 6.0f ), at( A, m * wa, 6.0f ),
                        concrete );
            props.quad( at( A, m * wa, 6.0f ), at( B, m * wb, 6.0f ), at( B, -m * ( wb + 0.6f ), 5.6f ),
                        at( A, -m * ( wa + 0.6f ), 5.6f ), under );
            props.setShade( 1.0f );
            props.quad( at( A, m * ( wa + 5.0f ), 9.5f ), at( B, m * ( wb + 5.0f ), 9.5f ),
                        at( B, -m * ( wb + 0.6f ), 6.4f ), at( A, -m * ( wa + 0.6f ), 6.4f ), sf.m_groundFar );
            props.quad( at( A, -m * ( wa + 0.6f ), 5.6f ), at( B, -m * ( wb + 0.6f ), 5.6f ),
                        at( B, -m * ( wb + 0.6f ), 6.4f ), at( A, -m * ( wa + 0.6f ), 6.4f ), concrete );
            if ( j % 2 == 0 )
                obox( props, at( A, -m * ( wa + 0.25f ), 2.8f ), A.m_right, A.m_forward, 0.3f, 2.8f, 0.45f, concrete );
        }
        {
            // Banks of snow along the road in winter and spring where the place clears it through deep snow: a foot of
            // slush, a cut face that leans back and bulges here and there, a rounded shoulder, an uneven crest, and a slope
            // down to the land behind. Their height and their bulges change slowly along the road, not from segment to
            // segment.
            const BiomeDef &place = bridgeBiome( seg );
            const bool      snowy = A.m_season.m_winter > 0.25f || A.m_season.m_spring > 0.35f;
            if ( place.m_snowWalls > 0.0f && snowy && !tunnel && !bridge && seg.m_gallery == 0 )
            {
                const Color slush = { 198, 206, 222, 255 }, face = { 230, 236, 247, 255 }, crest = { 248, 250, 254, 255 };
                const Mat   sparkle = { 0.0f, 0.3f };
                const Mat   mats[4] = { sparkle, sparkle, sparkle, sparkle };
                for ( float side : { -1.0f, 1.0f } )
                {
                    const uint64_t key = seed + 91 + ( side > 0.0f ? 7u : 0u );
                    constexpr int  Points = 7, Steps = 2;
                    Vector3        ring[Steps + 1][Points];
                    for ( int i = 0; i <= Steps; ++i )
                    {
                        const float  t = static_cast<float>( i ) / static_cast<float>( Steps );
                        const double s = sj + static_cast<double>( t * SegmentLength );
                        const float  high = place.m_snowWalls * ( 0.7f + 0.5f * valueNoise( s * 0.03, side * 1.7, key ) +
                                                                 0.14f * valueNoise( s * 0.23, side * 5.1, key + 1 ) );
                        auto         bulge = [&]( int level )
                        { return 0.5f * ( valueNoise( s * 0.17, level * 3.3 + side, key + 2 ) - 0.5f ); };
                        const float in = lerpf( A.m_hw, B.m_hw, t ) + RumbleWidth + 0.4f;
                        float       back = lerpf( A.m_edge, B.m_edge, t ) + 2.5f + 0.5f * high;   // where the crest ends
                        float       out = back + 1.6f * high;   // where the slope meets the land
                        // In a serpentine the land ends where the next leg's begins.
                        const float reach = side < 0.0f ? seg.m_reachL : seg.m_reachR;
                        if ( reach > 0.0f )
                        {
                            out = std::min( out, reach - 1.0f );
                            back = std::min( back, out - 0.5f );
                        }
                        ring[i][0] = along( t, side * in, 0.0f );
                        ring[i][1] = along( t, side * ( in + 0.35f ), 0.12f * high );
                        ring[i][2] = along( t, side * ( in + 0.6f + bulge( 0 ) ), 0.55f * high );
                        ring[i][3] = along( t, side * ( in + 1.1f + bulge( 1 ) ), 0.9f * high );
                        ring[i][4] = along( t, side * ( in + 2.1f + bulge( 2 ) ), high );
                        ring[i][5] = along( t, side * back, high + 0.5f * bulge( 3 ) );
                        ring[i][6] = along( t, side * out, 0.0f );
                        ring[i][6].y = groundY( j, std::min( t, 0.999f ), side * out ) - 0.2f;
                    }
                    const Color tones[Points] = { slush, slush, face, face, crest, crest, crest };
                    for ( int i = 0; i < Steps; ++i )
                        for ( int k = 0; k + 1 < Points; ++k )
                        {
                            const Vector3 q[4] = { ring[i][k], ring[i + 1][k], ring[i + 1][k + 1], ring[i][k + 1] };
                            const Color   col[4] = { tones[k], tones[k], tones[k + 1], tones[k + 1] };
                            props.quadV( q, col, mats );
                        }
                    // Run into, the snow turns the car away like a rail.
                    c.m_colliders.push_back( { sj, side * ( A.m_hw + RumbleWidth + 1.4f ), 1.0f, true } );
                }
            }
        }
        if ( runwayHere )
        {
            // The runway the low planes land on (Spectacle.h): across the road from the crossing, its threshold
            // painted, lights along its edges; on the other side the approach lights on their poles, a tower beyond.
            const float s0 = runwaySide;
            auto        rw = [&]( float l, float f, float h )
            { return Vector3Add( at( A, l, h ), Vector3Scale( A.m_forward, f ) ); };
            const Color tarmac = { 74, 74, 80, 255 }, paint = { 236, 236, 232, 255 };
            for ( int k = 0; k < 12; ++k )
            {
                const float l0 = s0 * ( A.m_edge + RunwayNear + RunwayLength * static_cast<float>( k ) / 12.0f );
                const float l1 = s0 * ( A.m_edge + RunwayNear + RunwayLength * static_cast<float>( k + 1 ) / 12.0f );
                const float h = ( rowY( landRow( j ), ( l0 + l1 ) * 0.5f ) - at( A, ( l0 + l1 ) * 0.5f, 0.0f ).y ) + 0.08f;
                props.quad( rw( l0, -RunwayHalf, h ), rw( l1, -RunwayHalf, h ), rw( l1, RunwayHalf, h ),
                            rw( l0, RunwayHalf, h ), tarmac, Mat{ 0.0f, 0.15f } );
                props.quad( rw( l0 + s0 * 10.0f, -0.6f, h + 0.02f ), rw( l0 + s0 * 34.0f, -0.6f, h + 0.02f ),
                            rw( l0 + s0 * 34.0f, 0.6f, h + 0.02f ), rw( l0 + s0 * 10.0f, 0.6f, h + 0.02f ), paint );
                for ( float f : { -RunwayHalf - 0.8f, RunwayHalf + 0.8f } )
                    props.box( rw( l0, f, h + 0.25f ), Vector3{ 0.25f, 0.25f, 0.25f }, Color{ 255, 236, 180, 255 },
                               Mat{ 1.0f, 0.0f } );
                if ( k == 0 )
                    for ( float f : { -18.0f, -14.0f, -10.0f, -6.0f, 6.0f, 10.0f, 14.0f, 18.0f } )
                        props.quad( rw( l0 + s0 * 6.0f, f - 1.0f, h + 0.02f ),
                                    rw( l0 + s0 * 36.0f, f - 1.0f, h + 0.02f ),
                                    rw( l0 + s0 * 36.0f, f + 1.0f, h + 0.02f ),
                                    rw( l0 + s0 * 6.0f, f + 1.0f, h + 0.02f ), paint );
            }
            for ( int k = 0; k < 10; ++k )
            {
                const float l = -s0 * ( A.m_edge + 20.0f + 30.0f * static_cast<float>( k ) );
                const float h = ( rowY( landRow( j ), l ) - at( A, l, 0.0f ).y );
                const float top = 3.0f + 0.5f * static_cast<float>( k );
                props.cylinder( rw( l, 0.0f, h ), 0.12f, 0.1f, top, 6, Color{ 210, 120, 40, 255 } );
                props.box( rw( l, 0.0f, h + top ), Vector3{ 0.3f, 0.2f, 0.3f }, Color{ 255, 250, 230, 255 },
                           Mat{ 1.0f, 0.0f } );
                for ( float f : { -4.0f, -2.0f, 2.0f, 4.0f } )
                    props.box( rw( l, f, h + top ), Vector3{ 0.2f, 0.15f, 0.2f }, Color{ 255, 240, 200, 255 },
                               Mat{ 1.0f, 0.0f } );
            }
            const float tl = s0 * ( A.m_edge + 220.0f ), th = ( rowY( landRow( j ), tl ) - at( A, tl, 0.0f ).y );
            props.cylinder( rw( tl, 160.0f, th ), 2.4f, 2.0f, 26.0f, 10, Color{ 226, 224, 218, 255 } );
            props.cylinder( rw( tl, 160.0f, th + 26.0f ), 3.6f, 4.2f, 4.0f, 10, Color{ 60, 90, 120, 255 },
                            Mat{ 0.2f, 0.9f } );
            props.cylinder( rw( tl, 160.0f, th + 30.0f ), 4.4f, 3.0f, 1.2f, 10, Color{ 200, 200, 196, 255 } );
            obox( props, rw( s0 * ( A.m_edge + 330.0f ), 220.0f, th + 6.0f ), A.m_right, A.m_forward, 30.0f, 6.0f,
                  90.0f, Color{ 214, 216, 220, 255 } );
            for ( float side : { -1.0f, 1.0f } )
            {
                MeshBuilder sign;
                buildPlaceSign( sign, "LOW FLYING AIRCRAFT", 1, 0.32f, 1.6f );
                props.append(
                    sign, Vector3Add( at( A, side * ( A.m_edge + 2.4f ), 0.0f ), Vector3Scale( A.m_forward, -60.0f ) ),
                    A.m_heading, 1.0f );
            }
        }
        if ( bridge )
        {
            const BiomeDef &bd = bridgeBiome( seg );
            const Color     paint = bd.m_bridgeColor;
            const float     da = A.m_hw + RumbleWidth, db = B.m_hw + RumbleWidth;
            float           metres = ValleyDepth;
            const float     depth = valleyDepth( track, j, &metres ) * metres;
            const Mat       metal = { 0.0f, 0.45f };
            props.quad( at( A, -da, -1.6f ), at( B, -db, -1.6f ), at( B, db, -1.6f ), at( A, da, -1.6f ),
                        scaleColor( paint, 0.7f ) );
            // Where the bridge begins and ends (the towers and pylons stand by those).
            long b0 = j, b1 = j;
            track.generateTo( static_cast<double>( j + 420 ) * SegmentLength );
            while ( b0 > std::max( track.firstIndex(), j - 400 ) && ( track.segment( b0 - 1 ).m_flags & SEG_BRIDGE ) )
                --b0;
            while ( b1 < std::min( track.endIndex() - 2, j + 400 ) && ( track.segment( b1 + 1 ).m_flags & SEG_BRIDGE ) )
                ++b1;
            auto piers = [&]( float half, float inset, Color col )
            {
                const float h = ( depth + 2.4f ) * 0.5f;
                for ( float side : { -1.0f, 1.0f } )
                    obox( props, at( A, side * ( da - inset ), -1.6f - h ), A.m_right, A.m_forward, half, h, half,
                          col );
            };
            switch ( bd.m_bridge )
            {
            case BridgeE::VIADUCT:
                if ( j % 10 == 0 )
                {   // One tall slim pier with a hammerhead.
                    const float h = ( depth + 1.0f ) * 0.5f;
                    obox( props, at( A, 0.0f, -2.4f - h ), A.m_right, A.m_forward, 2.2f, h, 1.6f, paint );
                    obox( props, at( A, 0.0f, -2.0f ), A.m_right, A.m_forward, da, 0.4f, 1.8f,
                          scaleColor( paint, 0.9f ) );
                }
                break;
            case BridgeE::ARCH:
            {   // Stone arches of 20 m on piers, solid spandrels.
                const int   k = static_cast<int>( ( ( j % 4 ) + 4 ) % 4 );
                const float rise = 7.0f;
                auto        arch = [&]( float t ) { return -2.4f - rise * ( 1.0f - std::sin( 3.14159f * t ) ); };
                const float y0 = arch( k / 4.0f ), y1 = arch( ( k + 1 ) / 4.0f );
                for ( float side : { -1.0f, 1.0f } )
                    props.quad( at( A, side * da, -1.6f ), at( B, side * db, -1.6f ), at( B, side * db, y1 ),
                                at( A, side * da, y0 ), paint );
                props.quad( at( A, -da, y0 ), at( A, da, y0 ), at( B, db, y1 ), at( B, -db, y1 ),
                            scaleColor( paint, 0.55f ) );
                if ( k == 0 )
                {
                    const float top = -2.4f - rise, h = ( top + depth + 2.0f ) * 0.5f;
                    obox( props, at( A, 0.0f, top - h ), A.m_right, A.m_forward, da + 0.3f, h, 1.6f,
                          scaleColor( paint, 0.92f ) );
                }
                break;
            }
            case BridgeE::TRUSS:
            {   // A through truss on both sides, braced overhead.
                if ( j % 8 == 0 ) piers( 0.9f, 1.0f, scaleColor( paint, 0.8f ) );
                for ( float side : { -1.0f, 1.0f } )
                {
                    const float xa = side * ( da + 0.9f ), xb = side * ( db + 0.9f );
                    props.tube( at( A, xa, 6.5f ), at( B, xb, 6.5f ), 0.25f, 4, paint, metal );
                    props.tube( at( A, xa, 0.2f ), at( B, xb, 0.2f ), 0.22f, 4, paint, metal );
                    props.tube( at( A, xa, 0.2f ), at( A, xa, 6.5f ), 0.16f, 4, paint, metal );
                    if ( j % 2 == 0 )
                        props.tube( at( A, xa, 0.2f ), at( B, xb, 6.5f ), 0.12f, 4, paint, metal );
                    else
                        props.tube( at( A, xa, 6.5f ), at( B, xb, 0.2f ), 0.12f, 4, paint, metal );
                }
                if ( j % 2 == 0 )
                {
                    props.tube( at( A, -da - 0.9f, 6.5f ), at( A, da + 0.9f, 6.5f ), 0.14f, 4, paint, metal );
                    props.tube( at( A, -da - 0.9f, 6.5f ), at( B, db + 0.9f, 6.5f ), 0.08f, 4, paint, metal );
                }
                break;
            }
            case BridgeE::SUSPENSION:
            {   // Two towers, the main cables in a curve between them, hangers.
                const long  n = b1 - b0 + 1;
                const long  t1 = b0 + std::max( 2L, n / 5 ), t2 = b1 - std::max( 2L, n / 5 );
                const float top = 42.0f;
                auto        cable = [&]( long i )
                {
                    if ( i <= t1 )
                        return lerpf( 1.2f, top,
                                      static_cast<float>( i - b0 ) / static_cast<float>( std::max( 1L, t1 - b0 ) ) );
                    if ( i >= t2 )
                        return lerpf( top, 1.2f,
                                      static_cast<float>( i - t2 ) /
                                          static_cast<float>( std::max( 1L, b1 + 1 - t2 ) ) );
                    const float mid = 0.5f * static_cast<float>( t1 + t2 ), half = 0.5f * static_cast<float>( t2 - t1 );
                    const float u = ( static_cast<float>( i ) - mid ) / half;
                    return 2.5f + ( top - 2.5f ) * u * u;
                };
                for ( float side : { -1.0f, 1.0f } )
                {
                    const float xa = side * ( da + 1.3f ), xb = side * ( db + 1.3f );
                    props.tube( at( A, xa, cable( j ) ), at( B, xb, cable( j + 1 ) ), 0.3f, 5, paint, metal );
                    if ( cable( j ) > 2.6f )
                        props.tube( at( A, xa, 0.3f ), at( A, xa, cable( j ) ), 0.05f, 3, paint, metal );
                    props.quad( at( A, side * da, -1.6f ), at( B, side * db, -1.6f ), at( B, side * db, -3.4f ),
                                at( A, side * da, -3.4f ), scaleColor( paint, 0.75f ) );
                }
                if ( j == t1 || j == t2 )
                {
                    const float bottom = -depth - 2.0f, h = ( top + 3.0f - bottom ) * 0.5f;
                    for ( float side : { -1.0f, 1.0f } )
                        obox( props, at( A, side * ( da + 1.3f ), bottom + h ), A.m_right, A.m_forward, 1.1f, h, 1.5f,
                              paint );
                    for ( float y : { top * 0.55f, top + 1.5f } )
                        obox( props, at( A, 0.0f, y ), A.m_right, A.m_forward, da + 1.3f, 0.9f, 1.0f, paint );
                    obox( props, at( A, 0.0f, -3.0f ), A.m_right, A.m_forward, da + 1.3f, 0.9f, 1.2f,
                          scaleColor( paint, 0.85f ) );
                }
                break;
            }
            case BridgeE::CABLE_STAYED:
            {   // An A-shaped pylon every 180 m, a fan of stays to the deck edges.
                const long  n = b1 - b0 + 1, period = 36;
                const long  pylon = n < period ? ( b0 + b1 ) / 2
                                               : std::min( b1 - 4, b0 + ( ( j - b0 ) / period ) * period + period / 2 );
                const float top = 38.0f;
                const Cross P = cross( pylon );
                if ( j == pylon )
                {
                    const float bottom = -depth - 2.0f;
                    for ( float side : { -1.0f, 1.0f } )
                        props.tube( at( A, side * ( da + 2.0f ), bottom ), at( A, side * 0.7f, top ), 1.0f, 6, paint );
                    props.tube( at( A, -da - 1.4f, -2.0f ), at( A, da + 1.4f, -2.0f ), 0.8f, 6, paint );
                    props.tube( at( A, -1.0f, top - 1.0f ), at( A, 1.0f, top - 1.0f ), 0.9f, 6, paint );
                }
                const long d = std::abs( j - pylon );
                if ( d >= 2 && j % 2 == 0 )
                    for ( float side : { -1.0f, 1.0f } )
                        props.tube( at( P, side * 0.9f, top - 3.0f - 0.25f * static_cast<float>( std::min( d, 30L ) ) ),
                                    at( A, side * ( da + 0.6f ), 0.4f ), 0.07f, 3, scaleColor( paint, 1.05f ), metal );
                break;
            }
            case BridgeE::COVERED:
            {   // Plank walls and a shingled roof; stone abutments at the ends.
                const Color roof = { 70, 58, 52, 255 };
                for ( float side : { -1.0f, 1.0f } )
                {
                    const float xa = side * ( da + 0.7f ), xb = side * ( db + 0.7f );
                    props.quad( at( A, xa, 0.0f ), at( B, xb, 0.0f ), at( B, xb, 4.4f ), at( A, xa, 4.4f ), paint );
                    props.quad( at( A, xa, 4.4f ), at( B, xb, 4.4f ), at( B, 0.0f, 6.6f ), at( A, 0.0f, 6.6f ), roof );
                }
                for ( long end : { b0, b1 } )
                    if ( j == end )
                    {
                        const Cross &E = end == b0 ? A : B;
                        props.tri( at( E, -da - 0.7f, 4.4f ), at( E, da + 0.7f, 4.4f ), at( E, 0.0f, 6.6f ),
                                   scaleColor( paint, 0.85f ) );
                        const float h = ( depth + 2.0f ) * 0.5f;
                        obox( props, at( E, 0.0f, -1.6f - h ), E.m_right, E.m_forward, da + 1.0f, h, 1.4f,
                              Color{ 140, 134, 124, 255 } );
                    }
                break;
            }
            case BridgeE::CAUSEWAY:
                if ( j % 3 == 0 ) piers( 0.45f, 1.2f, scaleColor( paint, 0.85f ) );
                break;
            default:
                if ( j % 6 == 0 )
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
        for ( float side : { -1.0f, 1.0f } )
        {
            if ( !( seg.m_flags & ( side < 0 ? SEG_RAIL_L : SEG_RAIL_R ) ) ) continue;
            const float   la = side * ( A.m_hw + RumbleWidth + 0.55f ), lb = side * ( B.m_hw + RumbleWidth + 0.55f );
            const BridgeE kind = bridge ? bridgeBiome( seg ).m_bridge : BridgeE::BEAM;
            if ( bridge && kind == BridgeE::COVERED ) continue;
            if ( bridge && kind == BridgeE::ARCH )
            {
                const Color stone = bridgeBiome( seg ).m_bridgeColor;
                const float lo = side * 0.5f;
                props.quad( at( A, la, 0.0f ), at( B, lb, 0.0f ), at( B, lb, 1.0f ), at( A, la, 1.0f ),
                            scaleColor( stone, 0.9f ) );
                props.quad( at( A, la, 1.0f ), at( B, lb, 1.0f ), at( B, lb + lo, 1.0f ), at( A, la + lo, 1.0f ),
                            scaleColor( stone, 1.05f ) );
                props.quad( at( A, la + lo, -1.6f ), at( B, lb + lo, -1.6f ), at( B, lb + lo, 1.0f ),
                            at( A, la + lo, 1.0f ), stone );
                continue;
            }
            if ( here.m_walls )
            {
                const Color concrete = { 172, 170, 162, 255 };
                const float lo = side * 0.3f;
                props.quad( at( A, la, 0.0f ), at( B, lb, 0.0f ), at( B, lb, 3.0f ), at( A, la, 3.0f ), concrete );
                props.quad( at( A, la, 3.0f ), at( B, lb, 3.0f ), at( B, lb + lo, 3.0f ), at( A, la + lo, 3.0f ),
                            scaleColor( concrete, 1.1f ) );
                props.quad( at( A, la + lo, 0.0f ), at( B, lb + lo, 0.0f ), at( B, lb + lo, 3.0f ),
                            at( A, la + lo, 3.0f ), scaleColor( concrete, 0.8f ) );
                if ( j % 2 == 0 )
                {
                    const Vector3 base = at( A, la - side * 0.02f, 0.0f );
                    props.box( Vector3{ base.x, base.y + 1.5f, base.z }, Vector3{ 0.06f, 1.5f, 0.06f },
                               scaleColor( concrete, 0.7f ) );
                }
                continue;
            }
            const Color steel = { 176, 180, 190, 255 };
            props.quad( at( A, la, 0.45f ), at( B, lb, 0.45f ), at( B, lb, 0.82f ), at( A, la, 0.82f ), steel,
                        Mat{ 0.0f, 0.6f } );
            props.quad( at( A, la + side * 0.12f, 0.82f ), at( B, lb + side * 0.12f, 0.82f ), at( B, lb, 0.82f ),
                        at( A, la, 0.82f ), scaleColor( steel, 0.8f ) );
            const Vector3 base = at( A, la + side * 0.15f, 0.0f );
            props.box( Vector3{ base.x, base.y + 0.4f, base.z }, Vector3{ 0.07f, 0.4f, 0.07f },
                       Color{ 120, 124, 132, 255 } );
        }

        // On the sea floor: steel ribs of a glass tube over the road, rails along it, a light in every crown, a portal
        // where it begins and ends.
        {
            const bool under = ruling( seg, 0.5f ).m_underwater;
            const bool before = ruling( track.segment( j - 1 ), 0.5f ).m_underwater,
                       after = ruling( track.segment( j + 1 ), 0.5f ).m_underwater;
            if ( under )
            {
                const float radius = A.m_hw + RumbleWidth + 2.6f;
                const Color steel = { 70, 86, 96, 255 }, rail = { 110, 128, 138, 255 };
                const int   n = 10;
                auto        arc = [&]( const Cross &x, int k )
                {
                    const float th = Tau * 0.5f * static_cast<float>( k ) / static_cast<float>( n );
                    return at( x, -std::cos( th ) * radius, 0.2f + std::sin( th ) * radius * 0.82f );
                };
                if ( j % 3 == 0 )
                {
                    for ( int k = 0; k < n; ++k ) props.tube( arc( A, k ), arc( A, k + 1 ), 0.16f, 4, steel );
                    const Vector3 top = arc( A, n / 2 );
                    props.box( Vector3{ top.x, top.y - 0.25f, top.z }, Vector3{ 0.35f, 0.08f, 0.35f },
                               Color{ 200, 240, 255, 255 }, Mat{ 1.0f, 0.0f } );
                }
                for ( int k : { 1, 3, 5, 7, 9 } ) props.tube( arc( A, k ), arc( B, k ), 0.06f, 3, rail );
                // Tinted glass between the ribs, drawn see-through after everything solid.
                const Color tint = { 46, 110, 124, 255 };
                for ( int k = 0; k < n; ++k )
                    glass.quad( arc( A, k ), arc( B, k ), arc( B, k + 1 ), arc( A, k + 1 ), tint, Mat{ 0.0f, 0.9f } );
                if ( !before || !after )
                {
                    // The portal: a thick concrete ring, set in a wall up to the land above the cutting.
                    const Color  concrete = { 168, 170, 168, 255 };
                    const Cross &P = before ? B : A;
                    for ( int k = 0; k < n; ++k ) props.tube( arc( P, k ), arc( P, k + 1 ), 0.9f, 5, concrete );
                    const float top = SeaDipDepth + 1.5f, wide = radius + 7.0f;
                    auto        outer = [&]( int k )
                    {
                        const float th = Tau * 0.5f * static_cast<float>( k ) / static_cast<float>( n );
                        return at( P, -std::cos( th ) * wide, 0.2f + std::sin( th ) * top );
                    };
                    for ( int k = 0; k < n; ++k )
                    {
                        props.quad( arc( P, k ), outer( k ), outer( k + 1 ), arc( P, k + 1 ),
                                    scaleColor( concrete, 0.9f ) );
                        props.quad( arc( P, k + 1 ), outer( k + 1 ), outer( k ), arc( P, k ),
                                    scaleColor( concrete, 0.9f ) );
                    }
                }
            }
        }

        // The cutting down to an undersea tunnel: concrete walls hold the land back.
        {
            const float ta = A.m_seg->m_trench, tb = B.m_seg->m_trench;
            if ( ta > 0.05f || tb > 0.05f )
            {
                const Color wall = { 172, 172, 166, 255 };
                for ( float side : { -1.0f, 1.0f } )
                {
                    const float   la = side * ( A.m_edge + 2.5f ), lb = side * ( B.m_edge + 2.5f );
                    const Vector3 a0 = at( A, la, -0.5f ), a1 = at( A, la, ta + 0.7f ), b0 = at( B, lb, -0.5f ),
                                  b1 = at( B, lb, tb + 0.7f );
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
        if ( xc.m_type != CrossingE::NONE )
        {
            const bool         ferry = xc.m_type == CrossingE::FERRY;
            const double       before = xc.m_board - sj, after = sj - xc.m_land;
            const BiomeDef    &dest = biomeDef( track.stageBiome( track.stageAt( xc.m_land ) ) );
            const CountryInfo &nation = countryInfo( here.m_country );
            const float        driver = nation.m_left ? -1.0f : 1.0f;
            for ( double at0 : { 600.0, 260.0 } )
                if ( before > at0 - SegmentLength && before <= at0 )
                {
                    MeshBuilder sign;
                    buildPlaceSign(
                        sign, std::string( ferry ? "FERRY TO " : "CAR TRAIN TO " ) + ( dest.m_name ? dest.m_name : "" ),
                        3, 0.5f, 2.2f );
                    const float l = driver * ( A.m_edge + 3.0f );
                    props.append( sign, onGround( j, 0.0f, l, 0.0f, 0.0f ), A.m_heading, 1.0f );
                    c.m_colliders.push_back( { sj, l, 0.6f } );
                }
            const int canopy = landmarkByName( "TERMINAL_CANOPY" );
            if ( canopy >= 0 && before > 180.0 - SegmentLength && before <= 180.0 )
                props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( canopy ) ), onGround( j, 0.0f, 0.0f, 0.0f, 0.0f ),
                              A.m_heading, 1.0f );
            if ( ferry && before > 0.0 && before <= SegmentLength )
                ground.quad( at( B, -1500.0f, -6.0f ), at( B, 1500.0f, -6.0f ), at( B, 1500.0f, 0.0f ),
                             at( B, -1500.0f, 0.0f ), Color{ 150, 150, 146, 255 } );
            if ( ferry && after >= 0.0 && after < SegmentLength )
                ground.quad( at( A, -1500.0f, -6.0f ), at( A, 1500.0f, -6.0f ), at( A, 1500.0f, 0.0f ),
                             at( A, -1500.0f, 0.0f ), Color{ 150, 150, 146, 255 } );
        }

        // The country's road customs: its speed limit after the place sign and before a speed camera, the camera, a
        // toll plaza across the road.
        {
            const int          stage = track.stageAt( static_cast<double>( j ) * SegmentLength );
            const long         stageSegs = track.stageSegments( stage ), local = j - track.stageSegment( stage );
            const BiomeDef    &place = biomeDef( track.stageBiome( stage ) );
            const RoadCustoms &roads = roadCustoms( place.m_country );
            const float        driver = countryInfo( place.m_country ).m_left ? -1.0f : 1.0f;
            double             cs = 0.0, ts = 0.0;
            int                limit = 0;
            bool               mph = false;
            const bool         camera = local < stageSegs / 2 && stageCamera( track, stage, cs, limit, mph );
            const bool         sign = ( local == 40 && place.m_country && !place.m_underwater && !tunnel && !bridge &&
                                        xc.m_type == CrossingE::NONE ) ||
                                      ( camera && j == static_cast<long>( ( cs - LimitAt ) / SegmentLength ) );
            if ( sign )
            {
                MeshBuilder m;
                buildSpeedSign( m,
                                camera && j == static_cast<long>( ( cs - LimitAt ) / SegmentLength )
                                    ? limit
                                    : speedLimit( seg, roads.m_mph ),
                                roads.m_board );
                const float l = driver * ( A.m_edge + 1.6f );
                props.append( m, onGround( j, 0.0f, l, 0.0f, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { sj, l, 0.3f } );
            }
            if ( camera && j == static_cast<long>( cs / SegmentLength ) )
            {
                const float l = driver * ( A.m_edge + 1.8f );
                props.append( models.m_camera, onGround( j, 0.0f, l, 0.0f, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { sj, l, 0.4f } );
            }
            if ( local < stageSegs / 2 && stageToll( track, stage, ts ) &&
                 j == static_cast<long>( ts / SegmentLength ) )
            {
                // A canopy over the road, booths on islands between the lanes, their barriers up.
                const Color   white = { 236, 236, 230, 255 }, stripe = { 30, 90, 170, 255 },
                              concrete = { 176, 174, 166, 255 };
                const Vector3 mid = at( A, 0.0f, 0.0f );
                obox( props, Vector3{ mid.x, mid.y + 5.8f, mid.z }, A.m_right, A.m_forward, A.m_hw + 3.0f, 0.3f, 6.0f,
                      white );
                obox( props, Vector3{ mid.x, mid.y + 6.15f, mid.z }, A.m_right, A.m_forward, A.m_hw + 3.05f, 0.06f,
                      6.05f, stripe );
                for ( int k = 0; k <= lanes; ++k )
                {
                    const float   x = -A.m_hw + 2.0f * A.m_hw * static_cast<float>( k ) / static_cast<float>( lanes );
                    const Vector3 p = at( A, x, 0.0f );
                    obox( props, Vector3{ p.x, p.y + 0.12f, p.z }, A.m_right, A.m_forward, 0.45f, 0.12f, 4.5f,
                          concrete );
                    obox( props, Vector3{ p.x, p.y + 1.35f, p.z }, A.m_right, A.m_forward, 0.36f, 1.15f, 1.1f, white );
                    obox( props, Vector3{ p.x, p.y + 3.6f, p.z + 0.0f }, A.m_right, A.m_forward, 0.1f, 2.2f, 0.1f,
                          Color{ 150, 150, 156, 255 } );
                    if ( k < lanes )
                        props.tube( at( A, x + 0.4f, 1.1f ), at( A, x + 0.7f, 3.4f ), 0.05f, 4,
                                    Color{ 220, 40, 40, 255 } );
                    c.m_colliders.push_back( { sj, x, 0.5f } );
                }
                MeshBuilder board;
                buildPlaceSign( board, "TOLL", 3, 0.7f, 4.2f, false );
                props.append( board, Vector3{ mid.x - A.m_forward.x * 6.1f, mid.y, mid.z - A.m_forward.z * 6.1f },
                              A.m_heading, 1.0f );
            }
        }

        // A festival: strings of flags, lanterns or lights across the road, little lamps along it, stalls, tents and
        // Christmas trees beside it.
        const int fest = tunnel || bridge || fork.m_exists || xc.m_type != CrossingE::NONE
                             ? -1
                             : festivalAt( static_cast<BiomeE>( seg.m_blend < 0.5f ? seg.m_biome : seg.m_biomeTo ),
                                           m_seasons.dateAt( sj ) );
        if ( fest >= 0 )
        {
            const Festival           &f = festivals()[static_cast<size_t>( fest )];
            const std::vector<Color> &string = !f.m_lanterns.empty()  ? f.m_lanterns
                                               : !f.m_bunting.empty() ? f.m_bunting
                                                                      : f.m_lights;
            if ( !string.empty() && j % 14 == 0 )
            {
                const float w = A.m_edge + 1.5f;
                for ( float side : { -1.0f, 1.0f } )
                    props.cylinder( at( A, side * w, 0.0f ), 0.09f, 0.07f, 7.0f, 5, Color{ 92, 92, 98, 255 } );
                const int n = 26;
                Vector3   last = at( A, -w, 6.8f );
                for ( int k = 1; k <= n; ++k )
                {
                    const float   x = -w + 2.0f * w * static_cast<float>( k ) / static_cast<float>( n ), t = x / w;
                    const Vector3 p = at( A, x, 6.8f - 1.5f * ( 1.0f - t * t ) );
                    const Color   col = string[static_cast<size_t>( k ) % string.size()];
                    props.tube( last, p, 0.025f, 3, Color{ 40, 40, 44, 255 } );
                    if ( !f.m_lanterns.empty() )
                        props.blob( Vector3{ p.x, p.y - 0.5f, p.z }, Vector3{ 0.28f, 0.36f, 0.28f }, col,
                                    static_cast<unsigned>( k ), 0.0f, Mat{ 0.8f, 0.0f } );
                    else if ( !f.m_bunting.empty() )
                        props.tri(
                            last, p,
                            Vector3{ ( p.x + last.x ) * 0.5f, ( p.y + last.y ) * 0.5f - 0.6f, ( p.z + last.z ) * 0.5f },
                            col );
                    else
                        props.blob( Vector3{ p.x, p.y - 0.12f, p.z }, Vector3{ 0.08f, 0.08f, 0.08f }, col,
                                    static_cast<unsigned>( k ), 0.0f, Mat{ 1.0f, 0.0f } );
                    last = p;
                }
            }
            if ( !f.m_lamps.empty() && j % 2 == 0 )
                for ( float side : { -1.0f, 1.0f } )
                    props.blob( at( A, side * ( A.m_edge + 0.4f ), 0.12f ), Vector3{ 0.13f, 0.1f, 0.13f }, f.m_lamps[0],
                                static_cast<unsigned>( j ), 0.0f, Mat{ 1.0f, 0.0f } );
            // Beside the road, facing it, on alternate sides.
            auto beside =
                [&]( const char *shape, const std::vector<Color> &paint, long every, float away, float radius )
            {
                const int v = landmarkByName( shape );
                if ( v < 0 || j % every != 0 ) return;
                const float side = ( j / every ) % 2 ? 1.0f : -1.0f, l = side * ( A.m_edge + away );
                const Color keys[3] = { paint.size() > 0 ? paint[0] : Color{ 176, 32, 32, 255 },
                                        paint.size() > 1 ? paint[1] : Color{ 242, 238, 228, 255 },
                                        !f.m_lights.empty() ? f.m_lights[0] : Color{ 255, 224, 160, 255 } };
                const float h = rowY( landRow( j ), l ) - at( A, l, 0.0f ).y;   // on the ground as drawn
                SeasonLook  snow = A.m_season;
                snow.m_autumn = snow.m_bare = snow.m_spring = 0.0f;
                props.append( models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( v ) ), at( A, l, h ),
                              A.m_heading + ( side > 0.0f ? 0.0f : Tau * 0.5f ), 1.0f, 1.0f, keys, &snow );
                c.m_colliders.push_back( { sj, l, radius } );
            };
            if ( !f.m_stalls.empty() ) beside( "MARKET_STALL", f.m_stalls, 7, 5.0f, 2.0f );
            if ( !f.m_tents.empty() ) beside( "BEER_TENT", f.m_tents, 31, 16.0f, 8.0f );
            if ( f.m_trees ) beside( "CHRISTMAS_TREE", {}, 23, 9.0f, 1.5f );
        }

        // The fork: a barrier between the carriageways, a cushion at its nose, the gantry with both places; after it
        // the other carriageway turns away into the fields.
        if ( fork.m_exists )
        {
            const Color concrete = { 196, 194, 186, 255 };
            if ( seg.m_flags & SEG_MEDIAN )
            {
                const Vector3 a0 = at( A, 0.0f, 0.0f ), b0 = at( B, 0.0f, 0.0f );
                const Vector3 mid = Vector3Lerp( a0, b0, 0.5f );
                obox( props, Vector3{ mid.x, mid.y + 0.42f, mid.z }, A.m_right, A.m_forward, 0.28f, 0.42f,
                      SegmentLength * 0.52f, concrete );
                if ( u < SegmentLength )
                {
                    const Color   yellow = { 250, 200, 30, 255 }, black = { 24, 24, 26, 255 };
                    const Vector3 nose = Vector3Add( a0, Vector3Scale( A.m_forward, 1.6f ) );
                    obox( props, Vector3{ nose.x, nose.y + 0.6f, nose.z }, A.m_right, A.m_forward, 0.7f, 0.6f, 1.6f,
                          yellow );
                    for ( int k = 0; k < 3; ++k )
                        obox( props,
                              Vector3{ a0.x - A.m_forward.x * 0.02f, a0.y + 0.3f + 0.4f * k,
                                       a0.z - A.m_forward.z * 0.02f },
                              A.m_right, A.m_forward, 0.72f, 0.08f, 0.05f, black );
                }
            }
            if ( ( u >= -285.0 && u < -285.0 + SegmentLength ) || ( u >= -780.0 && u < -780.0 + SegmentLength ) )
            {
                std::string left, right;
                forkNames( fork.m_left, fork.m_right, track.stageBiome( seg.m_stage ), left, right );
                MeshBuilder sign;
                buildForkSign( sign, left, right, A.m_hw );
                props.append( sign, onGround( j, 0.0f, 0.0f, 0.0f, 0.0f ), A.m_heading, 1.0f );
            }
            if ( peeling )
            {
                // The other carriageway, a little above the land, its kerbs along it.
                const double ua = u, ub = u + SegmentLength;
                const float  xa = branchX( ua ), xb = branchX( ub );
                const Color  kerb = j % 2 == 0 ? sf.m_rumbleA : sf.m_rumbleB;
                const float  lift = 0.04f;
                auto         strip = [&]( float a0, float a1, float b0, float b1, Color col )
                { ground.quad( at( A, a0, lift ), at( A, a1, lift ), at( B, b1, lift ), at( B, b0, lift ), col ); };
                strip( xa - carriage, xa + carriage, xb - carriage, xb + carriage, band ? sf.m_road : sf.m_roadAlt );
                strip( xa - carriage - RumbleWidth, xa - carriage, xb - carriage - RumbleWidth, xb - carriage, kerb );
                strip( xa + carriage, xa + carriage + RumbleWidth, xb + carriage, xb + carriage + RumbleWidth, kerb );
                if ( band ) strip( xa - 0.08f, xa + 0.08f, xb - 0.08f, xb + 0.08f, sf.m_line );
            }
        }

        // Furniture and props, per side.
        for ( float side : { -1.0f, 1.0f } )
        {
            const uint64_t     sideKey = side < 0 ? 0x1111ull : 0x2222ull;
            const BiomeDef    &rule = ruling( seg, hashFloat( seed ^ sideKey, static_cast<uint64_t>( j ) ) );
            const bool         railed = seg.m_flags & ( side < 0 ? SEG_RAIL_L : SEG_RAIL_R );
            const CountryInfo &signs = countryInfo( rule.m_country );

            // The place's name at the start of its stage, a border post where the country changes.
            const long stage = track.stageAt( static_cast<double>( j ) * SegmentLength ),
                       local = j - track.stageSegment( static_cast<int>( stage ) );
            if ( local >= 3 && local <= 7 && !tunnel && !bridge )
            {
                const BiomeDef    &place = biomeDef( track.stageBiome( static_cast<int>( stage ) ) );
                const CountryInfo &nation = countryInfo( place.m_country );
                const float        driver = nation.m_left ? -1.0f : 1.0f;
                const BiomeDef    *last =
                    stage > 0 ? &biomeDef( track.stageBiome( static_cast<int>( stage - 1 ) ) ) : nullptr;
                const char *before = last ? last->m_country : nullptr;
                const bool  border = place.m_country && before && std::strcmp( place.m_country, before ) != 0;
                // A state or province line inside a country: its name on a board.
                const bool regionBorder = !border && place.m_region && last &&
                                          ( !last->m_region || std::strcmp( place.m_region, last->m_region ) != 0 );
                if ( regionBorder && local == 5 && side == driver )
                {
                    MeshBuilder sign;
                    buildPlaceSign( sign, place.m_region, 4, 0.5f, 2.0f );
                    props.append( sign, onGround( j, 0.0f, side * ( A.m_edge + 3.0f ), 0.0f, 0.0f ), A.m_heading, 1.0f );
                    c.m_colliders.push_back(
                        { static_cast<double>( j ) * SegmentLength, side * ( A.m_edge + 3.0f ), 0.6f } );
                }
                if ( local == 7 && side == driver && place.m_name )
                {
                    MeshBuilder sign;
                    buildPlaceSign( sign, place.m_name, nation.m_townSign, 0.42f, 1.4f, true,
                                    scriptName( track.stageBiome( static_cast<int>( stage ) ) ) );
                    const float l = side * ( A.m_edge + 2.4f );
                    props.append( sign, onGround( j, 0.0f, l, 0.0f, 0.0f ), A.m_heading, 1.0f );
                    c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.6f } );
                }
                if ( border && local == 3 && side == driver )
                {
                    const float l = side * ( A.m_edge + 2.0f );
                    props.append( models.m_borderPost, onGround( j, 0.0f, l, 0.0f, 0.0f ), A.m_heading + ( side < 0 ? Tau * 0.5f : 0.0f ),
                                  1.0f );
                    c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 1.4f } );
                }
                if ( border && local == 5 )
                {
                    // The new country's flag and name on the driver's side, the old country's flag across the road.
                    const int   flag = countryIndex( side == driver ? place.m_country : before );
                    const float l = side * ( A.m_edge + 6.0f );
                    if ( flag >= 0 )
                        props.append( models.m_flags[static_cast<size_t>( flag )], onGround( j, 0.0f, l, 0.0f, 0.0f ), A.m_heading,
                                      1.0f );
                    if ( side == driver )
                    {
                        MeshBuilder sign;
                        buildPlaceSign( sign, place.m_country, 3, 0.6f, 2.4f );
                        props.append( sign, onGround( j, 0.0f, side * ( A.m_edge + 3.0f ), 0.0f, 0.0f ), A.m_heading, 1.0f );
                        c.m_colliders.push_back(
                            { static_cast<double>( j ) * SegmentLength, side * ( A.m_edge + 3.0f ), 0.6f } );
                    }
                }
            }

            // The hairpins of a famous pass are numbered on the outside, counting down to the top, where its name and
            // height stand.
            if ( seg.m_hairpin > 0 && side * track.segment( j + 4 ).m_curve < 0.0f )
            {
                MeshBuilder sign;
                buildPlaceSign( sign,
                                ( rule.m_hairpinWord ? std::string( rule.m_hairpinWord ) + " " : std::string() ) +
                                    std::to_string( seg.m_hairpin ),
                                0, 0.42f, 1.2f );
                const float l = side * ( A.m_edge + 1.8f );
                props.append( sign, onGround( j, 0.0f, l, 0.0f, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.5f } );
            }
            if ( ( seg.m_flags & SEG_SUMMIT ) && rule.m_summit && side == ( signs.m_left ? -1.0f : 1.0f ) )
            {
                MeshBuilder sign;
                buildPlaceSign( sign,
                                std::string( rule.m_summit ) + "  " + std::to_string( rule.m_summitHeight ) + " M",
                                signs.m_townSign, 0.5f, 1.8f );
                const float l = side * ( A.m_edge + 2.6f );
                props.append( sign, onGround( j, 0.0f, l, 0.0f, 0.0f ), A.m_heading, 1.0f );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.6f } );
            }

            const bool forkClear =
                ( seg.m_flags & SEG_FORK ) && ( u < 250.0 || u > ForkMedian - 150.0 );   // the gore and the peel
            if ( rule.m_postSpacing > 0.0f && !railed && !tunnel && !bridge && !forkClear &&
                 pavementAt( track, models, seed, j, side ).m_width <= 0.0f )
            {
                const long k = std::max( 2L, static_cast<long>( rule.m_postSpacing / SegmentLength + 0.5f ) );
                if ( ( j + ( side < 0 ? k / 2 : 0 ) ) % k == 0 )
                {
                    // The reflectors differ between the traffic's own side and the other.
                    const bool   keepLeft = rule.m_country && countryInfo( rule.m_country ).m_left;
                    const size_t far = ( side < 0 ) == keepLeft ? 0 : 1;
                    props.append(
                        models.m_post[std::min<size_t>( rule.m_postStyle, PostSchemes * PostBodies - 1 ) * 2 + far],
                        onGround( j, 0.0f, side * ( A.m_hw + RumbleWidth + 0.7f ), 0.0f, 0.0f ), A.m_heading, 1.0f );
                }
            }
            if ( rule.m_lampSpacing > 0.0f && !tunnel && !bridge && !forkClear )
            {
                const long k = std::max( 2L, static_cast<long>( rule.m_lampSpacing / SegmentLength + 0.5f ) );
                if ( j % k == ( side > 0 ? 0 : k / 2 ) )
                {
                    const float l = side * ( A.m_hw + RumbleWidth + 1.0f );
                    props.append( models.m_lamp[std::min<size_t>( rule.m_lampTone, 2 )], onGround( j, 0.0f, l, 0.0f, 0.0f ),
                                  A.m_heading + ( side > 0 ? -0.5f : 0.5f ) * Tau * 0.5f, 1.0f );
                    c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.35f } );
                }
            }
            // The poles of the overhead lines beside the country road (data/roadside.txt), every 60 m on the right.
            if ( rule.m_poleKind > 0 && side > 0 && !tunnel && !bridge && !forkClear &&
                 j % std::max( 2L, static_cast<long>( 60.0f / SegmentLength + 0.5f ) ) == 0 )
            {
                const float l = A.m_hw + RumbleWidth + 5.5f;
                props.append( models.m_utilityPole[std::min<size_t>( rule.m_poleKind - 1, 2 )],
                              onGround( j, 0.0f, l, 0.0f, -0.1f ), A.m_heading, 1.0f );
            }
            // The wires between the poles: from the insulators of this segment's end to the next one's, sagging between
            // two poles; a pair more on the pole where the data says the line carries six conductors or more.
            if ( rule.m_poleKind > 0 && side > 0 && !tunnel && !bridge && !forkClear )
            {
                const long  span = std::max( 2L, static_cast<long>( 60.0f / SegmentLength + 0.5f ) );
                const float u0 = static_cast<float>( j % span ) / static_cast<float>( span ),
                            u1 = static_cast<float>( j % span + 1 ) / static_cast<float>( span );
                const float lA = A.m_hw + RumbleWidth + 5.5f, lB = B.m_hw + RumbleWidth + 5.5f;
                const float hA = rowY( landRow( j ), lA ) - at( A, lA, 0.0f ).y - 0.1f;
                const float hB = rowY( landRow( j + 1 ), lB ) - at( B, lB, 0.0f ).y - 0.1f;
                // Where the insulators of the three pole models sit: across the road and above the ground.
                static const float arms[3][4] = { { -1.1f, -0.4f, 0.4f, 1.1f },
                                                  { -1.0f, 0.0f, 1.0f, 0.0f },
                                                  { -1.4f, -0.7f, 0.7f, 1.4f } };
                static const float tops[3] = { 8.7f, 9.57f, 10.56f };
                static const int   counts[3] = { 4, 3, 4 };
                const size_t       kind = std::min<size_t>( rule.m_poleKind - 1, 2 );
                auto               wire = [&]( float x, float y )
                {
                    const float sag0 = 3.6f * u0 * ( 1.0f - u0 ), sag1 = 3.6f * u1 * ( 1.0f - u1 );
                    props.tube( at( A, lA + x, hA + y - sag0 ), at( B, lB + x, hB + y - sag1 ), 0.012f, 3,
                                Color{ 34, 34, 38, 255 } );
                };
                for ( int w = 0; w < counts[kind]; ++w ) wire( arms[kind][w], tops[kind] );
                if ( rule.m_poleWires >= 6 )
                    for ( float x : { -0.25f, 0.25f } ) wire( x, tops[kind] - 1.3f );
            }
            const bool chevron = seg.m_flags & ( side < 0 ? SEG_CHEVRON_L : SEG_CHEVRON_R );
            if ( chevron && j % 2 == 0 && !tunnel && !bridge )
            {
                const float  l = side * ( A.m_edge + 1.4f );
                const size_t style = std::min<size_t>( signs.m_chevrons, 2 );
                props.append( side < 0 ? models.m_chevronRight[style] : models.m_chevronLeft[style], onGround( j, 0.0f, l, 0.0f, 0.0f ),
                              A.m_heading, 1.0f );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, l, 0.5f } );
            }
            if ( ( seg.m_flags & SEG_JUMP ) && !tunnel )
                props.append( models.m_jumpSign[std::min<size_t>( signs.m_warn, 3 )],
                              onGround( j, 0.0f, side * ( A.m_edge + 1.2f ), 0.0f, 0.0f ), A.m_heading, 1.0f );

            // Scattered props; the buildings of a town or a city on plots in rows back from the road, in the country in a
            // village here and there.
            const Plots plots = plotsOf( rule, models );
            const bool  built = plots.m_count > 0 && ( plots.m_city || inVillage( seed, j ) );
            const long  plotSegs = std::max( 2L, std::lround( plots.m_along / SegmentLength ) ), plot = j / plotSegs;
            const bool  wet =
                ( side < 0.0f ? A.m_terrain.m_waterLeft : A.m_terrain.m_waterRight ) > 0.5f;   // no plots in the sea
            const float reachHere = side < 0.0f ? seg.m_reachL : seg.m_reachR;
            const bool  forkLand = fork.m_exists && u > ForkMedian - 60.0 && u < ForkMedian + ForkPeel + 40.0;
            const bool  green = sf.m_groundA.g > sf.m_groundA.r + 10;

            // A town's street: cars parked along it (not where a lamp stands or a cross street meets it), and at a cross
            // street the street itself, a zebra across the road and, on the driver's side, the lights.
            const Pavement pave = pavementAt( track, models, seed, j, side );
            const bool     street = pave.m_width > 0.0f && !wet && !tunnel && !bridge && seg.m_gallery == 0 &&
                                !forkLand && !viaduct && !hasRunway && reachHere <= 0.0f;
            auto lampAt = [&]( long jj )
            {
                const BiomeDef &d = ruling( track.segment( jj ), hashFloat( seed ^ sideKey, static_cast<uint64_t>( jj ) ) );
                if ( d.m_lampSpacing <= 0.0f ) return false;
                const long k = std::max( 2L, static_cast<long>( d.m_lampSpacing / SegmentLength + 0.5f ) );
                return jj % k == ( side > 0 ? 0 : k / 2 );
            };
            if ( street && !pave.m_crossing && !railed && !models.m_parked.empty() &&
                 hashFloat( seed + 61 + sideKey, static_cast<uint64_t>( j ) ) < 0.3f && !lampAt( j ) &&
                 !lampAt( j + 1 ) && !pavementAt( track, models, seed, j - 1, side ).m_crossing &&
                 !pavementAt( track, models, seed, j + 1, side ).m_crossing )
            {
                // Parked the way the traffic on that side goes: an everyday car, or as often what the country drives (a
                // scooter, a tuk-tuk, a pickup). Run into, it turns the car away like a rail: no crash.
                const bool     leftHand = signs.m_left;
                const uint64_t pick = hash64( seed + 63 + static_cast<uint64_t>( j * 2 + ( side > 0 ) ) );
                const uint32_t fleetBit = 1u << static_cast<int>( signs.m_fleet );
                size_t         locals = 0, everyday = 0;
                for ( const ParkedModel &pm : models.m_parked )
                {
                    if ( pm.m_fleets == 0 )
                        ++everyday;
                    else if ( pm.m_fleets & fleetBit )
                        ++locals;
                }
                const bool regional = locals > 0 && rule.m_country && ( ( pick >> 20 ) & 1 );
                size_t     nth = ( pick >> 4 ) % std::max<size_t>( 1, regional ? locals : everyday ), k = 0;
                for ( size_t i = 0; i < models.m_parked.size(); ++i )
                {
                    const ParkedModel &pm = models.m_parked[i];
                    if ( regional ? !( pm.m_fleets != 0 && ( pm.m_fleets & fleetBit ) ) : pm.m_fleets != 0 ) continue;
                    k = i;
                    if ( nth-- == 0 ) break;
                }
                const ParkedModel &car = models.m_parked[k];
                const float        l = side * ( A.m_hw + RumbleWidth + 0.45f + car.m_width * 0.5f );
                const float        yawP = A.m_heading + ( ( side > 0.0f ) != leftHand ? 0.0f : Tau * 0.5f );
                const Vector3      spot = Vector3Lerp( at( A, l, 0.0f ), at( B, l, 0.0f ), 0.5f );
                scenery.append( car.m_full, spot, yawP, 1.0f );
                sceneryFar.append( car.m_far, spot, yawP, 1.0f );
                contactRing( j, 0.5f, l, yawP, Vector3{}, car.m_width * 0.5f, car.m_length * 0.5f, 0.6f, 110,
                             OnTheGround );
                c.m_colliders.push_back( { sj + 2.5, l, std::max( 0.3f, car.m_width * 0.4f ), true } );
            }
            if ( street && pave.m_crossing )
            {
                const float l0 = side * A.m_edge, l1 = side * ( A.m_edge + pave.m_width + 60.0f );
                groundStrip( props, j, 0.0f, 2.5f, l0, l1, 6.0f, scaleColor( sf.m_road, 1.02f ), Mat{ 0.0f, 0.16f } );
                const bool driver = ( side > 0.0f ) != signs.m_left;
                if ( driver )
                {
                    // The crossing just before the cross street, as the country paints it: two lines across the road
                    // in North America, yellow stripes in Latin America, white stripes elsewhere; in Britain and where
                    // its ways were kept an orange globe on a striped post at either end (a Belisha beacon).
                    const FleetE region = signs.m_fleet;
                    const bool   american = region == FleetE::NORTH_AMERICA, british = region == FleetE::BRITISH;
                    const Color  paint = region == FleetE::LATIN ? Color{ 240, 196, 40, 255 } : Color{ 236, 236, 230, 255 };
                    if ( american )
                        for ( float f : { -0.62f, -0.06f } )
                            ground.quad( along( f, -A.m_hw + 0.3f, 0.025f ), along( f, A.m_hw - 0.3f, 0.025f ),
                                         along( f + 0.06f, A.m_hw - 0.3f, 0.025f ), along( f + 0.06f, -A.m_hw + 0.3f, 0.025f ),
                                         paint, Mat{ 0.0f, 0.3f } );
                    else
                        for ( float x = -A.m_hw + 0.4f; x + 0.5f <= A.m_hw - 0.3f; x += 1.0f )
                            ground.quad( along( 0.0f, x, 0.025f ), along( 0.0f, x + 0.5f, 0.025f ),
                                         along( -0.5f, x + 0.5f, 0.025f ), along( -0.5f, x, 0.025f ), paint,
                                         Mat{ 0.0f, 0.3f } );
                    const float   lp = side * ( A.m_edge + 0.5f );
                    const Vector3 foot = onGround( j, 0.0f, lp, -1.5f, -0.05f );
                    MeshBuilder   g;
                    if ( british )
                    {
                        // The beacon stands for the lights; the far one across the road.
                        for ( int k = 0; k < 5; ++k )
                            g.cylinder( Vector3{ 0.0f, 0.5f * static_cast<float>( k ), 0.0f }, 0.07f, 0.07f, 0.5f, 6,
                                        k % 2 ? Color{ 236, 236, 232, 255 } : Color{ 24, 24, 26, 255 } );
                        g.blob( Vector3{ 0.0f, 2.72f, 0.0f }, Vector3{ 0.24f, 0.24f, 0.24f }, Color{ 255, 170, 30, 255 }, 7,
                                0.0f, Mat{ 0.9f, 0.3f } );
                        props.append( g, foot, A.m_heading, 1.0f );
                        props.append( g, onGround( j, 0.0f, -lp, -1.5f, -0.05f ), A.m_heading, 1.0f );
                        c.m_colliders.push_back( { sj - 1.5, lp, 0.3f } );
                        c.m_colliders.push_back( { sj - 1.5, -lp, 0.3f } );
                    }
                    else
                    {
                        // The lights, facing the traffic and showing green. North and Latin America: yellow heads on
                        // a long arm over the road. East Asia: heads lying on their side on the arm. South and
                        // Southeast Asia: a post striped black and yellow with an arm. Elsewhere: black heads with a
                        // white border on a post at the kerb, one low and one high.
                        const bool  armed = american || region == FleetE::LATIN || region == FleetE::EAST_ASIA ||
                                           region == FleetE::SOUTH_ASIA || region == FleetE::SOUTHEAST_ASIA;
                        const bool  lying = region == FleetE::EAST_ASIA;
                        const bool  striped = region == FleetE::SOUTH_ASIA || region == FleetE::SOUTHEAST_ASIA;
                        const Color steel = lying ? Color{ 196, 198, 200, 255 } : Color{ 70, 74, 80, 255 };
                        const Color housing = american || region == FleetE::LATIN ? Color{ 232, 186, 36, 255 }
                                                                                   : Color{ 24, 26, 30, 255 };
                        const float reach = A.m_hw * 0.75f + RumbleWidth + 2.2f, top = armed ? 6.2f : 4.9f;
                        const Color lamps[3] = { { 120, 26, 22, 255 }, { 130, 96, 20, 255 }, { 70, 255, 150, 255 } };
                        const float glow[3] = { 0.08f, 0.08f, 1.0f };
                        if ( striped )
                            for ( int k = 0; k < 8; ++k )
                                g.cylinder( Vector3{ 0.0f, top * static_cast<float>( k ) / 8.0f, 0.0f }, 0.13f, 0.13f,
                                            top / 8.0f, 6, k % 2 ? Color{ 236, 196, 40, 255 } : Color{ 24, 24, 26, 255 } );
                        else
                            g.cylinder( Vector3{ 0.0f, 0.0f, 0.0f }, 0.13f, 0.11f, top, 8, steel );
                        if ( armed ) g.box( Vector3{ reach * 0.5f, 6.0f, 0.0f }, Vector3{ reach * 0.5f, 0.09f, 0.09f }, steel );
                        auto head = [&]( float x, float y, bool flat )
                        {
                            // The lamps one over the other, or side by side with the green one towards the kerb.
                            const Vector3 half = flat ? Vector3{ 0.55f, 0.2f, 0.16f } : Vector3{ 0.2f, 0.55f, 0.16f };
                            if ( !armed )
                                g.box( Vector3{ x, y, 0.01f }, Vector3{ half.x + 0.07f, half.y + 0.07f, 0.14f },
                                       Color{ 236, 236, 232, 255 } );
                            g.box( Vector3{ x, y, 0.0f }, half, housing );
                            for ( int k = 0; k < 3; ++k )
                            {
                                const float off = 0.34f - 0.34f * static_cast<float>( k );
                                g.box( Vector3{ x + ( flat ? off : 0.0f ), y + ( flat ? 0.0f : off ), -0.17f },
                                       Vector3{ 0.12f, 0.12f, 0.02f }, lamps[k], Mat{ glow[k], 0.4f } );
                            }
                        };
                        head( 0.0f, armed ? 3.2f : 2.7f, false );
                        if ( armed )
                            for ( float x : { reach * 0.62f, reach } ) head( x, 5.3f, lying );
                        else
                            head( 0.0f, 4.3f, false );
                        // The model's +x points from the post over the road.
                        props.appendBasis( g, Vector3Scale( A.m_right, -side ), Vector3{ 0.0f, 1.0f, 0.0f }, A.m_forward,
                                           foot );
                        c.m_colliders.push_back( { sj - 1.5, lp, 0.35f } );
                    }
                }
            }

            // A forest: where the place grows one and nothing is built, its trees close up some way off the road, in five
            // staggered rows of plain trees (two cones for a conifer, a pointed crown on a trunk for a broadleaf) in the
            // colours of the season, thinning out where the forest begins and ends.
            auto forestAt = [&]( long jj )
            { return 62.0f + 26.0f * valueNoise( static_cast<double>( jj ) * 0.06, side * 3.1, seed + 79 ); };
            // How much of a place's scattered props are trees, and how much of those conifers.
            auto treesOf = [&]( const BiomeDef &d, float &conifers )
            {
                float trees = 0.0f;
                conifers = 0.0f;
                for ( const PropRule &pr : d.m_props )
                    if ( isTree( pr.m_kind ) && !pr.m_onWater && pr.m_perStage == 0 )
                    {
                        trees += pr.m_chance;
                        if ( isConifer( pr.m_kind ) ) conifers += pr.m_chance;
                    }
                return trees;
            };
            // How dense the forest stands at this segment, 0..1 (0: none).
            float forest = 0.0f, conifers = 0.0f;
            const float trees = treesOf( ruling( seg, 0.5f ), conifers );
            if ( trees >= 0.45f && ruling( seg, 0.5f ).m_climate != ClimateE::ARID && !built && !wet && !tunnel &&
                 !bridge && !viaduct && !hasRunway && !forkLand && seg.m_gallery == 0 &&
                 !( reachHere > 0.0f && A.m_edge + forestAt( j ) + 34.0f > reachHere ) )
            {
                forest = smoothstepf( 0.3f, 0.52f, valueNoise( static_cast<double>( j ) * 0.02, side * 7.3, seed + 83 ) );
                // Towards the tree line the forest thins out.
                forest *= 1.0f - smoothstepf( landRow( j ).m_treeLine - 400.0f, landRow( j ).m_treeLine - 100.0f,
                                              static_cast<float>( c.m_origin.m_y ) + A.m_center.y );
            }
            const float forestD = forest > 0.0f ? forestAt( j ) : 0.0f;
            if ( forest > 0.0f )
            {
                const float coniferShare = conifers / trees;
                const float snowy = std::max( A.m_season.m_winter, 0.85f * landRow( j ).m_snowCover );
                const Color bark = { 84, 64, 46, 255 };
                static const Color autumn[3] = { { 206, 120, 44, 255 }, { 184, 70, 42, 255 }, { 220, 176, 62, 255 } };
                // A crown from a ring of corners up to a point and down to a point.
                auto crown = [&]( Vector3 at0, float radius, float up, float down, Color top, Color under, float turn )
                {
                    Vector3 ring[5];
                    for ( int k = 0; k < 5; ++k )
                    {
                        const float a = turn + Tau * static_cast<float>( k ) / 5.0f;
                        ring[k] = Vector3{ at0.x + std::cos( a ) * radius, at0.y, at0.z + std::sin( a ) * radius };
                    }
                    for ( int k = 0; k < 5; ++k )
                    {
                        props.tri( ring[k], ring[( k + 1 ) % 5], Vector3{ at0.x, at0.y + up, at0.z }, top );
                        props.tri( ring[( k + 1 ) % 5], ring[k], Vector3{ at0.x, at0.y - down, at0.z }, under );
                    }
                };
                for ( int row = 0; row < 5; ++row )
                {
                    const uint64_t g = static_cast<uint64_t>( j ) * 8 + static_cast<uint64_t>( row );
                    const float    h1 = hashFloat( seed + 81 + sideKey, g ), h2 = hashFloat( seed + 82 + sideKey, g ),
                                   h3 = hashFloat( seed + 84 + sideKey, g ), h4 = hashFloat( seed + 86 + sideKey, g );
                    if ( h1 > 0.3f + 0.7f * forest ) continue;   // thinner where the forest begins and ends
                    const float t = std::fmod( h2 + 0.37f * static_cast<float>( row ), 1.0f );
                    const float l = side * ( lerpf( A.m_edge, B.m_edge, t ) + lerpf( forestAt( j ), forestAt( j + 1 ), t ) +
                                             6.5f * static_cast<float>( row ) + 3.0f * ( h3 - 0.5f ) );
                    Vector3     foot = Vector3Lerp( at( A, l, 0.0f ), at( B, l, 0.0f ), t );
                    foot.y = groundY( j, t, l ) - 0.4f;
                    const bool  conifer = hashFloat( seed + 85 + sideKey, g ) < coniferShare;
                    const float tall = ( conifer ? 14.0f : 10.5f ) * ( 0.75f + 0.5f * h3 ) * ( 0.75f + 0.25f * forest );
                    const float shade = 0.84f + 0.3f * h4;
                    if ( conifer )
                    {
                        const Color fir = scaleColor( Color{ 30, 84, 46, 255 }, shade );
                        const Color tip = lerpColor( scaleColor( fir, 1.12f ), Snow, 0.75f * snowy );
                        const Color skirt = lerpColor( fir, Snow, 0.35f * snowy );
                        props.cylinder( Vector3{ foot.x, foot.y + tall * ( row < 2 ? 0.14f : 0.0f ), foot.z }, tall * 0.25f,
                                        0.0f, tall * 0.56f, 5, skirt, Mat{}, h2 * Tau );
                        if ( row < 3 )
                            props.cylinder( Vector3{ foot.x, foot.y + tall * 0.46f, foot.z }, tall * 0.17f, 0.0f,
                                            tall * 0.54f, 5, tip, Mat{}, h4 * Tau );
                    }
                    else
                    {
                        Color leaf = scaleColor( Color{ 62, 118, 50, 255 }, shade );
                        leaf = lerpColor( leaf, autumn[g % 3], 0.85f * A.m_season.m_autumn );
                        leaf = lerpColor( leaf, Color{ 116, 104, 92, 255 }, 0.7f * A.m_season.m_bare );
                        leaf = lerpColor( leaf, Color{ 132, 196, 96, 255 }, 0.3f * A.m_season.m_spring );
                        const Color top = lerpColor( scaleColor( leaf, 1.1f ), Snow, 0.6f * snowy );
                        crown( Vector3{ foot.x, foot.y + tall * 0.5f, foot.z }, tall * 0.34f, tall * 0.5f, tall * 0.24f, top,
                               scaleColor( leaf, 0.72f ), h2 * Tau );
                    }
                    if ( row < 2 )
                        props.tube( foot, Vector3{ foot.x, foot.y + tall * ( conifer ? 0.2f : 0.34f ), foot.z }, 0.26f, 3,
                                    bark );
                }
            }

            // A building: where its walls meet the ground, the lowest and the highest.
            auto footing = [&]( float dd, float yaw, Vector3 mid, Vector3 base, float t, float &lo, float &hi )
            {
                const float   l = side * ( A.m_edge + dd );
                const Vector3 L = Vector3Negate( headingRight( yaw ) ), F = headingForward( yaw );
                lo = 1e9f;
                hi = -1e9f;
                for ( float x : { -1.0f, 0.0f, 1.0f } )
                    for ( float z : { -1.0f, 0.0f, 1.0f } )
                    {
                        const float y = groundAt(
                            j, t, l,
                            Vector3Add( Vector3Scale( L, mid.x + x * base.x ), Vector3Scale( F, mid.z + z * base.z ) ) );
                        lo = std::min( lo, y );
                        hi = std::max( hi, y );
                    }
            };

            auto place = [&]( const PropRule &pr, uint64_t key, float dist, int plotRow )
            {
                const bool onPlot = plotRow >= 0;
                if ( pr.m_seasons && !( pr.m_seasons & seasonBit( A.m_season ) ) ) return;   // a stand out of season
                const float h2 = hashFloat( key + 2, static_cast<uint64_t>( j ) );
                const float h3 = hashFloat( key + 3, static_cast<uint64_t>( j ) );
                const float h4 = hashFloat( key + 4, static_cast<uint64_t>( j ) );
                const float waterSide = side < 0 ? A.m_terrain.m_waterLeft : A.m_terrain.m_waterRight;
                // Whether something may stand this far from the road here.
                auto clear = [&]( float d )
                {
                    if ( pr.m_onWater )
                    {
                        if ( waterSide < 0.5f || d < 35.0f ) return false;
                    }
                    else if ( waterSide > 0.5f && d > 12.0f && !pr.m_shore )
                    {
                        return false;
                    }
                    if ( railed && d < 2.0f ) return false;
                    if ( ( tunnel && d < 7.0f ) || ( bridge && d < 4.0f ) ) return false;
                    const float lateral = side * ( A.m_edge + d );
                    if ( viaduct && std::abs( lateral - railL ) < 7.0f ) return false;
                    if ( reachHere > 0.0f && std::abs( lateral ) > reachHere - 3.0f ) return false;   // the next leg's land
                    if ( hasRunway &&
                         std::abs( sj + static_cast<double>( h2 * SegmentLength ) - runwayS ) < RunwayHalf + 130.0 &&
                         lateral * runwaySide > A.m_edge + RunwayNear - 30.0f &&
                         std::abs( lateral ) < A.m_edge + RunwayNear + RunwayLength + 40.0f )
                        return false;   // the runway's ground
                    if ( hasRunway && std::abs( sj + static_cast<double>( h2 * SegmentLength ) - runwayS ) < 12.0 &&
                         lateral * runwaySide < 0.0f && std::abs( lateral ) < A.m_edge + 320.0f )
                        return false;   // the approach lights
                    // Nothing where the other carriageway of a fork turns away (it is decided before this is baked).
                    if ( forkLand && ( fork.m_chosen == 0 || side * static_cast<float>( -fork.m_chosen ) > 0.0f ) )
                    {
                        const float bx = branchX( std::clamp( u, ForkMedian, ForkMedian + ForkPeel ) );
                        if ( std::abs( lateral - bx ) < carriage + 14.0f || std::abs( lateral ) < std::abs( bx ) + 14.0f )
                            return false;
                    }
                    // Not in a pond.
                    if ( !pr.m_onWater )
                    {
                        const LandRow &w = landRow( j );
                        const int      s = side < 0.0f ? 0 : 1;
                        for ( int k = 1; k <= 4; ++k )
                            if ( std::abs( lateral ) >= std::abs( w.m_lateral[s][k] ) &&
                                 std::abs( lateral ) < std::abs( w.m_lateral[s][k + 1] ) && pondAt( j, s, k ) )
                                return false;
                    }
                    return true;
                };
                // What reaches into the water stands where the land meets it, and only where that is not far below
                // the road (not under a cliff).
                const float shoreLevel = A.m_terrain.m_seaLevel + A.m_seg->m_trench;
                if ( pr.m_shore )
                {
                    if ( waterSide < 0.5f || shoreLevel < -8.0f ) return;
                    const LandRow &row = landRow( j );
                    const int      s = side < 0.0f ? 0 : 1;
                    float          edge = -1.0f;   // none: the water is not drawn here (a fork, a runway, a tunnel)
                    for ( int k = 0; k + 1 < LandRows; ++k )
                        if ( row.m_height[s][k] > shoreLevel && row.m_height[s][k + 1] <= shoreLevel )
                        {
                            edge = lerpf( std::abs( row.m_lateral[s][k] ), std::abs( row.m_lateral[s][k + 1] ),
                                          ( row.m_height[s][k] - shoreLevel ) /
                                              std::max( 0.01f, row.m_height[s][k] - row.m_height[s][k + 1] ) );
                            break;
                        }
                    if ( edge < 0.0f ) return;
                    dist = std::max( 1.0f, edge - A.m_edge - 1.0f );
                }
                if ( !clear( dist ) ) return;
                using KindT = PropKindE;
                const KindT kind = pr.m_kind;
                bool        tall = false;
                // The places' own landmarks and stands (the shapes of the data files) and every other structure that
                // keeps its place (towers, pylons, pyramids, ruins) stand on their base like houses.
                const bool landmark = keepsItsPlace( kind );
                const bool building = !pr.m_onWater && !pr.m_climb && !pr.m_shore &&
                                      ( landmark || ( pr.m_perStage == 0 && ( plotted( kind, tall ) ||
                                                                              standsOnFootprint( kind ) ) ) );
                // The forest's wall hides what would stand behind it.
                if ( forestD > 0.0f && !building && pr.m_perStage == 0 && dist > forestD - 4.0f ) return;
                // No trees above the tree line, fewer just below it.
                const float line = landRow( j ).m_treeLine;
                if ( isTree( kind ) && line < 1e8f )
                {
                    const float alt = static_cast<float>( c.m_origin.m_y ) + groundY( j, h2, side * ( A.m_edge + dist ) );
                    if ( hashFloat( key + 21, static_cast<uint64_t>( j ) ) < smoothstepf( line - 300.0f, line, alt ) )
                        return;
                }
                float yaw = A.m_heading + h3 * Tau;
                switch ( kind )
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
                    if ( models.facing( kind ) == FacingE::ROAD )
                        yaw = A.m_heading + ( side < 0 ? Tau * 0.5f : 0.0f ) + ( h3 - 0.5f ) * 0.3f;
                    else if ( models.facing( kind ) == FacingE::ALONG )
                        yaw = A.m_heading;
                    else if ( onPlot )
                        yaw = A.m_heading + Tau * 0.25f * std::floor( h3 * 4.0f ) + ( h3 - 0.5f ) * 0.1f;
                    break;
                }
                // A place's own shapes face the road with their fronts (+x), a little askew, unless they turn as they
                // will (boats, animals, trees, round things); what reaches into the water turns its front out over it,
                // what climbs a slope its back uphill.
                const bool local = kind == KindT::LANDMARK && pr.m_perStage == 0 && pr.m_variant >= 0;
                if ( local && !pr.m_free )
                    yaw = A.m_heading + ( ( side < 0 ) != pr.m_shore ? Tau * 0.5f : 0.0f ) +
                          ( h3 - 0.5f ) * ( pr.m_climb || pr.m_shore ? 0.1f : 0.35f );
                const float  scale = lerpf( pr.m_scaleMin, pr.m_scaleMax, h4 );
                const float  bright = 0.88f + 0.24f * hashFloat( key + 5, static_cast<uint64_t>( j ) );
                const Color *defaults = models.keys( kind );
                Color        keys[3] = {};
                if ( defaults )
                {
                    keys[0] = pr.m_a.a ? pr.m_a : defaults[0];
                    keys[1] = pr.m_b.a ? pr.m_b : defaults[1];
                    keys[2] = pr.m_c.a ? pr.m_c : defaults[2];
                    if ( pr.m_pastel )
                        keys[0] = Pastels[hash64( key + 7 + static_cast<uint64_t>( j ) ) % std::size( Pastels )];
                }
                uint64_t variant = pr.m_variant >= 0 ? static_cast<uint64_t>( pr.m_variant )
                                                     : hash64( key + 6 + static_cast<uint64_t>( j ) );
                // Warning signs in the country's shape: a yellow diamond, a white or a yellow triangle bordered red, a yellow
                // one bordered black.
                static const uint64_t warnShape[] = { 5, 10, 0, 15 };
                if ( kind == PropKindE::WARNING_SIGN )
                    variant = variant % 5 + warnShape[std::min<size_t>( signs.m_warn, 3 )];
                // Deciduous trees, bushes, hedges and vines turn, lose their leaves and some blossom; the rest only takes the
                // snow.
                const bool turns = kind == KindT::OAK || kind == KindT::BUSH || kind == KindT::BIRCH ||
                                   kind == KindT::POPLAR || kind == KindT::BROADLEAF || kind == KindT::SHRUB ||
                                   kind == KindT::HEDGE || kind == KindT::VINE_ROW || kind == KindT::FERN ||
                                   kind == KindT::FLOWER_BUSH;
                const bool blooms = kind == KindT::BROADLEAF || kind == KindT::BUSH || kind == KindT::SHRUB;
                SeasonLook look = A.m_season;
                if ( !turns ) look.m_autumn = look.m_bare = look.m_spring = 0.0f;
                const float        h = hashFloat( key + 13, static_cast<uint64_t>( j ) );
                const float        pick = blooms ? h : 0.3f + 0.7f * h;
                const MeshBuilder &full = models.prop( kind, variant );
                const Vector3      ext = Vector3Scale( extentOf( full ), scale );
                // Nothing of it on the road: facing the road, its front (from some a walkway reaches far); turned as
                // it will, its longest reach.
                // What reaches into the water or up a slope: the side towards the road of its foot (behind a pier,
                // in front of a funicular); one by the water that would reach the road stays away.
                float reach = pr.m_free ? std::max( ext.x, ext.z ) : ext.x;
                if ( pr.m_shore || pr.m_climb )
                {
                    const std::pair<Vector3, Vector3> &foot = baseOf( full );
                    reach = scale * ( pr.m_shore ? foot.second.x - foot.first.x : foot.first.x + foot.second.x );
                }
                if ( local && reach > dist - 0.5f )
                {
                    if ( pr.m_shore || !clear( reach + 1.0f ) ) return;
                    dist = reach + 1.0f;
                }
                // Where the road runs between banks of snow, snow lies on the roofs and the trees as well.
                look.m_winter = std::max( look.m_winter, 0.85f * landRow( j ).m_snowCover );
                // A building's walls where they meet the ground: the middle of their box and its half sizes.
                Vector3 mid{}, base = ext;
                if ( building )
                {
                    mid = Vector3Scale( baseOf( full ).first, scale );
                    base = Vector3Scale( baseOf( full ).second, scale );
                }

                // A building needs ground that does not climb much under its walls: where it does, it moves back or forth,
                // or stays away.
                float d = dist, lo = 0.0f, hi = 0.0f;
                if ( building )
                {
                    footing( d, yaw, mid, base, h2, lo, hi );
                    if ( hi - lo > MaxSpread && !landmark )
                    {
                        bool moved = false;
                        // Back from the road at will; towards it only while its walls keep well clear of the shoulder.
                        const float reachOut = std::max( ext.x, ext.z );
                        for ( float step : { 5.0f, -5.0f, 10.0f, -10.0f } )
                        {
                            float a = 0.0f, b = 0.0f;
                            if ( step < 0.0f && dist + step < reachOut + 3.0f ) continue;
                            if ( !clear( dist + step ) ) continue;
                            footing( dist + step, yaw, mid, base, h2, a, b );
                            if ( b - a > MaxSpread ) continue;
                            d = dist + step;
                            lo = a;
                            hi = b;
                            moved = true;
                            break;
                        }
                        if ( !moved ) return;
                    }
                }
                const float  lateral = side * ( A.m_edge + d );
                const double ps = sj + h2 * SegmentLength;
                Vector3      pos = Vector3Lerp( at( A, lateral, 0.0f ), at( B, lateral, 0.0f ), h2 );
                // On ground that climbs under it a building stands on a terrace of earth, half in the slope; on ground
                // nearly level it stands on the lowest of it.
                // (Nothing as wide as a hill gets a terrace: it stands on the lowest ground under it.)
                const bool terrace = building && hi - lo > 0.25f && std::max( base.x, base.z ) <= 20.0f;
                // How its part of the world builds on a slope: bases of stone under many houses of Europe (the Alps,
                // the Mediterranean) and East Asia, of plaster in Arabia and Latin America; houses on stilts in
                // Southeast Asia, Oceania and on the islands; earth elsewhere. A town's buildings stand on stone.
                UnderE under = UnderE::EARTH;
                Color  stone = { 132, 126, 118, 255 };
                if ( building && !pr.m_onWater && std::max( base.x, base.z ) <= 20.0f )
                {
                    const FleetE region = rule.m_country ? signs.m_fleet : FleetE::COUNT;
                    const float  share = hashFloat( key + 29, static_cast<uint64_t>( j ) );
                    float        walled = 0.2f, stilted = 0.0f;
                    switch ( region )
                    {
                    case FleetE::EUROPE: walled = 0.55f; break;
                    case FleetE::EAST_ASIA:
                        walled = 0.6f;
                        stone = Color{ 104, 102, 100, 255 };
                        break;
                    case FleetE::BRITISH: walled = 0.4f; break;
                    case FleetE::ARABIA:
                    case FleetE::LATIN:
                    case FleetE::SOUTH_ASIA:
                        walled = 0.5f;
                        stone = lerpColor( sf.m_shoulder, Color{ 226, 214, 190, 255 }, 0.7f );
                        break;
                    case FleetE::SOUTHEAST_ASIA:
                        walled = 0.15f;
                        stilted = 0.55f;
                        break;
                    case FleetE::OCEANIA:
                    case FleetE::ISLANDS:
                        walled = 0.15f;
                        stilted = 0.3f;
                        break;
                    default: break;
                    }
                    const bool house = onPlot && !plots.m_city && !tall && std::max( base.x, base.z ) <= 9.0f;
                    if ( ( onPlot && plots.m_city ) || tall )
                        under = UnderE::STONE;
                    else if ( house && share < stilted )
                        under = UnderE::STILTS;
                    else if ( share < stilted + walled )
                        under = UnderE::STONE;
                }
                float climb = 0.0f;   // the slope it climbs (rise per metre away from the road)
                if ( pr.m_onWater )
                {
                    pos.y = at( A, lateral, A.m_terrain.m_seaLevel + A.m_seg->m_trench ).y;
                }
                else if ( pr.m_shore )
                {
                    pos.y = at( A, lateral, shoreLevel ).y + 0.02f;   // its foot at the water (its own water on it)
                }
                else if ( pr.m_climb )
                {
                    // Up the land behind it: only where it climbs away from the road, steeply enough.
                    const float g0 = groundY( j, h2, lateral ), g1 = groundY( j, h2, lateral + side * ext.x );
                    climb = ( g1 - g0 ) / std::max( 1.0f, ext.x );
                    if ( climb < 0.15f || climb > 1.2f ) return;
                    pos.y = g0 - 0.15f;
                }
                else if ( building )
                {
                    // On stilts over the highest of the ground; else half in the slope, or on the lowest of level ground.
                    // Never high on a mound: what cannot move off a steep slope (a landmark) stands at most a storey's
                    // half over the lowest ground under it, the rest of it built into the slope.
                    pos.y = under == UnderE::STILTS ? hi + 0.7f
                            : terrace               ? std::min( lerpf( lo, hi, 0.5f ), lo + MaxSpread * 0.5f )
                                                    : lo;
                }
                else
                {
                    // Sunk a little where the ground slopes, so no foot hangs in the air.
                    const float slope =
                        std::abs( groundY( j, h2, lateral + 1.0f ) - groundY( j, h2, lateral - 1.0f ) ) * 0.5f;
                    pos.y = groundY( j, h2, lateral ) - std::min( 0.6f, 0.4f * slope + 0.03f );
                }

                // Up close in full; from afar (and deep beside the road) lighter; landmarks always in full.
                const MeshBuilder &far = pr.m_perStage > 0 ? full : models.propFar( kind, variant );
                const MeshBuilder &near = d > LodLateral ? far : full;
                const size_t       n0 = scenery.vertexCount(), f0 = sceneryFar.vertexCount();
                if ( near.triangleCount() )
                    scenery.append( near, pos, yaw, scale, bright, defaults ? keys : nullptr, &look, pick );
                if ( far.triangleCount() )
                    sceneryFar.append( far, pos, yaw, scale, bright, defaults ? keys : nullptr, &look, pick );
                if ( kind == KindT::WINDMILL )
                {
                    // A wind turbine: its blades turn at the hub.
                    const Vector3 F = headingForward( yaw );
                    c.m_rotors.push_back( { Vector3{ pos.x + F.x * TurbineHub.z * scale, pos.y + TurbineHub.y * scale,
                                                     pos.z + F.z * TurbineHub.z * scale },
                                            yaw, scale, h * Tau } );
                }
                if ( kind == KindT::BILLBOARD && d < LodLateral )
                {
                    // Where the country writes in its own script, many a billboard carries the place's name in it.
                    const BiomeE      town = track.stageBiome( static_cast<int>( stage ) );
                    const ScriptName *own = scriptName( town );
                    if ( own && biomeDef( town ).m_name && hashFloat( key + 27, static_cast<uint64_t>( j ) ) < 0.7f )
                    {
                        MeshBuilder board;
                        buildPlaceSign( board, biomeDef( town ).m_name, 0, 0.5f, 6.5f, false, own );
                        scenery.append( board, Vector3Add( pos, Vector3Scale( headingForward( yaw ), -0.3f * scale ) ), yaw,
                                        scale );
                    }
                }
                if ( climb > 0.0f )
                {
                    // Sheared to the slope: what stands on it stays upright, the track and the car follow the land.
                    const Vector3 away = Vector3Scale( A.m_right, side );
                    scenery.shearSince( n0, pos, away, climb );
                    sceneryFar.shearSince( f0, pos, away, climb );
                }
                if ( followsSlope( kind ) && !pr.m_onWater )
                {
                    // A fence or a row along the ground: tilted to its slope.
                    const Vector3 F = headingForward( yaw );
                    const float   len = std::max( 1.0f, ext.z );
                    const float   rise = groundAt( j, h2, lateral, Vector3Scale( F, len ) ) -
                                       groundAt( j, h2, lateral, Vector3Scale( F, -len ) );
                    scenery.shearSince( n0, pos, F, rise / ( 2.0f * len ) );
                    sceneryFar.shearSince( f0, pos, F, rise / ( 2.0f * len ) );
                }
                if ( building )
                {
                    // The foot of the walls a shade darker.
                    scenery.shadeBelow( n0, pos.y + 0.05f, 0.8f );
                    sceneryFar.shadeBelow( f0, pos.y + 0.05f, 0.8f );
                    {
                        // What the building stands on where the ground falls away under it (UnderE, chosen above): a
                        // base of stone or plaster under its walls, a terrace of earth in the land's colour, or both
                        // (the base at most a good metre high, the earth below it); a house on stilts stands on posts.
                        const Vector3 L = Vector3Negate( headingRight( yaw ) ), F = headingForward( yaw );
                        const float   sx[4] = { -1.0f, 1.0f, 1.0f, -1.0f }, sz[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
                        auto          corner = [&]( int k, float grow )
                        {
                            return Vector3Add( Vector3Scale( L, mid.x + sx[k] * ( base.x + grow ) ),
                                               Vector3Scale( F, mid.z + sz[k] * ( base.z + grow ) ) );
                        };
                        const float fall = pos.y - lo;   // the deepest the ground lies under the floor
                        const Vector3 centre = { pos.x + L.x * mid.x + F.x * mid.z, 0.0f, pos.z + L.z * mid.x + F.z * mid.z };
                        if ( under == UnderE::STILTS )
                        {
                            // A floor of planks on posts at the corners and between them.
                            const Color wood = scaleColor( Color{ 96, 72, 50, 255 }, bright );
                            for ( MeshBuilder *m : { &scenery, &sceneryFar } )
                            {
                                obox( *m, Vector3{ centre.x, pos.y - 0.1f, centre.z }, headingRight( yaw ), F, base.x + 0.25f,
                                      0.1f, base.z + 0.25f, wood );
                                for ( int k = 0; k < 4; ++k )
                                    for ( float along : { 0.0f, 0.5f } )
                                    {
                                        const Vector3 a = corner( k, -0.15f ), b = corner( ( k + 1 ) % 4, -0.15f );
                                        const Vector3 o = Vector3Lerp( a, b, along );
                                        const float   g = groundAt( j, h2, lateral, o ) - 0.4f;
                                        obox( *m, Vector3{ pos.x + o.x, ( g + pos.y ) * 0.5f, pos.z + o.z }, headingRight( yaw ),
                                              F, 0.11f, ( pos.y - g ) * 0.5f, 0.11f, scaleColor( wood, 0.8f ) );
                                    }
                            }
                        }
                        else if ( terrace )
                        {
                            const bool  walled = under == UnderE::STONE;
                            const float course = walled ? std::min( fall, 1.1f ) + 0.08f : 0.0f;   // the base's height
                            if ( walled )
                                for ( MeshBuilder *m : { &scenery, &sceneryFar } )
                                {
                                    const size_t p0 = m->vertexCount();
                                    obox( *m, Vector3{ centre.x, pos.y - course * 0.5f + 0.01f, centre.z }, headingRight( yaw ), F,
                                          base.x + 0.22f, course * 0.5f, base.z + 0.22f, scaleColor( stone, bright ) );
                                    m->shadeBelow( p0, pos.y - course + 0.05f, 0.72f );
                                }
                            if ( !walled || fall > 1.1f )
                            {
                                // The terrace of earth: level, a little wider than what stands on it, its sides sloping
                                // down to the land where that lies lower (where it lies higher, it is under the ground).
                                const float level = pos.y - ( walled ? 1.1f : 0.03f ), wide = walled ? 0.7f : 1.0f;
                                Vector3     top[4], foot[4];
                                for ( int k = 0; k < 4; ++k )
                                {
                                    const Vector3 o = corner( k, wide );
                                    const float   drop = std::max( 0.0f, level - groundAt( j, h2, lateral, o ) );
                                    const Vector3 q = corner( k, wide + 0.3f + 1.5f * drop );
                                    top[k] = Vector3{ pos.x + o.x, level, pos.z + o.z };
                                    foot[k] = Vector3{ pos.x + q.x, groundAt( j, h2, lateral, q ) - 0.3f, pos.z + q.z };
                                }
                                const Color tone = scaleColor( landTone( j, d, pos.y ), bright );
                                for ( MeshBuilder *m : { &scenery, &sceneryFar } )
                                {
                                    m->quad( top[0], top[1], top[2], top[3], tone );
                                    for ( int k = 0; k < 4; ++k )
                                        m->quad( top[k], top[( k + 1 ) % 4], foot[( k + 1 ) % 4], foot[k],
                                                 scaleColor( tone, 0.9f ) );
                                }
                            }
                        }
                    }
                    if ( d < LodLateral )
                    {
                        const bool onEarth = terrace && under == UnderE::EARTH;
                        contactRing( j, h2, lateral, yaw, mid, base.x, base.z, onEarth ? 0.9f : 1.8f, 125,
                                     onEarth ? pos.y + 0.02f : OnTheGround );
                    }
                }
                else if ( isTree( kind ) && d < LodLateral && !pr.m_onWater )
                {
                    contactDisc( j, h2, lateral, clampf( std::max( ext.x, ext.z ) * 0.75f, 0.8f, 5.5f ), 80 );
                }
                const float radius = pr.m_radius > 0.0f ? pr.m_radius : models.propRadius( kind );
                if ( radius > 0.0f && d < 45.0f && !pr.m_soft ) c.m_colliders.push_back( { ps, lateral, radius * scale } );

                // A town's first row: a shop on the ground floor of the front to the street, its window lit, its sign
                // above.
                if ( onPlot && plots.m_city && plotRow == 0 && building && d < LodLateral &&
                     hashFloat( key + 23, static_cast<uint64_t>( plot ) ) < 0.75f )
                {
                    const Vector3 toRoad = Vector3Scale( A.m_right, -side );
                    const Vector3 L = Vector3Negate( headingRight( yaw ) ), F = headingForward( yaw );
                    const float   dl = Vector3DotProduct( toRoad, L ), df = Vector3DotProduct( toRoad, F );
                    const bool    byX = std::abs( dl ) > std::abs( df );
                    const Vector3 n = Vector3Scale( byX ? L : F, ( byX ? dl : df ) > 0.0f ? 1.0f : -1.0f );
                    const Vector3 t = byX ? F : L;
                    // On the outside of the wall that faces the street.
                    const float   depth = byX ? base.x : base.z, width = ( byX ? base.z : base.x ) * 0.92f;
                    const Vector3 wall = Vector3Add( Vector3Add( pos, Vector3Scale( L, mid.x ) ), Vector3Scale( F, mid.z ) );
                    if ( width > 2.0f )
                    {
                        // Shops side by side, each some six metres wide: a dark frame, a lit window in it, and above it
                        // a sign in a colour of its part of the world; now and then a plain front between them. In East
                        // Asia many a shop hangs a tall sign out from the front as well, lit on both faces.
                        const int     bays = std::max( 1, static_cast<int>( 2.0f * width / 6.5f ) );
                        const float   bay = 2.0f * width / static_cast<float>( bays );
                        const Vector3 up = { 0.0f, 1.0f, 0.0f };
                        auto          pane = [&]( MeshBuilder &m, Vector3 centre, float hw, float hh, Color col, Mat mat )
                        {
                            const Vector3 a = Vector3Scale( t, hw ), b = Vector3Scale( up, hh );
                            m.quad( Vector3Subtract( Vector3Subtract( centre, a ), b ),
                                    Vector3Subtract( Vector3Add( centre, a ), b ),
                                    Vector3Add( Vector3Add( centre, a ), b ),
                                    Vector3Add( Vector3Subtract( centre, a ), b ), col, mat );
                        };
                        for ( int b = 0; b < bays; ++b )
                        {
                            const uint64_t shop = hash64( key + 25 + static_cast<uint64_t>( plot ) * 16 + static_cast<uint64_t>( b ) );
                            if ( shop % 5 == 0 ) continue;
                            float         lit = 0.9f;
                            const Color   sign = shopSign( rule.m_country ? signs.m_fleet : FleetE::COUNT, shop >> 8, lit );
                            const bool    hung = rule.m_country && signs.m_fleet == FleetE::EAST_ASIA && ( ( shop >> 28 ) & 1 );
                            const float   hw = bay * 0.5f - 0.35f;
                            const float   sw = hw * ( 0.55f + 0.4f * static_cast<float>( ( shop >> 16 ) % 100 ) / 100.0f );
                            const Vector3 face =
                                Vector3Add( Vector3Add( wall, Vector3Scale( n, depth + 0.03f ) ),
                                            Vector3Scale( t, -width + bay * ( static_cast<float>( b ) + 0.5f ) ) );
                            const Vector3 glass = Vector3Add( face, Vector3Scale( n, 0.02f ) );
                            const Vector3 board = Vector3Add( face, Vector3Scale( n, 0.08f ) );
                            const Vector3 front = Vector3Add( board, Vector3Scale( n, 0.09f ) );
                            for ( MeshBuilder *m : { &scenery, &sceneryFar } )
                            {
                                pane( *m, Vector3{ face.x, pos.y + 1.55f, face.z }, hw, 1.5f, Color{ 44, 42, 42, 255 }, Mat{} );
                                pane( *m, Vector3{ glass.x, pos.y + 1.65f, glass.z }, hw - 0.18f, 1.2f,
                                      Color{ 206, 186, 142, 255 }, Mat{ 0.4f, 0.7f } );
                                obox( *m, Vector3{ board.x, pos.y + 3.5f, board.z }, t, n, sw, 0.36f, 0.08f,
                                      scaleColor( sign, 0.35f ) );
                                pane( *m, Vector3{ front.x, pos.y + 3.5f, front.z }, sw * 0.94f, 0.28f, sign,
                                      Mat{ lit, 0.2f } );
                                if ( hung )
                                {
                                    const Vector3 out = Vector3Add( face, Vector3Scale( n, 0.75f ) );
                                    const Vector3 mid2 = { out.x, pos.y + 6.4f, out.z };
                                    obox( *m, mid2, t, n, 0.06f, 1.7f, 0.5f, scaleColor( sign, 0.3f ) );
                                    for ( float s2 : { -1.0f, 1.0f } )
                                    {
                                        const Vector3 at2 = Vector3Add( mid2, Vector3Scale( t, s2 * 0.07f ) );
                                        const Vector3 a2 = Vector3Scale( n, 0.44f ), b2 = Vector3Scale( up, 1.6f );
                                        m->quad( Vector3Subtract( Vector3Subtract( at2, a2 ), b2 ),
                                                 Vector3Subtract( Vector3Add( at2, a2 ), b2 ),
                                                 Vector3Add( Vector3Add( at2, a2 ), b2 ),
                                                 Vector3Add( Vector3Subtract( at2, a2 ), b2 ), sign, Mat{ 1.0f, 0.2f } );
                                    }
                                }
                            }
                        }
                    }
                }

                // A house of a village: smoke from its chimney where it is cold (the game lets it rise), a drive from
                // the road to it (in the first row), a hedge, a fence or a wall round its plot, a bush or two in its garden.
                if ( !onPlot || plots.m_city || !building || d >= LodLateral ) return;
                if ( !tall && c.m_chimneys.size() < 24 )
                    c.m_chimneys.push_back( Vector3{ pos.x, pos.y + ext.y * 0.96f, pos.z } );
                const Vector3 toRoad = Vector3Scale( A.m_right, -side );
                const Vector3 L = Vector3Negate( headingRight( yaw ) ), F = headingForward( yaw );
                const float   deep = std::abs( Vector3DotProduct( toRoad, L ) ) * base.x +
                                   std::abs( Vector3DotProduct( toRoad, F ) ) * base.z;
                const float wide = std::abs( Vector3DotProduct( A.m_forward, L ) ) * base.x +
                                   std::abs( Vector3DotProduct( A.m_forward, F ) ) * base.z;
                const float front = d - deep;   // the house's front from the shoulder's edge
                const float h5 = hashFloat( key + 17, static_cast<uint64_t>( plot ) );
                const float driveAt = ( h5 < 0.5f ? -1.0f : 1.0f ) * std::max( 0.0f, std::min( wide * 0.55f, wide - 1.6f ) );
                const bool  drive = plotRow == 0 && front > 2.5f;
                if ( drive )
                    groundStrip( scenery, j, h2, driveAt, side * A.m_edge, side * ( A.m_edge + front + 0.4f ), 2.8f,
                                 scaleColor( lerpColor( sf.m_shoulder, Color{ 150, 146, 138, 255 }, 0.6f ), bright ),
                                 Mat{ 0.0f, 0.05f } );
                // The plot: from its first segment to the next plot's, in metres along from the house.
                const float lo0 = static_cast<float>( static_cast<double>( plot * plotSegs ) * SegmentLength - ps ) + 0.6f;
                const float hi0 =
                    static_cast<float>( static_cast<double>( ( plot + 1 ) * plotSegs ) * SegmentLength - ps ) - 0.6f;
                const float edgeFront = std::max( 3.0f, front - 1.8f ), edgeBack = d + deep + 2.5f;
                const float h6 = hashFloat( key + 18, static_cast<uint64_t>( plot ) );
                // What stands round the plot, as its part of the world builds it: dry stone walls and hedges in
                // Britain and Ireland, white pickets or open lawns in North America and Oceania, dark wooden fences in
                // East Asia, painted plank fences in the East, fences of sticks in Africa, plastered walls in Arabia,
                // South and Southeast Asia and Latin America; elsewhere hedges and wooden fences on green land and low
                // pale walls on dry.
                float       height = 0.0f, thick = 0.0f;
                Color       col{};
                const Color hedge = lerpColor( Color{ 58, 94, 50, 255 }, Snow, 0.7f * look.m_winter );
                const Color plaster = scaleColor( lerpColor( sf.m_shoulder, Color{ 226, 214, 190, 255 }, 0.7f ), bright );
                auto        fence = [&]( float tall, float wide, Color paint )
                {
                    height = tall;
                    thick = wide;
                    col = paint;
                };
                switch ( rule.m_country ? signs.m_fleet : FleetE::COUNT )
                {
                case FleetE::BRITISH:
                    if ( h6 < 0.5f )
                        fence( 1.0f, 0.45f, scaleColor( Color{ 134, 130, 122, 255 }, bright ) );
                    else if ( h6 < 0.9f )
                        fence( 1.3f, 0.8f, hedge );
                    break;
                case FleetE::NORTH_AMERICA:
                case FleetE::OCEANIA:
                    if ( h6 < 0.4f ) fence( 0.9f, 0.06f, Color{ 240, 238, 230, 255 } );
                    break;
                case FleetE::EAST_ASIA:
                    if ( h6 < 0.5f )
                        fence( 1.4f, 0.08f, Color{ 46, 38, 32, 255 } );
                    else if ( h6 < 0.85f )
                        fence( 1.1f, 0.6f, hedge );
                    break;
                case FleetE::EASTERN:
                {
                    static const Color paints[3] = { { 70, 110, 150, 255 }, { 78, 122, 84, 255 }, { 124, 92, 64, 255 } };
                    if ( h6 < 0.75f ) fence( 1.3f, 0.07f, paints[static_cast<int>( h5 * 2.99f )] );
                    break;
                }
                case FleetE::AFRICA:
                    if ( h6 < 0.6f ) fence( 1.1f, 0.1f, Color{ 112, 86, 58, 255 } );
                    break;
                case FleetE::ARABIA:
                case FleetE::SOUTH_ASIA:
                case FleetE::SOUTHEAST_ASIA:
                case FleetE::LATIN:
                    if ( h6 < 0.65f ) fence( 1.2f + 0.4f * h5, 0.3f, plaster );
                    break;
                default:
                    if ( green && h6 < 0.55f )
                        fence( 1.0f, 0.7f, hedge );
                    else if ( green && h6 < 0.8f )
                        fence( 1.0f, 0.07f, Color{ 122, 92, 64, 255 } );
                    else if ( !green && h6 < 0.6f )
                        fence( 0.8f, 0.35f, plaster );
                    break;
                }
                if ( height > 0.0f && edgeBack - edgeFront > 2.0f && hi0 - lo0 > 4.0f )
                {
                    const float lf = side * ( A.m_edge + edgeFront ), lb = side * ( A.m_edge + edgeBack );
                    if ( drive )
                    {
                        groundWall( scenery, j, h2, lf, lo0, lf, driveAt - 1.8f, height, thick, col );
                        groundWall( scenery, j, h2, lf, driveAt + 1.8f, lf, hi0, height, thick, col );
                    }
                    else
                    {
                        groundWall( scenery, j, h2, lf, lo0, lf, hi0, height, thick, col );
                    }
                    groundWall( scenery, j, h2, lf, lo0, lb, lo0, height, thick, col );
                    // Run into, the hedge or the wall turns the car away like a rail (and keeps it from the house).
                    if ( edgeFront < 45.0f )
                        for ( float a = lo0; a < hi0; a += 3.0f )
                            if ( !drive || std::abs( a - driveAt ) > 1.8f )
                                c.m_colliders.push_back( { ps + static_cast<double>( a ), lf, 0.5f, true, true } );
                }
                // A bush or two beside the house.
                const PropKindE plant = green ? ( h6 < 0.5f ? KindT::FLOWER_BUSH : KindT::BUSH ) : KindT::AGAVE;
                SeasonLook      garden = A.m_season;
                for ( int b = 0; b < 1 + ( h5 > 0.6f ? 1 : 0 ); ++b )
                {
                    const float sideOf = ( b == 0 ? -1.0f : 1.0f ) * ( h5 < 0.5f ? -1.0f : 1.0f );
                    const float a = clampf( sideOf * ( wide + 1.6f ), lo0 + 1.0f, hi0 - 1.0f );
                    const float l =
                        side *
                        ( A.m_edge + d + ( hashFloat( key + 19 + b, static_cast<uint64_t>( plot ) ) - 0.5f ) * deep );
                    scenery.append( models.prop( plant, hash64( key + 20 + b ) ), onGround( j, h2, l, a, -0.05f ),
                                    yaw + static_cast<float>( b ), 0.8f + 0.4f * h6, bright, models.keys( plant ),
                                    &garden, hashFloat( key + 22, static_cast<uint64_t>( b ) ) );
                }
            };
            for ( size_t r = 0; r < rule.m_props.size(); ++r )
            {
                const PropRule &pr = rule.m_props[r];
                if ( built && plots.has( r ) ) continue;
                const uint64_t key = seed ^ ( sideKey * 31 + r * 7919 );
                const float    h0 = hashFloat( key, static_cast<uint64_t>( j ) );
                if ( pr.m_perStage > 0 )
                {   // A landmark stands at a few fixed spots per stage, not by chance.
                    bool spot = false;
                    for ( int k = 0; k < pr.m_perStage && !spot; ++k )
                    {
                        bool       right = false;
                        const long where =
                            landmarkSpot( seed, r, k, stage, track.stageSegments( static_cast<int>( stage ) ), right );
                        // On the dry side, or on the water for a monument that stands in it.
                        const bool wetL = A.m_terrain.m_waterLeft > 0.5f, wetR = A.m_terrain.m_waterRight > 0.5f;
                        if ( ( right ? wetR : wetL ) != pr.m_onWater && ( right ? wetL : wetR ) == pr.m_onWater )
                            right = !right;
                        spot = !tunnel && local == where && right == ( side > 0 );
                    }
                    if ( !spot ) continue;
                }
                else if ( h0 >= pr.m_chance )
                {
                    continue;
                }
                place( pr, key, lerpf( pr.m_nearest, pr.m_farthest, hashFloat( key + 1, static_cast<uint64_t>( j ) ) ),
                       -1 );
            }
            // One plot per few segments and row; a city leaves every fourth free for a cross street, a village thins out
            // towards its back.
            if ( built && !wet && j % plotSegs == plotSegs / 2 && !( plots.m_city && plot % 4 == 0 ) )
                for ( int row = 0; row < plots.m_rows; ++row )
                {
                    const uint64_t key = seed ^ ( sideKey * 37 + static_cast<uint64_t>( row ) * 104729 + 0x9107ull );
                    const float    fill = plots.m_city ? 0.94f : 0.8f - 0.18f * static_cast<float>( row );
                    if ( hashFloat( key, static_cast<uint64_t>( plot ) ) >= fill ) continue;
                    const float pick = hashFloat( key + 14, static_cast<uint64_t>( plot ) ) *
                                       plots.m_upTo[static_cast<size_t>( plots.m_count - 1 )];
                    int         i = 0;
                    while ( i + 1 < plots.m_count && pick >= plots.m_upTo[static_cast<size_t>( i )] ) ++i;
                    const float dist = plots.m_nearest + plots.m_half + plots.m_across * static_cast<float>( row ) +
                                       ( hashFloat( key + 15, static_cast<uint64_t>( plot ) ) - 0.5f ) * 3.0f;
                    place( rule.m_props[plots.m_rule[static_cast<size_t>( i )]], key, dist, row );
                }
        }

        // Overhead sign gantries.
        if ( here.m_gantrySpacing > 0.0f && !tunnel )
        {
            const long k = std::max( 4L, static_cast<long>( here.m_gantrySpacing / SegmentLength + 0.5f ) );
            if ( j % k == k / 3 && !( seg.m_flags & ( SEG_CHECKPOINT | SEG_START ) ) )
            {
                MeshBuilder g;
                const float span = A.m_hw + RumbleWidth + 0.3f;
                const Color steel = { 150, 154, 162, 255 };
                const Color white = { 240, 240, 240, 255 };
                const Mat   paint = { 0.35f, 0.0f };
                for ( float x : { -span, span } ) g.box( Vector3{ x, 3.8f, 0 }, Vector3{ 0.25f, 3.8f, 0.25f }, steel );
                g.box( Vector3{ 0, 7.4f, 0 }, Vector3{ span + 0.3f, 0.3f, 0.3f }, steel );
                for ( float sx : { -0.5f, 0.5f } )
                {
                    const float cx = sx * A.m_hw, hw = A.m_hw * 0.42f;
                    g.box( Vector3{ cx, 8.7f, -0.1f }, Vector3{ hw, 1.05f, 0.08f }, Color{ 20, 110, 64, 255 },
                           Mat{ 0.08f, 0.0f } );
                    g.box( Vector3{ cx, 9.65f, -0.19f }, Vector3{ hw - 0.1f, 0.04f, 0.01f }, white, paint );
                    g.box( Vector3{ cx, 7.75f, -0.19f }, Vector3{ hw - 0.1f, 0.04f, 0.01f }, white, paint );
                    for ( int line = 0; line < 2; ++line )
                    {
                        const uint64_t key = static_cast<uint64_t>( j * 4 + line + ( sx > 0 ? 2 : 0 ) );
                        const float    len = hw * ( 0.5f + 0.3f * hashFloat( seed + 55, key ) );
                        g.box(
                            Vector3{ cx - hw + 0.4f + len * 0.5f, 9.15f - 0.55f * static_cast<float>( line ), -0.19f },
                            Vector3{ len * 0.5f, 0.14f, 0.01f }, white, paint );
                    }
                    g.tri( Vector3{ cx + hw - 1.0f, 8.0f, -0.19f }, Vector3{ cx + hw - 0.4f, 8.0f, -0.19f },
                           Vector3{ cx + hw - 0.7f, 8.5f, -0.19f }, white, paint );
                }
                props.appendBasis( g, Vector3Negate( A.m_right ), Vector3{ 0, 1, 0 }, A.m_forward,
                                   at( A, 0.0f, 0.0f ) );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, -span, 0.5f } );
                c.m_colliders.push_back( { static_cast<double>( j ) * SegmentLength, span, 0.5f } );
            }
        }

        // Checkpoint gates and the start gantry.
        if ( ( seg.m_flags & ( SEG_CHECKPOINT | SEG_START | SEG_FINISH ) ) && !tunnel )
        {
            const bool  start = seg.m_flags & ( SEG_START | SEG_FINISH );
            MeshBuilder gate;
            const float span = A.m_edge + 1.0f;
            const Color pillar = start ? Color{ 60, 62, 70, 255 } : Color{ 230, 232, 240, 255 };
            for ( float x : { -span, span } ) gate.box( Vector3{ x, 4.6f, 0 }, Vector3{ 0.6f, 4.6f, 0.6f }, pillar );
            gate.box( Vector3{ 0, 9.0f, 0 }, Vector3{ span + 0.6f, 0.9f, 0.45f }, Color{ 30, 30, 34, 255 } );
            const int   squares = static_cast<int>( ( span + 0.6f ) * 2.0f / 0.9f );
            const float w = ( span + 0.6f ) * 2.0f / static_cast<float>( squares );
            for ( int row = 0; row < 2; ++row )
            {
                for ( int q = 0; q < squares; ++q )
                {
                    if ( ( q + row ) % 2 ) continue;
                    const float x0 = -span - 0.6f + w * static_cast<float>( q );
                    const float y0 = 8.1f + 0.9f * static_cast<float>( row );
                    gate.quad( Vector3{ x0, y0, -0.47f }, Vector3{ x0 + w, y0, -0.47f },
                               Vector3{ x0 + w, y0 + 0.9f, -0.47f }, Vector3{ x0, y0 + 0.9f, -0.47f },
                               Color{ 245, 245, 245, 255 }, Mat{ 0.55f, 0.0f } );
                }
            }
            const Color strip = start ? Color{ 255, 40, 40, 255 } : Color{ 60, 255, 140, 255 };
            gate.box( Vector3{ 0, 7.95f, -0.2f }, Vector3{ span, 0.08f, 0.08f }, strip, Mat{ 1.0f, 0.0f } );
            if ( start )
            {
                for ( int q = -2; q <= 2; ++q )
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
            for ( int row = 0; row < 2; ++row )
                for ( int q = 0; q < n; ++q )
                {
                    const Color col = ( q + row ) % 2 ? Color{ 20, 20, 24, 255 } : Color{ 240, 240, 240, 255 };
                    const float x0 = -A.m_hw + sq * static_cast<float>( q );
                    const float z0 = sq * static_cast<float>( row );
                    line.quad( Vector3{ x0, 0.03f, z0 }, Vector3{ x0 + sq, 0.03f, z0 },
                               Vector3{ x0 + sq, 0.03f, z0 + sq }, Vector3{ x0, 0.03f, z0 + sq }, col,
                               Mat{ 0.0f, 0.2f } );
                }
            const Vector3 tiltedLeft = Vector3Normalize( Vector3{ -A.m_right.x, A.m_tanBank, -A.m_right.z } );
            ground.appendBasis( line, tiltedLeft, Vector3{ 0, 1, 0 }, A.m_forward, at( A, 0.0f, 0.0f ) );
        }

        // Puddles: dark mirrors on the road.
        if ( seg.m_flags & SEG_PUDDLE )
        {
            MeshBuilder pud;
            const Color dark = { 26, 30, 40, 255 };
            for ( int q = 0; q < 10; ++q )
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

        if ( viaduct )
        {
            // Both windings: the viaduct is seen from below, beside and above.
            auto face = [&]( Vector3 a, Vector3 b, Vector3 cc, Vector3 d, Color col )
            {
                props.quad( a, b, cc, d, col );
                props.quad( d, cc, b, a, col );
            };
            const float l = railL, w = 2.6f, h = RailHeight;
            const Color concrete = { 178, 174, 166, 255 }, shade = { 128, 124, 118, 255 }, steel = { 64, 60, 58, 255 },
                        stone = { 132, 124, 112, 255 };
            const float groundA = rowY( landRow( j ), l ) - at( A, l, 0.0f ).y;   // the ground as drawn
            if ( sj >= viaFrom && sj < viaTo )
            {
                // The deck, its parapets and the rails.
                face( at( A, l - w, h - 0.15f ), at( A, l + w, h - 0.15f ), at( B, l + w, h - 0.15f ),
                      at( B, l - w, h - 0.15f ), concrete );
                face( at( A, l - w, h - 1.4f ), at( A, l + w, h - 1.4f ), at( B, l + w, h - 1.4f ),
                      at( B, l - w, h - 1.4f ), shade );
                for ( float edge : { -w, w } )
                    face( at( A, l + edge, h + 0.55f ), at( B, l + edge, h + 0.55f ), at( B, l + edge, h - 1.4f ),
                          at( A, l + edge, h - 1.4f ), concrete );
                for ( float r : { -0.75f, 0.75f } )
                    face( at( A, l + r - 0.07f, h ), at( A, l + r + 0.07f, h ), at( B, l + r + 0.07f, h ),
                          at( B, l + r - 0.07f, h ), steel );
                // A pillar down to the ground every 25 m, a mast for the wire every 50.
                if ( j % 5 == 0 && groundA < h - 1.4f )
                {
                    props.box( at( A, l, ( groundA + h - 1.4f ) * 0.5f ),
                               Vector3{ 0.9f, ( h - 1.4f - groundA ) * 0.5f, 0.9f }, shade );
                    c.m_colliders.push_back( { sj, l, 1.1f } );
                }
                if ( j % 10 == 5 )
                {
                    props.box( at( A, l + railway.m_side * ( w - 0.3f ), h + 3.0f ), Vector3{ 0.12f, 3.0f, 0.12f },
                               steel );
                    face( at( A, l - w, h + 5.9f ), at( A, l + w, h + 5.9f ), at( A, l + w, h + 5.75f ),
                          at( A, l - w, h + 5.75f ), steel );
                }
            }
            else
            {
                // A portal: a block of stone the rails run into, its mouth dark.
                const float bw = w + 1.6f, top = h + 5.0f;
                face( at( A, l - bw, top ), at( A, l + bw, top ), at( B, l + bw, top ), at( B, l - bw, top ), stone );
                for ( float edge : { -bw, bw } )
                    face( at( A, l + edge, top ), at( B, l + edge, top ), at( B, l + edge, groundA ),
                          at( A, l + edge, groundA ), stone );
                const bool startMouth = sj < viaFrom && sj + SegmentLength >= viaFrom,
                           endMouth = sj >= viaTo && sj - SegmentLength < viaTo;
                if ( startMouth || endMouth )
                {
                    const Cross &M = startMouth ? B : A;
                    const float  push = startMouth ? 0.06f : -0.06f;
                    auto         out = [&]( Vector3 p ) { return Vector3Add( p, Vector3Scale( M.m_forward, push ) ); };
                    face( at( M, l - bw, top ), at( M, l + bw, top ), at( M, l + bw, groundA ),
                          at( M, l - bw, groundA ), stone );
                    face( out( at( M, l - w, h + 4.2f ) ), out( at( M, l + w, h + 4.2f ) ),
                          out( at( M, l + w, h - 1.4f ) ), out( at( M, l - w, h - 1.4f ) ), Color{ 22, 20, 18, 255 } );
                }
            }
        }
    }

    c.m_ground = std::move( ground );
    c.m_props = std::move( props );
    c.m_scenery = std::move( scenery );
    c.m_sceneryFar = std::move( sceneryFar );
    c.m_glass = std::move( glass );
    c.m_decals = std::move( decals );
}

}   // namespace sunstreak
