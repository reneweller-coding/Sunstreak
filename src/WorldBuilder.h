/**
 * @file WorldBuilder.h
 * @brief The light presets and the chained builder that the world tour's places are written with.
 *
 * Shared by WorldBiomes.cpp and WorldPlaces.cpp only; everything here has internal linkage.
 *
 * @author reneweller
 */
#pragma once

#include "Biome.h"

#include "MathUtil.h"

#include <cstdint>

namespace sunstreak
{

namespace
{


/**
 * @brief A colour from 0xRRGGBB.
 * @param[in] v  the value
 * @return the opaque colour
 */
constexpr Color hx( uint32_t v )
{
    return Color{ static_cast<unsigned char>( v >> 16 ), static_cast<unsigned char>( ( v >> 8 ) & 255 ), static_cast<unsigned char>( v & 255 ), 255 };
}

/** @brief Light presets: time of day and weather. */
enum class SkyE
{
    MORNING,   ///< temperate, clear, the sun still low
    NOON,      ///< high sun, deep blue
    HAZE,      ///< humid, milky
    DESERT,    ///< hot, pale horizon
    GOLDEN,    ///< late afternoon
    SUNSET,    ///< the sun on the horizon
    DUSK,      ///< the blue hour, lights on
    NIGHT,     ///< a city at night
    OVERCAST,  ///< grey, soft light
    RAIN,      ///< grey and wet
    MIST,      ///< morning mist
    ALPINE,    ///< thin, crisp mountain air
    ARCTIC,    ///< a low polar sun
    TROPICAL,  ///< bright, cumulus
    WINTER,    ///< snow falling from a white sky
};

/**
 * @brief An environment for a preset.
 * @param[in] s  the preset
 * @return its light and air
 */
Environment preset( SkyE s )
{
    Environment e{};
    e.m_sunSize = 1.6f;
    e.m_cloudColor = hx( 0xFFFFFF );
    e.m_exposure = 1.0f;
    e.m_saturation = 1.08f;
    e.m_bloom = 0.6f;
    e.m_planetElevation = 20.0f;
    e.m_planetAzimuth = 30.0f;
    e.m_mountainHeight = 4.0f;
    e.m_cloudAmount = 0.35f;
    e.m_sunAzimuth = 25.0f;
    auto set = [&]( uint32_t zenith, uint32_t horizon, uint32_t sun, float intensity, float elevation, uint32_t top, uint32_t bottom, uint32_t fog,
                    float density, uint32_t nearR, uint32_t farR )
    {
        e.m_skyZenith = hx( zenith );
        e.m_skyHorizon = hx( horizon );
        e.m_sunColor = hx( sun );
        e.m_sunIntensity = intensity;
        e.m_sunElevation = elevation;
        e.m_ambientTop = hx( top );
        e.m_ambientBottom = hx( bottom );
        e.m_fogColor = hx( fog );
        e.m_fogDensity = density;
        e.m_mountainNear = hx( nearR );
        e.m_mountainFar = hx( farR );
    };
    switch( s )
    {
    case SkyE::MORNING:
        set( 0x3C7AD6, 0xBED6EC, 0xFFF0D6, 1.2f, 28.0f, 0x7896BE, 0x5A6448, 0xBCD2E6, 0.0011f, 0x648282, 0x96AFCD );
        break;
    case SkyE::NOON:
        set( 0x2864D2, 0xAAC8EB, 0xFFF6E2, 1.35f, 58.0f, 0x82A0C8, 0x6E6E5A, 0xB4CDE8, 0.0009f, 0x6E8796, 0xA0B9D7 );
        e.m_cloudAmount = 0.25f;
        break;
    case SkyE::HAZE:
        set( 0x5A8CC8, 0xD2DCDC, 0xFFF4DC, 1.2f, 50.0f, 0x96AABE, 0x6E785A, 0xCDD7D7, 0.0018f, 0x788C8C, 0xAAB9BE );
        e.m_cloudAmount = 0.5f;
        break;
    case SkyE::DESERT:
        set( 0x3270CE, 0xECD2AA, 0xFFEECD, 1.45f, 60.0f, 0x96A0B9, 0x966E4A, 0xE8CEA6, 0.0008f, 0xBE825F, 0xD7AF8C );
        e.m_cloudAmount = 0.08f;
        e.m_saturation = 1.1f;
        e.m_mountainHeight = 3.5f;
        break;
    case SkyE::GOLDEN:
        set( 0x466EBE, 0xFACE96, 0xFFCE8C, 1.25f, 14.0f, 0x8C8CAA, 0x826446, 0xF0C896, 0.0011f, 0x78646E, 0xBEA0A0 );
        e.m_sunAzimuth = -25.0f;
        e.m_cloudColor = hx( 0xFFDCBE );
        e.m_saturation = 1.12f;
        break;
    case SkyE::SUNSET:
        set( 0x323880, 0xFF965C, 0xFFAC6C, 1.15f, 5.0f, 0x7864A0, 0x785046, 0xEC9678, 0.0011f, 0x603E6E, 0xAA6482 );
        e.m_sunAzimuth = -10.0f;
        e.m_sunSize = 2.6f;
        e.m_cloudAmount = 0.45f;
        e.m_cloudColor = hx( 0xFFAA96 );
        e.m_headlights = 0.25f;
        e.m_saturation = 1.15f;
        e.m_bloom = 0.8f;
        e.m_mountainHeight = 3.0f;
        break;
    case SkyE::DUSK:
        set( 0x1A2456, 0xC8786E, 0xFF9678, 0.5f, 1.0f, 0x464C78, 0x3C323C, 0x6E5A6E, 0.0012f, 0x282846, 0x5A506E );
        e.m_sunAzimuth = 0.0f;
        e.m_sunSize = 2.0f;
        e.m_cloudAmount = 0.3f;
        e.m_cloudColor = hx( 0xB47882 );
        e.m_stars = 0.3f;
        e.m_headlights = 0.8f;
        e.m_bloom = 1.0f;
        e.m_mountainHeight = 3.0f;
        break;
    case SkyE::NIGHT:
        set( 0x080C1E, 0x282846, 0xC8D2FF, 0.2f, 30.0f, 0x32385A, 0x282432, 0x282840, 0.0014f, 0x141628, 0x282A42 );
        e.m_sunSize = 1.0f;
        e.m_cloudAmount = 0.2f;
        e.m_cloudColor = hx( 0x3C3C5A );
        e.m_stars = 0.6f;
        e.m_headlights = 1.0f;
        e.m_bloom = 1.1f;
        e.m_mountainHeight = 3.0f;
        break;
    case SkyE::OVERCAST:
        set( 0x8C96A0, 0xC4C8CC, 0xE6E8EC, 0.7f, 35.0f, 0xAAB0B8, 0x6E7068, 0xB4BAC0, 0.0018f, 0x6E787D, 0x96A0A8 );
        e.m_sunSize = 3.0f;
        e.m_cloudAmount = 0.9f;
        e.m_cloudColor = hx( 0xC8CCD2 );
        e.m_saturation = 0.95f;
        break;
    case SkyE::RAIN:
        set( 0x646C76, 0xA0A6AC, 0xDCDEE2, 0.5f, 35.0f, 0x969CA4, 0x5A5C56, 0x969CA4, 0.0025f, 0x5A646A, 0x828C94 );
        e.m_sunSize = 3.0f;
        e.m_cloudAmount = 1.0f;
        e.m_cloudColor = hx( 0x969CA4 );
        e.m_rain = 0.7f;
        e.m_saturation = 0.9f;
        e.m_headlights = 0.3f;
        break;
    case SkyE::MIST:
        set( 0x96AABE, 0xDCE2E6, 0xFFF8EC, 0.9f, 18.0f, 0xB4BEC8, 0x788278, 0xD6DCE0, 0.0035f, 0x96A5AA, 0xBEC8CE );
        e.m_sunSize = 3.0f;
        e.m_cloudAmount = 0.4f;
        break;
    case SkyE::ALPINE:
        set( 0x1E5AC8, 0xA0C3E8, 0xFFF8EC, 1.4f, 40.0f, 0x82A0D2, 0x5A645A, 0xAAC8E6, 0.0009f, 0x5A6E82, 0xC8D7EB );
        e.m_cloudAmount = 0.3f;
        e.m_mountainHeight = 10.0f;
        break;
    case SkyE::ARCTIC:
        set( 0x5A82BE, 0xE6D2D2, 0xFFE2C8, 1.0f, 8.0f, 0xAAB4D2, 0xA0A6B4, 0xDCDCE6, 0.0015f, 0xB4BED2, 0xD2D7E6 );
        e.m_sunSize = 2.2f;
        e.m_cloudAmount = 0.3f;
        e.m_saturation = 0.95f;
        e.m_mountainHeight = 6.0f;
        break;
    case SkyE::TROPICAL:
        set( 0x1E78DC, 0xAAD7F0, 0xFFF8E6, 1.4f, 62.0f, 0x8CAFD2, 0x6E8264, 0xBEDCEC, 0.0010f, 0x50826E, 0x96BEC8 );
        e.m_cloudAmount = 0.5f;
        e.m_saturation = 1.15f;
        break;
    case SkyE::WINTER:
        set( 0x96A5B9, 0xD7DEE8, 0xEBEEF5, 0.8f, 15.0f, 0xB4BCCD, 0xA0A6B2, 0xCDD4E0, 0.0022f, 0xAAB4C4, 0xC8D0DE );
        e.m_sunSize = 2.4f;
        e.m_cloudAmount = 0.8f;
        e.m_cloudColor = hx( 0xDCE0E8 );
        e.m_snow = 0.6f;
        e.m_saturation = 0.95f;
        e.m_mountainHeight = 6.0f;
        break;
    }
    return e;
}

/** @brief The variants of PropKindE::LANDMARK. */
namespace Lm
{
constexpr int BIG_BEN = 0, TOWER_BRIDGE = 1, COLOSSEUM = 2, LEANING_TOWER = 3, BASILICA = 4, SAGRADA = 5, NEUSCHWANSTEIN = 6,
              MONT_ST_MICHEL = 7, BRANDENBURG_GATE = 8, ACROPOLIS = 9, ST_BASIL = 10, OPEN_ARMS = 11, LIBERTY = 12, OPERA_HOUSE = 13,
              TWIN_TOWERS = 14, SPACE_NEEDLE = 15, CAPITOL = 16, OBELISK = 17, HOLLYWOOD_SIGN = 18, KUWAIT_TOWERS = 19,
              VICTORY_COLUMN = 20, COLOSSI = 21, SPHINX = 22, BOROBUDUR = 23, HAND = 24, ICE_CASTLE = 25, FIRE_CRATER = 26,
              TRIUMPHAL_ARCH = 27, GREAT_ZIMBABWE = 28, ISLAND_CHURCH = 29, SEA_TORII = 30;
} // namespace Lm

/** @brief Fills one biome by chained calls; the defaults make a temperate two-lane country road. */
class W
{
public:
    /**
     * @brief Starts a place.
     * @param[out] table    the biome table
     * @param[in]  id       which biome
     * @param[in]  name     its name on screen
     * @param[in]  country  its country
     * @param[in]  sky      its light
     */
    W( BiomeDef *table, BiomeE id, const char *name, const char *country, SkyE sky ) : m_d( table[static_cast<size_t>( id )] )
    {
        m_d = BiomeDef{};
        m_d.m_name = name;
        m_d.m_country = country;
        m_d.m_env = preset( sky );
        Surface &s = m_d.m_surface;
        s.m_groundA = hx( 0x62A03E );
        s.m_groundB = hx( 0x569236 );
        s.m_groundFar = hx( 0x689246 );
        road( 0x5A5C66 );
        s.m_rumbleA = hx( 0xD62828 );
        s.m_rumbleB = hx( 0xF0F0F0 );
        s.m_shoulder = hx( 0x968E74 );
        s.m_water = hx( 0x3C6EA0 );
        m_d.m_terrain = { 12.0f, 4.0f, 4.0f, 0.0f, 0.0f };
        m_d.m_grip = 1.0f;
        m_d.m_curviness = 0.5f;
        m_d.m_sharpness = 0.4f;
        m_d.m_hilliness = 0.5f;
        m_d.m_lanes = 2;
        m_d.m_traffic = 0.8f;
        m_d.m_postSpacing = 40.0f;
        if( sky == SkyE::NIGHT || sky == SkyE::DUSK )
        {
            m_windows = hx( 0xFFD696 );
            m_d.m_lampSpacing = 35.0f;
        }
        switch( sky )
        {
        case SkyE::DESERT: m_d.m_climate = ClimateE::ARID; break;
        case SkyE::ARCTIC:
        case SkyE::WINTER:
        case SkyE::ALPINE: m_d.m_climate = ClimateE::COLD; break;
        case SkyE::TROPICAL:
        case SkyE::HAZE: m_d.m_climate = ClimateE::TROPICAL; break;
        default: m_d.m_climate = ClimateE::TEMPERATE; break;
        }
    }

