/**
 * @file Local.cpp
 * @brief Reading data/roads.txt; where the cameras and the toll plazas stand.
 * @author reneweller
 */
#include "Local.h"

#include "Data.h"
#include "Rng.h"

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
            int plate = -1;
            if( head[0] == "plates" && head.size() == 2 )
                for( int k = 0; k < static_cast<int>( PlateE::COUNT ); ++k )
                    if( head[1] == plates[k] ) plate = k;
            const bool known = head[0] == "mph" || head[0] == "board" || head[0] == "tolls" || head[0] == "cameras" || plate >= 0;
            if( !known )
            {
                ++g_roadErrors;
                continue;
            }
            for( const std::string &country : splitLine( parts[1], ',' ) )
            {
                if( country.empty() ) continue;
                if( !countryInfo( country.c_str() ).m_name || std::string( countryInfo( country.c_str() ).m_name ) != country ) ++g_roadErrors;
                RoadCustoms &c = out[country];
                if( head[0] == "mph" ) c.m_mph = true;
                if( head[0] == "board" ) c.m_board = true;
                if( head[0] == "tolls" ) c.m_tolls = true;
                if( head[0] == "cameras" ) c.m_cameras = true;
                if( plate >= 0 ) c.m_plate = static_cast<PlateE>( plate );
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

} // namespace sunstreak
