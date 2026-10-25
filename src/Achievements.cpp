/**
 * @file Achievements.cpp
 * @brief The achievements and how far along each is.
 * @author Rene Weller
 */
#include "Achievements.h"

#include "Data.h"
#include "Festival.h"

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
    for ( int i = 0; i < biomeCount(); ++i )
    {
        const char *c = biomeDef( static_cast<BiomeE>( i ) ).m_country;
        if ( !c || !in( c ) ) continue;
        if ( std::find( all.begin(), all.end(), c ) == all.end() ) all.push_back( c );
        if ( records.m_seen.test( static_cast<size_t>( i ) ) && std::find( seen.begin(), seen.end(), c ) == seen.end() )
            seen.push_back( c );
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

}   // namespace

int policeLiveries()
{
    int n = 0;
    for ( const DataShape &d : dataShapes() ) n += d.m_name.rfind( "POLICE_CAR_", 0 ) == 0 ? 1 : 0;
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
        { "jobs10", "COURIER OF THE WORLD", "TEN COURIER JOBS: PARCELS, ORGANS, THIEVES" },
        { "legend1", "LEGENDARY", "DRIVE A LEGENDARY ROAD FROM END TO END" },
        { "legendall", "ALL THE LEGENDS", "DRIVE EVERY LEGENDARY ROAD FROM END TO END" },
        { "aqua", "SURF'S UP", "AQUAPLANE IN A DOWNPOUR" },
        { "cards50", "POSTCARD COLLECTOR", "50 POSTCARDS IN THE ALBUM" },
        { "album25", "PHOTOGRAPHER", "PHOTOGRAPH 25 LANDMARKS" },
        { "km1000", "LONG HAUL", "1000 KM ON THE ROAD" },
        { "km10000", "ROAD WARRIOR", "10000 KM ON THE ROAD" },
        { "champion", "CHAMPION", "WIN A CHAMPIONSHIP" },
        { "rallies", "RALLY LEGEND", "FINISH EVERY RALLY" },
        // The second set.
        { "countries50", "STAMP COLLECTOR", "DRIVE THROUGH 50 COUNTRIES" },
        { "countries150", "AMBASSADOR", "DRIVE THROUGH 150 COUNTRIES" },
        { "places100", "SIGHTSEER", "VISIT 100 PLACES OF THE TOUR" },
        { "places400", "CARTOGRAPHER", "VISIT 400 PLACES OF THE TOUR" },
        { "placesall", "EVERY CORNER", "VISIT EVERY PLACE OF THE TOUR" },
        { "eastern", "STEPPE AND TAIGA", "EVERY COUNTRY FROM THE BALTIC TO MONGOLIA" },
        { "islands", "ISLAND HOPPER", "EVERY COUNTRY OF THE CARIBBEAN AND THE PACIFIC" },
        { "seas10", "TWENTY THOUSAND LEAGUES", "CROSS 10 SEAS" },
        { "random25", "WANDERLUST", "25 STAGES OF THE RANDOM TOUR" },
        { "randomall", "AROUND THE WORLD BY CHANCE", "THE RANDOM TOUR TO EVERY COUNTRY" },
        { "festivals3", "PARTY PEOPLE", "BE THERE FOR 3 FESTIVALS" },
        { "festivalsall", "ALL YEAR ROUND", "BE THERE FOR EVERY FESTIVAL" },
        { "cards10", "WISH YOU WERE HERE", "10 POSTCARDS IN THE ALBUM" },
        { "cards200", "POSTMASTER", "200 POSTCARDS IN THE ALBUM" },
        { "album1", "SAY CHEESE", "PHOTOGRAPH A LANDMARK" },
        { "album100", "PHOTO ARCHIVE", "PHOTOGRAPH 100 LANDMARKS" },
        { "km100", "WARMED UP", "100 KM ON THE ROAD" },
        { "km40075", "ONCE ROUND THE EARTH", "40075 KM ON THE ROAD" },
        { "run100", "MARATHON", "100 KM IN ONE ENDLESS RUN" },
        { "score100k", "SIX FIGURES", "100000 POINTS IN ONE ENDLESS RUN" },
        { "overtakes1000", "SLIPSTREAM", "1000 OVERTAKES" },
        { "overtakes10000", "TRAFFIC SURGEON", "10000 OVERTAKES" },
        { "escape50", "MOST WANTED", "ESCAPE THE POLICE 50 TIMES" },
        { "hitch50", "BUS DRIVER", "50 HITCHHIKERS DELIVERED" },
        { "hitchworld50", "UNITED NATIONS", "HITCHHIKERS FROM 50 COUNTRIES" },
        { "makeway50", "BLUE LIGHT FRIEND", "LET 50 AMBULANCES AND FIRE ENGINES THROUGH" },
        { "hero100", "KING OF THE ROAD", "BEAT 100 LOCAL HEROES" },
        { "train10", "BULLET BEATER", "BEAT 10 FAST TRAINS" },
        { "jobs50", "SPECIAL DELIVERY", "50 COURIER JOBS" },
        { "legend10", "THE CLASSICS", "DRIVE 10 LEGENDARY ROADS FROM END TO END" },
        { "aqua25", "WATER SKIER", "AQUAPLANE 25 TIMES" },
        { "daily7", "DAILY DRIVER", "THE DAILY CHALLENGE ON 7 DAYS" },
        { "daily30", "CREATURE OF HABIT", "THE DAILY CHALLENGE ON 30 DAYS" },
        { "podium", "ON THE PODIUM", "FINISH A CHAMPIONSHIP IN THE TOP THREE" },
        { "champion3", "DYNASTY", "WIN 3 CHAMPIONSHIPS" },
        { "rally1", "RALLY DRIVER", "FINISH A RALLY" },
        { "journeysall", "EVERY JOURNEY", "FINISH EVERY JOURNEY FROM A PLACE TO ANOTHER" },
    };
    return list;
}

