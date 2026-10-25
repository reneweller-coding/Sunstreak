/**
 * @file ScriptNames.cpp
 * @brief Reading the traced names of data/script-names.txt.
 * @author Rene Weller
 */
#include "ScriptNames.h"

#include "Data.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <unordered_map>

namespace sunstreak
{

namespace
{

constexpr float Unit = 1.0f / 64.0f;   ///< the file's units per em

int g_errors = 0;   ///< unreadable lines in the last read

/**
 * @brief The names, one per place (an empty one where it writes in Latin letters).
 * @return them, by place
 */
const std::vector<ScriptName> &names()
{
    static DataCache<std::vector<ScriptName>> cache;
    return cache.get(
        []
        {
            g_errors = 0;
            std::vector<ScriptName>              out( static_cast<size_t>( biomeCount() ) );
            std::unordered_map<std::string, int> byKey;
            for ( int b = 0; b < biomeCount(); ++b ) byKey[biomeKey( static_cast<BiomeE>( b ) )] = b;
            for ( const std::string &line : dataLines( dataText( "script-names.txt" ) ) )
            {
                const std::vector<std::string> f = splitLine( line, '|' );
                const auto                     at = f.size() == 3 ? byKey.find( f[0] ) : byKey.end();
                if ( at == byKey.end() )
                {
                    ++g_errors;
                    continue;
                }
                ScriptName &n = out[static_cast<size_t>( at->second )];
                n.m_width = static_cast<float>( std::atof( f[1].c_str() ) ) * Unit;
                n.m_low = 1e9f;
                n.m_high = -1e9f;
                for ( const std::string &stroke : splitLine( f[2], ';' ) )
                {
                    std::vector<Vector2> pts;
                    for ( const std::string &pair : splitLine( stroke, ' ' ) )
                    {
                        const auto comma = pair.find( ',' );
                        if ( comma == std::string::npos ) continue;
                        pts.push_back( Vector2{ static_cast<float>( std::atof( pair.c_str() ) ) * Unit,
                                                static_cast<float>( std::atof( pair.c_str() + comma + 1 ) ) * Unit } );
                        n.m_low = std::min( n.m_low, pts.back().y );
                        n.m_high = std::max( n.m_high, pts.back().y );
                    }
                    if ( pts.size() == 1 ) pts.push_back( pts[0] );   // a dot
                    for ( size_t i = 0; i + 1 < pts.size(); ++i ) n.m_lines.push_back( { pts[i], pts[i + 1] } );
                }
                if ( n.m_lines.empty() )
                {
                    ++g_errors;
                    n = ScriptName{};
                }
            }
            return out;
        } );
}

}   // namespace

const ScriptName *scriptName( BiomeE place )
{
    const auto  &all = names();
    const size_t i = static_cast<size_t>( place );
    return i < all.size() && !all[i].m_lines.empty() ? &all[i] : nullptr;
}

int scriptNameErrors()
{
    names();
    return g_errors;
}

}   // namespace sunstreak
