/**
 * @file Records.cpp
 * @brief Track codes, the records file, unlocks and ghosts.
 * @author reneweller
 */
#include "Records.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sunstreak
{

namespace
{

const char *Alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";   ///< Crockford's base 32

/**
 * @brief A Track Lab value in four bits.
 * @param[in] v  -1 automatic, else 0..9
 * @return 15 or v
 */
uint64_t nibble( int v )
{
    return v < 0 ? 15u : static_cast<uint64_t>( std::min( v, 9 ) );
}

/**
 * @brief Four bits back to a Track Lab value.
 * @param[in]  bits  the bits
 * @param[out] v     -1 automatic, else 0..9
 * @return false if the bits are no valid value
 */
bool fromNibble( uint64_t bits, int &v )
{
    if( bits == 15 )
    {
        v = -1;
        return true;
    }
    v = static_cast<int>( bits );
    return bits <= 9;
}

} // namespace

std::string encodeTrack( const TrackSetup &setup )
{
    const TrackParams &p = setup.m_params;
    uint64_t           v = ( setup.m_seed & 0x1FFFFFFFull );
    v = ( v << 4 ) | nibble( p.m_curvy );
    v = ( v << 4 ) | nibble( p.m_sharp );
    v = ( v << 4 ) | nibble( p.m_hilly );
    v = ( v << 4 ) | nibble( p.m_traffic );
    v = ( v << 4 ) | nibble( p.m_obstacles );
    v = ( v << 6 ) | ( p.m_biome < 0 ? 63u : static_cast<uint64_t>( p.m_biome ) );
    const uint64_t len = p.m_length > 0.0 ? static_cast<uint64_t>( std::clamp( std::lround( p.m_length / 2500.0 ), 1L, 15L ) ) : 0u;
    v = ( v << 4 ) | len;
    v = ( v << 1 ) | ( setup.m_race ? 1u : 0u );
    std::string s;
    for( int i = 11; i >= 0; --i )
    {
        s += Alphabet[( v >> ( i * 5 ) ) & 31];
        if( i == 8 || i == 4 ) s += '-';
    }
    return s;
}

bool decodeTrack( const std::string &code, TrackSetup &setup )
{
    uint64_t v = 0;
    int      n = 0;
    for( char c : code )
    {
        if( c == '-' || c == ' ' ) continue;
        c = static_cast<char>( std::toupper( static_cast<unsigned char>( c ) ) );
        if( c == 'O' ) c = '0';
        if( c == 'I' || c == 'L' ) c = '1';
        const char *pos = std::strchr( Alphabet, c );
        if( !pos || c == 0 ) return false;
        v = ( v << 5 ) | static_cast<uint64_t>( pos - Alphabet );
        if( ++n > 12 ) return false;
    }
    if( n != 12 ) return false;
    TrackSetup out;
    out.m_race = v & 1u;
    v >>= 1;
    const uint64_t len = v & 15u;
    v >>= 4;
    const uint64_t biome = v & 63u;
    v >>= 6;
    int values[5];
    for( int i = 4; i >= 0; --i )
    {
        if( !fromNibble( v & 15u, values[i] ) ) return false;
        v >>= 4;
    }
    out.m_seed = v & 0x1FFFFFFFull;
    if( out.m_seed == 0 ) out.m_seed = 1;
    if( biome != 63 && biome >= static_cast<uint64_t>( BiomeE::COUNT ) ) return false;
    out.m_params.m_curvy = values[0];
    out.m_params.m_sharp = values[1];
    out.m_params.m_hilly = values[2];
    out.m_params.m_traffic = values[3];
    out.m_params.m_obstacles = values[4];
    out.m_params.m_biome = biome == 63 ? -1 : static_cast<int>( biome );
    out.m_params.m_length = static_cast<double>( len ) * 2500.0;
    if( out.m_race && len == 0 ) return false;
    setup = out;
    return true;
}

void Records::load( const std::string &path )
{
    std::ifstream in( path );
    std::string   line;
    while( std::getline( in, line ) )
    {
        const auto eq = line.find( '=' );
        if( eq == std::string::npos ) continue;
        const std::string key = line.substr( 0, eq ), val = line.substr( eq + 1 );
        try
        {
            if( key == "total_km" ) m_totalKm = std::stod( val );
            else if( key == "titles" ) m_titles = std::stoi( val );
            else if( key == "best_place" ) m_bestPlace = std::stoi( val );
            else if( key == "seen" ) m_seenBiomes = std::stoull( val );
            else if( key == "top" && m_top.size() < 10 )
            {
                std::stringstream ss( val );
                ScoreEntry        e;
                std::string       score, dist;
                std::getline( ss, e.m_initials, '|' );
                std::getline( ss, score, '|' );
                std::getline( ss, dist, '|' );
                std::getline( ss, e.m_code, '|' );
                e.m_score = std::stoll( score );
                e.m_distance = std::stod( dist );
                m_top.push_back( e );
            }
        }
        catch( ... )
        {
            // A broken line is skipped.
        }
    }
}

void Records::save( const std::string &path ) const
{
    std::ofstream out( path );
    out << "# Sunstreak records\n";
    out << "total_km=" << m_totalKm << "\n";
    out << "titles=" << m_titles << "\n";
    out << "best_place=" << m_bestPlace << "\n";
    out << "seen=" << m_seenBiomes << "\n";
    for( const ScoreEntry &e : m_top ) out << "top=" << e.m_initials << "|" << e.m_score << "|" << e.m_distance << "|" << e.m_code << "\n";
}

int Records::rankOf( int64_t score ) const
{
    if( score <= 0 ) return -1;
    for( size_t i = 0; i < m_top.size(); ++i )
        if( score > m_top[i].m_score ) return static_cast<int>( i );
    return m_top.size() < 10 ? static_cast<int>( m_top.size() ) : -1;
}

void Records::insert( const ScoreEntry &entry )
{
    const int rank = rankOf( entry.m_score );
    if( rank < 0 ) return;
    m_top.insert( m_top.begin() + rank, entry );
    if( m_top.size() > 10 ) m_top.resize( 10 );
}

bool Records::carUnlocked( int car ) const
{
    if( car == 1 ) return m_totalKm >= 30.0;
    if( car == 2 ) return m_totalKm >= 100.0 || m_titles > 0;
    return true;
}

bool Records::paintUnlocked( int paint ) const
{
    if( paint == 2 ) return m_totalKm >= 15.0;
    if( paint == 3 ) return m_totalKm >= 50.0;
    if( paint == 4 ) return m_titles > 0;
    return true;
}

std::string Records::carRequirement( int car )
{
    return car == 1 ? "30 KM" : car == 2 ? "100 KM OR A TITLE" : "";
}

std::string Records::paintRequirement( int paint )
{
    return paint == 2 ? "15 KM" : paint == 3 ? "50 KM" : paint == 4 ? "WIN A CHAMPIONSHIP" : "";
}

int Records::seenCount() const
{
    int n = 0;
    for( int i = 0; i < static_cast<int>( BiomeE::COUNT ); ++i )
        if( m_seenBiomes & ( 1ull << i ) ) ++n;
    return n;
}

void Ghost::startRecording()
{
    m_recording.clear();
    m_nextSample = 0.0f;
}

void Ghost::record( float t, const PlayerCar &car )
{
    if( t < m_nextSample ) return;
    m_nextSample = t + 0.1f;
    m_recording.push_back( { t, car.m_s, car.m_x, car.m_yawVisual + car.m_spin } );
}

bool Ghost::load( const std::string &dir, const std::string &code )
{
    m_best.clear();
    m_metric = -1e300;
    std::string name = code;
    std::erase( name, '-' );
    FILE *f = std::fopen( ( dir + "/" + name + ".ghost" ).c_str(), "rb" );
    if( !f ) return false;
    char     magic[4] = {};
    uint32_t count = 0;
    bool     ok = std::fread( magic, 1, 4, f ) == 4 && std::equal( magic, magic + 4, "SSG1" ) &&
              std::fread( &m_car, sizeof( m_car ), 1, f ) == 1 && std::fread( &m_paint, sizeof( m_paint ), 1, f ) == 1 &&
              std::fread( &m_metric, sizeof( m_metric ), 1, f ) == 1 && std::fread( &count, sizeof( count ), 1, f ) == 1 && count < 200000;
    if( ok )
    {
        m_best.resize( count );
        for( GhostSample &g : m_best )
            ok = ok && std::fread( &g.m_t, sizeof( g.m_t ), 1, f ) == 1 && std::fread( &g.m_s, sizeof( g.m_s ), 1, f ) == 1 &&
                 std::fread( &g.m_x, sizeof( g.m_x ), 1, f ) == 1 && std::fread( &g.m_yaw, sizeof( g.m_yaw ), 1, f ) == 1;
    }
    std::fclose( f );
    if( !ok )
    {
        m_best.clear();
        m_metric = -1e300;
    }
    m_car = std::clamp( m_car, 0, CarCount - 1 );
    m_paint = std::clamp( m_paint, 0, PaintCount - 1 );
    return ok && !m_best.empty();
}

bool Ghost::saveIfBest( const std::string &dir, const std::string &code, double metric, int car, int paint )
{
    if( metric <= m_metric || m_recording.size() < 20 ) return false;
    std::error_code ec;
    std::filesystem::create_directories( dir, ec );
    std::string name = code;
    std::erase( name, '-' );
    FILE *f = std::fopen( ( dir + "/" + name + ".ghost" ).c_str(), "wb" );
    if( !f ) return false;
    const uint32_t count = static_cast<uint32_t>( m_recording.size() );
    std::fwrite( "SSG1", 1, 4, f );
    std::fwrite( &car, sizeof( car ), 1, f );
    std::fwrite( &paint, sizeof( paint ), 1, f );
    std::fwrite( &metric, sizeof( metric ), 1, f );
    std::fwrite( &count, sizeof( count ), 1, f );
    for( const GhostSample &g : m_recording )
    {
        std::fwrite( &g.m_t, sizeof( g.m_t ), 1, f );
        std::fwrite( &g.m_s, sizeof( g.m_s ), 1, f );
        std::fwrite( &g.m_x, sizeof( g.m_x ), 1, f );
        std::fwrite( &g.m_yaw, sizeof( g.m_yaw ), 1, f );
    }
    std::fclose( f );
    m_best = m_recording;
    m_metric = metric;
    m_car = car;
    m_paint = paint;
    return true;
}

bool Ghost::at( float t, double &s, float &x, float &yaw ) const
{
    if( m_best.size() < 2 || t < 0.0f || t > m_best.back().m_t ) return false;
    const auto it = std::lower_bound( m_best.begin(), m_best.end(), t, []( const GhostSample &g, float v ) { return g.m_t < v; } );
    if( it == m_best.begin() )
    {
        s = it->m_s;
        x = it->m_x;
        yaw = it->m_yaw;
        return true;
    }
    const GhostSample &b = *it;
    const GhostSample &a = *( it - 1 );
    const float        k = ( t - a.m_t ) / std::max( 1e-4f, b.m_t - a.m_t );
    s = a.m_s + ( b.m_s - a.m_s ) * k;
    x = lerpf( a.m_x, b.m_x, k );
    yaw = lerpf( a.m_yaw, b.m_yaw, k );
    return true;
}

} // namespace sunstreak
