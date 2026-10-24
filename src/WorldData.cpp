/**
 * @file WorldData.cpp
 * @brief The world read from the data files: the places (data/places/*.txt), the countries (data/countries.txt), the
 *        route and where the places are (data/route.txt).
 *
 * A place file is a list of places, each a header "place ID | NAME | COUNTRY | SKY [| REGION]" and one builder step
 * per line (WorldBuilder.h); the steps are looked up in a table, not in a chain of ifs. Places whose ID names a
 * compiled biome (BiomeE) keep its number, so the records and track codes of earlier versions still mean the same
 * place; new places are numbered after them in the order of the files, which therefore only ever grow at the end.
 *
 * @author reneweller
 */
#include "Biome.h"

#include "Data.h"
#include "Music.h"
#include "WorldBuilder.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <unordered_map>

namespace sunstreak
{

namespace
{

// The names the files use, in the order of the enums.
const char *const BiomeNames[] = {
    "MEADOW", "FOREST", "DESERT", "SNOW", "COAST", "CITY", "MARSH", "NEON",
    "AUTUMN", "SAKURA", "JUNGLE", "SAVANNA", "VOLCANO", "AURORA", "LAVENDER", "TULIPS",
    "HIGHLANDS", "FJORD", "RICE", "SALT_FLATS", "MESA_SUNSET", "SUBURB", "HARBOR", "NEON_RAIN",
    "EXPRESSWAY", "MOON", "MARS", "CRYSTALS", "FUNGI", "CLOUDS", "ICEBERGS", "BAYOU",
    "WHEAT", "CLIFFS", "HALLOWEEN", "WINTER_VILLAGE", "RAINBOW", "ECLIPSE", "CONNEMARA", "WILD_ATLANTIC",
    "ISLE_OF_SKYE", "SNOWDONIA", "LAKE_DISTRICT", "COTSWOLDS", "LONDON", "BRITTANY", "NORMANDY", "PARIS",
    "CHAMPAGNE", "LOIRE_VALLEY", "FLANDERS", "RHINE_VALLEY", "BLACK_FOREST", "AUTOBAHN", "BAVARIA", "SWISS_ALPS",
    "LAVAUX", "SALZKAMMERGUT", "CHAMONIX", "DOLOMITES", "LAKE_COMO", "TUSCANY", "VIA_APPIA", "AMALFI",
    "SICILY", "COTE_D_AZUR", "CAMARGUE", "BASQUE_COAST", "RIOJA", "DOURO_VALLEY", "ALGARVE", "ANDALUSIA",
    "LA_MANCHA", "COSTA_DEL_SOL", "LANZAROTE", "CHEFCHAOUEN", "MARRAKECH", "HIGH_ATLAS", "SAHARA_DUNES", "SAHEL",
    "GOLD_COAST", "CONGO_BASIN", "RWANDA_HILLS", "ETHIOPIAN_HIGHLANDS", "DANAKIL", "LAKE_NAKURU", "SERENGETI", "KILIMANJARO",
    "ZANZIBAR", "ZAMBEZI", "OKAVANGO", "KALAHARI", "NAMIB", "SKELETON_COAST", "CAPE_PENINSULA", "CAPE_WINELANDS",
    "DRAKENSBERG", "BAOBAB_AVENUE", "SEYCHELLES", "SOCOTRA", "HAJAR_MOUNTAINS", "EMPTY_QUARTER", "DUBAI", "WADI_RUM",
    "DEAD_SEA", "GIZA", "NILE_VALLEY", "WHITE_DESERT", "CEDARS", "CAPPADOCIA", "LYCIAN_COAST", "ISTANBUL",
    "SANTORINI", "PELOPONNESE", "METEORA", "KOTOR", "DALMATIA", "ROSE_VALLEY", "DANUBE_DELTA", "TRANSYLVANIA",
    "PUSZTA", "BOHEMIA", "TATRA", "MASURIA", "SUNFLOWER_STEPPE", "CAUCASUS", "YAZD", "SAMARKAND",
    "KAZAKH_STEPPE", "TIAN_SHAN", "PAMIR_HIGHWAY", "KARAKORAM", "LADAKH", "RAJASTHAN", "AGRA", "MUMBAI",
    "GOA", "KERALA", "DARJEELING", "HIMALAYA", "KATHMANDU_VALLEY", "BHUTAN", "SRI_LANKA", "MALDIVES",
    "BAGAN", "NORTHERN_THAILAND", "ANDAMAN_COAST", "MEKONG", "ANGKOR", "SAPA", "HA_LONG_BAY", "HAI_VAN_PASS",
    "BORNEO", "SINGAPORE", "BALI", "BROMO", "CHOCOLATE_HILLS", "HONG_KONG", "GUILIN", "ZHANGJIAJIE",
    "BAMBOO_SEA", "TIBET", "ANHUI_VILLAGES", "SHANGHAI", "GREAT_WALL", "GOBI", "MONGOLIAN_STEPPE", "HANOK_VILLAGE",
    "JEJU", "MOUNT_FUJI", "KYOTO", "SHIRAKAWA_GO", "TOKYO", "FURANO", "DAINTREE", "RED_CENTRE",
    "BLUE_MOUNTAINS", "GREAT_OCEAN_ROAD", "MACKENZIE_BASIN", "ROTORUA", "FIORDLAND", "BORA_BORA", "NORTH_SHORE", "EASTER_ISLAND",
    "CARRETERA_AUSTRAL", "PATAGONIA", "PAMPAS", "MENDOZA", "ATACAMA", "ALTIPLANO", "TITICACA", "SACRED_VALLEY",
    "VOLCANO_AVENUE", "COCORA_VALLEY", "GRAN_SABANA", "AMAZON_RIVER", "PANTANAL", "LENCOIS", "RIO", "CLOUD_FOREST",
    "MAYAN_JUNGLE", "VINALES", "JAMAICA", "YUCATAN", "AGAVE_FIELDS", "MEXICAN_HIGHLANDS", "BAJA", "ROUTE_66",
    "LAS_VEGAS", "DEATH_VALLEY", "JOSHUA_TREE", "BIG_SUR", "YOSEMITE", "REDWOODS", "CASCADES", "YELLOWSTONE",
    "GRAND_CANYON", "ARCHES", "BADLANDS", "WEST_TEXAS", "FLORIDA_KEYS", "MIAMI", "SMOKY_MOUNTAINS", "NEW_YORK",
    "MAINE_COAST", "NEWFOUNDLAND", "PRAIRIES", "BANFF", "DENALI", "KAMCHATKA", "LAKE_BAIKAL", "SIBERIA",
    "GOLDEN_RING", "CURONIAN_SPIT", "FINNISH_LAKELAND", "LOFOTEN", "SMALAND", "JUTLAND", "RING_ROAD", "CAUSEWAY_COAST",
    "GLENCOE", "APPLECROSS", "STONEHENGE", "CORNWALL", "MONT_SAINT_MICHEL", "DUNE_OF_PILAT", "ALPE_D_HUEZ", "VERDON",
    "COL_DE_TURINI", "CORSICA", "MONACO", "ANDORRA", "AMSTERDAM", "LUXEMBOURG", "ROMANTIC_ROAD", "NORDSCHLEIFE",
    "BERLIN", "LIECHTENSTEIN", "FURKA_PASS", "GROSSGLOCKNER", "STELVIO", "VENICE", "CINQUE_TERRE", "PISA",
    "ROME", "SAN_MARINO", "VATICAN", "ALBEROBELLO", "SARDINIA", "MALTA", "SA_CALOBRA", "BARCELONA",
    "ALHAMBRA", "PICOS", "LISBON", "ESSAOUIRA", "SIDI_BOU_SAID", "TASSILI", "GHADAMES", "ADRAR",
    "MEROE", "LUXOR", "FOGO", "PINK_LAKE", "GAMBIA_RIVER", "FOUTA_DJALLON", "BIJAGOS", "FREETOWN",
    "LIBERIAN_COAST", "YAMOUSSOUKRO", "TIEBELE", "AGADEZ", "ZUMA_ROCK", "LAGOS", "KOUTAMMAKOU", "GANVIE",
    "MOUNT_CAMEROON", "ENNEDI", "DZANGA_SANGHA", "LOANGO", "CONGO_RAPIDS", "BIOKO", "PICO_CAO_GRANDE", "MOON_VIEWPOINT",
    "MURCHISON", "TANGANYIKA", "SUDD", "ERITREAN_HIGHLANDS", "LAKE_ASSAL", "LAAS_GEEL", "NGORONGORO", "BAZARUTO",
    "LAKE_MALAWI", "GREAT_ZIMBABWE", "SANI_PASS", "EZULWINI", "CHAMAREL", "KARTHALA", "CHAPMANS_PEAK", "GARDEN_ROUTE",
    "KRUGER", "ETOSHA", "DOHA", "KING_FAHD", "KUWAIT_CITY", "MESOPOTAMIA", "NEGEV", "BETHLEHEM",
    "PALMYRA", "CYPRUS", "PAMUKKALE", "ATHENS", "ARARAT", "CASPIAN", "DARVAZA", "BAND_E_AMIR",
    "ALBANIAN_RIVIERA", "LAKE_OHRID", "KOSOVO", "MOSTAR", "LAKE_BLED", "IRON_GATES", "ORHEI", "TRANSFAGARASAN",
    "SPIS", "BELOVEZHSKAYA", "LAHEMAA", "GAUJA", "ST_PETERSBURG", "MOSCOW", "CHUYSKY", "ROAD_OF_BONES",
    "CHARYN", "SPITI", "MANALI", "PUNJAB", "HAMPI", "MEGHALAYA", "SUNDARBANS", "SIGIRIYA",
    "BANGKOK", "HA_GIANG", "KUALA_LUMPUR", "BRUNEI", "BOROBUDUR", "PALAWAN", "DILI", "TAROKO",
    "TIANMEN", "ZHANGYE", "GUOLIANG", "HUANGSHAN", "BEIJING", "HARBIN", "KUMGANG", "SEOUL",
    "HAKONE", "IROHAZAKA", "MIYAJIMA", "OKINAWA", "ROCK_ISLANDS", "NAN_MADOL", "MAJURO", "TARAWA",
    "NAURU", "PNG_HIGHLANDS", "MAROVO", "YASUR", "SYDNEY", "SURFERS_PARADISE", "KIMBERLEY", "NULLARBOR",
    "TASMANIA", "CROWN_RANGE", "CORAL_COAST", "FUNAFUTI", "LALOMANU", "TONGA_BLOWHOLES", "TORRES_DEL_PAINE", "PERITO_MORENO",
    "RUTA_40", "VALPARAISO", "CARACOLES", "BUENOS_AIRES", "PUNTA_DEL_ESTE", "CHACO", "YUNGAS", "RAINBOW_MOUNTAIN",
    "NAZCA", "GALAPAGOS", "CARTAGENA", "KAIETEUR", "SURINAME_RIVER", "RIO_DO_RASTRO", "SAO_PAULO", "TRANSAMAZONICA",
    "HUMAHUACA", "IGUAZU", "MARACAS_BAY", "GRAND_ANSE", "LA_SOUFRIERE", "BATHSHEBA", "PITONS", "BOILING_LAKE",
    "ENGLISH_HARBOUR", "BRIMSTONE_HILL", "PUNTA_CANA", "CITADELLE", "EXUMA", "PANAMA_CANAL", "OMETEPE", "RUTA_DE_LAS_FLORES",
    "COPAN", "BLUE_HOLE", "RIVIERA_MAYA", "OAXACA", "MEXICO_CITY", "COPPER_CANYON", "LA_FREEWAY", "HOLLYWOOD",
    "NAPA", "SAN_FRANCISCO", "OREGON_COAST", "SEATTLE", "GOING_TO_THE_SUN", "TETONS", "BONNEVILLE", "SEDONA",
    "WHITE_SANDS", "PIKES_PEAK", "MILLION_DOLLAR", "MOUNT_RUSHMORE", "CHICAGO", "NEW_ORLEANS", "TAIL_OF_THE_DRAGON", "BLUE_RIDGE",
    "WASHINGTON_DC", "HANA", "DALTON", "YUKON", "ICEFIELDS", "SEA_TO_SKY", "NIAGARA", "TORONTO",
    "QUEBEC_CITY", "CABOT_TRAIL", "NORTH_CAPE", "TROLLSTIGEN", "ATLANTIC_ROAD", "ORESUND", "JOKULSARLON",
};

const char *const PropNames[] = {
    "PINE", "SNOW_PINE", "OAK", "BUSH", "ROCK", "BOULDER", "CACTUS", "MESA",
    "PALM", "DEAD_TREE", "REEDS", "LAMP", "BUILDING", "TOWER", "BILLBOARD", "POLE",
    "FENCE", "HAYBALE", "HOUSE", "WINDMILL", "NEON_PILLAR", "PYRAMID", "SNOWBANK", "AUTUMN_TREE",
    "BIRCH", "CHERRY", "TORII", "LANTERN", "PAGODA", "JUNGLE_TREE", "FERN", "ACACIA",
    "TERMITE_MOUND", "GRASS_TUFT", "LAVA_ROCK", "ICE_SPIKE", "CYPRESS", "DUTCH_MILL", "RUIN", "STONE_WALL",
    "CABIN", "BAMBOO", "HUT", "BUTTE", "MAILBOX", "CONTAINERS", "CRANE", "CHIMNEY",
    "NEON_TOWER", "CRATER", "DOME", "ANTENNA", "CRYSTAL", "MUSHROOM", "GLOW_SHROOM", "ICEBERG",
    "MOSS_TREE", "FIREFLIES", "BARN", "SILO", "LIGHTHOUSE", "PUMPKIN", "GRAVESTONE", "LIGHT_TREE",
    "SNOW_HOUSE", "OLIVE", "POPLAR", "UMBRELLA_PINE", "SPRUCE", "BAOBAB", "EUCALYPTUS", "JOSHUA_TREE",
    "AGAVE", "PRICKLY_PEAR", "BANANA", "CEDAR", "REDWOOD", "WAX_PALM", "DRAGON_TREE", "TREE_FERN",
    "LUPIN", "FLOWER_BUSH", "SHRUB", "BROADLEAF", "VINE_ROW", "TEA_ROW", "DATE_PALM", "MANGROVE",
    "SUPERTREE", "KARST", "SEA_STACK", "FAIRY_CHIMNEY", "PILLAR", "ARCH", "DUNE", "KOPJE",
    "CHALK_ROCK", "MENHIR", "MOAI", "MONOLITH", "TEPUI", "VOLCANO_CONE", "GRANITE_DOME", "HILLOCK",
    "BADLAND", "GEYSER", "TERRACES", "PEAK", "COTTAGE", "FLAT_HOUSE", "CHALET", "HALF_TIMBER",
    "ROW_HOUSES", "CHURCH", "ONION_CHURCH", "MOSQUE", "STUPA", "WAT", "TEMPLE_TOWER", "STEP_PYRAMID",
    "STONE_PYRAMID", "KASBAH", "ADOBE", "YURT", "RONDAVEL", "HANOK", "GASSHO", "HUIZHOU_HOUSE",
    "CASTLE", "COLUMNS", "AQUEDUCT", "ROUND_MILL", "WATCHTOWER", "FORT", "CAPE_DUTCH", "GRAIN_ELEVATOR",
    "WATER_TOWER", "PUMPJACK", "WIND_PUMP", "DINER", "MOTEL_SIGN", "GAS_STATION", "CASINO", "GLASS_TOWER",
    "SUPERTALL", "TV_TOWER", "LATTICE_TOWER", "PANEL_BLOCK", "FAVELA", "WATER_VILLA", "TEMPLE_GATE", "PRAYER_FLAGS",
    "WAYSIDE_CROSS", "PARASOL", "HEDGE", "RANCH_FENCE", "POWER_PYLON", "BALLOON", "SAILBOAT", "BOAT",
    "HOUSEBOAT", "COW", "SHEEP", "HORSE", "CAMEL", "ELEPHANT", "GIRAFFE", "ZEBRA",
    "KANGAROO", "LLAMA", "BISON", "MOOSE", "REINDEER", "YAK", "PANDA", "FLAMINGO",
    "LANDMARK", "WATERFALL", "BASALT_COLUMNS", "STONE_CIRCLE", "SHIP", "REED_HOUSE", "PIPELINE", "WARNING_SIGN",
    "TORTOISE", "TRULLO",
};

const char *const BridgeNames[] = {
    "BEAM", "VIADUCT", "ARCH", "TRUSS", "SUSPENSION", "CABLE_STAYED", "COVERED", "CAUSEWAY",
};

const char *const SkyNames[] = {
    "MORNING", "NOON", "HAZE", "DESERT", "GOLDEN", "SUNSET", "DUSK", "NIGHT",
    "OVERCAST", "RAIN", "MIST", "ALPINE", "ARCTIC", "TROPICAL", "WINTER",
};

const char *const LandmarkNames[] = {
    "BIG_BEN", "TOWER_BRIDGE", "COLOSSEUM", "LEANING_TOWER", "BASILICA", "SAGRADA", "NEUSCHWANSTEIN", "MONT_ST_MICHEL",
    "BRANDENBURG_GATE", "ACROPOLIS", "ST_BASIL", "OPEN_ARMS", "LIBERTY", "OPERA_HOUSE", "TWIN_TOWERS", "SPACE_NEEDLE",
    "CAPITOL", "OBELISK", "HOLLYWOOD_SIGN", "KUWAIT_TOWERS", "VICTORY_COLUMN", "COLOSSI", "SPHINX", "BOROBUDUR",
    "HAND", "ICE_CASTLE", "FIRE_CRATER", "TRIUMPHAL_ARCH", "GREAT_ZIMBABWE", "ISLAND_CHURCH", "SEA_TORII",
};


static_assert( std::size( BiomeNames ) == static_cast<size_t>( BiomeE::COUNT ), "BiomeNames follows BiomeE" );
static_assert( std::size( PropNames ) == static_cast<size_t>( PropKindE::COUNT ), "PropNames follows PropKindE" );

const char *const ClimateNames[] = { "NONE", "TEMPERATE", "TROPICAL", "ARID", "COLD" };   ///< ClimateE
const char *const FleetNames[] = { "EUROPE", "NORTH_AMERICA", "BRITISH", "LATIN", "SOUTH_ASIA", "SOUTHEAST_ASIA",
                                   "EAST_ASIA", "OCEANIA", "AFRICA", "ARABIA", "EASTERN", "ISLANDS" };   ///< FleetE
static_assert( std::size( FleetNames ) == static_cast<size_t>( FleetE::COUNT ), "FleetNames follows FleetE" );

/** @brief The stations a country's countryside can play, by name. */
const std::pair<const char *, int> StationNames[] = {
    { "CITYPOP", StationCityPop }, { "KOTO", StationKoto },     { "BOLLYWOOD", StationBollywood }, { "DESERT", StationDesert },
    { "AFRO", StationAfro },       { "SAMBA", StationSamba },   { "TANGO", StationTango },         { "ANDES", StationAndes },
    { "CARIBBEAN", StationCaribbean }, { "MARIACHI", StationMariachi }, { "CELTIC", StationCeltic }, { "BALKAN", StationBalkan },
    { "GAMELAN", StationGamelan }, { "OUTBACK", StationOutback }, { "STEPPE", StationSteppe },     { "NORDIC", StationNordic },
    { "ROUTE66", StationRoute66 }, { "LOFI", StationLofi } };

/**
 * @brief Keeps a string for good: names and countries the tables point to (one copy of each).
 * @param[in] s  the string
 * @return its lasting copy
 */
const char *keep( const std::string &s )
{
    static std::deque<std::string>                       pool;
    static std::unordered_map<std::string, const char *> index;
    const auto                                           it = index.find( s );
    if( it != index.end() ) return it->second;
    pool.push_back( s );
    index.emplace( s, pool.back().c_str() );
    return pool.back().c_str();
}

/**
 * @brief The place of a name in a list.
 * @param[in] names  the list
 * @param[in] n      its length
 * @param[in] s      the name
 * @return its index, or -1
 */
int indexOf( const char *const *names, size_t n, const std::string &s )
{
    for( size_t i = 0; i < n; ++i )
        if( s == names[i] ) return static_cast<int>( i );
    return -1;
}

int g_errors = 0;   ///< steps and names the files got wrong

const std::vector<std::pair<int, GeoPoint>> &seaAnchors();

/**
 * @brief The ID of every place, by number.
 * @return them
 */
std::vector<std::string> &keys()
{
    static std::vector<std::string> k;
    return k;
}

/**
 * @brief The number of every place by its ID.
 * @return the index
 */
std::unordered_map<std::string, int> &keyIndex()
{
    static std::unordered_map<std::string, int> k;
    return k;
}

/** @brief The words after a step's name. */
struct Args
{
    std::vector<std::string> m_w;   ///< the words

