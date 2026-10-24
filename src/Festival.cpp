/**
 * @file Festival.cpp
 * @brief Reading data/festivals.txt and finding a place's festival.
 * @author reneweller
 */
#include "Festival.h"

#include "Data.h"

#include <cstdlib>
#include <cstring>

namespace sunstreak
{

namespace
{

int g_festivalErrors = 0;   ///< lines of the festivals file that were wrong

/**
 * @brief A date as the day of the year.
 * @param[in] mmdd  "MM-DD"
 * @return 0..364, or -1
 */
int dayOfYear( const std::string &mmdd )
{
    static const int starts[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    if( mmdd.size() != 5 || mmdd[2] != '-' ) return -1;
    const int m = std::atoi( mmdd.substr( 0, 2 ).c_str() ), d = std::atoi( mmdd.substr( 3, 2 ).c_str() );
    if( m < 1 || m > 12 || d < 1 || d > 31 ) return -1;
    return starts[m - 1] + d - 1;
}

/**
 * @brief Colours from the words of a line.
 * @param[in] words  the words, the first being the line's keyword
 * @return the colours
 */
std::vector<Color> colours( const std::vector<std::string> &words )
{
    std::vector<Color> out;
    for( size_t i = 1; i < words.size(); ++i )
    {
        const auto v = static_cast<uint32_t>( std::strtoul( words[i].c_str(), nullptr, 16 ) );
        out.push_back( Color{ static_cast<unsigned char>( v >> 16 ), static_cast<unsigned char>( v >> 8 ), static_cast<unsigned char>( v ), 255 } );
    }
    return out;
}

/**
 * @brief Trims spaces.
 * @param[in] s  the text
 * @return it, trimmed
 */
std::string trimmed( const std::string &s )
{
    const size_t a = s.find_first_not_of( " \t" );
    if( a == std::string::npos ) return {};
    return s.substr( a, s.find_last_not_of( " \t" ) - a + 1 );
}

} // namespace

const std::vector<Festival> &festivals()
{
    static DataCache<std::vector<Festival>> cache;
    return cache.get( []
    {
        g_festivalErrors = 0;
        std::vector<Festival> out;
        for( const std::string &line : dataLines( dataText( "festivals.txt" ) ) )
        {
            if( line.rfind( "festival ", 0 ) == 0 )
            {
                // festival NAME | WHAT | MM-DD MM-DD | WHERE, WHERE
                const std::vector<std::string> parts = splitLine( line.substr( 9 ), '|' );
                if( parts.size() < 4 )
                {
                    ++g_festivalErrors;
                    continue;
                }
                Festival f;
                f.m_name = trimmed( parts[0] );
                f.m_what = trimmed( parts[1] );
                const std::vector<std::string> days = splitLine( trimmed( parts[2] ), ' ' );
                f.m_from = days.size() == 2 ? dayOfYear( days[0] ) : -1;
                f.m_to = days.size() == 2 ? dayOfYear( days[1] ) : -1;
                if( f.m_from < 0 || f.m_to < 0 ) ++g_festivalErrors;
                for( const std::string &w : splitLine( parts[3], ',' ) )
                {
                    const std::string place = trimmed( w );
                    if( place.rfind( "north ", 0 ) == 0 ) f.m_north = static_cast<float>( std::atof( place.c_str() + 6 ) );
                    else if( !place.empty() ) f.m_where.push_back( place );
                }
                out.push_back( std::move( f ) );
                continue;
            }
            if( out.empty() )
            {
                ++g_festivalErrors;
                continue;
            }
            Festival                      &f = out.back();
            const std::vector<std::string> words = splitLine( line, ' ' );
            const std::string             &what = words.empty() ? line : words[0];
            if( what == "bunting" ) f.m_bunting = colours( words );
            else if( what == "lanterns" ) f.m_lanterns = colours( words );
            else if( what == "lights" ) f.m_lights = colours( words );
            else if( what == "lamps" ) f.m_lamps = colours( words );
            else if( what == "stalls" ) f.m_stalls = colours( words );
            else if( what == "tents" ) f.m_tents = colours( words );
            else if( what == "confetti" ) f.m_confetti = colours( words );
            else if( what == "fireworks" ) f.m_fireworks = colours( words );
            else if( what == "trees" ) f.m_trees = true;
            else if( what == "aurora" ) f.m_aurora = true;
            else ++g_festivalErrors;
        }
        return out;
    } );
}

int festivalErrors()
{
    festivals();
    return g_festivalErrors;
}

int festivalAt( BiomeE place, float phase )
{
    if( phase < 0.0f ) return -1;
    const BiomeDef &d = biomeDef( place );
    if( !d.m_country || d.m_underwater ) return -1;   // the seas and the fantasy biomes keep no festivals
    const int   day = static_cast<int>( phase * 365.0f ) % 365;
    const auto &all = festivals();
    for( size_t i = 0; i < all.size(); ++i )
    {
        const Festival &f = all[i];
        const bool      on = f.m_from <= f.m_to ? day >= f.m_from && day <= f.m_to : day >= f.m_from || day <= f.m_to;
        if( !on ) continue;
        bool here = false;
        for( const std::string &w : f.m_where )
        {
            const size_t slash = w.find( '/' );
            if( w == "*" ) here = true;
            else if( slash == std::string::npos ) here = here || w == d.m_country;
            else here = here || ( w.compare( 0, slash, d.m_country ) == 0 && slash == std::strlen( d.m_country ) && d.m_region && w.substr( slash + 1 ) == d.m_region );
        }
        GeoPoint where;
        if( !here && f.m_north > 0.0f && placeLocation( place, where ) ) here = where.m_lat >= f.m_north;
        if( here ) return static_cast<int>( i );
    }
    return -1;
}

float dateFromText( const std::string &mmdd )
{
    const int day = dayOfYear( mmdd );
    return day < 0 ? -1.0f : ( static_cast<float>( day ) + 0.5f ) / 365.0f;
}

} // namespace sunstreak