    /**
     * @brief Sets the weather fronts the place gets (the sky preset decides otherwise).
     * @param[in] c  the climate
     * @return this
     */
    W &climate( ClimateE c )
    {
        m_d.m_climate = c;
        return *this;
    }

    // ---- Light and air.

    /**
     * @brief Moves the sun.
     * @param[in] elevation  height (degrees)
     * @param[in] azimuth    direction (degrees)
     * @return this
     */
    W &sun( float elevation, float azimuth )
    {
        m_d.m_env.m_sunElevation = elevation;
        m_d.m_env.m_sunAzimuth = azimuth;
        return *this;
    }

    /**
     * @brief Sets the haze.
     * @param[in] density  per metre
     * @param[in] color    its colour (0: keep)
     * @return this
     */
    W &fog( float density, uint32_t color = 0 )
    {
        m_d.m_env.m_fogDensity = density;
        if( color ) m_d.m_env.m_fogColor = hx( color );
        return *this;
    }

    /**
     * @brief Sets the clouds.
     * @param[in] amount  0..1
     * @param[in] color   their colour (0: keep)
     * @return this
     */
    W &clouds( float amount, uint32_t color = 0 )
    {
        m_d.m_env.m_cloudAmount = amount;
        if( color ) m_d.m_env.m_cloudColor = hx( color );
        return *this;
    }

