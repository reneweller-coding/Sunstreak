/**
 * @file Rallies.cpp
 * @brief The world tour's rallies: famous long-distance routes as a chain of places, driven in one go against the clock.
 * @author reneweller
 */
#include "Biome.h"
#include "Data.h"

namespace sunstreak
{

const std::vector<Rally> &rallies()
{
    using enum BiomeE;
    static const std::vector<Rally> list = {
        { "PARIS - DAKAR", "FRANCE TO SENEGAL ACROSS THE SAHARA",
          { PARIS, LOIRE_VALLEY, LA_MANCHA, ANDALUSIA, CHEFCHAOUEN, HIGH_ATLAS, SAHARA_DUNES, ADRAR, PINK_LAKE } },
        { "PANAMERICANA", "ALASKA TO PATAGONIA",
          { DALTON, DENALI, YUKON, SEA_TO_SKY, BIG_SUR, BAJA, MEXICO_CITY, COPAN, PANAMA_CANAL, VOLCANO_AVENUE, NAZCA, ATACAMA, RUTA_40,
            TORRES_DEL_PAINE } },
        { "ROUTE 66", "CHICAGO TO SANTA MONICA", { CHICAGO, WHEAT, WEST_TEXAS, ROUTE_66, GRAND_CANYON, LAS_VEGAS, JOSHUA_TREE, LA_FREEWAY } },
        { "SILK ROAD", "ISTANBUL TO BEIJING", { ISTANBUL, CAPPADOCIA, CAUCASUS, YAZD, SAMARKAND, PAMIR_HIGHWAY, ZHANGYE, GOBI, GREAT_WALL, BEIJING } },
        { "CAPE TO CAIRO", "THE LENGTH OF AFRICA",
          { CAPE_PENINSULA, DRAKENSBERG, KRUGER, ZAMBEZI, LAKE_MALAWI, SERENGETI, ETHIOPIAN_HIGHLANDS, MEROE, LUXOR, GIZA } },
        { "ALPINE PASSES", "THE HIGHEST ROADS OF EUROPE", { BAVARIA, GROSSGLOCKNER, STELVIO, FURKA_PASS, CHAMONIX, ALPE_D_HUEZ, COL_DE_TURINI, MONACO } },
        { "OUTBACK", "SYDNEY TO THE KIMBERLEY", { SYDNEY, BLUE_MOUNTAINS, GREAT_OCEAN_ROAD, NULLARBOR, RED_CENTRE, KIMBERLEY } },
        { "HIMALAYAN RAID", "THE ROOF OF THE WORLD", { MANALI, SPITI, LADAKH, KARAKORAM, PAMIR_HIGHWAY, TIBET, HIMALAYA, KATHMANDU_VALLEY, BHUTAN } },
        { "NORDKAPP", "FROM THE BALTIC TO THE NORTH CAPE", { CURONIAN_SPIT, GAUJA, LAHEMAA, FINNISH_LAKELAND, AURORA, NORTH_CAPE, LOFOTEN, TROLLSTIGEN,
                                                             ATLANTIC_ROAD } },
        { "MEKONG EXPRESS", "BANGKOK TO HA LONG BAY", { BANGKOK, ANGKOR, MEKONG, NORTHERN_THAILAND, SAPA, HA_GIANG, HA_LONG_BAY } },
    };
    return list;
}

const std::vector<Journey> &journeys()
{
    static DataCache<std::vector<Journey>> cache;
    return cache.get( []
    {
        std::vector<Journey> out;
        for( const std::string &line : dataLines( dataText( "journeys.txt" ) ) )
        {
            // journey FROM TO | WHAT
            if( line.rfind( "journey ", 0 ) != 0 ) continue;
            const std::vector<std::string> parts = splitLine( line.substr( 8 ), '|' );
            const std::vector<std::string> ends = splitLine( parts.empty() ? std::string() : parts[0], ' ' );
            if( ends.size() != 2 ) continue;
            const int from = biomeId( ends[0] ), to = biomeId( ends[1] );
            if( from < 0 || to < 0 ) continue;
            const BiomeDef &a = biomeDef( static_cast<BiomeE>( from ) ), &b = biomeDef( static_cast<BiomeE>( to ) );
            out.push_back( { std::string( a.m_name ? a.m_name : "" ) + " - " + ( b.m_name ? b.m_name : "" ), parts.size() > 1 ? parts[1] : std::string(),
                             static_cast<BiomeE>( from ), static_cast<BiomeE>( to ) } );
        }
        return out;
    } );
}

int routeStages( BiomeE from, BiomeE to )
{
    const int a = routeIndex( from ), b = routeIndex( to ), n = static_cast<int>( worldRoute().size() );
    if( a < 0 || b < 0 || n == 0 ) return -1;
    return ( b - a + n - 1 ) % n + 1;
}

} // namespace sunstreak
