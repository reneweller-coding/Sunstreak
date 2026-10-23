/**
 * @file Biome.cpp
 * @brief The eight biomes and their blending.
 * @author reneweller
 */
#include "Biome.h"

#include "MathUtil.h"

#include <array>

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
    return e;
}

/**
 * @brief Builds the table of biomes.
 * @return all of them, indexed by BiomeE
 */
std::array<BiomeDef, static_cast<size_t>( BiomeE::COUNT )> makeBiomes()
{
    std::array<BiomeDef, static_cast<size_t>( BiomeE::COUNT )> b{};

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
    return b;
}

} // namespace

const BiomeDef &biomeDef( BiomeE biome )
{
    static const auto table = makeBiomes();
    return table[static_cast<size_t>( biome ) % table.size()];
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
    return r;
}

} // namespace sunstreak