AchievementProgress achievementProgress( const Records &r, size_t index )
{
    const std::string key = index < achievements().size() ? achievements()[index].m_key : "";
    auto              upTo = []( int have, int need ) { return AchievementProgress{ std::min( have, need ), need }; };
    if ( key == "stamp2" ) return upTo( r.countriesSeen(), 2 );
    if ( key == "countries25" ) return upTo( r.countriesSeen(), 25 );
    if ( key == "countries100" ) return upTo( r.countriesSeen(), 100 );
    if ( key == "countriesall" ) return upTo( r.countriesSeen(), countryCount() );
    if ( key == "europe" )
        return countriesWhere( r, []( const char *c )
                               { return fleetIs( c, FleetE::EUROPE ) || fleetIs( c, FleetE::BRITISH ); } );
    if ( key == "africa" )
        return countriesWhere( r,
                               []( const char *c ) { return fleetIs( c, FleetE::AFRICA ) || landIs( c, "AFRICA" ); } );
    if ( key == "americas" ) return countriesWhere( r, []( const char *c ) { return landIs( c, "AMERICAS" ); } );
    if ( key == "asia" )
        return countriesWhere( r,
                               []( const char *c )
                               {
                                   return fleetIs( c, FleetE::SOUTH_ASIA ) || fleetIs( c, FleetE::SOUTHEAST_ASIA ) ||
                                          fleetIs( c, FleetE::EAST_ASIA );
                               } );
    if ( key == "arabia" ) return countriesWhere( r, []( const char *c ) { return fleetIs( c, FleetE::ARABIA ); } );
    if ( key == "downunder" )
        return countriesWhere( r, []( const char *c )
                               { return std::strcmp( c, "AUSTRALIA" ) == 0 || std::strcmp( c, "NEW ZEALAND" ) == 0; } );
    if ( key == "round" ) return upTo( r.sectorsSeen(), LongitudeSectors );
    if ( key == "escape1" ) return upTo( r.m_escapes, 1 );
    if ( key == "escape10" ) return upTo( r.m_escapes, 10 );
    if ( key == "escapeall" ) return upTo( static_cast<int>( r.m_escapedFrom.size() ), policeLiveries() );
    if ( key == "caught" ) return upTo( r.m_caught, 1 );
    if ( key == "hitch1" ) return upTo( r.m_delivered, 1 );
    if ( key == "hitch10" ) return upTo( r.m_delivered, 10 );
    if ( key == "hitchworld" ) return upTo( static_cast<int>( r.m_hitchFrom.size() ), 20 );
    if ( key == "makeway" ) return upTo( r.m_gaveWay, 10 );
    if ( key == "hero1" ) return upTo( r.m_heroWins, 1 );
    if ( key == "hero25" ) return upTo( r.m_heroWins, 25 );
    if ( key == "train1" ) return upTo( r.m_trainWins, 1 );
    if ( key == "jobs10" ) return upTo( r.m_jobs, 10 );
    if ( key == "legend1" ) return upTo( static_cast<int>( r.m_legendBest.size() ), 1 );
    if ( key == "legendall" )
    {
        int legends = 0;
        for ( int i = 0; i < biomeCount(); ++i ) legends += biomeDef( static_cast<BiomeE>( i ) ).m_legend != nullptr;
        return upTo( static_cast<int>( r.m_legendBest.size() ), legends );
    }
    if ( key == "aqua" ) return upTo( r.m_aquaplanes, 1 );
    if ( key == "cards50" ) return upTo( static_cast<int>( r.m_postcards.count() ), 50 );
    if ( key == "album25" ) return upTo( static_cast<int>( r.m_album.size() ), 25 );
    if ( key == "km1000" ) return upTo( static_cast<int>( r.m_totalKm ), 1000 );
    if ( key == "km10000" ) return upTo( static_cast<int>( r.m_totalKm ), 10000 );
    if ( key == "champion" ) return upTo( r.m_titles, 1 );
    if ( key == "rallies" )
    {
        int done = 0;
        for ( double best : r.m_rallyBest ) done += best > 0.0 ? 1 : 0;
        return upTo( done, static_cast<int>( rallies().size() ) );
    }
    if ( key == "countries50" ) return upTo( r.countriesSeen(), 50 );
    if ( key == "countries150" ) return upTo( r.countriesSeen(), 150 );
    if ( key == "places100" || key == "places400" || key == "placesall" )
    {
        int seen = 0;
        for ( BiomeE b : worldRoute() ) seen += r.m_seen.test( static_cast<size_t>( b ) ) ? 1 : 0;
        const int all = static_cast<int>( worldRoute().size() );
        return upTo( seen, key == "places100"   ? std::min( 100, all )
                           : key == "places400" ? std::min( 400, all )
                                                : all );
    }
    if ( key == "eastern" ) return countriesWhere( r, []( const char *c ) { return fleetIs( c, FleetE::EASTERN ); } );
    if ( key == "islands" ) return countriesWhere( r, []( const char *c ) { return fleetIs( c, FleetE::ISLANDS ); } );
    if ( key == "seas10" )
    {
        int seen = 0, all = 0;
        for ( int i = 0; i < biomeCount(); ++i )
        {
            if ( !biomeDef( static_cast<BiomeE>( i ) ).m_underwater ) continue;
            ++all;
            seen += r.m_seen.test( static_cast<size_t>( i ) ) ? 1 : 0;
        }
        return upTo( seen, std::clamp( all, 1, 10 ) );
    }
    if ( key == "random25" ) return upTo( r.m_randomStages, 25 );
    if ( key == "randomall" )
        return upTo( r.m_randomRounds > 0 ? countryCount() : static_cast<int>( r.m_randomCountries.size() ),
                     countryCount() );
    if ( key == "festivals3" ) return upTo( static_cast<int>( r.m_festivals.size() ), 3 );
    if ( key == "festivalsall" )
        return upTo( static_cast<int>( r.m_festivals.size() ), std::max( 1, static_cast<int>( festivals().size() ) ) );
    if ( key == "cards10" ) return upTo( static_cast<int>( r.m_postcards.count() ), 10 );
    if ( key == "cards200" ) return upTo( static_cast<int>( r.m_postcards.count() ), 200 );
    if ( key == "album1" ) return upTo( static_cast<int>( r.m_album.size() ), 1 );
    if ( key == "album100" ) return upTo( static_cast<int>( r.m_album.size() ), 100 );
    if ( key == "km100" ) return upTo( static_cast<int>( r.m_totalKm ), 100 );
    if ( key == "km40075" ) return upTo( static_cast<int>( r.m_totalKm ), 40075 );
    if ( key == "run100" || key == "score100k" )
    {
        // The endless top ten keeps the runs.
        double  far = 0.0;
        int64_t best = 0;
        for ( const ScoreEntry &e : r.m_top )
        {
            far = std::max( far, e.m_distance );
            best = std::max( best, e.m_score );
        }
        return key == "run100" ? upTo( static_cast<int>( far / 1000.0 ), 100 )
                               : upTo( static_cast<int>( std::min<int64_t>( best, 100000 ) ), 100000 );
    }
    if ( key == "overtakes1000" ) return upTo( r.m_overtakesAll, 1000 );
    if ( key == "overtakes10000" ) return upTo( r.m_overtakesAll, 10000 );
    if ( key == "escape50" ) return upTo( r.m_escapes, 50 );
    if ( key == "hitch50" ) return upTo( r.m_delivered, 50 );
    if ( key == "hitchworld50" ) return upTo( static_cast<int>( r.m_hitchFrom.size() ), 50 );
    if ( key == "makeway50" ) return upTo( r.m_gaveWay, 50 );
    if ( key == "hero100" ) return upTo( r.m_heroWins, 100 );
    if ( key == "train10" ) return upTo( r.m_trainWins, 10 );
    if ( key == "jobs50" ) return upTo( r.m_jobs, 50 );
    if ( key == "legend10" ) return upTo( static_cast<int>( r.m_legendBest.size() ), 10 );
    if ( key == "aqua25" ) return upTo( r.m_aquaplanes, 25 );
    if ( key == "daily7" ) return upTo( r.m_dailyDays, 7 );
    if ( key == "daily30" ) return upTo( r.m_dailyDays, 30 );
    if ( key == "podium" ) return upTo( r.m_bestPlace >= 1 && r.m_bestPlace <= 3 ? 1 : 0, 1 );
    if ( key == "champion3" ) return upTo( r.m_titles, 3 );
    if ( key == "rally1" || key == "journeysall" )
    {
        const std::vector<double> &best = key == "rally1" ? r.m_rallyBest : r.m_journeyBest;
        int                        done = 0;
        for ( double b : best ) done += b > 0.0 ? 1 : 0;
        return upTo( done, key == "rally1" ? 1 : std::max( 1, static_cast<int>( journeys().size() ) ) );
    }
    return {};
}

std::vector<size_t> earnAchievements( Records &records )
{
    std::vector<size_t> now;
    for ( size_t i = 0; i < achievements().size(); ++i )
    {
        const std::string key = achievements()[i].m_key;
        if ( std::find( records.m_achieved.begin(), records.m_achieved.end(), key ) != records.m_achieved.end() )
            continue;
        const AchievementProgress p = achievementProgress( records, i );
        if ( p.m_have < p.m_need ) continue;
        records.m_achieved.push_back( key );
        now.push_back( i );
    }
    return now;
}

}   // namespace sunstreak
