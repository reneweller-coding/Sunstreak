/**
 * @file WorldBiomes.cpp
 * @brief The world tour: two hundred places round the globe, written as data, and the route that links them.
 *
 * Every place is a sky preset changed where the place needs it, its ground, the shape of its land, the character of
 * its roads and the things beside them. Props with key colours take the place's paint (a cottage is whitewashed in
 * Ireland and pink in Marrakech); night places light their windows. The route runs east from Ireland through Europe,
 * Africa, Arabia, Asia, Oceania and the Americas, and home over Russia and Scandinavia.
 *
 * @author reneweller
 */
#include "Biome.h"

#include "MathUtil.h"
#include "Music.h"
#include "WorldBuilder.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

namespace sunstreak
{

namespace
{

/**
 * @brief The station a country's countryside plays in AUTO.
 * @param[in] country  the country
 * @return the station, -1 for the genre stations (much of Europe, New Zealand)
 */
int regionStation( const char *country )
{
    static const std::pair<const char *, int> map[] = {
        { "IRELAND", StationCeltic },
        { "SCOTLAND", StationCeltic },
        { "WALES", StationCeltic },
        { "NORTHERN IRELAND", StationCeltic },
        { "NORWAY", StationNordic },
        { "SWEDEN", StationNordic },
        { "FINLAND", StationNordic },
        { "DENMARK", StationNordic },
        { "ICELAND", StationNordic },
        { "GREENLAND", StationNordic },
        { "ESTONIA", StationNordic },
        { "LATVIA", StationNordic },
        { "LITHUANIA", StationNordic },
        { "FAROE ISLANDS", StationNordic },
        { "CROATIA", StationBalkan },
        { "MONTENEGRO", StationBalkan },
        { "BULGARIA", StationBalkan },
        { "ROMANIA", StationBalkan },
        { "SERBIA", StationBalkan },
        { "BOSNIA", StationBalkan },
        { "ALBANIA", StationBalkan },
        { "NORTH MACEDONIA", StationBalkan },
        { "KOSOVO", StationBalkan },
        { "MOLDOVA", StationBalkan },
        { "HUNGARY", StationBalkan },
        { "GREECE", StationBalkan },
        { "SLOVENIA", StationBalkan },
        { "CYPRUS", StationBalkan },
        { "MOROCCO", StationDesert },
        { "ALGERIA", StationDesert },
        { "TUNISIA", StationDesert },
        { "LIBYA", StationDesert },
        { "EGYPT", StationDesert },
        { "JORDAN", StationDesert },
        { "LEBANON", StationDesert },
        { "SYRIA", StationDesert },
        { "IRAQ", StationDesert },
        { "SAUDI ARABIA", StationDesert },
        { "UAE", StationDesert },
        { "OMAN", StationDesert },
        { "YEMEN", StationDesert },
        { "QATAR", StationDesert },
        { "KUWAIT", StationDesert },
        { "BAHRAIN", StationDesert },
        { "IRAN", StationDesert },
        { "TURKEY", StationDesert },
        { "ISRAEL", StationDesert },
        { "PALESTINE", StationDesert },
        { "SUDAN", StationDesert },
        { "MAURITANIA", StationDesert },
        { "WESTERN SAHARA", StationDesert },
        { "INDIA", StationBollywood },
        { "NEPAL", StationBollywood },
        { "BHUTAN", StationBollywood },
        { "SRI LANKA", StationBollywood },
        { "PAKISTAN", StationBollywood },
        { "BANGLADESH", StationBollywood },
        { "MALDIVES", StationBollywood },
        { "KAZAKHSTAN", StationSteppe },
        { "KYRGYZSTAN", StationSteppe },
        { "UZBEKISTAN", StationSteppe },
        { "TAJIKISTAN", StationSteppe },
        { "TURKMENISTAN", StationSteppe },
        { "MONGOLIA", StationSteppe },
        { "RUSSIA", StationSteppe },
        { "AFGHANISTAN", StationSteppe },
        { "AZERBAIJAN", StationSteppe },
        { "GEORGIA", StationSteppe },
        { "ARMENIA", StationSteppe },
        { "BELARUS", StationSteppe },
        { "UKRAINE", StationSteppe },
        { "INDONESIA", StationGamelan },
        { "MALAYSIA", StationGamelan },
        { "PHILIPPINES", StationGamelan },
        { "THAILAND", StationGamelan },
        { "CAMBODIA", StationGamelan },
        { "LAOS", StationGamelan },
        { "MYANMAR", StationGamelan },
        { "VIETNAM", StationGamelan },
        { "BRUNEI", StationGamelan },
        { "EAST TIMOR", StationGamelan },
        { "SINGAPORE", StationGamelan },
        { "CHINA", StationKoto },
        { "TAIWAN", StationKoto },
        { "JAPAN", StationKoto },
        { "NORTH KOREA", StationKoto },
        { "SOUTH KOREA", StationCityPop },
        { "MALI", StationAfro },
        { "GHANA", StationAfro },
        { "DR CONGO", StationAfro },
        { "RWANDA", StationAfro },
        { "ETHIOPIA", StationAfro },
        { "KENYA", StationAfro },
        { "TANZANIA", StationAfro },
        { "ZAMBIA", StationAfro },
        { "BOTSWANA", StationAfro },
        { "NAMIBIA", StationAfro },
        { "SOUTH AFRICA", StationAfro },
        { "MADAGASCAR", StationAfro },
        { "SEYCHELLES", StationAfro },
        { "NIGERIA", StationAfro },
        { "SENEGAL", StationAfro },
        { "GAMBIA", StationAfro },
        { "GUINEA", StationAfro },
        { "GUINEA-BISSAU", StationAfro },
        { "SIERRA LEONE", StationAfro },
        { "LIBERIA", StationAfro },
        { "IVORY COAST", StationAfro },
        { "BURKINA FASO", StationAfro },
        { "NIGER", StationAfro },
        { "CHAD", StationAfro },
        { "TOGO", StationAfro },
        { "BENIN", StationAfro },
        { "CAMEROON", StationAfro },
        { "CENTRAL AFRICAN REPUBLIC", StationAfro },
        { "GABON", StationAfro },
        { "CONGO", StationAfro },
        { "EQUATORIAL GUINEA", StationAfro },
        { "ANGOLA", StationAfro },
        { "UGANDA", StationAfro },
        { "BURUNDI", StationAfro },
        { "SOUTH SUDAN", StationAfro },
        { "ERITREA", StationAfro },
        { "DJIBOUTI", StationAfro },
        { "SOMALIA", StationAfro },
        { "MOZAMBIQUE", StationAfro },
        { "MALAWI", StationAfro },
        { "ZIMBABWE", StationAfro },
        { "LESOTHO", StationAfro },
        { "ESWATINI", StationAfro },
        { "MAURITIUS", StationAfro },
        { "COMOROS", StationAfro },
        { "CAPE VERDE", StationAfro },
        { "SAO TOME AND PRINCIPE", StationAfro },
        { "BRAZIL", StationSamba },
        { "ARGENTINA", StationTango },
        { "URUGUAY", StationTango },
        { "PARAGUAY", StationTango },
        { "PERU", StationAndes },
        { "BOLIVIA", StationAndes },
        { "ECUADOR", StationAndes },
        { "CHILE", StationAndes },
        { "COLOMBIA", StationAndes },
        { "CUBA", StationCaribbean },
        { "JAMAICA", StationCaribbean },
        { "BAHAMAS", StationCaribbean },
        { "HAITI", StationCaribbean },
        { "DOMINICAN REPUBLIC", StationCaribbean },
        { "TRINIDAD AND TOBAGO", StationCaribbean },
        { "BARBADOS", StationCaribbean },
        { "PUERTO RICO", StationCaribbean },
        { "VENEZUELA", StationCaribbean },
        { "GUYANA", StationCaribbean },
        { "SURINAME", StationCaribbean },
        { "SAINT LUCIA", StationCaribbean },
        { "GRENADA", StationCaribbean },
        { "DOMINICA", StationCaribbean },
        { "ANTIGUA AND BARBUDA", StationCaribbean },
        { "SAINT KITTS AND NEVIS", StationCaribbean },
        { "SAINT VINCENT", StationCaribbean },
        { "FIJI", StationCaribbean },
        { "SAMOA", StationCaribbean },
        { "TONGA", StationCaribbean },
        { "VANUATU", StationCaribbean },
        { "PAPUA NEW GUINEA", StationCaribbean },
        { "SOLOMON ISLANDS", StationCaribbean },
        { "FRENCH POLYNESIA", StationCaribbean },
        { "KIRIBATI", StationCaribbean },
        { "TUVALU", StationCaribbean },
        { "NAURU", StationCaribbean },
        { "PALAU", StationCaribbean },
        { "MARSHALL ISLANDS", StationCaribbean },
        { "MICRONESIA", StationCaribbean },
        { "HAWAII", StationCaribbean },
        { "MEXICO", StationMariachi },
        { "GUATEMALA", StationMariachi },
        { "BELIZE", StationMariachi },
        { "HONDURAS", StationMariachi },
        { "EL SALVADOR", StationMariachi },
        { "NICARAGUA", StationMariachi },
        { "COSTA RICA", StationMariachi },
        { "PANAMA", StationMariachi },
        { "AUSTRALIA", StationOutback },
        { "USA", StationRoute66 },
        { "CANADA", StationRoute66 },
        { "ALASKA", StationRoute66 },
    };
    for( const auto &[name, station] : map )
        if( std::strcmp( name, country ) == 0 ) return station;
    return -1;
}

} // namespace

void addWorldBiomes( BiomeDef *b )
{
    using enum PropKindE;
    using BE = BiomeE;
    using BR = BridgeE;
    using S = SkyE;

    // The first biomes, where they are on the map.
    const std::pair<BiomeE, const char *> places[] = {
        { BE::MEADOW, "GERMANY" },       { BE::FOREST, "SWITZERLAND" },      { BE::DESERT, "USA" },         { BE::SNOW, "CANADA" },
        { BE::COAST, "USA" },            { BE::AUTUMN, "USA" },              { BE::SAKURA, "JAPAN" },       { BE::JUNGLE, "BRAZIL" },
        { BE::SAVANNA, "KENYA" },        { BE::VOLCANO, "HAWAII" },          { BE::AURORA, "FINLAND" },     { BE::LAVENDER, "FRANCE" },
        { BE::TULIPS, "NETHERLANDS" },   { BE::HIGHLANDS, "SCOTLAND" },      { BE::FJORD, "NORWAY" },       { BE::RICE, "VIETNAM" },
        { BE::SALT_FLATS, "BOLIVIA" },   { BE::MESA_SUNSET, "USA" },         { BE::SUBURB, "USA" },         { BE::HARBOR, "NETHERLANDS" },
        { BE::ICEBERGS, "GREENLAND" },   { BE::BAYOU, "USA" },               { BE::WHEAT, "USA" },          { BE::CLIFFS, "ENGLAND" },
        { BE::WINTER_VILLAGE, "AUSTRIA" }, { BE::RAINBOW, "IRELAND" } };
    for( const auto &[biome, country] : places ) b[static_cast<size_t>( biome )].m_country = country;

    // ---- British Isles ---------------------------------------------------------------------------------------------
    W( b, BE::CONNEMARA, "CONNEMARA", "IRELAND", S::OVERCAST ).rain( 0.15f ).land( 0x6F9A48, 0x648C42, 0x7C8A5C ).hills( 18, 5, 5, 0.3f )
        .water( 0x4A6878 ).drive( 2, 0.35f, 0.93f, 6 ).bends( 0.65f, 0.5f, 0.45f ).posts( 0 ).bridges( 0.08f, BR::ARCH, 0x8C8A80 )
        .p( STONE_WALL, 0.55f, 3, 3, 1, 1 ).p( COTTAGE, 0.02f, 15, 90, 0.9f, 1.1f ).c( 0xF2F0E6, 0x5A5650 ).p( SHEEP, 0.07f, 6, 70 )
        .p( SHRUB, 0.25f, 2, 50, 0.6f, 1.1f ).c( 0x8A7A50 ).p( ROCK, 0.1f, 3, 60 ).p( MENHIR, 0.004f, 20, 120 );
    W( b, BE::WILD_ATLANTIC, "WILD ATLANTIC WAY", "IRELAND", S::MORNING ).clouds( 0.55f ).sea( 1, 0, -16 ).water( 0x2E5C7A )
        .land( 0x74A852, 0x689A4A, 0x6C9048 ).hills( 20, -4, 8 ).drive( 2, 0.4f, 0.97f, 6 ).bends( 0.6f, 0.45f, 0.5f )
        .p( STONE_WALL, 0.4f, 3, 3, 1, 1 ).p( SHEEP, 0.08f, 6, 60 ).p( COTTAGE, 0.012f, 12, 80 ).c( 0xF4F2EA, 0x3C3C44 )
        .p( FLOWER_BUSH, 0.08f, 2, 20, 0.7f, 1.0f ).c( 0xC02850 ).p( LIGHTHOUSE, 0.004f, 30, 120 );
    W( b, BE::ISLE_OF_SKYE, "ISLE OF SKYE", "SCOTLAND", S::OVERCAST ).clouds( 0.7f ).ranges( 0x3E4650, 0x6A7480, 9 ).sea( 0, 1, -6 )
        .land( 0x7E9A58, 0x728E50, 0x6A7E58 ).hills( 30, 10, 0 ).drive( 2, 0.25f, 0.92f, 3 ).bends( 0.7f, 0.55f, 0.6f ).bridges( 0.1f, BR::ARCH, 0x7A7870 )
        .p( PEAK, 0.004f, 250, 500, 0.9f, 1.2f ).c( 0x4A4A50, 0x6A6A70 ).p( SHEEP, 0.06f, 6, 60 ).p( SHRUB, 0.2f, 2, 50, 0.6f, 1.0f ).c( 0x7A6A48 )
        .p( ROCK, 0.12f, 3, 60 ).p( COTTAGE, 0.008f, 15, 90 ).c( 0xF4F2EA, 0x3A3A40 ).p( STONE_WALL, 0.15f, 3, 3, 1, 1 );
    W( b, BE::SNOWDONIA, "SNOWDONIA", "WALES", S::RAIN ).rain( 0.4f ).land( 0x5E8A44, 0x56803E, 0x6A7A5A ).ranges( 0x4C5660, 0x7C8894, 8 )
        .hills( 40, 14, 10 ).drive( 2, 0.4f, 0.9f, 3 ).bends( 0.7f, 0.6f, 0.75f ).tunnels( 0.1f ).bridges( 0.1f, BR::ARCH, 0x7A7A78 )
        .p( STONE_WALL, 0.5f, 3, 3, 1, 1 ).p( SHEEP, 0.08f, 6, 60 ).p( COTTAGE, 0.012f, 12, 80 ).c( 0x8C8A86, 0x3E3E46 ).p( SPRUCE, 0.1f, 30, 150 )
        .p( ROCK, 0.15f, 3, 60 ).p( BOULDER, 0.03f, 10, 80 );
    W( b, BE::LAKE_DISTRICT, "LAKE DISTRICT", "ENGLAND", S::MORNING ).clouds( 0.5f ).land( 0x6A9E4C, 0x5E9244, 0x76905A ).hills( 30, 8, 8, 0.35f )
        .water( 0x50708A ).ranges( 0x56686E, 0x8A9AA4, 6 ).drive( 2, 0.5f, 0.97f, 6 ).bends( 0.65f, 0.5f, 0.6f ).bridges( 0.12f, BR::ARCH, 0x8A867A )
        .p( STONE_WALL, 0.45f, 3, 3, 1, 1 ).p( SHEEP, 0.07f, 6, 70 ).p( COTTAGE, 0.015f, 10, 80 ).c( 0xE8E6DE, 0x4A4A52 )
        .p( BROADLEAF, 0.1f, 4, 60 ).c( 0x4E8A3C ).p( SPRUCE, 0.08f, 40, 200 );
    W( b, BE::COTSWOLDS, "COTSWOLDS", "ENGLAND", S::MORNING ).land( 0x7EB050, 0x72A248, 0x88A860 ).fields( 0.35f, 0xD8C060, 0x8AB050, 0xC8A858 )
        .hills( 14, 4, 4 ).drive( 2, 0.6f, 1.0f, 6 ).bends( 0.55f, 0.45f, 0.45f ).posts( 0 )
        .p( HEDGE, 0.5f, 3, 3, 1, 1 ).p( COTTAGE, 0.04f, 8, 70 ).c( 0xD8B878, 0x6A6458 ).p( CHURCH, 0.004f, 30, 150 ).c( 0xD8B878, 0x6A6458 )
        .p( OAK, 0.08f, 6, 80 ).p( SHEEP, 0.06f, 10, 80 ).p( HAYBALE, 0.02f, 12, 60 );
    W( b, BE::LONDON, "LONDON", "ENGLAND", S::OVERCAST ).land( 0x6E8A58, 0x667F52, 0x707870 ).drive( 3, 1.3f, 0.95f, 1 ).bends( 0.45f, 0.4f, 0.2f )
        .lamps( 35 ).posts( 0 ).bridges( 0.12f, BR::ARCH, 0xA8A498 )
        .p( ROW_HOUSES, 0.12f, 6, 30 ).c( 0xB86848, 0x4A4A52 ).p( ROW_HOUSES, 0.06f, 6, 40 ).c( 0xE8E2D6, 0x4A4A52 )
        .p( GLASS_TOWER, 0.04f, 40, 200 ).c( 0x7E98AE ).p( PANEL_BLOCK, 0.02f, 30, 120 ).c( 0xC8BCA8 ).p( BROADLEAF, 0.08f, 2, 20 ).c( 0x5C8A44 )
        .p( CHURCH, 0.006f, 20, 120 ).c( 0xD8D4CC, 0x5A6070 )
        .sea( 0, 1, -5 ).water( 0x4A5A58 ).landmark( Lm::BIG_BEN, 50, 90 ).landmark( Lm::TOWER_BRIDGE, 60, 100 ).floating();

    // ---- France and the Low Countries ------------------------------------------------------------------------------
    W( b, BE::BRITTANY, "BRITTANY", "FRANCE", S::MORNING ).clouds( 0.5f ).sea( 0, 1, -6 ).water( 0x2E5E7A ).land( 0x6E9E4A, 0x64924A, 0x7A8E5A )
        .drive( 2, 0.5f, 0.97f, 6 ).bridges( 0.1f, BR::BEAM )
        .p( COTTAGE, 0.025f, 10, 80 ).c( 0xB8B4AC, 0x3E4048 ).p( MENHIR, 0.03f, 8, 80 ).p( FLOWER_BUSH, 0.08f, 2, 25 ).c( 0x6A88D8 )
        .p( HEDGE, 0.3f, 3, 3, 1, 1 ).p( LIGHTHOUSE, 0.004f, 30, 120 ).p( ROCK, 0.08f, 3, 40 );
    W( b, BE::NORMANDY, "NORMANDY", "FRANCE", S::OVERCAST ).clouds( 0.6f ).land( 0x6AA24A, 0x5E9442, 0x78965A ).drive( 2, 0.6f, 0.97f, 6 )
        .bridges( 0.08f, BR::ARCH, 0xB0A890 )
        .p( HALF_TIMBER, 0.03f, 10, 70 ).c( 0xEDE6D2, 0x8A5A44 ).p( BROADLEAF, 0.15f, 4, 60, 0.6f, 0.9f ).c( 0x5E9440 ).p( COW, 0.06f, 8, 80 )
        .p( HEDGE, 0.45f, 3, 3, 1, 1 ).p( CHURCH, 0.004f, 30, 150 ).c( 0xD8D2C4, 0x50565E ).p( HAYBALE, 0.015f, 12, 60 );
    W( b, BE::PARIS, "PARIS", "FRANCE", S::MORNING ).land( 0x7A8A6A, 0x728262, 0x8A8A7A ).road( 0x6E7078 ).drive( 3, 1.2f, 0.95f, 0 )
        .bends( 0.4f, 0.35f, 0.15f ).lamps( 30 ).posts( 0 ).bridges( 0.1f, BR::ARCH, 0xC8C0AC )
        .p( ROW_HOUSES, 0.16f, 5, 30 ).c( 0xE6DCC6, 0x5A6470 ).p( BROADLEAF, 0.2f, 1.5f, 6, 0.9f, 1.1f ).c( 0x5A8E40 )
        .p( LATTICE_TOWER, 0.0025f, 200, 450, 2.2f, 2.5f ).c( 0x6A5A48, 0x6A5A48 ).p( CHURCH, 0.004f, 30, 150 ).c( 0xF0ECE4, 0x7A8490 )
        .landmark( Lm::TRIUMPHAL_ARCH, 40, 70 );
    W( b, BE::CHAMPAGNE, "CHAMPAGNE", "FRANCE", S::MORNING ).land( 0x8CB25A, 0x80A452, 0xB8B07A ).verge( 0xE0DCCC ).hills( 16, 4, 4 )
        .drive( 2, 0.5f, 1.0f, 6 ).bends( 0.45f, 0.35f, 0.45f )
        .p( VINE_ROW, 0.85f, 5, 12, 1, 1 ).p( VINE_ROW, 0.85f, 14, 21, 1, 1 ).p( VINE_ROW, 0.7f, 23, 30, 1, 1 ).p( VINE_ROW, 0.5f, 32, 50, 1, 1 )
        .p( CHURCH, 0.006f, 60, 200 ).c( 0xE6DED0, 0x5A6068 ).p( COTTAGE, 0.01f, 20, 80 ).c( 0xE6DED0, 0x8A5A48 );
    W( b, BE::LOIRE_VALLEY, "LOIRE VALLEY", "FRANCE", S::GOLDEN ).sea( 0, 1, -5 ).water( 0x4A7890 ).land( 0x7CAE52, 0x70A04A, 0x86A462 )
        .drive( 2, 0.5f, 1.0f, 6 ).bends( 0.45f, 0.35f, 0.3f ).bridges( 0.15f, BR::ARCH, 0xE0D8C4 )
        .p( POPLAR, 0.2f, 5, 40 ).p( CASTLE, 0.004f, 60, 250 ).c( 0xEAE4D4, 0x4E5664 ).p( VINE_ROW, 0.3f, 6, 30, 1, 1 )
        .p( COTTAGE, 0.012f, 15, 80 ).c( 0xEAE2D0, 0x4E5664 );
    W( b, BE::FLANDERS, "FLANDERS", "BELGIUM", S::OVERCAST ).land( 0x7AAE4E, 0x70A048, 0x82A05A ).fields( 0.6f, 0xC8302C, 0x8AB048, 0xD8C25A )
        .hills( 4, 2, 2 ).drive( 2, 0.7f, 0.97f, 1 ).bends( 0.35f, 0.3f, 0.15f ).bridges( 0.06f, BR::BEAM )
        .p( ROW_HOUSES, 0.01f, 20, 90 ).c( 0xA85A40, 0x5A4A44 ).v( 1 ).p( COTTAGE, 0.015f, 15, 90 ).c( 0xB06048, 0x6A3A30 )
        .p( CHURCH, 0.004f, 40, 200 ).c( 0xB06048, 0x4A4E58 ).p( POPLAR, 0.12f, 6, 60 ).p( POWER_PYLON, 0.006f, 30, 120 )
        .p( WINDMILL, 0.006f, 60, 250 ).p( COW, 0.03f, 10, 80 );

    // ---- Germany and the Alps --------------------------------------------------------------------------------------
    W( b, BE::RHINE_VALLEY, "RHINE VALLEY", "GERMANY", S::MORNING ).sea( 0, 1, -6 ).water( 0x4A7090 ).land( 0x6E9E4A, 0x64904A, 0x7A8E60 )
        .hills( 30, 30, 0 ).drive( 2, 0.7f, 1.0f, 1 ).bends( 0.6f, 0.45f, 0.3f ).tunnels( 0.12f ).bridges( 0.1f, BR::TRUSS, 0x3A4A5A )
        .p( VINE_ROW, 0.6f, 6, 30, 1, 1 ).p( CASTLE, 0.006f, 60, 200 ).c( 0xA89C88, 0x4A4E5A ).p( HALF_TIMBER, 0.025f, 8, 50 ).c( 0xF2ECDC, 0x5A5A64 )
        .p( CHURCH, 0.004f, 20, 120 ).c( 0xF0ECE0, 0x4A4E58 ).p( BOAT, 0.015f, 40, 120, 1.4f, 1.8f ).c( 0x2A4A7A, 0xF0F0F0 ).floating();
    W( b, BE::BLACK_FOREST, "BLACK FOREST", "GERMANY", S::MIST ).land( 0x4E8040, 0x46763A, 0x3E6A40 ).hills( 36, 12, 10 )
        .drive( 2, 0.5f, 0.97f, 1 ).bends( 0.75f, 0.6f, 0.75f ).tunnels( 0.15f ).bridges( 0.08f, BR::COVERED, 0x7A4A30 )
        .p( SPRUCE, 0.55f, 3, 60 ).p( SPRUCE, 0.3f, 60, 220, 1.0f, 1.6f ).p( CHALET, 0.02f, 10, 80 ).c( 0xEDE6D8, 0x5A4A40 )
        .p( COW, 0.02f, 10, 60 ).p( WAYSIDE_CROSS, 0.006f, 3, 10 ).v( 0 );
    W( b, BE::AUTOBAHN, "AUTOBAHN", "GERMANY", S::MORNING ).land( 0x6A9A48, 0x5E8E42, 0x5A8048 ).drive( 4, 1.5f, 1.0f, 1 )
        .bends( 0.3f, 0.2f, 0.3f ).gantries( 600 ).posts( 50 ).bridges( 0.1f, BR::BEAM )
        .p( PINE, 0.35f, 6, 80 ).p( SPRUCE, 0.2f, 6, 120 ).p( WINDMILL, 0.012f, 60, 300 ).p( POWER_PYLON, 0.008f, 40, 150 );
    W( b, BE::BAVARIA, "BAVARIA", "GERMANY", S::MORNING ).land( 0x76B04C, 0x6AA244, 0x7EA25A ).ranges( 0x7A8CA0, 0xC8D4E4, 9 )
        .drive( 2, 0.6f, 1.0f, 6 ).bends( 0.5f, 0.4f, 0.45f ).bridges( 0.06f, BR::COVERED, 0x7A4A30 )
        .p( ONION_CHURCH, 0.005f, 40, 200 ).c( 0xF4F0E6, 0x3A5A44 ).v( 1 ).p( CHALET, 0.02f, 10, 80 ).c( 0xF2ECDE, 0x7A4A36 ).p( COW, 0.06f, 8, 80 )
        .p( HAYBALE, 0.02f, 10, 60 ).p( WAYSIDE_CROSS, 0.008f, 3, 10 ).v( 0 ).p( BROADLEAF, 0.06f, 4, 80 ).c( 0x5E9A40 );
    W( b, BE::SWISS_ALPS, "SWISS ALPS", "SWITZERLAND", S::ALPINE ).land( 0x62A844, 0x58983E, 0x6E9A58 ).ranges( 0x6E8296, 0xE6ECF4, 12 )
        .hills( 50, 16, -8 ).drive( 2, 0.5f, 1.0f, 3 ).bends( 0.75f, 0.65f, 0.85f ).tunnels( 0.25f ).bridges( 0.2f, BR::VIADUCT, 0xB8B6B0 )
        .p( CHALET, 0.03f, 10, 90 ).c( 0x8A5A36, 0x5A4030 ).p( COW, 0.07f, 8, 90 ).p( SPRUCE, 0.2f, 20, 150 )
        .p( PEAK, 0.003f, 300, 600, 1.2f, 1.6f ).c( 0x8A8C92, 0xF2F4F8 ).p( WAYSIDE_CROSS, 0.005f, 3, 10 ).v( 0 )
        .p( FLOWER_BUSH, 0.05f, 2, 30, 0.4f, 0.6f ).c( 0xE8D84A );
    W( b, BE::LAVAUX, "LAVAUX", "SWITZERLAND", S::GOLDEN ).sea( 1, 0, -14 ).water( 0x3E6A8A ).land( 0x7AAA4E, 0x70A048, 0x88A462 )
        .ranges( 0x6A7C96, 0xD0D8E8, 8 ).hills( 24, 0, 18 ).drive( 2, 0.5f, 1.0f, 6 ).bends( 0.6f, 0.5f, 0.6f ).bridges( 0.08f, BR::ARCH, 0xC8C0A8 )
        .p( VINE_ROW, 0.7f, 4, 40, 1, 1 ).p( STONE_WALL, 0.4f, 3, 3, 1, 1 ).p( COTTAGE, 0.02f, 10, 80 ).c( 0xF0E8D8, 0x9A5040 )
        .p( CHURCH, 0.004f, 30, 120 ).c( 0xF0E8D8, 0x6A5A50 ).p( SAILBOAT, 0.01f, 40, 200 ).floating();
    W( b, BE::SALZKAMMERGUT, "SALZKAMMERGUT", "AUSTRIA", S::MORNING ).clouds( 0.5f ).sea( 1, 0, -6 ).water( 0x3E6A7A )
        .land( 0x5E9E46, 0x56923E, 0x6A8C5A ).ranges( 0x5A6878, 0xC8D2E0, 10 ).hills( 40, 0, 24 ).drive( 2, 0.5f, 0.98f, 6 )
        .bends( 0.6f, 0.5f, 0.5f ).tunnels( 0.2f ).bridges( 0.06f, BR::ARCH, 0xB8B0A0 )
        .p( CHALET, 0.03f, 8, 80 ).c( 0xF0EADC, 0x5A4636 ).p( CHURCH, 0.005f, 20, 120 ).c( 0xF2EEE4, 0x3A3E48 ).p( SPRUCE, 0.3f, 10, 120 )
        .p( BOAT, 0.01f, 40, 150 ).c( 0x8A5A3A, 0xE8E8E8 ).floating().p( WAYSIDE_CROSS, 0.005f, 3, 10 );
    W( b, BE::CHAMONIX, "CHAMONIX", "FRANCE", S::ALPINE ).land( 0x5E9A44, 0x56903E, 0x6E8A60 ).ranges( 0x606E80, 0xEEF2F8, 14 )
        .hills( 50, 20, 10 ).drive( 2, 0.5f, 0.98f, 3 ).bends( 0.7f, 0.6f, 0.8f ).tunnels( 0.2f ).bridges( 0.15f, BR::VIADUCT, 0xB0AEA8 )
        .p( PEAK, 0.004f, 250, 500, 1.4f, 1.8f ).c( 0x6C7078, 0xF4F6FA ).p( SPRUCE, 0.4f, 4, 120 ).p( CHALET, 0.02f, 10, 80 ).c( 0x7A5232, 0x4A3A2E )
        .p( ROCK, 0.06f, 3, 50 );
    W( b, BE::DOLOMITES, "DOLOMITES", "ITALY", S::ALPINE ).land( 0x6AAE4A, 0x60A042, 0x7AA25E ).ranges( 0xB8B0A4, 0xE6E2DC, 12 )
        .hills( 45, 14, 10 ).drive( 2, 0.4f, 0.98f, 3 ).bends( 0.85f, 0.75f, 0.9f ).tunnels( 0.12f ).bridges( 0.1f, BR::ARCH, 0xD0C8B8 )
        .p( PEAK, 0.006f, 200, 450, 0.9f, 1.3f ).c( 0xD8D0C2, 0xF4F4F6 ).p( SPRUCE, 0.25f, 10, 150 ).p( CHALET, 0.02f, 10, 80 ).c( 0x8A5A38, 0x4A3A2E )
        .p( COW, 0.04f, 10, 80 ).p( WAYSIDE_CROSS, 0.006f, 3, 10 ).v( 0 );

    // ---- Italy and the south of France -------------------------------------------------------------------------------
    W( b, BE::LAKE_COMO, "LAKE COMO", "ITALY", S::GOLDEN ).sea( 1, 0, -8 ).water( 0x3E7090 ).land( 0x5E9A48, 0x569040, 0x5A8A50 )
        .ranges( 0x56687A, 0xA8B4C4, 10 ).hills( 40, 0, 20 ).drive( 2, 0.7f, 0.98f, 6 ).bends( 0.7f, 0.6f, 0.5f ).tunnels( 0.3f )
        .p( CYPRESS, 0.12f, 3, 40 ).p( COTTAGE, 0.025f, 6, 60 ).c( 0xE8C890, 0xA8583A ).p( ROW_HOUSES, 0.03f, 6, 40 ).c( 0xE8C890, 0xA8583A ).pastel()
        .p( BROADLEAF, 0.08f, 3, 60 ).c( 0x4E8A3A ).p( SAILBOAT, 0.01f, 40, 200 ).floating().p( CHURCH, 0.004f, 20, 120 ).c( 0xEEE6D4, 0xA8583A );
    W( b, BE::TUSCANY, "TUSCANY", "ITALY", S::GOLDEN ).land( 0x9AA65A, 0x8E9A52, 0xA8A060 ).fields( 0.6f, 0xD8B860, 0x8AA050, 0xA88A58 )
        .hills( 26, 6, 6 ).drive( 2, 0.5f, 1.0f, 6 ).bends( 0.55f, 0.45f, 0.65f ).bridges( 0.06f, BR::ARCH, 0xC8B494 )
        .p( CYPRESS, 0.12f, 4, 40 ).p( CYPRESS, 0.05f, 40, 200, 1.0f, 1.3f ).p( COTTAGE, 0.02f, 15, 120 ).c( 0xD8B888, 0xA8583A )
        .p( OLIVE, 0.1f, 6, 60 ).p( VINE_ROW, 0.3f, 10, 60, 1, 1 ).p( UMBRELLA_PINE, 0.02f, 8, 80 ).p( CHURCH, 0.003f, 60, 200 ).c( 0xD8C8A8, 0xA8583A );
    W( b, BE::VIA_APPIA, "VIA APPIA", "ITALY", S::NOON ).land( 0x8E9E58, 0x82924E, 0x9A9A62 ).road( 0x6A6A70 ).drive( 2, 0.6f, 1.0f, 7 )
        .bends( 0.25f, 0.2f, 0.25f ).bridges( 0.08f, BR::ARCH, 0xC0A884 )
        .p( UMBRELLA_PINE, 0.15f, 3, 60 ).p( CYPRESS, 0.06f, 4, 60 ).p( COLUMNS, 0.008f, 10, 80 ).p( AQUEDUCT, 0.006f, 40, 160 ).c( 0xC0A884 )
        .p( RUIN, 0.006f, 10, 80 ).p( STONE_WALL, 0.2f, 3, 3, 1, 1 ).p( OLIVE, 0.05f, 8, 60 );
    W( b, BE::AMALFI, "AMALFI COAST", "ITALY", S::NOON ).sea( 1, 0, -30 ).water( 0x1E6A9A ).land( 0x6E9A4E, 0x64904A, 0x7A8A5C )
        .hills( 40, 0, 40 ).drive( 2, 0.6f, 0.98f, 6 ).bends( 0.9f, 0.8f, 0.8f ).tunnels( 0.25f ).bridges( 0.1f, BR::ARCH, 0xD8CCB0 )
        .p( ROW_HOUSES, 0.05f, 6, 40 ).c( 0xF0D8B0, 0xC86A48 ).pastel().p( FLAT_HOUSE, 0.03f, 6, 40 ).c( 0xF0D8B0, 0x3A6AA8 ).pastel()
        .p( BROADLEAF, 0.08f, 3, 40, 0.6f, 0.9f ).c( 0x5E9A3A ).p( UMBRELLA_PINE, 0.03f, 5, 60 ).p( BOAT, 0.02f, 40, 150 ).c( 0x3A6AB8 ).pastel().floating()
        .p( SAILBOAT, 0.01f, 60, 250 ).floating();
    W( b, BE::SICILY, "SICILY", "ITALY", S::NOON ).land( 0xB0A060, 0xA4945A, 0x9A8E62 ).hills( 24, 6, 6 ).drive( 2, 0.5f, 0.98f, 6 )
        .bends( 0.55f, 0.45f, 0.6f ).bridges( 0.1f, BR::VIADUCT, 0xC8C0B0 )
        .p( VOLCANO_CONE, 0.003f, 300, 600, 1.2f, 1.5f ).c( 0x6A625E, 0xE8ECF0 ).p( PRICKLY_PEAR, 0.1f, 3, 40 ).p( OLIVE, 0.12f, 5, 60 )
        .p( BROADLEAF, 0.05f, 4, 60, 0.6f, 0.8f ).c( 0x4E8A3A ).p( FLAT_HOUSE, 0.015f, 10, 80 ).c( 0xE0CCA4, 0x8A5A3A )
        .p( CHURCH, 0.003f, 40, 200 ).c( 0xE0CCA4, 0xA8583A ).p( STONE_WALL, 0.2f, 3, 3, 1, 1 );
    W( b, BE::COTE_D_AZUR, "COTE D AZUR", "FRANCE", S::NOON ).sea( 1, 0, -14 ).water( 0x1E7AB0 ).land( 0x6E9A4E, 0x64904A, 0x7A8E5C )
        .hills( 30, 0, 18 ).drive( 2, 0.8f, 1.0f, 0 ).bends( 0.7f, 0.55f, 0.5f ).tunnels( 0.15f ).bridges( 0.06f, BR::ARCH, 0xD8CCB0 )
        .p( UMBRELLA_PINE, 0.1f, 3, 50 ).p( PALM, 0.06f, 2, 20 ).p( FLAT_HOUSE, 0.03f, 6, 50 ).c( 0xF0D8B0, 0x4A7AA8 ).pastel()
        .p( COTTAGE, 0.02f, 8, 60 ).c( 0xF2D2A0, 0xB8603E ).p( SAILBOAT, 0.015f, 40, 200 ).floating()
        .p( BOAT, 0.015f, 40, 150, 1.6f, 2.0f ).c( 0xF4F4F4, 0x2A4A8A ).v( 1 ).floating().p( FLOWER_BUSH, 0.06f, 2, 20 ).c( 0xD03A9A );
    W( b, BE::CAMARGUE, "CAMARGUE", "FRANCE", S::MORNING ).hills( 2, 1, 1, 0.6f ).water( 0x6A9AB0 ).land( 0x8AA060, 0x7E9458, 0xA8A07A )
        .fields( 0.3f, 0xE8E4DC, 0x9AB070, 0xB8AE88 ).drive( 2, 0.3f, 1.0f, 6 ).bends( 0.3f, 0.3f, 0.1f ).bridges( 0.15f, BR::CAUSEWAY, 0xB8B4AC )
        .p( HORSE, 0.05f, 8, 80 ).v( 1 ).p( FLAMINGO, 0.06f, 12, 120 ).p( REEDS, 0.2f, 2, 40 ).p( COW, 0.03f, 10, 80 ).v( 2 )
        .p( COTTAGE, 0.006f, 15, 80 ).c( 0xF4F2EA, 0x6A6050 );

    // ---- Iberia ----------------------------------------------------------------------------------------------------
    W( b, BE::BASQUE_COAST, "BASQUE COAST", "SPAIN", S::MORNING ).clouds( 0.6f ).sea( 1, 0, -18 ).water( 0x2E5E80 ).hills( 30, 0, 12 )
        .land( 0x5EA44A, 0x569844, 0x6A9458 ).drive( 2, 0.6f, 0.97f, 6 ).bends( 0.7f, 0.55f, 0.65f ).tunnels( 0.15f ).bridges( 0.12f, BR::BEAM )
        .p( COTTAGE, 0.025f, 8, 80 ).c( 0xF2F0E8, 0xB8583C ).p( BROADLEAF, 0.12f, 3, 80 ).c( 0x4E8A3A ).p( SHEEP, 0.04f, 8, 60 )
        .p( BOAT, 0.015f, 40, 150 ).c( 0x2A6AA8 ).pastel().floating().p( PINE, 0.08f, 10, 100 );
    W( b, BE::RIOJA, "RIOJA", "SPAIN", S::GOLDEN ).land( 0xA88A5A, 0x9C7E52, 0x9A8A68 ).fields( 0.5f, 0x8AA04A, 0xB07A4A, 0xC8B070 )
        .ranges( 0x8A7A80, 0xB8B0C0, 6 ).hills( 16, 4, 4 ).drive( 2, 0.5f, 1.0f, 6 ).bends( 0.45f, 0.35f, 0.4f )
        .p( VINE_ROW, 0.7f, 5, 15, 1, 1 ).p( VINE_ROW, 0.6f, 17, 30, 1, 1 ).p( VINE_ROW, 0.4f, 32, 60, 1, 1 )
        .p( FLAT_HOUSE, 0.01f, 15, 80 ).c( 0xD8BC90, 0xA8583A ).p( CHURCH, 0.003f, 60, 200 ).c( 0xC8A878, 0xA8583A )
        .p( CASTLE, 0.003f, 120, 300 ).c( 0xC0A078, 0x8A6A50 );
    W( b, BE::DOURO_VALLEY, "DOURO VALLEY", "PORTUGAL", S::GOLDEN ).sea( 0, 1, -20 ).water( 0x3E6A70 ).hills( 30, 30, 0 )
        .land( 0x8AA050, 0x7E9448, 0x8A8A5A ).drive( 2, 0.4f, 1.0f, 6 ).bends( 0.75f, 0.6f, 0.6f ).bridges( 0.15f, BR::TRUSS, 0x8A4A3A )
        .p( TERRACES, 0.02f, 20, 120 ).c( 0x7A9A40, 0x8A7056 ).p( VINE_ROW, 0.6f, 4, 30, 1, 1 ).p( COTTAGE, 0.012f, 15, 90 ).c( 0xF2EEE4, 0xB8603E )
        .p( OLIVE, 0.05f, 6, 60 ).p( BOAT, 0.01f, 40, 120 ).c( 0x6A4A30, 0xE8E0D0 ).floating();
    W( b, BE::ALGARVE, "ALGARVE", "PORTUGAL", S::NOON ).sea( 1, 0, -14 ).water( 0x2E8AA0 ).land( 0xC8A060, 0xBC9658, 0xA89A6A ).hills( 14, 0, 6 )
        .drive( 2, 0.6f, 1.0f, 6 ).bends( 0.5f, 0.4f, 0.35f )
        .p( SEA_STACK, 0.015f, 40, 150, 0.4f, 0.8f ).c( 0xD8A060 ).floating().p( FLAT_HOUSE, 0.03f, 8, 70 ).c( 0xF8F6F0, 0x3A7ABE )
        .p( OLIVE, 0.06f, 6, 60 ).p( PRICKLY_PEAR, 0.03f, 3, 30 ).p( PALM, 0.03f, 3, 30 ).p( UMBRELLA_PINE, 0.04f, 6, 60 );
    W( b, BE::ANDALUSIA, "ANDALUSIA", "SPAIN", S::NOON ).land( 0xB8A060, 0xAC9658, 0xA09A6A ).hills( 28, 6, 6 ).drive( 2, 0.5f, 1.0f, 6 )
        .bends( 0.55f, 0.45f, 0.55f ).bridges( 0.08f, BR::VIADUCT, 0xD8D0C0 )
        .p( OLIVE, 0.3f, 4, 30, 0.8f, 1.0f ).p( OLIVE, 0.3f, 30, 120, 0.8f, 1.0f ).p( FLAT_HOUSE, 0.02f, 20, 120 ).c( 0xF8F6F2, 0xC8803A )
        .p( CASTLE, 0.002f, 120, 300 ).c( 0xC8A878, 0x8A6A50 ).p( CHURCH, 0.003f, 60, 200 ).c( 0xF4F2EA, 0xC8803A );
    W( b, BE::LA_MANCHA, "LA MANCHA", "SPAIN", S::NOON ).land( 0xB8A26A, 0xAC9660, 0xC0A878 ).fields( 0.5f, 0xC8A860, 0x8A9A4A, 0xA87A50 )
        .hills( 10, 3, 3 ).drive( 2, 0.4f, 1.0f, 6 ).bends( 0.25f, 0.25f, 0.2f )
        .p( ROUND_MILL, 0.015f, 40, 200, 1.0f, 1.2f ).v( 0 ).p( VINE_ROW, 0.2f, 8, 60, 1, 1 ).p( FLAT_HOUSE, 0.008f, 20, 100 ).c( 0xF4F2EA, 0x3A5A9A )
        .p( CASTLE, 0.002f, 150, 350 ).c( 0xC8B088, 0x8A6A50 );
    W( b, BE::COSTA_DEL_SOL, "COSTA DEL SOL", "SPAIN", S::NOON ).sea( 1, 0, -6 ).water( 0x2A78A8 ).land( 0xB8A878, 0xAC9C70, 0x9A9A6A )
        .hills( 26, 0, 14 ).drive( 3, 1.1f, 1.0f, 1 ).bends( 0.45f, 0.35f, 0.35f ).lamps( 40 )
        .p( PALM, 0.15f, 2, 30 ).p( FLAT_HOUSE, 0.03f, 6, 60 ).c( 0xF6F4EE, 0xC88A4A ).p( PANEL_BLOCK, 0.02f, 20, 100 ).c( 0xF2EEE6 ).v( 1 )
        .p( PARASOL, 0.06f, 2, 10 ).pastel().p( FLOWER_BUSH, 0.05f, 2, 20 ).c( 0xD0307A );
    W( b, BE::LANZAROTE, "LANZAROTE", "SPAIN", S::DESERT ).clouds( 0.2f ).sea( 0, 1, -10 ).water( 0x2A6A9A ).land( 0x3A3634, 0x34302E, 0x4A4440 )
        .verge( 0x2A2826 ).hills( 18, 4, 0 ).drive( 2, 0.4f, 0.98f, 3 ).bends( 0.45f, 0.35f, 0.45f )
        .p( FLAT_HOUSE, 0.03f, 8, 80 ).c( 0xFAFAF6, 0x2E7A4A ).p( SHRUB, 0.15f, 3, 60, 0.5f, 0.8f ).c( 0x6A9A3A ).p( STONE_WALL, 0.1f, 3, 3, 1, 1 )
        .p( VOLCANO_CONE, 0.004f, 200, 400, 0.4f, 0.6f ).c( 0x6A3A2E, 0x8A4A36 ).p( PALM, 0.03f, 4, 40 ).p( CACTUS, 0.03f, 4, 40 );

    // ---- Morocco -----------------------------------------------------------------------------------------------------
    W( b, BE::CHEFCHAOUEN, "CHEFCHAOUEN", "MOROCCO", S::MORNING ).land( 0x8AA060, 0x7E9458, 0x9A9A72 ).ranges( 0x6A7480, 0xA8B0BC, 8 )
        .hills( 36, 14, 6 ).drive( 2, 0.5f, 0.98f, 6 ).bends( 0.75f, 0.6f, 0.7f )
        .p( FLAT_HOUSE, 0.06f, 6, 60 ).c( 0x5A9AD8, 0x2A5A9A ).p( FLAT_HOUSE, 0.03f, 6, 60 ).c( 0xF0F2F8, 0x3A7ACA )
        .p( MOSQUE, 0.004f, 30, 150 ).c( 0xF0EEE6, 0x3A7A5A ).p( OLIVE, 0.06f, 6, 60 ).p( PRICKLY_PEAR, 0.03f, 3, 30 );
    W( b, BE::MARRAKECH, "MARRAKECH", "MOROCCO", S::NOON ).land( 0xC89A6A, 0xBC8E60, 0xB89A72 ).ranges( 0xB8A0A0, 0xE8E4EC, 9 )
        .drive( 3, 1.0f, 0.98f, 6 ).bends( 0.35f, 0.3f, 0.15f ).lamps( 45 )
        .p( DATE_PALM, 0.2f, 3, 80 ).p( KASBAH, 0.006f, 40, 150 ).c( 0xC87A5A ).p( FLAT_HOUSE, 0.04f, 6, 50 ).c( 0xD0805E, 0x2A7A5A )
        .p( MOSQUE, 0.003f, 40, 200 ).c( 0xD0805E, 0x2A7A5A ).v( 0 );
    W( b, BE::HIGH_ATLAS, "HIGH ATLAS", "MOROCCO", S::ALPINE ).land( 0xA87A58, 0x9C7050, 0x9A8070 ).verge( 0x9A6A50 ).hills( 45, 16, 12 )
        .ranges( 0x8A6A60, 0xEEF0F4, 12 ).drive( 2, 0.3f, 0.95f, 6 ).bends( 0.85f, 0.8f, 0.9f ).tunnels( 0.05f )
        .p( KASBAH, 0.01f, 30, 150 ).c( 0xB8805A ).p( ADOBE, 0.02f, 10, 80 ).c( 0xB8805A, 0x9A7A50 ).p( BROADLEAF, 0.05f, 4, 40 ).c( 0x6A9A44 )
        .p( SHRUB, 0.1f, 3, 60 ).c( 0x8A8A5A ).p( ROCK, 0.1f, 3, 60 );
    W( b, BE::SAHARA_DUNES, "SAHARA DUNES", "MOROCCO", S::DESERT ).land( 0xE09A58, 0xD48E52, 0xD8A060 ).verge( 0xD49050 )
        .drive( 2, 0.3f, 0.93f, 7 ).bends( 0.35f, 0.3f, 0.4f )
        .p( DUNE, 0.03f, 45, 220, 0.8f, 1.6f ).c( 0xE29A5A ).p( DUNE, 0.02f, 220, 500, 1.6f, 2.6f ).c( 0xE6A462 ).p( CAMEL, 0.03f, 15, 120 ).v( 0 )
        .p( DATE_PALM, 0.02f, 6, 80 ).p( KASBAH, 0.003f, 60, 200 ).c( 0xC0885A ).p( SHRUB, 0.02f, 3, 60, 0.4f, 0.7f ).c( 0x8A8A5A );

    // ---- West and Central Africa ---------------------------------------------------------------------------------------
    W( b, BE::SAHEL, "SAHEL", "MALI", S::HAZE ).sky( 0x8AA8C8, 0xE0C8A0 ).fog( 0.0018f, 0xE0C8A0 ).land( 0xC8A070, 0xBC9468, 0xB0A070 )
        .road( 0x6A625E ).drive( 2, 0.3f, 0.95f, 4 ).bends( 0.3f, 0.3f, 0.2f )
        .p( BAOBAB, 0.025f, 15, 150 ).p( ACACIA, 0.06f, 6, 120 ).p( GRASS_TUFT, 0.25f, 1, 40 ).p( ADOBE, 0.015f, 15, 100 ).c( 0xB07A50, 0xC8A060 )
        .p( MOSQUE, 0.002f, 100, 250 ).c( 0xA8784E, 0xA8784E ).p( CAMEL, 0.01f, 15, 100 ).v( 0 ).p( COW, 0.02f, 10, 80 ).v( 1 );
    W( b, BE::GOLD_COAST, "GOLD COAST", "GHANA", S::TROPICAL ).sea( 1, 0, -5 ).water( 0x2A7AA0 ).land( 0x6E9A42, 0x649040, 0xA06A44 )
        .verge( 0xB0603C ).drive( 2, 0.6f, 0.97f, 4 ).bends( 0.4f, 0.35f, 0.3f )
        .p( PALM, 0.3f, 2, 40 ).p( BOAT, 0.03f, 40, 150 ).c( 0xD84A3A, 0xF0D040 ).pastel().floating().p( COTTAGE, 0.02f, 8, 60 ).c( 0xE8D8B0, 0x8A8A8E ).pastel()
        .p( BANANA, 0.06f, 3, 40 ).p( BROADLEAF, 0.05f, 4, 60 ).c( 0x3E7A34 );
    W( b, BE::CONGO_BASIN, "CONGO BASIN", "DR CONGO", S::HAZE ).rain( 0.2f ).road( 0xA85A36, 0xA85A36 ).kerbs( 0x9A5236, 0xA86040 )
        .verge( 0xB0603A ).land( 0x3E7A34, 0x387030, 0x2E6432 ).sea( 0, 1, -5 ).water( 0x6A5A40, 0.8f ).drive( 2, 0.25f, 0.9f, 4 )
        .bends( 0.55f, 0.45f, 0.4f ).posts( 0 )
        .p( JUNGLE_TREE, 0.5f, 4, 80 ).p( FERN, 0.3f, 1, 30 ).p( BANANA, 0.08f, 3, 30 ).p( BROADLEAF, 0.2f, 3, 60, 1.0f, 1.6f ).c( 0x2E6A2A )
        .p( HUT, 0.01f, 8, 40 );

    // ---- East Africa -------------------------------------------------------------------------------------------------
    W( b, BE::RWANDA_HILLS, "THOUSAND HILLS", "RWANDA", S::MIST ).land( 0x5A9E3E, 0x50923A, 0x5E8E4A ).fields( 0.5f, 0x6AAA40, 0x8A9A44, 0xA87A50 )
        .hills( 40, 10, 10 ).drive( 2, 0.5f, 0.97f, 4 ).bends( 0.8f, 0.6f, 0.8f )
        .p( TERRACES, 0.02f, 30, 150 ).c( 0x6AAA40, 0x9A6A44 ).p( BANANA, 0.12f, 4, 40 ).p( EUCALYPTUS, 0.08f, 6, 80 ).p( TEA_ROW, 0.2f, 6, 30, 1, 1 )
        .p( COTTAGE, 0.015f, 10, 70 ).c( 0xC08A60, 0x8A8A90 );
    W( b, BE::ETHIOPIAN_HIGHLANDS, "ETHIOPIAN HIGHLANDS", "ETHIOPIA", S::MORNING ).land( 0x8AA050, 0x7E944A, 0x9A9A62 )
        .fields( 0.5f, 0xD8C050, 0x8AAA4A, 0xA88054 ).hills( 40, 10, -10 ).ranges( 0x6A7A70, 0xA0AAB0, 8 ).drive( 2, 0.3f, 0.97f, 4 )
        .bends( 0.65f, 0.55f, 0.6f )
        .p( RONDAVEL, 0.03f, 15, 90 ).c( 0xA8805A, 0x8A7448 ).p( EUCALYPTUS, 0.1f, 6, 80 ).p( ACACIA, 0.05f, 8, 100 ).p( COW, 0.03f, 10, 80 ).v( 1 );
    W( b, BE::DANAKIL, "DANAKIL DESERT", "ETHIOPIA", S::DESERT ).fog( 0.0016f, 0xE8D8B0 ).land( 0xE8DCB0, 0xDCD0A4, 0xC8B890 )
        .fields( 0.4f, 0xE8D040, 0x8ACA90, 0xE8E4D8 ).shine( 0.3f ).hills( 6, 2, 2, 0.2f ).water( 0x6AC8B0 ).drive( 2, 0.1f, 0.9f, 2 )
        .bends( 0.3f, 0.3f, 0.15f ).p( ROCK, 0.05f, 3, 60 ).p( CAMEL, 0.02f, 15, 100 ).v( 0 );
    W( b, BE::LAKE_NAKURU, "LAKE NAKURU", "KENYA", S::GOLDEN ).sea( 1, 0, -3 ).water( 0x8AA8A0 ).land( 0x8AA058, 0x7E9450, 0xA0A068 )
        .drive( 2, 0.2f, 0.98f, 4 ).bends( 0.4f, 0.35f, 0.2f )
        .p( FLAMINGO, 0.2f, 2, 12, 0.9f, 1.1f ).p( ACACIA, 0.08f, 8, 120 ).p( GRASS_TUFT, 0.2f, 1, 40 ).p( ZEBRA, 0.03f, 15, 120 );
    W( b, BE::SERENGETI, "SERENGETI", "TANZANIA", S::GOLDEN ).land( 0xB8A868, 0xAC9C60, 0xC0A870 ).hills( 8, 2, 2 ).look( 1.15f )
        .drive( 2, 0.15f, 0.95f, 4 ).bends( 0.3f, 0.3f, 0.2f )
        .p( ACACIA, 0.04f, 20, 200 ).p( KOPJE, 0.01f, 40, 200 ).p( ZEBRA, 0.08f, 10, 150 ).p( GIRAFFE, 0.02f, 15, 150 ).p( ELEPHANT, 0.015f, 20, 200 )
        .p( GRASS_TUFT, 0.3f, 1, 50 ).p( TERMITE_MOUND, 0.02f, 6, 80 );
    W( b, BE::KILIMANJARO, "KILIMANJARO", "TANZANIA", S::MORNING ).land( 0x7AA050, 0x6E9448, 0x8A9A60 ).hills( 16, 4, 4 )
        .drive( 2, 0.4f, 0.98f, 4 ).bends( 0.5f, 0.4f, 0.5f )
        .p( VOLCANO_CONE, 0.006f, 350, 650, 1.6f, 2.0f ).c( 0x6A625E, 0xF4F6FA ).p( BANANA, 0.1f, 3, 40 ).p( BROADLEAF, 0.08f, 4, 60 ).c( 0x4E8A3A )
        .p( ACACIA, 0.04f, 10, 120 ).p( COTTAGE, 0.01f, 10, 80 ).c( 0xC89A70, 0x8A8A90 );
    W( b, BE::ZANZIBAR, "ZANZIBAR", "TANZANIA", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x30B0C0 ).land( 0xE8E0C8, 0xDCD4BC, 0x6A9A50 )
        .verge( 0xE0D8C0 ).drive( 2, 0.4f, 0.95f, 4 ).bends( 0.4f, 0.35f, 0.15f )
        .p( PALM, 0.3f, 2, 40 ).p( SAILBOAT, 0.03f, 40, 200 ).c( 0xF4F0E4, 0x8A6A4A ).v( 1 ).floating().p( FLAT_HOUSE, 0.015f, 8, 60 ).c( 0xE8E2D2, 0x3A6A9A )
        .p( BOAT, 0.02f, 40, 150 ).c( 0x3A8AC8 ).pastel().floating();

    // ---- Southern Africa ---------------------------------------------------------------------------------------------
    W( b, BE::ZAMBEZI, "ZAMBEZI", "ZAMBIA", S::HAZE ).sea( 0, 1, -12 ).water( 0x5A7A6A ).land( 0x7A9A4E, 0x6E8E46, 0x8A9A5A )
        .drive( 2, 0.3f, 0.97f, 4 ).bends( 0.45f, 0.4f, 0.4f ).bridges( 0.2f, BR::TRUSS, 0x9A7A5A )
        .p( GEYSER, 0.004f, 200, 400, 2.0f, 3.0f ).v( 1 ).p( BAOBAB, 0.03f, 10, 120 ).p( ACACIA, 0.05f, 8, 120 ).p( ELEPHANT, 0.015f, 20, 150 );
    W( b, BE::OKAVANGO, "OKAVANGO DELTA", "BOTSWANA", S::GOLDEN ).hills( 2, 1, 1, 0.6f ).water( 0x4A7A8A ).land( 0x8AAA58, 0x7E9E50, 0xA0A060 )
        .drive( 2, 0.15f, 0.97f, 4 ).bends( 0.35f, 0.3f, 0.1f ).bridges( 0.15f, BR::CAUSEWAY, 0x8A8070 )
        .p( REEDS, 0.25f, 2, 40 ).p( PALM, 0.06f, 6, 100 ).p( ELEPHANT, 0.03f, 15, 150 ).p( ACACIA, 0.04f, 10, 120 );
    W( b, BE::KALAHARI, "KALAHARI", "BOTSWANA", S::DESERT ).land( 0xC86A3A, 0xBC6036, 0xC88A5A ).verge( 0xC06A40 ).drive( 2, 0.15f, 0.95f, 7 )
        .bends( 0.25f, 0.25f, 0.2f )
        .p( GRASS_TUFT, 0.3f, 1, 60 ).p( ACACIA, 0.04f, 8, 150 ).p( SHRUB, 0.08f, 3, 60 ).c( 0x9A9A6A ).p( DUNE, 0.01f, 30, 200, 0.5f, 0.8f ).c( 0xC8683A );
    W( b, BE::NAMIB, "NAMIB DESERT", "NAMIBIA", S::DESERT ).sky( 0x2A6ACC, 0xE8C8A0 ).land( 0xD07040, 0xC46638, 0xE0A070 ).verge( 0xC86A3A )
        .fields( 0.2f, 0xEAE6DC, 0xE0DCD0, 0xD8D0C0 ).ranges( 0xB85A30, 0xD8905A, 4 ).drive( 2, 0.15f, 0.94f, 7 ).bends( 0.3f, 0.3f, 0.3f )
        .p( DUNE, 0.04f, 30, 250, 1.5f, 3.0f ).c( 0xD86A38 ).p( DEAD_TREE, 0.03f, 10, 80 );
    W( b, BE::SKELETON_COAST, "SKELETON COAST", "NAMIBIA", S::MIST ).fog( 0.004f, 0xD8D4C8 ).sea( 1, 0, -3 ).water( 0x5A7A80 )
        .land( 0xD8C8A0, 0xCCBC94, 0xC0B090 ).verge( 0xD0C098 ).drive( 2, 0.1f, 0.94f, 2 ).bends( 0.2f, 0.25f, 0.15f )
        .p( DUNE, 0.02f, 20, 200 ).c( 0xD8C8A0 ).p( ROCK, 0.04f, 3, 60 ).p( BOAT, 0.006f, 35, 80, 2.0f, 2.5f ).c( 0x5A4A3A, 0x4A3A2A ).floating();
    W( b, BE::CAPE_PENINSULA, "CAPE PENINSULA", "SOUTH AFRICA", S::MORNING ).sea( 1, 0, -20 ).water( 0x2A5A8A ).land( 0x7A8A58, 0x6E7E50, 0x6A7A58 )
        .hills( 30, 0, 20 ).drive( 2, 0.5f, 0.98f, 6 ).bends( 0.7f, 0.55f, 0.7f )
        .p( TEPUI, 0.003f, 300, 600, 1.0f, 1.3f ).c( 0x6A6A6E, 0x5A7A4A ).p( SHRUB, 0.3f, 2, 60 ).c( 0x7A8A5A ).p( FLOWER_BUSH, 0.05f, 2, 30 ).c( 0xE87A3A )
        .p( ROCK, 0.08f, 3, 60 );
    W( b, BE::CAPE_WINELANDS, "CAPE WINELANDS", "SOUTH AFRICA", S::GOLDEN ).ranges( 0x5A6A80, 0x9AA8C0, 10 ).land( 0x7AA050, 0x6E9448, 0x86985E )
        .hills( 16, 4, 4 ).drive( 2, 0.5f, 1.0f, 6 ).bends( 0.5f, 0.4f, 0.45f )
        .p( VINE_ROW, 0.7f, 5, 40, 1, 1 ).p( CAPE_DUTCH, 0.01f, 20, 100 ).p( OAK, 0.08f, 6, 80 );
    W( b, BE::DRAKENSBERG, "DRAKENSBERG", "SOUTH AFRICA", S::MORNING ).land( 0x6AA048, 0x609440, 0x7A9A5A ).hills( 50, 20, 10 )
        .ranges( 0x5A6A5A, 0x8A9AA0, 12 ).drive( 2, 0.3f, 0.97f, 4 ).bends( 0.6f, 0.5f, 0.7f )
        .p( TEPUI, 0.004f, 300, 600 ).c( 0x8A7A6A, 0x6A9A4A ).p( RONDAVEL, 0.02f, 15, 90 ).c( 0xE8E0D0, 0x9A8A5A ).p( COW, 0.02f, 10, 80 ).v( 1 )
        .p( SHRUB, 0.1f, 3, 60 ).c( 0x8A9A5A );
    W( b, BE::BAOBAB_AVENUE, "AVENUE OF BAOBABS", "MADAGASCAR", S::SUNSET ).land( 0xB8703C, 0xAC6636, 0x8A9A4A ).fields( 0.3f, 0x8AB04A, 0xA8603A, 0x9AA858 )
        .hills( 4, 1, 1, 0.2f ).water( 0x7A6A50 ).road( 0x9A6A50, 0x9A6A50 ).kerbs( 0x9A6A50, 0xA87050 ).verge( 0xA86A44 ).drive( 2, 0.15f, 0.92f, 6 )
        .bends( 0.15f, 0.2f, 0.1f ).posts( 0 )
        .p( BAOBAB, 0.12f, 4, 10, 1.1f, 1.4f ).p( BAOBAB, 0.02f, 10, 150 ).p( COW, 0.02f, 10, 60 ).v( 1 ).p( COTTAGE, 0.006f, 15, 80 ).c( 0xB8703C, 0x8A7A5A );
    W( b, BE::SEYCHELLES, "SEYCHELLES", "SEYCHELLES", S::TROPICAL ).sea( 1, 0, -2 ).water( 0x28B8C8 ).land( 0xF0EAD8, 0xE6E0CE, 0x4E8A44 )
        .verge( 0xE8E2D0 ).hills( 30, 0, 24 ).drive( 2, 0.3f, 0.97f, 4 ).bends( 0.65f, 0.55f, 0.5f )
        .p( BOULDER, 0.04f, 2, 20 ).p( KOPJE, 0.01f, 3, 30 ).p( PALM, 0.25f, 2, 50 ).p( SAILBOAT, 0.01f, 40, 200 ).floating();

    // ---- Arabia ------------------------------------------------------------------------------------------------------
    W( b, BE::SOCOTRA, "SOCOTRA", "YEMEN", S::DESERT ).land( 0xB8A890, 0xAC9C88, 0xA09484 ).hills( 40, 12, 12 ).ranges( 0x8A7A70, 0xB0A8A0, 8 )
        .drive( 2, 0.1f, 0.95f, 2 ).bends( 0.6f, 0.5f, 0.6f )
        .p( DRAGON_TREE, 0.08f, 6, 150 ).p( BAOBAB, 0.02f, 6, 80, 0.5f, 0.7f ).p( ROCK, 0.1f, 3, 60 );
    W( b, BE::HAJAR_MOUNTAINS, "HAJAR MOUNTAINS", "OMAN", S::NOON ).land( 0xA8957C, 0x9C8A72, 0x8C8070 ).verge( 0x9A8A70 ).hills( 50, 24, 18 )
        .drive( 2, 0.3f, 0.98f, 2 ).bends( 0.8f, 0.7f, 0.8f ).bridges( 0.05f, BR::BEAM, 0xC8B898 )
        .p( DATE_PALM, 0.15f, 4, 40 ).p( FORT, 0.004f, 40, 160 ).c( 0xC8A878, 0xB8986A ).v( 0 ).p( ADOBE, 0.02f, 10, 80 ).c( 0xC0A07A, 0xA88A60 )
        .p( ROCK, 0.1f, 3, 60 );
    W( b, BE::EMPTY_QUARTER, "EMPTY QUARTER", "SAUDI ARABIA", S::DESERT ).land( 0xE0A060, 0xD49458, 0xE8B070 ).verge( 0xD8A060 )
        .drive( 2, 0.1f, 0.94f, 7 ).bends( 0.2f, 0.2f, 0.3f )
        .p( DUNE, 0.05f, 20, 300, 2.0f, 3.5f ).c( 0xE6A462 ).p( CAMEL, 0.02f, 15, 150 ).v( 0 );
    W( b, BE::DUBAI, "DUBAI", "UAE", S::HAZE ).sky( 0x6A9ACC, 0xE8DCC0 ).fog( 0.0014f, 0xE0D4B8 ).land( 0xD8C098, 0xCCB48C, 0xD0B890 )
        .drive( 4, 1.4f, 1.0f, 1 ).bends( 0.25f, 0.2f, 0.1f ).lamps( 40 ).gantries( 500 ).bridges( 0.08f, BR::CABLE_STAYED, 0xE8E8E8 )
        .p( SUPERTALL, 0.002f, 200, 450, 1.0f, 1.2f ).p( GLASS_TOWER, 0.08f, 30, 250 ).c( 0x7A9AB8 ).p( GLASS_TOWER, 0.04f, 30, 250 ).c( 0xC8C8C0 )
        .p( DATE_PALM, 0.2f, 3, 12 );

    // ---- The Levant and Egypt ----------------------------------------------------------------------------------------
    W( b, BE::WADI_RUM, "WADI RUM", "JORDAN", S::GOLDEN ).land( 0xD87A48, 0xCC7042, 0xD89060 ).verge( 0xC87048 ).drive( 2, 0.1f, 0.95f, 7 )
        .bends( 0.35f, 0.3f, 0.3f )
        .p( ARCH, 0.006f, 50, 200 ).c( 0xC8704A ).p( MESA, 0.01f, 60, 300 ).p( DUNE, 0.015f, 20, 150 ).c( 0xD87A48 ).p( CAMEL, 0.02f, 15, 100 ).v( 0 )
        .p( ROCK, 0.08f, 3, 60 );
    W( b, BE::DEAD_SEA, "DEAD SEA", "JORDAN", S::HAZE ).fog( 0.0025f, 0xC8C0B0 ).sea( 0, 1, -8 ).water( 0x6AA8B0 ).land( 0xC8B898, 0xBCAC8E, 0xB0A080 )
        .fields( 0.2f, 0xF4F2EA, 0xE8E4DC, 0xD8D0C0 ).ranges( 0xA89A90, 0xC8C0B8, 7 ).hills( 20, 16, 0 ).drive( 2, 0.3f, 0.98f, 2 ).bends( 0.4f, 0.35f, 0.3f )
        .p( DATE_PALM, 0.03f, 6, 60 ).p( ROCK, 0.05f, 3, 60 );
    W( b, BE::GIZA, "GIZA PLATEAU", "EGYPT", S::DESERT ).land( 0xDCC090, 0xD0B488, 0xD8BC90 ).verge( 0xD0B488 ).drive( 3, 0.8f, 0.97f, 7 )
        .bends( 0.3f, 0.3f, 0.2f )
        .p( STONE_PYRAMID, 0.006f, 120, 400, 0.9f, 1.4f ).p( CAMEL, 0.03f, 10, 100 ).v( 0 ).p( DATE_PALM, 0.03f, 6, 60 )
        .p( FLAT_HOUSE, 0.01f, 10, 80 ).c( 0xC8B090, 0x8A6A4A )
        .landmark( Lm::SPHINX, 40, 80 );
    W( b, BE::NILE_VALLEY, "NILE VALLEY", "EGYPT", S::NOON ).sea( 0, 1, -4 ).water( 0x3A6A80 ).land( 0x6AA048, 0x5E9440, 0xD8BC90 )
        .fields( 0.6f, 0x6AAA44, 0x9AB050, 0xD8C890 ).ranges( 0xD8B890, 0xE8D8B8, 5 ).drive( 2, 0.6f, 0.98f, 7 ).bends( 0.35f, 0.3f, 0.1f )
        .p( DATE_PALM, 0.3f, 3, 60 ).p( SAILBOAT, 0.03f, 40, 160 ).c( 0xF4F2EA, 0x6A4A30 ).v( 1 ).floating().p( FLAT_HOUSE, 0.015f, 10, 80 ).c( 0xC8A880, 0x6A6A70 )
        .p( COW, 0.02f, 8, 60 ).v( 2 );
    W( b, BE::WHITE_DESERT, "WHITE DESERT", "EGYPT", S::DESERT ).land( 0xEAE4D4, 0xE0D8C6, 0xD8CCB0 ).verge( 0xE0D8C6 ).ranges( 0xE0D6C0, 0xF0EAE0, 3 )
        .drive( 2, 0.05f, 0.95f, 2 ).bends( 0.3f, 0.3f, 0.2f )
        .p( CHALK_ROCK, 0.05f, 10, 150, 0.8f, 2.0f ).p( ROCK, 0.03f, 3, 60 ).p( DUNE, 0.01f, 30, 200 ).c( 0xE8D8B8 );
    W( b, BE::CEDARS, "CEDAR MOUNTAINS", "LEBANON", S::ALPINE ).land( 0x7A9A58, 0x6E8E50, 0x8A8E70 ).ranges( 0x8A8A90, 0xF0F2F6, 10 )
        .hills( 40, 10, 10 ).drive( 2, 0.4f, 0.98f, 6 ).bends( 0.7f, 0.6f, 0.75f )
        .p( CEDAR, 0.2f, 6, 100 ).p( ROCK, 0.1f, 3, 60 ).p( STONE_WALL, 0.2f, 3, 3, 1, 1 ).p( FLAT_HOUSE, 0.01f, 10, 80 ).c( 0xE0D8C0, 0xA8583A )
        .p( CHURCH, 0.003f, 30, 150 ).c( 0xD8CCB0, 0x8A6A50 );

    // ---- Turkey, Greece, the Balkans ---------------------------------------------------------------------------------
    W( b, BE::CAPPADOCIA, "CAPPADOCIA", "TURKEY", S::GOLDEN ).sun( 10, 20 ).land( 0xD8C0A0, 0xCCB494, 0xB8A890 ).verge( 0xD0B898 )
        .hills( 24, 10, 10 ).drive( 2, 0.3f, 0.98f, 6 ).bends( 0.55f, 0.45f, 0.5f )
        .p( FAIRY_CHIMNEY, 0.04f, 8, 150, 0.8f, 1.6f ).p( BALLOON, 0.03f, 0, 400, 0.8f, 1.2f ).pastel().p( VINE_ROW, 0.1f, 8, 50, 1, 1 )
        .p( BROADLEAF, 0.03f, 4, 60, 0.6f, 0.8f ).c( 0x7A9A4A );
    W( b, BE::LYCIAN_COAST, "LYCIAN COAST", "TURKEY", S::NOON ).sea( 1, 0, -16 ).water( 0x1E9AB8 ).land( 0x6E9A4E, 0x64904A, 0x7A8A5C )
        .hills( 30, 0, 20 ).drive( 2, 0.4f, 0.98f, 6 ).bends( 0.75f, 0.6f, 0.6f ).tunnels( 0.08f )
        .p( PINE, 0.25f, 3, 80 ).p( UMBRELLA_PINE, 0.05f, 5, 60 ).p( COLUMNS, 0.004f, 10, 80 ).p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0xF2EEE4, 0x3A6A9A )
        .p( SAILBOAT, 0.02f, 40, 200 ).c( 0xF4F2EA, 0x8A5A3A ).floating();
    W( b, BE::ISTANBUL, "ISTANBUL", "TURKEY", S::GOLDEN ).sea( 0, 1, -6 ).water( 0x2A5A80 ).land( 0x7A8A6A, 0x728262, 0x8A8A7A )
        .drive( 3, 1.3f, 0.97f, 6 ).bends( 0.45f, 0.4f, 0.3f ).lamps( 35 ).posts( 0 ).bridges( 0.25f, BR::SUSPENSION, 0xC8CCD0 )
        .p( MOSQUE, 0.015f, 40, 200 ).c( 0xE0D8C8, 0x7A8A9A ).p( ROW_HOUSES, 0.1f, 6, 40 ).c( 0xE0D8C8, 0xA8583A ).pastel()
        .p( GLASS_TOWER, 0.01f, 120, 300 ).c( 0x7A9AB8 ).p( BOAT, 0.02f, 40, 160, 1.8f, 2.2f ).c( 0xF0F0F0, 0x2A3A5A ).v( 1 ).floating();
    W( b, BE::SANTORINI, "SANTORINI", "GREECE", S::NOON ).sea( 1, 0, -30 ).water( 0x1A5AA0 ).land( 0xA08A70, 0x947E66, 0x8A7A68 ).verge( 0x9A8A78 )
        .hills( 20, 0, 10 ).drive( 2, 0.5f, 0.97f, 3 ).bends( 0.8f, 0.7f, 0.6f )
        .p( FLAT_HOUSE, 0.1f, 4, 40 ).c( 0xFAFAF8, 0x2A5AB8 ).p( ROUND_MILL, 0.005f, 20, 100 ).v( 1 ).p( SHRUB, 0.08f, 3, 40, 0.4f, 0.6f ).c( 0x7A8A50 );
    W( b, BE::PELOPONNESE, "PELOPONNESE", "GREECE", S::NOON ).land( 0x9AA060, 0x8E9458, 0xA09A6A ).ranges( 0x7A8890, 0xA8B4C0, 7 )
        .hills( 24, 8, 8 ).drive( 2, 0.4f, 0.98f, 6 ).bends( 0.6f, 0.5f, 0.55f )
        .p( OLIVE, 0.3f, 4, 100 ).p( CYPRESS, 0.06f, 5, 60 ).p( COLUMNS, 0.006f, 15, 120 ).p( FLAT_HOUSE, 0.015f, 10, 80 ).c( 0xF2EEE4, 0x3A6A9A )
        .p( STONE_WALL, 0.15f, 3, 3, 1, 1 ).p( SHEEP, 0.02f, 8, 60 );
    W( b, BE::METEORA, "METEORA", "GREECE", S::MORNING ).land( 0x7A9A58, 0x6E8E50, 0x8A9468 ).hills( 20, 6, 6 ).drive( 2, 0.4f, 0.98f, 6 )
        .bends( 0.6f, 0.5f, 0.5f )
        .p( PILLAR, 0.02f, 60, 300, 0.8f, 1.2f ).p( CYPRESS, 0.05f, 5, 60 ).p( BROADLEAF, 0.08f, 4, 80 ).c( 0x5A8A40 )
        .p( CHURCH, 0.004f, 30, 150 ).c( 0xC8A880, 0xA8583A );
    W( b, BE::KOTOR, "BAY OF KOTOR", "MONTENEGRO", S::MORNING ).sea( 1, 0, -6 ).water( 0x1E5A7A ).hills( 40, 0, 40 ).ranges( 0x4A5660, 0x8A96A4, 12 )
        .land( 0x5E8A48, 0x568040, 0x6A8A58 ).drive( 2, 0.5f, 0.98f, 6 ).bends( 0.8f, 0.6f, 0.4f )
        .p( ROW_HOUSES, 0.04f, 6, 30 ).c( 0xE0D4BC, 0xA8583A ).p( CHURCH, 0.006f, 15, 80 ).c( 0xE0D8C8, 0xA8583A ).p( CYPRESS, 0.1f, 3, 50 )
        .p( BROADLEAF, 0.05f, 4, 60 ).c( 0x4E8A3A ).p( BOAT, 0.02f, 40, 150 ).c( 0x3A6AB8 ).pastel().floating();
    W( b, BE::DALMATIA, "DALMATIAN COAST", "CROATIA", S::NOON ).sea( 1, 0, -10 ).water( 0x1E8AB0 ).land( 0x9A9A80, 0x8E8E76, 0xB0B0A0 ).verge( 0xC8C4B8 )
        .hills( 26, 0, 14 ).drive( 2, 0.5f, 0.98f, 6 ).bends( 0.7f, 0.55f, 0.5f ).bridges( 0.08f, BR::ARCH, 0xD8D4C8 )
        .p( ROCK, 0.2f, 2, 40 ).p( STONE_WALL, 0.4f, 3, 3, 1, 1 ).p( SHRUB, 0.2f, 2, 50 ).c( 0x6A7A50 ).p( OLIVE, 0.05f, 5, 60 )
        .p( ROW_HOUSES, 0.02f, 8, 60 ).c( 0xE8E2D4, 0xB8603E ).p( SAILBOAT, 0.02f, 40, 200 ).floating();
    W( b, BE::ROSE_VALLEY, "ROSE VALLEY", "BULGARIA", S::MORNING ).land( 0x7AAA50, 0x6E9E48, 0x86A05E ).fields( 0.6f, 0xE878A8, 0x6AA04A, 0xC8B060 )
        .ranges( 0x6A7A70, 0x9AA8B0, 8 ).hills( 14, 4, 4 ).drive( 2, 0.5f, 1.0f, 6 ).bends( 0.45f, 0.4f, 0.35f )
        .p( FLOWER_BUSH, 0.1f, 2, 20 ).c( 0xE8609A ).p( COTTAGE, 0.015f, 10, 80 ).c( 0xF0E8D8, 0xA8583A ).p( BROADLEAF, 0.05f, 6, 80 ).c( 0x5A9440 );
    W( b, BE::DANUBE_DELTA, "DANUBE DELTA", "ROMANIA", S::GOLDEN ).hills( 1, 0, 0, 0.7f ).sea( 0, 1, -2 ).water( 0x5A8070 ).land( 0x7A9A50, 0x6E8E48, 0x8A9A60 )
        .drive( 2, 0.2f, 0.98f, 6 ).bends( 0.35f, 0.3f, 0.05f ).bridges( 0.2f, BR::CAUSEWAY, 0x8A8A80 )
        .p( REEDS, 0.5f, 2, 60 ).p( BROADLEAF, 0.05f, 6, 80 ).c( 0x8AA060 ).p( HUT, 0.01f, 8, 40 ).p( BOAT, 0.02f, 40, 150 ).c( 0x3A5A8A, 0xE8E8E8 ).floating();
    W( b, BE::TRANSYLVANIA, "TRANSYLVANIA", "ROMANIA", S::MORNING ).land( 0x6EA24A, 0x649644, 0x7A9A5A ).hills( 30, 8, 8 ).ranges( 0x5A6A6A, 0x8A9AA0, 8 )
        .drive( 2, 0.4f, 0.98f, 6 ).bends( 0.6f, 0.5f, 0.6f ).bridges( 0.08f, BR::ARCH, 0xB0A890 )
        .p( HAYBALE, 0.03f, 10, 60 ).p( CHURCH, 0.006f, 30, 150 ).c( 0xE8E0D0, 0x7A4A3A ).p( CASTLE, 0.003f, 120, 300 ).c( 0xE8E2D6, 0xA8583A )
        .p( FENCE, 0.3f, 4, 4, 1, 1 ).p( HORSE, 0.02f, 10, 80 ).p( SPRUCE, 0.1f, 40, 200 ).p( COTTAGE, 0.015f, 10, 80 ).c( 0xE8DCB8, 0x6A4A3A );

