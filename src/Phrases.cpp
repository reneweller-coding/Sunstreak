/**
 * @file Phrases.cpp
 * @brief Reads data/phrases.txt.
 * @author Rene Weller
 */
#include "Phrases.h"

#include "Biome.h"
#include "Data.h"

#include <map>

namespace sunstreak
{

namespace
{

int g_phraseErrors = 0;   ///< wrong lines in the last read

/** @brief The phrases by key (a country, fleet:NAME or default). */
using PhraseMapT = std::map<std::string, Phrases>;

/**
 * @brief The file's phrases, read again after a reload.
 * @return them
 */
const PhraseMapT &phrases()
{
    static DataCache<PhraseMapT> cache;
    return cache.get( []
    {
        g_phraseErrors = 0;
        PhraseMapT out;
        for( const std::string &line : dataLines( dataText( "phrases.txt" ) ) )
        {
            const std::vector<std::string> f = splitLine( line, '|' );
            if( f.size() != 7 || f[0].empty() )
            {
                ++g_phraseErrors;
                continue;
            }
            const bool known = f[0] == "default" || f[0].rfind( "fleet:", 0 ) == 0 ||
                               ( countryInfo( f[0].c_str() ).m_name && std::string( countryInfo( f[0].c_str() ).m_name ) == f[0] );
            if( !known ) ++g_phraseErrors;
            Phrases p{ f[1], f[2], f[3], f[4], f[5], {} };
            for( const std::string &name : splitLine( f[6], ',' ) )
                if( !name.empty() ) p.m_names.push_back( name );
            if( p.m_names.empty() ) ++g_phraseErrors;
            out[f[0]] = std::move( p );
        }
        if( !out.count( "default" ) )
        {
            ++g_phraseErrors;
            out["default"] = Phrases{ "HELLO", "THANKS", "RACE YOU!", "TOO SLOW!", "DAMN!", { "ALEX" } };
        }
        return out;
    } );
}

} // namespace

const Phrases &phrasesOf( const char *country )
{
    const PhraseMapT &all = phrases();
    if( country )
    {
        auto it = all.find( country );
        if( it != all.end() ) return it->second;
        static const char *const fleets[] = { "EUROPE", "NORTH_AMERICA", "BRITISH", "LATIN", "SOUTH_ASIA", "SOUTHEAST_ASIA",
                                              "EAST_ASIA", "OCEANIA", "AFRICA", "ARABIA", "EASTERN", "ISLANDS" };
        const size_t fleet = static_cast<size_t>( countryInfo( country ).m_fleet );
        if( fleet < std::size( fleets ) )
        {
            it = all.find( std::string( "fleet:" ) + fleets[fleet] );
            if( it != all.end() ) return it->second;
        }
    }
    return all.at( "default" );
}

int phraseErrors()
{
    phrases();
    return g_phraseErrors;
}

} // namespace sunstreak
