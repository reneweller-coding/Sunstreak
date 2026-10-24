/**
 * @file Records.cpp
 * @brief Track codes, the records file, unlocks and ghosts.
 * @author reneweller
 */
#include "Records.h"

#include "Rng.h"

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

/** @brief A code's bits: up to 128, written at the bottom and read back from the bottom. */
struct Bits
{
    uint64_t m_hi = 0;   ///< the upper word
    uint64_t m_lo = 0;   ///< the lower word

    /**
     * @brief Shifts n bits in at the bottom.
     * @param[in] v  the value
     * @param[in] n  1..63 bits
     */
    void push( uint64_t v, int n )
    {
        m_hi = ( m_hi << n ) | ( m_lo >> ( 64 - n ) );
        m_lo = ( m_lo << n ) | ( v & ( ( 1ull << n ) - 1 ) );
    }

    /**
     * @brief Takes n bits off the bottom.
     * @param[in] n  1..63 bits
     * @return them
     */
    uint64_t pop( int n )
    {
        const uint64_t v = m_lo & ( ( 1ull << n ) - 1 );
        m_lo = ( m_lo >> n ) | ( m_hi << ( 64 - n ) );
        m_hi >>= n;
        return v;
    }

    /**
     * @brief The five bits of one symbol.
     * @param[in] i  the symbol, 0 the lowest
     * @return 0..31
     */
    uint64_t symbol( int i ) const
    {
        const int s = 5 * i;
        if( s >= 64 ) return ( m_hi >> ( s - 64 ) ) & 31;
        return ( ( m_lo >> s ) | ( s ? m_hi << ( 64 - s ) : 0 ) ) & 31;
    }

    /**
     * @brief A check value of everything so far.
     * @return eight bits
     */
    uint64_t check() const { return hash64( m_hi * 0x9E3779B97F4A7C15ull ^ m_lo ^ 0xC0DEull ) & 255; }
};

constexpr int      CodeVersion = 1;     ///< the layout of the fifteen-symbol codes
constexpr uint64_t CodeWorld = 510;     ///< scenery field: the world tour
constexpr uint64_t CodeMixed = 511;     ///< scenery field: all biomes mixed

/**
 * @brief Reads the old twelve-symbol codes (before the world tour): seed, five values, a biome in six bits.
 * @param[in]  v      the 60 bits
 * @param[out] setup  the track
 * @return false if the bits are no valid code
 */
bool decodeLegacy( uint64_t v, TrackSetup &setup )
{
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
    out.m_params.m_biome = biome == 63 ? SceneryMixed : static_cast<int>( biome );
    out.m_params.m_length = static_cast<double>( len ) * 2500.0;
    if( out.m_race && len == 0 ) return false;
    setup = out;
    return true;
}

} // namespace

std::string encodeTrack( const TrackSetup &setup )
{
    const TrackParams &p = setup.m_params;
    Bits               b;
    b.push( CodeVersion, 4 );
    b.push( setup.m_seed & 0x1FFFFFFFull, 29 );
    for( int v : { p.m_curvy, p.m_sharp, p.m_hilly, p.m_traffic, p.m_obstacles } ) b.push( nibble( v ), 4 );
    b.push( p.m_biome >= 0 ? static_cast<uint64_t>( p.m_biome ) : p.m_biome == SceneryWorld ? CodeWorld : CodeMixed, 9 );
    b.push( p.m_length > 0.0 ? static_cast<uint64_t>( std::clamp( std::lround( p.m_length / 2500.0 ), 1L, 15L ) ) : 0u, 4 );
    b.push( setup.m_race ? 1u : 0u, 1 );
    b.push( b.check(), 8 );
    std::string s;
    for( int i = 14; i >= 0; --i )
    {
        s += Alphabet[b.symbol( i )];
        if( i == 10 || i == 5 ) s += '-';
    }
    return s;
}

