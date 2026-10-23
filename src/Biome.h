/**
 * @file Biome.h
 * @brief The sceneries a run passes through: their light, colours, props and the style of their roads.
 *
 * A run is a chain of stages, one biome each. Everything a stage boundary changes is blendable, so the game crossfades
 * the light, the fog and the ground colours over a few hundred metres instead of cutting.
 *
 * @author reneweller
 */
#pragma once

#include <raylib.h>

#include <cstdint>
#include <vector>

namespace sunstreak
{

/** @brief The sceneries. */
enum class BiomeE : uint8_t
{
    MEADOW,   ///< green valley in the morning
    FOREST,   ///< alpine pass, pines and steep hills
    DESERT,   ///< red canyon at noon
    SNOW,     ///< frozen peaks, falling snow, less grip
    COAST,    ///< sunset coast beside the sea
    CITY,     ///< motorway through a city at night
    MARSH,    ///< storm over a marsh: rain, fog, lightning
    NEON,     ///< synthwave night with a striped sun
    COUNT     ///< the number of biomes
};

/** @brief The kinds of objects beside the road. */
enum class PropKindE : uint8_t
{
    PINE,          ///< conifer
    SNOW_PINE,     ///< conifer with snow
    OAK,           ///< round deciduous tree
    BUSH,          ///< low shrub
    ROCK,          ///< stone
    BOULDER,       ///< big stone
    CACTUS,        ///< saguaro
    MESA,          ///< layered rock tower
    PALM,          ///< palm tree
    DEAD_TREE,     ///< bare tree
    REEDS,         ///< tuft of reeds
    LAMP,          ///< street lamp leaning over the road
    BUILDING,      ///< block with lit windows
    TOWER,         ///< high-rise
    BILLBOARD,     ///< lit advertising board
    POLE,          ///< telegraph pole
    FENCE,         ///< a fence panel along the road
    HAYBALE,       ///< round bale
    HOUSE,         ///< house with a pitched roof
    WINDMILL,      ///< wind turbine
    NEON_PILLAR,   ///< glowing pylon
    PYRAMID,       ///< glowing wireframe-ish pyramid
    SNOWBANK,      ///< heap of snow
    COUNT          ///< the number of kinds
};

/** @brief One rule for scattering props beside the road. */
struct PropRule
{
    PropKindE m_kind;       ///< what to place
    float     m_chance;     ///< probability per segment and side
    float     m_nearest;    ///< least distance from the road's edge (m)
    float     m_farthest;   ///< largest distance from the road's edge (m)
    float     m_scaleMin;   ///< smallest scale
    float     m_scaleMax;   ///< largest scale
};

/** @brief Light and air: everything the renderer blends between biomes. */
struct Environment
{
    Color m_skyZenith;          ///< sky straight up
    Color m_skyHorizon;         ///< sky at the horizon
    Color m_sunColor;           ///< sun (or moon) light
    float m_sunIntensity;       ///< its strength
    float m_sunElevation;       ///< its height above the horizon (degrees)
    float m_sunAzimuth;         ///< its direction (degrees, 0 = north, the track's mean direction)
    float m_sunSize;            ///< apparent radius (degrees)
    float m_sunStripes;         ///< 0..1 synthwave stripes across the sun's lower half
    Color m_ambientTop;         ///< light from above
    Color m_ambientBottom;      ///< light from below
    Color m_fogColor;           ///< haze colour
    float m_fogDensity;         ///< haze per metre
    Color m_mountainNear;       ///< nearer silhouette range
    Color m_mountainFar;        ///< farther range
    float m_mountainHeight;     ///< their height (degrees)
    float m_cloudAmount;        ///< 0..1 cover
    Color m_cloudColor;         ///< cloud colour
    float m_stars;              ///< 0..1 star field
    float m_headlights;         ///< 0..1 how much the headlights matter
    float m_rain;               ///< 0..1 rain
    float m_snow;               ///< 0..1 snowfall
    float m_lightning;          ///< 0..1 lightning frequency
    float m_exposure;           ///< brightness before tone mapping
    float m_saturation;         ///< colour grading
    float m_bloom;              ///< bloom strength
};

/** @brief Colours of the road and the ground. */
struct Surface
{
    Color m_groundA;        ///< ground, even stripes
    Color m_groundB;        ///< ground, odd stripes
    Color m_groundFar;      ///< ground far from the road
    Color m_road;           ///< asphalt
    Color m_roadAlt;        ///< asphalt, alternate bands
    Color m_rumbleA;        ///< kerb colour one
    Color m_rumbleB;        ///< kerb colour two
    Color m_line;           ///< lane markings
    Color m_shoulder;       ///< gravel beside the kerb
    Color m_water;          ///< sea or ponds
    float m_lineGlow;       ///< 0..1 emissive markings (neon)
    float m_gridGlow;       ///< 0..1 glowing ground grid (neon)
};

/** @brief How the land beside the road rises and falls. */
struct Terrain
{
    float m_hills;       ///< amplitude of the far hills (m)
    float m_wallLeft;    ///< cliff or embankment height on the left (m, negative: drops away)
    float m_wallRight;   ///< and on the right
    float m_waterLeft;   ///< 0..1 sea on the left
    float m_ponds;       ///< 0..1 ponds in the near ground
};

/** @brief A biome: its look and the style of its roads. */
struct BiomeDef
{
    const char           *m_name;          ///< shown when the stage begins
    Environment           m_env;           ///< light and air
    Surface               m_surface;       ///< colours
    Terrain               m_terrain;       ///< land shape
    float                 m_grip;          ///< tyre grip (1 dry asphalt)
    float                 m_curviness;     ///< 0..1 how often the road bends
    float                 m_sharpness;     ///< 0..1 how tight the bends are
    float                 m_hilliness;     ///< 0..1 how much it climbs and falls
    int                   m_lanes;         ///< lanes in one direction
    float                 m_traffic;       ///< traffic density factor
    bool                  m_railLeft;      ///< guard rail on the left
    bool                  m_railRight;     ///< and on the right
    float                 m_postSpacing;   ///< delineator posts every n metres (0: none)
    float                 m_lampSpacing;   ///< street lamps every n metres (0: none)
    std::vector<PropRule> m_props;         ///< scatter rules
};

/**
 * @brief The definition of a biome.
 * @param[in] biome  which one
 * @return its definition (static storage)
 */
const BiomeDef &biomeDef( BiomeE biome );

/**
 * @brief Blends two environments.
 * @param[in] a  at t = 0
 * @param[in] b  at t = 1
 * @param[in] t  the position between them
 * @return the blend
 */
Environment blendEnvironment( const Environment &a, const Environment &b, float t );

/**
 * @brief Blends two surfaces.
 * @param[in] a  at t = 0
 * @param[in] b  at t = 1
 * @param[in] t  the position between them
 * @return the blend
 */
Surface blendSurface( const Surface &a, const Surface &b, float t );

/**
 * @brief Blends two terrains.
 * @param[in] a  at t = 0
 * @param[in] b  at t = 1
 * @param[in] t  the position between them
 * @return the blend
 */
Terrain blendTerrain( const Terrain &a, const Terrain &b, float t );

} // namespace sunstreak