    // ---- Central and Eastern Europe ------------------------------------------------------------------------------------
    W( b, BE::PUSZTA, "PUSZTA", "HUNGARY", S::GOLDEN ).land( 0x9AAA5A, 0x8E9E52, 0xA8A86A ).hills( 3, 1, 1 ).drive( 2, 0.3f, 1.0f, 6 )
        .bends( 0.2f, 0.25f, 0.1f )
        .p( HORSE, 0.06f, 10, 150 ).p( COW, 0.04f, 10, 120 ).v( 1 ).p( COTTAGE, 0.006f, 15, 100 ).c( 0xF4F2EA, 0x9A8A5A ).p( GRASS_TUFT, 0.2f, 1, 50 )
        .p( POPLAR, 0.03f, 10, 120 ).p( BROADLEAF, 0.03f, 8, 100 ).c( 0x6A9A44 );
    W( b, BE::BOHEMIA, "BOHEMIA", "CZECHIA", S::MORNING ).land( 0x6EA24A, 0x649644, 0x7A9A5A ).fields( 0.6f, 0xF0D830, 0x7AAA48, 0xA88A5A )
        .hills( 18, 4, 4 ).drive( 2, 0.5f, 1.0f, 1 ).bends( 0.45f, 0.4f, 0.45f )
        .p( CASTLE, 0.004f, 80, 250 ).c( 0xE8E0D0, 0xB8583A ).p( COTTAGE, 0.02f, 10, 80 ).c( 0xF0E6D0, 0xC0583A ).p( CHURCH, 0.004f, 30, 150 ).c( 0xF0E8D8, 0x3A5A4A )
        .p( BROADLEAF, 0.25f, 2.5f, 3, 0.7f, 0.9f ).c( 0x5E9440 );
    W( b, BE::TATRA, "TATRA MOUNTAINS", "POLAND", S::ALPINE ).land( 0x5E9A44, 0x56903E, 0x6A8A58 ).ranges( 0x5A6A78, 0xC8D0DC, 11 ).hills( 40, 12, 10 )
        .drive( 2, 0.4f, 0.97f, 3 ).bends( 0.7f, 0.6f, 0.75f )
        .p( SPRUCE, 0.45f, 4, 150 ).p( CHALET, 0.03f, 10, 80 ).c( 0x6A4A30, 0x4A3628 ).p( SHEEP, 0.03f, 8, 60 ).p( WAYSIDE_CROSS, 0.006f, 3, 10 ).v( 0 );
    W( b, BE::MASURIA, "MASURIA", "POLAND", S::MORNING ).hills( 10, 3, 3, 0.5f ).water( 0x3E6A8A ).land( 0x6AA048, 0x609440, 0x76965A )
        .drive( 2, 0.4f, 1.0f, 3 ).bends( 0.5f, 0.4f, 0.35f )
        .p( BIRCH, 0.2f, 4, 80 ).p( PINE, 0.15f, 8, 120 ).p( COTTAGE, 0.01f, 15, 80 ).c( 0xB06040, 0x8A3A2A ).p( BROADLEAF, 0.15f, 2.5f, 3, 0.8f, 1.0f ).c( 0x5A9440 );
    W( b, BE::SUNFLOWER_STEPPE, "SUNFLOWER STEPPE", "UKRAINE", S::GOLDEN ).land( 0x8AA04A, 0x7E9442, 0x9AA058 ).look( 1.18f )
        .fields( 0.9f, 0xF0C020, 0x6A8A30, 0xD8B040 ).hills( 6, 1, 1 ).drive( 2, 0.3f, 1.0f, 3 ).bends( 0.2f, 0.2f, 0.15f )
        .p( POPLAR, 0.1f, 60, 70 ).p( COTTAGE, 0.006f, 15, 90 ).c( 0xF0EEE6, 0x5A7AA0 ).p( ONION_CHURCH, 0.002f, 80, 250 ).c( 0xF0EEE6, 0x3A8A5A ).v( 1 );
    W( b, BE::CAUCASUS, "CAUCASUS", "GEORGIA", S::ALPINE ).land( 0x5E9E46, 0x56923E, 0x6E8E5A ).ranges( 0x5A6878, 0xEEF2F8, 14 ).hills( 45, 14, 12 )
        .drive( 2, 0.3f, 0.95f, 6 ).bends( 0.75f, 0.65f, 0.8f )
        .p( WATCHTOWER, 0.02f, 15, 150 ).c( 0x9A9488 ).v( 1 ).p( CHURCH, 0.004f, 30, 150 ).c( 0xC8B898, 0x6A6A70 ).p( COW, 0.03f, 10, 80 ).v( 1 )
        .p( SPRUCE, 0.1f, 20, 150 );
    W( b, BE::YAZD, "PERSIAN DESERT", "IRAN", S::DESERT ).land( 0xD8B890, 0xCCAC84, 0xC8B08A ).ranges( 0xB8A090, 0xD8C8B8, 6 ).drive( 2, 0.4f, 0.97f, 2 )
        .bends( 0.3f, 0.3f, 0.2f )
        .p( ADOBE, 0.04f, 10, 100 ).c( 0xC8A47A, 0xB89068 ).p( MOSQUE, 0.004f, 40, 200 ).c( 0xD8C0A0, 0x3A9AB8 ).p( DATE_PALM, 0.03f, 6, 80 )
        .p( WATCHTOWER, 0.006f, 20, 100, 0.6f, 0.8f ).c( 0xC8A47A ).v( 1 );

