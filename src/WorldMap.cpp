/**
 * @file WorldMap.cpp
 * @brief Where the world tour's places are on the globe (for the passport's map) and what driving round it means.
 *
 * Latitude and longitude in degrees, rounded to a tenth or so: enough to draw the route as a chain of dots that
 * outlines the continents.
 *
 * @author reneweller
 */
#include "Biome.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace sunstreak
{

namespace
{

/** @brief A place and where it is. */
struct Located
{
    BiomeE m_biome;   ///< the place
    float  m_lat;     ///< degrees north
    float  m_lon;     ///< degrees east
};

using enum BiomeE;

/** @brief Every place on the route, in route order. */
const Located Locations[] = {
    // British Isles, France, the Low Countries, Germany, the Alps.
    { RAINBOW, 53.0f, -8.0f }, { CONNEMARA, 53.5f, -9.8f }, { WILD_ATLANTIC, 52.1f, -10.2f }, { CAUSEWAY_COAST, 55.2f, -6.5f },
    { HIGHLANDS, 57.0f, -4.8f }, { GLENCOE, 56.7f, -5.1f }, { APPLECROSS, 57.4f, -5.8f }, { ISLE_OF_SKYE, 57.3f, -6.2f },
    { SNOWDONIA, 53.1f, -3.9f }, { LAKE_DISTRICT, 54.5f, -3.1f }, { COTSWOLDS, 51.8f, -1.8f }, { CORNWALL, 50.2f, -5.2f },
    { STONEHENGE, 51.2f, -1.8f }, { LONDON, 51.5f, -0.1f }, { CLIFFS, 50.8f, 0.2f }, { BRITTANY, 48.2f, -3.5f },
    { MONT_SAINT_MICHEL, 48.6f, -1.5f }, { NORMANDY, 49.2f, -0.5f }, { PARIS, 48.9f, 2.4f }, { LOIRE_VALLEY, 47.4f, 1.0f },
    { CHAMPAGNE, 49.1f, 4.0f }, { LUXEMBOURG, 49.6f, 6.1f }, { FLANDERS, 50.9f, 3.2f }, { TULIPS, 52.3f, 4.6f },
    { AMSTERDAM, 52.4f, 4.9f }, { HARBOR, 51.9f, 4.4f }, { NORDSCHLEIFE, 50.3f, 7.0f }, { RHINE_VALLEY, 50.1f, 7.7f },
    { MEADOW, 51.0f, 10.0f }, { BERLIN, 52.5f, 13.4f }, { ROMANTIC_ROAD, 49.4f, 10.2f }, { BLACK_FOREST, 48.0f, 8.2f },
    { AUTOBAHN, 48.8f, 9.6f }, { BAVARIA, 47.7f, 11.5f }, { LIECHTENSTEIN, 47.1f, 9.5f }, { SWISS_ALPS, 46.6f, 8.0f },
    { FURKA_PASS, 46.6f, 8.4f }, { FOREST, 46.9f, 7.5f }, { LAVAUX, 46.5f, 6.8f }, { CHAMONIX, 45.9f, 6.9f },
    { ALPE_D_HUEZ, 45.1f, 6.1f }, { SALZKAMMERGUT, 47.7f, 13.6f }, { GROSSGLOCKNER, 47.1f, 12.8f }, { WINTER_VILLAGE, 47.2f, 11.4f },
    // Italy, the south of France, Iberia.
    { DOLOMITES, 46.5f, 11.9f }, { STELVIO, 46.5f, 10.5f }, { LAKE_COMO, 46.0f, 9.3f }, { VENICE, 45.4f, 12.3f },
    { CINQUE_TERRE, 44.1f, 9.7f }, { PISA, 43.7f, 10.4f }, { TUSCANY, 43.3f, 11.3f }, { SAN_MARINO, 43.9f, 12.5f },
    { ROME, 41.9f, 12.5f }, { VATICAN, 41.9f, 12.4f }, { VIA_APPIA, 41.8f, 12.6f }, { AMALFI, 40.6f, 14.6f },
    { ALBEROBELLO, 40.8f, 17.2f }, { SICILY, 37.6f, 14.3f }, { MALTA, 35.9f, 14.5f }, { SARDINIA, 40.1f, 9.5f },
    { CORSICA, 42.1f, 9.0f }, { COTE_D_AZUR, 43.6f, 7.1f }, { MONACO, 43.7f, 7.4f }, { COL_DE_TURINI, 44.0f, 7.4f },
    { VERDON, 43.8f, 6.3f }, { LAVENDER, 43.8f, 5.8f }, { CAMARGUE, 43.5f, 4.5f }, { DUNE_OF_PILAT, 44.6f, -1.2f },
    { BASQUE_COAST, 43.4f, -2.5f }, { PICOS, 43.2f, -4.9f }, { RIOJA, 42.4f, -2.5f }, { ANDORRA, 42.5f, 1.6f },
    { BARCELONA, 41.4f, 2.2f }, { SA_CALOBRA, 39.9f, 2.8f }, { LA_MANCHA, 39.4f, -3.2f }, { DOURO_VALLEY, 41.2f, -7.6f },
    { LISBON, 38.7f, -9.1f }, { ALGARVE, 37.1f, -8.5f }, { ANDALUSIA, 36.8f, -5.2f }, { ALHAMBRA, 37.2f, -3.6f },
    { COSTA_DEL_SOL, 36.5f, -4.8f }, { LANZAROTE, 29.0f, -13.6f },
    // North and West Africa.
    { CHEFCHAOUEN, 35.2f, -5.3f }, { MARRAKECH, 31.6f, -8.0f }, { ESSAOUIRA, 31.5f, -9.8f }, { HIGH_ATLAS, 31.2f, -7.5f },
    { SAHARA_DUNES, 31.1f, -4.0f }, { SIDI_BOU_SAID, 36.9f, 10.4f }, { GHADAMES, 30.1f, 9.5f }, { TASSILI, 25.5f, 9.0f },
    { ADRAR, 20.5f, -13.0f }, { FOGO, 15.0f, -24.4f }, { PINK_LAKE, 14.8f, -17.2f }, { GAMBIA_RIVER, 13.5f, -15.5f },
    { BIJAGOS, 11.3f, -15.9f }, { FOUTA_DJALLON, 11.0f, -12.0f }, { FREETOWN, 8.4f, -13.2f }, { LIBERIAN_COAST, 6.8f, -11.4f },
    { SAHEL, 14.5f, -4.2f }, { TIEBELE, 11.1f, -1.0f }, { AGADEZ, 17.0f, 8.0f }, { YAMOUSSOUKRO, 6.8f, -5.3f },
    { GOLD_COAST, 5.1f, -1.3f }, { KOUTAMMAKOU, 10.0f, 1.2f }, { GANVIE, 6.5f, 2.4f }, { LAGOS, 6.5f, 3.4f },
    { ZUMA_ROCK, 9.1f, 7.2f }, { ENNEDI, 17.0f, 22.0f }, { MOUNT_CAMEROON, 4.2f, 9.2f }, { BIOKO, 3.5f, 8.7f },
    { PICO_CAO_GRANDE, 0.1f, 6.6f },
    // Central, East and Southern Africa, the islands.
    { LOANGO, -2.3f, 9.6f }, { DZANGA_SANGHA, 2.5f, 16.2f }, { CONGO_RAPIDS, -4.3f, 15.2f }, { CONGO_BASIN, 0.5f, 23.0f },
    { RWANDA_HILLS, -1.9f, 29.9f }, { TANGANYIKA, -3.9f, 29.4f }, { MURCHISON, 2.3f, 31.7f }, { SUDD, 7.5f, 30.5f },
    { ETHIOPIAN_HIGHLANDS, 13.0f, 38.0f }, { ERITREAN_HIGHLANDS, 15.3f, 39.0f }, { DANAKIL, 14.2f, 40.3f }, { LAKE_ASSAL, 11.7f, 42.4f },
    { LAAS_GEEL, 9.8f, 44.5f }, { SAVANNA, -1.5f, 35.2f }, { LAKE_NAKURU, -0.4f, 36.1f }, { SERENGETI, -2.3f, 34.8f },
    { NGORONGORO, -3.2f, 35.5f }, { KILIMANJARO, -3.1f, 37.4f }, { ZANZIBAR, -6.2f, 39.2f }, { LAKE_MALAWI, -12.0f, 34.6f },
    { ZAMBEZI, -17.9f, 25.9f }, { GREAT_ZIMBABWE, -20.3f, 30.9f }, { BAZARUTO, -21.7f, 35.5f }, { KRUGER, -24.0f, 31.5f },
    { EZULWINI, -26.4f, 31.2f }, { OKAVANGO, -19.3f, 22.9f }, { KALAHARI, -23.0f, 22.0f }, { ETOSHA, -18.9f, 16.3f },
    { MOON_VIEWPOINT, -9.1f, 13.0f }, { SKELETON_COAST, -20.0f, 13.2f }, { NAMIB, -24.7f, 15.3f }, { CAPE_PENINSULA, -34.3f, 18.5f },
    { CHAPMANS_PEAK, -34.1f, 18.4f }, { CAPE_WINELANDS, -33.9f, 18.9f }, { GARDEN_ROUTE, -34.0f, 23.0f }, { SANI_PASS, -29.6f, 29.3f },
    { DRAKENSBERG, -28.8f, 29.0f }, { BAOBAB_AVENUE, -20.3f, 44.4f }, { CHAMAREL, -20.4f, 57.4f }, { KARTHALA, -11.8f, 43.3f },
    { SEYCHELLES, -4.6f, 55.5f }, { SOCOTRA, 12.5f, 54.0f },
    // Arabia, the Levant, Egypt, Turkey, Greece, the Balkans, Eastern Europe, the Caucasus.
    { HAJAR_MOUNTAINS, 23.1f, 57.3f }, { EMPTY_QUARTER, 20.0f, 51.0f }, { DUBAI, 25.2f, 55.3f }, { DOHA, 25.3f, 51.5f },
    { KING_FAHD, 26.2f, 50.4f }, { KUWAIT_CITY, 29.4f, 48.0f }, { MESOPOTAMIA, 31.0f, 47.0f }, { WADI_RUM, 29.6f, 35.4f },
    { NEGEV, 30.6f, 34.8f }, { DEAD_SEA, 31.5f, 35.5f }, { BETHLEHEM, 31.7f, 35.2f }, { GIZA, 30.0f, 31.1f },
    { NILE_VALLEY, 27.5f, 30.8f }, { LUXOR, 25.7f, 32.6f }, { MEROE, 16.9f, 33.8f }, { WHITE_DESERT, 27.3f, 28.0f },
    { CEDARS, 34.2f, 36.1f }, { PALMYRA, 34.6f, 38.3f }, { CYPRUS, 34.9f, 32.9f }, { CAPPADOCIA, 38.6f, 34.8f },
    { PAMUKKALE, 37.9f, 29.1f }, { LYCIAN_COAST, 36.4f, 29.6f }, { ISTANBUL, 41.0f, 29.0f }, { ATHENS, 38.0f, 23.7f },
    { SANTORINI, 36.4f, 25.4f }, { PELOPONNESE, 37.5f, 22.4f }, { METEORA, 39.7f, 21.6f }, { LAKE_OHRID, 41.0f, 20.8f },
    { ALBANIAN_RIVIERA, 40.1f, 19.7f }, { KOTOR, 42.4f, 18.8f }, { KOSOVO, 42.7f, 20.2f }, { MOSTAR, 43.3f, 17.8f },
    { DALMATIA, 43.5f, 16.4f }, { LAKE_BLED, 46.4f, 14.1f }, { IRON_GATES, 44.6f, 22.2f }, { ROSE_VALLEY, 42.6f, 25.4f },
    { DANUBE_DELTA, 45.1f, 29.3f }, { ORHEI, 47.3f, 29.0f }, { TRANSFAGARASAN, 45.6f, 24.6f }, { TRANSYLVANIA, 45.5f, 25.4f },
    { PUSZTA, 47.0f, 19.5f }, { SPIS, 49.0f, 20.8f }, { TATRA, 49.2f, 20.0f }, { BOHEMIA, 49.8f, 14.0f },
    { MASURIA, 53.8f, 21.5f }, { BELOVEZHSKAYA, 52.6f, 23.9f }, { SUNFLOWER_STEPPE, 48.5f, 34.0f }, { CAUCASUS, 42.5f, 44.5f },
    { ARARAT, 39.9f, 44.6f }, { CASPIAN, 40.4f, 49.9f },
    // Iran, Central and South Asia.
    { YAZD, 31.9f, 54.4f }, { DARVAZA, 40.3f, 58.4f }, { SAMARKAND, 39.7f, 67.0f }, { KAZAKH_STEPPE, 48.0f, 68.0f },
    { CHARYN, 43.4f, 79.1f }, { TIAN_SHAN, 42.0f, 77.0f }, { PAMIR_HIGHWAY, 38.5f, 73.0f }, { BAND_E_AMIR, 34.8f, 67.2f },
    { KARAKORAM, 36.3f, 74.6f }, { LADAKH, 34.2f, 77.6f }, { SPITI, 32.3f, 78.0f }, { MANALI, 32.2f, 77.2f },
    { PUNJAB, 30.9f, 75.9f }, { RAJASTHAN, 26.9f, 71.0f }, { AGRA, 27.2f, 78.0f }, { MUMBAI, 19.1f, 72.9f },
    { GOA, 15.3f, 74.0f }, { HAMPI, 15.3f, 76.5f }, { KERALA, 9.5f, 76.5f }, { SRI_LANKA, 7.0f, 80.8f },
    { SIGIRIYA, 8.0f, 80.8f }, { MALDIVES, 4.2f, 73.5f }, { DARJEELING, 27.0f, 88.3f }, { HIMALAYA, 28.0f, 86.9f },
    { KATHMANDU_VALLEY, 27.7f, 85.3f }, { BHUTAN, 27.5f, 90.4f }, { MEGHALAYA, 25.3f, 91.7f }, { SUNDARBANS, 22.0f, 89.2f },
    // Southeast and East Asia.
    { BAGAN, 21.2f, 94.9f }, { NORTHERN_THAILAND, 18.8f, 99.0f }, { BANGKOK, 13.8f, 100.5f }, { ANDAMAN_COAST, 8.0f, 98.3f },
    { MEKONG, 19.9f, 102.1f }, { ANGKOR, 13.4f, 103.9f }, { SAPA, 22.3f, 103.8f }, { HA_GIANG, 23.0f, 105.0f },
    { RICE, 20.5f, 105.5f }, { HA_LONG_BAY, 20.9f, 107.1f }, { HAI_VAN_PASS, 16.2f, 108.1f }, { KUALA_LUMPUR, 3.1f, 101.7f },
    { SINGAPORE, 1.4f, 103.8f }, { BORNEO, 5.5f, 117.0f }, { BRUNEI, 4.9f, 114.9f }, { BOROBUDUR, -7.6f, 110.2f },
    { BROMO, -7.9f, 113.0f }, { BALI, -8.4f, 115.2f }, { DILI, -8.6f, 125.6f }, { PALAWAN, 11.2f, 119.4f },
    { CHOCOLATE_HILLS, 9.8f, 124.2f }, { HONG_KONG, 22.3f, 114.2f }, { TAROKO, 24.2f, 121.6f }, { GUILIN, 25.3f, 110.3f },
    { ZHANGJIAJIE, 29.3f, 110.4f }, { TIANMEN, 29.1f, 110.5f }, { BAMBOO_SEA, 28.5f, 105.0f }, { TIBET, 29.7f, 91.1f },
    { ZHANGYE, 38.9f, 100.5f }, { ANHUI_VILLAGES, 29.9f, 117.9f }, { HUANGSHAN, 30.1f, 118.2f }, { SHANGHAI, 31.2f, 121.5f },
    { GUOLIANG, 35.7f, 113.6f }, { BEIJING, 39.9f, 116.4f }, { GREAT_WALL, 40.4f, 116.6f }, { GOBI, 43.0f, 105.0f },
    { MONGOLIAN_STEPPE, 47.5f, 103.0f }, { HARBIN, 45.8f, 126.5f }, { KUMGANG, 38.6f, 128.1f }, { SEOUL, 37.6f, 127.0f },
    { HANOK_VILLAGE, 35.8f, 127.2f }, { JEJU, 33.4f, 126.5f }, { MOUNT_FUJI, 35.4f, 138.7f }, { HAKONE, 35.2f, 139.1f },
    { KYOTO, 35.0f, 135.8f }, { MIYAJIMA, 34.3f, 132.3f }, { SAKURA, 34.4f, 135.9f }, { IROHAZAKA, 36.7f, 139.6f },
    { SHIRAKAWA_GO, 36.3f, 136.9f }, { TOKYO, 35.7f, 139.7f }, { FURANO, 43.3f, 142.4f }, { OKINAWA, 26.3f, 127.8f },
    // The Pacific, Australia, New Zealand.
    { ROCK_ISLANDS, 7.3f, 134.5f }, { NAN_MADOL, 6.8f, 158.3f }, { MAJURO, 7.1f, 171.4f }, { TARAWA, 1.5f, 173.0f },
    { NAURU, -0.5f, 166.9f }, { PNG_HIGHLANDS, -6.0f, 145.0f }, { MAROVO, -8.5f, 158.0f }, { YASUR, -19.5f, 169.4f },
    { DAINTREE, -16.2f, 145.4f }, { SURFERS_PARADISE, -28.0f, 153.4f }, { KIMBERLEY, -16.5f, 125.5f }, { RED_CENTRE, -25.3f, 131.0f },
    { NULLARBOR, -31.5f, 129.0f }, { GREAT_OCEAN_ROAD, -38.7f, 143.1f }, { TASMANIA, -41.7f, 146.0f }, { BLUE_MOUNTAINS, -33.7f, 150.3f },
    { SYDNEY, -33.9f, 151.2f }, { FIORDLAND, -45.4f, 167.7f }, { CROWN_RANGE, -45.0f, 168.9f }, { MACKENZIE_BASIN, -44.0f, 170.2f },
    { ROTORUA, -38.1f, 176.3f }, { CORAL_COAST, -18.2f, 177.8f }, { FUNAFUTI, -8.5f, 179.2f }, { LALOMANU, -14.1f, -171.4f },
    { TONGA_BLOWHOLES, -21.2f, -175.3f }, { BORA_BORA, -16.5f, -151.7f }, { VOLCANO, 19.4f, -155.3f }, { HANA, 20.8f, -156.0f },
    { NORTH_SHORE, 21.6f, -158.1f }, { EASTER_ISLAND, -27.1f, -109.4f },
    // South America.
    { TORRES_DEL_PAINE, -51.0f, -73.0f }, { PERITO_MORENO, -50.5f, -73.0f }, { CARRETERA_AUSTRAL, -45.5f, -72.4f }, { PATAGONIA, -48.0f, -70.0f },
    { RUTA_40, -42.0f, -70.5f }, { VALPARAISO, -33.1f, -71.6f }, { CARACOLES, -32.8f, -70.2f }, { MENDOZA, -32.9f, -68.9f },
    { PAMPAS, -35.5f, -62.0f }, { BUENOS_AIRES, -34.6f, -58.4f }, { PUNTA_DEL_ESTE, -35.0f, -55.0f }, { RIO_DO_RASTRO, -28.4f, -49.6f },
    { IGUAZU, -25.7f, -54.4f }, { CHACO, -22.0f, -60.0f }, { HUMAHUACA, -23.2f, -65.4f }, { ATACAMA, -23.0f, -68.2f },
    { SALT_FLATS, -20.1f, -67.5f }, { ALTIPLANO, -18.0f, -67.5f }, { YUNGAS, -16.2f, -67.8f }, { TITICACA, -15.8f, -69.4f },
    { RAINBOW_MOUNTAIN, -13.9f, -71.3f }, { SACRED_VALLEY, -13.3f, -72.1f }, { NAZCA, -14.7f, -75.1f }, { GALAPAGOS, -0.7f, -90.3f },
    { VOLCANO_AVENUE, -1.0f, -78.6f }, { COCORA_VALLEY, 4.6f, -75.5f }, { CARTAGENA, 10.4f, -75.5f }, { GRAN_SABANA, 5.0f, -61.0f },
    { KAIETEUR, 5.2f, -59.5f }, { SURINAME_RIVER, 4.5f, -55.2f }, { AMAZON_RIVER, -3.1f, -60.0f }, { JUNGLE, -5.0f, -55.0f },
    { TRANSAMAZONICA, -6.5f, -50.0f }, { PANTANAL, -17.0f, -57.0f }, { SAO_PAULO, -23.6f, -46.6f }, { RIO, -22.9f, -43.2f },
    { LENCOIS, -2.5f, -43.1f },
    // The Caribbean, Central America, Mexico.
    { MARACAS_BAY, 10.8f, -61.4f }, { GRAND_ANSE, 12.0f, -61.8f }, { LA_SOUFRIERE, 13.3f, -61.2f }, { BATHSHEBA, 13.2f, -59.5f },
    { PITONS, 13.8f, -61.1f }, { BOILING_LAKE, 15.3f, -61.3f }, { ENGLISH_HARBOUR, 17.0f, -61.8f }, { BRIMSTONE_HILL, 17.4f, -62.8f },
    { PUNTA_CANA, 18.6f, -68.4f }, { CITADELLE, 19.6f, -72.2f }, { JAMAICA, 18.1f, -77.3f }, { EXUMA, 24.2f, -76.5f },
    { VINALES, 22.6f, -83.7f }, { PANAMA_CANAL, 9.1f, -79.7f }, { CLOUD_FOREST, 10.3f, -84.8f }, { OMETEPE, 11.5f, -85.6f },
    { RUTA_DE_LAS_FLORES, 13.9f, -89.8f }, { COPAN, 14.8f, -89.1f }, { MAYAN_JUNGLE, 17.2f, -89.6f }, { BLUE_HOLE, 17.3f, -87.5f },
    { RIVIERA_MAYA, 20.2f, -87.4f }, { YUCATAN, 20.7f, -88.6f }, { OAXACA, 16.9f, -96.3f }, { MEXICO_CITY, 19.4f, -99.1f },
    { AGAVE_FIELDS, 20.9f, -103.8f }, { MEXICAN_HIGHLANDS, 21.0f, -101.3f }, { COPPER_CANYON, 27.5f, -107.8f }, { BAJA, 28.0f, -113.5f },
    // The United States, Canada, Alaska.
    { LA_FREEWAY, 34.1f, -118.2f }, { HOLLYWOOD, 34.1f, -118.3f }, { JOSHUA_TREE, 33.9f, -115.9f }, { ROUTE_66, 35.2f, -113.0f },
    { LAS_VEGAS, 36.2f, -115.1f }, { DEATH_VALLEY, 36.5f, -117.0f }, { COAST, 35.3f, -120.9f }, { BIG_SUR, 36.3f, -121.8f },
    { NAPA, 38.5f, -122.3f }, { SAN_FRANCISCO, 37.8f, -122.4f }, { YOSEMITE, 37.8f, -119.6f }, { REDWOODS, 41.2f, -124.0f },
    { OREGON_COAST, 45.0f, -124.0f }, { CASCADES, 47.5f, -121.0f }, { SEATTLE, 47.6f, -122.3f }, { GOING_TO_THE_SUN, 48.7f, -113.8f },
    { YELLOWSTONE, 44.6f, -110.5f }, { TETONS, 43.7f, -110.8f }, { BONNEVILLE, 40.8f, -113.8f }, { ARCHES, 38.7f, -109.6f },
    { GRAND_CANYON, 36.1f, -112.1f }, { SEDONA, 34.9f, -111.8f }, { DESERT, 37.0f, -110.1f }, { MESA_SUNSET, 36.5f, -108.0f },
    { MILLION_DOLLAR, 37.9f, -107.7f }, { PIKES_PEAK, 38.8f, -105.0f }, { WHITE_SANDS, 32.8f, -106.2f }, { WEST_TEXAS, 31.0f, -103.0f },
    { MOUNT_RUSHMORE, 43.9f, -103.5f }, { BADLANDS, 43.9f, -102.3f }, { WHEAT, 38.5f, -98.0f }, { CHICAGO, 41.9f, -87.6f },
    { BAYOU, 30.0f, -91.5f }, { NEW_ORLEANS, 30.2f, -90.1f }, { FLORIDA_KEYS, 24.7f, -81.1f }, { MIAMI, 25.8f, -80.1f },
    { TAIL_OF_THE_DRAGON, 35.5f, -83.9f }, { SMOKY_MOUNTAINS, 35.6f, -83.5f }, { BLUE_RIDGE, 36.5f, -81.0f }, { WASHINGTON_DC, 38.9f, -77.0f },
    { SUBURB, 40.2f, -75.0f }, { NEW_YORK, 40.7f, -74.0f }, { AUTUMN, 44.0f, -72.7f }, { MAINE_COAST, 44.3f, -68.3f },
    { QUEBEC_CITY, 46.8f, -71.2f }, { CABOT_TRAIL, 46.7f, -60.6f }, { NEWFOUNDLAND, 48.5f, -56.0f }, { NIAGARA, 43.1f, -79.1f },
    { TORONTO, 43.7f, -79.4f }, { PRAIRIES, 50.5f, -105.0f }, { BANFF, 51.2f, -115.6f }, { ICEFIELDS, 52.2f, -117.2f },
    { SEA_TO_SKY, 49.7f, -123.2f }, { YUKON, 60.7f, -135.0f }, { SNOW, 62.0f, -128.0f }, { DENALI, 63.1f, -151.0f },
    { DALTON, 68.0f, -149.5f },
    // Russia, the Baltic, Scandinavia, Greenland, Iceland.
    { KAMCHATKA, 53.0f, 158.7f }, { ROAD_OF_BONES, 63.0f, 145.0f }, { LAKE_BAIKAL, 53.5f, 108.0f }, { CHUYSKY, 50.4f, 86.5f },
    { SIBERIA, 58.0f, 95.0f }, { GOLDEN_RING, 56.4f, 40.4f }, { MOSCOW, 55.8f, 37.6f }, { ST_PETERSBURG, 59.9f, 30.3f },
    { LAHEMAA, 59.6f, 25.9f }, { GAUJA, 57.2f, 24.9f }, { CURONIAN_SPIT, 55.3f, 21.0f }, { FINNISH_LAKELAND, 62.0f, 28.0f },
    { AURORA, 68.5f, 27.0f }, { NORTH_CAPE, 71.2f, 25.8f }, { LOFOTEN, 68.2f, 13.9f }, { TROLLSTIGEN, 62.5f, 7.7f },
    { FJORD, 62.1f, 7.2f }, { ATLANTIC_ROAD, 63.0f, 7.4f }, { SMALAND, 57.0f, 15.0f }, { ORESUND, 55.6f, 12.9f },
    { JUTLAND, 56.0f, 9.0f }, { ICEBERGS, 69.2f, -51.1f }, { JOKULSARLON, 64.1f, -16.2f }, { RING_ROAD, 64.5f, -19.0f },
};

/** @brief The table indexed by biome. */
const auto &byBiome()
{
    static const auto table = []
    {
        std::array<std::pair<bool, GeoPoint>, static_cast<size_t>( BiomeE::COUNT )> t{};
        for( const Located &l : Locations ) t[static_cast<size_t>( l.m_biome )] = { true, GeoPoint{ l.m_lat, l.m_lon } };
        return t;
    }();
    return table;
}

} // namespace

bool placeLocation( BiomeE biome, GeoPoint &where )
{
    const auto &entry = byBiome()[static_cast<size_t>( biome ) % byBiome().size()];
    if( entry.first ) where = entry.second;
    return entry.first;
}

int longitudeSector( float lon )
{
    const float wrapped = std::fmod( lon + 180.0f + 360.0f, 360.0f );
    return std::min( LongitudeSectors - 1, static_cast<int>( wrapped / ( 360.0f / LongitudeSectors ) ) );
}

} // namespace sunstreak