    /**
     * @brief Sets the silhouettes of the mountains on the horizon.
     * @param[in] nearR   nearer range
     * @param[in] farR    farther range
     * @param[in] height  degrees
     * @return this
     */
    W &ranges( uint32_t nearR, uint32_t farR, float height )
    {
        m_d.m_env.m_mountainNear = hx( nearR );
        m_d.m_env.m_mountainFar = hx( farR );
        m_d.m_env.m_mountainHeight = height;
        return *this;
    }

    /**
     * @brief Sets the colours of the sky.
     * @param[in] zenith   straight up
     * @param[in] horizon  at the horizon
     * @return this
     */
    W &sky( uint32_t zenith, uint32_t horizon )
    {
        m_d.m_env.m_skyZenith = hx( zenith );
        m_d.m_env.m_skyHorizon = hx( horizon );
        return *this;
    }

    /**
     * @brief Lets it rain.
     * @param[in] r  0..1
     * @return this
     */
    W &rain( float r )
    {
        m_d.m_env.m_rain = r;
        return *this;
    }

    /**
     * @brief Lets it snow.
     * @param[in] s  0..1
     * @return this
     */
    W &snow( float s )
    {
        m_d.m_env.m_snow = s;
        return *this;
    }

    /**
     * @brief Sets the colour grading.
     * @param[in] saturation  colourfulness
     * @return this
     */
    W &look( float saturation )
    {
        m_d.m_env.m_saturation = saturation;
        return *this;
    }

