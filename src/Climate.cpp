/**
 * @file Climate.cpp
 * @brief Reading data/climate.txt.
 * @author reneweller
 */
#include "Climate.h"

#include "Data.h"
#include "MathUtil.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace sunstreak
{

namespace
{

int g_climateErrors = 0;   ///< lines of the climate file that were wrong

/**
 * @brief Twelve monthly values at a time of year, each taken for the middle of its month and joined linearly.
 * @param[in] v      the values, January first
 * @param[in] phase  the time of year (0: 1 January)
 * @return the value
 */
float monthly( const float ( &v )[12], float phase )
{
    const float f = phase * 12.0f - 0.5f;
    const float whole = std::floor( f );
    const int   i = ( ( static_cast<int>( whole ) % 12 ) + 12 ) % 12;
    return lerpf( v[i], v[( i + 1 ) % 12], f - whole );
}

} // namespace

const PlaceClimate *placeClimate( BiomeE place )
{
    static DataCache<std::vector<PlaceClimate>> cache;
    const auto &all = cache.get( []
    {
        g_climateErrors = 0;
        std::vector<PlaceClimate> out( static_cast<size_t>( biomeCount() ) );
        for( const std::string &line : dataLines( dataText( "climate.txt" ) ) )
        {
            // climate PLACE t1 .. t12 p1 .. p12
            const std::vector<std::string> w = splitLine( line, ' ' );
            const int                      id = w.size() == 26 && w[0] == "climate" ? biomeId( w[1] ) : -1;
            if( id < 0 )
            {
                ++g_climateErrors;
                continue;
            }
            PlaceClimate &c = out[static_cast<size_t>( id )];
            for( size_t m = 0; m < 12; ++m )
            {
                c.m_t[m] = std::strtof( w[2 + m].c_str(), nullptr );
                c.m_p[m] = std::max( 0.0f, std::strtof( w[14 + m].c_str(), nullptr ) );
            }
            c.m_cold = *std::min_element( c.m_t, c.m_t + 12 );
            c.m_rain = 0.0f;
            for( float p : c.m_p ) c.m_rain += p;
            c.m_known = true;
        }
        return out;
    } );
    const size_t i = static_cast<size_t>( place );
    return i < all.size() && all[i].m_known ? &all[i] : nullptr;
}

float climateTemperature( const PlaceClimate &c, float phase )
{
    return monthly( c.m_t, phase );
}

float climatePrecipitation( const PlaceClimate &c, float phase )
{
    return monthly( c.m_p, phase );
}

int climateErrors()
{
    placeClimate( static_cast<BiomeE>( 0 ) );
    return g_climateErrors;
}

} // namespace sunstreak
