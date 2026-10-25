/**
 * @file WorldBiomes.cpp
 * @brief The world tour: the classic biomes put on the map, the places from the data files (WorldData.cpp), the radio
 *        the countryside plays, the light of the tour clock and the names on screen.
 *
 * @author Rene Weller
 */
#include "Biome.h"
#include "Data.h"
#include "Local.h"

#include "MathUtil.h"
#include "Music.h"
#include "WorldBuilder.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

namespace sunstreak
{

void addWorldBiomes( std::vector<BiomeDef> &b )
{
    using BiomeT = BiomeE;

    // The first biomes, where they are on the map.
    using ClimateT = ClimateE;
    struct Placed
    {
        BiomeE      m_biome;
        const char *m_country;
        ClimateE    m_climate;
        const char *m_region = nullptr;
    };
    const Placed places[] = { { BiomeT::MEADOW, "GERMANY", ClimateT::TEMPERATE, "HESSE" },
                              { BiomeT::FOREST, "SWITZERLAND", ClimateT::TEMPERATE },
                              { BiomeT::DESERT, "USA", ClimateT::ARID, "ARIZONA" },
                              { BiomeT::SNOW, "CANADA", ClimateT::COLD, "NORTHWEST TERRITORIES" },
                              { BiomeT::COAST, "USA", ClimateT::TEMPERATE, "CALIFORNIA" },
                              { BiomeT::AUTUMN, "USA", ClimateT::TEMPERATE, "VERMONT" },
                              { BiomeT::SAKURA, "JAPAN", ClimateT::TEMPERATE, "KANSAI" },
                              { BiomeT::JUNGLE, "BRAZIL", ClimateT::TROPICAL, "ACRE" },
                              { BiomeT::SAVANNA, "KENYA", ClimateT::ARID },
                              { BiomeT::VOLCANO, "USA", ClimateT::TROPICAL, "HAWAII" },
                              { BiomeT::AURORA, "FINLAND", ClimateT::COLD, "LAPLAND" },
                              { BiomeT::LAVENDER, "FRANCE", ClimateT::TEMPERATE },
                              { BiomeT::TULIPS, "NETHERLANDS", ClimateT::TEMPERATE },
                              { BiomeT::HIGHLANDS, "SCOTLAND", ClimateT::TEMPERATE },
                              { BiomeT::FJORD, "NORWAY", ClimateT::TEMPERATE, "MORE OG ROMSDAL" },
                              { BiomeT::RICE, "VIETNAM", ClimateT::TROPICAL },
                              { BiomeT::SALT_FLATS, "BOLIVIA", ClimateT::ARID, "POTOSI" },
                              { BiomeT::MESA_SUNSET, "USA", ClimateT::ARID, "UTAH" },
                              { BiomeT::SUBURB, "USA", ClimateT::TEMPERATE, "NEW JERSEY" },
                              { BiomeT::HARBOR, "NETHERLANDS", ClimateT::TEMPERATE },
                              { BiomeT::ICEBERGS, "GREENLAND", ClimateT::COLD },
                              { BiomeT::BAYOU, "USA", ClimateT::TROPICAL, "LOUISIANA" },
                              { BiomeT::WHEAT, "USA", ClimateT::TEMPERATE, "KANSAS" },
                              { BiomeT::CLIFFS, "ENGLAND", ClimateT::TEMPERATE, "SOUTH EAST" },
                              { BiomeT::WINTER_VILLAGE, "AUSTRIA", ClimateT::COLD },
                              { BiomeT::RAINBOW, "IRELAND", ClimateT::TEMPERATE } };
    for ( const Placed &p : places )
    {
        b[static_cast<size_t>( p.m_biome )].m_country = p.m_country;
        b[static_cast<size_t>( p.m_biome )].m_climate = p.m_climate;
        b[static_cast<size_t>( p.m_biome )].m_region = p.m_region;
    }
    b[static_cast<size_t>( BiomeT::VOLCANO )].m_island = "HAWAII";

    addDataPlaces( b );
    applyRoadCustoms( b );
    applyRoadside( b );
    strewLitter( b );

    // AUTO radio: the countryside plays its region's station; cities and motorways keep their genre (Japan's and
    // Korea's turn to city pop).
    for ( BiomeDef &d : b )
    {
        if ( !d.m_country ) continue;
        const bool japanOrKorea =
            std::strcmp( d.m_country, "JAPAN" ) == 0 || std::strcmp( d.m_country, "SOUTH KOREA" ) == 0;
        const int region = countryInfo( d.m_country ).m_station;
        if ( d.m_lanes >= 3 )
            d.m_station = japanOrKorea ? StationCityPop : d.m_station;
        else if ( region >= 0 )
            d.m_station = region;
    }
    b[static_cast<size_t>( BiomeT::BRITTANY )].m_station = StationCeltic;
    b[static_cast<size_t>( BiomeT::RIO )].m_station = StationSamba;
}

Environment environmentAtHour( const Environment &place, float hour )
{
    // The sky of the hour: night, dawn, morning, noon, afternoon, golden hour, sunset, dusk, night.
    struct Key
    {
        float m_hour;
        SkyE  m_sky;
    };
    static const Key  keys[] = { { 0.0f, SkyE::NIGHT },   { 4.6f, SkyE::NIGHT },    { 5.6f, SkyE::DUSK },
                                 { 6.4f, SkyE::SUNSET },  { 7.4f, SkyE::GOLDEN },   { 9.0f, SkyE::MORNING },
                                 { 12.5f, SkyE::NOON },   { 15.5f, SkyE::MORNING }, { 17.4f, SkyE::GOLDEN },
                                 { 18.9f, SkyE::SUNSET }, { 19.8f, SkyE::DUSK },    { 20.9f, SkyE::NIGHT },
                                 { 24.0f, SkyE::NIGHT } };
    static const auto presets = []
    {
        std::array<Environment, 15> p{};
        for ( int i = 0; i < 15; ++i ) p[static_cast<size_t>( i )] = preset( static_cast<SkyE>( i ) );
        return p;
    }();
    const float h = std::fmod( std::fmod( hour, 24.0f ) + 24.0f, 24.0f );
    size_t      k = 0;
    while ( k + 2 < std::size( keys ) && h >= keys[k + 1].m_hour ) ++k;
    const float t = ( h - keys[k].m_hour ) / std::max( 0.01f, keys[k + 1].m_hour - keys[k].m_hour );
    Environment sky = blendEnvironment( presets[static_cast<size_t>( keys[k].m_sky )],
                                        presets[static_cast<size_t>( keys[k + 1].m_sky )], t );
    // Morning sun in the east, evening sun in the west.
    sky.m_sunAzimuth = h < 12.0f ? 60.0f - h * 3.0f : -( h - 12.0f ) * 5.0f;

    // Mostly the hour's light; the place keeps some of its colour and all of its weather and wonders.
    const float dark = smoothstepf( 18.5f, 21.0f, h ) + 1.0f - smoothstepf( 4.5f, 7.0f, h );
    Environment e = blendEnvironment( place, sky, 0.72f + 0.2f * std::min( 1.0f, dark ) );
    e.m_mountainHeight = place.m_mountainHeight;
    e.m_rain = place.m_rain;
    e.m_snow = place.m_snow;
    e.m_lightning = place.m_lightning;
    e.m_dust = place.m_dust;
    e.m_aurora = place.m_aurora;
    e.m_rainbow = dark > 0.5f ? 0.0f : place.m_rainbow;
    e.m_eclipse = place.m_eclipse;
    e.m_planetSize = place.m_planetSize;
    e.m_planetElevation = place.m_planetElevation;
    e.m_planetAzimuth = place.m_planetAzimuth;
    e.m_planetColor = place.m_planetColor;
    e.m_sunStripes = place.m_sunStripes;
    e.m_cloudAmount = std::max( sky.m_cloudAmount, place.m_cloudAmount );
    // Stars, the sun's place and the headlights belong to the hour alone.
    e.m_stars = sky.m_stars;
    e.m_headlights = sky.m_headlights;
    e.m_sunElevation = sky.m_sunElevation;
    e.m_sunSize = sky.m_sunSize;
    return e;
}

std::string placeName( BiomeE biome )
{
    const BiomeDef &d = biomeDef( biome );
    std::string     s = d.m_name;
    if ( d.m_region && s != d.m_region ) s += std::string( "  -  " ) + d.m_region;
    if ( d.m_country && s != d.m_country ) s += std::string( "  -  " ) + d.m_country;
    return s;
}

std::string borderName( BiomeE dest, BiomeE from )
{
    const BiomeDef &d = biomeDef( dest ), &f = biomeDef( from );
    const bool      same = d.m_country && f.m_country && std::strcmp( d.m_country, f.m_country ) == 0;
    if ( same && d.m_region ) return d.m_region;
    return d.m_country ? d.m_country : d.m_name ? d.m_name : "";
}

void forkNames( BiomeE left, BiomeE right, BiomeE from, std::string &a, std::string &b )
{
    a = borderName( left, from );
    b = borderName( right, from );
    if ( a != b ) return;
    const BiomeDef &l = biomeDef( left ), &r = biomeDef( right );
    if ( l.m_region && r.m_region && std::strcmp( l.m_region, r.m_region ) != 0 )
    {
        a = l.m_region;
        b = r.m_region;
        return;
    }
    a = l.m_name ? l.m_name : a;
    b = r.m_name ? r.m_name : b;
}

int countryCount()
{
    static DataCache<int> cache;
    return cache.get(
        []
        {
            std::vector<std::string> seen;
            for ( int i = 0; i < biomeCount(); ++i )
            {
                const char *c = biomeDef( static_cast<BiomeE>( i ) ).m_country;
                if ( c && std::find( seen.begin(), seen.end(), c ) == seen.end() ) seen.push_back( c );
            }
            return static_cast<int>( seen.size() );
        } );
}

}   // namespace sunstreak
