/**
 * @file Data.cpp
 * @brief Reading the data files: from the folder SUNSTREAK_DATA names, else from the copies built into the program.
 * @author Rene Weller
 */
#include "Data.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sunstreak
{

namespace
{

std::string      g_found;               ///< the data folder a reload found (empty: none)
std::atomic<int> g_generation{ 0 };     ///< bumped by every reload

/**
 * @brief The folder that overrides the built-in data.
 * @return its path, or empty
 */
std::string overrideDir()
{
    const char *dir = std::getenv( "SUNSTREAK_DATA" );
    return dir ? std::string( dir ) : g_found;
}

/**
 * @brief Trims spaces, tabs and carriage returns.
 * @param[in] s  the text
 * @return it, trimmed
 */
std::string trim( const std::string &s )
{
    const size_t a = s.find_first_not_of( " \t\r\n" );
    if( a == std::string::npos ) return {};
    const size_t b = s.find_last_not_of( " \t\r\n" );
    return s.substr( a, b - a + 1 );
}

} // namespace

std::string dataText( const std::string &name )
{
    const std::string dir = overrideDir();
    if( !dir.empty() )
    {
        std::ifstream in( dir + "/" + name, std::ios::binary );
        if( in )
        {
            std::stringstream ss;
            ss << in.rdbuf();
            return ss.str();
        }
    }
    for( const EmbeddedFile &f : embeddedFiles() )
        if( name == f.m_name ) return std::string( reinterpret_cast<const char *>( f.m_data ), f.m_size );
    return {};
}

std::vector<std::string> dataNames( const std::string &prefix )
{
    std::vector<std::string> names;
    const std::string        dir = overrideDir();
    if( !dir.empty() )
    {
        std::error_code ec;
        for( const auto &e : std::filesystem::recursive_directory_iterator( dir, ec ) )
        {
            if( !e.is_regular_file() ) continue;
            std::string rel = std::filesystem::relative( e.path(), dir, ec ).generic_string();
            if( rel.rfind( prefix, 0 ) == 0 && e.path().extension() == ".txt" ) names.push_back( rel );
        }
    }
    for( const EmbeddedFile &f : embeddedFiles() )
        if( std::string( f.m_name ).rfind( prefix, 0 ) == 0 && std::find( names.begin(), names.end(), f.m_name ) == names.end() ) names.push_back( f.m_name );
    std::sort( names.begin(), names.end() );
    return names;
}

std::vector<std::string> dataLines( const std::string &text )
{
    std::vector<std::string> lines;
    std::stringstream        ss( text );
    std::string              line;
    while( std::getline( ss, line ) )
    {
        const size_t hash = line.find( '#' );
        if( hash != std::string::npos ) line.erase( hash );
        line = trim( line );
        if( !line.empty() ) lines.push_back( line );
    }
    return lines;
}

std::vector<std::string> splitLine( const std::string &line, char sep )
{
    std::vector<std::string> parts;
    std::string              cur;
    for( char c : line + sep )
    {
        if( c == sep || ( sep == ' ' && c == '\t' ) )
        {
            cur = trim( cur );
            if( !cur.empty() || sep != ' ' ) parts.push_back( cur );
            cur.clear();
        }
        else
        {
            cur += c;
        }
    }
    return parts;
}

int dataGeneration()
{
    return g_generation.load( std::memory_order_acquire );
}

std::string reloadData( const std::string &programDir )
{
    namespace fs = std::filesystem;
    g_found.clear();
    std::error_code ec;
    for( fs::path base : { fs::current_path( ec ), fs::path( programDir ) } )
        for( int up = 0; up < 4 && g_found.empty() && !base.empty(); ++up, base = base.parent_path() )
            if( fs::is_directory( base / "data" / "places", ec ) ) g_found = ( base / "data" ).string();
    g_generation.fetch_add( 1, std::memory_order_acq_rel );
    return overrideDir();
}

const std::vector<DataShape> &dataShapes()
{
    static DataCache<std::vector<DataShape>> cache;
    return cache.get( []
    {
        std::vector<DataShape> out;
        for( const char *folder : { "landmarks/", "creatures/" } )
            for( const std::string &file : dataNames( folder ) )
            {
                bool open = false;   // lines before a file's first shape belong to none
                for( const std::string &line : dataLines( dataText( file ) ) )
                {
                    if( line.rfind( "shape ", 0 ) == 0 )
                    {
                        out.push_back( DataShape{ trim( line.substr( 6 ) ), {}, std::string( folder ) == "creatures/" } );
                        open = true;
                    }
                    else if( open && line.rfind( "home ", 0 ) == 0 ) out.back().m_homes.push_back( line.substr( 5 ) );
                    else if( open && line.rfind( "litter ", 0 ) == 0 ) out.back().m_litter = trim( line.substr( 7 ) );
                    else if( open ) out.back().m_lines.push_back( line );
                }
            }
        return out;
    } );
}

} // namespace sunstreak