    /**
     * @brief A number.
     * @param[in] i    which word
     * @param[in] def  if there is none
     * @return its value
     */
    float f( size_t i, float def = 0.0f ) const { return i < m_w.size() ? std::strtof( m_w[i].c_str(), nullptr ) : def; }

    /**
     * @brief A colour (RRGGBB).
     * @param[in] i    which word
     * @param[in] def  if there is none
     * @return its value
     */
    uint32_t c( size_t i, uint32_t def = 0 ) const
    {
        return i < m_w.size() ? static_cast<uint32_t>( std::strtoul( m_w[i].c_str(), nullptr, 16 ) ) : def;
    }

    /**
     * @brief A word.
     * @param[in] i  which
     * @return it, or empty
     */
    const std::string &s( size_t i ) const
    {
        static const std::string none;
        return i < m_w.size() ? m_w[i] : none;
    }
};

using Step = std::function<void( W &, const Args & )>;

/**
 * @brief A landmark's variant by name: the compiled ones first, then those of data/landmarks.
 * @param[in] name  e.g. "BIG_BEN"
 * @return the variant, or -1
 */
int landmarkVariant( const std::string &name )
{
    const int compiled = indexOf( LandmarkNames, std::size( LandmarkNames ), name );
    if( compiled >= 0 ) return compiled;
    const auto &shapes = dataShapes();
    for( size_t i = 0; i < shapes.size(); ++i )
        if( shapes[i].m_name == name ) return static_cast<int>( std::size( LandmarkNames ) + i );
    return -1;
}

/**
 * @brief The builder's steps by name.
 * @return the table
 */
const std::unordered_map<std::string, Step> &steps()
{
    static const std::unordered_map<std::string, Step> table = {
        { "sun", []( W &w, const Args &a ) { w.sun( a.f( 0 ), a.f( 1 ) ); } },
        { "fog", []( W &w, const Args &a ) { w.fog( a.f( 0 ), a.c( 1 ) ); } },
        { "clouds", []( W &w, const Args &a ) { w.clouds( a.f( 0 ), a.c( 1 ) ); } },
        { "ranges", []( W &w, const Args &a ) { w.ranges( a.c( 0 ), a.c( 1 ), a.f( 2 ) ); } },
        { "sky", []( W &w, const Args &a ) { w.sky( a.c( 0 ), a.c( 1 ) ); } },
        { "rain", []( W &w, const Args &a ) { w.rain( a.f( 0 ) ); } },
        { "snow", []( W &w, const Args &a ) { w.snow( a.f( 0 ) ); } },
        { "look", []( W &w, const Args &a ) { w.look( a.f( 0 ) ); } },
        { "land", []( W &w, const Args &a ) { w.land( a.c( 0 ), a.c( 1 ), a.c( 2 ) ); } },
        { "verge", []( W &w, const Args &a ) { w.verge( a.c( 0 ) ); } },
        { "water", []( W &w, const Args &a ) { w.water( a.c( 0 ), a.f( 1, 0.95f ) ); } },
        { "road", []( W &w, const Args &a ) { w.road( a.c( 0 ), a.c( 1, 0xF0F0F0 ) ); } },
        { "kerbs", []( W &w, const Args &a ) { w.kerbs( a.c( 0 ), a.c( 1 ) ); } },
        { "fields", []( W &w, const Args &a ) { w.fields( a.f( 0 ), a.c( 1 ), a.c( 2 ), a.c( 3 ) ); } },
        { "shine", []( W &w, const Args &a ) { w.shine( a.f( 0 ) ); } },
        { "hills", []( W &w, const Args &a ) { w.hills( a.f( 0 ), a.f( 1 ), a.f( 2 ), a.f( 3 ) ); } },
        { "sea", []( W &w, const Args &a ) { w.sea( a.f( 0 ), a.f( 1 ), a.f( 2, -4.5f ) ); } },
        { "drive", []( W &w, const Args &a ) { w.drive( static_cast<int>( a.f( 0 ) ), a.f( 1 ), a.f( 2 ), static_cast<int>( a.f( 3 ) ) ); } },
        { "bends", []( W &w, const Args &a ) { w.bends( a.f( 0 ), a.f( 1 ), a.f( 2 ) ); } },
        { "posts", []( W &w, const Args &a ) { w.posts( a.f( 0 ) ); } },
        { "lamps", []( W &w, const Args &a ) { w.lamps( a.f( 0 ) ); } },
        { "gantries", []( W &w, const Args &a ) { w.gantries( a.f( 0 ) ); } },
        { "rails", []( W &w, const Args & ) { w.rails(); } },
        { "tunnels", []( W &w, const Args &a ) { w.tunnels( a.f( 0 ) ); } },
        { "bridges",
          []( W &w, const Args &a )
          {
              const int style = indexOf( BridgeNames, std::size( BridgeNames ), a.s( 1 ) );
              if( style < 0 ) ++g_errors;
              w.bridges( a.f( 0 ), static_cast<BridgeE>( std::max( 0, style ) ), a.c( 2 ) );
          } },
        { "sand", []( W &w, const Args & ) { w.sand(); } },
        { "climate",
          []( W &w, const Args &a )
          {
              const int c = indexOf( ClimateNames, std::size( ClimateNames ), a.s( 0 ) );
              if( c < 0 ) ++g_errors;
              else w.climate( static_cast<ClimateE>( c ) );
          } },
        { "p",
          []( W &w, const Args &a )
          {
              const int kind = indexOf( PropNames, std::size( PropNames ), a.s( 0 ) );
              if( kind < 0 )
              {
                  ++g_errors;
                  return;
              }
              w.p( static_cast<PropKindE>( kind ), a.f( 1 ), a.f( 2 ), a.f( 3 ), a.f( 4, 0.8f ), a.f( 5, 1.2f ) );
          } },
        { "c", []( W &w, const Args &a ) { w.c( a.c( 0 ), a.c( 1 ), a.c( 2 ) ); } },
        { "pastel", []( W &w, const Args & ) { w.pastel(); } },
        { "floating", []( W &w, const Args & ) { w.floating(); } },
        { "v", []( W &w, const Args &a ) { w.v( static_cast<int>( a.f( 0 ) ) ); } },
        { "once", []( W &w, const Args &a ) { w.once( static_cast<int>( a.f( 0, 1.0f ) ) ); } },
        { "windows", []( W &w, const Args &a ) { w.windows( a.c( 0 ) ); } },
        { "landmark",
          []( W &w, const Args &a )
          {
              const int lm = landmarkVariant( a.s( 0 ) );
              if( lm < 0 )
              {
                  ++g_errors;
                  return;
              }
              w.landmark( lm, a.f( 1 ), a.f( 2 ), a.f( 3, 1.0f ) );
          } },
        { "underwater", []( W &w, const Args & ) { w.underwater(); } },
        { "island", []( W &w, const Args &a ) { w.island( keep( a.s( 0 ) ) ); } },
        { "light", []( W &w, const Args &a ) { w.light( a.f( 0 ), a.f( 1 ), a.f( 2, 1.0f ) ); } },
        { "scatter",
          []( W &w, const Args &a )
          {
              // A landmark shape strewn by chance (coral, kelp, anemones), not once per stage.
              const int lm = landmarkVariant( a.s( 0 ) );
              if( lm < 0 )
              {
                  ++g_errors;
                  return;
              }
              w.p( PropKindE::LANDMARK, a.f( 1 ), a.f( 2 ), a.f( 3 ), a.f( 4, 0.8f ), a.f( 5, 1.2f ) ).v( lm );
          } },
        { "cobbles", []( W &w, const Args & ) { w.cobbles(); } },
        { "crossing",
          []( W &w, const Args &a )
          {
              // crossing ferry|train: the tour crosses this sea aboard instead of under it
              if( a.s( 0 ) == "ferry" ) w.crossing( CrossingE::FERRY );
              else if( a.s( 0 ) == "train" ) w.crossing( CrossingE::TRAIN );
              else ++g_errors;
          } },
        { "swim",
          []( W &w, const Args &a )
          {
              // swim NAME weight liftMin liftMax [countMin countMax] [circle]
              SwimRule r;
              Args     b = a;
              const auto circle = std::find( b.m_w.begin(), b.m_w.end(), std::string( "circle" ) );
              if( circle != b.m_w.end() )
              {
                  r.m_circle = true;
                  b.m_w.erase( circle );
              }
              const auto &shapes = dataShapes();
              for( size_t i = 0; i < shapes.size(); ++i )
                  if( shapes[i].m_name == a.s( 0 ) && shapes[i].m_creature ) r.m_shape = static_cast<int>( i );
              if( r.m_shape < 0 )
              {
                  ++g_errors;
                  return;
              }
              r.m_weight = b.f( 1, 1.0f );
              r.m_liftMin = b.f( 2, 2.0f );
              r.m_liftMax = b.f( 3, 10.0f );
              r.m_countMin = static_cast<int>( b.f( 4, 1.0f ) );
              r.m_countMax = std::max( r.m_countMin, static_cast<int>( b.f( 5, static_cast<float>( r.m_countMin ) ) ) );
              w.swim( r );
          } },
        { "drop",
          []( W &w, const Args &a )
          {
              const int kind = indexOf( PropNames, std::size( PropNames ), a.s( 0 ) );
              if( kind < 0 )
              {
                  ++g_errors;
                  return;
              }
              int variant = -1;
              if( a.m_w.size() > 1 ) variant = static_cast<PropKindE>( kind ) == PropKindE::LANDMARK ? landmarkVariant( a.s( 1 ) ) : static_cast<int>( a.f( 1 ) );
              w.drop( static_cast<PropKindE>( kind ), variant );
          } },
    };
    return table;
}

/** @brief A place's (or a template's) header in a file. */
struct Header
{
    std::string m_key;                ///< its ID
    std::string m_name;               ///< on screen
    std::string m_country;            ///< its country
    std::string m_region;             ///< its state or province (may be empty)
    int         m_sky = -1;           ///< SkyE (-1: "-", the light of the place it is like)
    bool        m_template = false;   ///< a model for places, not a place
    std::vector<std::pair<std::string, Args>> m_steps;   ///< the builder steps
};

/**
 * @brief Reads every place file.
 * @return the places, in the files' order
 */
std::vector<Header> readPlaces()
{
    std::vector<Header> places;
    for( const std::string &file : dataNames( "places/" ) )
        for( const std::string &line : dataLines( dataText( file ) ) )
        {
            const bool place = line.rfind( "place ", 0 ) == 0, model = line.rfind( "template ", 0 ) == 0;
            if( place || model )
            {
                // place ID | NAME | COUNTRY | SKY [| REGION]   or   template ID | SKY
                const std::vector<std::string> parts = splitLine( line.substr( place ? 6 : 9 ), '|' );
                if( parts.size() < ( place ? 4u : 2u ) )
                {
                    ++g_errors;
                    continue;
                }
                Header h;
                h.m_key = parts[0];
                h.m_template = model;
                if( place )
                {
                    h.m_name = parts[1];
                    h.m_country = parts[2];
                    if( parts.size() > 4 ) h.m_region = parts[4];
                }
                const std::string &sky = parts[place ? 3 : 1];
                h.m_sky = sky == "-" ? -1 : indexOf( SkyNames, std::size( SkyNames ), sky );
                if( h.m_sky < 0 && sky != "-" )
                {
                    ++g_errors;
                    h.m_sky = 0;
                }
                places.push_back( std::move( h ) );
                continue;
            }
            if( places.empty() )
            {
                ++g_errors;
                continue;
            }
            std::vector<std::string> words = splitLine( line, ' ' );
            std::string              name = words.front();
            words.erase( words.begin() );
            places.back().m_steps.push_back( { name, Args{ std::move( words ) } } );
        }
    return places;
}

/**
 * @brief The countries, read once.
 * @return them, in the file's order
 */
const std::vector<CountryInfo> &countries()
{
    static DataCache<std::vector<CountryInfo>> cache;
    return cache.get( []
    {
        std::vector<CountryInfo> out;
        const char *const        lines[] = { "WHITE", "YELLOW", "EDGE" };
        const char *const        warns[] = { "TRIANGLE", "YELLOW", "DIAMOND" };
        const char *const        chevrons[] = { "RED", "BLACK", "YELLOW" };
        const char *const        towns[] = { "WHITE", "YELLOW", "RED", "BLUE", "GREEN" };
        for( const std::string &line : dataLines( dataText( "countries.txt" ) ) )
        {
            const std::vector<std::string> p = splitLine( line, '|' );
            if( p.size() < 9 )
            {
                ++g_errors;
                continue;
            }
            CountryInfo c{};
            c.m_name = keep( p[0] );
            c.m_left = p[1] == "L";
            const int fleet = indexOf( FleetNames, std::size( FleetNames ), p[2] );
            c.m_fleet = static_cast<FleetE>( std::max( 0, fleet ) );
            c.m_lines = static_cast<uint8_t>( std::max( 0, indexOf( lines, std::size( lines ), p[3] ) ) );
            c.m_warn = static_cast<uint8_t>( std::max( 0, indexOf( warns, std::size( warns ), p[4] ) ) );
            c.m_chevrons = static_cast<uint8_t>( std::max( 0, indexOf( chevrons, std::size( chevrons ), p[5] ) ) );
            c.m_townSign = static_cast<uint8_t>( std::max( 0, indexOf( towns, std::size( towns ), p[6] ) ) );
            c.m_station = -1;
            for( const auto &[name, station] : StationNames )
                if( p[7] == name ) c.m_station = station;
            c.m_flag = keep( p[8] );
            c.m_land = keep( p.size() > 9 && !p[9].empty() ? p[9] : p[0] );
            if( fleet < 0 ) ++g_errors;
            out.push_back( c );
        }
        return out;
    } );
}

/** @brief The route and where its places are. */
struct RouteData
{
    std::vector<BiomeE>                    m_route;      ///< in order
    std::vector<std::pair<bool, GeoPoint>> m_where;      ///< by place number
    std::vector<int>                       m_index;      ///< place number -> route position (-1 off the route)
    std::vector<RouteFork>                 m_forks;      ///< by route position
};

/**
 * @brief The distance between two points on the globe.
 * @param[in] a  one
 * @param[in] b  the other
 * @return kilometres
 */
float greatCircle( const GeoPoint &a, const GeoPoint &b )
{
    const float k = Tau / 360.0f;
    const float h = std::pow( std::sin( ( b.m_lat - a.m_lat ) * k * 0.5f ), 2.0f ) +
                    std::cos( a.m_lat * k ) * std::cos( b.m_lat * k ) * std::pow( std::sin( ( b.m_lon - a.m_lon ) * k * 0.5f ), 2.0f );
    return 2.0f * 6371.0f * std::asin( std::min( 1.0f, std::sqrt( h ) ) );
}

/**
 * @brief The direction from one point to another.
 * @param[in] a  from
 * @param[in] b  to
 * @return degrees east of north
 */
float bearing( const GeoPoint &a, const GeoPoint &b )
{
    const float k = Tau / 360.0f;
    const float y = std::sin( ( b.m_lon - a.m_lon ) * k ) * std::cos( b.m_lat * k );
    const float x = std::cos( a.m_lat * k ) * std::sin( b.m_lat * k ) - std::sin( a.m_lat * k ) * std::cos( b.m_lat * k ) * std::cos( ( b.m_lon - a.m_lon ) * k );
    return std::atan2( y, x ) / k;
}

/**
 * @brief Which country or state a place belongs to, for the borders the forks stand at.
 * @param[in] biome  the place
 * @return "COUNTRY/STATE"
 */
std::string landOf( BiomeE biome )
{
    const BiomeDef &d = biomeDef( biome );
    return std::string( d.m_country ? d.m_country : "" ) + "/" + ( d.m_region ? d.m_region : "" );
}

/**
 * @brief Finds the forks: where the route crosses a border, a neighbour a little further on is the other choice.
 * @param[in,out] d  the route
 */
void findForks( RouteData &d )
{
    const int n = static_cast<int>( d.m_route.size() );
    d.m_forks.assign( static_cast<size_t>( n ), RouteFork{} );
    auto where = [&]( int pos ) { return d.m_where[static_cast<size_t>( d.m_route[static_cast<size_t>( ( pos % n + n ) % n )] )].second; };
    auto land = [&]( int pos ) { return landOf( d.m_route[static_cast<size_t>( ( pos % n + n ) % n )] ); };
    for( int i = 0; i < n; ++i )
    {
        RouteFork &f = d.m_forks[static_cast<size_t>( i )];
        f.m_main = ( i + 1 ) % n;
        const std::string here = land( i ), next = land( i + 1 );
        if( here == next ) continue;
        // The nearest place that starts another land a little further on (not the next one's, not this one's).
        const GeoPoint at = where( i );
        const float    hop = greatCircle( at, where( i + 1 ) );
        float          best = std::max( 900.0f, hop * 2.5f + 300.0f );
        for( int j = i + 2; j < i + 40; ++j )
        {
            const std::string other = land( j );
            if( other == here || other == next || other == land( j - 1 ) ) continue;
            const float dist = greatCircle( at, where( j ) );
            if( dist < best )
            {
                best = dist;
                f.m_other = j % n;
            }
        }
        if( f.m_other < 0 ) continue;
        float turn = bearing( at, where( f.m_other ) ) - bearing( at, where( f.m_main ) );
        while( turn > 180.0f ) turn -= 360.0f;
        while( turn <= -180.0f ) turn += 360.0f;
        f.m_otherLeft = turn < 0.0f;
    }
}

/**
 * @brief The route, read once.
 * @return it
 */
const RouteData &routeData()
{
    static DataCache<RouteData> cache;
    return cache.get( []
    {
        RouteData d;
        const int count = biomeCount();
        d.m_where.resize( static_cast<size_t>( count ) );
        d.m_index.assign( static_cast<size_t>( count ), -1 );
        for( const std::string &line : dataLines( dataText( "route.txt" ) ) )
        {
            const std::vector<std::string> p = splitLine( line, ' ' );
            const int                      id = p.empty() ? -1 : biomeId( p[0] );
            if( id < 0 || p.size() < 3 )
            {
                ++g_errors;
                continue;
            }
            d.m_index[static_cast<size_t>( id )] = static_cast<int>( d.m_route.size() );
            d.m_route.push_back( static_cast<BiomeE>( id ) );
            d.m_where[static_cast<size_t>( id )] = { true, GeoPoint{ std::strtof( p[1].c_str(), nullptr ), std::strtof( p[2].c_str(), nullptr ) } };
        }
        findForks( d );
        return d;
    } );
}

} // namespace

void addDataPlaces( std::vector<BiomeDef> &table )
{
    std::vector<Header> all = readPlaces(), places, models;
    for( Header &h : all ) ( h.m_template ? models : places ).push_back( std::move( h ) );
    // Numbers: a compiled biome's, or the next free one.
    keys().assign( std::begin( BiomeNames ), std::end( BiomeNames ) );
    std::vector<int> ids;
    for( const Header &h : places )
    {
        int id = indexOf( BiomeNames, std::size( BiomeNames ), h.m_key );
        if( id < 0 )
        {
            const auto it = std::find( keys().begin(), keys().end(), h.m_key );
            if( it != keys().end() ) id = static_cast<int>( it - keys().begin() );
            else
            {
                id = static_cast<int>( keys().size() );
                keys().push_back( h.m_key );
            }
        }
        ids.push_back( id );
    }
    if( keys().size() > static_cast<size_t>( MaxBiomes ) ) keys().resize( static_cast<size_t>( MaxBiomes ) );
    table.resize( std::max( table.size(), keys().size() ) );
    keyIndex().clear();
    for( size_t i = 0; i < keys().size(); ++i ) keyIndex().emplace( keys()[i], static_cast<int>( i ) );

    // Templates are built into a table of their own, places into the biome table; "like" may name either, and a place
    // it names is built first wherever it stands in the files.
    std::vector<BiomeDef>                models_( models.size() );
    std::unordered_map<std::string, int> modelIndex, placeIndex;
    for( size_t i = 0; i < models.size(); ++i ) modelIndex.emplace( models[i].m_key, static_cast<int>( i ) );
    for( size_t i = 0; i < places.size(); ++i ) placeIndex.emplace( places[i].m_key, static_cast<int>( i ) );
    std::vector<uint8_t> modelState( models.size(), 0 ), placeState( places.size(), 0 );   // 0 to do, 1 building, 2 done
    const auto          &named = steps();
    std::function<void( bool, size_t )> build = [&]( bool model, size_t i )
    {
        std::vector<uint8_t> &state = model ? modelState : placeState;
        if( state[i] ) return;   // done, or a loop of "like"s (left as far as it got)
        state[i] = 1;
        const Header   &h = model ? models[i] : places[i];
        const bool      dark = h.m_sky < 0;
        BiomeDef       *target = model ? models_.data() : table.data();
        const size_t    slot = model ? i : static_cast<size_t>( ids[i] );
        W               w( target, static_cast<BiomeE>( slot ), keep( model ? h.m_key : h.m_name ),
                           model || h.m_country == "-" ? nullptr : keep( h.m_country ),
                           static_cast<SkyE>( dark ? 0 : h.m_sky ) );
        if( !h.m_region.empty() ) target[slot].m_region = keep( h.m_region );
        for( const auto &[name, args] : h.m_steps )
        {
            if( name == "like" )
            {
                const std::string &src = args.s( 0 );
                const auto         m = modelIndex.find( src );
                const auto         p = placeIndex.find( src );
                if( m != modelIndex.end() )
                {
                    build( true, static_cast<size_t>( m->second ) );
                    w.from( models_[static_cast<size_t>( m->second )], dark );
                }
                else if( p != placeIndex.end() )
                {
                    build( false, static_cast<size_t>( p->second ) );
                    w.from( table[static_cast<size_t>( ids[static_cast<size_t>( p->second )] )], dark );
                }
                else if( indexOf( BiomeNames, std::size( BiomeNames ), src ) >= 0 )
                    w.from( table[static_cast<size_t>( indexOf( BiomeNames, std::size( BiomeNames ), src ) )], dark );   // a compiled biome
                else ++g_errors;
                continue;
            }
            const auto step = named.find( name );
            if( step == named.end() ) ++g_errors;
            else step->second( w, args );
        }
        state[i] = 2;
    };
    for( size_t i = 0; i < models.size(); ++i ) build( true, i );
    for( size_t i = 0; i < places.size(); ++i )
        if( ids[i] < MaxBiomes ) build( false, i );
}

int compiledLandmarks()
{
    return static_cast<int>( std::size( LandmarkNames ) );
}

int landmarkCount()
{
    return compiledLandmarks() + static_cast<int>( dataShapes().size() );
}

std::string landmarkName( int variant )
{
    if( variant >= 0 && variant < compiledLandmarks() ) return LandmarkNames[variant];
    const auto  &shapes = dataShapes();
    const size_t i = static_cast<size_t>( std::max( 0, variant - compiledLandmarks() ) );
    return variant >= compiledLandmarks() && i < shapes.size() ? shapes[i].m_name : std::string();
}

int landmarkByName( const std::string &name )
{
    return landmarkVariant( name );
}

void resetDataErrors()
{
    g_errors = 0;
}

int dataErrors()
{
    biomeCount();
    worldRoute();
    countries();
    seaAnchors();
    return g_errors;
}

int biomeId( std::string_view key )
{
    biomeCount();
    const auto it = keyIndex().find( std::string( key ) );
    return it != keyIndex().end() ? it->second : -1;
}

const std::string &biomeKey( BiomeE biome )
{
    static const std::string none;
    biomeCount();
    const size_t i = static_cast<size_t>( biome );
    return i < keys().size() ? keys()[i] : none;
}

const CountryInfo &countryInfo( const char *country )
{
    static const CountryInfo fallback = { "", false, FleetE::EUROPE, 0, 0, 0, 0, "H FFFFFF", -1, nullptr };
    if( !country ) return fallback;
    for( const CountryInfo &c : countries() )
        if( std::strcmp( c.m_name, country ) == 0 ) return c;
    return fallback;
}

int countryIndex( const char *country )
{
    if( !country ) return -1;
    const auto &list = countries();
    for( size_t i = 0; i < list.size(); ++i )
        if( std::strcmp( list[i].m_name, country ) == 0 ) return static_cast<int>( i );
    return -1;
}

int countryTableSize()
{
    return static_cast<int>( countries().size() );
}

const CountryInfo &countryAt( int index )
{
    return index >= 0 && index < countryTableSize() ? countries()[static_cast<size_t>( index )] : countryInfo( nullptr );
}

const std::vector<BiomeE> &worldRoute()
{
    return routeData().m_route;
}

RouteFork routeFork( int pos )
{
    const auto &forks = routeData().m_forks;
    if( forks.empty() ) return {};
    return forks[static_cast<size_t>( ( pos % static_cast<int>( forks.size() ) + static_cast<int>( forks.size() ) ) % static_cast<int>( forks.size() ) )];
}

namespace
{

/**
 * @brief The seas a crossing may lie in, by where they are (data/seas.txt).
 * @return sea biome and a point in it
 */
const std::vector<std::pair<int, GeoPoint>> &seaAnchors()
{
    static DataCache<std::vector<std::pair<int, GeoPoint>>> cache;
    return cache.get( []
    {
        std::vector<std::pair<int, GeoPoint>> out;
        for( const std::string &line : dataLines( dataText( "seas.txt" ) ) )
        {
            const std::vector<std::string> p = splitLine( line, ' ' );
            const int                      id = p.empty() ? -1 : biomeId( p[0] );
            if( id < 0 || p.size() < 3 )
            {
                ++g_errors;
                continue;
            }
            out.push_back( { id, GeoPoint{ std::strtof( p[1].c_str(), nullptr ), std::strtof( p[2].c_str(), nullptr ) } } );
        }
        return out;
    } );
}

/**
 * @brief The landmass a place lies on.
 * @param[in] biome  the place
 * @return its island, or its country's landmass (empty off the map)
 */
std::string landmassOf( BiomeE biome )
{
    const BiomeDef &d = biomeDef( biome );
    if( d.m_island ) return d.m_island;
    if( !d.m_country ) return {};
    return countryInfo( d.m_country ).m_land ? countryInfo( d.m_country ).m_land : d.m_country;
}

} // namespace

int seaCrossing( BiomeE from, BiomeE to )
{
    const std::string a = landmassOf( from ), b = landmassOf( to );
    if( a.empty() || b.empty() || a == b ) return -1;
    GeoPoint pa, pb;
    if( !placeLocation( from, pa ) || !placeLocation( to, pb ) ) return -1;
    // The sea nearest the middle of the crossing (longitudes taken the short way round).
    float dLon = pb.m_lon - pa.m_lon;
    if( dLon > 180.0f ) dLon -= 360.0f;
    if( dLon < -180.0f ) dLon += 360.0f;
    GeoPoint mid{ ( pa.m_lat + pb.m_lat ) * 0.5f, pa.m_lon + dLon * 0.5f };
    if( mid.m_lon > 180.0f ) mid.m_lon -= 360.0f;
    if( mid.m_lon < -180.0f ) mid.m_lon += 360.0f;
    int   best = -1;
    float bestKm = 1e9f;
    for( const auto &[sea, at] : seaAnchors() )
    {
        const float km = greatCircle( mid, at );
        if( km < bestKm )
        {
            bestKm = km;
            best = sea;
        }
    }
    return best;
}

int routeIndex( BiomeE biome )
{
    const auto  &index = routeData().m_index;
    const size_t i = static_cast<size_t>( biome );
    return i < index.size() ? index[i] : -1;
}

bool placeLocation( BiomeE biome, GeoPoint &where )
{
    const auto  &all = routeData().m_where;
    const size_t i = static_cast<size_t>( biome );
    if( i >= all.size() || !all[i].first ) return false;
    where = all[i].second;
    return true;
}

int longitudeSector( float lon )
{
    const float wrapped = std::fmod( lon + 180.0f + 360.0f, 360.0f );
    return std::min( LongitudeSectors - 1, static_cast<int>( wrapped / ( 360.0f / LongitudeSectors ) ) );
}

} // namespace sunstreak