bool decodeTrack( const std::string &code, TrackSetup &setup )
{
    Bits     b;
    uint64_t legacy = 0;
    int      n = 0;
    for( char c : code )
    {
        if( c == '-' || c == ' ' ) continue;
        c = static_cast<char>( std::toupper( static_cast<unsigned char>( c ) ) );
        if( c == 'O' ) c = '0';
        if( c == 'I' || c == 'L' ) c = '1';
        const char *pos = std::strchr( Alphabet, c );
        if( !pos || c == 0 ) return false;
        b.push( static_cast<uint64_t>( pos - Alphabet ), 5 );
        legacy = ( legacy << 5 ) | static_cast<uint64_t>( pos - Alphabet );
        if( ++n > 15 ) return false;
    }
    if( n == 12 ) return decodeLegacy( legacy, setup );
    if( n != 15 ) return false;
    const uint64_t check = b.pop( 8 );
    if( check != b.check() ) return false;
    TrackSetup out;
    out.m_race = b.pop( 1 );
    const uint64_t len = b.pop( 4 );
    const uint64_t scenery = b.pop( 9 );
    int            values[5];
    for( int i = 4; i >= 0; --i )
        if( !fromNibble( b.pop( 4 ), values[i] ) ) return false;
    out.m_seed = b.pop( 29 );
    if( b.pop( 4 ) != CodeVersion ) return false;
    if( out.m_seed == 0 ) out.m_seed = 1;
    if( scenery < CodeWorld && scenery >= static_cast<uint64_t>( BiomeE::COUNT ) ) return false;
    out.m_params.m_curvy = values[0];
    out.m_params.m_sharp = values[1];
    out.m_params.m_hilly = values[2];
    out.m_params.m_traffic = values[3];
    out.m_params.m_obstacles = values[4];
    out.m_params.m_biome = scenery == CodeWorld ? SceneryWorld : scenery == CodeMixed ? SceneryMixed : static_cast<int>( scenery );
    out.m_params.m_length = static_cast<double>( len ) * 2500.0;
    if( out.m_race && len == 0 ) return false;
    setup = out;
    return true;
}

namespace
{

/**
 * @brief Reads a set of biome bits written as 128 hex digits (bit 511 first).
 * @param[in]  val   the digits
 * @param[out] bits  the set (bits are added)
 */
void readBits( const std::string &val, std::bitset<512> &bits )
{
    const size_t n = std::min<size_t>( val.size(), 128 );
    for( size_t i = 0; i < n; ++i )
    {
        const int d = std::isdigit( static_cast<unsigned char>( val[i] ) ) ? val[i] - '0' : std::toupper( val[i] ) - 'A' + 10;
        for( int k = 0; k < 4; ++k )
            if( d & ( 8 >> k ) ) bits.set( n * 4 - 1 - i * 4 - static_cast<size_t>( k ) );
    }
}

/**
 * @brief Writes a set of biome bits as 128 hex digits.
 * @param[in,out] out   the stream
 * @param[in]     bits  the set
 */
void writeBits( std::ostream &out, const std::bitset<512> &bits )
{
    for( int i = 0; i < 128; ++i )
    {
        int d = 0;
        for( int k = 0; k < 4; ++k ) d = d * 2 + ( bits.test( static_cast<size_t>( 511 - i * 4 - k ) ) ? 1 : 0 );
        out << "0123456789ABCDEF"[d];
    }
    out << "\n";
}

/** @brief What the paints from SUN YELLOW on cost, in career points (0: unlocked otherwise). */
constexpr int64_t PaintPoints[PaintCount] = { 0, 0, 0, 0, 0, 25000, 50000, 75000, 100000, 150000, 200000, 300000, 400000, 0 };
constexpr int64_t RimPoints[RimCount] = { 0, 10000, 30000, 60000, 120000, 180000, 250000 };      ///< rims, in career points
constexpr int64_t DecalPoints[DecalCount] = { 0, 20000, 40000, 80000, 120000, 200000, 0 };      ///< decals, in career points
constexpr int     GlobeCountries = 50;   ///< countries seen for the GLOBE decal

/**
 * @brief A number of points as text.
 * @param[in] points  the number
 * @return e.g. "25 000 POINTS"
 */
std::string groupedPoints( int64_t points )
{
    std::string digits = std::to_string( points ), out;
    for( size_t i = 0; i < digits.size(); ++i )
    {
        if( i > 0 && ( digits.size() - i ) % 3 == 0 ) out += ' ';
        out += digits[i];
    }
    return out + " POINTS";
}

} // namespace

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
            else if( key == "seen" ) m_seen = std::bitset<512>( std::stoull( val ) );
            else if( key == "seen2" ) readBits( val, m_seen );
            else if( key == "postcards" ) readBits( val, m_postcards );
            else if( key == "points" ) m_points = std::max<int64_t>( 0, std::stoll( val ) );
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
    out << "points=" << m_points << "\n";
    out << "seen2=";
    writeBits( out, m_seen );
    out << "postcards=";
    writeBits( out, m_postcards );
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
    if( paint == PaintCount - 1 ) return circumnavigated();
    return m_points >= PaintPoints[std::clamp( paint, 0, PaintCount - 1 )];
}