    // ---- Central Asia ------------------------------------------------------------------------------------------------
    W( b, BE::SAMARKAND, "SAMARKAND", "UZBEKISTAN", S::NOON ).land( 0xB8A880, 0xAC9C78, 0xA8A080 ).drive( 3, 0.8f, 1.0f, 3 ).bends( 0.35f, 0.3f, 0.2f )
        .p( MOSQUE, 0.015f, 30, 150 ).c( 0xD8C8A8, 0x2AA0C0 ).p( POPLAR, 0.15f, 3, 50 ).p( ADOBE, 0.02f, 10, 80 ).c( 0xC8A880, 0xB89A70 )
        .p( BROADLEAF, 0.04f, 4, 60 ).c( 0x6A9A44 );
    W( b, BE::KAZAKH_STEPPE, "KAZAKH STEPPE", "KAZAKHSTAN", S::GOLDEN ).land( 0xA8A868, 0x9C9C60, 0xB0A870 ).hills( 6, 1, 1 ).drive( 2, 0.2f, 1.0f, 3 )
        .bends( 0.2f, 0.2f, 0.15f )
        .p( YURT, 0.01f, 20, 150 ).p( HORSE, 0.05f, 10, 200 ).p( GRASS_TUFT, 0.3f, 1, 60 ).p( CAMEL, 0.01f, 20, 150 ).v( 1 );
    W( b, BE::TIAN_SHAN, "TIAN SHAN", "KYRGYZSTAN", S::ALPINE ).land( 0x6AA44C, 0x609844, 0x7A9A5E ).hills( 30, 8, 8, 0.2f ).water( 0x3AA0C0 )
        .ranges( 0x6A7A8A, 0xF0F4F8, 13 ).drive( 2, 0.2f, 0.97f, 3 ).bends( 0.55f, 0.45f, 0.6f )
        .p( YURT, 0.03f, 15, 150 ).c( 0xF2EEE4, 0xC03A3A ).p( HORSE, 0.05f, 10, 150 ).p( SHEEP, 0.05f, 8, 100 ).p( SPRUCE, 0.05f, 60, 200 );
    W( b, BE::PAMIR_HIGHWAY, "PAMIR HIGHWAY", "TAJIKISTAN", S::ALPINE ).look( 0.9f ).land( 0xA8957C, 0x9C8A72, 0x8A8478 ).verge( 0x9A8A78 )
        .road( 0x6E6A66 ).ranges( 0x8A7A70, 0xF0F2F6, 14 ).hills( 40, 12, 10 ).drive( 2, 0.1f, 0.9f, 3 ).bends( 0.7f, 0.6f, 0.7f )
        .p( ROCK, 0.15f, 3, 60 ).p( YAK, 0.02f, 10, 100 ).p( ADOBE, 0.006f, 15, 80 ).c( 0xB8A080, 0x9A8A70 );

