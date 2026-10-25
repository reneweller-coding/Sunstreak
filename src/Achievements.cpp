/**
 * @file Achievements.cpp
 * @brief The achievements and how far along each is.
 * @author reneweller
 */
#include "Achievements.h"

#include "Data.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>

namespace sunstreak
{

namespace
{

/**
 * @brief The countries of the tour that answer a question, and how many of them were driven through.
 * @param[in] records  the records
 * @param[in] in       whether a country counts
 * @return seen and all
 */
AchievementProgress countriesWhere( const Records &records, const std::function<bool( const char * )> &in )
{
    std::vector<std::string> all, seen;
    for( int i = 0; i < biomeCount(); ++i )
    {
        const char *c = biomeDef( static_cast<BiomeE>( i ) ).m_country;
        if( !c || !in( c ) ) continue;
        if( std::find( all.begin(), all.end(), c ) == all.end() ) all.push_back( c );
        if( records.m_seen.test( static_cast<size_t>( i ) ) && std::find( seen.begin(), seen.end(), c ) == seen.end() ) seen.push_back( c );
    }
    return { static_cast<int>( seen.size() ), std::max( 1, static_cast<int>( all.size() ) ) };
}

/**
 * @brief Whether a country drives with a fleet.
 * @param[in] country  its name
 * @param[in] fleet    the fleet
 * @return true if so
 */
bool fleetIs( const char *country, FleetE fleet )
{
    return countryInfo( country ).m_fleet == fleet;
}

/**
 * @brief Whether a country lies on a landmass.
 * @param[in] country  its name
 * @param[in] land     the landmass
 * @return true if so
 */
bool landIs( const char *country, const char *land )
{
    const char *l = countryInfo( country ).m_land;
    return l && std::strcmp( l, land ) == 0;
}

} // namespace

int policeLiveries()
{
    int n = 0;
    for( const DataShape &d : dataShapes() ) n += d.m_name.rfind( "POLICE_CAR_", 0 ) == 0 ? 1 : 0;
    return std::max( 1, n );
}

const std::vector<Achievement> &achievements()
{
    static const std::vector<Achievement> list = {
        { "stamp2", "FIRST STAMP", "CROSS INTO A SECOND COUNTRY" },
        { "countries25", "GLOBETROTTER", "DRIVE THROUGH 25 COUNTRIES" },
        { "countries100", "WORLD CITIZEN", "DRIVE THROUGH 100 COUNTRIES" },
        { "countriesall", "EVERY FLAG", "EVERY COUNTRY OF THE TOUR" },
        { "europe", "ACROSS EUROPE", "EVERY COUNTRY OF EUROPE" },
        { "africa", "ACROSS AFRICA", "EVERY COUNTRY OF AFRICA" },
        { "americas", "THE AMERICAS", "FROM ALASKA TO TIERRA DEL FUEGO" },
        { "asia", "THE SILK ROAD AND BEYOND", "EVERY COUNTRY OF SOUTH, SOUTHEAST AND EAST ASIA" },
        { "arabia", "ARABIAN NIGHTS", "EVERY COUNTRY FROM MOROCCO TO OMAN" },
        { "downunder", "DOWN UNDER", "AUSTRALIA AND NEW ZEALAND" },
        { "round", "ROUND THE WORLD", "CROSS EVERY LINE OF LONGITUDE" },
        { "escape1", "GETAWAY", "ESCAPE THE POLICE" },
        { "escape10", "ESCAPE ARTIST", "ESCAPE THE POLICE TEN TIMES" },
        { "escapeall", "EVERY POLICE", "ESCAPE THE POLICE OF EVERY LIVERY" },
        { "caught", "CAUGHT RED-HANDED", "GET CAUGHT BY THE POLICE" },
        { "hitch1", "GOOD SAMARITAN", "TAKE A HITCHHIKER WHERE THEY WANTED TO GO" },
        { "hitch10", "TAXI DRIVER", "TEN HITCHHIKERS DELIVERED" },
        { "hitchworld", "WORLD TAXI", "HITCHHIKERS FROM 20 COUNTRIES" },
        { "makeway", "MAKE WAY", "LET TEN AMBULANCES AND FIRE ENGINES THROUGH" },
        { "hero1", "LOCAL KNOWLEDGE", "BEAT A LOCAL HERO TO THE CHECKPOINT" },
        { "hero25", "LEGEND OF THE ROADS", "BEAT 25 LOCAL HEROES" },
        { "train1", "FASTER THAN THE TRAIN", "BEAT A FAST TRAIN TO THE END OF ITS VIADUCT" },
        { "aqua", "SURF'S UP", "AQUAPLANE IN A DOWNPOUR" },
        { "cards50", "POSTCARD COLLECTOR", "50 POSTCARDS IN THE ALBUM" },
        { "album25", "PHOTOGRAPHER", "PHOTOGRAPH 25 LANDMARKS" },
        { "km1000", "LONG HAUL", "1000 KM ON THE ROAD" },
        { "km10000", "ROAD WARRIOR", "10000 KM ON THE ROAD" },
        { "champion", "CHAMPION", "WIN A CHAMPIONSHIP" },
        { "rallies", "RALLY LEGEND", "FINISH EVERY RALLY" },
    };
    return list;
}

AchievementProgress achievementProgress( const Records &r, size_t index )
{
    const std::string key = index < achievements().size() ? achievements()[index].m_key : "";
    auto              upTo = []( int have, int need ) { return AchievementProgress{ std::min( have, need ), need }; };
    if( key == "stamp2" ) return upTo( r.countriesSeen(), 2 );
    if( key == "countries25" ) return upTo( r.countriesSeen(), 25 );
    if( key == "countries100" ) return upTo( r.countriesSeen(), 100 );
    if( key == "countriesall" ) return upTo( r.countriesSeen(), countryCount() );
    if( key == "europe" ) return countriesWhere( r, []( const char *c ) { return fleetIs( c, FleetE::EUROPE ) || fleetIs( c, FleetE::BRITISH ); } );
    if( key == "africa" ) return countriesWhere( r, []( const char *c ) { return fleetIs( c, FleetE::AFRICA ) || landIs( c, "AFRICA" ); } );
    if( key == "americas" ) return countriesWhere( r, []( const char *c ) { return landIs( c, "AMERICAS" ); } );
    if( key == "asia" )
        return countriesWhere( r, []( const char *c )
                               { return fleetIs( c, FleetE::SOUTH_ASIA ) || fleetIs( c, FleetE::SOUTHEAST_ASIA ) || fleetIs( c, FleetE::EAST_ASIA ); } );
    if( key == "arabia" ) return countriesWhere( r, []( const char *c ) { return fleetIs( c, FleetE::ARABIA ); } );
    if( key == "downunder" )
        return countriesWhere( r, []( const char *c ) { return std::strcmp( c, "AUSTRALIA" ) == 0 || std::strcmp( c, "NEW ZEALAND" ) == 0; } );
    if( key == "round" ) return upTo( r.sectorsSeen(), LongitudeSectors );
    if( key == "escape1" ) return upTo( r.m_escapes, 1 );
    if( key == "escape10" ) return upTo( r.m_escapes, 10 );
    if( key == "escapeall" ) return upTo( static_cast<int>( r.m_escapedFrom.size() ), policeLiveries() );
    if( key == "caught" ) return upTo( r.m_caught, 1 );
    if( key == "hitch1" ) return upTo( r.m_delivered, 1 );
    if( key == "hitch10" ) return upTo( r.m_delivered, 10 );
    if( key == "hitchworld" ) return upTo( static_cast<int>( r.m_hitchFrom.size() ), 20 );
    if( key == "makeway" ) return upTo( r.m_gaveWay, 10 );
    if( key == "hero1" ) return upTo( r.m_heroWins, 1 );
    if( key == "hero25" ) return upTo( r.m_heroWins, 25 );
    if( key == "train1" ) return upTo( r.m_trainWins, 1 );
    if( key == "aqua" ) return upTo( r.m_aquaplanes, 1 );
    if( key == "cards50" ) return upTo( static_cast<int>( r.m_postcards.count() ), 50 );
    if( key == "album25" ) return upTo( static_cast<int>( r.m_album.size() ), 25 );
    if( key == "km1000" ) return upTo( static_cast<int>( r.m_totalKm ), 1000 );
    if( key == "km10000" ) return upTo( static_cast<int>( r.m_totalKm ), 10000 );
    if( key == "champion" ) return upTo( r.m_titles, 1 );
    if( key == "rallies" )
    {
        int done = 0;
        for( double best : r.m_rallyBest ) done += best > 0.0 ? 1 : 0;
        return upTo( done, static_cast<int>( rallies().size() ) );
    }
    return {};
}

std::vector<size_t> earnAchievements( Records &records )
{
    std::vector<size_t> now;
    for( size_t i = 0; i < achievements().size(); ++i )
    {
        const std::string key = achievements()[i].m_key;
        if( std::find( records.m_achieved.begin(), records.m_achieved.end(), key ) != records.m_achieved.end() ) continue;
        const AchievementProgress p = achievementProgress( records, i );
        if( p.m_have < p.m_need ) continue;
        records.m_achieved.push_back( key );
        now.push_back( i );
    }
    return now;
}

} // namespace sunstreak
