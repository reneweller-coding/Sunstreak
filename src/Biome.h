/**
 * @file Biome.h
 * @brief The sceneries a run passes through: their light, colours, props and the style of their roads.
 *
 * A run is a chain of stages, one biome each. Everything a stage boundary changes is blendable, so the game crossfades
 * the light, the fog and the ground colours over a few hundred metres instead of cutting.
 *
 * @author reneweller
 */
#pragma once

#include <raylib.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sunstreak
{

/** @brief The sceneries. */
enum class BiomeE : uint16_t
{
    MEADOW,          ///< green valley in the morning
    FOREST,          ///< alpine pass, pines and steep hills
    DESERT,          ///< red canyon at noon
    SNOW,            ///< frozen peaks, falling snow, less grip
    COAST,           ///< sunset coast beside the sea
    CITY,            ///< motorway through a city at night
    MARSH,           ///< storm over a marsh: rain, fog, lightning
    NEON,            ///< synthwave night with a striped sun
    AUTUMN,          ///< amber woods in the golden hour
    SAKURA,          ///< cherry blossom hills, torii and pagodas
    JUNGLE,          ///< steaming rainforest
    SAVANNA,         ///< acacias at sunset
    VOLCANO,         ///< black rock, lava, ash sky
    AURORA,          ///< arctic night under the northern lights
    LAVENDER,        ///< Provence: lavender rows and cypresses
    TULIPS,          ///< Dutch polder: tulip fields, mills, canals
    HIGHLANDS,       ///< misty heather hills and ruins
    FJORD,           ///< cliffs, deep water, red cabins
    RICE,            ///< flooded paddies and bamboo
    SALT_FLATS,      ///< a white mirror under a deep sky
    MESA_SUNSET,     ///< buttes in the evening light
    SUBURB,          ///< suburbia on a summer afternoon
    HARBOR,          ///< cranes and containers at dusk
    NEON_RAIN,       ///< rainy cyberpunk city
    EXPRESSWAY,      ///< motorway at dawn, sound walls, sign gantries
    MOON,            ///< lunar highway, the Earth in the sky, low gravity
    MARS,            ///< the red planet
    CRYSTALS,        ///< twilight desert of glowing crystals
    FUNGI,           ///< giant glowing mushrooms
    CLOUDS,          ///< a ridge road above the clouds at sunrise
    ICEBERGS,        ///< polar bay with icebergs
    BAYOU,           ///< swamp at dusk, moss trees, fireflies
    WHEAT,           ///< golden wheat plains, barns and silos
    CLIFFS,          ///< chalk cliffs, the sea and a lighthouse
    HALLOWEEN,       ///< pumpkins and graves under an orange moon
    WINTER_VILLAGE,  ///< snowy village with festive lights
    RAINBOW,         ///< green ridge after the rain
    ECLIPSE,         ///< a total solar eclipse
    // The world tour (WorldBiomes.cpp), in the order of the route.
    CONNEMARA,           ///< Ireland: bog, stone walls, white cottages
    WILD_ATLANTIC,       ///< Ireland: sea cliffs and sheep
    ISLE_OF_SKYE,        ///< Scotland: bare peaks above the sea
    SNOWDONIA,           ///< Wales: slate, stone walls, rain
    LAKE_DISTRICT,       ///< England: fells and lakes
    COTSWOLDS,           ///< England: honey-stone villages and hedges
    LONDON,              ///< England: terraces and glass towers
    BRITTANY,            ///< France: granite, menhirs, the sea
    NORMANDY,            ///< France: half-timbering, orchards, cows
    PARIS,               ///< France: boulevards and an iron tower
    CHAMPAGNE,           ///< France: vineyards on chalk
    LOIRE_VALLEY,        ///< France: chateaux and poplars
    FLANDERS,            ///< Belgium: poppy fields, brick farms
    RHINE_VALLEY,        ///< Germany: steep vineyards, castles, the river
    BLACK_FOREST,        ///< Germany: dark firs, big-roofed farms
    AUTOBAHN,            ///< Germany: four lanes through pine forest
    BAVARIA,             ///< Germany: onion domes and meadows before the Alps
    SWISS_ALPS,          ///< Switzerland: chalets, cows, peaks
    LAVAUX,              ///< Switzerland: vineyard terraces above the lake
    SALZKAMMERGUT,       ///< Austria: lakes under steep mountains
    CHAMONIX,            ///< France: Mont Blanc and larches
    DOLOMITES,           ///< Italy: pale spires over meadows
    LAKE_COMO,           ///< Italy: villas and cypresses by the lake
    TUSCANY,             ///< Italy: cypress rows and golden hills
    VIA_APPIA,           ///< Italy: umbrella pines, ruins, an aqueduct
    AMALFI,              ///< Italy: pastel houses above the sea
    SICILY,              ///< Italy: Etna, olives, prickly pears
    COTE_D_AZUR,         ///< France: the Riviera
    CAMARGUE,            ///< France: white horses, flamingos, salt marsh
    BASQUE_COAST,        ///< Spain: green hills by the sea
    RIOJA,               ///< Spain: vines on red earth
    DOURO_VALLEY,        ///< Portugal: terraced vineyards over the river
    ALGARVE,             ///< Portugal: ochre cliffs
    ANDALUSIA,           ///< Spain: white villages, endless olives
    LA_MANCHA,           ///< Spain: white windmills on the plain
    COSTA_DEL_SOL,       ///< Spain: palms and white hotels
    LANZAROTE,           ///< Spain: black lava and white houses
    CHEFCHAOUEN,         ///< Morocco: the blue town in the Rif
    MARRAKECH,           ///< Morocco: pink walls and palms
    HIGH_ATLAS,          ///< Morocco: kasbahs under snowy peaks
    SAHARA_DUNES,        ///< Morocco: Erg Chebbi
    SAHEL,               ///< Mali: baobabs and a mud mosque
    GOLD_COAST,          ///< Ghana: palms, red earth, pirogues
    CONGO_BASIN,         ///< DR Congo: rainforest on a red road
    RWANDA_HILLS,        ///< Rwanda: a thousand green hills
    ETHIOPIAN_HIGHLANDS, ///< Ethiopia: terraces and round huts
    DANAKIL,             ///< Ethiopia: salt and sulphur
    LAKE_NAKURU,         ///< Kenya: flamingos
    SERENGETI,           ///< Tanzania: the herds and the kopjes
    KILIMANJARO,         ///< Tanzania: the snowy mountain over the farms
    ZANZIBAR,            ///< Tanzania: white sand and dhows
    ZAMBEZI,             ///< Zambia: the river and its spray
    OKAVANGO,            ///< Botswana: channels and elephants
    KALAHARI,            ///< Botswana: red sand and camelthorn
    NAMIB,               ///< Namibia: red dunes, dead trees
    SKELETON_COAST,      ///< Namibia: fog, sand and the sea
    CAPE_PENINSULA,      ///< South Africa: Table Mountain and the sea
    CAPE_WINELANDS,      ///< South Africa: Cape Dutch farms and vines
    DRAKENSBERG,         ///< South Africa: the green escarpment
    BAOBAB_AVENUE,       ///< Madagascar: the avenue of baobabs
    SEYCHELLES,          ///< Seychelles: granite boulders on white sand
    SOCOTRA,             ///< Yemen: dragon's blood trees
    HAJAR_MOUNTAINS,     ///< Oman: wadis, palms and forts
    EMPTY_QUARTER,       ///< Saudi Arabia: the biggest dunes
    DUBAI,               ///< UAE: towers in the desert
    WADI_RUM,            ///< Jordan: red sand, sandstone walls
    DEAD_SEA,            ///< Jordan: salt shores in the haze
    GIZA,                ///< Egypt: the pyramids
    NILE_VALLEY,         ///< Egypt: palms, fields and feluccas
    WHITE_DESERT,        ///< Egypt: chalk mushrooms
    CEDARS,              ///< Lebanon: cedars below the snow
    CAPPADOCIA,          ///< Turkey: fairy chimneys and balloons
    LYCIAN_COAST,        ///< Turkey: pines over a turquoise sea
    ISTANBUL,            ///< Turkey: mosques by the Bosphorus
    SANTORINI,           ///< Greece: white houses, blue domes
    PELOPONNESE,         ///< Greece: olives and temples
    METEORA,             ///< Greece: monasteries on rock pillars
    KOTOR,               ///< Montenegro: the bay under the mountains
    DALMATIA,            ///< Croatia: white karst and red roofs
    ROSE_VALLEY,         ///< Bulgaria: rose fields
    DANUBE_DELTA,        ///< Romania: reeds and water
    TRANSYLVANIA,        ///< Romania: haystacks and fortified churches
    PUSZTA,              ///< Hungary: the plain, wells and horses
    BOHEMIA,             ///< Czechia: rapeseed, castles, red roofs
    TATRA,               ///< Poland: wooden houses under the Tatra
    MASURIA,             ///< Poland: lakes and birches
    SUNFLOWER_STEPPE,    ///< Ukraine: sunflowers to the horizon
    CAUCASUS,            ///< Georgia: stone towers under the peaks
    YAZD,                ///< Iran: mud brick and wind towers
    SAMARKAND,           ///< Uzbekistan: turquoise domes
    KAZAKH_STEPPE,       ///< Kazakhstan: grass, yurts, horses
    TIAN_SHAN,           ///< Kyrgyzstan: yurts by alpine lakes
    PAMIR_HIGHWAY,       ///< Tajikistan: the roof of the world
    KARAKORAM,           ///< Pakistan: the highway under giants
    LADAKH,              ///< India: stupas and prayer flags
    RAJASTHAN,           ///< India: desert forts and camels
    AGRA,                ///< India: a white marble tomb
    MUMBAI,              ///< India: the bay and its towers
    GOA,                 ///< India: palms, beaches, white churches
    KERALA,              ///< India: backwaters and houseboats
    DARJEELING,          ///< India: tea on the hills
    HIMALAYA,            ///< Nepal: flags, stupas, the highest peaks
    KATHMANDU_VALLEY,    ///< Nepal: brick temples and terraces
    BHUTAN,              ///< Bhutan: dzongs in the pines
    SRI_LANKA,           ///< Sri Lanka: the south coast
    MALDIVES,            ///< Maldives: water villas on the lagoon
    BAGAN,               ///< Myanmar: temples and balloons
    NORTHERN_THAILAND,   ///< Thailand: wats in the misty hills
    ANDAMAN_COAST,       ///< Thailand: limestone in the sea
    MEKONG,              ///< Laos: the river and golden stupas
    ANGKOR,              ///< Cambodia: temple towers in the forest
    SAPA,                ///< Vietnam: terraces on steep hills
    HA_LONG_BAY,         ///< Vietnam: karst islands and junks
    HAI_VAN_PASS,        ///< Vietnam: the coast road over the pass
    BORNEO,              ///< Malaysia: rainforest giants
    SINGAPORE,           ///< Singapore: supertrees and towers
    BALI,                ///< Indonesia: temple gates and terraces
    BROMO,               ///< Indonesia: the volcano in the sea of sand
    CHOCOLATE_HILLS,     ///< Philippines: a thousand round hills
    HONG_KONG,           ///< China: towers between the hills and the harbour
    GUILIN,              ///< China: karst towers on the Li river
    ZHANGJIAJIE,         ///< China: sandstone pillars in the clouds
    BAMBOO_SEA,          ///< China: bamboo and pandas
    TIBET,               ///< China: the high plateau
    ANHUI_VILLAGES,      ///< China: white walls, black gables, rapeseed
    SHANGHAI,            ///< China: the skyline at night
    GREAT_WALL,          ///< China: the wall on the ridges
    GOBI,                ///< Mongolia: gravel desert and camels
    MONGOLIAN_STEPPE,    ///< Mongolia: gers and horses
    HANOK_VILLAGE,       ///< South Korea: curved roofs
    JEJU,                ///< South Korea: volcanic walls and tangerines
    MOUNT_FUJI,          ///< Japan: the mountain over the tea
    KYOTO,               ///< Japan: temples and bamboo
    SHIRAKAWA_GO,        ///< Japan: steep thatch in the snow
    TOKYO,               ///< Japan: the expressway at night
    FURANO,              ///< Japan: striped flower fields
    DAINTREE,            ///< Australia: rainforest and cane fields
    RED_CENTRE,          ///< Australia: Uluru and kangaroos
    BLUE_MOUNTAINS,      ///< Australia: eucalyptus haze
    GREAT_OCEAN_ROAD,    ///< Australia: sea stacks
    MACKENZIE_BASIN,     ///< New Zealand: lupins and turquoise lakes
    ROTORUA,             ///< New Zealand: steam and green hills
    FIORDLAND,           ///< New Zealand: rain, ferns, waterfalls
    BORA_BORA,           ///< French Polynesia: the lagoon
    NORTH_SHORE,         ///< Hawaii: green ridges and surf
    EASTER_ISLAND,       ///< Chile: the moai
    CARRETERA_AUSTRAL,   ///< Chile: rainforest and glaciers
    PATAGONIA,           ///< Argentina: granite spires over the steppe
    PAMPAS,              ///< Argentina: grass, cattle, estancias
    MENDOZA,             ///< Argentina: vines under the Andes
    ATACAMA,             ///< Chile: lagoons and flamingos in the desert
    ALTIPLANO,           ///< Bolivia: llamas and volcanoes
    TITICACA,            ///< Peru: the high lake
    SACRED_VALLEY,       ///< Peru: Inca terraces
    VOLCANO_AVENUE,      ///< Ecuador: the avenue of volcanoes
    COCORA_VALLEY,       ///< Colombia: wax palms
    GRAN_SABANA,         ///< Venezuela: the tepuis
    AMAZON_RIVER,        ///< Brazil: the river and stilt houses
    PANTANAL,            ///< Brazil: wetlands
    LENCOIS,             ///< Brazil: white dunes, blue lagoons
    RIO,                 ///< Brazil: Sugarloaf and favelas
    CLOUD_FOREST,        ///< Costa Rica: mist and a volcano
    MAYAN_JUNGLE,        ///< Guatemala: pyramids in the forest
    VINALES,             ///< Cuba: mogotes and tobacco
    JAMAICA,             ///< Jamaica: beaches and bananas
    YUCATAN,             ///< Mexico: a step pyramid and the jungle
    AGAVE_FIELDS,        ///< Mexico: blue agave
    MEXICAN_HIGHLANDS,   ///< Mexico: colonial towns, volcanoes
    BAJA,                ///< Mexico: cardon cacti by the sea
    ROUTE_66,            ///< USA: diners and motels
    LAS_VEGAS,           ///< USA: the Strip at night
    DEATH_VALLEY,        ///< USA: salt and heat
    JOSHUA_TREE,         ///< USA: Joshua trees and boulders
    BIG_SUR,             ///< USA: the coast highway
    YOSEMITE,            ///< USA: granite domes
    REDWOODS,            ///< USA: the giants
    CASCADES,            ///< USA: firs under a snowy volcano
    YELLOWSTONE,         ///< USA: geysers and bison
    GRAND_CANYON,        ///< USA: the rim
    ARCHES,              ///< USA: stone arches
    BADLANDS,            ///< USA: striped hills
    WEST_TEXAS,          ///< USA: pumpjacks and longhorns
    FLORIDA_KEYS,        ///< USA: the overseas highway
    MIAMI,               ///< USA: art deco and palms
    SMOKY_MOUNTAINS,     ///< USA: blue haze and log cabins
    NEW_YORK,            ///< USA: Manhattan
    MAINE_COAST,         ///< USA: lighthouses and lobster boats
    NEWFOUNDLAND,        ///< Canada: coloured houses on rock
    PRAIRIES,            ///< Canada: canola and grain elevators
    BANFF,               ///< Canada: turquoise lakes under the Rockies
    DENALI,              ///< Alaska: tundra and the great mountain
    KAMCHATKA,           ///< Russia: volcanoes and tundra
    LAKE_BAIKAL,         ///< Russia: the frozen lake
    SIBERIA,             ///< Russia: taiga in winter
    GOLDEN_RING,         ///< Russia: golden domes and wooden houses
    CURONIAN_SPIT,       ///< Lithuania: dunes and pines
    FINNISH_LAKELAND,    ///< Finland: lakes, birches, red houses
    LOFOTEN,             ///< Norway: fishing huts under steep peaks
    SMALAND,             ///< Sweden: red cottages in the forest
    JUTLAND,             ///< Denmark: thatch and wind turbines
    RING_ROAD,           ///< Iceland: moss on lava, waterfalls
    CAUSEWAY_COAST,      ///< Northern Ireland: Causeway Coast
    GLENCOE,             ///< Scotland: Glencoe
    APPLECROSS,          ///< Scotland: Applecross Pass
    STONEHENGE,          ///< England: Salisbury Plain
    CORNWALL,            ///< England: Cornwall
    MONT_SAINT_MICHEL,   ///< France: Mont Saint-Michel
    DUNE_OF_PILAT,       ///< France: Dune du Pilat
    ALPE_D_HUEZ,         ///< France: Alpe D Huez
    VERDON,              ///< France: Gorges Du Verdon
    COL_DE_TURINI,       ///< France: Col de Turini
    CORSICA,             ///< France: Corsica
    MONACO,              ///< Monaco: Monte Carlo
    ANDORRA,             ///< Andorra
    AMSTERDAM,           ///< Netherlands: Amsterdam
    LUXEMBOURG,          ///< Luxembourg
    ROMANTIC_ROAD,       ///< Germany: Romantic Road
    NORDSCHLEIFE,        ///< Germany: Nordschleife
    BERLIN,              ///< Germany: Berlin
    LIECHTENSTEIN,       ///< Liechtenstein
    FURKA_PASS,          ///< Switzerland: Furka Pass
    GROSSGLOCKNER,       ///< Austria: Grossglockner
    STELVIO,             ///< Italy: Stelvio Pass
    VENICE,              ///< Italy: Venice
    CINQUE_TERRE,        ///< Italy: Cinque Terre
    PISA,                ///< Italy: Pisa
    ROME,                ///< Italy: Rome
    SAN_MARINO,          ///< San Marino
    VATICAN,             ///< Vatican: Vatican City
    ALBEROBELLO,         ///< Italy: Alberobello
    SARDINIA,            ///< Italy: Sardinia
    MALTA,               ///< Malta: Valletta
    SA_CALOBRA,          ///< Spain: Sa Calobra
    BARCELONA,           ///< Spain: Barcelona
    ALHAMBRA,            ///< Spain: Granada
    PICOS,               ///< Spain: Picos de Europa
    LISBON,              ///< Portugal: Lisbon
    ESSAOUIRA,           ///< Morocco: Essaouira
    SIDI_BOU_SAID,       ///< Tunisia: Sidi Bou Said
    TASSILI,             ///< Algeria: Tassili N Ajjer
    GHADAMES,            ///< Libya: Ghadames
    ADRAR,               ///< Mauritania: Adrar
    MEROE,               ///< Sudan: Meroe
    LUXOR,               ///< Egypt: Luxor
    FOGO,                ///< Cape Verde: Fogo
    PINK_LAKE,           ///< Senegal: Lac Rose
    GAMBIA_RIVER,        ///< Gambia: Gambia River
    FOUTA_DJALLON,       ///< Guinea: Fouta Djallon
    BIJAGOS,             ///< Guinea-bissau: Bijagos Islands
    FREETOWN,            ///< Sierra Leone: Freetown Peninsula
    LIBERIAN_COAST,      ///< Liberia: Robertsport
    YAMOUSSOUKRO,        ///< Ivory Coast: Yamoussoukro
    TIEBELE,             ///< Burkina Faso: Tiebele
    AGADEZ,              ///< Niger: Agadez
    ZUMA_ROCK,           ///< Nigeria: Zuma Rock
    LAGOS,               ///< Nigeria: Lagos
    KOUTAMMAKOU,         ///< Togo: Koutammakou
    GANVIE,              ///< Benin: Ganvie
    MOUNT_CAMEROON,      ///< Cameroon: Mount Cameroon
    ENNEDI,              ///< Chad: Ennedi
    DZANGA_SANGHA,       ///< Central African Republic: Dzanga-sangha
    LOANGO,              ///< Gabon: Loango
    CONGO_RAPIDS,        ///< Congo: Congo Rapids
    BIOKO,               ///< Equatorial Guinea: Bioko
    PICO_CAO_GRANDE,     ///< Sao Tome and Principe: Pico Cao Grande
    MOON_VIEWPOINT,      ///< Angola: Miradouro da Lua
    MURCHISON,           ///< Uganda: Murchison Falls
    TANGANYIKA,          ///< Burundi: Lake Tanganyika
    SUDD,                ///< South Sudan: Sudd
    ERITREAN_HIGHLANDS,  ///< Eritrea: Asmara Road
    LAKE_ASSAL,          ///< Djibouti: Lake Assal
    LAAS_GEEL,           ///< Somalia: Laas Geel
    NGORONGORO,          ///< Tanzania: Ngorongoro
    BAZARUTO,            ///< Mozambique: Bazaruto
    LAKE_MALAWI,         ///< Malawi: Lake Malawi
    GREAT_ZIMBABWE,      ///< Zimbabwe: Great Zimbabwe
    SANI_PASS,           ///< Lesotho: Sani Pass
    EZULWINI,            ///< Eswatini: Ezulwini Valley
    CHAMAREL,            ///< Mauritius: Chamarel
    KARTHALA,            ///< Comoros: Grande Comore
    CHAPMANS_PEAK,       ///< South Africa: Chapmans Peak
    GARDEN_ROUTE,        ///< South Africa: Garden Route
    KRUGER,              ///< South Africa: Kruger
    ETOSHA,              ///< Namibia: Etosha
    DOHA,                ///< Qatar: Doha
    KING_FAHD,           ///< Bahrain: King Fahd Causeway
    KUWAIT_CITY,         ///< Kuwait: Kuwait City
    MESOPOTAMIA,         ///< Iraq: Mesopotamian Marshes
    NEGEV,               ///< Israel: Negev
    BETHLEHEM,           ///< Palestine: Bethlehem Hills
    PALMYRA,             ///< Syria: Palmyra
    CYPRUS,              ///< Cyprus: Troodos
    PAMUKKALE,           ///< Turkey: Pamukkale
    ATHENS,              ///< Greece: Athens
    ARARAT,              ///< Armenia: Ararat Plain
    CASPIAN,             ///< Azerbaijan: Caspian Coast
    DARVAZA,             ///< Turkmenistan: Darvaza
    BAND_E_AMIR,         ///< Afghanistan: Band-e Amir
    ALBANIAN_RIVIERA,    ///< Albania: Albanian Riviera
    LAKE_OHRID,          ///< North Macedonia: Lake Ohrid
    KOSOVO,              ///< Kosovo: Rugova Gorge
    MOSTAR,              ///< Bosnia: Mostar
    LAKE_BLED,           ///< Slovenia: Lake Bled
    IRON_GATES,          ///< Serbia: Iron Gates
    ORHEI,               ///< Moldova: Orheiul Vechi
    TRANSFAGARASAN,      ///< Romania: Transfagarasan
    SPIS,                ///< Slovakia: Spis Castle
    BELOVEZHSKAYA,       ///< Belarus: Belovezhskaya Pushcha
    LAHEMAA,             ///< Estonia: Lahemaa
    GAUJA,               ///< Latvia: Gauja Valley
    ST_PETERSBURG,       ///< Russia: Saint Petersburg
    MOSCOW,              ///< Russia: Moscow
    CHUYSKY,             ///< Russia: Chuysky Trakt
    ROAD_OF_BONES,       ///< Russia: Road of Bones
    CHARYN,              ///< Kazakhstan: Charyn Canyon
    SPITI,               ///< India: Spiti Valley
    MANALI,              ///< India: Manali Highway
    PUNJAB,              ///< India: Punjab
    HAMPI,               ///< India: Hampi
    MEGHALAYA,           ///< India: Meghalaya
    SUNDARBANS,          ///< Bangladesh: Sundarbans
    SIGIRIYA,            ///< Sri Lanka: Sigiriya
    BANGKOK,             ///< Thailand: Bangkok
    HA_GIANG,            ///< Vietnam: Ha Giang Loop
    KUALA_LUMPUR,        ///< Malaysia: Kuala Lumpur
    BRUNEI,              ///< Brunei: Bandar Seri Begawan
    BOROBUDUR,           ///< Indonesia: Borobudur
    PALAWAN,             ///< Philippines: Palawan
    DILI,                ///< East Timor: Dili
    TAROKO,              ///< Taiwan: Taroko Gorge
    TIANMEN,             ///< China: Tianmen Mountain
    ZHANGYE,             ///< China: Rainbow Mountains
    GUOLIANG,            ///< China: Guoliang Tunnel
    HUANGSHAN,           ///< China: Huangshan
    BEIJING,             ///< China: Beijing
    HARBIN,              ///< China: Harbin
    KUMGANG,             ///< North Korea: Kumgang Mountains
    SEOUL,               ///< South Korea: Seoul
    HAKONE,              ///< Japan: Hakone Turnpike
    IROHAZAKA,           ///< Japan: Irohazaka
    MIYAJIMA,            ///< Japan: Miyajima
    OKINAWA,             ///< Japan: Okinawa
    ROCK_ISLANDS,        ///< Palau: Rock Islands
    NAN_MADOL,           ///< Micronesia: Nan Madol
    MAJURO,              ///< Marshall Islands: Majuro
    TARAWA,              ///< Kiribati: Tarawa
    NAURU,               ///< Nauru
    PNG_HIGHLANDS,       ///< Papua New Guinea: Highlands
    MAROVO,              ///< Solomon Islands: Marovo Lagoon
    YASUR,               ///< Vanuatu: Mount Yasur
    SYDNEY,              ///< Australia: Sydney
    SURFERS_PARADISE,    ///< Australia: Gold Coast
    KIMBERLEY,           ///< Australia: Gibb River Road
    NULLARBOR,           ///< Australia: Nullarbor Plain
    TASMANIA,            ///< Australia: Cradle Mountain
    CROWN_RANGE,         ///< New Zealand: Crown Range
    CORAL_COAST,         ///< Fiji: Coral Coast
    FUNAFUTI,            ///< Tuvalu: Funafuti
    LALOMANU,            ///< Samoa: Lalomanu
    TONGA_BLOWHOLES,     ///< Tonga: Mapu A Vaea
    TORRES_DEL_PAINE,    ///< Chile: Torres del Paine
    PERITO_MORENO,       ///< Argentina: Perito Moreno
    RUTA_40,             ///< Argentina: Ruta 40
    VALPARAISO,          ///< Chile: Valparaiso
    CARACOLES,           ///< Chile: Paso Los Caracoles
    BUENOS_AIRES,        ///< Argentina: Buenos Aires
    PUNTA_DEL_ESTE,      ///< Uruguay: Punta del Este
    CHACO,               ///< Paraguay: Gran Chaco
    YUNGAS,              ///< Bolivia: Yungas Road
    RAINBOW_MOUNTAIN,    ///< Peru: Vinicunca
    NAZCA,               ///< Peru: Nazca Desert
    GALAPAGOS,           ///< Ecuador: Galapagos
    CARTAGENA,           ///< Colombia: Cartagena
    KAIETEUR,            ///< Guyana: Kaieteur Falls
    SURINAME_RIVER,      ///< Suriname: Suriname River
    RIO_DO_RASTRO,       ///< Brazil: Serra do Rio do Rastro
    SAO_PAULO,           ///< Brazil: Sao Paulo
    TRANSAMAZONICA,      ///< Brazil: Transamazonica
    HUMAHUACA,           ///< Argentina: Quebrada de Humahuaca
    IGUAZU,              ///< Argentina: Iguazu Falls
    MARACAS_BAY,         ///< Trinidad and Tobago: Maracas Bay
    GRAND_ANSE,          ///< Grenada: Grand Anse
    LA_SOUFRIERE,        ///< Saint Vincent: La Soufriere
    BATHSHEBA,           ///< Barbados: Bathsheba
    PITONS,              ///< Saint Lucia: The Pitons
    BOILING_LAKE,        ///< Dominica: Morne Trois Pitons
    ENGLISH_HARBOUR,     ///< Antigua and Barbuda: English Harbour
    BRIMSTONE_HILL,      ///< Saint Kitts and Nevis: Brimstone Hill
    PUNTA_CANA,          ///< Dominican Republic: Punta Cana
    CITADELLE,           ///< Haiti: Citadelle Laferriere
    EXUMA,               ///< Bahamas: Exuma Cays
    PANAMA_CANAL,        ///< Panama: Panama Canal
    OMETEPE,             ///< Nicaragua: Ometepe
    RUTA_DE_LAS_FLORES,  ///< El Salvador: Ruta de Las Flores
    COPAN,               ///< Honduras: Copan
    BLUE_HOLE,           ///< Belize: Barrier Reef
    RIVIERA_MAYA,        ///< Mexico: Tulum
    OAXACA,              ///< Mexico: Hierve El Agua
    MEXICO_CITY,         ///< Mexico: Mexico City
    COPPER_CANYON,       ///< Mexico: Copper Canyon
    LA_FREEWAY,          ///< Usa: Los Angeles
    HOLLYWOOD,           ///< Usa: Hollywood Hills
    NAPA,                ///< Usa: Napa Valley
    SAN_FRANCISCO,       ///< Usa: San Francisco
    OREGON_COAST,        ///< Usa: Oregon Coast
    SEATTLE,             ///< Usa: Seattle
    GOING_TO_THE_SUN,    ///< Usa: Going To The Sun Road
    TETONS,              ///< Usa: Grand Teton
    BONNEVILLE,          ///< Usa: Bonneville Salt Flats
    SEDONA,              ///< Usa: Sedona
    WHITE_SANDS,         ///< Usa: White Sands
    PIKES_PEAK,          ///< Usa: Pikes Peak
    MILLION_DOLLAR,      ///< Usa: Million Dollar Highway
    MOUNT_RUSHMORE,      ///< Usa: Black Hills
    CHICAGO,             ///< Usa: Chicago
    NEW_ORLEANS,         ///< Usa: Lake Pontchartrain
    TAIL_OF_THE_DRAGON,  ///< Usa: Tail of The Dragon
    BLUE_RIDGE,          ///< Usa: Blue Ridge Parkway
    WASHINGTON_DC,       ///< Usa: Washington
    HANA,                ///< Hawaii: Road To Hana
    DALTON,              ///< Alaska: Dalton Highway
    YUKON,               ///< Canada: Klondike Highway
    ICEFIELDS,           ///< Canada: Icefields Parkway
    SEA_TO_SKY,          ///< Canada: Sea To Sky Highway
    NIAGARA,             ///< Canada: Niagara Falls
    TORONTO,             ///< Canada: Toronto
    QUEBEC_CITY,         ///< Canada: Quebec City
    CABOT_TRAIL,         ///< Canada: Cabot Trail
    NORTH_CAPE,          ///< Norway: North Cape
    TROLLSTIGEN,         ///< Norway: Trollstigen
    ATLANTIC_ROAD,       ///< Norway: Atlantic Road
    ORESUND,             ///< Sweden: Oresund Bridge
    JOKULSARLON,         ///< Iceland: Jokulsarlon
    COUNT            ///< the number of biomes
};

/** @brief The kinds of objects beside the road. */
enum class PropKindE : uint8_t
{
    PINE,          ///< conifer
    SNOW_PINE,     ///< conifer with snow
    OAK,           ///< round deciduous tree
    BUSH,          ///< low shrub
    ROCK,          ///< stone
    BOULDER,       ///< big stone
    CACTUS,        ///< saguaro
    MESA,          ///< layered rock tower
    PALM,          ///< palm tree
    DEAD_TREE,     ///< bare tree
    REEDS,         ///< tuft of reeds
    LAMP,          ///< street lamp leaning over the road
    BUILDING,      ///< block with lit windows
    TOWER,         ///< high-rise
    BILLBOARD,     ///< lit advertising board
    POLE,          ///< telegraph pole
    FENCE,         ///< a fence panel along the road
    HAYBALE,       ///< round bale
    HOUSE,         ///< house with a pitched roof
    WINDMILL,      ///< wind turbine
    NEON_PILLAR,   ///< glowing pylon
    PYRAMID,       ///< glowing wireframe-ish pyramid
    SNOWBANK,      ///< heap of snow
    AUTUMN_TREE,   ///< deciduous tree in autumn colours
    BIRCH,         ///< white birch
    CHERRY,        ///< cherry tree in blossom
    TORII,         ///< red shrine gate
    LANTERN,       ///< stone lantern
    PAGODA,        ///< tiered tower
    JUNGLE_TREE,   ///< tall rainforest tree
    FERN,          ///< fern
    ACACIA,        ///< flat-topped acacia
    TERMITE_MOUND, ///< termite mound
    GRASS_TUFT,    ///< tuft of dry grass
    LAVA_ROCK,     ///< black rock with glowing cracks
    ICE_SPIKE,     ///< ice crystal spike
    CYPRESS,       ///< slim cypress
    DUTCH_MILL,    ///< traditional windmill
    RUIN,          ///< ruined tower
    STONE_WALL,    ///< dry stone wall along the road
    CABIN,         ///< red wooden cabin
    BAMBOO,        ///< bamboo cluster
    HUT,           ///< stilt hut
    BUTTE,         ///< tall slim rock tower
    MAILBOX,       ///< mailbox at a driveway
    CONTAINERS,    ///< stacked shipping containers
    CRANE,         ///< harbour crane
    CHIMNEY,       ///< factory chimney
    NEON_TOWER,    ///< high-rise with neon strips
    CRATER,        ///< crater rim
    DOME,          ///< habitat dome
    ANTENNA,       ///< dish antenna
    CRYSTAL,       ///< glowing crystal cluster
    MUSHROOM,      ///< giant mushroom
    GLOW_SHROOM,   ///< small glowing mushrooms
    ICEBERG,       ///< iceberg (on the water)
    MOSS_TREE,     ///< swamp cypress with hanging moss
    FIREFLIES,     ///< a swarm of fireflies
    BARN,          ///< red barn
    SILO,          ///< grain silo
    LIGHTHOUSE,    ///< lighthouse
    PUMPKIN,       ///< glowing jack-o'-lantern
    GRAVESTONE,    ///< gravestone
    LIGHT_TREE,    ///< pine with festive lights
    SNOW_HOUSE,    ///< house with a snowy roof and lit windows
    // The world tour's props (ModelsWorld.cpp). Keyed colours follow the rule's colours.
    OLIVE,          ///< gnarled olive tree
    POPLAR,         ///< slim Lombardy poplar
    UMBRELLA_PINE,  ///< stone pine
    SPRUCE,         ///< slender dark spruce
    BAOBAB,         ///< baobab
    EUCALYPTUS,     ///< pale trunk, sparse crown
    JOSHUA_TREE,    ///< Joshua tree
    AGAVE,          ///< blue agave
    PRICKLY_PEAR,   ///< opuntia
    BANANA,         ///< banana plant
    CEDAR,          ///< Lebanon cedar, flat tiers
    REDWOOD,        ///< giant redwood
    WAX_PALM,       ///< very tall slim palm
    DRAGON_TREE,    ///< dragon's blood tree
    TREE_FERN,      ///< tree fern
    LUPIN,          ///< flower spikes (keyed: blossom)
    FLOWER_BUSH,    ///< flowering bush (keyed: blossom)
    SHRUB,          ///< low bush (keyed: leaves)
    BROADLEAF,      ///< round tree (keyed: leaves)
    VINE_ROW,       ///< a row of vines along the road
    TEA_ROW,        ///< a row of tea bushes
    DATE_PALM,      ///< date palm
    MANGROVE,       ///< mangrove on its roots
    SUPERTREE,      ///< Singapore's supertree
    KARST,          ///< limestone tower with green (keyed: rock, green)
    SEA_STACK,      ///< rock in the sea (keyed: rock)
    FAIRY_CHIMNEY,  ///< cone with a cap
    PILLAR,         ///< sandstone pillar with trees on top
    ARCH,           ///< natural stone arch (keyed: rock)
    DUNE,           ///< sand dune (keyed: sand)
    KOPJE,          ///< granite boulder pile
    CHALK_ROCK,     ///< white chalk mushroom
    MENHIR,         ///< standing stone
    MOAI,           ///< moai
    MONOLITH,       ///< a great lone rock (keyed: rock)
    TEPUI,          ///< table mountain (keyed: cliff, top)
    VOLCANO_CONE,   ///< volcano (keyed: rock, snow cap)
    GRANITE_DOME,   ///< granite dome (keyed: rock)
    HILLOCK,        ///< round hill (keyed: grass)
    BADLAND,        ///< eroded striped hill (keyed: two bands)
    GEYSER,         ///< steam column
    TERRACES,       ///< terraced hillside (keyed: crop, wall)
    PEAK,           ///< rock spire with snow (keyed: rock, snow)
    COTTAGE,        ///< small gabled house (keyed: wall, roof, windows)
    FLAT_HOUSE,     ///< cube house with a flat roof (keyed: wall, trim, windows)
    CHALET,         ///< wide-roofed wooden house (keyed: wall, roof, windows)
    HALF_TIMBER,    ///< half-timbered house (keyed: plaster, roof, windows)
    ROW_HOUSES,     ///< a terrace of houses (keyed: wall, roof, windows)
    CHURCH,         ///< church with a spire (keyed: wall, roof)
    ONION_CHURCH,   ///< church with onion domes (keyed: wall, domes)
    MOSQUE,         ///< dome and minaret (keyed: wall, dome)
    STUPA,          ///< stupa (keyed: body, spire)
    WAT,            ///< Thai temple
    TEMPLE_TOWER,   ///< Khmer or Hindu temple tower (keyed: stone)
    STEP_PYRAMID,   ///< Maya pyramid (keyed: stone)
    STONE_PYRAMID,  ///< Egyptian pyramid
    KASBAH,         ///< earthen fortress (keyed: earth)
    ADOBE,          ///< adobe house (keyed: earth)
    YURT,           ///< yurt (keyed: felt, band)
    RONDAVEL,       ///< round hut (keyed: wall, thatch)
    HANOK,          ///< Korean house with a curved roof
    GASSHO,         ///< steep thatched farmhouse
    HUIZHOU_HOUSE,  ///< white walls, black stepped gables
    CASTLE,         ///< castle (keyed: stone, roofs)
    COLUMNS,        ///< ruined temple columns
    AQUEDUCT,       ///< arcade of an aqueduct (keyed: stone)
    ROUND_MILL,     ///< white round windmill
    WATCHTOWER,     ///< stone tower with a wall (keyed: stone)
    FORT,           ///< fortress (keyed: wall, band)
    CAPE_DUTCH,     ///< white gabled farmhouse
    GRAIN_ELEVATOR, ///< prairie grain elevator (keyed: wall)
    WATER_TOWER,    ///< water tower on legs (keyed: tank)
    PUMPJACK,       ///< oil pump
    WIND_PUMP,      ///< farm wind pump
    DINER,          ///< roadside diner with its sign
    MOTEL_SIGN,     ///< tall motel sign
    GAS_STATION,    ///< filling station (keyed: canopy)
    CASINO,         ///< casino hotel, lit
    GLASS_TOWER,    ///< glass high-rise (keyed: glass, -, windows)
    SUPERTALL,      ///< needle of a supertall tower
    TV_TOWER,       ///< concrete tower with spheres
    LATTICE_TOWER,  ///< iron lattice tower (keyed: paint)
    PANEL_BLOCK,    ///< panel block of flats (keyed: wall, -, windows)
    FAVELA,         ///< stacked hillside houses
    WATER_VILLA,    ///< bungalow on stilts in the lagoon
    TEMPLE_GATE,    ///< Balinese split gate
    PRAYER_FLAGS,   ///< lines of prayer flags
    WAYSIDE_CROSS,  ///< wayside cross or shrine
    PARASOL,        ///< beach umbrella (keyed: cloth)
    HEDGE,          ///< hedgerow along the road
    RANCH_FENCE,    ///< white ranch fence along the road
    POWER_PYLON,    ///< lattice power pylon
    BALLOON,        ///< hot air balloon in the sky (keyed: envelope)
    SAILBOAT,       ///< sailing boat (keyed: sail)
    BOAT,           ///< small boat (keyed: hull)
    HOUSEBOAT,      ///< houseboat
    COW,            ///< cow
    SHEEP,          ///< sheep
    HORSE,          ///< horse
    CAMEL,          ///< camel
    ELEPHANT,       ///< elephant
    GIRAFFE,        ///< giraffe
    ZEBRA,          ///< zebra
    KANGAROO,       ///< kangaroo
    LLAMA,          ///< llama
    BISON,          ///< bison
    MOOSE,          ///< moose
    REINDEER,       ///< reindeer
    YAK,            ///< yak
    PANDA,          ///< panda
    FLAMINGO,       ///< flamingo
    LANDMARK,       ///< a famous building or monument (variant: which one; see ModelsWorld.cpp)
    WATERFALL,      ///< a cliff with falling water (keyed: rock, water)
    BASALT_COLUMNS, ///< hexagonal basalt columns
    STONE_CIRCLE,   ///< a ring of trilithons
    SHIP,           ///< cargo ship (keyed: hull)
    REED_HOUSE,     ///< arched reed hall of the marshes
    PIPELINE,       ///< raised pipeline along the road
    WARNING_SIGN,   ///< yellow animal warning sign (variant: kangaroo, camel, moose, cow, deer)
    TORTOISE,       ///< giant tortoise
    TRULLO,         ///< whitewashed hut with a stone cone
    COUNT          ///< the number of kinds
};

/** @brief One rule for scattering props beside the road. */
struct PropRule
{
    PropKindE m_kind;            ///< what to place
    float     m_chance;          ///< probability per segment and side
    float     m_nearest;         ///< least distance from the road's edge (m)
    float     m_farthest;        ///< largest distance from the road's edge (m)
    float     m_scaleMin;        ///< smallest scale
    float     m_scaleMax;        ///< largest scale
    bool      m_onWater = false; ///< floats on the sea (icebergs): placed only on a water side, at sea level
    Color     m_a{};             ///< keyed colour one (alpha 0: the kind's own)
    Color     m_b{};             ///< keyed colour two
    Color     m_c{};             ///< keyed colour three (windows: dark by day, warm at night)
    bool      m_pastel = false;  ///< colour one changes per object (painted rows of houses)
    int16_t   m_variant = -1;    ///< one of the kind's variants (-1: any)
    uint8_t   m_perStage = 0;    ///< placed this many times per stage at fixed spots instead of by chance (landmarks)
};

/** @brief How a bridge is built. */
enum class BridgeE : uint8_t
{
    BEAM,          ///< concrete deck on pillar pairs
    VIADUCT,       ///< tall slim piers over a deep valley
    ARCH,          ///< stone arches
    TRUSS,         ///< steel truss over the deck
    SUSPENSION,    ///< towers and cables
    CABLE_STAYED,  ///< one pylon, a fan of stays
    COVERED,       ///< wooden, roofed
    CAUSEWAY,      ///< low and long over the sea
};

/** @brief Light and air: everything the renderer blends between biomes. */
struct Environment
{
    Color m_skyZenith;          ///< sky straight up
    Color m_skyHorizon;         ///< sky at the horizon
    Color m_sunColor;           ///< sun (or moon) light
    float m_sunIntensity;       ///< its strength
    float m_sunElevation;       ///< its height above the horizon (degrees)
    float m_sunAzimuth;         ///< its direction (degrees, 0 = north, the track's mean direction)
    float m_sunSize;            ///< apparent radius (degrees)
    float m_sunStripes;         ///< 0..1 synthwave stripes across the sun's lower half
    Color m_ambientTop;         ///< light from above
    Color m_ambientBottom;      ///< light from below
    Color m_fogColor;           ///< haze colour
    float m_fogDensity;         ///< haze per metre
    Color m_mountainNear;       ///< nearer silhouette range
    Color m_mountainFar;        ///< farther range
    float m_mountainHeight;     ///< their height (degrees)
    float m_cloudAmount;        ///< 0..1 cover
    Color m_cloudColor;         ///< cloud colour
    float m_stars;              ///< 0..1 star field
    float m_headlights;         ///< 0..1 how much the headlights matter
    float m_rain;               ///< 0..1 rain
    float m_snow;               ///< 0..1 snowfall
    float m_lightning;          ///< 0..1 lightning frequency
    float m_exposure;           ///< brightness before tone mapping
    float m_saturation;         ///< colour grading
    float m_bloom;              ///< bloom strength
    float m_aurora;             ///< 0..1 northern lights
    float m_rainbow;            ///< 0..1 rainbow opposite the sun
    float m_eclipse;            ///< 0..1 the moon covers the sun (corona)
    float m_planetSize;         ///< apparent radius of a planet in the sky (degrees, 0: none)
    float m_planetElevation;    ///< its height (degrees)
    float m_planetAzimuth;      ///< its direction (degrees)
    Color m_planetColor;        ///< its colour
    float m_dust;               ///< 0..1 blowing sand (a sandstorm)
};

/** @brief The vehicles a region adds to the everyday traffic. */
enum class FleetE : uint8_t
{
    EUROPE,          ///< tractors, camper vans
    NORTH_AMERICA,   ///< yellow cabs, school buses, pickups, big rigs
    BRITISH,         ///< black cabs, red double-deckers
    LATIN,           ///< beetles, painted chicken buses, pickups
    SOUTH_ASIA,      ///< auto rickshaws, decorated trucks, scooters
    SOUTHEAST_ASIA,  ///< tuk-tuks, scooters, jeepneys, minibuses
    EAST_ASIA,       ///< kei trucks, scooters
    OCEANIA,         ///< road trains, utes, camper vans
    AFRICA,          ///< matatus, pickups, land cruisers
    ARABIA,          ///< white land cruisers, pickups
    EASTERN,         ///< boxy saloons, marshrutkas, tractors
    ISLANDS,         ///< minibuses, pickups
    COUNT            ///< number of fleets
};

/** @brief A country's roads: the side, the vehicles, the markings and signs, the flag, the radio (data/countries.txt). */
struct CountryInfo
{
    const char *m_name;       ///< as the places write it
    bool        m_left;       ///< drives on the left
    FleetE      m_fleet;      ///< its vehicles
    uint8_t     m_lines;      ///< 0 white, 1 a yellow line on the centre side, 2 a yellow outer edge
    uint8_t     m_warn;       ///< warning signs: 0 red triangle, 1 red triangle on yellow, 2 yellow diamond
    uint8_t     m_chevrons;   ///< chevron boards: 0 red and white, 1 black and white, 2 yellow and black
    uint8_t     m_townSign;   ///< place signs: 0 white, 1 yellow, 2 white with a red border, 3 blue, 4 green
    const char *m_flag;       ///< the flag (data/countries.txt tells how it is written)
    int         m_station;    ///< the radio station its countryside plays in AUTO (-1: the genre stations)
};

/**
 * @brief A country's roads.
 * @param[in] country  its name (nullptr: a place off the map)
 * @return its entry, or Europe's for none
 */
const CountryInfo &countryInfo( const char *country );

/**
 * @brief A country's place in the table (the flags are built in that order).
 * @param[in] country  its name
 * @return 0..countryTableSize()-1, or -1
 */
int countryIndex( const char *country );

/**
 * @brief How many countries the table holds.
 * @return the number
 */
int countryTableSize();

/**
 * @brief A country by its place in the table.
 * @param[in] index  0..countryTableSize()-1
 * @return its entry
 */
const CountryInfo &countryAt( int index );

/** @brief What weather a place gets when a front passes. */
enum class ClimateE : uint8_t
{
    NONE,        ///< none (the fantasy biomes)
    TEMPERATE,   ///< rain, fog, now and then a thunderstorm
    TROPICAL,    ///< downpours and thunderstorms
    ARID,        ///< sandstorms
    COLD,        ///< snow showers
};

/** @brief Colours of the road and the ground. */
struct Surface
{
    Color m_groundA;            ///< ground, even stripes
    Color m_groundB;            ///< ground, odd stripes
    Color m_groundFar;          ///< ground far from the road
    Color m_road;               ///< asphalt
    Color m_roadAlt;            ///< asphalt, alternate bands
    Color m_rumbleA;            ///< kerb colour one
    Color m_rumbleB;            ///< kerb colour two
    Color m_line;               ///< lane markings
    Color m_shoulder;           ///< gravel beside the kerb
    Color m_water;              ///< sea or ponds
    float m_lineGlow;           ///< 0..1 emissive markings (neon)
    float m_gridGlow;           ///< 0..1 glowing ground grid (neon)
    Color m_fieldA{};           ///< field colour one (lavender, tulips, wheat)
    Color m_fieldB{};           ///< field colour two
    Color m_fieldC{};           ///< field colour three
    float m_fields = 0.0f;      ///< 0..1 share of the land laid out as fields
    float m_waterGlow = 0.0f;   ///< 0..1 self-lit water (lava)
    float m_groundGloss = 0.0f; ///< 0..1 shiny ground (salt flats)
    float m_waterGloss = 0.95f; ///< 0..1 how mirror-like the water is (a sea of clouds is matte)
};

/** @brief How the land beside the road rises and falls. */
struct Terrain
{
    float m_hills;               ///< amplitude of the far hills (m)
    float m_wallLeft;            ///< cliff or embankment height on the left (m, negative: drops away)
    float m_wallRight;           ///< and on the right
    float m_waterLeft;           ///< 0..1 sea on the left
    float m_ponds;               ///< 0..1 ponds in the near ground
    float m_waterRight = 0.0f;   ///< 0..1 sea on the right
    float m_seaLevel = -4.5f;    ///< the sea's height below the road (m)
};

/** @brief A biome: its look and the style of its roads. */
struct BiomeDef
{
    const char           *m_name;               ///< shown when the stage begins
    Environment           m_env;                ///< light and air
    Surface               m_surface;            ///< colours
    Terrain               m_terrain;            ///< land shape
    float                 m_grip;               ///< tyre grip (1 dry asphalt)
    float                 m_curviness;          ///< 0..1 how often the road bends
    float                 m_sharpness;          ///< 0..1 how tight the bends are
    float                 m_hilliness;          ///< 0..1 how much it climbs and falls
    int                   m_lanes;              ///< lanes in one direction
    float                 m_traffic;            ///< traffic density factor
    bool                  m_railLeft;           ///< guard rail on the left
    bool                  m_railRight;          ///< and on the right
    float                 m_postSpacing;        ///< delineator posts every n metres (0: none)
    float                 m_lampSpacing;        ///< street lamps every n metres (0: none)
    std::vector<PropRule> m_props;              ///< scatter rules
    float                 m_gravity = 1.0f;     ///< gravity factor (the Moon: low)
    bool                  m_walls = false;      ///< the rails are tall concrete sound walls
    float                 m_gantrySpacing = 0;  ///< overhead sign gantries every n metres (0: none)
    int                   m_station = 0;        ///< the radio station this biome plays in automatic mode
    float                 m_tunnels = 0.0f;     ///< chance that a stretch runs through a tunnel
    float                 m_bridges = 0.0f;     ///< chance that a straight crosses a valley on a bridge
    const char           *m_country = nullptr;  ///< where on the world tour (none: off the map)
    BridgeE               m_bridge = BridgeE::BEAM;               ///< how its bridges are built
    Color                 m_bridgeColor = { 150, 150, 154, 255 }; ///< their paint or stone
    ClimateE              m_climate = ClimateE::NONE;             ///< the weather fronts it gets
    uint8_t               m_unpaved = 0;                          ///< 0 asphalt, 1 gravel or dirt, 2 sand
    const char           *m_region = nullptr;                     ///< its state or province (none: the whole country)
};

constexpr int MaxBiomes = 4096;   ///< compiled biomes and places of the data files together (records and codes have room)

/**
 * @brief How many biomes there are: the compiled ones and the places of the data files.
 * @return the number; every BiomeE below it is valid
 */
int biomeCount();

/**
 * @brief A biome's number by its ID in the data files (the enum's name for the compiled ones).
 * @param[in] key  e.g. "TUSCANY"
 * @return its number, or -1
 */
int biomeId( std::string_view key );

/**
 * @brief A biome's ID in the data files.
 * @param[in] biome  which one
 * @return its ID (empty for none)
 */
const std::string &biomeKey( BiomeE biome );

/**
 * @brief Reads the places of the data files into the table (WorldData.cpp).
 * @param[in,out] table  the biome table; grows by the new places
 */
void addDataPlaces( std::vector<BiomeDef> &table );

/**
 * @brief How many steps, names and lines the data files got wrong (0 for a clean world).
 * @return the count
 */
int dataErrors();

/**
 * @brief How many landmarks are compiled (ModelsWorld.cpp); those of data/landmarks follow them.
 * @return the number
 */
int compiledLandmarks();

/**
 * @brief The definition of a biome.
 * @param[in] biome  which one
 * @return its definition (static storage)
 */
const BiomeDef &biomeDef( BiomeE biome );

/**
 * @brief The world tour: every biome with a country, in the order of a trip round the world.
 * @return the route (static storage)
 */
const std::vector<BiomeE> &worldRoute();

/** @brief A famous long-distance route: a chain of places driven in one go. */
struct Rally
{
    const char         *m_name;     ///< e.g. "PARIS - DAKAR"
    const char         *m_sub;      ///< where it goes
    std::vector<BiomeE> m_stages;   ///< a place per stage
};

/**
 * @brief The rallies (Rallies.cpp).
 * @return them, in the menu's order
 */
const std::vector<Rally> &rallies();

/** @brief Where the world tour may go after a place: the next on the route, and now and then a neighbour further on. */
struct RouteFork
{
    int m_main = -1;    ///< the next route position
    int m_other = -1;   ///< another route position across a border (-1: no fork here)
    bool m_otherLeft = false;   ///< the other road leaves to the left (as the map lies)
};

/**
 * @brief The fork after a route position (data/route.txt; at the borders of countries and states).
 * @param[in] pos  the position in worldRoute()
 * @return the choices
 */
RouteFork routeFork( int pos );

/**
 * @brief A biome's place on the world tour.
 * @param[in] biome  which one
 * @return its index in worldRoute(), -1 if it is not on the route
 */
int routeIndex( BiomeE biome );

/**
 * @brief A biome's name with its country ("TUSCANY - ITALY").
 * @param[in] biome  which one
 * @return the text for the screen
 */
std::string placeName( BiomeE biome );

/**
 * @brief What a sign at a border calls a place: its state if it lies in the same country, else its country.
 * @param[in] dest  where the road leads
 * @param[in] from  where it comes from
 * @return the name
 */
std::string borderName( BiomeE dest, BiomeE from );

/**
 * @brief How many countries the world tour visits.
 * @return the number of different countries
 */
int countryCount();

/**
 * @brief Fills in the world tour's biomes (WorldBiomes.cpp).
 * @param[in,out] table  the biome table, indexed by BiomeE; grows by the places of the data files
 */
void addWorldBiomes( std::vector<BiomeDef> &table );

/** @brief A point on the globe. */
struct GeoPoint
{
    float m_lat = 0.0f;   ///< degrees north
    float m_lon = 0.0f;   ///< degrees east
};

constexpr int LongitudeSectors = 12;   ///< the globe in slices of 30 degrees: one place in each is a circumnavigation

/**
 * @brief Where a place of the world tour is (data/route.txt).
 * @param[in]  biome  the place
 * @param[out] where  its latitude and longitude
 * @return false for the places that are not on the map
 */
bool placeLocation( BiomeE biome, GeoPoint &where );

/**
 * @brief Which slice of the globe a longitude falls in.
 * @param[in] lon  degrees east
 * @return 0..LongitudeSectors-1, eastward from the date line
 */
int longitudeSector( float lon );

/**
 * @brief A place's light at another hour (the tour clock, or a fixed time of day): the sky of the hour, the place's own
 *        air, ranges, rain and snow kept (WorldBiomes.cpp).
 * @param[in] place  the place's environment as written
 * @param[in] hour   local time, 0..24
 * @return the environment
 */
Environment environmentAtHour( const Environment &place, float hour );

/**
 * @brief Blends two environments.
 * @param[in] a  at t = 0
 * @param[in] b  at t = 1
 * @param[in] t  the position between them
 * @return the blend
 */
Environment blendEnvironment( const Environment &a, const Environment &b, float t );

/**
 * @brief Blends two surfaces.
 * @param[in] a  at t = 0
 * @param[in] b  at t = 1
 * @param[in] t  the position between them
 * @return the blend
 */
Surface blendSurface( const Surface &a, const Surface &b, float t );

/**
 * @brief Blends two terrains.
 * @param[in] a  at t = 0
 * @param[in] b  at t = 1
 * @param[in] t  the position between them
 * @return the blend
 */
Terrain blendTerrain( const Terrain &a, const Terrain &b, float t );

} // namespace sunstreak
