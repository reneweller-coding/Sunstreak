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
    MEADOW,          ///< green valley in the morning
    FOREST,          ///< alpine pass, pines and steep hills
    DESERT,          ///< red canyon at noon
    SNOW,            ///< frozen peaks, falling snow, less grip
    COAST,           ///< sunset coast beside the sea
    CITY,            ///< motorway through a city at night
    MARSH,           ///< storm over a marsh: rain, fog, lightning
    NEON,            ///< synthwave night with a striped sun
    AUTUMN,          ///< amber woods in the golden hour
    SAKURA,          ///< cherry blossom hills, torii and pagodas
    JUNGLE,          ///< steaming rainforest
    SAVANNA,         ///< acacias at sunset
    VOLCANO,         ///< black rock, lava, ash sky
    AURORA,          ///< arctic night under the northern lights
    LAVENDER,        ///< Provence: lavender rows and cypresses
    TULIPS,          ///< Dutch polder: tulip fields, mills, canals
    HIGHLANDS,       ///< misty heather hills and ruins
    FJORD,           ///< cliffs, deep water, red cabins
    RICE,            ///< flooded paddies and bamboo
    SALT_FLATS,      ///< a white mirror under a deep sky
    MESA_SUNSET,     ///< buttes in the evening light
    SUBURB,          ///< suburbia on a summer afternoon
    HARBOR,          ///< cranes and containers at dusk
    NEON_RAIN,       ///< rainy cyberpunk city
    EXPRESSWAY,      ///< motorway at dawn, sound walls, sign gantries
    MOON,            ///< lunar highway, the Earth in the sky, low gravity
    MARS,            ///< the red planet
    CRYSTALS,        ///< twilight desert of glowing crystals
    FUNGI,           ///< giant glowing mushrooms
    CLOUDS,          ///< a ridge road above the clouds at sunrise
    ICEBERGS,        ///< polar bay with icebergs
    BAYOU,           ///< swamp at dusk, moss trees, fireflies
    WHEAT,           ///< golden wheat plains, barns and silos
    CLIFFS,          ///< chalk cliffs, the sea and a lighthouse
    HALLOWEEN,       ///< pumpkins and graves under an orange moon
    WINTER_VILLAGE,  ///< snowy village with festive lights
    RAINBOW,         ///< green ridge after the rain
    ECLIPSE,         ///< a total solar eclipse
    COUNT            ///< the number of biomes
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
    AUTUMN_TREE,   ///< deciduous tree in autumn colours
    BIRCH,         ///< white birch
    CHERRY,        ///< cherry tree in blossom
    TORII,         ///< red shrine gate
    LANTERN,       ///< stone lantern
    PAGODA,        ///< tiered tower
    JUNGLE_TREE,   ///< tall rainforest tree
    FERN,          ///< fern
    ACACIA,        ///< flat-topped acacia
    TERMITE_MOUND, ///< termite mound
    GRASS_TUFT,    ///< tuft of dry grass
    LAVA_ROCK,     ///< black rock with glowing cracks
    ICE_SPIKE,     ///< ice crystal spike
    CYPRESS,       ///< slim cypress
    DUTCH_MILL,    ///< traditional windmill
    RUIN,          ///< ruined tower
    STONE_WALL,    ///< dry stone wall along the road
    CABIN,         ///< red wooden cabin
    BAMBOO,        ///< bamboo cluster
    HUT,           ///< stilt hut
    BUTTE,         ///< tall slim rock tower
    MAILBOX,       ///< mailbox at a driveway
    CONTAINERS,    ///< stacked shipping containers
    CRANE,         ///< harbour crane
    CHIMNEY,       ///< factory chimney
    NEON_TOWER,    ///< high-rise with neon strips
    CRATER,        ///< crater rim
    DOME,          ///< habitat dome
    ANTENNA,       ///< dish antenna
    CRYSTAL,       ///< glowing crystal cluster
    MUSHROOM,      ///< giant mushroom
    GLOW_SHROOM,   ///< small glowing mushrooms
    ICEBERG,       ///< iceberg (on the water)
    MOSS_TREE,     ///< swamp cypress with hanging moss
    FIREFLIES,     ///< a swarm of fireflies
    BARN,          ///< red barn
    SILO,          ///< grain silo
    LIGHTHOUSE,    ///< lighthouse
    PUMPKIN,       ///< glowing jack-o'-lantern
    GRAVESTONE,    ///< gravestone
    LIGHT_TREE,    ///< pine with festive lights
    SNOW_HOUSE,    ///< house with a snowy roof and lit windows
    COUNT          ///< the number of kinds
};