bool Records::rimsUnlocked( int rims ) const
{
    return m_points >= RimPoints[std::clamp( rims, 0, RimCount - 1 )];
}

bool Records::decalUnlocked( int decal ) const
{
    if( decal == DecalCount - 1 ) return countriesSeen() >= GlobeCountries;
    return m_points >= DecalPoints[std::clamp( decal, 0, DecalCount - 1 )];
}

std::string Records::carRequirement( int car )
{
    return car == 1 ? "30 KM" : car == 2 ? "100 KM OR A TITLE" : "";
}

std::string Records::paintRequirement( int paint )
{
    if( paint == 2 ) return "15 KM";
    if( paint == 3 ) return "50 KM";
    if( paint == 4 ) return "WIN A CHAMPIONSHIP";
    if( paint == PaintCount - 1 ) return "DRIVE ROUND THE WORLD";
    const int64_t p = PaintPoints[std::clamp( paint, 0, PaintCount - 1 )];
    return p > 0 ? pointsText( p ) : "";
}

std::string Records::rimsRequirement( int rims )
{
    const int64_t p = RimPoints[std::clamp( rims, 0, RimCount - 1 )];
    return p > 0 ? pointsText( p ) : "";
}

std::string Records::decalRequirement( int decal )
{
    if( decal == DecalCount - 1 ) return std::to_string( GlobeCountries ) + " COUNTRIES";
    const int64_t p = DecalPoints[std::clamp( decal, 0, DecalCount - 1 )];
    return p > 0 ? pointsText( p ) : "";
}

std::string Records::pointsText( int64_t points )
{
    return groupedPoints( points );
}

int Records::sectorsSeen() const
{
    std::bitset<LongitudeSectors> sectors;
    for( int i = 0; i < static_cast<int>( BiomeE::COUNT ); ++i )
    {
        GeoPoint where;
        if( m_seen.test( static_cast<size_t>( i ) ) && placeLocation( static_cast<BiomeE>( i ), where ) )
            sectors.set( static_cast<size_t>( longitudeSector( where.m_lon ) ) );
    }
    return static_cast<int>( sectors.count() );
}

int Records::seenCount() const
{
    return static_cast<int>( m_seen.count() );
}

int Records::countriesSeen() const
{
    std::vector<std::string> countries;
    for( int i = 0; i < static_cast<int>( BiomeE::COUNT ); ++i )
    {
        const char *c = biomeDef( static_cast<BiomeE>( i ) ).m_country;
        if( c && m_seen.test( static_cast<size_t>( i ) ) && std::find( countries.begin(), countries.end(), c ) == countries.end() )
            countries.push_back( c );
    }
    return static_cast<int>( countries.size() );
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
