/**
 * @file Local.cpp
 * @brief Reading data/roads.txt; where the cameras and the toll plazas stand.
 * @author reneweller
 */
#include "Local.h"

#include "Data.h"
#include "Rng.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

namespace sunstreak
{

namespace
{

int g_roadErrors = 0;   ///< lines of data/roads.txt that were wrong

/** @brief The customs by country. */
using CustomsMap = std::map<std::string, RoadCustoms>;

/**
 * @brief The customs of every country the file names, read again after a reload.
 * @return them
 */
const CustomsMap &customs()
{
    static DataCache<CustomsMap> cache;
    return cache.get( []
    {
        g_roadErrors = 0;
        CustomsMap out;
        for( const std::string &line : dataLines( dataText( "roads.txt" ) ) )
        {
            const std::vector<std::string> parts = splitLine( line, '|' );
            const std::vector<std::string> head = parts.empty() ? std::vector<std::string>() : splitLine( parts[0], ' ' );
            if( parts.size() != 2 || head.empty() )
            {
                ++g_roadErrors;
                continue;
            }
            static const char *const plates[] = { "WHITE", "EU", "UK", "US", "YELLOW", "JP", "BLACK" };
            static const char *const styles[] = { "SCOOTERS", "CHAOS", "TRACTORS", "ROADTRAINS" };
            int style = -1;
            if( head[0] == "style" && head.size() == 2 )
                for( int k = 0; k < 4; ++k )
                    if( head[1] == styles[k] ) style = k;
            int plate = -1;
            if( head[0] == "plates" && head.size() == 2 )
                for( int k = 0; k < static_cast<int>( PlateE::COUNT ); ++k )
                    if( head[1] == plates[k] ) plate = k;
            const std::string &key = head[0];
            const float        value = head.size() > 1 ? std::strtof( head[1].c_str(), nullptr ) : 0.0f;
            const bool flag = ( key == "mph" || key == "board" || key == "tolls" || key == "cameras" || key == "shoulders" || key == "studs" ) && head.size() == 1;
            const bool number = ( key == "quality" || key == "width" || key == "gravel" || key == "concrete" || key == "traffic" ) && value > 0.0f &&
                                ( head.size() == 2 || ( key == "concrete" && head.size() == 3 ) );
            const bool kerbs = key == "kerbs" && head.size() == 3 && head[1].size() == 6 && head[2].size() == 6;
            if( !flag && !number && !kerbs && plate < 0 && style < 0 )
            {
                ++g_roadErrors;
                continue;
            }
            auto colour = []( const std::string &hex )
            {
                const unsigned long v = std::strtoul( hex.c_str(), nullptr, 16 );
                return Color{ static_cast<unsigned char>( v >> 16 ), static_cast<unsigned char>( v >> 8 ), static_cast<unsigned char>( v ), 255 };
            };
            for( const std::string &country : splitLine( parts[1], ',' ) )
            {
                if( country.empty() ) continue;
                if( !countryInfo( country.c_str() ).m_name || std::string( countryInfo( country.c_str() ).m_name ) != country ) ++g_roadErrors;
                RoadCustoms &c = out[country];
                if( key == "mph" ) c.m_mph = true;
                if( key == "board" ) c.m_board = true;
                if( key == "tolls" ) c.m_tolls = true;
                if( key == "cameras" ) c.m_cameras = true;
                if( plate >= 0 ) c.m_plate = static_cast<PlateE>( plate );
                if( key == "quality" ) c.m_quality = value;
                if( key == "width" ) c.m_width = value;
                if( key == "shoulders" ) c.m_shoulders = true;
                if( key == "studs" ) c.m_studs = true;
                if( key == "gravel" ) c.m_gravel = value / 100.0f;
                if( key == "traffic" ) c.m_traffic = value;
                if( style >= 0 ) c.m_styles = static_cast<uint8_t>( c.m_styles | ( 1u << style ) );
                if( key == "concrete" )
                {
                    c.m_concrete = value / 100.0f;
                    c.m_concreteLanes = head.size() == 3 ? std::atoi( head[2].c_str() ) : 0;
                }
                if( kerbs )
                {
                    c.m_kerbs = true;
                    c.m_kerbA = colour( head[1] );
                    c.m_kerbB = colour( head[2] );
                }
            }
        }
        return out;
    } );
}

/**
 * @brief Whether the road at a distance is free for a camera or a plaza: no tunnel, bridge, fork or crossing near.
 * @param[in] track  the road
 * @param[in] s      the distance
 * @return true if free
 */
bool clearAt( const Track &track, double s )
{
    if( track.inForkZone( s, 150.0 ) || track.crossingAt( s ).m_type != CrossingE::NONE ) return false;
    for( double d = -60.0; d <= 60.0; d += SegmentLength * 4.0 )
        if( track.segment( static_cast<long>( ( s + d ) / SegmentLength ) ).m_flags & ( SEG_TUNNEL | SEG_BRIDGE | SEG_JUMP | SEG_ROADWORKS ) ) return false;
    return true;
}

constexpr double GravelFrom = 1900.0;   ///< a stage keeps its asphalt this far (past its camera and its toll plaza)
constexpr double GravelTo = 600.0;      ///< and this far before its end
constexpr double GravelBlock = 600.0;   ///< a stretch of gravel, or of asphalt
constexpr double GravelRamp = 15.0;     ///< where the one turns into the other

/**
 * @brief A number for a name, the same on every machine.
 * @param[in] text  the name
 * @return its FNV-1a hash
 */
uint64_t nameHash( const char *text )
{
    uint64_t h = 14695981039346656037ull;
    for( const char *c = text; c && *c; ++c ) h = ( h ^ static_cast<unsigned char>( *c ) ) * 1099511628211ull;
    return h;
}

} // namespace

const RoadCustoms &roadCustoms( const char *country )
{
    static const RoadCustoms plain;
    if( !country ) return plain;
    const CustomsMap &all = customs();
    const auto        it = all.find( country );
    return it != all.end() ? it->second : plain;
}

int roadErrors()
{
    customs();
    return g_roadErrors;
}

int speedLimit( const Segment &seg, bool mph )
{
    const bool town = seg.m_lanes >= 3;
    return mph ? ( town ? 30 : 60 ) : ( town ? 50 : 100 );
}

bool stageCamera( const Track &track, int stage, double &s, int &limit, bool &mph )
{
    const BiomeDef    &place = biomeDef( track.stageBiome( stage ) );
    const RoadCustoms &c = roadCustoms( place.m_country );
    if( !c.m_cameras || place.m_underwater || hash64( track.seed() * 977 + static_cast<uint64_t>( stage ) ) % 3 == 0 ) return false;
    // The first clear spot from CameraAt on, the limit sign clear before it.
    for( int k = 0; k < 6; ++k )
    {
        s = static_cast<double>( stage ) * StageLength + CameraAt + 100.0 * k;
        if( !clearAt( track, s ) || !clearAt( track, s - LimitAt ) ) continue;
        mph = c.m_mph;
        limit = speedLimit( track.segment( static_cast<long>( s / SegmentLength ) ), mph );
        return true;
    }
    return false;
}

bool stageToll( const Track &track, int stage, double &s )
{
    const BiomeDef    &place = biomeDef( track.stageBiome( stage ) );
    const RoadCustoms &c = roadCustoms( place.m_country );
    if( !c.m_tolls || place.m_underwater || hash64( track.seed() * 991 + static_cast<uint64_t>( stage ) ) % 3 == 0 ) return false;
    // The first clear spot from TollAt on.
    for( int k = 0; k < 6; ++k )
    {
        s = static_cast<double>( stage ) * StageLength + TollAt + 100.0 * k;
        if( track.segment( static_cast<long>( s / SegmentLength ) ).m_lanes >= 2 && clearAt( track, s ) ) return true;
    }
    return false;
}

double policeBase( const Track &track, int stage )
{
    const BiomeDef &place = biomeDef( track.stageBiome( stage ) );
    if( stage < 0 || !place.m_country || place.m_underwater || place.m_surface.m_lineGlow > 0.0f ) return -1.0;
    const uint64_t h = hash64( track.seed() * 1013 + static_cast<uint64_t>( stage ) * 7 + 5 );
    if( h % 10 != 0 ) return -1.0;   // a surprise: one stage in ten
    return static_cast<double>( stage ) * StageLength + PoliceAt + static_cast<double>( ( h >> 8 ) % 7000 );
}

bool stagePolice( const Track &track, int stage, double &s, float &side )
{
    const double base = policeBase( track, stage );
    if( base < 0.0 ) return false;
    side = countryInfo( biomeDef( track.stageBiome( stage ) ).m_country ).m_left ? -1.0f : 1.0f;
    for( int k = 0; k < 6; ++k )
    {
        s = base + 100.0 * k;
        if( clearAt( track, s ) && clearAt( track, s + 60.0 ) && gravelAt( track, s ) <= 0.0f ) return true;
    }
    return false;
}

void applyRoadCustoms( std::vector<BiomeDef> &table )
{
    for( BiomeDef &d : table )
    {
        const bool neon = d.m_surface.m_lineGlow > 0.0f;
        if( !d.m_country || d.m_underwater || neon )
        {
            d.m_gravel = std::max( 0.0f, d.m_gravel );
            continue;
        }
        const RoadCustoms &c = roadCustoms( d.m_country );
        const bool         town = d.m_lampSpacing > 0.0f || d.m_cobbles;
        Surface           &sf = d.m_surface;
        d.m_laneWidth = c.m_width;
        if( c.m_traffic > 0.0f ) d.m_traffic *= clampf( std::sqrt( c.m_traffic / 130.0f ), 0.75f, 1.5f );
        d.m_wear = d.m_unpaved ? 0.0f : clampf( ( 5.0f - c.m_quality ) / 2.8f, 0.0f, 1.0f );
        if( d.m_gravel < 0.0f ) d.m_gravel = town || d.m_unpaved || d.m_lanes >= 3 ? 0.0f : c.m_gravel;
        d.m_studs = c.m_studs && !d.m_unpaved;
        // Concrete for some of the country's places, the same ones every time.
        const int lanes = std::max( 2, c.m_concreteLanes );
        if( !d.m_unpaved && !d.m_cobbles && d.m_lanes >= lanes && static_cast<float>( nameHash( d.m_name ) % 1000 ) < c.m_concrete * 1000.0f )
        {
            d.m_concrete = true;
            sf.m_road = Color{ 172, 170, 162, 255 };
            sf.m_roadAlt = Color{ 166, 164, 156, 255 };
        }
        if( d.m_kerbs || d.m_unpaved ) continue;
        // The edges: the painted or plain stone kerbs of a town, a paved shoulder, or the verge right after the line.
        if( town )
        {
            sf.m_rumbleA = c.m_kerbs ? c.m_kerbA : Color{ 184, 180, 172, 255 };
            sf.m_rumbleB = c.m_kerbs ? c.m_kerbB : Color{ 200, 196, 188, 255 };
        }
        else if( c.m_shoulders && d.m_wear < 0.6f )
        {
            const Color paved = d.m_concrete ? Color{ 92, 94, 100, 255 } : scaleColor( sf.m_road, 1.05f );
            sf.m_rumbleA = sf.m_rumbleB = paved;
        }
        else
        {
            sf.m_rumbleA = sf.m_shoulder;
            sf.m_rumbleB = scaleColor( sf.m_shoulder, 0.95f );
        }
    }
}

float gravelAt( const Track &track, double s )
{
    if( s < 0.0 ) return 0.0f;
    const int       stage = static_cast<int>( s / StageLength );
    const BiomeDef &d = biomeDef( track.stageBiome( stage ) );
    const double    local = s - static_cast<double>( stage ) * StageLength;
    if( d.m_gravel <= 0.0f || local < GravelFrom || local > StageLength - GravelTo ) return 0.0f;
    if( track.segment( static_cast<long>( s / SegmentLength ) ).m_flags & ( SEG_TUNNEL | SEG_BRIDGE | SEG_FORK ) ) return 0.0f;
    if( track.inForkZone( s, 60.0 ) || track.crossingAt( s ).m_type != CrossingE::NONE ) return 0.0f;
    // Stretches of gravel and of asphalt by turns, as many of gravel as the place has of it.
    const long blocks = static_cast<long>( ( StageLength - GravelTo - GravelFrom ) / GravelBlock );
    auto       on = [&]( long b )
    {
        if( b < 0 || b >= blocks ) return 0.0f;
        return hashFloat( track.seed() * 7907 + static_cast<uint64_t>( stage ) * 131, static_cast<uint64_t>( b ) ) < d.m_gravel ? 1.0f : 0.0f;
    };
    const double u = local - GravelFrom;
    const long   b = static_cast<long>( u / GravelBlock );
    const double t = u - static_cast<double>( b ) * GravelBlock;
    if( t < GravelRamp ) return lerpf( on( b - 1 ), on( b ), 0.5f + 0.5f * static_cast<float>( t / GravelRamp ) );
    if( t > GravelBlock - GravelRamp ) return lerpf( on( b ), on( b + 1 ), 0.5f * static_cast<float>( ( t - GravelBlock + GravelRamp ) / GravelRamp ) );
    return on( b );
}

bool potholeAt( const Track &track, long j, float &x, float &r )
{
    const Segment  &seg = track.segment( j );
    const BiomeDef &d = biomeDef( static_cast<BiomeE>( seg.m_blend < 0.5f ? seg.m_biome : seg.m_biomeTo ) );
    constexpr uint16_t clear = SEG_TUNNEL | SEG_BRIDGE | SEG_FORK | SEG_CHECKPOINT | SEG_START | SEG_FINISH | SEG_PUDDLE | SEG_ROADWORKS | SEG_JUMP;
    if( d.m_wear < 0.25f || ( seg.m_flags & clear ) || seg.m_halfWidth < 1.5f ) return false;
    const uint64_t seed = track.seed();
    if( hashFloat( seed * 6271 + 3, static_cast<uint64_t>( j ) ) >= 0.07f * ( d.m_wear - 0.2f ) ) return false;
    if( gravelAt( track, static_cast<double>( j ) * SegmentLength ) > 0.0f ) return false;
    x = ( hashFloat( seed * 17 + 5, static_cast<uint64_t>( j ) ) * 2.0f - 1.0f ) * ( seg.m_halfWidth - 0.9f );
    r = 0.35f + 0.4f * hashFloat( seed * 31 + 7, static_cast<uint64_t>( j ) );
    return true;
}

} // namespace sunstreak
