/**
 * @file Season.cpp
 * @brief The seasons by place and time of year.
 * @author reneweller
 */
#include "Season.h"

#include "Climate.h"
#include "MathUtil.h"

#include <cmath>
#include <ctime>

namespace sunstreak
{

namespace
{

constexpr double YearMetres = 400000.0;   ///< the tour clock: a year every 400 km

/**
 * @brief How near a time of year lies to a season's middle.
 * @param[in] phase   the time of year
 * @param[in] middle  the season's middle
 * @param[in] reach   how far it reaches either way
 * @return 1 in the middle, 0 beyond its reach
 */
float near( float phase, float middle, float reach )
{
    const float d = std::abs( std::remainder( phase - middle, 1.0f ) );
    return 1.0f - smoothstepf( reach * 0.45f, reach, d );
}

/**
 * @brief The seasons of a place from its real climate: snow lies where the air stayed below freezing, trees that lose
 *        their leaves turn while it cools and bud while it warms, and the tropics keep their one look all year.
 * @param[in] c      the place's climate
 * @param[in] phase  the date (0: 1 January; the calendar, the same in both hemispheres)
 * @return the look
 */
SeasonLook climateSeason( const PlaceClimate &c, float phase )
{
    SeasonLook  look;
    const float now = climateTemperature( c, phase );
    // Snow and bare branches follow the air a fortnight late.
    const float lag = climateTemperature( c, phase - 0.04f );
    const float cooling = climateTemperature( c, phase - 0.05f ) - climateTemperature( c, phase + 0.05f );
    // Dry places keep little snow, unless it is bitterly cold there.
    const float moist = smoothstepf( 120.0f, 350.0f, c.m_rain );
    const float bitter = 1.0f - smoothstepf( -16.0f, -6.0f, c.m_cold );
    look.m_winter = ( 1.0f - smoothstepf( -3.0f, 4.0f, lag ) ) * ( moist + ( 1.0f - moist ) * bitter );
    // Trees that lose their leaves: a cold month, and rain enough to grow them.
    const float leafy = ( 1.0f - smoothstepf( 2.0f, 9.0f, c.m_cold ) ) * smoothstepf( 200.0f, 500.0f, c.m_rain );
    look.m_autumn = leafy * smoothstepf( 0.0f, 3.0f, cooling ) * smoothstepf( 2.0f, 8.0f, now ) * ( 1.0f - smoothstepf( 13.0f, 19.0f, now ) );
    look.m_spring = leafy * smoothstepf( 0.0f, 3.0f, -cooling ) * smoothstepf( 1.0f, 5.0f, now ) * ( 1.0f - smoothstepf( 11.0f, 16.0f, now ) );
    look.m_bare = leafy * ( 1.0f - smoothstepf( 1.0f, 7.0f, lag ) );
    return look;
}

} // namespace

float SeasonClock::phaseAt( double s ) const
{
    if( m_mode == 2 ) return -1.0f;
    if( m_mode == 0 ) return m_today;
    const double p = std::fmod( static_cast<double>( m_today ) + s / YearMetres, 1.0 );
    return static_cast<float>( p < 0.0 ? p + 1.0 : p );
}

float SeasonClock::dateAt( double s ) const
{
    return m_mode == 2 ? m_today : phaseAt( s );
}

float todayPhase()
{
    const std::time_t now = std::time( nullptr );
    std::tm           t{};
#ifdef _WIN32
    localtime_s( &t, &now );
#else
    localtime_r( &now, &t );
#endif
    return static_cast<float>( t.tm_yday ) / 365.0f;
}

SeasonLook seasonAt( BiomeE place, float phase )
{
    SeasonLook look;
    const BiomeDef &d = biomeDef( place );
    if( phase < 0.0f || d.m_underwater || d.m_climate == ClimateE::NONE ) return look;
    // Where the data knows the place's climate, that decides; elsewhere the latitude does.
    if( const PlaceClimate *c = placeClimate( place ) ) return climateSeason( *c, phase );
    if( d.m_climate != ClimateE::TEMPERATE && d.m_climate != ClimateE::COLD ) return look;
    // South of the equator the year turns round; near it there are no seasons to speak of.
    GeoPoint where{ 45.0f, 0.0f };
    placeLocation( place, where );
    const float lat = std::abs( where.m_lat );
    if( lat < 18.0f ) return look;
    if( where.m_lat < 0.0f ) phase = std::fmod( phase + 0.5f, 1.0f );
    const bool  cold = d.m_climate == ClimateE::COLD;
    const float north = smoothstepf( 45.0f, 65.0f, lat );
    // Winter round the middle of January, longer in the cold and the far north; snow only north of the warm coasts.
    look.m_winter = near( phase, 0.04f, 0.15f + ( cold ? 0.07f : 0.0f ) + 0.06f * north ) * ( cold ? 1.0f : smoothstepf( 32.0f, 46.0f, lat ) );
    look.m_spring = near( phase, 0.31f, 0.1f ) * ( 1.0f - 0.5f * north );
    look.m_autumn = near( phase, 0.8f, 0.12f );
    look.m_bare = look.m_winter;
    return look;
}

void seasonSurface( Surface &s, const SeasonLook &look )
{
    auto toward = [&]( Color &c, Color to, float k ) { c = lerpColor( c, to, clampf( k, 0.0f, 1.0f ) ); };
    if( look.m_autumn > 0.0f )
    {
        // Stubble and fallen leaves.
        toward( s.m_groundA, Color{ 176, 150, 80, 255 }, 0.5f * look.m_autumn );
        toward( s.m_groundB, Color{ 168, 140, 74, 255 }, 0.5f * look.m_autumn );
        toward( s.m_groundFar, Color{ 164, 128, 72, 255 }, 0.55f * look.m_autumn );
        for( Color *f : { &s.m_fieldA, &s.m_fieldB, &s.m_fieldC } ) toward( *f, Color{ 200, 170, 100, 255 }, 0.6f * look.m_autumn );
    }
    if( look.m_spring > 0.0f )
    {
        // Fresh green.
        toward( s.m_groundA, Color{ 104, 186, 76, 255 }, 0.25f * look.m_spring );
        toward( s.m_groundB, Color{ 98, 176, 70, 255 }, 0.25f * look.m_spring );
        toward( s.m_groundFar, Color{ 96, 160, 74, 255 }, 0.2f * look.m_spring );
    }
    if( look.m_winter > 0.0f )
    {
        // Snow on the land, a little on the gravel beside the road.
        toward( s.m_groundA, Color{ 232, 236, 244, 255 }, 0.85f * look.m_winter );
        toward( s.m_groundB, Color{ 224, 230, 240, 255 }, 0.85f * look.m_winter );
        toward( s.m_groundFar, Color{ 226, 232, 242, 255 }, 0.85f * look.m_winter );
        for( Color *f : { &s.m_fieldA, &s.m_fieldB, &s.m_fieldC } ) toward( *f, Color{ 230, 234, 242, 255 }, 0.85f * look.m_winter );
        toward( s.m_shoulder, Color{ 200, 202, 210, 255 }, 0.4f * look.m_winter );
    }
}

} // namespace sunstreak