/** @brief One rule for scattering props beside the road. */
struct PropRule
{
    PropKindE m_kind;            ///< what to place
    float     m_chance;          ///< probability per segment and side
    float     m_nearest;         ///< least distance from the road's edge (m)
    float     m_farthest;        ///< largest distance from the road's edge (m)
    float     m_scaleMin;        ///< smallest scale
    float     m_scaleMax;        ///< largest scale
    bool      m_onWater = false; ///< floats on the sea (icebergs): placed only on a water side, at sea level
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
    float m_aurora;             ///< 0..1 northern lights
    float m_rainbow;            ///< 0..1 rainbow opposite the sun
    float m_eclipse;            ///< 0..1 the moon covers the sun (corona)
    float m_planetSize;         ///< apparent radius of a planet in the sky (degrees, 0: none)
    float m_planetElevation;    ///< its height (degrees)
    float m_planetAzimuth;      ///< its direction (degrees)
    Color m_planetColor;        ///< its colour
};

/** @brief Colours of the road and the ground. */
struct Surface
{
    Color m_groundA;            ///< ground, even stripes
    Color m_groundB;            ///< ground, odd stripes
    Color m_groundFar;          ///< ground far from the road
    Color m_road;               ///< asphalt
    Color m_roadAlt;            ///< asphalt, alternate bands
    Color m_rumbleA;            ///< kerb colour one
    Color m_rumbleB;            ///< kerb colour two
    Color m_line;               ///< lane markings
    Color m_shoulder;           ///< gravel beside the kerb
    Color m_water;              ///< sea or ponds
    float m_lineGlow;           ///< 0..1 emissive markings (neon)
    float m_gridGlow;           ///< 0..1 glowing ground grid (neon)
    Color m_fieldA{};           ///< field colour one (lavender, tulips, wheat)
    Color m_fieldB{};           ///< field colour two
    Color m_fieldC{};           ///< field colour three
    float m_fields = 0.0f;      ///< 0..1 share of the land laid out as fields
    float m_waterGlow = 0.0f;   ///< 0..1 self-lit water (lava)
    float m_groundGloss = 0.0f; ///< 0..1 shiny ground (salt flats)
    float m_waterGloss = 0.95f; ///< 0..1 how mirror-like the water is (a sea of clouds is matte)
};

/** @brief How the land beside the road rises and falls. */
struct Terrain
{
    float m_hills;               ///< amplitude of the far hills (m)
    float m_wallLeft;            ///< cliff or embankment height on the left (m, negative: drops away)
    float m_wallRight;           ///< and on the right
    float m_waterLeft;           ///< 0..1 sea on the left
    float m_ponds;               ///< 0..1 ponds in the near ground
    float m_waterRight = 0.0f;   ///< 0..1 sea on the right
    float m_seaLevel = -4.5f;    ///< the sea's height below the road (m)
};

/** @brief A biome: its look and the style of its roads. */
struct BiomeDef
{
    const char           *m_name;               ///< shown when the stage begins
    Environment           m_env;                ///< light and air
    Surface               m_surface;            ///< colours
    Terrain               m_terrain;            ///< land shape
    float                 m_grip;               ///< tyre grip (1 dry asphalt)
    float                 m_curviness;          ///< 0..1 how often the road bends
    float                 m_sharpness;          ///< 0..1 how tight the bends are
    float                 m_hilliness;          ///< 0..1 how much it climbs and falls
    int                   m_lanes;              ///< lanes in one direction
    float                 m_traffic;            ///< traffic density factor
    bool                  m_railLeft;           ///< guard rail on the left
    bool                  m_railRight;          ///< and on the right
    float                 m_postSpacing;        ///< delineator posts every n metres (0: none)
    float                 m_lampSpacing;        ///< street lamps every n metres (0: none)
    std::vector<PropRule> m_props;              ///< scatter rules
    float                 m_gravity = 1.0f;     ///< gravity factor (the Moon: low)
    bool                  m_walls = false;      ///< the rails are tall concrete sound walls
    float                 m_gantrySpacing = 0;  ///< overhead sign gantries every n metres (0: none)
    int                   m_station = 0;        ///< the radio station this biome plays in automatic mode
    float                 m_tunnels = 0.0f;     ///< chance that a stretch runs through a tunnel
    float                 m_bridges = 0.0f;     ///< chance that a straight crosses a valley on a bridge
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