    // ---- Ground.

    /**
     * @brief Sets the ground.
     * @param[in] a         stripes one
     * @param[in] b         stripes two
     * @param[in] farLand   far from the road
     * @return this
     */
    W &land( uint32_t a, uint32_t b, uint32_t farLand )
    {
        m_d.m_surface.m_groundA = hx( a );
        m_d.m_surface.m_groundB = hx( b );
        m_d.m_surface.m_groundFar = hx( farLand );
        return *this;
    }

    /**
     * @brief Sets the verge (and steep rock faces).
     * @param[in] c  its colour
     * @return this
     */
    W &verge( uint32_t c )
    {
        m_d.m_surface.m_shoulder = hx( c );
        return *this;
    }

    /**
     * @brief Sets the water.
     * @param[in] c      its colour
     * @param[in] gloss  0..1 mirror
     * @return this
     */
    W &water( uint32_t c, float gloss = 0.95f )
    {
        m_d.m_surface.m_water = hx( c );
        m_d.m_surface.m_waterGloss = gloss;
        return *this;
    }

    /**
     * @brief Sets the road's colours.
     * @param[in] asphalt  the surface
     * @param[in] line     the markings
     * @return this
     */
    W &road( uint32_t asphalt, uint32_t line = 0xF0F0F0 )
    {
        m_d.m_surface.m_road = hx( asphalt );
        m_d.m_surface.m_roadAlt = scaleColor( hx( asphalt ), 0.93f );
        m_d.m_surface.m_line = hx( line );
        if( line == asphalt )
        { // No markings: a gravel or dirt road, its edges of the same stuff.
            m_d.m_unpaved = 1;
            m_d.m_surface.m_rumbleA = scaleColor( hx( asphalt ), 0.88f );
            m_d.m_surface.m_rumbleB = scaleColor( hx( asphalt ), 1.06f );
        }
        return *this;
    }

