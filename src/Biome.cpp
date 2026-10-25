/**
 * @file Biome.cpp
 * @brief The biomes and their blending.
 * @author Rene Weller
 */
#include "Biome.h"
#include "Data.h"

#include "MathUtil.h"

#include <array>
#include <utility>

namespace sunstreak
{

namespace
{

/**
 * @brief Shorthand for an opaque colour.
 * @param[in] r  red
 * @param[in] g  green
 * @param[in] b  blue
 * @return the colour
 */
constexpr Color rgb( int r, int g, int b )
{
    return Color{ static_cast<unsigned char>( r ), static_cast<unsigned char>( g ), static_cast<unsigned char>( b ),
                  255 };
}

/**
 * @brief An environment with the values most biomes share.
 * @return the defaults
 */
Environment baseEnvironment()
{
    Environment e{};
    e.m_sunIntensity = 1.0f;
    e.m_sunElevation = 35.0f;
    e.m_sunAzimuth = 20.0f;
    e.m_sunSize = 1.6f;
    e.m_sunStripes = 0.0f;
    e.m_fogDensity = 0.0012f;
    e.m_mountainHeight = 4.0f;
    e.m_cloudAmount = 0.35f;
    e.m_cloudColor = rgb( 255, 255, 255 );
    e.m_stars = 0.0f;
    e.m_headlights = 0.0f;
    e.m_rain = 0.0f;
    e.m_snow = 0.0f;
    e.m_lightning = 0.0f;
    e.m_exposure = 1.0f;
    e.m_saturation = 1.08f;
    e.m_bloom = 0.6f;
    e.m_planetElevation = 20.0f;
    e.m_planetAzimuth = 30.0f;
    return e;
}

/**
 * @brief The common part of an environment in one call.
 * @param[in] zenith     sky straight up
 * @param[in] horizon    sky at the horizon
 * @param[in] sun        sun colour
 * @param[in] intensity  sun strength
 * @param[in] elevation  sun height (degrees)
 * @param[in] azimuth    sun direction (degrees)
 * @param[in] ambTop     light from above
 * @param[in] ambBottom  light from below
 * @param[in] fog        haze colour
 * @param[in] density    haze per metre
 * @param[in] nearRange  nearer mountains
 * @param[in] farRange   farther mountains
 * @param[in] height     mountain height (degrees)
 * @return the environment (the rest as in baseEnvironment())
 */
Environment sky( Color zenith, Color horizon, Color sun, float intensity, float elevation, float azimuth, Color ambTop,
                 Color ambBottom, Color fog, float density, Color nearRange, Color farRange, float height )
{
    Environment e = baseEnvironment();
    e.m_skyZenith = zenith;
    e.m_skyHorizon = horizon;
    e.m_sunColor = sun;
    e.m_sunIntensity = intensity;
    e.m_sunElevation = elevation;
    e.m_sunAzimuth = azimuth;
    e.m_ambientTop = ambTop;
    e.m_ambientBottom = ambBottom;
    e.m_fogColor = fog;
    e.m_fogDensity = density;
    e.m_mountainNear = nearRange;
    e.m_mountainFar = farRange;
    e.m_mountainHeight = height;
    return e;
}

/**
 * @brief The colours of road and ground in one call.
 * @param[in] a         ground stripes one
 * @param[in] b         ground stripes two
 * @param[in] farGround far ground
 * @param[in] road      asphalt
 * @param[in] rumbleA   kerb one
 * @param[in] rumbleB   kerb two
 * @param[in] line      markings
 * @param[in] shoulder  verge (and steep rock faces)
 * @param[in] water     water
 * @return the surface
 */
Surface ground( Color a, Color b, Color farGround, Color road, Color rumbleA, Color rumbleB, Color line, Color shoulder, Color water )
{
    Surface s{};
    s.m_groundA = a;
    s.m_groundB = b;
    s.m_groundFar = farGround;
    s.m_road = road;
    s.m_roadAlt = scaleColor( road, 0.93f );
    s.m_rumbleA = rumbleA;
    s.m_rumbleB = rumbleB;
    s.m_line = line;
    s.m_shoulder = shoulder;
    s.m_water = water;
    return s;
}

/**
 * @brief The road's character in one call.
 * @param[out] d        the biome
 * @param[in]  grip     tyre grip
 * @param[in]  curvy    how often it bends
 * @param[in]  sharp    how tight
 * @param[in]  hilly    how much it climbs
 * @param[in]  lanes    lanes
 * @param[in]  traffic  traffic density
 * @param[in]  station  radio station in automatic mode
 */
void style( BiomeDef &d, float grip, float curvy, float sharp, float hilly, int lanes, float traffic, int station )
{
    d.m_grip = grip;
    d.m_curviness = curvy;
    d.m_sharpness = sharp;
    d.m_hilliness = hilly;
    d.m_lanes = lanes;
    d.m_traffic = traffic;
    d.m_station = station;
}

/**
 * @brief Builds the table of biomes: the compiled ones, then the places of the data files.
 * @return all of them, indexed by BiomeE (and the numbers past it)
 */
std::vector<BiomeDef> makeBiomes()
{
    std::vector<BiomeDef> b( static_cast<size_t>( BiomeE::COUNT ) );

    { // Green valley: the gentle first stage.
        BiomeDef &d = b[static_cast<size_t>( BiomeE::MEADOW )];
        d.m_name = "GREEN VALLEY";
        Environment e = baseEnvironment();
        e.m_skyZenith = rgb( 58, 120, 214 );
        e.m_skyHorizon = rgb( 186, 214, 240 );
        e.m_sunColor = rgb( 255, 242, 214 );
        e.m_sunIntensity = 1.25f;
        e.m_sunElevation = 32.0f;
        e.m_ambientTop = rgb( 120, 150, 190 );
        e.m_ambientBottom = rgb( 90, 100, 70 );
        e.m_fogColor = rgb( 186, 210, 232 );
        e.m_fogDensity = 0.0010f;
        e.m_mountainNear = rgb( 96, 130, 120 );
        e.m_mountainFar = rgb( 140, 170, 200 );
        e.m_mountainHeight = 3.5f;
        d.m_env = e;
        d.m_surface = { rgb( 98, 164, 62 ), rgb( 84, 148, 54 ), rgb( 106, 150, 70 ), rgb( 92, 94, 104 ),
                        rgb( 86, 88, 98 ), rgb( 214, 40, 40 ), rgb( 240, 240, 240 ), rgb( 236, 236, 236 ),
                        rgb( 158, 148, 118 ), rgb( 60, 110, 160 ), 0.0f, 0.0f };
        d.m_terrain = { 14.0f, 4.0f, 4.0f, 0.0f, 0.15f };
        d.m_grip = 1.0f;
        d.m_curviness = 0.45f;
        d.m_sharpness = 0.3f;
        d.m_hilliness = 0.45f;
        d.m_lanes = 3;
        d.m_traffic = 1.0f;
        d.m_postSpacing = 40.0f;
        d.m_props = { { PropKindE::OAK, 0.10f, 6.0f, 60.0f, 0.8f, 1.4f },
                      { PropKindE::BUSH, 0.10f, 3.0f, 40.0f, 0.7f, 1.3f },
                      { PropKindE::FENCE, 0.55f, 4.0f, 4.0f, 1.0f, 1.0f },
                      { PropKindE::HAYBALE, 0.025f, 12.0f, 50.0f, 0.9f, 1.1f },
                      { PropKindE::HOUSE, 0.010f, 25.0f, 90.0f, 0.9f, 1.3f },
                      { PropKindE::WINDMILL, 0.006f, 60.0f, 200.0f, 1.0f, 1.3f },
                      { PropKindE::OAK, 0.05f, 60.0f, 220.0f, 1.0f, 1.8f } };
    }
    { // Alpine pass: narrow, curvy, steep, dense pines.
        BiomeDef &d = b[static_cast<size_t>( BiomeE::FOREST )];
        d.m_name = "ALPINE PASS";
        Environment e = baseEnvironment();
        e.m_skyZenith = rgb( 46, 100, 186 );
        e.m_skyHorizon = rgb( 168, 198, 224 );
        e.m_sunColor = rgb( 255, 236, 206 );
        e.m_sunIntensity = 1.15f;
        e.m_sunElevation = 26.0f;
        e.m_sunAzimuth = -35.0f;
        e.m_ambientTop = rgb( 110, 140, 180 );
        e.m_ambientBottom = rgb( 60, 76, 60 );
        e.m_fogColor = rgb( 160, 186, 204 );
        e.m_fogDensity = 0.0015f;
        e.m_mountainNear = rgb( 70, 92, 104 );
        e.m_mountainFar = rgb( 150, 172, 196 );
        e.m_mountainHeight = 9.0f;
        e.m_cloudAmount = 0.5f;
        d.m_env = e;
        d.m_surface = { rgb( 58, 118, 54 ), rgb( 50, 104, 48 ), rgb( 46, 92, 52 ), rgb( 88, 90, 98 ),
                        rgb( 82, 84, 92 ), rgb( 220, 50, 40 ), rgb( 240, 240, 240 ), rgb( 236, 236, 230 ),
                        rgb( 130, 124, 110 ), rgb( 50, 90, 120 ), 0.0f, 0.0f };
        d.m_terrain = { 40.0f, 14.0f, -6.0f, 0.0f, 0.0f };
        d.m_grip = 1.0f;
        d.m_curviness = 0.8f;
        d.m_sharpness = 0.65f;
        d.m_hilliness = 0.9f;
        d.m_lanes = 2;
        d.m_traffic = 0.8f;
        d.m_postSpacing = 25.0f;
        d.m_props = { { PropKindE::PINE, 0.55f, 3.5f, 45.0f, 0.8f, 1.5f },
                      { PropKindE::PINE, 0.25f, 45.0f, 220.0f, 1.0f, 2.0f },
                      { PropKindE::ROCK, 0.08f, 3.0f, 40.0f, 0.6f, 1.4f },
                      { PropKindE::BOULDER, 0.02f, 10.0f, 80.0f, 0.8f, 1.5f } };
    }
    { // Red canyon: wide open, fast, hot light.
        BiomeDef &d = b[static_cast<size_t>( BiomeE::DESERT )];
        d.m_name = "RED CANYON";
        Environment e = baseEnvironment();
        e.m_skyZenith = rgb( 48, 110, 206 );
        e.m_skyHorizon = rgb( 236, 204, 160 );
        e.m_sunColor = rgb( 255, 236, 200 );
        e.m_sunIntensity = 1.45f;
        e.m_sunElevation = 58.0f;
        e.m_sunAzimuth = 10.0f;
        e.m_ambientTop = rgb( 140, 150, 180 );
        e.m_ambientBottom = rgb( 140, 96, 64 );
        e.m_fogColor = rgb( 232, 200, 160 );
        e.m_fogDensity = 0.0008f;
        e.m_mountainNear = rgb( 176, 98, 70 );
        e.m_mountainFar = rgb( 214, 160, 128 );
        e.m_mountainHeight = 5.0f;
        e.m_cloudAmount = 0.1f;
        e.m_saturation = 1.12f;
        d.m_env = e;
        d.m_surface = { rgb( 216, 152, 94 ), rgb( 204, 140, 86 ), rgb( 194, 124, 78 ), rgb( 112, 102, 98 ),
                        rgb( 106, 96, 92 ), rgb( 230, 60, 30 ), rgb( 245, 236, 220 ), rgb( 250, 230, 120 ),
                        rgb( 196, 160, 120 ), rgb( 60, 120, 160 ), 0.0f, 0.0f };
        d.m_terrain = { 20.0f, 30.0f, 22.0f, 0.0f, 0.0f };
        d.m_grip = 0.96f;
        d.m_curviness = 0.4f;
        d.m_sharpness = 0.4f;
        d.m_hilliness = 0.5f;
        d.m_lanes = 3;
        d.m_traffic = 0.8f;
        d.m_postSpacing = 50.0f;
        d.m_props = { { PropKindE::CACTUS, 0.10f, 4.0f, 70.0f, 0.7f, 1.4f },
                      { PropKindE::ROCK, 0.10f, 3.0f, 60.0f, 0.6f, 1.6f },
                      { PropKindE::BUSH, 0.05f, 3.0f, 40.0f, 0.5f, 0.9f },
                      { PropKindE::POLE, 0.03f, 6.0f, 6.0f, 1.0f, 1.0f },
                      { PropKindE::MESA, 0.012f, 70.0f, 260.0f, 1.0f, 2.2f },
                      { PropKindE::BOULDER, 0.02f, 15.0f, 100.0f, 0.8f, 1.8f } };
    }
    { // Frozen peaks: overcast, snowfall, slippery.
        BiomeDef &d = b[static_cast<size_t>( BiomeE::SNOW )];
        d.m_name = "FROZEN PEAKS";
        Environment e = baseEnvironment();
        e.m_skyZenith = rgb( 128, 146, 170 );
        e.m_skyHorizon = rgb( 206, 214, 226 );
        e.m_sunColor = rgb( 226, 230, 242 );
        e.m_sunIntensity = 0.75f;
        e.m_sunElevation = 18.0f;
        e.m_sunAzimuth = 40.0f;
        e.m_sunSize = 2.4f;
        e.m_ambientTop = rgb( 170, 180, 200 );
        e.m_ambientBottom = rgb( 150, 156, 170 );
        e.m_fogColor = rgb( 200, 208, 220 );
        e.m_fogDensity = 0.0022f;
        e.m_mountainNear = rgb( 170, 180, 196 );
        e.m_mountainFar = rgb( 196, 204, 218 );
        e.m_mountainHeight = 10.0f;
        e.m_cloudAmount = 0.85f;
        e.m_cloudColor = rgb( 220, 224, 232 );
        e.m_snow = 1.0f;
        e.m_saturation = 0.95f;
        e.m_bloom = 0.5f;
        d.m_env = e;
        d.m_surface = { rgb( 236, 240, 248 ), rgb( 222, 228, 240 ), rgb( 214, 222, 236 ), rgb( 118, 122, 132 ),
                        rgb( 112, 116, 126 ), rgb( 40, 90, 200 ), rgb( 240, 240, 240 ), rgb( 230, 230, 230 ),
                        rgb( 180, 184, 192 ), rgb( 70, 100, 130 ), 0.0f, 0.0f };
        d.m_terrain = { 34.0f, 10.0f, 10.0f, 0.0f, 0.0f };
        d.m_grip = 0.78f;
        d.m_curviness = 0.6f;
        d.m_sharpness = 0.5f;
        d.m_hilliness = 0.7f;
        d.m_lanes = 2;
        d.m_traffic = 0.7f;
        d.m_postSpacing = 20.0f;
        d.m_props = { { PropKindE::SNOW_PINE, 0.40f, 4.0f, 60.0f, 0.8f, 1.6f },
                      { PropKindE::SNOW_PINE, 0.15f, 60.0f, 220.0f, 1.0f, 2.0f },
                      { PropKindE::SNOWBANK, 0.25f, 0.5f, 3.0f, 0.8f, 1.4f },
                      { PropKindE::ROCK, 0.05f, 5.0f, 50.0f, 0.6f, 1.4f } };
    }
    { // Sunset coast: the sea on the left, palms, warm low sun ahead.
        BiomeDef &d = b[static_cast<size_t>( BiomeE::COAST )];
        d.m_name = "SUNSET COAST";
        Environment e = baseEnvironment();
        e.m_skyZenith = rgb( 52, 54, 130 );
        e.m_skyHorizon = rgb( 255, 150, 90 );
        e.m_sunColor = rgb( 255, 176, 110 );
        e.m_sunIntensity = 1.2f;
        e.m_sunElevation = 6.0f;
        e.m_sunAzimuth = -12.0f;
        e.m_sunSize = 2.6f;
        e.m_ambientTop = rgb( 120, 100, 160 );
        e.m_ambientBottom = rgb( 120, 80, 70 );
        e.m_fogColor = rgb( 236, 150, 120 );
        e.m_fogDensity = 0.0011f;
        e.m_mountainNear = rgb( 96, 60, 110 );
        e.m_mountainFar = rgb( 170, 100, 130 );
        e.m_mountainHeight = 3.0f;
        e.m_cloudAmount = 0.45f;
        e.m_cloudColor = rgb( 255, 170, 150 );
        e.m_headlights = 0.25f;
        e.m_saturation = 1.15f;
        e.m_bloom = 0.8f;
        d.m_env = e;
        d.m_surface = { rgb( 206, 190, 140 ), rgb( 194, 178, 130 ), rgb( 120, 150, 80 ), rgb( 96, 92, 104 ),
                        rgb( 90, 86, 98 ), rgb( 230, 230, 230 ), rgb( 40, 120, 210 ), rgb( 245, 236, 220 ),
                        rgb( 200, 186, 150 ), rgb( 40, 70, 130 ), 0.0f, 0.0f };
        d.m_terrain = { 18.0f, -12.0f, 14.0f, 1.0f, 0.0f };
        d.m_grip = 1.0f;
        d.m_curviness = 0.6f;
        d.m_sharpness = 0.35f;
        d.m_hilliness = 0.35f;
        d.m_lanes = 3;
        d.m_traffic = 1.1f;
        d.m_railLeft = true;
        d.m_lampSpacing = 60.0f;
        d.m_props = { { PropKindE::PALM, 0.14f, 3.0f, 30.0f, 0.8f, 1.3f },
                      { PropKindE::BUSH, 0.08f, 3.0f, 30.0f, 0.6f, 1.1f },
                      { PropKindE::HOUSE, 0.025f, 18.0f, 80.0f, 0.8f, 1.2f },
                      { PropKindE::PALM, 0.05f, 30.0f, 120.0f, 1.0f, 1.5f } };
    }
    { // Night metro: four lanes, lamps, towers, heavy traffic.
        BiomeDef &d = b[static_cast<size_t>( BiomeE::CITY )];
        d.m_name = "NIGHT METRO";
        Environment e = baseEnvironment();
        e.m_skyZenith = rgb( 6, 8, 26 );
        e.m_skyHorizon = rgb( 46, 34, 80 );
        e.m_sunColor = rgb( 160, 176, 230 );
        e.m_sunIntensity = 0.22f;
        e.m_sunElevation = 34.0f;
        e.m_sunAzimuth = 25.0f;
        e.m_sunSize = 1.2f;
        e.m_ambientTop = rgb( 46, 50, 86 );
        e.m_ambientBottom = rgb( 30, 26, 40 );
        e.m_fogColor = rgb( 34, 30, 64 );
        e.m_fogDensity = 0.0016f;
        e.m_mountainNear = rgb( 20, 18, 40 );
        e.m_mountainFar = rgb( 40, 34, 70 );
        e.m_mountainHeight = 2.0f;
        e.m_cloudAmount = 0.2f;
        e.m_cloudColor = rgb( 60, 50, 90 );
        e.m_stars = 0.4f;
        e.m_headlights = 1.0f;
        e.m_exposure = 1.15f;
        e.m_saturation = 1.15f;
        e.m_bloom = 1.1f;
        d.m_env = e;
        d.m_surface = { rgb( 44, 46, 56 ), rgb( 40, 42, 52 ), rgb( 30, 32, 40 ), rgb( 52, 52, 62 ),
                        rgb( 48, 48, 58 ), rgb( 220, 200, 60 ), rgb( 40, 40, 44 ), rgb( 236, 230, 200 ),
                        rgb( 60, 60, 66 ), rgb( 20, 30, 50 ), 0.0f, 0.0f };
        d.m_terrain = { 4.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        d.m_grip = 1.0f;
        d.m_curviness = 0.35f;
        d.m_sharpness = 0.3f;
        d.m_hilliness = 0.3f;
        d.m_lanes = 4;
        d.m_traffic = 1.5f;
        d.m_railLeft = true;
        d.m_railRight = true;
        d.m_lampSpacing = 35.0f;
        d.m_props = { { PropKindE::BUILDING, 0.35f, 12.0f, 70.0f, 0.8f, 1.6f },
                      { PropKindE::TOWER, 0.08f, 50.0f, 220.0f, 0.8f, 1.6f },
                      { PropKindE::BILLBOARD, 0.03f, 6.0f, 14.0f, 1.0f, 1.2f } };
    }
    { // Storm marsh: rain, thick fog, flashes, slippery.
        BiomeDef &d = b[static_cast<size_t>( BiomeE::MARSH )];
        d.m_name = "STORM MARSH";
        Environment e = baseEnvironment();
        e.m_skyZenith = rgb( 36, 44, 54 );
        e.m_skyHorizon = rgb( 92, 102, 108 );
        e.m_sunColor = rgb( 180, 190, 200 );
        e.m_sunIntensity = 0.35f;
        e.m_sunElevation = 20.0f;
        e.m_sunSize = 3.0f;
        e.m_ambientTop = rgb( 90, 100, 110 );
        e.m_ambientBottom = rgb( 50, 56, 50 );
        e.m_fogColor = rgb( 84, 94, 98 );
        e.m_fogDensity = 0.0034f;
        e.m_mountainNear = rgb( 60, 68, 72 );
        e.m_mountainFar = rgb( 80, 90, 96 );
        e.m_mountainHeight = 2.0f;
        e.m_cloudAmount = 1.0f;
        e.m_cloudColor = rgb( 70, 78, 86 );
        e.m_headlights = 0.8f;
        e.m_rain = 1.0f;
        e.m_lightning = 1.0f;
        e.m_saturation = 0.9f;
        e.m_bloom = 0.8f;
        d.m_env = e;
        d.m_surface = { rgb( 72, 88, 56 ), rgb( 64, 80, 50 ), rgb( 60, 72, 52 ), rgb( 62, 64, 70 ),
                        rgb( 58, 60, 66 ), rgb( 230, 230, 230 ), rgb( 40, 40, 40 ), rgb( 230, 230, 220 ),
                        rgb( 90, 90, 80 ), rgb( 46, 60, 64 ), 0.0f, 0.0f };
        d.m_terrain = { 6.0f, -2.0f, -2.0f, 0.0f, 1.0f };
        d.m_grip = 0.86f;
        d.m_curviness = 0.6f;
        d.m_sharpness = 0.5f;
        d.m_hilliness = 0.25f;
        d.m_lanes = 2;
        d.m_traffic = 0.7f;
        d.m_postSpacing = 25.0f;
        d.m_props = { { PropKindE::DEAD_TREE, 0.12f, 4.0f, 80.0f, 0.8f, 1.5f },
                      { PropKindE::REEDS, 0.45f, 2.0f, 40.0f, 0.7f, 1.3f },
                      { PropKindE::POLE, 0.06f, 7.0f, 7.0f, 1.0f, 1.0f },
                      { PropKindE::BUSH, 0.06f, 3.0f, 40.0f, 0.6f, 1.0f } };
    }
    { // Neon horizon: synthwave grid, striped sun, glowing lines.
        BiomeDef &d = b[static_cast<size_t>( BiomeE::NEON )];
        d.m_name = "NEON HORIZON";
        Environment e = baseEnvironment();
        e.m_skyZenith = rgb( 14, 0, 36 );
        e.m_skyHorizon = rgb( 230, 50, 140 );
        e.m_sunColor = rgb( 255, 120, 80 );
        e.m_sunIntensity = 0.6f;
        e.m_sunElevation = 9.0f;
        e.m_sunAzimuth = 0.0f;
        e.m_sunSize = 7.5f;
        e.m_sunStripes = 1.0f;
        e.m_ambientTop = rgb( 70, 30, 110 );
        e.m_ambientBottom = rgb( 40, 10, 50 );
        e.m_fogColor = rgb( 70, 14, 80 );
        e.m_fogDensity = 0.0011f;
        e.m_mountainNear = rgb( 26, 6, 46 );
        e.m_mountainFar = rgb( 90, 20, 110 );
        e.m_mountainHeight = 5.0f;
        e.m_cloudAmount = 0.0f;
        e.m_stars = 1.0f;
        e.m_headlights = 0.6f;
        e.m_exposure = 1.1f;
        e.m_saturation = 1.2f;
        e.m_bloom = 1.0f;
        d.m_env = e;
        d.m_surface = { rgb( 16, 8, 34 ), rgb( 20, 10, 40 ), rgb( 14, 6, 30 ), rgb( 24, 18, 42 ),
                        rgb( 28, 22, 48 ), rgb( 255, 40, 200 ), rgb( 40, 220, 255 ), rgb( 60, 230, 255 ),
                        rgb( 34, 20, 60 ), rgb( 30, 10, 60 ), 1.0f, 1.0f };
        d.m_terrain = { 26.0f, 6.0f, 6.0f, 0.0f, 0.0f };
        d.m_grip = 1.0f;
        d.m_curviness = 0.5f;
        d.m_sharpness = 0.45f;
        d.m_hilliness = 0.65f;
        d.m_lanes = 3;
        d.m_traffic = 0.9f;
        d.m_postSpacing = 20.0f;
        d.m_props = { { PropKindE::NEON_PILLAR, 0.06f, 4.0f, 40.0f, 0.8f, 1.4f },
                      { PropKindE::PYRAMID, 0.012f, 40.0f, 240.0f, 1.0f, 2.5f } };
    }

    // Radio stations of the first eight (0 Sunset FM, 1 Turbo House, 2 Amiga Techno, 3 Trance Gate,
    // 4 Jungle Pressure, 5 8-Bit Racer, 6 Fusion Drive, 7 Big Beat).
    b[static_cast<size_t>( BiomeE::MEADOW )].m_station = 6;
    b[static_cast<size_t>( BiomeE::FOREST )].m_station = 2;
    b[static_cast<size_t>( BiomeE::DESERT )].m_station = 7;
    b[static_cast<size_t>( BiomeE::SNOW )].m_station = 3;
    b[static_cast<size_t>( BiomeE::COAST )].m_station = 0;
    b[static_cast<size_t>( BiomeE::CITY )].m_station = 1;
    b[static_cast<size_t>( BiomeE::MARSH )].m_station = 4;
    b[static_cast<size_t>( BiomeE::NEON )].m_station = 0;

    auto at = [&]( BiomeE e ) -> BiomeDef & { return b[static_cast<size_t>( e )]; };

    { // Amber woods: autumn in the golden hour.
        BiomeDef &d = at( BiomeE::AUTUMN );
        d.m_name = "AMBER WOODS";
        d.m_env = sky( rgb( 70, 110, 190 ), rgb( 250, 190, 130 ), rgb( 255, 200, 140 ), 1.2f, 12.0f, -25.0f, rgb( 130, 120, 140 ),
                       rgb( 110, 80, 50 ), rgb( 230, 180, 140 ), 0.0013f, rgb( 120, 80, 70 ), rgb( 190, 140, 130 ), 5.0f );
        d.m_env.m_sunSize = 2.2f;
        d.m_env.m_cloudAmount = 0.4f;
        d.m_env.m_cloudColor = rgb( 255, 200, 170 );
        d.m_env.m_saturation = 1.15f;
        d.m_env.m_bloom = 0.8f;
        d.m_surface = ground( rgb( 156, 120, 48 ), rgb( 140, 106, 42 ), rgb( 130, 96, 50 ), rgb( 88, 86, 90 ), rgb( 220, 60, 30 ),
                              rgb( 240, 240, 240 ), rgb( 240, 240, 240 ), rgb( 150, 120, 90 ), rgb( 60, 90, 120 ) );
        d.m_terrain = { 22.0f, 6.0f, 6.0f, 0.0f, 0.05f };
        style( d, 0.97f, 0.6f, 0.45f, 0.6f, 2, 0.9f, 6 );
        d.m_postSpacing = 30.0f;
        d.m_props = { { PropKindE::AUTUMN_TREE, 0.45f, 3.5f, 50.0f, 0.8f, 1.4f },
                      { PropKindE::BIRCH, 0.15f, 3.0f, 40.0f, 0.8f, 1.3f },
                      { PropKindE::AUTUMN_TREE, 0.2f, 50.0f, 220.0f, 1.0f, 1.8f },
                      { PropKindE::BUSH, 0.08f, 3.0f, 30.0f, 0.6f, 1.1f },
                      { PropKindE::FENCE, 0.2f, 4.0f, 4.0f, 1.0f, 1.0f } };
    }
    { // Sakura hills: cherry blossom, torii, pagodas.
        BiomeDef &d = at( BiomeE::SAKURA );
        d.m_name = "SAKURA HILLS";
        d.m_env = sky( rgb( 110, 160, 230 ), rgb( 235, 220, 235 ), rgb( 255, 240, 230 ), 1.15f, 30.0f, 30.0f, rgb( 170, 160, 190 ),
                       rgb( 120, 110, 100 ), rgb( 230, 215, 230 ), 0.0012f, rgb( 150, 140, 170 ), rgb( 200, 190, 215 ), 7.0f );
        d.m_env.m_cloudAmount = 0.3f;
        d.m_env.m_saturation = 1.12f;
        d.m_surface = ground( rgb( 120, 170, 90 ), rgb( 110, 158, 82 ), rgb( 110, 150, 100 ), rgb( 96, 94, 104 ), rgb( 220, 50, 60 ),
                              rgb( 245, 245, 245 ), rgb( 245, 245, 245 ), rgb( 190, 170, 160 ), rgb( 90, 140, 180 ) );
        d.m_terrain = { 24.0f, 8.0f, 4.0f, 0.0f, 0.1f };
        style( d, 1.0f, 0.6f, 0.5f, 0.6f, 2, 0.8f, 5 );
        d.m_props = { { PropKindE::CHERRY, 0.4f, 3.0f, 50.0f, 0.8f, 1.4f },
                      { PropKindE::CHERRY, 0.15f, 50.0f, 200.0f, 1.0f, 1.6f },
                      { PropKindE::TORII, 0.02f, 3.0f, 6.0f, 1.0f, 1.0f },
                      { PropKindE::LANTERN, 0.08f, 1.5f, 4.0f, 0.9f, 1.1f },
                      { PropKindE::PAGODA, 0.008f, 30.0f, 120.0f, 1.0f, 1.4f },
                      { PropKindE::BAMBOO, 0.05f, 4.0f, 30.0f, 0.8f, 1.2f } };
    }
    { // Jungle run: steaming rainforest with a passing shower.
        BiomeDef &d = at( BiomeE::JUNGLE );
        d.m_name = "JUNGLE RUN";
        d.m_env = sky( rgb( 90, 140, 170 ), rgb( 200, 215, 200 ), rgb( 255, 245, 210 ), 1.0f, 45.0f, 10.0f, rgb( 120, 150, 140 ),
                       rgb( 50, 70, 40 ), rgb( 170, 195, 180 ), 0.0028f, rgb( 60, 100, 80 ), rgb( 120, 150, 140 ), 8.0f );
        d.m_env.m_cloudAmount = 0.7f;
        d.m_env.m_cloudColor = rgb( 230, 235, 230 );
        d.m_env.m_rain = 0.35f;
        d.m_env.m_saturation = 1.15f;
        d.m_surface = ground( rgb( 60, 120, 40 ), rgb( 52, 108, 36 ), rgb( 40, 90, 40 ), rgb( 80, 78, 74 ), rgb( 230, 200, 40 ),
                              rgb( 40, 40, 40 ), rgb( 240, 230, 180 ), rgb( 120, 90, 60 ), rgb( 60, 90, 70 ) );
        d.m_terrain = { 30.0f, 10.0f, 10.0f, 0.0f, 0.2f };
        style( d, 0.9f, 0.75f, 0.6f, 0.6f, 2, 0.7f, 4 );
        d.m_props = { { PropKindE::JUNGLE_TREE, 0.45f, 4.0f, 60.0f, 0.8f, 1.4f },
                      { PropKindE::FERN, 0.5f, 1.0f, 20.0f, 0.8f, 1.5f },
                      { PropKindE::PALM, 0.15f, 3.0f, 40.0f, 0.9f, 1.3f },
                      { PropKindE::JUNGLE_TREE, 0.25f, 60.0f, 220.0f, 1.0f, 1.6f },
                      { PropKindE::BUSH, 0.2f, 2.0f, 30.0f, 0.8f, 1.4f } };
    }
    { // Golden savanna: acacias against a low sun.
        BiomeDef &d = at( BiomeE::SAVANNA );
        d.m_name = "GOLDEN SAVANNA";
        d.m_env = sky( rgb( 70, 80, 160 ), rgb( 255, 170, 90 ), rgb( 255, 160, 80 ), 1.2f, 5.0f, 15.0f, rgb( 150, 110, 120 ),
                       rgb( 140, 90, 50 ), rgb( 240, 170, 110 ), 0.0009f, rgb( 130, 80, 80 ), rgb( 200, 130, 110 ), 2.5f );
        d.m_env.m_sunSize = 3.0f;
        d.m_env.m_cloudAmount = 0.35f;
        d.m_env.m_cloudColor = rgb( 255, 170, 120 );
        d.m_env.m_saturation = 1.15f;
        d.m_env.m_bloom = 0.8f;
        d.m_surface = ground( rgb( 200, 160, 80 ), rgb( 190, 150, 72 ), rgb( 180, 140, 80 ), rgb( 100, 90, 84 ), rgb( 230, 230, 230 ),
                              rgb( 200, 60, 40 ), rgb( 245, 230, 200 ), rgb( 190, 150, 100 ), rgb( 80, 110, 120 ) );
        d.m_terrain = { 10.0f, 2.0f, 2.0f, 0.0f, 0.0f };
        style( d, 0.97f, 0.35f, 0.3f, 0.3f, 2, 0.6f, 7 );
        d.m_postSpacing = 50.0f;
        d.m_props = { { PropKindE::ACACIA, 0.07f, 6.0f, 120.0f, 0.9f, 1.5f },
                      { PropKindE::GRASS_TUFT, 0.6f, 1.0f, 30.0f, 0.8f, 1.4f },
                      { PropKindE::TERMITE_MOUND, 0.03f, 5.0f, 40.0f, 0.8f, 1.4f },
                      { PropKindE::ROCK, 0.03f, 4.0f, 60.0f, 0.6f, 1.2f },
                      { PropKindE::ACACIA, 0.03f, 120.0f, 300.0f, 1.0f, 1.8f } };
    }
    { // Volcano ridge: black rock, lava and an ash sky.
        BiomeDef &d = at( BiomeE::VOLCANO );
        d.m_name = "VOLCANO RIDGE";
        d.m_env = sky( rgb( 30, 20, 26 ), rgb( 140, 50, 30 ), rgb( 255, 120, 60 ), 0.5f, 10.0f, -20.0f, rgb( 80, 50, 50 ),
                       rgb( 90, 30, 20 ), rgb( 90, 40, 30 ), 0.0020f, rgb( 25, 18, 20 ), rgb( 70, 30, 25 ), 9.0f );
        d.m_env.m_sunSize = 2.5f;
        d.m_env.m_cloudAmount = 0.8f;
        d.m_env.m_cloudColor = rgb( 60, 40, 40 );
        d.m_env.m_headlights = 0.6f;
        d.m_env.m_lightning = 0.3f;
        d.m_env.m_exposure = 1.1f;
        d.m_env.m_bloom = 1.0f;
        d.m_surface = ground( rgb( 36, 32, 34 ), rgb( 30, 27, 30 ), rgb( 26, 22, 24 ), rgb( 44, 42, 46 ), rgb( 255, 100, 20 ),
                              rgb( 30, 30, 30 ), rgb( 255, 180, 60 ), rgb( 50, 40, 40 ), rgb( 255, 90, 20 ) );
        d.m_surface.m_waterGlow = 1.0f;
        d.m_surface.m_waterGloss = 0.2f;
        d.m_terrain = { 34.0f, 12.0f, 12.0f, 0.0f, 0.6f };
        style( d, 0.95f, 0.6f, 0.55f, 0.8f, 2, 0.5f, 4 );
        d.m_postSpacing = 25.0f;
        d.m_props = { { PropKindE::LAVA_ROCK, 0.2f, 3.0f, 60.0f, 0.7f, 1.6f },
                      { PropKindE::BOULDER, 0.04f, 10.0f, 80.0f, 0.8f, 1.5f },
                      { PropKindE::DEAD_TREE, 0.03f, 4.0f, 50.0f, 0.8f, 1.2f } };
    }
    { // Aurora tundra: arctic night under the northern lights.
        BiomeDef &d = at( BiomeE::AURORA );
        d.m_name = "AURORA TUNDRA";
        d.m_env = sky( rgb( 4, 10, 24 ), rgb( 20, 40, 60 ), rgb( 160, 190, 230 ), 0.3f, 25.0f, 40.0f, rgb( 50, 70, 100 ),
                       rgb( 60, 70, 90 ), rgb( 24, 40, 58 ), 0.0012f, rgb( 20, 30, 44 ), rgb( 40, 60, 80 ), 5.0f );
        d.m_env.m_sunSize = 1.4f;
        d.m_env.m_cloudAmount = 0.1f;
        d.m_env.m_stars = 1.0f;
        d.m_env.m_aurora = 1.0f;
        d.m_env.m_headlights = 1.0f;
        d.m_env.m_snow = 0.25f;
        d.m_env.m_exposure = 1.15f;
        d.m_env.m_bloom = 0.9f;
        d.m_surface = ground( rgb( 190, 205, 225 ), rgb( 178, 194, 215 ), rgb( 160, 178, 200 ), rgb( 70, 74, 84 ), rgb( 220, 60, 60 ),
                              rgb( 230, 230, 230 ), rgb( 240, 240, 240 ), rgb( 140, 150, 165 ), rgb( 20, 40, 60 ) );
        d.m_terrain = { 20.0f, 4.0f, 4.0f, 0.0f, 0.0f };
        style( d, 0.8f, 0.5f, 0.4f, 0.4f, 2, 0.5f, 3 );
        d.m_postSpacing = 20.0f;
        d.m_props = { { PropKindE::SNOW_PINE, 0.12f, 5.0f, 80.0f, 0.8f, 1.5f },
                      { PropKindE::ICE_SPIKE, 0.08f, 4.0f, 60.0f, 0.7f, 1.6f },
                      { PropKindE::SNOWBANK, 0.2f, 0.5f, 3.0f, 0.8f, 1.4f },
                      { PropKindE::ROCK, 0.03f, 5.0f, 50.0f, 0.6f, 1.2f } };
    }
    { // Lavender vale: Provence rows and cypresses.
        BiomeDef &d = at( BiomeE::LAVENDER );
        d.m_name = "LAVENDER VALE";
        d.m_env = sky( rgb( 60, 120, 210 ), rgb( 210, 215, 235 ), rgb( 255, 240, 215 ), 1.3f, 40.0f, 30.0f, rgb( 140, 140, 180 ),
                       rgb( 110, 90, 110 ), rgb( 210, 205, 230 ), 0.0010f, rgb( 130, 120, 160 ), rgb( 180, 170, 210 ), 3.5f );
        d.m_env.m_cloudAmount = 0.25f;
        d.m_env.m_saturation = 1.15f;
        d.m_surface = ground( rgb( 120, 150, 70 ), rgb( 110, 140, 64 ), rgb( 130, 120, 150 ), rgb( 100, 98, 106 ), rgb( 230, 230, 230 ),
                              rgb( 150, 90, 200 ), rgb( 245, 245, 245 ), rgb( 200, 180, 150 ), rgb( 80, 120, 170 ) );
        d.m_surface.m_fieldA = rgb( 140, 100, 200 );
        d.m_surface.m_fieldB = rgb( 120, 84, 180 );
        d.m_surface.m_fieldC = rgb( 150, 170, 80 );
        d.m_surface.m_fields = 0.85f;
        d.m_terrain = { 14.0f, 3.0f, 3.0f, 0.0f, 0.0f };
        style( d, 1.0f, 0.5f, 0.35f, 0.45f, 2, 0.8f, 6 );
        d.m_postSpacing = 40.0f;
        d.m_props = { { PropKindE::CYPRESS, 0.12f, 4.0f, 80.0f, 0.8f, 1.4f },
                      { PropKindE::HOUSE, 0.012f, 20.0f, 90.0f, 0.8f, 1.1f },
                      { PropKindE::OAK, 0.03f, 6.0f, 60.0f, 0.8f, 1.2f },
                      { PropKindE::STONE_WALL, 0.15f, 4.0f, 4.0f, 1.0f, 1.0f } };
    }
    { // Tulip polder: flat, colourful, windmills and canals.
        BiomeDef &d = at( BiomeE::TULIPS );
        d.m_name = "TULIP POLDER";
        d.m_env = sky( rgb( 70, 130, 220 ), rgb( 200, 220, 245 ), rgb( 255, 245, 225 ), 1.25f, 35.0f, -20.0f, rgb( 140, 160, 200 ),
                       rgb( 100, 110, 80 ), rgb( 200, 215, 235 ), 0.0011f, rgb( 120, 150, 170 ), rgb( 170, 195, 220 ), 1.2f );
        d.m_env.m_cloudAmount = 0.55f;
        d.m_env.m_saturation = 1.2f;
        d.m_surface = ground( rgb( 100, 170, 70 ), rgb( 90, 158, 64 ), rgb( 100, 160, 80 ), rgb( 96, 96, 104 ), rgb( 230, 120, 30 ),
                              rgb( 240, 240, 240 ), rgb( 245, 245, 245 ), rgb( 160, 150, 120 ), rgb( 70, 110, 160 ) );
        d.m_surface.m_fieldA = rgb( 230, 40, 60 );
        d.m_surface.m_fieldB = rgb( 250, 210, 40 );
        d.m_surface.m_fieldC = rgb( 240, 120, 180 );
        d.m_surface.m_fields = 0.9f;
        d.m_terrain = { 2.0f, 0.0f, 0.0f, 0.0f, 0.5f };
        style( d, 1.0f, 0.3f, 0.25f, 0.1f, 2, 1.0f, 5 );
        d.m_postSpacing = 40.0f;
        d.m_props = { { PropKindE::DUTCH_MILL, 0.012f, 15.0f, 140.0f, 0.9f, 1.2f },
                      { PropKindE::HOUSE, 0.015f, 15.0f, 80.0f, 0.8f, 1.1f },
                      { PropKindE::FENCE, 0.2f, 3.0f, 3.0f, 1.0f, 1.0f },
                      { PropKindE::BIRCH, 0.06f, 3.0f, 30.0f, 0.8f, 1.2f },
                      { PropKindE::POLE, 0.03f, 6.0f, 6.0f, 1.0f, 1.0f } };
    }
    { // Misty highlands: heather, stone walls, ruins.
        BiomeDef &d = at( BiomeE::HIGHLANDS );
        d.m_name = "MISTY HIGHLANDS";
        d.m_env = sky( rgb( 110, 125, 140 ), rgb( 180, 190, 195 ), rgb( 230, 230, 225 ), 0.7f, 20.0f, 50.0f, rgb( 140, 150, 160 ),
                       rgb( 90, 90, 80 ), rgb( 170, 178, 182 ), 0.0026f, rgb( 80, 90, 95 ), rgb( 130, 140, 148 ), 7.0f );
        d.m_env.m_sunSize = 2.5f;
        d.m_env.m_cloudAmount = 0.85f;
        d.m_env.m_cloudColor = rgb( 190, 195, 200 );
        d.m_env.m_rain = 0.2f;
        d.m_surface = ground( rgb( 90, 110, 60 ), rgb( 110, 100, 70 ), rgb( 110, 90, 100 ), rgb( 84, 84, 90 ), rgb( 220, 40, 40 ),
                              rgb( 240, 240, 240 ), rgb( 240, 240, 240 ), rgb( 130, 125, 110 ), rgb( 70, 90, 100 ) );
        d.m_terrain = { 36.0f, 10.0f, -4.0f, 0.0f, 0.1f };
        style( d, 0.92f, 0.7f, 0.55f, 0.8f, 2, 0.5f, 2 );
        d.m_postSpacing = 30.0f;
        d.m_props = { { PropKindE::STONE_WALL, 0.35f, 3.5f, 3.5f, 1.0f, 1.0f },
                      { PropKindE::RUIN, 0.008f, 20.0f, 120.0f, 0.9f, 1.4f },
                      { PropKindE::BUSH, 0.15f, 3.0f, 40.0f, 0.5f, 1.0f },
                      { PropKindE::ROCK, 0.1f, 3.0f, 60.0f, 0.6f, 1.4f },
                      { PropKindE::BOULDER, 0.02f, 10.0f, 80.0f, 0.8f, 1.5f } };
    }
    { // Fjord light: deep water left, cliffs right, red cabins, evening.
        BiomeDef &d = at( BiomeE::FJORD );
        d.m_name = "FJORD LIGHT";
        d.m_env = sky( rgb( 60, 90, 150 ), rgb( 240, 190, 150 ), rgb( 255, 200, 150 ), 1.0f, 8.0f, 60.0f, rgb( 110, 120, 150 ),
                       rgb( 70, 80, 80 ), rgb( 190, 180, 180 ), 0.0011f, rgb( 60, 70, 90 ), rgb( 120, 130, 160 ), 14.0f );
        d.m_env.m_sunSize = 2.0f;
        d.m_env.m_cloudAmount = 0.4f;
        d.m_env.m_cloudColor = rgb( 255, 210, 190 );
        d.m_surface = ground( rgb( 70, 120, 60 ), rgb( 62, 110, 54 ), rgb( 60, 100, 60 ), rgb( 86, 86, 92 ), rgb( 230, 230, 230 ),
                              rgb( 40, 90, 180 ), rgb( 240, 240, 240 ), rgb( 120, 116, 110 ), rgb( 40, 70, 100 ) );
        d.m_terrain = { 50.0f, -14.0f, 40.0f, 1.0f, 0.0f, 0.0f, -6.0f };
        style( d, 0.95f, 0.65f, 0.5f, 0.5f, 2, 0.6f, 3 );
        d.m_railLeft = true;
        d.m_props = { { PropKindE::CABIN, 0.02f, 8.0f, 50.0f, 0.9f, 1.2f },
                      { PropKindE::PINE, 0.25f, 4.0f, 60.0f, 0.8f, 1.5f },
                      { PropKindE::ROCK, 0.06f, 3.0f, 40.0f, 0.6f, 1.4f },
                      { PropKindE::BOULDER, 0.02f, 8.0f, 60.0f, 0.8f, 1.4f } };
    }
    { // Rice terraces: flooded paddies, bamboo, morning mist.
        BiomeDef &d = at( BiomeE::RICE );
        d.m_name = "RICE TERRACES";
        d.m_env = sky( rgb( 120, 170, 200 ), rgb( 220, 230, 225 ), rgb( 255, 245, 220 ), 1.0f, 22.0f, -40.0f, rgb( 150, 170, 170 ),
                       rgb( 100, 120, 90 ), rgb( 210, 222, 218 ), 0.0020f, rgb( 90, 120, 110 ), rgb( 150, 175, 170 ), 9.0f );
        d.m_env.m_cloudAmount = 0.5f;
        d.m_env.m_saturation = 1.1f;
        d.m_surface = ground( rgb( 110, 170, 60 ), rgb( 96, 156, 52 ), rgb( 90, 140, 70 ), rgb( 100, 96, 92 ), rgb( 230, 230, 230 ),
                              rgb( 60, 140, 60 ), rgb( 240, 240, 240 ), rgb( 140, 120, 90 ), rgb( 130, 170, 150 ) );
        d.m_terrain = { 30.0f, 8.0f, 8.0f, 0.0f, 0.9f };
        style( d, 0.95f, 0.6f, 0.5f, 0.55f, 2, 0.6f, 6 );
        d.m_props = { { PropKindE::BAMBOO, 0.2f, 3.0f, 40.0f, 0.8f, 1.4f },
                      { PropKindE::HUT, 0.015f, 10.0f, 60.0f, 0.9f, 1.2f },
                      { PropKindE::PALM, 0.06f, 4.0f, 50.0f, 0.9f, 1.3f },
                      { PropKindE::BUSH, 0.08f, 2.0f, 30.0f, 0.7f, 1.2f } };
    }
    { // Salt flats: a white mirror, long straights, top speed.
        BiomeDef &d = at( BiomeE::SALT_FLATS );
        d.m_name = "SALT FLATS";
        d.m_env = sky( rgb( 20, 70, 180 ), rgb( 190, 215, 240 ), rgb( 255, 250, 240 ), 1.5f, 60.0f, 0.0f, rgb( 170, 190, 220 ),
                       rgb( 200, 200, 200 ), rgb( 215, 228, 240 ), 0.0007f, rgb( 140, 150, 170 ), rgb( 190, 200, 220 ), 2.0f );
        d.m_env.m_cloudAmount = 0.15f;
        d.m_env.m_exposure = 0.9f;
        d.m_surface = ground( rgb( 240, 240, 236 ), rgb( 232, 232, 228 ), rgb( 236, 238, 240 ), rgb( 96, 96, 104 ), rgb( 220, 40, 40 ),
                              rgb( 240, 240, 240 ), rgb( 250, 250, 250 ), rgb( 220, 220, 215 ), rgb( 180, 200, 220 ) );
        d.m_surface.m_groundGloss = 0.6f;
        d.m_terrain = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        style( d, 1.0f, 0.15f, 0.15f, 0.05f, 3, 0.6f, 1 );
        d.m_postSpacing = 80.0f;
        d.m_props = { { PropKindE::ROCK, 0.01f, 10.0f, 200.0f, 0.6f, 1.4f }, { PropKindE::POLE, 0.01f, 8.0f, 8.0f, 1.0f, 1.0f } };
    }
    { // Mesa sunset: buttes in the evening light.
        BiomeDef &d = at( BiomeE::MESA_SUNSET );
        d.m_name = "MESA SUNSET";
        d.m_env = sky( rgb( 60, 50, 120 ), rgb( 255, 130, 70 ), rgb( 255, 150, 70 ), 1.2f, 4.0f, -10.0f, rgb( 140, 90, 110 ),
                       rgb( 160, 80, 50 ), rgb( 230, 130, 90 ), 0.0009f, rgb( 110, 50, 50 ), rgb( 200, 100, 80 ), 4.0f );
        d.m_env.m_sunSize = 3.0f;
        d.m_env.m_cloudAmount = 0.4f;
        d.m_env.m_cloudColor = rgb( 255, 140, 110 );
        d.m_env.m_saturation = 1.15f;
        d.m_env.m_bloom = 0.85f;
        d.m_surface = ground( rgb( 196, 110, 60 ), rgb( 184, 100, 54 ), rgb( 170, 90, 56 ), rgb( 96, 84, 84 ), rgb( 230, 230, 230 ),
                              rgb( 200, 60, 40 ), rgb( 250, 220, 120 ), rgb( 190, 120, 80 ), rgb( 80, 100, 120 ) );
        d.m_terrain = { 16.0f, 6.0f, 6.0f, 0.0f, 0.0f };
        style( d, 0.96f, 0.4f, 0.35f, 0.35f, 2, 0.6f, 7 );
        d.m_postSpacing = 50.0f;
        d.m_props = { { PropKindE::BUTTE, 0.015f, 60.0f, 300.0f, 1.0f, 2.0f },
                      { PropKindE::MESA, 0.008f, 100.0f, 300.0f, 1.0f, 2.0f },
                      { PropKindE::CACTUS, 0.04f, 4.0f, 60.0f, 0.7f, 1.3f },
                      { PropKindE::GRASS_TUFT, 0.3f, 1.0f, 30.0f, 0.7f, 1.2f },
                      { PropKindE::ROCK, 0.05f, 3.0f, 60.0f, 0.6f, 1.4f } };
    }
    { // Suburbia: lawns, houses, mailboxes, a summer afternoon.
        BiomeDef &d = at( BiomeE::SUBURB );
        d.m_name = "SUBURBIA";
        d.m_env = sky( rgb( 60, 130, 220 ), rgb( 190, 215, 240 ), rgb( 255, 245, 225 ), 1.3f, 45.0f, 120.0f, rgb( 140, 160, 200 ),
                       rgb( 110, 120, 90 ), rgb( 200, 215, 235 ), 0.0012f, rgb( 110, 140, 150 ), rgb( 160, 190, 215 ), 2.0f );
        d.m_env.m_cloudAmount = 0.35f;
        d.m_surface = ground( rgb( 90, 160, 60 ), rgb( 80, 148, 54 ), rgb( 100, 150, 80 ), rgb( 84, 84, 92 ), rgb( 200, 200, 200 ),
                              rgb( 160, 160, 160 ), rgb( 250, 230, 120 ), rgb( 190, 190, 185 ), rgb( 80, 140, 200 ) );
        d.m_terrain = { 6.0f, 1.0f, 1.0f, 0.0f, 0.0f };
        style( d, 1.0f, 0.4f, 0.4f, 0.3f, 2, 1.2f, 6 );
        d.m_lampSpacing = 45.0f;
        d.m_props = { { PropKindE::HOUSE, 0.18f, 8.0f, 30.0f, 0.8f, 1.1f },
                      { PropKindE::OAK, 0.15f, 3.0f, 40.0f, 0.8f, 1.3f },
                      { PropKindE::MAILBOX, 0.1f, 1.0f, 2.0f, 1.0f, 1.0f },
                      { PropKindE::FENCE, 0.1f, 4.0f, 4.0f, 1.0f, 1.0f },
                      { PropKindE::BUSH, 0.1f, 2.0f, 20.0f, 0.6f, 1.1f } };
    }
    { // Steel harbour: cranes and containers at dusk, the water on the left.
        BiomeDef &d = at( BiomeE::HARBOR );
        d.m_name = "STEEL HARBOR";
        d.m_env = sky( rgb( 40, 50, 90 ), rgb( 240, 140, 90 ), rgb( 255, 150, 90 ), 0.9f, 4.0f, 70.0f, rgb( 90, 90, 120 ),
                       rgb( 80, 70, 70 ), rgb( 160, 110, 100 ), 0.0016f, rgb( 50, 45, 60 ), rgb( 100, 80, 90 ), 2.5f );
        d.m_env.m_sunSize = 2.4f;
        d.m_env.m_cloudAmount = 0.45f;
        d.m_env.m_cloudColor = rgb( 220, 130, 110 );
        d.m_env.m_headlights = 0.5f;
        d.m_env.m_bloom = 0.9f;
        d.m_surface = ground( rgb( 90, 90, 94 ), rgb( 84, 84, 88 ), rgb( 80, 80, 86 ), rgb( 64, 64, 70 ), rgb( 240, 200, 40 ),
                              rgb( 40, 40, 40 ), rgb( 240, 220, 120 ), rgb( 100, 100, 104 ), rgb( 40, 60, 80 ) );
        d.m_terrain = { 2.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.0f, -2.0f };
        style( d, 1.0f, 0.4f, 0.4f, 0.2f, 3, 1.2f, 1 );
        d.m_railLeft = true;
        d.m_lampSpacing = 40.0f;
        d.m_props = { { PropKindE::CONTAINERS, 0.15f, 8.0f, 60.0f, 0.9f, 1.2f },
                      { PropKindE::CRANE, 0.03f, 6.0f, 11.0f, 0.9f, 1.1f },
                      { PropKindE::CHIMNEY, 0.012f, 40.0f, 160.0f, 0.9f, 1.4f },
                      { PropKindE::BUILDING, 0.05f, 20.0f, 80.0f, 0.8f, 1.2f } };
    }
    { // Neon rain: cyberpunk city, wet streets.
        BiomeDef &d = at( BiomeE::NEON_RAIN );
        d.m_name = "NEON RAIN";
        d.m_env = sky( rgb( 10, 6, 24 ), rgb( 60, 30, 80 ), rgb( 180, 140, 255 ), 0.15f, 30.0f, 0.0f, rgb( 50, 40, 90 ),
                       rgb( 40, 30, 50 ), rgb( 50, 30, 70 ), 0.0024f, rgb( 20, 14, 36 ), rgb( 40, 26, 60 ), 3.0f );
        d.m_env.m_sunSize = 1.0f;
        d.m_env.m_cloudAmount = 0.9f;
        d.m_env.m_cloudColor = rgb( 60, 40, 80 );
        d.m_env.m_rain = 1.0f;
        d.m_env.m_headlights = 1.0f;
        d.m_env.m_exposure = 1.2f;
        d.m_env.m_saturation = 1.25f;
        d.m_env.m_bloom = 1.2f;
        d.m_surface = ground( rgb( 30, 30, 40 ), rgb( 26, 26, 36 ), rgb( 22, 22, 32 ), rgb( 34, 34, 44 ), rgb( 255, 40, 180 ),
                              rgb( 30, 30, 40 ), rgb( 60, 220, 255 ), rgb( 40, 40, 50 ), rgb( 20, 20, 40 ) );
        d.m_surface.m_lineGlow = 0.5f;
        d.m_terrain = { 3.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        style( d, 0.88f, 0.45f, 0.4f, 0.25f, 3, 1.4f, 4 );
        d.m_railLeft = true;
        d.m_railRight = true;
        d.m_lampSpacing = 30.0f;
        d.m_props = { { PropKindE::NEON_TOWER, 0.3f, 12.0f, 70.0f, 0.8f, 1.5f },
                      { PropKindE::TOWER, 0.08f, 50.0f, 220.0f, 0.8f, 1.6f },
                      { PropKindE::BILLBOARD, 0.06f, 6.0f, 14.0f, 1.0f, 1.2f } };
    }
    { // Dawn expressway: four lanes, sound walls, sign gantries, the sun straight ahead.
        BiomeDef &d = at( BiomeE::EXPRESSWAY );
        d.m_name = "DAWN EXPRESSWAY";
        d.m_env = sky( rgb( 70, 90, 160 ), rgb( 255, 190, 140 ), rgb( 255, 190, 130 ), 1.1f, 3.0f, 0.0f, rgb( 120, 120, 160 ),
                       rgb( 110, 90, 80 ), rgb( 230, 180, 160 ), 0.0012f, rgb( 90, 80, 110 ), rgb( 170, 140, 160 ), 2.0f );
        d.m_env.m_sunSize = 2.8f;
        d.m_env.m_cloudAmount = 0.4f;
        d.m_env.m_cloudColor = rgb( 255, 190, 170 );
        d.m_env.m_headlights = 0.3f;
        d.m_env.m_bloom = 0.9f;
        d.m_surface = ground( rgb( 110, 130, 80 ), rgb( 100, 120, 74 ), rgb( 110, 120, 100 ), rgb( 80, 80, 88 ), rgb( 220, 220, 220 ),
                              rgb( 120, 120, 120 ), rgb( 245, 245, 245 ), rgb( 120, 120, 124 ), rgb( 70, 100, 140 ) );
        d.m_terrain = { 6.0f, 2.0f, 2.0f, 0.0f, 0.0f };
        style( d, 1.0f, 0.25f, 0.2f, 0.2f, 4, 1.6f, 0 );
        d.m_railLeft = true;
        d.m_railRight = true;
        d.m_walls = true;
        d.m_gantrySpacing = 450.0f;
        d.m_lampSpacing = 50.0f;
        d.m_props = { { PropKindE::BUILDING, 0.04f, 30.0f, 120.0f, 0.8f, 1.4f },
                      { PropKindE::TOWER, 0.02f, 80.0f, 250.0f, 0.8f, 1.4f },
                      { PropKindE::OAK, 0.05f, 20.0f, 60.0f, 0.8f, 1.3f } };
    }
    { // Lunar highway: black sky, the Earth above, low gravity.
        BiomeDef &d = at( BiomeE::MOON );
        d.m_name = "LUNAR HIGHWAY";
        d.m_env = sky( rgb( 0, 0, 0 ), rgb( 6, 6, 10 ), rgb( 255, 255, 250 ), 1.6f, 25.0f, 60.0f, rgb( 40, 40, 50 ),
                       rgb( 60, 60, 64 ), rgb( 10, 10, 14 ), 0.0004f, rgb( 70, 70, 74 ), rgb( 100, 100, 104 ), 5.0f );
        d.m_env.m_sunSize = 1.0f;
        d.m_env.m_cloudAmount = 0.0f;
        d.m_env.m_stars = 1.0f;
        d.m_env.m_planetSize = 6.0f;
        d.m_env.m_planetElevation = 18.0f;
        d.m_env.m_planetAzimuth = -20.0f;
        d.m_env.m_planetColor = rgb( 70, 120, 220 );
        d.m_env.m_headlights = 0.5f;
        d.m_env.m_saturation = 0.9f;
        d.m_env.m_bloom = 0.7f;
        d.m_surface = ground( rgb( 120, 120, 118 ), rgb( 112, 112, 110 ), rgb( 100, 100, 98 ), rgb( 54, 54, 58 ), rgb( 240, 240, 240 ),
                              rgb( 240, 120, 20 ), rgb( 240, 200, 60 ), rgb( 90, 90, 90 ), rgb( 0, 0, 0 ) );
        d.m_terrain = { 20.0f, 4.0f, 4.0f, 0.0f, 0.0f };
        style( d, 0.9f, 0.45f, 0.4f, 0.7f, 2, 0.3f, 3 );
        d.m_gravity = 0.35f;
        d.m_postSpacing = 40.0f;
        d.m_props = { { PropKindE::CRATER, 0.06f, 6.0f, 120.0f, 0.8f, 2.5f },
                      { PropKindE::DOME, 0.008f, 30.0f, 150.0f, 0.9f, 1.4f },
                      { PropKindE::ANTENNA, 0.008f, 20.0f, 120.0f, 0.9f, 1.3f },
                      { PropKindE::ROCK, 0.08f, 3.0f, 60.0f, 0.6f, 1.4f },
                      { PropKindE::BOULDER, 0.02f, 10.0f, 80.0f, 0.8f, 1.5f } };
    }
    { // Red planet: butterscotch sky, red dust, Phobos.
        BiomeDef &d = at( BiomeE::MARS );
        d.m_name = "RED PLANET";
        d.m_env = sky( rgb( 120, 80, 60 ), rgb( 220, 160, 110 ), rgb( 255, 230, 200 ), 1.1f, 30.0f, 20.0f, rgb( 170, 110, 80 ),
                       rgb( 150, 80, 50 ), rgb( 210, 140, 100 ), 0.0012f, rgb( 150, 70, 40 ), rgb( 200, 120, 80 ), 7.0f );
        d.m_env.m_sunSize = 1.0f;
        d.m_env.m_cloudAmount = 0.15f;
        d.m_env.m_cloudColor = rgb( 230, 180, 150 );
        d.m_env.m_planetSize = 1.2f;
        d.m_env.m_planetElevation = 40.0f;
        d.m_env.m_planetAzimuth = -60.0f;
        d.m_env.m_planetColor = rgb( 150, 130, 120 );
        d.m_surface = ground( rgb( 190, 90, 50 ), rgb( 178, 84, 46 ), rgb( 160, 80, 46 ), rgb( 80, 62, 56 ), rgb( 240, 240, 240 ),
                              rgb( 60, 60, 60 ), rgb( 250, 250, 250 ), rgb( 170, 90, 60 ), rgb( 0, 0, 0 ) );
        d.m_terrain = { 30.0f, 10.0f, 10.0f, 0.0f, 0.0f };
        style( d, 0.9f, 0.5f, 0.45f, 0.6f, 2, 0.3f, 2 );
        d.m_gravity = 0.45f;
        d.m_postSpacing = 40.0f;
        d.m_props = { { PropKindE::ROCK, 0.12f, 3.0f, 60.0f, 0.6f, 1.5f },
                      { PropKindE::BOULDER, 0.04f, 8.0f, 80.0f, 0.8f, 1.6f },
                      { PropKindE::BUTTE, 0.006f, 80.0f, 300.0f, 1.0f, 2.0f },
                      { PropKindE::DOME, 0.006f, 30.0f, 150.0f, 0.9f, 1.4f },
                      { PropKindE::ANTENNA, 0.006f, 20.0f, 120.0f, 0.9f, 1.3f },
                      { PropKindE::CRATER, 0.03f, 6.0f, 120.0f, 0.8f, 2.0f } };
    }
    { // Crystal desert: violet twilight, glowing crystals.
        BiomeDef &d = at( BiomeE::CRYSTALS );
        d.m_name = "CRYSTAL DESERT";
        d.m_env = sky( rgb( 30, 20, 70 ), rgb( 180, 110, 200 ), rgb( 255, 170, 220 ), 0.5f, 3.0f, -15.0f, rgb( 90, 70, 140 ),
                       rgb( 80, 60, 110 ), rgb( 120, 80, 150 ), 0.0014f, rgb( 50, 30, 80 ), rgb( 110, 70, 150 ), 4.0f );
        d.m_env.m_sunSize = 2.2f;
        d.m_env.m_stars = 0.6f;
        d.m_env.m_cloudAmount = 0.2f;
        d.m_env.m_cloudColor = rgb( 200, 150, 220 );
        d.m_env.m_headlights = 0.6f;
        d.m_env.m_saturation = 1.15f;
        d.m_env.m_bloom = 1.1f;
        d.m_surface = ground( rgb( 110, 90, 140 ), rgb( 102, 84, 132 ), rgb( 90, 74, 120 ), rgb( 54, 48, 70 ), rgb( 120, 240, 255 ),
                              rgb( 40, 30, 60 ), rgb( 180, 240, 255 ), rgb( 100, 84, 120 ), rgb( 60, 40, 90 ) );
        d.m_surface.m_lineGlow = 0.4f;
        d.m_terrain = { 18.0f, 6.0f, 6.0f, 0.0f, 0.0f };
        style( d, 0.98f, 0.5f, 0.45f, 0.45f, 2, 0.5f, 0 );
        d.m_props = { { PropKindE::CRYSTAL, 0.12f, 4.0f, 100.0f, 0.7f, 2.0f }, { PropKindE::ROCK, 0.05f, 3.0f, 60.0f, 0.6f, 1.3f } };
    }
    { // Fungal glade: giant glowing mushrooms in a teal night.
        BiomeDef &d = at( BiomeE::FUNGI );
        d.m_name = "FUNGAL GLADE";
        d.m_env = sky( rgb( 6, 16, 24 ), rgb( 20, 60, 70 ), rgb( 150, 230, 220 ), 0.2f, 30.0f, 30.0f, rgb( 40, 70, 80 ),
                       rgb( 30, 50, 50 ), rgb( 20, 60, 64 ), 0.0028f, rgb( 10, 30, 34 ), rgb( 20, 50, 56 ), 4.0f );
        d.m_env.m_sunSize = 1.2f;
        d.m_env.m_stars = 0.5f;
        d.m_env.m_headlights = 1.0f;
        d.m_env.m_saturation = 1.2f;
        d.m_env.m_bloom = 1.2f;
        d.m_surface = ground( rgb( 30, 60, 50 ), rgb( 26, 54, 46 ), rgb( 20, 46, 40 ), rgb( 40, 40, 46 ), rgb( 100, 255, 200 ),
                              rgb( 30, 40, 40 ), rgb( 150, 255, 220 ), rgb( 40, 50, 46 ), rgb( 20, 60, 70 ) );
        d.m_surface.m_lineGlow = 0.3f;
        d.m_terrain = { 14.0f, 4.0f, 4.0f, 0.0f, 0.2f };
        style( d, 0.95f, 0.6f, 0.5f, 0.4f, 2, 0.5f, 4 );
        d.m_props = { { PropKindE::MUSHROOM, 0.18f, 4.0f, 60.0f, 0.7f, 1.8f },
                      { PropKindE::GLOW_SHROOM, 0.35f, 1.0f, 20.0f, 0.8f, 1.4f },
                      { PropKindE::FERN, 0.2f, 1.0f, 20.0f, 0.8f, 1.3f },
                      { PropKindE::FIREFLIES, 0.1f, 2.0f, 30.0f, 1.0f, 1.0f },
                      { PropKindE::MUSHROOM, 0.06f, 60.0f, 200.0f, 1.5f, 3.0f } };
    }
    { // Cloud highway: a ridge road above a sea of clouds at sunrise.
        BiomeDef &d = at( BiomeE::CLOUDS );
        d.m_name = "CLOUD HIGHWAY";
        d.m_env = sky( rgb( 60, 100, 190 ), rgb( 255, 210, 170 ), rgb( 255, 220, 170 ), 1.3f, 6.0f, 25.0f, rgb( 150, 160, 200 ),
                       rgb( 230, 200, 190 ), rgb( 250, 225, 210 ), 0.0008f, rgb( 200, 170, 190 ), rgb( 230, 210, 225 ), 3.0f );
        d.m_env.m_sunSize = 2.4f;
        d.m_env.m_cloudAmount = 0.25f;
        d.m_env.m_cloudColor = rgb( 255, 230, 210 );
        d.m_env.m_bloom = 0.8f;
        d.m_surface = ground( rgb( 120, 150, 90 ), rgb( 110, 140, 82 ), rgb( 150, 140, 130 ), rgb( 90, 90, 98 ), rgb( 240, 240, 240 ),
                              rgb( 60, 120, 220 ), rgb( 245, 245, 245 ), rgb( 150, 140, 130 ), rgb( 250, 244, 240 ) );
        d.m_surface.m_waterGloss = 0.0f;
        d.m_surface.m_waterGlow = 0.15f;
        d.m_terrain = { 0.0f, -70.0f, -70.0f, 1.0f, 0.0f, 1.0f, -40.0f };
        style( d, 1.0f, 0.55f, 0.4f, 0.6f, 2, 0.5f, 3 );
        d.m_postSpacing = 30.0f;
        d.m_props = { { PropKindE::ROCK, 0.05f, 2.0f, 12.0f, 0.6f, 1.2f }, { PropKindE::PINE, 0.06f, 2.0f, 10.0f, 0.7f, 1.1f } };
    }
    { // Iceberg bay: polar day, icebergs on the water.
        BiomeDef &d = at( BiomeE::ICEBERGS );
        d.m_name = "ICEBERG BAY";
        d.m_env = sky( rgb( 80, 130, 200 ), rgb( 210, 230, 245 ), rgb( 255, 250, 240 ), 1.2f, 15.0f, 40.0f, rgb( 170, 190, 220 ),
                       rgb( 160, 175, 195 ), rgb( 210, 225, 240 ), 0.0012f, rgb( 190, 205, 225 ), rgb( 215, 228, 242 ), 6.0f );
        d.m_env.m_cloudAmount = 0.4f;
        d.m_surface = ground( rgb( 236, 242, 250 ), rgb( 224, 232, 244 ), rgb( 220, 230, 242 ), rgb( 90, 94, 104 ), rgb( 220, 40, 40 ),
                              rgb( 240, 240, 240 ), rgb( 240, 240, 240 ), rgb( 180, 186, 196 ), rgb( 40, 90, 130 ) );
        d.m_terrain = { 20.0f, 0.0f, 16.0f, 1.0f, 0.0f, 0.0f, -2.5f };
        style( d, 0.82f, 0.5f, 0.4f, 0.4f, 2, 0.5f, 2 );
        d.m_railLeft = true;
        d.m_postSpacing = 25.0f;
        d.m_props = { { PropKindE::ICEBERG, 0.05f, 40.0f, 400.0f, 0.8f, 3.0f, true },
                      { PropKindE::SNOWBANK, 0.2f, 0.5f, 3.0f, 0.8f, 1.4f },
                      { PropKindE::ROCK, 0.04f, 4.0f, 40.0f, 0.6f, 1.3f },
                      { PropKindE::SNOW_PINE, 0.06f, 6.0f, 80.0f, 0.8f, 1.4f } };
    }
    { // Bayou dusk: water everywhere, moss trees, fireflies.
        BiomeDef &d = at( BiomeE::BAYOU );
        d.m_name = "BAYOU DUSK";
        d.m_env = sky( rgb( 40, 50, 70 ), rgb( 220, 140, 80 ), rgb( 255, 160, 90 ), 0.8f, 5.0f, -30.0f, rgb( 90, 90, 90 ),
                       rgb( 60, 60, 40 ), rgb( 130, 110, 80 ), 0.0026f, rgb( 40, 44, 36 ), rgb( 80, 80, 60 ), 2.0f );
        d.m_env.m_sunSize = 2.4f;
        d.m_env.m_cloudAmount = 0.5f;
        d.m_env.m_cloudColor = rgb( 200, 140, 100 );
        d.m_env.m_headlights = 0.6f;
        d.m_env.m_bloom = 1.0f;
        d.m_surface = ground( rgb( 70, 90, 46 ), rgb( 62, 82, 40 ), rgb( 56, 70, 40 ), rgb( 60, 60, 64 ), rgb( 230, 230, 230 ),
                              rgb( 60, 60, 60 ), rgb( 240, 220, 120 ), rgb( 90, 80, 60 ), rgb( 50, 60, 40 ) );
        d.m_terrain = { 3.0f, -2.0f, -2.0f, 1.0f, 1.0f, 1.0f, -1.5f };
        style( d, 0.9f, 0.55f, 0.45f, 0.1f, 2, 0.5f, 7 );
        d.m_postSpacing = 25.0f;
        d.m_props = { { PropKindE::MOSS_TREE, 0.2f, 3.0f, 22.0f, 0.8f, 1.4f },
                      { PropKindE::REEDS, 0.4f, 1.0f, 22.0f, 0.7f, 1.3f },
                      { PropKindE::FIREFLIES, 0.15f, 1.0f, 22.0f, 1.0f, 1.0f },
                      { PropKindE::MOSS_TREE, 0.08f, 30.0f, 200.0f, 1.0f, 1.6f, true },
                      { PropKindE::DEAD_TREE, 0.04f, 3.0f, 22.0f, 0.8f, 1.2f } };
    }
    { // Wheat plains: golden fields, barns, silos, big cumulus.
        BiomeDef &d = at( BiomeE::WHEAT );
        d.m_name = "WHEAT PLAINS";
        d.m_env = sky( rgb( 50, 110, 210 ), rgb( 200, 220, 240 ), rgb( 255, 245, 220 ), 1.35f, 50.0f, 140.0f, rgb( 150, 170, 210 ),
                       rgb( 150, 130, 80 ), rgb( 210, 215, 225 ), 0.0009f, rgb( 120, 140, 150 ), rgb( 170, 190, 210 ), 1.5f );
        d.m_env.m_cloudAmount = 0.5f;
        d.m_surface = ground( rgb( 210, 180, 80 ), rgb( 196, 166, 70 ), rgb( 190, 170, 90 ), rgb( 90, 88, 92 ), rgb( 230, 230, 230 ),
                              rgb( 200, 40, 40 ), rgb( 240, 240, 240 ), rgb( 170, 150, 110 ), rgb( 80, 120, 170 ) );
        d.m_surface.m_fieldA = rgb( 230, 200, 90 );
        d.m_surface.m_fieldB = rgb( 200, 170, 70 );
        d.m_surface.m_fieldC = rgb( 120, 150, 60 );
        d.m_surface.m_fields = 0.8f;
        d.m_terrain = { 10.0f, 2.0f, 2.0f, 0.0f, 0.0f };
        style( d, 1.0f, 0.25f, 0.25f, 0.25f, 2, 0.7f, 1 );
        d.m_postSpacing = 50.0f;
        d.m_props = { { PropKindE::BARN, 0.01f, 20.0f, 90.0f, 0.9f, 1.2f },
                      { PropKindE::SILO, 0.01f, 20.0f, 90.0f, 0.9f, 1.3f },
                      { PropKindE::HAYBALE, 0.05f, 8.0f, 60.0f, 0.9f, 1.1f },
                      { PropKindE::POLE, 0.05f, 6.0f, 6.0f, 1.0f, 1.0f },
                      { PropKindE::FENCE, 0.15f, 4.0f, 4.0f, 1.0f, 1.0f },
                      { PropKindE::WINDMILL, 0.003f, 80.0f, 250.0f, 1.0f, 1.3f } };
    }
    { // Chalk cliffs: the sea far below on the left, a lighthouse.
        BiomeDef &d = at( BiomeE::CLIFFS );
        d.m_name = "CHALK CLIFFS";
        d.m_env = sky( rgb( 60, 120, 210 ), rgb( 200, 220, 240 ), rgb( 255, 245, 225 ), 1.25f, 35.0f, -60.0f, rgb( 150, 170, 210 ),
                       rgb( 120, 130, 110 ), rgb( 205, 220, 235 ), 0.0011f, rgb( 180, 190, 200 ), rgb( 200, 215, 230 ), 1.5f );
        d.m_env.m_cloudAmount = 0.5f;
        d.m_surface = ground( rgb( 100, 150, 70 ), rgb( 90, 138, 64 ), rgb( 110, 140, 80 ), rgb( 92, 92, 98 ), rgb( 220, 40, 40 ),
                              rgb( 240, 240, 240 ), rgb( 240, 240, 240 ), rgb( 225, 225, 215 ), rgb( 40, 100, 150 ) );
        d.m_terrain = { 14.0f, 0.0f, 8.0f, 1.0f, 0.0f, 0.0f, -30.0f };
        style( d, 1.0f, 0.55f, 0.4f, 0.5f, 2, 0.6f, 2 );
        d.m_railLeft = true;
        d.m_postSpacing = 30.0f;
        d.m_props = { { PropKindE::LIGHTHOUSE, 0.004f, 4.0f, 9.0f, 1.0f, 1.2f },
                      { PropKindE::STONE_WALL, 0.2f, 3.5f, 3.5f, 1.0f, 1.0f },
                      { PropKindE::BUSH, 0.08f, 3.0f, 30.0f, 0.6f, 1.1f },
                      { PropKindE::HOUSE, 0.01f, 15.0f, 70.0f, 0.8f, 1.1f } };
    }
    { // Pumpkin hollow: an orange moon, graves, jack-o'-lanterns.
        BiomeDef &d = at( BiomeE::HALLOWEEN );
        d.m_name = "PUMPKIN HOLLOW";
        d.m_env = sky( rgb( 10, 6, 20 ), rgb( 60, 30, 50 ), rgb( 255, 150, 60 ), 0.35f, 14.0f, 10.0f, rgb( 50, 40, 70 ),
                       rgb( 40, 30, 30 ), rgb( 50, 40, 60 ), 0.0026f, rgb( 16, 10, 20 ), rgb( 40, 24, 40 ), 4.0f );
        d.m_env.m_sunSize = 3.2f;
        d.m_env.m_cloudAmount = 0.4f;
        d.m_env.m_cloudColor = rgb( 60, 40, 60 );
        d.m_env.m_stars = 0.6f;
        d.m_env.m_headlights = 1.0f;
        d.m_env.m_saturation = 1.15f;
        d.m_env.m_bloom = 1.1f;
        d.m_surface = ground( rgb( 40, 40, 30 ), rgb( 36, 36, 26 ), rgb( 30, 30, 24 ), rgb( 44, 40, 46 ), rgb( 255, 120, 20 ),
                              rgb( 30, 20, 30 ), rgb( 255, 160, 60 ), rgb( 50, 44, 40 ), rgb( 30, 40, 40 ) );
        d.m_terrain = { 14.0f, 4.0f, 4.0f, 0.0f, 0.1f };
        style( d, 0.95f, 0.6f, 0.55f, 0.4f, 2, 0.4f, 5 );
        d.m_props = { { PropKindE::PUMPKIN, 0.25f, 1.0f, 15.0f, 0.8f, 1.6f },
                      { PropKindE::GRAVESTONE, 0.2f, 3.0f, 40.0f, 0.8f, 1.2f },
                      { PropKindE::DEAD_TREE, 0.15f, 4.0f, 80.0f, 0.8f, 1.5f },
                      { PropKindE::FENCE, 0.1f, 3.0f, 3.0f, 1.0f, 1.0f } };
    }
    { // Frosted village: snowy evening, lit windows, festive trees.
        BiomeDef &d = at( BiomeE::WINTER_VILLAGE );
        d.m_name = "FROSTED VILLAGE";
        d.m_env = sky( rgb( 20, 30, 60 ), rgb( 90, 100, 140 ), rgb( 200, 210, 255 ), 0.25f, 20.0f, 20.0f, rgb( 80, 90, 130 ),
                       rgb( 100, 100, 120 ), rgb( 80, 90, 120 ), 0.0020f, rgb( 60, 66, 90 ), rgb( 90, 100, 130 ), 6.0f );
        d.m_env.m_sunSize = 1.2f;
        d.m_env.m_cloudAmount = 0.6f;
        d.m_env.m_cloudColor = rgb( 90, 100, 130 );
        d.m_env.m_snow = 0.6f;
        d.m_env.m_stars = 0.3f;
        d.m_env.m_headlights = 1.0f;
        d.m_env.m_bloom = 1.0f;
        d.m_surface = ground( rgb( 220, 228, 245 ), rgb( 208, 218, 238 ), rgb( 200, 210, 232 ), rgb( 70, 74, 86 ), rgb( 220, 40, 40 ),
                              rgb( 240, 240, 240 ), rgb( 240, 240, 240 ), rgb( 150, 156, 170 ), rgb( 40, 60, 90 ) );
        d.m_terrain = { 16.0f, 4.0f, 4.0f, 0.0f, 0.0f };
        style( d, 0.82f, 0.55f, 0.5f, 0.4f, 2, 0.6f, 5 );
        d.m_lampSpacing = 40.0f;
        d.m_props = { { PropKindE::SNOW_HOUSE, 0.12f, 8.0f, 50.0f, 0.8f, 1.1f },
                      { PropKindE::LIGHT_TREE, 0.1f, 4.0f, 40.0f, 0.8f, 1.3f },
                      { PropKindE::SNOW_PINE, 0.15f, 6.0f, 80.0f, 0.8f, 1.5f },
                      { PropKindE::SNOWBANK, 0.2f, 0.5f, 3.0f, 0.8f, 1.4f } };
    }
    { // Rainbow ridge: the shower has passed, the sun behind, a bow ahead.
        BiomeDef &d = at( BiomeE::RAINBOW );
        d.m_name = "RAINBOW RIDGE";
        d.m_env = sky( rgb( 50, 110, 200 ), rgb( 200, 215, 230 ), rgb( 255, 245, 220 ), 1.3f, 18.0f, 180.0f, rgb( 140, 160, 200 ),
                       rgb( 90, 110, 70 ), rgb( 190, 205, 220 ), 0.0012f, rgb( 80, 110, 100 ), rgb( 150, 170, 190 ), 5.0f );
        d.m_env.m_cloudAmount = 0.6f;
        d.m_env.m_cloudColor = rgb( 240, 240, 245 );
        d.m_env.m_rainbow = 1.0f;
        d.m_env.m_rain = 0.06f;
        d.m_env.m_saturation = 1.12f;
        d.m_surface = ground( rgb( 80, 160, 60 ), rgb( 70, 148, 54 ), rgb( 70, 140, 70 ), rgb( 80, 82, 90 ), rgb( 220, 40, 40 ),
                              rgb( 240, 240, 240 ), rgb( 240, 240, 240 ), rgb( 130, 130, 120 ), rgb( 70, 120, 170 ) );
        d.m_terrain = { 26.0f, 8.0f, 8.0f, 0.0f, 0.1f };
        style( d, 0.95f, 0.6f, 0.45f, 0.65f, 2, 0.8f, 6 );
        d.m_postSpacing = 30.0f;
        d.m_props = { { PropKindE::OAK, 0.12f, 4.0f, 60.0f, 0.8f, 1.4f },
                      { PropKindE::BUSH, 0.12f, 2.0f, 30.0f, 0.6f, 1.2f },
                      { PropKindE::FENCE, 0.15f, 4.0f, 4.0f, 1.0f, 1.0f },
                      { PropKindE::HOUSE, 0.008f, 20.0f, 90.0f, 0.8f, 1.1f },
                      { PropKindE::BIRCH, 0.05f, 3.0f, 40.0f, 0.8f, 1.2f } };
    }
    { // Total eclipse: the corona high ahead, dusk all round the horizon.
        BiomeDef &d = at( BiomeE::ECLIPSE );
        d.m_name = "TOTAL ECLIPSE";
        d.m_env = sky( rgb( 6, 8, 20 ), rgb( 190, 110, 70 ), rgb( 255, 240, 220 ), 0.25f, 22.0f, 0.0f, rgb( 40, 44, 70 ),
                       rgb( 60, 40, 40 ), rgb( 90, 70, 80 ), 0.0010f, rgb( 30, 26, 40 ), rgb( 80, 60, 70 ), 3.0f );
        d.m_env.m_sunSize = 1.6f;
        d.m_env.m_eclipse = 1.0f;
        d.m_env.m_stars = 0.8f;
        d.m_env.m_cloudAmount = 0.05f;
        d.m_env.m_headlights = 0.7f;
        d.m_env.m_bloom = 1.1f;
        d.m_surface = ground( rgb( 150, 120, 90 ), rgb( 140, 112, 84 ), rgb( 120, 100, 80 ), rgb( 60, 58, 62 ), rgb( 230, 230, 230 ),
                              rgb( 60, 60, 60 ), rgb( 240, 220, 160 ), rgb( 110, 96, 80 ), rgb( 40, 50, 60 ) );
        d.m_terrain = { 14.0f, 4.0f, 4.0f, 0.0f, 0.0f };
        style( d, 0.97f, 0.4f, 0.35f, 0.35f, 2, 0.4f, 0 );
        d.m_postSpacing = 40.0f;
        d.m_props = { { PropKindE::ROCK, 0.06f, 3.0f, 60.0f, 0.6f, 1.4f },
                      { PropKindE::BOULDER, 0.02f, 10.0f, 80.0f, 0.8f, 1.5f },
                      { PropKindE::GRASS_TUFT, 0.2f, 1.0f, 30.0f, 0.7f, 1.2f },
                      { PropKindE::BUTTE, 0.004f, 80.0f, 300.0f, 1.0f, 1.8f } };
    }
    addWorldBiomes( b );

    // Tunnels in the mountains and under cities, bridges over valleys, canals and the clouds.
    const std::pair<BiomeE, float> tunnels[] = { { BiomeE::FOREST, 0.3f },   { BiomeE::SNOW, 0.25f },     { BiomeE::FJORD, 0.35f },
                                                 { BiomeE::HIGHLANDS, 0.18f }, { BiomeE::SAKURA, 0.12f }, { BiomeE::JUNGLE, 0.12f },
                                                 { BiomeE::VOLCANO, 0.22f }, { BiomeE::MESA_SUNSET, 0.1f }, { BiomeE::RICE, 0.1f },
                                                 { BiomeE::CLIFFS, 0.12f },  { BiomeE::CITY, 0.15f },     { BiomeE::NEON_RAIN, 0.15f },
                                                 { BiomeE::EXPRESSWAY, 0.12f }, { BiomeE::MARS, 0.1f },   { BiomeE::CRYSTALS, 0.08f },
                                                 { BiomeE::DESERT, 0.08f },  { BiomeE::AURORA, 0.06f },   { BiomeE::WINTER_VILLAGE, 0.06f } };
    const std::pair<BiomeE, float> bridges[] = { { BiomeE::FJORD, 0.3f },    { BiomeE::FOREST, 0.14f },   { BiomeE::HIGHLANDS, 0.14f },
                                                 { BiomeE::MEADOW, 0.1f },   { BiomeE::AUTUMN, 0.12f },   { BiomeE::JUNGLE, 0.14f },
                                                 { BiomeE::RICE, 0.18f },    { BiomeE::TULIPS, 0.12f },   { BiomeE::CLIFFS, 0.1f },
                                                 { BiomeE::COAST, 0.1f },    { BiomeE::DESERT, 0.1f },    { BiomeE::MESA_SUNSET, 0.12f },
                                                 { BiomeE::BAYOU, 0.18f },   { BiomeE::CLOUDS, 0.25f },   { BiomeE::HALLOWEEN, 0.08f },
                                                 { BiomeE::WINTER_VILLAGE, 0.1f }, { BiomeE::RAINBOW, 0.12f }, { BiomeE::HARBOR, 0.12f },
                                                 { BiomeE::SAKURA, 0.1f },   { BiomeE::SAVANNA, 0.08f },  { BiomeE::FUNGI, 0.06f } };
    for( const auto &[biome, chance] : tunnels ) b[static_cast<size_t>( biome )].m_tunnels = chance;
    for( const auto &[biome, chance] : bridges ) b[static_cast<size_t>( biome )].m_bridges = chance;
    return b;
}

/**
 * @brief The table, built once.
 * @return it
 */
const std::vector<BiomeDef> &biomeTable()
{
    static DataCache<std::vector<BiomeDef>> cache;
    return cache.get( makeBiomes );
}

} // namespace

const BiomeDef &biomeDef( BiomeE biome )
{
    const auto &table = biomeTable();
    return table[static_cast<size_t>( biome ) % table.size()];
}

int biomeCount()
{
    return static_cast<int>( biomeTable().size() );
}

Environment blendEnvironment( const Environment &a, const Environment &b, float t )
{
    Environment e;
    e.m_skyZenith = lerpColor( a.m_skyZenith, b.m_skyZenith, t );
    e.m_skyHorizon = lerpColor( a.m_skyHorizon, b.m_skyHorizon, t );
    e.m_sunColor = lerpColor( a.m_sunColor, b.m_sunColor, t );
    e.m_sunIntensity = lerpf( a.m_sunIntensity, b.m_sunIntensity, t );
    e.m_sunElevation = lerpf( a.m_sunElevation, b.m_sunElevation, t );
    e.m_sunAzimuth = lerpf( a.m_sunAzimuth, b.m_sunAzimuth, t );
    e.m_sunSize = lerpf( a.m_sunSize, b.m_sunSize, t );
    e.m_sunStripes = lerpf( a.m_sunStripes, b.m_sunStripes, t );
    e.m_ambientTop = lerpColor( a.m_ambientTop, b.m_ambientTop, t );
    e.m_ambientBottom = lerpColor( a.m_ambientBottom, b.m_ambientBottom, t );
    e.m_fogColor = lerpColor( a.m_fogColor, b.m_fogColor, t );
    e.m_fogDensity = lerpf( a.m_fogDensity, b.m_fogDensity, t );
    e.m_mountainNear = lerpColor( a.m_mountainNear, b.m_mountainNear, t );
    e.m_mountainFar = lerpColor( a.m_mountainFar, b.m_mountainFar, t );
    e.m_mountainHeight = lerpf( a.m_mountainHeight, b.m_mountainHeight, t );
    e.m_cloudAmount = lerpf( a.m_cloudAmount, b.m_cloudAmount, t );
    e.m_cloudColor = lerpColor( a.m_cloudColor, b.m_cloudColor, t );
    e.m_stars = lerpf( a.m_stars, b.m_stars, t );
    e.m_headlights = lerpf( a.m_headlights, b.m_headlights, t );
    e.m_rain = lerpf( a.m_rain, b.m_rain, t );
    e.m_snow = lerpf( a.m_snow, b.m_snow, t );
    e.m_lightning = lerpf( a.m_lightning, b.m_lightning, t );
    e.m_exposure = lerpf( a.m_exposure, b.m_exposure, t );
    e.m_saturation = lerpf( a.m_saturation, b.m_saturation, t );
    e.m_bloom = lerpf( a.m_bloom, b.m_bloom, t );
    e.m_aurora = lerpf( a.m_aurora, b.m_aurora, t );
    e.m_rainbow = lerpf( a.m_rainbow, b.m_rainbow, t );
    e.m_eclipse = lerpf( a.m_eclipse, b.m_eclipse, t );
    e.m_dust = lerpf( a.m_dust, b.m_dust, t );
    e.m_groundFog = lerpf( a.m_groundFog, b.m_groundFog, t );
    e.m_heat = lerpf( a.m_heat, b.m_heat, t );
    e.m_underwater = lerpf( a.m_underwater, b.m_underwater, t );
    e.m_leaves = lerpf( a.m_leaves, b.m_leaves, t );
    e.m_petals = lerpf( a.m_petals, b.m_petals, t );
    e.m_confetti = lerpf( a.m_confetti, b.m_confetti, t );
    e.m_festival = t < 0.5f ? a.m_festival : b.m_festival;
    // A planet fades by size; its place and colour jump to whichever biome has one.
    e.m_planetSize = lerpf( a.m_planetSize, b.m_planetSize, t );
    const Environment &p = b.m_planetSize > 0.0f && ( t > 0.5f || a.m_planetSize <= 0.0f ) ? b : a;
    e.m_planetElevation = p.m_planetElevation;
    e.m_planetAzimuth = p.m_planetAzimuth;
    e.m_planetColor = p.m_planetColor;
    return e;
}

Surface blendSurface( const Surface &a, const Surface &b, float t )
{
    Surface s;
    s.m_groundA = lerpColor( a.m_groundA, b.m_groundA, t );
    s.m_groundB = lerpColor( a.m_groundB, b.m_groundB, t );
    s.m_groundFar = lerpColor( a.m_groundFar, b.m_groundFar, t );
    s.m_road = lerpColor( a.m_road, b.m_road, t );
    s.m_roadAlt = lerpColor( a.m_roadAlt, b.m_roadAlt, t );
    s.m_rumbleA = lerpColor( a.m_rumbleA, b.m_rumbleA, t );
    s.m_rumbleB = lerpColor( a.m_rumbleB, b.m_rumbleB, t );
    s.m_line = lerpColor( a.m_line, b.m_line, t );
    s.m_shoulder = lerpColor( a.m_shoulder, b.m_shoulder, t );
    s.m_water = lerpColor( a.m_water, b.m_water, t );
    s.m_lineGlow = lerpf( a.m_lineGlow, b.m_lineGlow, t );
    s.m_gridGlow = lerpf( a.m_gridGlow, b.m_gridGlow, t );
    s.m_fieldA = lerpColor( a.m_fields > 0.0f ? a.m_fieldA : b.m_fieldA, b.m_fields > 0.0f ? b.m_fieldA : a.m_fieldA, t );
    s.m_fieldB = lerpColor( a.m_fields > 0.0f ? a.m_fieldB : b.m_fieldB, b.m_fields > 0.0f ? b.m_fieldB : a.m_fieldB, t );
    s.m_fieldC = lerpColor( a.m_fields > 0.0f ? a.m_fieldC : b.m_fieldC, b.m_fields > 0.0f ? b.m_fieldC : a.m_fieldC, t );
    s.m_fields = lerpf( a.m_fields, b.m_fields, t );
    s.m_waterGlow = lerpf( a.m_waterGlow, b.m_waterGlow, t );
    s.m_groundGloss = lerpf( a.m_groundGloss, b.m_groundGloss, t );
    s.m_waterGloss = lerpf( a.m_waterGloss, b.m_waterGloss, t );
    return s;
}

Terrain blendTerrain( const Terrain &a, const Terrain &b, float t )
{
    Terrain r;
    r.m_hills = lerpf( a.m_hills, b.m_hills, t );
    r.m_wallLeft = lerpf( a.m_wallLeft, b.m_wallLeft, t );
    r.m_wallRight = lerpf( a.m_wallRight, b.m_wallRight, t );
    r.m_waterLeft = lerpf( a.m_waterLeft, b.m_waterLeft, t );
    r.m_ponds = lerpf( a.m_ponds, b.m_ponds, t );
    r.m_waterRight = lerpf( a.m_waterRight, b.m_waterRight, t );
    r.m_seaLevel = lerpf( a.m_seaLevel, b.m_seaLevel, t );
    return r;
}

} // namespace sunstreak