    // ---- South Asia --------------------------------------------------------------------------------------------------
    W( b, BE::KARAKORAM, "KARAKORAM HIGHWAY", "PAKISTAN", S::MORNING ).land( 0x9A8A70, 0x8E7E66, 0x8A8070 ).verge( 0x8A7A66 ).hills( 50, 40, 0 )
        .sea( 0, 1, -16 ).water( 0x7A8A80, 0.6f ).ranges( 0x6A6460, 0xF4F6FA, 16 ).drive( 2, 0.3f, 0.95f, 3 ).bends( 0.75f, 0.65f, 0.7f )
        .tunnels( 0.1f ).bridges( 0.15f, BR::SUSPENSION, 0x8A8A8A )
        .p( PEAK, 0.006f, 200, 450, 1.4f, 2.0f ).c( 0x7A7470, 0xF4F6FA ).p( BROADLEAF, 0.06f, 4, 40, 0.6f, 0.9f ).c( 0x7AA04A ).p( POPLAR, 0.08f, 4, 40 )
        .p( FLAT_HOUSE, 0.01f, 10, 60 ).c( 0xB8A888, 0x8A7A60 );
    W( b, BE::LADAKH, "LADAKH", "INDIA", S::ALPINE ).land( 0xB8A07C, 0xAC9472, 0xA89880 ).verge( 0xA89070 ).ranges( 0x9A8070, 0xF2F4F8, 12 )
        .hills( 30, 10, 10 ).drive( 2, 0.2f, 0.95f, 3 ).bends( 0.6f, 0.55f, 0.6f )
        .p( STUPA, 0.02f, 10, 120 ).c( 0xF4F2EA, 0xD8A84A ).p( PRAYER_FLAGS, 0.03f, 4, 40 ).p( FLAT_HOUSE, 0.01f, 10, 80 ).c( 0xF0ECE0, 0x6A4A3A )
        .p( FORT, 0.002f, 100, 250 ).c( 0xF0ECE0, 0x9A2A2A ).v( 1 ).p( POPLAR, 0.03f, 6, 60 );
    W( b, BE::RAJASTHAN, "RAJASTHAN", "INDIA", S::DESERT ).fog( 0.0014f, 0xE0C8A0 ).land( 0xD8B88A, 0xCCAC80, 0xD0B088 ).drive( 2, 0.6f, 0.95f, 6 )
        .bends( 0.35f, 0.3f, 0.25f )
        .p( FORT, 0.005f, 80, 250 ).c( 0xD8B080, 0xC89A68 ).v( 0 ).p( CAMEL, 0.04f, 10, 100 ).v( 0 ).p( ACACIA, 0.04f, 8, 100 )
        .p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0x6AA0D8, 0xE8A040 ).pastel();
    W( b, BE::AGRA, "AGRA", "INDIA", S::HAZE ).sky( 0x8AA8C8, 0xF0D8B0 ).land( 0x8AA050, 0x7E9448, 0x9AA068 ).drive( 3, 1.2f, 0.95f, 6 )
        .bends( 0.35f, 0.3f, 0.15f ).lamps( 50 )
        .p( MOSQUE, 0.003f, 150, 350, 2.0f, 2.6f ).c( 0xF8F6F0, 0xF4F2EC ).v( 1 ).p( PALM, 0.05f, 4, 40 ).p( BROADLEAF, 0.08f, 4, 60 ).c( 0x5E9440 )
        .p( COW, 0.03f, 4, 40 ).v( 0 ).p( FLAT_HOUSE, 0.03f, 8, 60 ).c( 0xE8C890, 0xA8603A ).pastel();
    W( b, BE::MUMBAI, "MUMBAI", "INDIA", S::DUSK ).sea( 1, 0, -4 ).water( 0x3A5A70 ).land( 0x7A8A6A, 0x728262, 0x6A7A62 ).drive( 3, 1.3f, 0.97f, 1 )
        .bends( 0.35f, 0.3f, 0.1f ).lamps( 25 ).posts( 0 )
        .p( PALM, 0.2f, 2, 8 ).p( GLASS_TOWER, 0.06f, 20, 200 ).c( 0xA8B8C8 ).p( PANEL_BLOCK, 0.06f, 20, 150 ).c( 0xD8D0C0 ).pastel();
    W( b, BE::GOA, "GOA", "INDIA", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x2A8AA8 ).land( 0x7AA050, 0x6E9448, 0xB86A40 ).verge( 0xB86A40 )
        .drive( 2, 0.5f, 0.97f, 6 ).bends( 0.45f, 0.4f, 0.3f )
        .p( PALM, 0.35f, 2, 60 ).p( CHURCH, 0.006f, 20, 120 ).c( 0xF8F6F0, 0xF0EEE6 ).p( COTTAGE, 0.02f, 8, 60 ).c( 0xE8C890, 0xB8583A ).pastel()
        .p( BOAT, 0.02f, 40, 150 ).c( 0x3A8AC8 ).pastel().floating();
    W( b, BE::KERALA, "KERALA BACKWATERS", "INDIA", S::TROPICAL ).fog( 0.0016f, 0xC8DCD8 ).hills( 2, 1, 1, 0.5f ).sea( 0, 1, -2 )
        .water( 0x3A6A5A ).land( 0x5EA048, 0x549440, 0x4E8A44 ).drive( 2, 0.6f, 0.95f, 4 ).bends( 0.45f, 0.4f, 0.05f )
        .p( PALM, 0.45f, 2, 80 ).p( HOUSEBOAT, 0.02f, 40, 120 ).floating().p( BANANA, 0.1f, 3, 40 ).p( COTTAGE, 0.01f, 8, 60 ).c( 0xE0D8C0, 0xA8583A )
        .p( CHURCH, 0.003f, 20, 120 ).c( 0xF8F6F0, 0xE8E6E0 );
    W( b, BE::SRI_LANKA, "SOUTH COAST", "SRI LANKA", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x2A8AA8 ).land( 0x6AA44C, 0x609844, 0x5E8A48 )
        .drive( 2, 0.5f, 0.97f, 4 ).bends( 0.45f, 0.4f, 0.3f )
        .p( PALM, 0.35f, 2, 60 ).p( STUPA, 0.004f, 30, 150 ).c( 0xF8F6F2, 0xF0EEE6 ).v( 0 ).p( BOAT, 0.03f, 40, 150 ).c( 0x3A7AB8, 0xF0D040 ).pastel().floating()
        .p( BROADLEAF, 0.05f, 4, 60 ).c( 0x3E7A34 );
    W( b, BE::MALDIVES, "MALDIVES", "MALDIVES", S::TROPICAL ).sea( 1, 1, -2 ).water( 0x30C8D0 ).land( 0xF4EEDA, 0xEAE4D0, 0xE8E2CE ).verge( 0xF0EAD6 )
        .hills( 0, 0, 0 ).drive( 2, 0.1f, 0.95f, 3 ).bends( 0.35f, 0.3f, 0.05f ).bridges( 0.3f, BR::CAUSEWAY, 0xE0E0DC )
        .p( PALM, 0.3f, 1, 10 ).p( WATER_VILLA, 0.02f, 40, 150 ).floating();
    W( b, BE::DARJEELING, "DARJEELING", "INDIA", S::MIST ).land( 0x5EA046, 0x56943F, 0x5E8A4A ).ranges( 0x6A7A8A, 0xF2F4F8, 12 ).hills( 40, 14, -8 )
        .drive( 2, 0.4f, 0.95f, 4 ).bends( 0.85f, 0.75f, 0.85f )
        .p( TEA_ROW, 0.7f, 4, 12, 1, 1 ).p( TEA_ROW, 0.6f, 14, 30, 1, 1 ).p( TEA_ROW, 0.4f, 30, 60, 1, 1 ).p( SPRUCE, 0.06f, 40, 150 )
        .p( COTTAGE, 0.01f, 10, 80 ).c( 0xF0EAE0, 0xB8403A ).p( PRAYER_FLAGS, 0.01f, 4, 30 );
    W( b, BE::HIMALAYA, "HIMALAYA", "NEPAL", S::ALPINE ).land( 0x7A9A58, 0x6E8E50, 0x8A8E70 ).ranges( 0x6A707A, 0xF6F8FA, 18 ).road( 0x6E6A66 )
        .hills( 45, 16, 12 ).drive( 2, 0.2f, 0.9f, 3 ).bends( 0.85f, 0.8f, 0.9f ).bridges( 0.1f, BR::SUSPENSION, 0x8A8A8A )
        .p( PEAK, 0.006f, 250, 550, 1.6f, 2.2f ).c( 0x7A7A80, 0xF6F8FA ).p( STUPA, 0.006f, 15, 100 ).c( 0xF4F2EA, 0xD8A84A ).p( PRAYER_FLAGS, 0.03f, 4, 40 )
        .p( TERRACES, 0.015f, 30, 150 ).c( 0x8AAA4A, 0x8A7056 ).p( FLAT_HOUSE, 0.01f, 10, 80 ).c( 0xE8E0D0, 0x6A4A3A ).p( YAK, 0.02f, 10, 100 );
    W( b, BE::KATHMANDU_VALLEY, "KATHMANDU VALLEY", "NEPAL", S::HAZE ).land( 0x7AA04E, 0x6E9448, 0x8A9A60 ).ranges( 0x6A7480, 0xE8ECF2, 12 )
        .drive( 2, 0.9f, 0.95f, 6 ).bends( 0.55f, 0.45f, 0.5f )
        .p( PAGODA, 0.006f, 20, 120 ).p( STUPA, 0.004f, 30, 150 ).c( 0xF4F2EA, 0xD8A84A ).v( 0 ).p( TERRACES, 0.02f, 30, 150 ).c( 0x8AAA4A, 0x8A7056 )
        .p( ROW_HOUSES, 0.03f, 8, 50 ).c( 0xA8583A, 0x6A4A3A ).p( PRAYER_FLAGS, 0.01f, 4, 30 );
    W( b, BE::BHUTAN, "PARO VALLEY", "BHUTAN", S::MIST ).land( 0x5E9A46, 0x56903E, 0x5E8A4E ).ranges( 0x4A5A5A, 0xC8D0DC, 12 ).hills( 40, 12, 12 )
        .drive( 2, 0.2f, 0.96f, 3 ).bends( 0.85f, 0.7f, 0.8f ).bridges( 0.1f, BR::COVERED, 0x8A5A3A )
        .p( SPRUCE, 0.35f, 4, 150 ).p( FORT, 0.003f, 60, 200 ).c( 0xF4F2EA, 0x9A2A2A ).v( 1 ).p( PRAYER_FLAGS, 0.03f, 4, 40 )
        .p( FLAT_HOUSE, 0.01f, 10, 80 ).c( 0xF0EAE0, 0x7A4A30 );

    // ---- Southeast Asia ----------------------------------------------------------------------------------------------
    W( b, BE::BAGAN, "BAGAN", "MYANMAR", S::GOLDEN ).sun( 8, 30 ).fog( 0.0016f, 0xE0C8A0 ).land( 0xB8A070, 0xAC9468, 0x9A9A62 ).drive( 2, 0.3f, 0.97f, 6 )
        .bends( 0.35f, 0.3f, 0.15f )
        .p( STUPA, 0.04f, 10, 250, 0.6f, 1.4f ).c( 0xB8704A, 0xD8A84A ).v( 1 ).p( BALLOON, 0.03f, 0, 400 ).c( 0xE0A040 ).p( PALM, 0.05f, 6, 80 )
        .p( ACACIA, 0.03f, 8, 100 );
    W( b, BE::NORTHERN_THAILAND, "NORTHERN THAILAND", "THAILAND", S::MIST ).land( 0x6AA44C, 0x609844, 0x5E8A4E ).fields( 0.4f, 0x8AC04A, 0x6AA040, 0xA8A050 )
        .hills( 36, 10, 10 ).ranges( 0x5A6A6A, 0x9AA8AA, 8 ).drive( 2, 0.5f, 0.96f, 4 ).bends( 0.75f, 0.6f, 0.7f )
        .p( WAT, 0.004f, 40, 150 ).p( STUPA, 0.004f, 30, 150 ).c( 0xF4F2EA, 0xD8A84A ).v( 1 ).p( BAMBOO, 0.1f, 3, 40 ).p( BANANA, 0.08f, 3, 40 );
    W( b, BE::ANDAMAN_COAST, "ANDAMAN COAST", "THAILAND", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x20A8B8 ).land( 0x5E9A48, 0x56903E, 0xE8E0C8 )
        .hills( 30, 0, 20 ).drive( 2, 0.4f, 0.95f, 4 ).bends( 0.6f, 0.5f, 0.5f )
        .p( KARST, 0.01f, 40, 300, 0.6f, 1.0f ).c( 0xB8A890, 0x3E7A3A ).floating().p( KARST, 0.006f, 30, 200 ).c( 0xB8A890, 0x3E7A3A )
        .p( BOAT, 0.03f, 40, 150 ).c( 0x8A5A3A, 0xE8D040 ).pastel().floating().p( PALM, 0.25f, 2, 40 );
    W( b, BE::MEKONG, "MEKONG", "LAOS", S::HAZE ).sea( 1, 0, -6 ).water( 0x8A7A5A, 0.7f ).land( 0x6A9E48, 0x609240, 0x7A9A58 ).drive( 2, 0.3f, 0.95f, 4 )
        .bends( 0.5f, 0.4f, 0.4f )
        .p( STUPA, 0.006f, 20, 150 ).c( 0xD8A84A, 0xD8A84A ).v( 1 ).p( HUT, 0.02f, 6, 40 ).p( PALM, 0.1f, 3, 60 ).p( BANANA, 0.06f, 3, 40 )
        .p( BOAT, 0.02f, 40, 150 ).c( 0x6A4A30, 0xE8E0D0 ).floating().p( BROADLEAF, 0.05f, 6, 80 ).c( 0x3E7A34 );
    W( b, BE::ANGKOR, "ANGKOR", "CAMBODIA", S::GOLDEN ).hills( 3, 1, 1, 0.25f ).water( 0x5A7A6A ).land( 0x7AA04E, 0x6E9448, 0x6A8A4E )
        .drive( 2, 0.4f, 0.97f, 4 ).bends( 0.35f, 0.3f, 0.1f )
        .p( TEMPLE_TOWER, 0.006f, 40, 200, 0.8f, 1.2f ).c( 0x9A9080 ).v( 0 ).p( JUNGLE_TREE, 0.12f, 10, 120 ).p( PALM, 0.08f, 6, 80 );
    W( b, BE::SAPA, "SAPA TERRACES", "VIETNAM", S::MIST ).land( 0x7AAA48, 0x6E9E40, 0x6A8E4A ).hills( 50, 16, -10 ).ranges( 0x4A5A5A, 0x8A9A9A, 12 )
        .drive( 2, 0.3f, 0.93f, 4 ).bends( 0.85f, 0.75f, 0.9f )
        .p( TERRACES, 0.03f, 15, 150 ).c( 0x9AC04A, 0x8A6A4A ).v( 1 ).p( COTTAGE, 0.01f, 10, 80 ).c( 0x8A6A4A, 0x5A5A60 ).p( BAMBOO, 0.05f, 3, 40 )
        .p( COW, 0.01f, 10, 60 ).v( 2 );
    W( b, BE::HA_LONG_BAY, "HA LONG BAY", "VIETNAM", S::MIST ).sea( 1, 0, -3 ).water( 0x3A8A8A ).land( 0x5E8A48, 0x56803E, 0x6A8A58 )
        .drive( 2, 0.5f, 0.95f, 3 ).bends( 0.5f, 0.4f, 0.35f )
        .p( KARST, 0.03f, 40, 400, 0.6f, 1.2f ).c( 0xB0A894, 0x4A7A44 ).floating().p( SAILBOAT, 0.02f, 40, 200 ).c( 0xB8602A, 0x6A4A30 ).v( 2 ).floating()
        .p( KARST, 0.008f, 30, 200 ).c( 0xB0A894, 0x4A7A44 );
    W( b, BE::HAI_VAN_PASS, "HAI VAN PASS", "VIETNAM", S::HAZE ).sea( 1, 0, -40 ).water( 0x2A7AA0 ).hills( 40, 0, 30 ).land( 0x5E9A48, 0x569040, 0x5E8A4E )
        .drive( 2, 0.4f, 0.96f, 4 ).bends( 0.9f, 0.8f, 0.9f ).tunnels( 0.1f )
        .p( JUNGLE_TREE, 0.1f, 6, 80 ).p( BROADLEAF, 0.1f, 4, 60 ).c( 0x3E7A34 ).p( ROCK, 0.05f, 3, 40 );
    W( b, BE::BORNEO, "BORNEO", "MALAYSIA", S::HAZE ).rain( 0.2f ).land( 0x3E7A34, 0x387030, 0x2E6430 ).hills( 30, 10, 10 ).sea( 0, 1, -6 )
        .water( 0x7A6A4A, 0.7f ).drive( 2, 0.3f, 0.9f, 4 ).bends( 0.6f, 0.5f, 0.55f )
        .p( JUNGLE_TREE, 0.5f, 4, 120, 1.0f, 1.6f ).p( FERN, 0.3f, 1, 30 ).p( TREE_FERN, 0.1f, 3, 40 ).p( BANANA, 0.05f, 3, 40 ).p( HUT, 0.01f, 8, 40 );
    W( b, BE::SINGAPORE, "SINGAPORE", "SINGAPORE", S::TROPICAL ).land( 0x5E9A48, 0x569040, 0x6A8A5A ).drive( 3, 1.2f, 1.0f, 1 ).bends( 0.35f, 0.3f, 0.1f )
        .lamps( 30 ).posts( 0 ).bridges( 0.1f, BR::CABLE_STAYED, 0xE8E8E8 )
        .p( SUPERTREE, 0.015f, 20, 120 ).p( GLASS_TOWER, 0.1f, 30, 250 ).c( 0x6A9AB8 ).p( GLASS_TOWER, 0.05f, 30, 250 ).c( 0xD8D8D0 )
        .p( PANEL_BLOCK, 0.04f, 20, 150 ).c( 0xF0EEE6 ).pastel().v( 1 ).p( PALM, 0.15f, 2, 6 ).p( BROADLEAF, 0.2f, 2, 8 ).c( 0x3E8A34 );
    W( b, BE::BALI, "BALI", "INDONESIA", S::TROPICAL ).land( 0x6AAE4A, 0x60A244, 0x5E9A4E ).hills( 30, 8, 8 ).drive( 2, 0.8f, 0.95f, 6 )
        .bends( 0.6f, 0.5f, 0.55f )
        .p( TERRACES, 0.03f, 20, 150 ).c( 0x8AC04A, 0x7A5A40 ).v( 1 ).p( TEMPLE_GATE, 0.01f, 6, 60 ).p( PAGODA, 0.004f, 30, 150 ).p( PALM, 0.25f, 2, 60 )
        .p( BANANA, 0.06f, 3, 40 ).p( VOLCANO_CONE, 0.003f, 300, 600 ).c( 0x5A6060, 0x7A8070 ).p( FLOWER_BUSH, 0.05f, 2, 20 ).c( 0xF8F0F0 );
    W( b, BE::BROMO, "MOUNT BROMO", "INDONESIA", S::GOLDEN ).sun( 6, 30 ).fog( 0.002f, 0xB8B0A8 ).land( 0x7A7470, 0x6E6A66, 0x5A5A58 ).verge( 0x6A6662 )
        .ranges( 0x4A4A50, 0x7A7A80, 6 ).drive( 2, 0.2f, 0.93f, 3 ).bends( 0.5f, 0.45f, 0.5f )
        .p( VOLCANO_CONE, 0.008f, 150, 400, 0.7f, 1.0f ).c( 0x5A5450, 0x6E6862 ).p( GEYSER, 0.006f, 200, 400, 2.5f, 3.0f ).p( SHRUB, 0.05f, 3, 60 ).c( 0x8A9A6A )
        .p( HORSE, 0.02f, 10, 80 );
    W( b, BE::CHOCOLATE_HILLS, "CHOCOLATE HILLS", "PHILIPPINES", S::TROPICAL ).land( 0x8AA050, 0x7E9448, 0x9AA060 ).hills( 8, 2, 2 )
        .drive( 2, 0.3f, 0.97f, 4 ).bends( 0.5f, 0.45f, 0.4f )
        .p( HILLOCK, 0.05f, 20, 300, 0.7f, 1.3f ).c( 0x9A7A48 ).p( HILLOCK, 0.02f, 20, 300, 0.7f, 1.2f ).c( 0x7A9A44 ).p( PALM, 0.08f, 3, 60 ).p( BANANA, 0.04f, 3, 40 );

    // ---- East Asia ---------------------------------------------------------------------------------------------------
    W( b, BE::HONG_KONG, "HONG KONG", "CHINA", S::HAZE ).sea( 1, 0, -6 ).water( 0x3A6A80 ).hills( 50, 0, 20 ).land( 0x5E8A48, 0x568040, 0x6A8058 )
        .drive( 3, 1.3f, 0.97f, 1 ).bends( 0.55f, 0.45f, 0.4f ).lamps( 30 ).posts( 0 ).tunnels( 0.2f ).bridges( 0.15f, BR::SUSPENSION, 0xD0D4D8 )
        .p( GLASS_TOWER, 0.12f, 20, 200 ).c( 0x8AA0B8 ).p( PANEL_BLOCK, 0.08f, 20, 150 ).c( 0xE0DCD0 ).pastel().v( 1 ).p( BROADLEAF, 0.1f, 3, 40 ).c( 0x3E7A34 )
        .p( BOAT, 0.02f, 40, 160, 1.8f, 2.2f ).c( 0x2A6A3A, 0xF0F0F0 ).v( 1 ).floating().p( SAILBOAT, 0.005f, 60, 250 ).c( 0xB0402A, 0x6A4A30 ).v( 2 ).floating();
    W( b, BE::GUILIN, "GUILIN", "CHINA", S::MIST ).sea( 0, 1, -3 ).water( 0x4A8A7A ).land( 0x6AA44C, 0x609844, 0x5E8E4E )
        .fields( 0.3f, 0x8AC04A, 0x7AAA44, 0x9AB050 ).drive( 2, 0.4f, 0.95f, 3 ).bends( 0.5f, 0.4f, 0.3f )
        .p( KARST, 0.04f, 30, 400, 0.8f, 1.4f ).c( 0xB0AC9A, 0x4A8A44 ).p( KARST, 0.02f, 40, 300 ).c( 0xB0AC9A, 0x4A8A44 ).floating()
        .p( BAMBOO, 0.15f, 2, 30 ).p( BOAT, 0.02f, 40, 150 ).c( 0x6A5A3A, 0xD8C8A0 ).floating();
    W( b, BE::ZHANGJIAJIE, "ZHANGJIAJIE", "CHINA", S::MIST ).fog( 0.004f ).land( 0x4E8A44, 0x467E3E, 0x4A7A48 ).hills( 40, 20, -10 )
        .drive( 2, 0.3f, 0.95f, 3 ).bends( 0.8f, 0.7f, 0.8f ).bridges( 0.1f, BR::VIADUCT, 0xC8C4BC )
        .p( PILLAR, 0.05f, 30, 300, 0.8f, 1.5f ).p( PINE, 0.2f, 4, 80 );
    W( b, BE::BAMBOO_SEA, "BAMBOO SEA", "CHINA", S::MIST ).land( 0x5A9A44, 0x50903E, 0x4E8848 ).hills( 30, 8, 8 ).drive( 2, 0.3f, 0.96f, 3 )
        .bends( 0.6f, 0.5f, 0.6f )
        .p( BAMBOO, 0.6f, 2, 60 ).p( PANDA, 0.01f, 4, 30 ).p( COTTAGE, 0.005f, 10, 60 ).c( 0xF0EEE6, 0x3A3A40 ).p( TEA_ROW, 0.1f, 8, 40, 1, 1 );
    W( b, BE::TIBET, "TIBET", "CHINA", S::ALPINE ).sky( 0x1E5ACC, 0xA8C8E8 ).land( 0xA89A70, 0x9C8E68, 0x8A8A70 ).hills( 20, 6, 6, 0.15f )
        .water( 0x2A8AC8 ).ranges( 0x8A8070, 0xF4F6FA, 12 ).drive( 2, 0.15f, 0.95f, 3 ).bends( 0.45f, 0.4f, 0.4f )
        .p( YAK, 0.05f, 10, 150 ).p( PRAYER_FLAGS, 0.04f, 4, 40 ).p( STUPA, 0.006f, 20, 150 ).c( 0xF4F2EA, 0xD8A84A ).p( FORT, 0.003f, 100, 300 ).c( 0xF4F2EA, 0x8A2A2A ).v( 1 );
    W( b, BE::ANHUI_VILLAGES, "ANHUI VILLAGES", "CHINA", S::MIST ).land( 0x6AAA48, 0x60A042, 0x6A9A50 ).fields( 0.6f, 0xF0D830, 0x7AAA48, 0x8AB050 )
        .hills( 14, 4, 4, 0.2f ).water( 0x4A7A70 ).ranges( 0x5A6A6A, 0x8A9A9A, 9 ).drive( 2, 0.4f, 0.97f, 3 ).bends( 0.5f, 0.4f, 0.35f )
        .p( HUIZHOU_HOUSE, 0.04f, 10, 90 ).p( BAMBOO, 0.06f, 3, 40 );
    W( b, BE::SHANGHAI, "SHANGHAI", "CHINA", S::NIGHT ).sea( 0, 1, -6 ).water( 0x1A2A40 ).land( 0x2A3A30, 0x26342C, 0x22302A ).drive( 4, 1.4f, 1.0f, 1 )
        .bends( 0.35f, 0.3f, 0.15f ).lamps( 30 ).gantries( 500 ).rails().bridges( 0.2f, BR::CABLE_STAYED, 0xE0E4E8 )
        .p( TV_TOWER, 0.004f, 150, 350 ).v( 0 ).p( GLASS_TOWER, 0.15f, 20, 250 ).c( 0x4A6A8A ).p( SUPERTALL, 0.002f, 200, 400 );
    W( b, BE::GREAT_WALL, "GREAT WALL", "CHINA", S::MORNING ).land( 0x8A9A5A, 0x7E8E52, 0x8A8A62 ).hills( 50, 24, 24 ).drive( 2, 0.3f, 0.97f, 3 )
        .bends( 0.8f, 0.7f, 0.85f )
        .p( WATCHTOWER, 0.02f, 40, 200 ).c( 0x9A9488 ).v( 0 ).p( BROADLEAF, 0.2f, 3, 80 ).c( 0xC86A3A ).p( BROADLEAF, 0.1f, 3, 80 ).c( 0xD8A040 )
        .p( ROCK, 0.05f, 3, 60 );
    W( b, BE::GOBI, "GOBI DESERT", "MONGOLIA", S::DESERT ).land( 0xB8A078, 0xAC9470, 0xA89A78 ).hills( 10, 2, 2 ).drive( 2, 0.1f, 0.95f, 3 )
        .bends( 0.2f, 0.2f, 0.2f )
        .p( CAMEL, 0.04f, 10, 150 ).v( 1 ).p( YURT, 0.006f, 20, 150 ).p( DUNE, 0.01f, 30, 200 ).c( 0xD8B080 ).p( SHRUB, 0.1f, 3, 60, 0.4f, 0.7f ).c( 0x8A8A5A );
    W( b, BE::MONGOLIAN_STEPPE, "MONGOLIAN STEPPE", "MONGOLIA", S::MORNING ).land( 0x7AA850, 0x6E9C48, 0x8AA460 ).hills( 20, 3, 3 ).drive( 2, 0.15f, 0.98f, 3 )
        .bends( 0.3f, 0.3f, 0.3f )
        .p( YURT, 0.02f, 20, 200 ).p( HORSE, 0.08f, 10, 200 ).p( SHEEP, 0.04f, 10, 150 ).p( YAK, 0.02f, 10, 150 );
    W( b, BE::HANOK_VILLAGE, "HANOK VILLAGE", "SOUTH KOREA", S::GOLDEN ).land( 0x8A9A58, 0x7E8E50, 0x8A8A62 ).ranges( 0x6A7A70, 0x9AA8A8, 8 )
        .drive( 2, 0.5f, 0.98f, 1 ).bends( 0.5f, 0.4f, 0.4f )
        .p( HANOK, 0.04f, 10, 80 ).p( BROADLEAF, 0.12f, 3, 60 ).c( 0xD8A03A ).p( BROADLEAF, 0.06f, 3, 60 ).c( 0xC8402A ).p( STONE_WALL, 0.2f, 3, 3, 1, 1 )
        .p( PINE, 0.05f, 10, 100 );
    W( b, BE::JEJU, "JEJU", "SOUTH KOREA", S::MORNING ).sea( 1, 0, -4 ).water( 0x2A7A9A ).land( 0x6AA048, 0x609440, 0x4A4A46 )
        .fields( 0.5f, 0xF0D830, 0x6AA040, 0x3A3A38 ).drive( 2, 0.5f, 0.98f, 1 ).bends( 0.5f, 0.4f, 0.35f )
        .p( STONE_WALL, 0.4f, 3, 3, 1, 1 ).p( BROADLEAF, 0.1f, 4, 60, 0.5f, 0.7f ).c( 0x3E7A30 ).p( VOLCANO_CONE, 0.002f, 300, 600 ).c( 0x5A6A50, 0x6A7A58 )
        .p( COTTAGE, 0.01f, 10, 80 ).c( 0x4A4844, 0x8A7A5A );
    W( b, BE::MOUNT_FUJI, "MOUNT FUJI", "JAPAN", S::MORNING ).land( 0x6AA048, 0x609440, 0x76965A ).hills( 16, 4, 4 ).drive( 2, 0.6f, 1.0f, 0 )
        .bends( 0.5f, 0.4f, 0.4f ).bridges( 0.1f, BR::BEAM )
        .p( VOLCANO_CONE, 0.006f, 400, 700, 1.8f, 2.2f ).c( 0x5A6070, 0xF4F6FA ).p( TEA_ROW, 0.4f, 5, 30, 1, 1 ).p( COTTAGE, 0.01f, 10, 80 ).c( 0xE8E4DC, 0x3A3E48 )
        .p( TORII, 0.003f, 8, 30 ).p( PINE, 0.05f, 10, 100 );
    W( b, BE::KYOTO, "KYOTO", "JAPAN", S::GOLDEN ).land( 0x7A9A58, 0x6E8E50, 0x7A8A62 ).hills( 20, 6, 6 ).drive( 2, 0.6f, 0.98f, 0 ).bends( 0.5f, 0.4f, 0.35f )
        .p( BAMBOO, 0.2f, 2, 30 ).p( BROADLEAF, 0.12f, 3, 60 ).c( 0xD8402A ).p( PAGODA, 0.005f, 30, 150 ).p( TORII, 0.006f, 8, 30 ).p( LANTERN, 0.02f, 2, 8 )
        .p( COTTAGE, 0.02f, 8, 60 ).c( 0x8A6A4A, 0x3A3E48 );
    W( b, BE::SHIRAKAWA_GO, "SHIRAKAWA-GO", "JAPAN", S::WINTER ).windows( 0xFFD696 ).land( 0xE8ECF2, 0xDCE2EA, 0xD8DEE8 ).ranges( 0xB8C0CC, 0xD8DEE8, 9 )
        .drive( 2, 0.3f, 0.8f, 0 ).bends( 0.5f, 0.4f, 0.4f ).lamps( 50 )
        .p( GASSHO, 0.04f, 10, 80 ).p( SPRUCE, 0.2f, 6, 120 ).p( SNOWBANK, 0.25f, 0.5f, 3, 0.8f, 1.4f );
    W( b, BE::TOKYO, "TOKYO", "JAPAN", S::NIGHT ).land( 0x2A2A30, 0x26262C, 0x222228 ).drive( 3, 1.4f, 1.0f, 1 ).bends( 0.5f, 0.45f, 0.3f )
        .lamps( 25 ).gantries( 400 ).posts( 0 ).tunnels( 0.2f ).bridges( 0.15f, BR::SUSPENSION, 0xE8E8E8 )
        .p( GLASS_TOWER, 0.2f, 15, 150 ).c( 0x3A4A60 ).p( PANEL_BLOCK, 0.08f, 15, 120 ).c( 0xC8C4BC ).v( 1 )
        .p( LATTICE_TOWER, 0.002f, 150, 300, 1.6f, 2.0f ).c( 0xD83A2A, 0xF0F0F0 ).p( BILLBOARD, 0.03f, 6, 30 );
    W( b, BE::FURANO, "FURANO FIELDS", "JAPAN", S::MORNING ).land( 0x7AAE50, 0x70A048, 0x86A860 ).fields( 0.8f, 0x9A6AC8, 0xE84A6A, 0xF0D040 )
        .hills( 14, 3, 3 ).ranges( 0x6A7A8A, 0xC8D0DC, 8 ).drive( 2, 0.3f, 1.0f, 0 ).bends( 0.45f, 0.4f, 0.45f )
        .p( BIRCH, 0.04f, 8, 80 ).p( COTTAGE, 0.006f, 15, 90 ).c( 0xF0EEE6, 0x3A6A8A ).p( LUPIN, 0.1f, 2, 10 ).pastel();

    // ---- Oceania -----------------------------------------------------------------------------------------------------
    W( b, BE::DAINTREE, "DAINTREE", "AUSTRALIA", S::TROPICAL ).rain( 0.1f ).sea( 1, 0, -3 ).water( 0x2A8AA0 ).land( 0x3E7A34, 0x387030, 0x5E9A44 )
        .fields( 0.4f, 0x8ABA4A, 0x7AAA44, 0x6A9A40 ).drive( 2, 0.4f, 0.95f, 4 ).bends( 0.55f, 0.45f, 0.35f )
        .p( JUNGLE_TREE, 0.3f, 4, 100 ).p( TREE_FERN, 0.1f, 3, 40 ).p( PALM, 0.15f, 2, 40 ).p( BANANA, 0.04f, 3, 40 ).p( MANGROVE, 0.05f, 2, 12 );
    W( b, BE::RED_CENTRE, "RED CENTRE", "AUSTRALIA", S::DESERT ).land( 0xC8603A, 0xBC5834, 0xC87A4A ).verge( 0xB85A34 ).drive( 2, 0.15f, 0.95f, 7 )
        .bends( 0.2f, 0.2f, 0.15f )
        .p( MONOLITH, 0.003f, 250, 500, 0.9f, 1.2f ).p( SHRUB, 0.3f, 2, 60, 0.5f, 0.8f ).c( 0xB8A85A ).p( EUCALYPTUS, 0.03f, 8, 120 )
        .p( TERMITE_MOUND, 0.03f, 4, 80 ).p( KANGAROO, 0.03f, 8, 80 );
    W( b, BE::BLUE_MOUNTAINS, "BLUE MOUNTAINS", "AUSTRALIA", S::HAZE ).fog( 0.002f, 0x9AB4D0 ).land( 0x6A8A4E, 0x607E48, 0x5A7A50 ).hills( 40, 10, -20 )
        .ranges( 0x4A6080, 0x8AA4C8, 6 ).drive( 2, 0.4f, 0.97f, 6 ).bends( 0.75f, 0.6f, 0.7f )
        .p( EUCALYPTUS, 0.35f, 3, 150 ).p( ROCK, 0.05f, 3, 40 ).p( KANGAROO, 0.01f, 8, 60 );
    W( b, BE::GREAT_OCEAN_ROAD, "GREAT OCEAN ROAD", "AUSTRALIA", S::MORNING ).clouds( 0.5f ).sea( 1, 0, -30 ).water( 0x2A6A90 )
        .land( 0x7A9A50, 0x6E8E48, 0x8A9A5A ).hills( 20, 0, 10 ).drive( 2, 0.4f, 0.98f, 6 ).bends( 0.7f, 0.55f, 0.6f )
        .p( SEA_STACK, 0.02f, 40, 200, 1.0f, 1.6f ).c( 0xD8A870 ).floating().p( SHRUB, 0.3f, 2, 50 ).c( 0x6A8A50 ).p( EUCALYPTUS, 0.05f, 6, 80 );
    W( b, BE::MACKENZIE_BASIN, "MACKENZIE BASIN", "NEW ZEALAND", S::ALPINE ).land( 0xB8AE70, 0xB0A468, 0x9A9A68 ).hills( 14, 3, 3, 0.25f )
        .water( 0x3AB8D0 ).ranges( 0x6A7A8A, 0xF2F4F8, 13 ).drive( 2, 0.2f, 1.0f, 3 ).bends( 0.35f, 0.3f, 0.25f )
        .p( LUPIN, 0.3f, 2, 30 ).pastel().p( PEAK, 0.003f, 300, 600 ).c( 0x7A7A82, 0xF4F6FA ).p( SHEEP, 0.03f, 10, 100 );
    W( b, BE::ROTORUA, "ROTORUA", "NEW ZEALAND", S::MIST ).land( 0x6AA048, 0x609440, 0x7A9A5A ).hills( 20, 5, 5, 0.25f ).water( 0x7AA8A0 )
        .drive( 2, 0.4f, 0.97f, 6 ).bends( 0.5f, 0.4f, 0.4f )
        .p( GEYSER, 0.02f, 20, 200 ).p( TREE_FERN, 0.1f, 3, 40 ).p( REDWOOD, 0.03f, 20, 120 ).p( SHEEP, 0.03f, 10, 80 );
    W( b, BE::FIORDLAND, "FIORDLAND", "NEW ZEALAND", S::RAIN ).rain( 0.5f ).land( 0x3E7A3A, 0x387034, 0x4A6A48 ).hills( 50, 40, 40 )
        .ranges( 0x3A4A50, 0x6A7A84, 14 ).drive( 2, 0.2f, 0.9f, 3 ).bends( 0.7f, 0.6f, 0.6f ).tunnels( 0.2f )
        .p( TREE_FERN, 0.25f, 2, 40 ).p( FERN, 0.3f, 1, 20 ).p( BROADLEAF, 0.2f, 4, 100 ).c( 0x3E6A34 ).p( TEPUI, 0.002f, 200, 400, 0.6f, 0.8f ).c( 0x3A4A44, 0x3E6A3A );
    W( b, BE::BORA_BORA, "BORA BORA", "FRENCH POLYNESIA", S::TROPICAL ).sea( 1, 0, -2 ).water( 0x28C8D0 ).land( 0x5E9A48, 0x569040, 0xE8E0C8 )
        .hills( 20, 0, 12 ).drive( 2, 0.2f, 0.95f, 4 ).bends( 0.45f, 0.4f, 0.3f )
        .p( PEAK, 0.004f, 200, 400, 0.8f, 1.0f ).c( 0x3E6A3A, 0x4E7A44 ).p( WATER_VILLA, 0.03f, 40, 150 ).floating().p( PALM, 0.3f, 2, 40 );
    W( b, BE::NORTH_SHORE, "NORTH SHORE", "HAWAII", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x2A90B8 ).land( 0x5E9A48, 0x569040, 0xE8DCC0 )
        .hills( 30, 0, 30 ).ranges( 0x3E6A44, 0x6A9A6A, 10 ).drive( 2, 0.6f, 0.97f, 6 ).bends( 0.45f, 0.4f, 0.25f )
        .p( PALM, 0.25f, 2, 40 ).p( PARASOL, 0.02f, 2, 10 ).pastel().p( FLOWER_BUSH, 0.06f, 2, 20 ).c( 0xE84A6A );
    W( b, BE::EASTER_ISLAND, "EASTER ISLAND", "CHILE", S::MORNING ).clouds( 0.45f ).sea( 1, 0, -8 ).water( 0x1E5A8A ).land( 0x7AA050, 0x6E9448, 0x8A9A60 )
        .drive( 2, 0.1f, 0.97f, 6 ).bends( 0.4f, 0.35f, 0.35f )
        .p( MOAI, 0.02f, 10, 80, 0.9f, 1.3f ).p( ROCK, 0.1f, 3, 60 ).p( VOLCANO_CONE, 0.002f, 200, 400, 0.4f, 0.5f ).c( 0x6A8A50, 0x7A9A58 ).p( HORSE, 0.02f, 10, 80 );

    // ---- South America -----------------------------------------------------------------------------------------------
    W( b, BE::CARRETERA_AUSTRAL, "CARRETERA AUSTRAL", "CHILE", S::RAIN ).rain( 0.3f ).land( 0x4E8A44, 0x467E3E, 0x4A7A48 ).hills( 40, 14, 10, 0.2f )
        .water( 0x5AB0C0 ).ranges( 0x4A5A6A, 0xE8EEF4, 13 ).road( 0x7A7672 ).drive( 2, 0.15f, 0.88f, 3 ).bends( 0.7f, 0.6f, 0.6f )
        .p( BROADLEAF, 0.3f, 3, 120 ).c( 0x3E6A34 ).p( TREE_FERN, 0.08f, 3, 30 ).p( FLOWER_BUSH, 0.05f, 2, 20 ).c( 0xD8405A );
    W( b, BE::PATAGONIA, "PATAGONIA", "ARGENTINA", S::ALPINE ).clouds( 0.3f ).land( 0xB8A870, 0xAC9C68, 0xA09A70 ).hills( 14, 3, 3, 0.1f ).water( 0x4A90C0 )
        .drive( 2, 0.1f, 0.95f, 3 ).bends( 0.25f, 0.25f, 0.25f )
        .p( PEAK, 0.006f, 250, 500, 1.2f, 1.6f ).c( 0x8A8A92, 0xF4F6FA ).p( GRASS_TUFT, 0.3f, 1, 60 ).p( LLAMA, 0.03f, 10, 120 ).v( 0 )
        .p( SHRUB, 0.1f, 3, 60 ).c( 0x8A8A5A );
    W( b, BE::PAMPAS, "PAMPAS", "ARGENTINA", S::GOLDEN ).land( 0x8AAA50, 0x7E9E48, 0x9AA860 ).fields( 0.5f, 0xC8B060, 0x8AB04A, 0x6A9A40 ).hills( 3, 1, 1 )
        .drive( 2, 0.3f, 1.0f, 6 ).bends( 0.15f, 0.2f, 0.1f )
        .p( COW, 0.08f, 10, 150 ).v( 1 ).p( HORSE, 0.03f, 10, 120 ).p( COTTAGE, 0.006f, 20, 120 ).c( 0xF0EEE6, 0xA8583A )
        .p( BROADLEAF, 0.01f, 20, 150, 1.5f, 1.8f ).c( 0x4E8A3A ).p( EUCALYPTUS, 0.05f, 10, 150 ).p( WIND_PUMP, 0.01f, 15, 120 );
    W( b, BE::MENDOZA, "MENDOZA", "ARGENTINA", S::NOON ).land( 0xB8A07A, 0xAC9472, 0xA89A7A ).ranges( 0x8A7A80, 0xF2F4F8, 14 ).drive( 2, 0.4f, 1.0f, 6 )
        .bends( 0.35f, 0.3f, 0.25f )
        .p( VINE_ROW, 0.7f, 5, 40, 1, 1 ).p( POPLAR, 0.1f, 4, 80 ).p( ADOBE, 0.006f, 15, 100 ).c( 0xC8A880, 0x8A6A50 );
    W( b, BE::ATACAMA, "ATACAMA", "CHILE", S::DESERT ).sky( 0x1E5ACC, 0xD8C8B0 ).land( 0xB8987A, 0xAC8C72, 0xC8A888 ).hills( 14, 4, 4, 0.3f )
        .water( 0x7AC0D0 ).fields( 0.3f, 0xF0ECE0, 0xE0D8C8, 0xC8B898 ).drive( 2, 0.1f, 0.95f, 2 ).bends( 0.3f, 0.3f, 0.3f )
        .p( FLAMINGO, 0.05f, 2, 30 ).p( VOLCANO_CONE, 0.004f, 250, 500 ).c( 0x8A6A5A, 0xF0F2F6 ).p( SHRUB, 0.05f, 3, 60, 0.4f, 0.6f ).c( 0xC8B060 )
        .p( LLAMA, 0.01f, 15, 120 ).v( 0 );
    W( b, BE::ALTIPLANO, "ALTIPLANO", "BOLIVIA", S::ALPINE ).land( 0xA8A070, 0x9C9468, 0x9A8E70 ).hills( 14, 4, 4, 0.1f ).water( 0xC86A5A )
        .road( 0x6E6A66 ).drive( 2, 0.15f, 0.93f, 2 ).bends( 0.35f, 0.3f, 0.3f )
        .p( LLAMA, 0.06f, 10, 120 ).p( ADOBE, 0.015f, 10, 100 ).c( 0xB8987A, 0xC8B070 ).v( 1 ).p( VOLCANO_CONE, 0.004f, 250, 500 ).c( 0x7A6A60, 0xF2F4F6 )
        .p( SHRUB, 0.2f, 2, 60, 0.5f, 0.8f ).c( 0xC8B060 );
    W( b, BE::TITICACA, "LAKE TITICACA", "PERU", S::ALPINE ).sea( 0, 1, -4 ).water( 0x1E5AA8 ).land( 0x8AA050, 0x7E9448, 0x9A9A68 )
        .ranges( 0x6A7A8A, 0xF2F4F8, 10 ).drive( 2, 0.3f, 0.97f, 6 ).bends( 0.45f, 0.4f, 0.4f )
        .p( TERRACES, 0.015f, 30, 150 ).c( 0x8AAA4A, 0x8A7A60 ).p( REEDS, 0.2f, 2, 12 ).p( BOAT, 0.02f, 40, 150 ).c( 0xC8B070, 0xC8B070 ).floating()
        .p( ADOBE, 0.01f, 10, 80 ).c( 0xB8987A, 0xB8583A ).v( 1 ).p( LLAMA, 0.02f, 10, 100 );
    W( b, BE::SACRED_VALLEY, "SACRED VALLEY", "PERU", S::MORNING ).land( 0x7AA04E, 0x6E9448, 0x8A9A60 ).hills( 50, 22, 22 ).ranges( 0x5A6A5A, 0xE8EEF4, 12 )
        .drive( 2, 0.3f, 0.97f, 6 ).bends( 0.6f, 0.5f, 0.5f ).bridges( 0.1f, BR::ARCH, 0x9A9488 )
        .p( TERRACES, 0.04f, 20, 200 ).c( 0x8AAA4A, 0x8A7A60 ).v( 0 ).p( ADOBE, 0.02f, 10, 80 ).c( 0xB8987A, 0xB8583A ).v( 1 ).p( EUCALYPTUS, 0.08f, 6, 80 )
        .p( LLAMA, 0.03f, 10, 100 ).p( PEAK, 0.004f, 200, 400 ).c( 0x4E6A44, 0x5A7A4E );
    W( b, BE::VOLCANO_AVENUE, "AVENUE OF VOLCANOES", "ECUADOR", S::MORNING ).land( 0x7AA050, 0x6E9448, 0x8A9A60 ).fields( 0.4f, 0x8AB04A, 0xA8A050, 0x6A9A40 )
        .drive( 2, 0.4f, 0.98f, 6 ).bends( 0.45f, 0.4f, 0.45f )
        .p( VOLCANO_CONE, 0.008f, 300, 650, 1.4f, 1.9f ).c( 0x5A5A60, 0xF4F6FA ).p( EUCALYPTUS, 0.08f, 6, 80 ).p( COW, 0.03f, 10, 80 ).v( 1 )
        .p( LLAMA, 0.01f, 10, 80 ).p( ADOBE, 0.008f, 10, 80 ).c( 0xE0D8C8, 0xA8583A ).v( 1 );
    W( b, BE::COCORA_VALLEY, "COCORA VALLEY", "COLOMBIA", S::MIST ).land( 0x6AA44C, 0x609844, 0x5E8E4E ).hills( 40, 12, 12 ).drive( 2, 0.3f, 0.96f, 6 )
        .bends( 0.8f, 0.7f, 0.8f )
        .p( WAX_PALM, 0.08f, 6, 150, 0.9f, 1.3f ).p( COW, 0.03f, 10, 80 ).v( 1 ).p( COTTAGE, 0.015f, 8, 60 ).c( 0xF4F2EA, 0xA8583A ).pastel()
        .p( BANANA, 0.05f, 3, 40 ).p( TEA_ROW, 0.2f, 6, 30, 1, 1 );
    W( b, BE::GRAN_SABANA, "GRAN SABANA", "VENEZUELA", S::HAZE ).land( 0x8AA050, 0x7E9448, 0x9AA068 ).hills( 10, 3, 3, 0.1f ).water( 0x7A4A2A )
        .drive( 2, 0.15f, 0.97f, 4 ).bends( 0.35f, 0.3f, 0.3f )
        .p( TEPUI, 0.006f, 250, 600, 0.9f, 1.3f ).p( PALM, 0.04f, 6, 100 ).p( GRASS_TUFT, 0.2f, 1, 50 );
    W( b, BE::AMAZON_RIVER, "AMAZON RIVER", "BRAZIL", S::HAZE ).rain( 0.15f ).sea( 1, 0, -4 ).water( 0x9A8060, 0.7f ).land( 0x3E7A34, 0x387030, 0x2E6432 )
        .drive( 2, 0.3f, 0.9f, 4 ).bends( 0.45f, 0.4f, 0.1f )
        .p( JUNGLE_TREE, 0.4f, 4, 100 ).p( HUT, 0.03f, 0, 12 ).p( BOAT, 0.02f, 40, 160 ).c( 0x2A6A9A, 0xF0F0F0 ).floating().p( BANANA, 0.06f, 3, 40 )
        .p( PALM, 0.1f, 3, 60 );
    W( b, BE::PANTANAL, "PANTANAL", "BRAZIL", S::GOLDEN ).hills( 2, 1, 1, 0.55f ).water( 0x5A7A5A ).land( 0x7AA050, 0x6E9448, 0x8A9A60 )
        .drive( 2, 0.2f, 0.95f, 6 ).bends( 0.3f, 0.3f, 0.05f ).bridges( 0.3f, BR::BEAM, 0x7A5A3A )
        .p( PALM, 0.1f, 4, 100 ).p( BROADLEAF, 0.05f, 6, 100 ).c( 0xE89AC8 ).p( REEDS, 0.2f, 2, 30 ).p( COW, 0.04f, 10, 120 ).v( 1 ).p( FLAMINGO, 0.01f, 6, 60 );
    W( b, BE::LENCOIS, "LENCOIS MARANHENSES", "BRAZIL", S::TROPICAL ).land( 0xF2EEE2, 0xE8E4D8, 0xE6E0D2 ).hills( 10, 2, 2, 0.5f ).water( 0x2A98C8 )
        .verge( 0xEAE6DA ).road( 0x8A8A8E ).drive( 2, 0.05f, 0.9f, 4 ).bends( 0.35f, 0.3f, 0.35f )
        .p( DUNE, 0.05f, 20, 250, 1.5f, 2.5f ).c( 0xF4F0E6 );
    W( b, BE::RIO, "RIO DE JANEIRO", "BRAZIL", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x2A80A8 ).land( 0xE8DCC0, 0xDCD0B4, 0x4E8A44 ).verge( 0xE0D4B8 )
        .hills( 40, 0, 30 ).drive( 3, 1.1f, 1.0f, 6 ).bends( 0.45f, 0.4f, 0.3f ).lamps( 35 ).posts( 0 )
        .p( GRANITE_DOME, 0.006f, 150, 350, 0.8f, 1.1f ).v( 1 ).p( FAVELA, 0.04f, 20, 120 ).p( GLASS_TOWER, 0.03f, 30, 150 ).c( 0xE0E0D8 )
        .p( PALM, 0.25f, 2, 10 ).p( PARASOL, 0.04f, 2, 10 ).pastel()
        .landmark( Lm::OPEN_ARMS, 250, 350, 1.5f );

    // ---- Central America, the Caribbean, Mexico ------------------------------------------------------------------------
    W( b, BE::CLOUD_FOREST, "CLOUD FOREST", "COSTA RICA", S::MIST ).fog( 0.004f ).rain( 0.2f ).land( 0x3E8A3A, 0x388034, 0x3A7A40 ).hills( 36, 12, 12 )
        .drive( 2, 0.3f, 0.9f, 4 ).bends( 0.75f, 0.6f, 0.75f )
        .p( JUNGLE_TREE, 0.3f, 4, 100 ).p( TREE_FERN, 0.2f, 2, 40 ).p( FERN, 0.3f, 1, 20 ).p( BROADLEAF, 0.1f, 3, 60 ).c( 0x3E7A34 )
        .p( VOLCANO_CONE, 0.003f, 300, 600 ).c( 0x4A5A44, 0x5A6A50 ).p( BANANA, 0.05f, 3, 40 );
    W( b, BE::MAYAN_JUNGLE, "MAYAN JUNGLE", "GUATEMALA", S::HAZE ).land( 0x3E7A34, 0x387030, 0x2E6432 ).hills( 14, 4, 4, 0.1f ).drive( 2, 0.2f, 0.95f, 4 )
        .bends( 0.45f, 0.4f, 0.3f )
        .p( STEP_PYRAMID, 0.008f, 40, 200 ).c( 0xB0AC9C ).p( JUNGLE_TREE, 0.35f, 4, 120 ).p( PALM, 0.1f, 3, 60 ).p( FERN, 0.2f, 1, 20 );
    W( b, BE::VINALES, "VINALES", "CUBA", S::GOLDEN ).land( 0x6AA048, 0x609440, 0xB06A40 ).fields( 0.5f, 0xB0603A, 0x6AAA44, 0x8ABA50 ).verge( 0xA8603A )
        .drive( 2, 0.3f, 0.95f, 6 ).bends( 0.45f, 0.4f, 0.35f )
        .p( KARST, 0.02f, 60, 300, 0.6f, 0.9f ).c( 0x9A9A8A, 0x3E7A3A ).p( PALM, 0.1f, 4, 100 ).p( COTTAGE, 0.01f, 10, 80 ).c( 0xE8D8B8, 0x8A7A5A )
        .p( HORSE, 0.02f, 10, 80 );
    W( b, BE::JAMAICA, "JAMAICAN COAST", "JAMAICA", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x2AA0C0 ).land( 0x5E9A48, 0x569040, 0x4E8A44 )
        .ranges( 0x3E5A4A, 0x6A8A7A, 8 ).drive( 2, 0.5f, 0.95f, 6 ).bends( 0.55f, 0.45f, 0.4f )
        .p( PALM, 0.3f, 2, 60 ).p( BANANA, 0.1f, 3, 40 ).p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0xF0D8B0, 0x3A6A9A ).pastel()
        .p( BOAT, 0.02f, 40, 150 ).c( 0x3A8AC8 ).pastel().floating();
    W( b, BE::YUCATAN, "YUCATAN", "MEXICO", S::TROPICAL ).land( 0x6A9A48, 0x609040, 0x5E8A48 ).hills( 4, 1, 1, 0.08f ).water( 0x2AA0B0 )
        .drive( 2, 0.4f, 0.98f, 6 ).bends( 0.15f, 0.2f, 0.1f )
        .p( STEP_PYRAMID, 0.004f, 60, 250 ).p( BROADLEAF, 0.3f, 3, 120 ).c( 0x4E8A3A ).p( PALM, 0.05f, 4, 60 ).p( FLAT_HOUSE, 0.01f, 8, 60 ).c( 0xF0D8B0 ).pastel();
    W( b, BE::AGAVE_FIELDS, "AGAVE FIELDS", "MEXICO", S::NOON ).land( 0x9A8A60, 0x8E7E58, 0x9A9068 ).ranges( 0x6A7A8A, 0x9AA8B8, 7 ).drive( 2, 0.4f, 1.0f, 6 )
        .bends( 0.35f, 0.3f, 0.35f )
        .p( AGAVE, 0.9f, 4, 14, 0.9f, 1.2f ).p( AGAVE, 0.8f, 14, 40, 0.9f, 1.2f ).p( AGAVE, 0.5f, 40, 80, 0.9f, 1.2f )
        .p( ADOBE, 0.006f, 15, 100 ).c( 0xC89A70, 0xA8583A ).p( VOLCANO_CONE, 0.002f, 300, 600, 0.6f, 0.7f ).c( 0x5A6A5A, 0x6A7A68 );
    W( b, BE::MEXICAN_HIGHLANDS, "MEXICAN HIGHLANDS", "MEXICO", S::MORNING ).land( 0x8AA058, 0x7E9450, 0x9A9A68 ).drive( 2, 0.7f, 0.97f, 6 )
        .bends( 0.5f, 0.4f, 0.45f )
        .p( VOLCANO_CONE, 0.006f, 300, 650, 1.5f, 1.9f ).c( 0x6A6460, 0xF4F6FA ).p( CHURCH, 0.006f, 20, 150 ).c( 0xF0D8A8, 0xD8A050 )
        .p( FLAT_HOUSE, 0.04f, 6, 60 ).c( 0xF0D8B0, 0x3A6A9A ).pastel().p( AGAVE, 0.1f, 3, 40 ).p( PRICKLY_PEAR, 0.08f, 3, 40 );
    W( b, BE::BAJA, "BAJA CALIFORNIA", "MEXICO", S::DESERT ).sea( 1, 0, -10 ).water( 0x1E7AB0 ).land( 0xC8B08A, 0xBCA482, 0xA89A7A )
        .ranges( 0xA87A6A, 0xC8A894, 6 ).drive( 2, 0.2f, 0.95f, 7 ).bends( 0.5f, 0.4f, 0.4f )
        .p( CACTUS, 0.12f, 4, 100, 1.4f, 2.4f ).p( SHRUB, 0.1f, 3, 60 ).c( 0x8A8A5A ).p( ROCK, 0.06f, 3, 60 );

    // ---- The United States and Canada ------------------------------------------------------------------------------------
    W( b, BE::ROUTE_66, "ROUTE 66", "USA", S::GOLDEN ).land( 0xC8A47A, 0xBC9872, 0xB89A7A ).drive( 2, 0.4f, 1.0f, 7 ).bends( 0.15f, 0.2f, 0.2f )
        .p( DINER, 0.004f, 15, 40 ).p( MOTEL_SIGN, 0.006f, 8, 20 ).p( GAS_STATION, 0.003f, 15, 40 ).p( POLE, 0.15f, 6, 6, 1, 1 ).p( BILLBOARD, 0.006f, 8, 30 )
        .p( CACTUS, 0.02f, 6, 80 ).p( SHRUB, 0.15f, 2, 60 ).c( 0x9A9A6A ).p( MESA, 0.006f, 100, 300 ).p( WATER_TOWER, 0.003f, 40, 120 ).c( 0xD8D8D8 );
    W( b, BE::LAS_VEGAS, "LAS VEGAS", "USA", S::NIGHT ).land( 0x3A3028, 0x362C24, 0x302820 ).road( 0x3A3A40 ).drive( 4, 1.4f, 1.0f, 1 )
        .bends( 0.2f, 0.2f, 0.1f ).lamps( 30 ).posts( 0 )
        .p( CASINO, 0.08f, 20, 120 ).p( GLASS_TOWER, 0.04f, 30, 200 ).c( 0xC8A060 ).p( PALM, 0.2f, 2, 6 ).p( MOTEL_SIGN, 0.03f, 8, 16 ).p( BILLBOARD, 0.03f, 6, 30 );
    W( b, BE::DEATH_VALLEY, "DEATH VALLEY", "USA", S::DESERT ).fog( 0.0012f, 0xE8D8C0 ).land( 0xD8C8A8, 0xCCBC9E, 0xC0A888 ).fields( 0.3f, 0xF2F0E8, 0xE8E4D8, 0xD8CCB8 )
        .shine( 0.2f ).ranges( 0x8A6A5A, 0xB09080, 7 ).drive( 2, 0.15f, 0.97f, 7 ).bends( 0.25f, 0.25f, 0.25f )
        .p( DUNE, 0.015f, 30, 200 ).c( 0xE0C8A0 ).p( ROCK, 0.08f, 3, 60 ).p( SHRUB, 0.05f, 3, 60, 0.4f, 0.6f ).c( 0x9A9A70 );
    W( b, BE::JOSHUA_TREE, "JOSHUA TREE", "USA", S::GOLDEN ).land( 0xC8B08A, 0xBCA482, 0xB8A888 ).drive( 2, 0.2f, 1.0f, 7 ).bends( 0.35f, 0.3f, 0.3f )
        .p( JOSHUA_TREE, 0.12f, 5, 150 ).p( KOPJE, 0.015f, 20, 200 ).p( SHRUB, 0.1f, 2, 60 ).c( 0x9A9A70 ).p( ROCK, 0.06f, 3, 60 );
    W( b, BE::BIG_SUR, "BIG SUR", "USA", S::MORNING ).fog( 0.0018f ).sea( 1, 0, -40 ).water( 0x1E5A8A ).hills( 30, 0, 30 ).land( 0x8A9A58, 0x7E8E50, 0x6A8A50 )
        .drive( 2, 0.5f, 0.98f, 6 ).bends( 0.8f, 0.65f, 0.7f ).bridges( 0.15f, BR::ARCH, 0xD8D4CC )
        .p( REDWOOD, 0.03f, 20, 80 ).p( SHRUB, 0.3f, 2, 60 ).c( 0x7A8A58 ).p( BROADLEAF, 0.04f, 4, 60 ).c( 0x4E7A3A ).p( SEA_STACK, 0.01f, 40, 200 ).c( 0x8A8076 ).floating();
    W( b, BE::YOSEMITE, "YOSEMITE", "USA", S::ALPINE ).land( 0x6A9A50, 0x608E48, 0x7A9060 ).hills( 30, 14, 14, 0.1f ).ranges( 0x8A8A8A, 0xD8D8D8, 10 )
        .drive( 2, 0.4f, 0.98f, 7 ).bends( 0.6f, 0.5f, 0.6f ).tunnels( 0.1f )
        .p( GRANITE_DOME, 0.008f, 150, 400, 0.8f, 1.2f ).v( 0 ).p( PINE, 0.3f, 4, 150 ).p( SPRUCE, 0.1f, 6, 150 ).p( ROCK, 0.05f, 3, 50 );
    W( b, BE::REDWOODS, "REDWOODS", "USA", S::MIST ).rain( 0.05f ).land( 0x4E7A3E, 0x467036, 0x3E6A3A ).drive( 2, 0.3f, 0.95f, 6 ).bends( 0.6f, 0.45f, 0.4f )
        .p( REDWOOD, 0.4f, 4, 80, 0.9f, 1.3f ).p( FERN, 0.4f, 1, 30 );
    W( b, BE::CASCADES, "CASCADES", "USA", S::RAIN ).rain( 0.2f ).land( 0x4E8A44, 0x467E3E, 0x4A7A48 ).hills( 36, 12, 12, 0.1f )
        .drive( 2, 0.4f, 0.92f, 6 ).bends( 0.6f, 0.5f, 0.6f ).tunnels( 0.1f ).bridges( 0.15f, BR::TRUSS, 0x4A6A5A )
        .p( SPRUCE, 0.55f, 3, 150 ).p( VOLCANO_CONE, 0.004f, 350, 650, 1.6f, 2.0f ).c( 0x5A6070, 0xF4F6FA );
    W( b, BE::YELLOWSTONE, "YELLOWSTONE", "USA", S::MORNING ).land( 0x8A9A60, 0x7E8E58, 0x8A8E68 ).hills( 20, 4, 4, 0.15f ).water( 0x2A9AB8 )
        .fields( 0.15f, 0xE8A040, 0x3AA8C8, 0xE8D8A0 ).drive( 2, 0.4f, 0.98f, 6 ).bends( 0.45f, 0.4f, 0.35f )
        .p( GEYSER, 0.015f, 30, 250 ).p( SPRUCE, 0.3f, 10, 150 ).p( BISON, 0.05f, 10, 120 ).p( DEAD_TREE, 0.05f, 6, 80 );
    W( b, BE::GRAND_CANYON, "GRAND CANYON", "USA", S::GOLDEN ).land( 0xC89A6A, 0xBC8E62, 0xB0866A ).hills( 14, -50, 4 ).ranges( 0xB8604A, 0xD8906A, 8 )
        .drive( 2, 0.3f, 1.0f, 7 ).bends( 0.45f, 0.4f, 0.35f )
        .p( BADLAND, 0.01f, 100, 300 ).c( 0xD8A070, 0xB8603A ).p( PINE, 0.08f, 4, 60, 0.5f, 0.8f ).p( SHRUB, 0.1f, 2, 60 ).c( 0x8A9A6A ).p( BUTTE, 0.004f, 150, 400 );
    W( b, BE::ARCHES, "ARCHES", "USA", S::NOON ).land( 0xD8865A, 0xCC7C52, 0xD0906A ).verge( 0xC8805A ).drive( 2, 0.3f, 1.0f, 7 ).bends( 0.45f, 0.4f, 0.45f )
        .p( ARCH, 0.015f, 40, 250, 0.8f, 1.3f ).p( BUTTE, 0.006f, 80, 300 ).p( MESA, 0.004f, 100, 300 ).p( ROCK, 0.08f, 3, 60 ).p( SHRUB, 0.08f, 2, 60 ).c( 0x8A9A6A )
        .p( PINE, 0.03f, 4, 60, 0.4f, 0.6f );
    W( b, BE::BADLANDS, "BADLANDS", "USA", S::GOLDEN ).land( 0xC8B898, 0xBCAC8E, 0xB8A888 ).drive( 2, 0.2f, 1.0f, 7 ).bends( 0.4f, 0.35f, 0.4f )
        .p( BADLAND, 0.04f, 20, 250, 0.7f, 1.5f ).p( GRASS_TUFT, 0.2f, 1, 50 ).p( BISON, 0.02f, 30, 150 );
    W( b, BE::WEST_TEXAS, "WEST TEXAS", "USA", S::GOLDEN ).land( 0xB8A878, 0xAC9C70, 0xA89A78 ).drive( 2, 0.4f, 1.0f, 7 ).bends( 0.1f, 0.15f, 0.15f )
        .p( PUMPJACK, 0.03f, 15, 150 ).p( WIND_PUMP, 0.008f, 15, 120 ).p( COW, 0.03f, 10, 120 ).v( 1 ).p( SHRUB, 0.15f, 2, 60 ).c( 0x8A9A60 )
        .p( WINDMILL, 0.01f, 60, 300 ).p( FENCE, 0.4f, 6, 6, 1, 1 ).p( WATER_TOWER, 0.002f, 40, 150 ).c( 0xD8D8D8 );
    W( b, BE::FLORIDA_KEYS, "FLORIDA KEYS", "USA", S::TROPICAL ).sea( 1, 1, -2 ).water( 0x30C0C8 ).land( 0xE8E0C8, 0xDCD4BC, 0xDCD4BC ).verge( 0xE0D8C4 )
        .hills( 0, 0, 0 ).drive( 2, 0.5f, 1.0f, 6 ).bends( 0.15f, 0.2f, 0.05f ).bridges( 0.5f, BR::CAUSEWAY, 0xE0E0DA )
        .p( PALM, 0.2f, 1, 8 ).p( MANGROVE, 0.05f, 1, 10 ).p( BOAT, 0.02f, 40, 150 ).c( 0x3A8AC8 ).pastel().floating().p( SAILBOAT, 0.01f, 60, 250 ).floating();
    W( b, BE::MIAMI, "MIAMI BEACH", "USA", S::TROPICAL ).sea( 1, 0, -2 ).water( 0x2AB0C8 ).land( 0xE8DCC0, 0xDCD0B4, 0x6A9A5A ).drive( 3, 1.1f, 1.0f, 0 )
        .bends( 0.25f, 0.25f, 0.05f ).lamps( 30 ).posts( 0 )
        .p( FLAT_HOUSE, 0.08f, 3, 20 ).c( 0xF0D8B0, 0xF0F0F0 ).pastel().v( 1 ).p( PALM, 0.3f, 2, 10 ).p( GLASS_TOWER, 0.05f, 30, 150 ).c( 0x9AC0D8 )
        .p( PARASOL, 0.04f, 2, 10 ).pastel();
    W( b, BE::SMOKY_MOUNTAINS, "SMOKY MOUNTAINS", "USA", S::HAZE ).fog( 0.0022f, 0x9AB0C8 ).land( 0x5E8A44, 0x56803E, 0x507A48 ).hills( 36, 10, 10 )
        .ranges( 0x4A6080, 0x8AA0C0, 8 ).drive( 2, 0.4f, 0.97f, 6 ).bends( 0.75f, 0.6f, 0.7f )
        .p( BROADLEAF, 0.3f, 3, 150 ).c( 0x4E8A3A ).p( SPRUCE, 0.1f, 10, 150 ).p( COTTAGE, 0.01f, 10, 80 ).c( 0x8A6A4A, 0x4A4A50 );
    W( b, BE::NEW_YORK, "NEW YORK", "USA", S::MORNING ).land( 0x6A7A6A, 0x627262, 0x5A6A5A ).drive( 4, 1.5f, 0.98f, 1 ).bends( 0.3f, 0.3f, 0.15f )
        .lamps( 30 ).posts( 0 ).tunnels( 0.15f ).bridges( 0.25f, BR::SUSPENSION, 0x8A9AA8 )
        .p( GLASS_TOWER, 0.2f, 10, 200 ).c( 0x7A8A9A ).p( GLASS_TOWER, 0.1f, 10, 200 ).c( 0xC8B8A0 ).p( ROW_HOUSES, 0.06f, 6, 30 ).c( 0xA86A4A, 0x4A4A50 )
        .p( SUPERTALL, 0.002f, 200, 400 )
        .landmark( Lm::LIBERTY, 120, 200 );
    W( b, BE::MAINE_COAST, "MAINE COAST", "USA", S::MORNING ).sea( 1, 0, -6 ).water( 0x1E4A6A ).land( 0x5E8A48, 0x567E42, 0x4E7A48 )
        .drive( 2, 0.4f, 0.98f, 6 ).bends( 0.6f, 0.5f, 0.45f ).bridges( 0.1f, BR::TRUSS, 0x4A6A5A )
        .p( LIGHTHOUSE, 0.006f, 20, 100 ).p( SPRUCE, 0.4f, 4, 120 ).p( ROCK, 0.15f, 2, 40 ).p( COTTAGE, 0.015f, 8, 60 ).c( 0xF2F0EA, 0x4A4A50 )
        .p( COTTAGE, 0.01f, 8, 60 ).c( 0xA83A30, 0x4A4A50 ).p( BOAT, 0.03f, 40, 150 ).c( 0xF0F0F0, 0x2A4A6A ).v( 1 ).floating();
    W( b, BE::NEWFOUNDLAND, "NEWFOUNDLAND", "CANADA", S::OVERCAST ).sea( 1, 0, -10 ).water( 0x2A4A6A ).land( 0x7A8A58, 0x6E7E50, 0x6A7A58 )
        .drive( 2, 0.3f, 0.95f, 6 ).bends( 0.55f, 0.45f, 0.5f )
        .p( ROW_HOUSES, 0.04f, 6, 40 ).c( 0xD84A3A, 0x3A3A40 ).pastel().v( 0 ).p( ROCK, 0.15f, 2, 60 ).p( SPRUCE, 0.15f, 6, 100, 0.5f, 0.7f )
        .p( ICEBERG, 0.006f, 50, 300 ).floating().p( BOAT, 0.02f, 40, 150 ).c( 0x3A8AC8 ).pastel().floating();
    W( b, BE::PRAIRIES, "PRAIRIES", "CANADA", S::MORNING ).land( 0x8AAA50, 0x7E9E48, 0x9AAA60 ).fields( 0.9f, 0xF0D830, 0x8AB04A, 0xC8B060 ).hills( 3, 1, 1 )
        .drive( 2, 0.4f, 1.0f, 3 ).bends( 0.1f, 0.15f, 0.1f )
        .p( GRAIN_ELEVATOR, 0.004f, 40, 150 ).c( 0xA83A30, 0x3A3A40 ).p( SILO, 0.01f, 30, 150 ).p( BARN, 0.005f, 30, 150 ).p( COTTAGE, 0.004f, 30, 150 ).c( 0xF2F0EA, 0x4A4A50 );
    W( b, BE::BANFF, "BANFF", "CANADA", S::ALPINE ).land( 0x5E9046, 0x56863E, 0x5E8050 ).hills( 40, 14, 14, 0.25f ).water( 0x2AB8C8 )
        .ranges( 0x6A7A8A, 0xF2F4F8, 15 ).drive( 2, 0.4f, 0.98f, 3 ).bends( 0.5f, 0.45f, 0.5f ).bridges( 0.1f, BR::ARCH, 0x8A8A80 )
        .p( SPRUCE, 0.5f, 3, 150 ).p( PEAK, 0.005f, 250, 550, 1.2f, 1.6f ).c( 0x8A8A90, 0xF4F6FA ).p( MOOSE, 0.01f, 10, 80 );
    W( b, BE::DENALI, "DENALI", "ALASKA", S::ARCTIC ).land( 0x8A9A58, 0x7E8E50, 0xA08A5A ).fields( 0.3f, 0xB8603A, 0xC8A040, 0x7A8A4A ).ranges( 0xB8C0CC, 0xE8ECF4, 12 )
        .drive( 2, 0.15f, 0.97f, 3 ).bends( 0.4f, 0.35f, 0.35f )
        .p( PEAK, 0.004f, 400, 700, 2.0f, 2.4f ).c( 0xE8ECF2, 0xF8F8FA ).p( SPRUCE, 0.15f, 4, 150, 0.5f, 0.8f ).p( MOOSE, 0.02f, 10, 100 );

    // ---- Russia, the Baltic, Scandinavia, Iceland ----------------------------------------------------------------------
    W( b, BE::KAMCHATKA, "KAMCHATKA", "RUSSIA", S::MIST ).land( 0x6A9A50, 0x608E48, 0x6A8458 ).road( 0x6E6E72 ).drive( 2, 0.1f, 0.9f, 3 )
        .bends( 0.5f, 0.45f, 0.5f )
        .p( VOLCANO_CONE, 0.008f, 300, 600, 1.3f, 1.7f ).c( 0x4A4A50, 0xF2F4F8 ).p( GEYSER, 0.006f, 30, 200 ).p( BIRCH, 0.1f, 4, 80 )
        .p( SHRUB, 0.2f, 2, 60 ).c( 0x6A8A50 );
    W( b, BE::LAKE_BAIKAL, "LAKE BAIKAL", "RUSSIA", S::ARCTIC ).sea( 1, 0, -1.5f ).water( 0x8AC8E0, 0.98f ).land( 0xE8ECF2, 0xDCE2EA, 0xD8DEE8 )
        .ranges( 0x8A9AAA, 0xC8D4E0, 8 ).drive( 2, 0.15f, 0.78f, 3 ).bends( 0.4f, 0.35f, 0.3f )
        .p( SNOW_PINE, 0.2f, 4, 120 ).p( ROCK, 0.05f, 3, 40 );
    W( b, BE::SIBERIA, "SIBERIA", "RUSSIA", S::WINTER ).windows( 0xFFD696 ).land( 0xE8ECF2, 0xDCE2EA, 0xD0D8E4 ).drive( 2, 0.2f, 0.8f, 3 )
        .bends( 0.3f, 0.3f, 0.3f )
        .p( SNOW_PINE, 0.45f, 4, 200 ).p( BIRCH, 0.1f, 4, 80 ).p( COTTAGE, 0.006f, 15, 80 ).c( 0x6A4A3A, 0x8A8A90 ).p( SNOWBANK, 0.25f, 0.5f, 3, 0.8f, 1.4f );
    W( b, BE::GOLDEN_RING, "GOLDEN RING", "RUSSIA", S::MORNING ).land( 0x7AA04E, 0x6E9448, 0x8A9A5E ).hills( 12, 3, 3, 0.1f ).drive( 2, 0.4f, 0.98f, 3 )
        .bends( 0.4f, 0.35f, 0.35f )
        .p( ONION_CHURCH, 0.008f, 30, 200 ).c( 0xF4F2EA, 0xD8A84A ).p( ONION_CHURCH, 0.004f, 30, 200 ).c( 0xF4F2EA, 0x3A6AB8 )
        .p( COTTAGE, 0.025f, 8, 70 ).c( 0x8A6A4A, 0x5A7A5A ).p( BIRCH, 0.2f, 4, 100 ).p( FENCE, 0.2f, 4, 4, 1, 1 );
    W( b, BE::CURONIAN_SPIT, "CURONIAN SPIT", "LITHUANIA", S::MORNING ).sea( 1, 0, -3 ).water( 0x3A6A8A ).land( 0xE0D8B8, 0xD4CCAC, 0x5E8A48 )
        .verge( 0xD8D0B0 ).drive( 2, 0.3f, 1.0f, 3 ).bends( 0.35f, 0.3f, 0.3f )
        .p( DUNE, 0.02f, 30, 200 ).c( 0xE8DCB8 ).p( PINE, 0.25f, 4, 80 ).p( COTTAGE, 0.01f, 10, 60 ).c( 0xA8583A, 0x3A3A40 );
    W( b, BE::FINNISH_LAKELAND, "FINNISH LAKELAND", "FINLAND", S::GOLDEN ).sun( 6, -60 ).hills( 12, 3, 3, 0.5f ).water( 0x2A4A6A )
        .land( 0x5E9046, 0x56863E, 0x4E7A46 ).drive( 2, 0.25f, 1.0f, 3 ).bends( 0.5f, 0.4f, 0.4f )
        .p( SPRUCE, 0.3f, 4, 150 ).p( BIRCH, 0.2f, 3, 80 ).p( COTTAGE, 0.015f, 8, 60 ).c( 0xA83A2A, 0x3A3A40 );
    W( b, BE::LOFOTEN, "LOFOTEN", "NORWAY", S::ALPINE ).sun( 8, 0 ).sea( 1, 0, -4 ).water( 0x1E5A7A ).hills( 40, 0, 40 ).ranges( 0x4A5664, 0xC8D0DC, 14 )
        .land( 0x6A9A50, 0x608E48, 0x5E8050 ).drive( 2, 0.3f, 0.97f, 3 ).bends( 0.6f, 0.5f, 0.4f ).bridges( 0.2f, BR::CABLE_STAYED, 0xE8E8E8 )
        .p( PEAK, 0.008f, 150, 400, 0.8f, 1.2f ).c( 0x5A626A, 0xE8ECF2 ).p( CABIN, 0.04f, 0, 12 ).p( BOAT, 0.03f, 40, 150 ).c( 0xC83A2A, 0xF0F0F0 ).floating();
    W( b, BE::SMALAND, "SMALAND", "SWEDEN", S::MORNING ).land( 0x5E9A46, 0x56903E, 0x4E8048 ).hills( 12, 3, 3, 0.2f ).water( 0x3A5A7A )
        .drive( 2, 0.3f, 1.0f, 3 ).bends( 0.5f, 0.4f, 0.4f )
        .p( COTTAGE, 0.025f, 6, 80 ).c( 0x9A2A22, 0x3A3A40 ).p( SPRUCE, 0.3f, 4, 150 ).p( BIRCH, 0.1f, 3, 80 ).p( STONE_WALL, 0.2f, 3, 3, 1, 1 ).p( MOOSE, 0.01f, 10, 80 );
    W( b, BE::JUTLAND, "JUTLAND", "DENMARK", S::OVERCAST ).clouds( 0.7f ).land( 0x7AAA50, 0x6E9E48, 0x86A05E ).fields( 0.5f, 0xF0D830, 0x8AB04A, 0xC8B060 )
        .hills( 8, 2, 2 ).drive( 2, 0.5f, 1.0f, 3 ).bends( 0.35f, 0.3f, 0.25f ).bridges( 0.15f, BR::SUSPENSION, 0xD8DCE0 )
        .p( WINDMILL, 0.03f, 40, 300 ).p( COTTAGE, 0.015f, 10, 90 ).c( 0xF2F0EA, 0x8A7A5A ).p( HEDGE, 0.2f, 3, 3, 1, 1 ).p( CHURCH, 0.004f, 30, 150 ).c( 0xF4F2EA, 0xA8583A );
    W( b, BE::RING_ROAD, "RING ROAD", "ICELAND", S::OVERCAST ).land( 0x6E8A50, 0x647E4A, 0x4A4A48 ).verge( 0x3A3A38 ).ranges( 0x4A5058, 0xE8ECF2, 10 )
        .drive( 2, 0.2f, 0.92f, 3 ).bends( 0.45f, 0.4f, 0.35f ).bridges( 0.1f, BR::TRUSS, 0x6A6A6E )
        .p( ROCK, 0.15f, 2, 60 ).p( GEYSER, 0.004f, 30, 250 ).p( CHURCH, 0.003f, 30, 150 ).c( 0xF4F4F2, 0xC83A2A ).p( COTTAGE, 0.006f, 15, 90 ).c( 0xF2F0EA, 0x3A6A3A )
        .p( HORSE, 0.03f, 10, 100 ).p( SHEEP, 0.02f, 10, 80 ).p( TEPUI, 0.002f, 250, 450, 0.6f, 0.8f ).c( 0x4A4A50, 0x5E7A48 );

    addWorldPlaces( b );

    // AUTO radio: the countryside plays its region's station; cities and motorways keep their genre (Japan's and
    // Korea's turn to city pop).
    for( int i = 0; i < static_cast<int>( BiomeE::COUNT ); ++i )
    {
        BiomeDef &d = b[i];
        if( !d.m_country ) continue;
        const bool japanOrKorea = std::strcmp( d.m_country, "JAPAN" ) == 0 || std::strcmp( d.m_country, "SOUTH KOREA" ) == 0;
        const int  region = regionStation( d.m_country );
        if( d.m_lanes >= 3 ) d.m_station = japanOrKorea ? StationCityPop : d.m_station;
        else if( region >= 0 ) d.m_station = region;
    }
    b[static_cast<size_t>( BE::BRITTANY )].m_station = StationCeltic;
    b[static_cast<size_t>( BE::RIO )].m_station = StationSamba;
}