    /**
     * @brief Makes the road a sand track (after road()).
     * @return this
     */
    W &sand()
    {
        m_d.m_unpaved = 2;
        return *this;
    }

    /**
     * @brief Sets the kerbs.
     * @param[in] a  one colour
     * @param[in] b  the other
     * @return this
     */
    W &kerbs( uint32_t a, uint32_t b )
    {
        m_d.m_surface.m_rumbleA = hx( a );
        m_d.m_surface.m_rumbleB = hx( b );
        return *this;
    }

    /**
     * @brief Lays out fields.
     * @param[in] share  0..1 of the land
     * @param[in] a      crop one
     * @param[in] b      crop two
     * @param[in] c      crop three
     * @return this
     */
    W &fields( float share, uint32_t a, uint32_t b, uint32_t c )
    {
        m_d.m_surface.m_fields = share;
        m_d.m_surface.m_fieldA = hx( a );
        m_d.m_surface.m_fieldB = hx( b );
        m_d.m_surface.m_fieldC = hx( c );
        return *this;
    }

    /**
     * @brief Makes the ground shine (salt, ice).
     * @param[in] g  0..1
     * @return this
     */
    W &shine( float g )
    {
        m_d.m_surface.m_groundGloss = g;
        return *this;
    }

    // ---- Land.

    /**
     * @brief Shapes the land.
     * @param[in] amplitude  the far hills (m)
     * @param[in] wallLeft   rise (or drop) on the left (m)
     * @param[in] wallRight  and on the right
     * @param[in] ponds      0..1 ponds and lakes
     * @return this
     */
    W &hills( float amplitude, float wallLeft, float wallRight, float ponds = 0.0f )
    {
        m_d.m_terrain.m_hills = amplitude;
        m_d.m_terrain.m_wallLeft = wallLeft;
        m_d.m_terrain.m_wallRight = wallRight;
        m_d.m_terrain.m_ponds = ponds;
        return *this;
    }

    /**
     * @brief Puts the sea (or a lake or a river) beside the road.
     * @param[in] left   0..1 on the left
     * @param[in] right  0..1 on the right
     * @param[in] level  its height below the road
     * @return this
     */
    W &sea( float left, float right, float level = -4.5f )
    {
        m_d.m_terrain.m_waterLeft = left;
        m_d.m_terrain.m_waterRight = right;
        m_d.m_terrain.m_seaLevel = level;
        return *this;
    }

    // ---- Road.

    /**
     * @brief Sets the road's size, traffic, grip and music.
     * @param[in] lanes    lanes
     * @param[in] traffic  density factor
     * @param[in] grip     tyre grip
     * @param[in] station  radio station in AUTO
     * @return this
     */
    W &drive( int lanes, float traffic, float grip, int station )
    {
        m_d.m_lanes = lanes;
        m_d.m_traffic = traffic;
        m_d.m_grip = grip;
        m_d.m_station = station;
        return *this;
    }

    /**
     * @brief Sets how the road bends and climbs.
     * @param[in] curvy  how often it bends
     * @param[in] sharp  how tight
     * @param[in] hilly  how much it climbs
     * @return this
     */
    W &bends( float curvy, float sharp, float hilly )
    {
        m_d.m_curviness = curvy;
        m_d.m_sharpness = sharp;
        m_d.m_hilliness = hilly;
        return *this;
    }

    /**
     * @brief Sets the delineator posts.
     * @param[in] spacing  metres (0: none)
     * @return this
     */
    W &posts( float spacing )
    {
        m_d.m_postSpacing = spacing;
        return *this;
    }

    /**
     * @brief Sets the street lamps.
     * @param[in] spacing  metres (0: none)
     * @return this
     */
    W &lamps( float spacing )
    {
        m_d.m_lampSpacing = spacing;
        return *this;
    }

    /**
     * @brief Sets the overhead sign gantries.
     * @param[in] spacing  metres (0: none)
     * @return this
     */
    W &gantries( float spacing )
    {
        m_d.m_gantrySpacing = spacing;
        return *this;
    }

    /**
     * @brief Puts guard rails on both sides.
     * @return this
     */
    W &rails()
    {
        m_d.m_railLeft = m_d.m_railRight = true;
        return *this;
    }

    /**
     * @brief Sets the tunnels.
     * @param[in] chance  per suitable stretch
     * @return this
     */
    W &tunnels( float chance )
    {
        m_d.m_tunnels = chance;
        return *this;
    }

    /**
     * @brief Sets the bridges.
     * @param[in] chance  per suitable straight
     * @param[in] style   how they are built
     * @param[in] color   their paint or stone (0: concrete)
     * @return this
     */
    W &bridges( float chance, BridgeE style, uint32_t color = 0 )
    {
        m_d.m_bridges = chance;
        m_d.m_bridge = style;
        if( color ) m_d.m_bridgeColor = hx( color );
        return *this;
    }

    // ---- Props.

    /**
     * @brief Adds a scatter rule.
     * @param[in] kind     what
     * @param[in] chance   per segment and side
     * @param[in] nearest  least distance from the road (m)
     * @param[in] farthest largest distance (m)
     * @param[in] smin     smallest scale
     * @param[in] smax     largest scale
     * @return this
     */
    W &p( PropKindE kind, float chance, float nearest, float farthest, float smin = 0.8f, float smax = 1.2f )
    {
        PropRule r{ kind, chance, nearest, farthest, smin, smax };
        if( m_windows.a ) r.m_c = m_windows;
        m_d.m_props.push_back( r );
        return *this;
    }

    /**
     * @brief Paints the last rule's props.
     * @param[in] a  key colour one
     * @param[in] b  key colour two (0: the kind's)
     * @param[in] c  key colour three (0: the kind's, or the lit windows)
     * @return this
     */
    W &c( uint32_t a, uint32_t b = 0, uint32_t c = 0 )
    {
        PropRule &r = m_d.m_props.back();
        r.m_a = hx( a );
        if( b ) r.m_b = hx( b );
        if( c ) r.m_c = hx( c );
        return *this;
    }

    /**
     * @brief Gives each of the last rule's props another paint.
     * @return this
     */
    W &pastel()
    {
        m_d.m_props.back().m_pastel = true;
        return *this;
    }

    /**
     * @brief Puts the last rule's props on the water.
     * @return this
     */
    W &floating()
    {
        m_d.m_props.back().m_onWater = true;
        return *this;
    }

    /**
     * @brief Picks one variant for the last rule.
     * @param[in] n  which
     * @return this
     */
    W &v( int n )
    {
        m_d.m_props.back().m_variant = static_cast<int8_t>( n );
        return *this;
    }

    /**
     * @brief Places the last rule's props a few times per stage at fixed spots (landmarks).
     * @param[in] n  how many per stage
     * @return this
     */
    W &once( int n = 1 )
    {
        m_d.m_props.back().m_perStage = static_cast<uint8_t>( n );
        return *this;
    }

    /**
     * @brief Puts a landmark beside the road, once per stage.
     * @param[in] variant   which (Lm)
     * @param[in] nearest   least distance from the road (m)
     * @param[in] farthest  largest distance (m)
     * @param[in] scale     its size
     * @return this
     */
    W &landmark( int variant, float nearest, float farthest, float scale = 1.0f )
    {
        return p( PropKindE::LANDMARK, 0.0f, nearest, farthest, scale, scale ).v( variant ).once();
    }

    /**
     * @brief Lights the windows of the props that follow.
     * @param[in] color  their light
     * @return this
     */
    W &windows( uint32_t color )
    {
        m_windows = hx( color );
        return *this;
    }

private:
    BiomeDef &m_d;            ///< the place
    Color     m_windows{};    ///< window light for the rules that follow (alpha 0: dark)
};

} // namespace

} // namespace sunstreak