std::string placeName( BiomeE biome )
{
    const BiomeDef &d = biomeDef( biome );
    std::string     s = d.m_name;
    if( d.m_country && s != d.m_country ) s += std::string( "  -  " ) + d.m_country;
    return s;
}

int countryCount()
{
    static const int n = []
    {
        std::vector<std::string> seen;
        for( int i = 0; i < static_cast<int>( BiomeE::COUNT ); ++i )
        {
            const char *c = biomeDef( static_cast<BiomeE>( i ) ).m_country;
            if( c && std::find( seen.begin(), seen.end(), c ) == seen.end() ) seen.push_back( c );
        }
        return static_cast<int>( seen.size() );
    }();
    return n;
}

const std::vector<BiomeE> &worldRoute()
{
    using enum BiomeE;
    static const std::vector<BiomeE> route = {
        RAINBOW, CONNEMARA, WILD_ATLANTIC, CAUSEWAY_COAST, HIGHLANDS, GLENCOE, APPLECROSS, ISLE_OF_SKYE, SNOWDONIA, LAKE_DISTRICT,
        COTSWOLDS, CORNWALL, STONEHENGE, LONDON, CLIFFS, BRITTANY, MONT_SAINT_MICHEL, NORMANDY, PARIS, LOIRE_VALLEY, CHAMPAGNE,
        LUXEMBOURG, FLANDERS, TULIPS, AMSTERDAM, HARBOR, NORDSCHLEIFE, RHINE_VALLEY, MEADOW, BERLIN, ROMANTIC_ROAD, BLACK_FOREST,
        AUTOBAHN, BAVARIA, LIECHTENSTEIN, SWISS_ALPS, FURKA_PASS, FOREST, LAVAUX, CHAMONIX, ALPE_D_HUEZ, SALZKAMMERGUT, GROSSGLOCKNER,
        WINTER_VILLAGE, DOLOMITES, STELVIO, LAKE_COMO, VENICE, CINQUE_TERRE, PISA, TUSCANY, SAN_MARINO, ROME, VATICAN, VIA_APPIA,
        AMALFI, ALBEROBELLO, SICILY, MALTA, SARDINIA, CORSICA, COTE_D_AZUR, MONACO, COL_DE_TURINI, VERDON, LAVENDER, CAMARGUE,
        DUNE_OF_PILAT, BASQUE_COAST, PICOS, RIOJA, ANDORRA, BARCELONA, SA_CALOBRA, LA_MANCHA, DOURO_VALLEY, LISBON, ALGARVE, ANDALUSIA,
        ALHAMBRA, COSTA_DEL_SOL, LANZAROTE, CHEFCHAOUEN, MARRAKECH, ESSAOUIRA, HIGH_ATLAS, SAHARA_DUNES, SIDI_BOU_SAID, GHADAMES,
        TASSILI, ADRAR, FOGO, PINK_LAKE, GAMBIA_RIVER, BIJAGOS, FOUTA_DJALLON, FREETOWN, LIBERIAN_COAST, SAHEL, TIEBELE, AGADEZ,
        YAMOUSSOUKRO, GOLD_COAST, KOUTAMMAKOU, GANVIE, LAGOS, ZUMA_ROCK, ENNEDI, MOUNT_CAMEROON, BIOKO, PICO_CAO_GRANDE, LOANGO,
        DZANGA_SANGHA, CONGO_RAPIDS, CONGO_BASIN, RWANDA_HILLS, TANGANYIKA, MURCHISON, SUDD, ETHIOPIAN_HIGHLANDS, ERITREAN_HIGHLANDS,
        DANAKIL, LAKE_ASSAL, LAAS_GEEL, SAVANNA, LAKE_NAKURU, SERENGETI, NGORONGORO, KILIMANJARO, ZANZIBAR, LAKE_MALAWI, ZAMBEZI,
        GREAT_ZIMBABWE, BAZARUTO, KRUGER, EZULWINI, OKAVANGO, KALAHARI, ETOSHA, MOON_VIEWPOINT, SKELETON_COAST, NAMIB, CAPE_PENINSULA,
        CHAPMANS_PEAK, CAPE_WINELANDS, GARDEN_ROUTE, SANI_PASS, DRAKENSBERG, BAOBAB_AVENUE, CHAMAREL, KARTHALA, SEYCHELLES, SOCOTRA,
        HAJAR_MOUNTAINS, EMPTY_QUARTER, DUBAI, DOHA, KING_FAHD, KUWAIT_CITY, MESOPOTAMIA, WADI_RUM, NEGEV, DEAD_SEA, BETHLEHEM, GIZA,
        NILE_VALLEY, LUXOR, MEROE, WHITE_DESERT, CEDARS, PALMYRA, CYPRUS, CAPPADOCIA, PAMUKKALE, LYCIAN_COAST, ISTANBUL, ATHENS,
        SANTORINI, PELOPONNESE, METEORA, LAKE_OHRID, ALBANIAN_RIVIERA, KOTOR, KOSOVO, MOSTAR, DALMATIA, LAKE_BLED, IRON_GATES,
        ROSE_VALLEY, DANUBE_DELTA, ORHEI, TRANSFAGARASAN, TRANSYLVANIA, PUSZTA, SPIS, TATRA, BOHEMIA, MASURIA, BELOVEZHSKAYA,
        SUNFLOWER_STEPPE, CAUCASUS, ARARAT, CASPIAN, YAZD, DARVAZA, SAMARKAND, KAZAKH_STEPPE, CHARYN, TIAN_SHAN, PAMIR_HIGHWAY,
        BAND_E_AMIR, KARAKORAM, LADAKH, SPITI, MANALI, PUNJAB, RAJASTHAN, AGRA, MUMBAI, GOA, HAMPI, KERALA, SRI_LANKA, SIGIRIYA,
        MALDIVES, DARJEELING, HIMALAYA, KATHMANDU_VALLEY, BHUTAN, MEGHALAYA, SUNDARBANS, BAGAN, NORTHERN_THAILAND, BANGKOK,
        ANDAMAN_COAST, MEKONG, ANGKOR, SAPA, HA_GIANG, RICE, HA_LONG_BAY, HAI_VAN_PASS, KUALA_LUMPUR, SINGAPORE, BORNEO, BRUNEI,
        BOROBUDUR, BROMO, BALI, DILI, PALAWAN, CHOCOLATE_HILLS, HONG_KONG, TAROKO, GUILIN, ZHANGJIAJIE, TIANMEN, BAMBOO_SEA, TIBET,
        ZHANGYE, ANHUI_VILLAGES, HUANGSHAN, SHANGHAI, GUOLIANG, BEIJING, GREAT_WALL, GOBI, MONGOLIAN_STEPPE, HARBIN, KUMGANG, SEOUL,
        HANOK_VILLAGE, JEJU, MOUNT_FUJI, HAKONE, KYOTO, MIYAJIMA, SAKURA, IROHAZAKA, SHIRAKAWA_GO, TOKYO, FURANO, OKINAWA, ROCK_ISLANDS,
        NAN_MADOL, MAJURO, TARAWA, NAURU, PNG_HIGHLANDS, MAROVO, YASUR, DAINTREE, SURFERS_PARADISE, KIMBERLEY, RED_CENTRE, NULLARBOR,
        GREAT_OCEAN_ROAD, TASMANIA, BLUE_MOUNTAINS, SYDNEY, FIORDLAND, CROWN_RANGE, MACKENZIE_BASIN, ROTORUA, CORAL_COAST, FUNAFUTI,
        LALOMANU, TONGA_BLOWHOLES, BORA_BORA, VOLCANO, HANA, NORTH_SHORE, EASTER_ISLAND, TORRES_DEL_PAINE, PERITO_MORENO,
        CARRETERA_AUSTRAL, PATAGONIA, RUTA_40, VALPARAISO, CARACOLES, MENDOZA, PAMPAS, BUENOS_AIRES, PUNTA_DEL_ESTE, RIO_DO_RASTRO,
        IGUAZU, CHACO, HUMAHUACA, ATACAMA, SALT_FLATS, ALTIPLANO, YUNGAS, TITICACA, RAINBOW_MOUNTAIN, SACRED_VALLEY, NAZCA, GALAPAGOS,
        VOLCANO_AVENUE, COCORA_VALLEY, CARTAGENA, GRAN_SABANA, KAIETEUR, SURINAME_RIVER, AMAZON_RIVER, JUNGLE, TRANSAMAZONICA, PANTANAL,
        SAO_PAULO, RIO, LENCOIS, MARACAS_BAY, GRAND_ANSE, LA_SOUFRIERE, BATHSHEBA, PITONS, BOILING_LAKE, ENGLISH_HARBOUR,
        BRIMSTONE_HILL, PUNTA_CANA, CITADELLE, JAMAICA, EXUMA, VINALES, PANAMA_CANAL, CLOUD_FOREST, OMETEPE, RUTA_DE_LAS_FLORES, COPAN,
        MAYAN_JUNGLE, BLUE_HOLE, RIVIERA_MAYA, YUCATAN, OAXACA, MEXICO_CITY, AGAVE_FIELDS, MEXICAN_HIGHLANDS, COPPER_CANYON, BAJA,
        LA_FREEWAY, HOLLYWOOD, JOSHUA_TREE, ROUTE_66, LAS_VEGAS, DEATH_VALLEY, COAST, BIG_SUR, NAPA, SAN_FRANCISCO, YOSEMITE, REDWOODS,
        OREGON_COAST, CASCADES, SEATTLE, GOING_TO_THE_SUN, YELLOWSTONE, TETONS, BONNEVILLE, ARCHES, GRAND_CANYON, SEDONA, DESERT,
        MESA_SUNSET, MILLION_DOLLAR, PIKES_PEAK, WHITE_SANDS, WEST_TEXAS, MOUNT_RUSHMORE, BADLANDS, WHEAT, CHICAGO, BAYOU, NEW_ORLEANS,
        FLORIDA_KEYS, MIAMI, TAIL_OF_THE_DRAGON, SMOKY_MOUNTAINS, BLUE_RIDGE, WASHINGTON_DC, SUBURB, NEW_YORK, AUTUMN, MAINE_COAST,
        QUEBEC_CITY, CABOT_TRAIL, NEWFOUNDLAND, NIAGARA, TORONTO, PRAIRIES, BANFF, ICEFIELDS, SEA_TO_SKY, YUKON, SNOW, DENALI, DALTON,
        KAMCHATKA, ROAD_OF_BONES, LAKE_BAIKAL, CHUYSKY, SIBERIA, GOLDEN_RING, MOSCOW, ST_PETERSBURG, LAHEMAA, GAUJA, CURONIAN_SPIT,
        FINNISH_LAKELAND, AURORA, NORTH_CAPE, LOFOTEN, TROLLSTIGEN, FJORD, ATLANTIC_ROAD, SMALAND, ORESUND, JUTLAND, ICEBERGS,
        JOKULSARLON, RING_ROAD };
    return route;
}

int routeIndex( BiomeE biome )
{
    static const auto index = []
    {
        std::array<int, static_cast<size_t>( BiomeE::COUNT )> a{};
        a.fill( -1 );
        const auto &route = worldRoute();
        for( size_t i = 0; i < route.size(); ++i ) a[static_cast<size_t>( route[i] )] = static_cast<int>( i );
        return a;
    }();
    return index[static_cast<size_t>( biome ) % index.size()];
}

} // namespace sunstreak
