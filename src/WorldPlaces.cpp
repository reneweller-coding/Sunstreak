/**
 * @file WorldPlaces.cpp
 * @brief The rest of the world: every remaining country, the second and third places of the big ones, the famous
 *        roads and passes, and the landmarks that stand where they belong.
 *
 * Written like WorldBiomes.cpp: a sky preset, the ground, the shape of the land, the road and the things beside it.
 * A landmark (Big Ben, the Colosseum, the Opera House) stands once per stage at a fixed spot.
 *
 * @author reneweller
 */
#include "Biome.h"

#include "Music.h"
#include "WorldBuilder.h"

namespace sunstreak
{

void addWorldPlaces( BiomeDef *b )
{
    using enum PropKindE;
    using BE = BiomeE;
    using BR = BridgeE;
    using S = SkyE;

    // ---- British Isles ---------------------------------------------------------------------------------------------
    W( b, BE::CAUSEWAY_COAST, "CAUSEWAY COAST", "NORTHERN IRELAND", S::OVERCAST ).clouds( 0.7f ).sea( 0, 1, -12 ).water( 0x2A5068 )
        .land( 0x6E9A4A, 0x648E44, 0x768A5C ).hills( 16, 6, -10 ).drive( 2, 0.4f, 0.95f, 6 ).bends( 0.6f, 0.45f, 0.5f )
        .p( BASALT_COLUMNS, 0.05f, 4, 11, 0.8f, 1.4f ).v( 0 ).p( SEA_STACK, 0.01f, 40, 160, 0.6f, 1.0f ).c( 0x3E3E42 ).floating()
        .p( STONE_WALL, 0.4f, 3, 3, 1, 1 ).p( SHEEP, 0.06f, 6, 70 ).p( COTTAGE, 0.012f, 12, 80 ).c( 0xF2F0E8, 0x3A3A42 )
        .p( RUIN, 0.004f, 30, 120 ).p( SHRUB, 0.15f, 2, 50, 0.6f, 1.0f ).c( 0x6A7A48 );
    W( b, BE::GLENCOE, "GLENCOE", "SCOTLAND", S::MIST ).rain( 0.2f ).land( 0x7A8A50, 0x6E7E48, 0x6A7458 ).ranges( 0x3A4048, 0x5E6670, 11 )
        .hills( 50, 26, 22, 0.15f ).water( 0x3A4E5A ).drive( 2, 0.3f, 0.92f, 3 ).bends( 0.45f, 0.4f, 0.4f ).bridges( 0.06f, BR::ARCH, 0x6E6C66 )
        .p( PEAK, 0.008f, 150, 400, 1.0f, 1.4f ).c( 0x4A4E48, 0x5E625C ).p( SHRUB, 0.3f, 2, 60, 0.6f, 1.0f ).c( 0x7A6A48 ).p( ROCK, 0.15f, 3, 60 )
        .p( WATERFALL, 0.004f, 60, 160, 0.3f, 0.5f ).v( 1 ).p( COTTAGE, 0.002f, 40, 160 ).c( 0xF4F2EA, 0x3A3A40 ).p( SHEEP, 0.02f, 8, 60 );
    W( b, BE::APPLECROSS, "APPLECROSS PASS", "SCOTLAND", S::OVERCAST ).clouds( 0.75f ).land( 0x86885A, 0x7A7E52, 0x6E7258 )
        .ranges( 0x4A525A, 0x7A8490, 7 ).sea( 0, 0.6f, -60 ).water( 0x3A5468 ).hills( 40, 30, -20 ).drive( 2, 0.12f, 0.93f, 3 )
        .bends( 0.95f, 0.95f, 1.0f ).posts( 25 )
        .p( ROCK, 0.25f, 3, 60 ).p( BOULDER, 0.04f, 8, 80 ).p( SHRUB, 0.2f, 2, 40, 0.5f, 0.8f ).c( 0x8A7450 ).p( SHEEP, 0.03f, 6, 50 )
        .p( PEAK, 0.004f, 200, 400, 0.8f, 1.0f ).c( 0x56544E, 0x6A6862 );
    W( b, BE::STONEHENGE, "SALISBURY PLAIN", "ENGLAND", S::OVERCAST ).clouds( 0.8f ).land( 0x7EA452, 0x749A4A, 0x8AA060 )
        .fields( 0.4f, 0xC8B860, 0x88A850, 0xA89A58 ).hills( 10, 3, 3 ).drive( 2, 0.7f, 1.0f, 6 ).bends( 0.3f, 0.3f, 0.35f )
        .p( STONE_CIRCLE, 0.0f, 60, 110, 1, 1 ).once().p( HILLOCK, 0.008f, 30, 150, 0.6f, 1.0f ).c( 0x6E9A48 ).p( SHEEP, 0.05f, 10, 90 )
        .p( HEDGE, 0.3f, 3, 3, 1, 1 ).p( OAK, 0.03f, 10, 120 ).p( HAYBALE, 0.01f, 12, 60 );
    W( b, BE::CORNWALL, "CORNWALL", "ENGLAND", S::MORNING ).clouds( 0.45f ).sea( 0, 1, -14 ).water( 0x2A7A8A ).land( 0x72A04C, 0x689646, 0x7A905A )
        .hills( 18, 8, -6 ).drive( 2, 0.5f, 0.97f, 6 ).bends( 0.65f, 0.5f, 0.55f ).posts( 0 )
        .p( HEDGE, 0.5f, 3, 3, 1, 1 ).p( COTTAGE, 0.03f, 8, 70 ).c( 0xF4F2EA, 0x4A4E58 ).p( CHIMNEY, 0.004f, 30, 150, 0.6f, 0.8f )
        .p( LIGHTHOUSE, 0.004f, 20, 100 ).p( BOAT, 0.02f, 40, 150 ).c( 0x2A5A8A, 0xF0F0F0 ).pastel().floating().p( FLOWER_BUSH, 0.06f, 2, 20 ).c( 0xE8C040 );

    // ---- France ------------------------------------------------------------------------------------------------------
    W( b, BE::MONT_SAINT_MICHEL, "MONT SAINT-MICHEL", "FRANCE", S::MORNING ).clouds( 0.5f ).sea( 1, 1, -1.5f ).water( 0x8A9A94, 0.6f )
        .land( 0x86A458, 0x7C9A50, 0x8A9A6A ).hills( 2, 0, 0 ).drive( 2, 0.5f, 1.0f, 6 ).bends( 0.2f, 0.2f, 0.05f ).bridges( 0.4f, BR::CAUSEWAY, 0xC8C4B8 )
        .landmark( Lm::MONT_ST_MICHEL, 220, 320 ).floating().p( SHEEP, 0.06f, 3, 12 ).p( SAILBOAT, 0.004f, 60, 250 ).floating();
    W( b, BE::DUNE_OF_PILAT, "DUNE DU PILAT", "FRANCE", S::GOLDEN ).sea( 0, 1, -3 ).water( 0x2A7A9A ).land( 0x6E9050, 0x648648, 0xE2D2A8 )
        .verge( 0xE0D2B0 ).hills( 6, 2, 0 ).drive( 2, 0.6f, 0.98f, 0 ).bends( 0.35f, 0.3f, 0.3f )
        .p( DUNE, 0.05f, 4, 11, 1.4f, 2.0f ).c( 0xEADAB4 ).p( PINE, 0.45f, 4, 150 ).p( UMBRELLA_PINE, 0.08f, 4, 80 ).p( PARASOL, 0.03f, 3, 11 ).pastel()
        .p( SAILBOAT, 0.01f, 60, 250 ).floating().p( COTTAGE, 0.006f, 10, 60 ).c( 0xF4F0E4, 0xB8584A );
    W( b, BE::ALPE_D_HUEZ, "ALPE D HUEZ", "FRANCE", S::ALPINE ).land( 0x6AA44A, 0x609842, 0x749660 ).ranges( 0x66748A, 0xEAEEF6, 13 )
        .hills( 50, 30, -24 ).drive( 2, 0.4f, 0.98f, 3 ).bends( 0.95f, 0.95f, 1.0f ).rails()
        .p( PEAK, 0.005f, 220, 480, 1.2f, 1.6f ).c( 0x7A7C82, 0xF4F6FA ).p( SPRUCE, 0.25f, 6, 120 ).p( CHALET, 0.02f, 10, 70 ).c( 0x8A5A36, 0x4A3A30 )
        .p( FLOWER_BUSH, 0.04f, 2, 20, 0.4f, 0.6f ).c( 0xE8D84A ).p( BILLBOARD, 0.002f, 5, 12 );
    W( b, BE::VERDON, "GORGES DU VERDON", "FRANCE", S::NOON ).land( 0x8E9E5A, 0x84944E, 0xA0A070 ).verge( 0xD8D2C2 ).sea( 0, 1, -90 )
        .water( 0x2ABAA8 ).hills( 40, 30, -60 ).drive( 2, 0.3f, 0.98f, 6 ).bends( 0.75f, 0.7f, 0.6f ).tunnels( 0.12f ).rails()
        .bridges( 0.1f, BR::ARCH, 0xD8D0BC )
        .p( SHRUB, 0.35f, 2, 60, 0.6f, 1.0f ).c( 0x6E7E48 ).p( CHALK_ROCK, 0.02f, 10, 120 ).p( OLIVE, 0.04f, 6, 60 )
        .p( FLOWER_BUSH, 0.06f, 3, 30, 0.6f, 0.9f ).c( 0x8A6AC8 ).p( COTTAGE, 0.004f, 20, 100 ).c( 0xE8D8B8, 0xB0583A );
    W( b, BE::COL_DE_TURINI, "COL DE TURINI", "FRANCE", S::NIGHT ).windows( 0xFFD696 ).lamps( 0 ).land( 0xD8DEE6, 0x6A7A5A, 0x5A6A54 )
        .verge( 0xC8CED8 ).hills( 40, 20, -20 ).drive( 2, 0.2f, 0.86f, 2 ).bends( 0.95f, 0.9f, 0.85f ).kerbs( 0xE02020, 0xF0F0F0 )
        .p( SPRUCE, 0.45f, 4, 150 ).p( SNOWBANK, 0.25f, 0.5f, 3, 0.7f, 1.2f ).p( CHALET, 0.01f, 10, 60 ).c( 0xE8E0D0, 0x6A5040 )
        .p( BILLBOARD, 0.004f, 4, 10 ).p( ROCK, 0.08f, 3, 40 );
    W( b, BE::CORSICA, "CORSICA", "FRANCE", S::NOON ).sea( 0, 1, -24 ).water( 0x1E7A9E ).land( 0x7E8E52, 0x74844A, 0x8A8A60 ).verge( 0xC8907A )
        .hills( 36, 24, -20 ).drive( 2, 0.4f, 0.97f, 6 ).bends( 0.85f, 0.75f, 0.7f ).bridges( 0.06f, BR::ARCH, 0xA8A098 )
        .p( SHRUB, 0.45f, 2, 80, 0.6f, 1.0f ).c( 0x5E7040 ).p( BOULDER, 0.05f, 6, 80 ).p( WATCHTOWER, 0.004f, 30, 120 ).c( 0xB8A88A ).v( 1 )
        .p( UMBRELLA_PINE, 0.04f, 6, 80 ).p( CHURCH, 0.003f, 30, 150 ).c( 0xD8CCB8, 0x8A6A58 ).p( SAILBOAT, 0.01f, 60, 250 ).floating();

    // ---- Monaco, Andorra, the Low Countries --------------------------------------------------------------------------
    W( b, BE::MONACO, "MONTE CARLO", "MONACO", S::GOLDEN ).sea( 0, 1, -10 ).water( 0x1E6E9A ).land( 0x7A8A62, 0x70805A, 0x8A8A70 )
        .hills( 30, 30, -6 ).drive( 2, 1.0f, 1.0f, 0 ).bends( 0.75f, 0.8f, 0.4f ).tunnels( 0.3f ).lamps( 30 ).posts( 0 ).rails()
        .kerbs( 0xE02020, 0xF0F0F0 )
        .p( ROW_HOUSES, 0.12f, 4, 30 ).c( 0xF0D8B8, 0xB0583A ).pastel().p( PALM, 0.15f, 2, 10 ).p( CASINO, 0.0f, 20, 40, 1, 1 ).once()
        .p( GLASS_TOWER, 0.03f, 20, 120 ).c( 0xC8C0B0 ).p( BOAT, 0.04f, 40, 150, 1.2f, 1.6f ).c( 0xF4F4F4, 0x2A3A5A ).floating();
    W( b, BE::ANDORRA, "ANDORRA", "ANDORRA", S::ALPINE ).land( 0x6AA048, 0x609640, 0x749458 ).ranges( 0x5E6A7A, 0xE0E6F0, 12 )
        .hills( 46, 24, 24 ).drive( 2, 0.7f, 0.98f, 3 ).bends( 0.7f, 0.6f, 0.8f ).tunnels( 0.15f ).bridges( 0.08f, BR::ARCH, 0x8A8278 )
        .p( CHALET, 0.03f, 6, 70 ).c( 0x8A8278, 0x4A4A50 ).p( CHURCH, 0.004f, 20, 100 ).c( 0x8A8278, 0x5A5A60 ).p( SPRUCE, 0.3f, 6, 150 )
        .p( PEAK, 0.004f, 220, 450 ).c( 0x6A6A70, 0xF0F2F6 ).p( HORSE, 0.02f, 10, 80 ).p( GAS_STATION, 0.004f, 6, 12 );
    W( b, BE::AMSTERDAM, "AMSTERDAM", "NETHERLANDS", S::OVERCAST ).clouds( 0.7f ).sea( 0.6f, 0.6f, -2 ).water( 0x4A6A6A, 0.85f )
        .land( 0x6E8A58, 0x668250, 0x707A68 ).road( 0x7A6A64 ).hills( 0, 0, 0 ).drive( 2, 0.8f, 0.97f, 1 ).bends( 0.35f, 0.4f, 0.0f )
        .lamps( 25 ).posts( 0 ).bridges( 0.35f, BR::ARCH, 0x6A5A50 )
        .p( ROW_HOUSES, 0.25f, 3, 11 ).c( 0x6A3A2A, 0xF0F0EA ).p( ROW_HOUSES, 0.12f, 3, 11 ).c( 0xE8E4DC, 0x3A3A40 )
        .p( HOUSEBOAT, 0.03f, 36, 60 ).floating().p( BROADLEAF, 0.15f, 1.5f, 10, 0.8f, 1.0f ).c( 0x5A8A40 ).p( CHURCH, 0.004f, 20, 100 ).c( 0x8A5A44, 0x3A4A44 );
    W( b, BE::LUXEMBOURG, "LUXEMBOURG", "LUXEMBOURG", S::MORNING ).land( 0x6EA04A, 0x649444, 0x76925A ).verge( 0xB8B0A0 ).hills( 34, -24, 20 )
        .drive( 2, 0.8f, 0.98f, 1 ).bends( 0.5f, 0.45f, 0.55f ).bridges( 0.3f, BR::ARCH, 0xB8B0A0 ).lamps( 40 )
        .p( CASTLE, 0.006f, 50, 200 ).c( 0xB8B0A0, 0x4A5060 ).p( ROW_HOUSES, 0.06f, 6, 40 ).c( 0xE8E0D0, 0x4A4E58 ).p( BROADLEAF, 0.2f, 3, 100 ).c( 0x4E8A3C )
        .p( CHURCH, 0.004f, 30, 150 ).c( 0xD8D0C0, 0x3E4A50 ).p( GLASS_TOWER, 0.008f, 60, 200 ).c( 0x8AA0B4 );

    // ---- Germany -----------------------------------------------------------------------------------------------------
    W( b, BE::ROMANTIC_ROAD, "ROMANTIC ROAD", "GERMANY", S::MORNING ).land( 0x76AC4C, 0x6CA046, 0x7E9E5A ).fields( 0.45f, 0xD8C25A, 0x8AB04A, 0xB8A860 )
        .ranges( 0x7A8CA2, 0xC8D4E4, 7 ).hills( 16, 4, 4 ).drive( 2, 0.6f, 1.0f, 6 ).bends( 0.5f, 0.4f, 0.45f ).bridges( 0.08f, BR::ARCH, 0xB8A890 )
        .landmark( Lm::NEUSCHWANSTEIN, 180, 260 ).p( HALF_TIMBER, 0.05f, 6, 50 ).c( 0xF2E6CC, 0xA8483A ).p( HALF_TIMBER, 0.02f, 6, 50 ).c( 0xE8D0A8, 0x8A3A2A )
        .p( WATCHTOWER, 0.008f, 10, 60 ).c( 0xC8B8A0 ).v( 0 ).p( CHURCH, 0.004f, 30, 150 ).c( 0xF2ECDC, 0x8A3A2A ).p( BROADLEAF, 0.08f, 4, 80 ).c( 0x5E9A40 );
    W( b, BE::NORDSCHLEIFE, "NORDSCHLEIFE", "GERMANY", S::OVERCAST ).clouds( 0.75f ).land( 0x5E8E44, 0x56843E, 0x4E7444 ).hills( 34, 10, 10 )
        .drive( 2, 0.7f, 1.0f, 1 ).bends( 0.85f, 0.7f, 0.9f ).rails().kerbs( 0xE02020, 0xF0F0F0 ).posts( 0 )
        .p( SPRUCE, 0.45f, 6, 150 ).p( BROADLEAF, 0.15f, 6, 100 ).c( 0x4E7E3A ).p( BILLBOARD, 0.01f, 4, 10 ).p( CASTLE, 0.003f, 100, 250 ).c( 0x8A8478, 0x4A4E58 );
    W( b, BE::BERLIN, "BERLIN", "GERMANY", S::MORNING ).land( 0x6E8A58, 0x668250, 0x707E68 ).road( 0x666870 ).drive( 3, 1.2f, 0.98f, 1 )
        .bends( 0.3f, 0.3f, 0.05f ).lamps( 30 ).posts( 0 ).bridges( 0.1f, BR::TRUSS, 0x5A6A5A )
        .landmark( Lm::BRANDENBURG_GATE, 30, 50 ).p( ROW_HOUSES, 0.12f, 5, 30 ).c( 0xE0D4BE, 0x5A5A60 ).p( PANEL_BLOCK, 0.04f, 20, 120 ).c( 0xC8C0B0 )
        .p( TV_TOWER, 0.002f, 150, 350 ).v( 0 ).p( BROADLEAF, 0.18f, 1.5f, 8, 0.8f, 1.0f ).c( 0x5A8A40 ).p( GLASS_TOWER, 0.02f, 40, 200 ).c( 0x7A9AB0 );

    // ---- The Alps ----------------------------------------------------------------------------------------------------
    W( b, BE::LIECHTENSTEIN, "LIECHTENSTEIN", "LIECHTENSTEIN", S::MORNING ).land( 0x6AA848, 0x609A42, 0x749A5A ).ranges( 0x5E6E82, 0xE2E8F2, 12 )
        .hills( 40, 30, 6 ).drive( 2, 0.6f, 1.0f, 3 ).bends( 0.5f, 0.45f, 0.5f )
        .p( CASTLE, 0.005f, 60, 160 ).c( 0xE8E2D4, 0x8A4A3A ).p( VINE_ROW, 0.35f, 6, 30, 1, 1 ).p( CHALET, 0.03f, 8, 80 ).c( 0xF0EADC, 0x6A4A3A )
        .p( CHURCH, 0.004f, 20, 120 ).c( 0xEEE8DC, 0x5A6070 ).p( COW, 0.04f, 10, 80 ).p( SPRUCE, 0.15f, 30, 150 );
    W( b, BE::FURKA_PASS, "FURKA PASS", "SWITZERLAND", S::ALPINE ).land( 0x7A9A5A, 0x6E8E52, 0x8A8A7A ).verge( 0x8A8A88 ).ranges( 0x5A6476, 0xF0F4FA, 14 )
        .hills( 60, 40, -40 ).drive( 2, 0.3f, 0.98f, 3 ).bends( 0.9f, 0.9f, 0.95f ).tunnels( 0.08f ).posts( 20 )
        .p( PEAK, 0.008f, 150, 400, 1.2f, 1.6f ).c( 0x6A6C72, 0xF4F6FA ).p( ROCK, 0.25f, 3, 60 ).p( BOULDER, 0.04f, 6, 80 )
        .p( SNOWBANK, 0.08f, 1, 6, 0.6f, 1.0f ).p( CHALET, 0.0f, 12, 20, 1.2f, 1.2f ).c( 0xE8D8A0, 0x7A5A44 ).once();
    W( b, BE::GROSSGLOCKNER, "GROSSGLOCKNER", "AUSTRIA", S::ALPINE ).land( 0x6EA04C, 0x649444, 0x7A8E66 ).ranges( 0x6A7488, 0xF4F6FA, 15 )
        .hills( 60, 30, -30 ).drive( 2, 0.5f, 0.98f, 3 ).bends( 0.9f, 0.85f, 1.0f ).rails().tunnels( 0.05f )
        .p( PEAK, 0.006f, 200, 450, 1.4f, 1.8f ).c( 0x7A7C82, 0xF6F8FA ).p( SNOWBANK, 0.15f, 1, 8, 0.6f, 1.2f ).p( ROCK, 0.15f, 3, 60 )
        .p( SPRUCE, 0.12f, 10, 150 ).p( COW, 0.03f, 10, 80 ).p( CHALET, 0.008f, 10, 80 ).c( 0x8A5A3A, 0x4A3A30 );

    // ---- Italy, San Marino, the Vatican, Malta -----------------------------------------------------------------------
    W( b, BE::STELVIO, "STELVIO PASS", "ITALY", S::ALPINE ).land( 0x7A9858, 0x6E8C50, 0x8A8C80 ).verge( 0x8A8884 ).ranges( 0x5E6676, 0xF2F4F8, 15 )
        .hills( 60, 50, -10 ).drive( 2, 0.4f, 0.98f, 3 ).bends( 1.0f, 1.0f, 1.0f ).posts( 15 )
        .p( PEAK, 0.008f, 150, 400, 1.3f, 1.8f ).c( 0x6E6C70, 0xF4F6FA ).p( STONE_WALL, 0.5f, 2, 2, 1, 1 ).p( ROCK, 0.2f, 3, 60 )
        .p( SNOWBANK, 0.12f, 1, 8, 0.6f, 1.2f ).p( CHALET, 0.004f, 12, 60 ).c( 0xE8E0D0, 0x6A4A3A );
    W( b, BE::VENICE, "VENICE", "ITALY", S::HAZE ).sea( 1, 1, -1.5f ).water( 0x4A7E76, 0.9f ).land( 0xB89A7A, 0xAC8E70, 0xA89A80 )
        .hills( 0, 0, 0 ).drive( 2, 0.6f, 1.0f, 6 ).bends( 0.2f, 0.25f, 0.0f ).lamps( 30 ).bridges( 0.5f, BR::CAUSEWAY, 0xC8B8A0 )
        .p( ROW_HOUSES, 0.25f, 3, 11 ).c( 0xE0B890, 0xA8583A ).pastel().p( CHURCH, 0.01f, 4, 11 ).c( 0xF0EADC, 0x8A9A9A )
        .p( BOAT, 0.05f, 36, 120, 0.8f, 1.0f ).c( 0x1A1A1E, 0xC8A040 ).floating().p( BOAT, 0.02f, 40, 200 ).c( 0xF0F0F0, 0x2A4A6A ).floating()
        .p( WATCHTOWER, 0.004f, 4, 11, 1.2f, 1.6f ).c( 0xB8583A ).v( 1 );
    W( b, BE::CINQUE_TERRE, "CINQUE TERRE", "ITALY", S::GOLDEN ).sea( 0, 1, -30 ).water( 0x1E6A90 ).land( 0x7A9450, 0x708A48, 0x86965E )
        .hills( 40, 40, -20 ).drive( 2, 0.3f, 0.97f, 6 ).bends( 0.85f, 0.8f, 0.7f ).tunnels( 0.2f ).rails()
        .p( ROW_HOUSES, 0.06f, 6, 60, 1.0f, 1.4f ).c( 0xE8A870, 0x8A5A44 ).pastel().p( VINE_ROW, 0.4f, 4, 40, 1, 1 ).p( TERRACES, 0.02f, 20, 120 ).c( 0x7A9A40, 0x8A7A66 )
        .p( OLIVE, 0.1f, 4, 60 ).p( BOAT, 0.03f, 40, 150 ).c( 0x2A6AAA, 0xF0F0F0 ).pastel().floating().p( CHURCH, 0.004f, 20, 100 ).c( 0xE8D8C0, 0x6A6060 );
    W( b, BE::PISA, "PISA", "ITALY", S::NOON ).land( 0x8EA458, 0x849A50, 0x9AA066 ).fields( 0.4f, 0xD8B860, 0x8AA050, 0xA88A58 ).hills( 6, 0, 0 )
        .drive( 2, 0.8f, 1.0f, 6 ).bends( 0.3f, 0.3f, 0.1f ).lamps( 40 ).bridges( 0.08f, BR::ARCH, 0xC8B494 )
        .landmark( Lm::LEANING_TOWER, 60, 100 ).p( ROW_HOUSES, 0.08f, 6, 40 ).c( 0xE0B880, 0xA8583A ).pastel().p( UMBRELLA_PINE, 0.08f, 4, 80 )
        .p( CYPRESS, 0.06f, 4, 60 ).p( CHURCH, 0.003f, 40, 160 ).c( 0xEEE6D4, 0xA8583A );
    W( b, BE::ROME, "ROME", "ITALY", S::GOLDEN ).land( 0x8A9A60, 0x809058, 0x9A9470 ).road( 0x6A6A70 ).drive( 3, 1.3f, 0.97f, 6 ).bends( 0.4f, 0.4f, 0.3f )
        .lamps( 30 ).posts( 0 ).bridges( 0.12f, BR::ARCH, 0xD8CCB0 )
        .landmark( Lm::COLOSSEUM, 50, 90 ).landmark( Lm::BASILICA, 120, 200, 1.2f ).p( ROW_HOUSES, 0.14f, 4, 30 ).c( 0xE0A070, 0x9A5A3A ).pastel()
        .p( UMBRELLA_PINE, 0.12f, 3, 60 ).p( COLUMNS, 0.006f, 10, 60 ).p( RUIN, 0.006f, 10, 60 ).p( CHURCH, 0.006f, 20, 120 ).c( 0xF0E8D8, 0x8A9A9A );
    W( b, BE::SAN_MARINO, "SAN MARINO", "SAN MARINO", S::MORNING ).land( 0x7AA04E, 0x709448, 0x86985E ).verge( 0xC8C0B0 ).hills( 44, 36, -16 )
        .drive( 2, 0.5f, 0.98f, 6 ).bends( 0.85f, 0.8f, 0.85f ).rails()
        .p( WATCHTOWER, 0.008f, 30, 200, 1.4f, 1.8f ).c( 0xD8D0C0 ).v( 0 ).p( CASTLE, 0.003f, 120, 250 ).c( 0xD8D0C0, 0x8A5A44 )
        .p( ROW_HOUSES, 0.05f, 6, 40 ).c( 0xE0D4BC, 0x9A5A3A ).p( CYPRESS, 0.06f, 4, 60 ).p( VINE_ROW, 0.2f, 10, 40, 1, 1 ).p( OLIVE, 0.06f, 6, 60 );
    W( b, BE::VATICAN, "VATICAN CITY", "VATICAN", S::NOON ).land( 0x8A9A60, 0x809058, 0x9A9470 ).road( 0x6E6E72 ).drive( 2, 0.9f, 1.0f, 6 )
        .bends( 0.35f, 0.4f, 0.15f ).lamps( 30 ).posts( 0 )
        .landmark( Lm::BASILICA, 50, 80, 1.3f ).p( COLUMNS, 0.05f, 6, 30 ).p( ROW_HOUSES, 0.1f, 4, 30 ).c( 0xE8C8A0, 0x9A5A3A ).pastel()
        .p( UMBRELLA_PINE, 0.1f, 3, 60 ).landmark( Lm::OBELISK, 30, 45, 0.15f );
    W( b, BE::ALBEROBELLO, "ALBEROBELLO", "ITALY", S::NOON ).land( 0x8E9A58, 0x849050, 0xA06A4A ).fields( 0.35f, 0xA8603A, 0x9AA050, 0xC8B060 )
        .verge( 0xD8D0C0 ).hills( 8, 2, 2 ).drive( 2, 0.5f, 1.0f, 6 ).bends( 0.35f, 0.3f, 0.2f ).posts( 0 )
        .p( TRULLO, 0.06f, 6, 60 ).p( OLIVE, 0.35f, 4, 150 ).p( STONE_WALL, 0.5f, 3, 3, 1, 1 ).p( CHURCH, 0.003f, 30, 120 ).c( 0xF4F2EA, 0x8A8A8A )
        .p( PRICKLY_PEAR, 0.03f, 3, 30 );
    W( b, BE::SARDINIA, "SARDINIA", "ITALY", S::NOON ).sea( 0, 1, -6 ).water( 0x2ABAC8 ).land( 0x8A9058, 0x808650, 0x9A9468 ).verge( 0xD8C8B0 )
        .hills( 24, 10, -4 ).drive( 2, 0.4f, 0.98f, 6 ).bends( 0.65f, 0.55f, 0.55f )
        .p( BOULDER, 0.06f, 4, 100, 1.0f, 2.2f ).p( SHRUB, 0.4f, 2, 80, 0.6f, 1.0f ).c( 0x5E7040 ).p( ROUND_MILL, 0.004f, 30, 150, 1.2f, 1.5f ).v( 1 )
        .p( FLAT_HOUSE, 0.03f, 6, 60 ).c( 0xF0E8D8, 0xC8603A ).pastel().p( SAILBOAT, 0.015f, 40, 200 ).floating().p( OLIVE, 0.05f, 6, 60 );
    W( b, BE::MALTA, "VALLETTA", "MALTA", S::NOON ).sea( 0, 1, -10 ).water( 0x1E6AA0 ).land( 0xC8B888, 0xBCAC7E, 0xB8A880 ).verge( 0xD8C8A0 )
        .hills( 14, 6, -4 ).drive( 2, 0.8f, 0.98f, 6 ).bends( 0.5f, 0.5f, 0.35f ).lamps( 35 ).posts( 0 )
        .p( ROW_HOUSES, 0.15f, 4, 30 ).c( 0xE0C890, 0x8A7A5A ).p( FORT, 0.006f, 20, 100 ).c( 0xD8C08A, 0xC8B07A ).v( 0 ).p( CHURCH, 0.008f, 20, 120 ).c( 0xE0C890, 0x8A7A6A )
        .p( BOAT, 0.04f, 40, 150 ).c( 0xE8C040, 0x2A6AAA ).pastel().floating().p( PRICKLY_PEAR, 0.04f, 3, 30 ).p( STONE_WALL, 0.25f, 3, 3, 1, 1 );

    // ---- Spain and Portugal ------------------------------------------------------------------------------------------
    W( b, BE::SA_CALOBRA, "SA CALOBRA", "SPAIN", S::NOON ).land( 0x8A9058, 0x808650, 0x9A9672 ).verge( 0xD0C8B8 ).sea( 0, 0.5f, -90 )
        .water( 0x1E7AAA ).hills( 50, 40, -30 ).drive( 2, 0.4f, 0.98f, 6 ).bends( 1.0f, 1.0f, 0.95f ).posts( 0 )
        .p( STONE_WALL, 0.45f, 2, 2, 1, 1 ).p( CHALK_ROCK, 0.03f, 10, 120 ).p( OLIVE, 0.08f, 6, 80 ).p( SHRUB, 0.3f, 2, 60, 0.6f, 1.0f ).c( 0x6E7A48 )
        .p( PILLAR, 0.01f, 60, 250, 0.8f, 1.2f );
    W( b, BE::BARCELONA, "BARCELONA", "SPAIN", S::GOLDEN ).sea( 0, 0.5f, -4 ).water( 0x2A7AAA ).land( 0x8A9A60, 0x809058, 0x9A9470 )
        .drive( 3, 1.3f, 0.98f, 1 ).bends( 0.3f, 0.3f, 0.15f ).lamps( 30 ).posts( 0 )
        .landmark( Lm::SAGRADA, 80, 140 ).p( ROW_HOUSES, 0.18f, 4, 30 ).c( 0xE0CCA8, 0x8A6A58 ).p( PALM, 0.12f, 2, 12 )
        .p( GLASS_TOWER, 0.02f, 40, 200 ).c( 0x8AA4BA ).p( BROADLEAF, 0.1f, 1.5f, 8 ).c( 0x6A9A48 );
    W( b, BE::ALHAMBRA, "GRANADA", "SPAIN", S::GOLDEN ).land( 0x9A9A5E, 0x909054, 0xA89A6E ).ranges( 0x8A8EA8, 0xF0F2F6, 9 ).hills( 24, 10, 6 )
        .drive( 2, 0.7f, 1.0f, 6 ).bends( 0.6f, 0.55f, 0.55f )
        .p( FORT, 0.004f, 60, 180, 1.4f, 1.8f ).c( 0xC07A50, 0xB06A44 ).v( 0 ).p( CYPRESS, 0.12f, 3, 60 ).p( OLIVE, 0.15f, 6, 100 )
        .p( FLAT_HOUSE, 0.06f, 6, 60 ).c( 0xF4F2EA, 0xB8583A ).p( POPLAR, 0.05f, 6, 60 );
    W( b, BE::PICOS, "PICOS DE EUROPA", "SPAIN", S::MIST ).land( 0x62A048, 0x589440, 0x6A8E5A ).verge( 0xB0ACA4 ).ranges( 0x8A8A88, 0xD8D8DA, 11 )
        .hills( 50, 30, 20 ).drive( 2, 0.3f, 0.95f, 3 ).bends( 0.75f, 0.65f, 0.75f ).tunnels( 0.1f ).bridges( 0.08f, BR::ARCH, 0x9A9890 )
        .p( PEAK, 0.008f, 150, 400, 1.0f, 1.4f ).c( 0xA8A49C, 0xE8E8EA ).p( COW, 0.06f, 8, 80 ).p( COTTAGE, 0.015f, 10, 80 ).c( 0x8A8278, 0x8A4A3A )
        .p( BROADLEAF, 0.15f, 4, 100 ).c( 0x4E8A3A ).p( ROCK, 0.1f, 3, 60 );
    W( b, BE::LISBON, "LISBON", "PORTUGAL", S::GOLDEN ).sea( 0, 1, -8 ).water( 0x2A6A8A ).land( 0x8A9A5E, 0x809056, 0x9A9470 ).hills( 24, 10, -4 )
        .drive( 3, 1.1f, 0.97f, 6 ).bends( 0.45f, 0.45f, 0.6f ).lamps( 30 ).posts( 0 ).bridges( 0.5f, BR::SUSPENSION, 0xC0402C )
        .landmark( Lm::OPEN_ARMS, 160, 260, 1.4f ).p( ROW_HOUSES, 0.15f, 4, 40 ).c( 0xE8C8A0, 0xB0583A ).pastel()
        .p( CHURCH, 0.006f, 20, 120 ).c( 0xF0ECE0, 0xB0583A ).p( PALM, 0.06f, 2, 20 ).p( SAILBOAT, 0.01f, 60, 250 ).floating();

    // ---- North Africa ------------------------------------------------------------------------------------------------
    W( b, BE::ESSAOUIRA, "ESSAOUIRA", "MOROCCO", S::MORNING ).clouds( 0.3f ).sea( 1, 0, -6 ).water( 0x2A6A8A ).land( 0xC8B48A, 0xBCA880, 0x9A9A6A )
        .verge( 0xD8C8A8 ).hills( 8, 0, 4 ).drive( 2, 0.5f, 0.97f, 6 ).bends( 0.35f, 0.3f, 0.2f )
        .p( FLAT_HOUSE, 0.1f, 4, 40 ).c( 0xF4F2EC, 0x2A5AA8 ).p( FORT, 0.006f, 10, 60 ).c( 0xC8B088, 0xB8A078 ).v( 0 ).p( OLIVE, 0.08f, 6, 80 )
        .p( BOAT, 0.05f, 36, 120 ).c( 0x2A5AA8, 0xE8E4D8 ).floating().p( DATE_PALM, 0.04f, 4, 40 ).p( MOSQUE, 0.003f, 30, 120 ).c( 0xF4F2EC, 0xF0EEE6 );
    W( b, BE::SIDI_BOU_SAID, "SIDI BOU SAID", "TUNISIA", S::NOON ).sea( 0, 1, -20 ).water( 0x1E6AB0 ).land( 0x9A9A62, 0x90905A, 0xA8A274 )
        .hills( 20, 10, -10 ).drive( 2, 0.7f, 0.98f, 6 ).bends( 0.55f, 0.5f, 0.5f ).lamps( 40 )
        .p( FLAT_HOUSE, 0.18f, 3, 40 ).c( 0xF6F6F2, 0x1E5AB0 ).p( FLOWER_BUSH, 0.1f, 2, 20, 0.6f, 1.0f ).c( 0xC8307A ).p( MOSQUE, 0.006f, 20, 120 ).c( 0xF6F6F2, 0xF0F0EA )
        .p( DATE_PALM, 0.06f, 3, 40 ).p( OLIVE, 0.06f, 6, 60 ).p( SAILBOAT, 0.01f, 60, 250 ).floating();
    W( b, BE::TASSILI, "TASSILI N AJJER", "ALGERIA", S::DESERT ).land( 0xD89A5E, 0xCC9056, 0xC8885A ).verge( 0x6A4A3A ).road( 0x8A6A50, 0x8A6A50 ).sand()
        .hills( 20, 8, 8 ).drive( 2, 0.05f, 0.86f, 7 ).bends( 0.45f, 0.4f, 0.3f ).posts( 0 )
        .p( PILLAR, 0.05f, 20, 250, 0.8f, 1.4f ).p( ARCH, 0.006f, 40, 200 ).c( 0x5A3A2A ).p( DUNE, 0.02f, 60, 300, 1.0f, 1.8f ).c( 0xE0904E )
        .p( ROCK, 0.1f, 3, 60 ).p( CAMEL, 0.01f, 15, 120 ).p( ACACIA, 0.006f, 10, 100, 0.5f, 0.8f );
    W( b, BE::GHADAMES, "GHADAMES", "LIBYA", S::DESERT ).land( 0xE0C08A, 0xD4B482, 0xDCB880 ).verge( 0xE0CCA8 ).hills( 6, 2, 2 )
        .drive( 2, 0.15f, 0.92f, 7 ).bends( 0.25f, 0.25f, 0.15f ).posts( 0 )
        .p( ADOBE, 0.06f, 6, 60 ).c( 0xF0E8D8, 0xD8C8A8 ).p( KASBAH, 0.006f, 20, 100 ).c( 0xD8C8A0 ).p( DATE_PALM, 0.2f, 3, 60 )
        .p( DUNE, 0.03f, 80, 400, 1.2f, 2.2f ).c( 0xE8B878 ).p( CAMEL, 0.015f, 15, 120 ).p( WATER_TOWER, 0.002f, 30, 120 );
    W( b, BE::ADRAR, "ADRAR", "MAURITANIA", S::DESERT ).land( 0xD8A870, 0xCC9C66, 0xC89A6A ).verge( 0x8A6A50 ).hills( 14, 4, 4 )
        .drive( 2, 0.08f, 0.9f, 7 ).bends( 0.2f, 0.25f, 0.2f ).posts( 0 )
        .p( BUTTE, 0.006f, 80, 300 ).p( DUNE, 0.04f, 40, 300, 1.0f, 2.0f ).c( 0xE89A58 ).p( ADOBE, 0.01f, 10, 80 ).c( 0xB86A40, 0xA85A38 )
        .p( DATE_PALM, 0.03f, 4, 60 ).p( CAMEL, 0.02f, 10, 120 ).p( ROCK, 0.06f, 3, 60 );
    W( b, BE::MEROE, "MEROE", "SUDAN", S::DESERT ).land( 0xE0A868, 0xD49C60, 0xD8A06A ).verge( 0x6A4A3A ).hills( 8, 2, 2 )
        .drive( 2, 0.08f, 0.9f, 7 ).bends( 0.2f, 0.2f, 0.15f ).posts( 0 )
        .p( STONE_PYRAMID, 0.01f, 40, 200, 0.25f, 0.4f ).p( DUNE, 0.05f, 30, 300, 0.8f, 1.6f ).c( 0xE0A060 ).p( CAMEL, 0.02f, 10, 120 )
        .p( ACACIA, 0.02f, 6, 100, 0.5f, 0.8f ).p( ROCK, 0.08f, 3, 60 ).p( RUIN, 0.004f, 30, 150 );
    W( b, BE::LUXOR, "LUXOR", "EGYPT", S::NOON ).sea( 0, 1, -3 ).water( 0x2A6A7A ).land( 0x7AA048, 0x709440, 0xD8B880 ).verge( 0xD8C8A0 ).hills( 4, 0, 0 )
        .drive( 2, 0.6f, 0.98f, 7 ).bends( 0.25f, 0.25f, 0.05f )
        .landmark( Lm::COLOSSI, 50, 90 ).landmark( Lm::OBELISK, 40, 70, 0.14f ).p( COLUMNS, 0.02f, 10, 80 ).p( RUIN, 0.01f, 10, 80 )
        .p( DATE_PALM, 0.15f, 3, 60 ).p( SAILBOAT, 0.03f, 40, 150 ).c( 0xF4F2EA, 0xF0EEE6 ).floating().p( BALLOON, 0.004f, 100, 300 )
        .p( FLAT_HOUSE, 0.03f, 8, 60 ).c( 0xD8C8A8, 0xB8A888 ).p( CAMEL, 0.008f, 10, 60 );

    // ---- West Africa -------------------------------------------------------------------------------------------------
    W( b, BE::FOGO, "FOGO", "CAPE VERDE", S::NOON ).sea( 0, 1, -20 ).water( 0x1E5A8A ).land( 0x4A4644, 0x403C3A, 0x5A5450 ).verge( 0x3A3634 )
        .road( 0x4A4A50 ).hills( 30, 16, -10 ).drive( 2, 0.2f, 0.95f, 12 ).bends( 0.65f, 0.55f, 0.75f )
        .p( VOLCANO_CONE, 0.006f, 250, 500, 1.4f, 1.8f ).c( 0x3A3634, 0x4A4440 ).p( LAVA_ROCK, 0.15f, 3, 60 ).p( VINE_ROW, 0.2f, 8, 30, 1, 1 )
        .p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0xE8C8A0, 0x2A6AAA ).pastel().p( SHRUB, 0.08f, 2, 40 ).c( 0x6A7A48 );
    W( b, BE::PINK_LAKE, "LAC ROSE", "SENEGAL", S::TROPICAL ).sea( 1, 0, -1.5f ).water( 0xE098A8, 0.8f ).land( 0xD8C8A0, 0xCCBC96, 0x9AA060 )
        .verge( 0xE0D4B8 ).hills( 4, 0, 2 ).drive( 2, 0.3f, 0.92f, 12 ).bends( 0.3f, 0.3f, 0.1f )
        .p( HILLOCK, 0.08f, 3, 11, 0.3f, 0.5f ).c( 0xF4F0EA ).p( BAOBAB, 0.02f, 10, 120 ).p( PALM, 0.06f, 3, 60 ).p( HUT, 0.008f, 8, 40 )
        .p( BOAT, 0.03f, 36, 120 ).c( 0x2A8A4A, 0xE8C040 ).pastel().floating().p( DUNE, 0.01f, 60, 250, 0.6f, 1.0f ).c( 0xE8D8B0 );
    W( b, BE::GAMBIA_RIVER, "GAMBIA RIVER", "GAMBIA", S::HAZE ).sea( 0, 1, -3 ).water( 0x6A6A48, 0.8f ).land( 0x7A9A48, 0x709040, 0x8A9A5A )
        .hills( 4, 0, 0, 0.2f ).drive( 2, 0.3f, 0.95f, 12 ).bends( 0.35f, 0.3f, 0.1f )
        .p( MANGROVE, 0.15f, 2, 11 ).p( PALM, 0.12f, 3, 80 ).p( BAOBAB, 0.02f, 10, 150 ).p( HUT, 0.02f, 8, 60 )
        .p( BOAT, 0.04f, 36, 120 ).c( 0x2A8A4A, 0xD84A3A ).pastel().floating().p( COW, 0.02f, 8, 60 );
    W( b, BE::FOUTA_DJALLON, "FOUTA DJALLON", "GUINEA", S::HAZE ).land( 0x6A9A42, 0x60903C, 0x5E8A4A ).verge( 0x9A5A3A ).road( 0x9A6A4A, 0x9A6A4A )
        .hills( 40, 20, 14 ).drive( 2, 0.25f, 0.88f, 12 ).bends( 0.7f, 0.55f, 0.75f ).posts( 0 )
        .p( WATERFALL, 0.006f, 60, 200, 0.6f, 1.0f ).v( 1 ).p( HUT, 0.03f, 8, 60 ).p( JUNGLE_TREE, 0.1f, 6, 120 ).p( BROADLEAF, 0.15f, 4, 100 ).c( 0x3E7A34 )
        .p( COW, 0.03f, 8, 80 ).p( TEPUI, 0.003f, 250, 500, 0.6f, 0.8f ).c( 0x7A5A4A, 0x4A8A3A );
    W( b, BE::BIJAGOS, "BIJAGOS ISLANDS", "GUINEA-BISSAU", S::TROPICAL ).sea( 1, 1, -1.5f ).water( 0x2A9AA8 ).land( 0xE0D4B0, 0xD4C8A4, 0x6A9A4A )
        .verge( 0xE0D8C0 ).hills( 2, 0, 0 ).drive( 2, 0.15f, 0.95f, 12 ).bends( 0.3f, 0.3f, 0.05f ).bridges( 0.3f, BR::CAUSEWAY, 0xC8C0B0 )
        .p( PALM, 0.3f, 2, 11 ).p( MANGROVE, 0.1f, 2, 11 ).p( HUT, 0.03f, 3, 11 ).p( BOAT, 0.03f, 36, 150 ).c( 0x8A5A3A, 0xE8E0D0 ).floating();
    W( b, BE::FREETOWN, "FREETOWN PENINSULA", "SIERRA LEONE", S::TROPICAL ).sea( 0, 1, -6 ).water( 0x1E8AA0 ).land( 0x4E8A3A, 0x468034, 0x4A7A44 )
        .verge( 0xE8DCC0 ).hills( 40, 30, -4 ).drive( 2, 0.6f, 0.95f, 12 ).bends( 0.65f, 0.55f, 0.55f )
        .p( JUNGLE_TREE, 0.2f, 6, 150 ).p( PALM, 0.15f, 2, 40 ).p( FLAT_HOUSE, 0.04f, 6, 60 ).c( 0xE8C890, 0x8A8A8A ).pastel()
        .p( BOAT, 0.03f, 36, 150 ).c( 0x2A6AAA, 0xE8C040 ).pastel().floating().p( BANANA, 0.08f, 3, 40 );
    W( b, BE::LIBERIAN_COAST, "ROBERTSPORT", "LIBERIA", S::HAZE ).rain( 0.2f ).sea( 0, 1, -3 ).water( 0x2A7A8A ).land( 0x5E9040, 0x56863A, 0x4E7A42 )
        .verge( 0xE0D4B8 ).hills( 14, 6, 0 ).drive( 2, 0.2f, 0.9f, 12 ).bends( 0.5f, 0.45f, 0.3f ).road( 0x8A6A50, 0x8A6A50 ).posts( 0 )
        .p( PALM, 0.3f, 2, 60 ).p( JUNGLE_TREE, 0.15f, 10, 150 ).p( HUT, 0.02f, 6, 40 ).p( BOAT, 0.03f, 36, 120 ).c( 0x2A8A4A, 0xD84A3A ).pastel().floating();
    W( b, BE::YAMOUSSOUKRO, "YAMOUSSOUKRO", "IVORY COAST", S::NOON ).land( 0x6A9A48, 0x609040, 0x7A9A5A ).hills( 6, 0, 0, 0.2f )
        .drive( 3, 0.3f, 1.0f, 12 ).bends( 0.15f, 0.2f, 0.05f ).lamps( 30 ).posts( 0 )
        .landmark( Lm::BASILICA, 120, 200, 1.4f ).p( PALM, 0.4f, 2, 6, 1.0f, 1.1f ).p( BROADLEAF, 0.06f, 10, 100 ).c( 0x3E7A34 )
        .p( FLAT_HOUSE, 0.01f, 20, 100 ).c( 0xE8D8B8, 0xB0583A );
    W( b, BE::TIEBELE, "TIEBELE", "BURKINA FASO", S::DESERT ).land( 0xC8A070, 0xBC9466, 0xB89A6A ).verge( 0xB87A50 ).road( 0xA87A58, 0xA87A58 )
        .hills( 6, 2, 2 ).drive( 2, 0.15f, 0.9f, 12 ).bends( 0.3f, 0.3f, 0.15f ).posts( 0 )
        .p( ADOBE, 0.04f, 8, 60 ).c( 0xA85A40, 0xE8E0D0 ).p( HUT, 0.03f, 8, 60 ).p( BAOBAB, 0.02f, 10, 120 ).p( SHRUB, 0.12f, 2, 60 ).c( 0x8A8A48 )
        .p( ACACIA, 0.03f, 8, 120 ).p( COW, 0.02f, 10, 80 );
    W( b, BE::AGADEZ, "AGADEZ", "NIGER", S::DESERT ).land( 0xD8B07A, 0xCCA472, 0xD0A878 ).verge( 0xC89A6A ).hills( 6, 2, 2 )
        .drive( 2, 0.15f, 0.9f, 12 ).bends( 0.2f, 0.25f, 0.1f ).posts( 0 )
        .p( MOSQUE, 0.004f, 40, 150, 1.2f, 1.6f ).c( 0xB88A60, 0xB88A60 ).p( ADOBE, 0.06f, 6, 80 ).c( 0xB88A60, 0xA87A50 ).p( CAMEL, 0.03f, 8, 120 )
        .p( DUNE, 0.02f, 80, 400, 1.0f, 2.0f ).c( 0xE0B070 ).p( ACACIA, 0.02f, 6, 100, 0.5f, 0.8f );
    W( b, BE::ZUMA_ROCK, "ZUMA ROCK", "NIGERIA", S::HAZE ).land( 0x8AA050, 0x809448, 0x9A9A5E ).hills( 10, 2, 2 ).drive( 3, 0.9f, 0.97f, 12 )
        .bends( 0.3f, 0.3f, 0.2f )
        .p( GRANITE_DOME, 0.0f, 200, 300, 1.6f, 1.6f ).v( 0 ).once().p( GRANITE_DOME, 0.004f, 150, 400, 0.6f, 0.9f ).v( 1 )
        .p( BROADLEAF, 0.08f, 6, 100 ).c( 0x3E7A34 ).p( HUT, 0.02f, 8, 60 ).p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0xE8D8B8, 0x8A8A8A ).p( ACACIA, 0.04f, 8, 120 );
    W( b, BE::LAGOS, "LAGOS", "NIGERIA", S::HAZE ).sea( 0, 1, -3 ).water( 0x4A6A6A, 0.85f ).land( 0x7A8A5A, 0x708050, 0x7A7A62 )
        .drive( 4, 1.7f, 0.95f, 12 ).bends( 0.25f, 0.25f, 0.05f ).lamps( 35 ).posts( 0 ).bridges( 0.5f, BR::BEAM )
        .p( GLASS_TOWER, 0.1f, 20, 200 ).c( 0x6A8AA8 ).p( PANEL_BLOCK, 0.05f, 20, 120 ).c( 0xD8C8A8 ).p( FLAT_HOUSE, 0.06f, 4, 30 ).c( 0xE8C890, 0x8A8A8A ).pastel()
        .p( BILLBOARD, 0.02f, 4, 12 ).p( PALM, 0.08f, 2, 20 ).p( BOAT, 0.03f, 36, 100 ).c( 0x2A8A4A, 0xE8C040 ).pastel().floating();
    W( b, BE::KOUTAMMAKOU, "KOUTAMMAKOU", "TOGO", S::HAZE ).land( 0x9A9A56, 0x908E4E, 0x8A9A5A ).verge( 0xA87A50 ).road( 0xA87A58, 0xA87A58 )
        .hills( 24, 8, 8 ).drive( 2, 0.12f, 0.9f, 12 ).bends( 0.45f, 0.4f, 0.45f ).posts( 0 )
        .p( FORT, 0.01f, 10, 60, 0.4f, 0.5f ).c( 0xA87A50, 0x9A6A44 ).v( 0 ).p( HUT, 0.03f, 8, 60 ).p( BAOBAB, 0.03f, 10, 120 )
        .p( SHRUB, 0.15f, 2, 60 ).c( 0x7A8A44 ).p( BROADLEAF, 0.05f, 6, 100 ).c( 0x5A8A3A );
    W( b, BE::GANVIE, "GANVIE", "BENIN", S::HAZE ).sea( 1, 1, -1.5f ).water( 0x5A7A6A, 0.85f ).land( 0x6A9048, 0x608640, 0x6A8A52 )
        .drive( 2, 0.3f, 0.95f, 12 ).bends( 0.25f, 0.25f, 0.0f ).bridges( 0.5f, BR::CAUSEWAY, 0xB8B0A0 )
        .p( WATER_VILLA, 0.05f, 36, 150 ).floating().p( BOAT, 0.06f, 36, 150 ).c( 0x6A4A30, 0xE8E0D0 ).floating().p( PALM, 0.2f, 2, 11 ).p( REEDS, 0.2f, 2, 11 );
    W( b, BE::MOUNT_CAMEROON, "MOUNT CAMEROON", "CAMEROON", S::MIST ).rain( 0.3f ).land( 0x4E8A3A, 0x468034, 0x4A7442 ).verge( 0x4A3A34 )
        .hills( 36, 14, 10 ).drive( 2, 0.4f, 0.92f, 12 ).bends( 0.6f, 0.5f, 0.65f )
        .p( VOLCANO_CONE, 0.006f, 300, 600, 1.6f, 2.0f ).c( 0x3E4A3A, 0x6A6A60 ).p( TEA_ROW, 0.5f, 6, 30, 1, 1 ).p( JUNGLE_TREE, 0.15f, 30, 150 )
        .p( BANANA, 0.12f, 3, 40 ).p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0xE8D8B8, 0x8A8A8A );
    W( b, BE::ENNEDI, "ENNEDI", "CHAD", S::DESERT ).land( 0xD8A068, 0xCC9460, 0xC89462 ).verge( 0x7A4A34 ).road( 0x9A7050, 0x9A7050 ).sand()
        .hills( 10, 4, 4 ).drive( 2, 0.04f, 0.85f, 12 ).bends( 0.35f, 0.35f, 0.2f ).posts( 0 )
        .p( ARCH, 0.006f, 60, 250, 1.2f, 1.6f ).c( 0x8A4A30 ).p( PILLAR, 0.04f, 30, 250, 0.8f, 1.4f ).p( DUNE, 0.02f, 60, 300 ).c( 0xE0A060 )
        .p( CAMEL, 0.02f, 10, 120 ).p( ACACIA, 0.008f, 10, 100, 0.5f, 0.8f );

    // ---- Central Africa ----------------------------------------------------------------------------------------------
    W( b, BE::DZANGA_SANGHA, "DZANGA-SANGHA", "CENTRAL AFRICAN REPUBLIC", S::HAZE ).fog( 0.0024f, 0xB8C8B0 ).land( 0x3E7A32, 0x38702C, 0x3A6A36 )
        .verge( 0x9A5A3A ).road( 0x9A5A3A, 0x9A5A3A ).hills( 10, 6, 6, 0.15f ).drive( 2, 0.05f, 0.86f, 12 ).bends( 0.55f, 0.45f, 0.3f ).posts( 0 )
        .p( JUNGLE_TREE, 0.5f, 3, 150 ).p( FERN, 0.3f, 1, 20 ).p( ELEPHANT, 0.01f, 15, 80 ).p( HUT, 0.006f, 6, 30 ).p( BANANA, 0.06f, 3, 30 );
    W( b, BE::LOANGO, "LOANGO", "GABON", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x2A7A8A ).land( 0x5A9040, 0x52863A, 0x4E7A42 ).verge( 0xE8DCC0 )
        .hills( 6, 4, 0, 0.25f ).drive( 2, 0.05f, 0.9f, 12 ).bends( 0.35f, 0.3f, 0.1f ).road( 0xA88A60, 0xA88A60 ).posts( 0 )
        .p( JUNGLE_TREE, 0.25f, 6, 150 ).p( PALM, 0.2f, 2, 11 ).p( ELEPHANT, 0.015f, 6, 80 ).p( REEDS, 0.1f, 2, 30 ).p( BUSH, 0.1f, 2, 30 );
    W( b, BE::CONGO_RAPIDS, "CONGO RAPIDS", "CONGO", S::HAZE ).sea( 0, 1, -6 ).water( 0x7A6A50, 0.7f ).land( 0x5A8A3E, 0x528038, 0x56783E )
        .hills( 14, 6, -4 ).drive( 2, 0.4f, 0.93f, 12 ).bends( 0.4f, 0.4f, 0.3f )
        .p( BOULDER, 0.04f, 36, 150, 1.0f, 2.0f ).floating().p( JUNGLE_TREE, 0.25f, 6, 150 ).p( PALM, 0.08f, 3, 40 )
        .p( BOAT, 0.03f, 40, 150 ).c( 0x6A4A30, 0xE8E0D0 ).floating().p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0xE8D8B8, 0x8A8A8A );
    W( b, BE::BIOKO, "BIOKO", "EQUATORIAL GUINEA", S::MIST ).rain( 0.3f ).sea( 0, 1, -20 ).water( 0x1E5A7A ).land( 0x3E7A32, 0x38702C, 0x3E6A3A )
        .hills( 40, 24, -10 ).drive( 2, 0.25f, 0.92f, 12 ).bends( 0.65f, 0.55f, 0.65f )
        .p( JUNGLE_TREE, 0.3f, 6, 150 ).p( TREE_FERN, 0.12f, 2, 40 ).p( BROADLEAF, 0.15f, 4, 60 ).c( 0x2E5E2A ).p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0xE8D8C0, 0xB0583A )
        .p( VOLCANO_CONE, 0.003f, 300, 600, 1.2f, 1.5f ).c( 0x2E5A2E, 0x4A6A4A );
    W( b, BE::PICO_CAO_GRANDE, "PICO CAO GRANDE", "SAO TOME AND PRINCIPE", S::MIST ).rain( 0.35f ).land( 0x3A7A30, 0x34702A, 0x3A6A36 )
        .hills( 40, 20, 20 ).drive( 2, 0.1f, 0.9f, 12 ).bends( 0.6f, 0.5f, 0.6f )
        .p( MONOLITH, 0.0f, 220, 320, 2.0f, 2.0f ).c( 0x5A6A50 ).once().p( JUNGLE_TREE, 0.4f, 4, 150 ).p( BANANA, 0.12f, 3, 40 ).p( TREE_FERN, 0.1f, 2, 40 )
        .p( HUT, 0.008f, 6, 30 );
    W( b, BE::MOON_VIEWPOINT, "MIRADOURO DA LUA", "ANGOLA", S::NOON ).sea( 0, 1, -40 ).water( 0x1E6A9A ).land( 0xC8A878, 0xBC9C70, 0xB89A70 )
        .verge( 0xC88A60 ).hills( 20, 4, -24 ).drive( 2, 0.3f, 0.98f, 12 ).bends( 0.3f, 0.3f, 0.25f )
        .p( BADLAND, 0.03f, 12, 120, 0.8f, 1.4f ).c( 0xD8B890, 0xB86A48 ).p( BAOBAB, 0.02f, 10, 150 ).p( SHRUB, 0.1f, 2, 60 ).c( 0x7A8A48 )
        .p( CACTUS, 0.01f, 4, 40, 0.4f, 0.6f );

    // ---- East Africa -------------------------------------------------------------------------------------------------
    W( b, BE::MURCHISON, "MURCHISON FALLS", "UGANDA", S::HAZE ).sea( 0, 1, -6 ).water( 0x5A6A58, 0.8f ).land( 0x8AA04A, 0x809444, 0x8A9A58 )
        .hills( 14, 4, -4 ).drive( 2, 0.2f, 0.93f, 12 ).bends( 0.35f, 0.3f, 0.25f ).road( 0xA87A58, 0xA87A58 ).posts( 0 )
        .p( WATERFALL, 0.0f, 80, 140, 0.8f, 0.8f ).v( 1 ).once().p( ACACIA, 0.05f, 8, 150 ).p( GIRAFFE, 0.02f, 15, 150 ).p( ELEPHANT, 0.01f, 20, 150 )
        .p( PALM, 0.05f, 6, 80 ).p( BOAT, 0.01f, 40, 150 ).c( 0xF0F0F0, 0x2A5A3A ).floating().p( GRASS_TUFT, 0.3f, 1, 40 );
    W( b, BE::TANGANYIKA, "LAKE TANGANYIKA", "BURUNDI", S::HAZE ).sea( 0, 1, -6 ).water( 0x2A6A8A ).land( 0x5E9A44, 0x56903E, 0x5E8A4A )
        .hills( 36, 20, -4 ).drive( 2, 0.4f, 0.94f, 12 ).bends( 0.6f, 0.5f, 0.55f )
        .p( BANANA, 0.2f, 3, 60 ).p( HUT, 0.02f, 6, 60 ).p( FLAT_HOUSE, 0.02f, 6, 60 ).c( 0xC88A60, 0x8A8A8A ).p( PALM, 0.08f, 3, 40 )
        .p( BOAT, 0.03f, 40, 150 ).c( 0x6A4A30, 0xE8E0D0 ).floating().p( EUCALYPTUS, 0.06f, 6, 80 );
    W( b, BE::SUDD, "SUDD", "SOUTH SUDAN", S::HAZE ).land( 0x8AA048, 0x809442, 0x8A9A58 ).water( 0x5A6A50, 0.7f ).hills( 2, 0, 0, 0.7f )
        .drive( 2, 0.08f, 0.86f, 12 ).bends( 0.25f, 0.25f, 0.05f ).road( 0x8A6A50, 0x8A6A50 ).posts( 0 )
        .p( REEDS, 0.4f, 2, 60 ).p( COW, 0.05f, 6, 80 ).v( 2 ).p( HUT, 0.02f, 8, 60 ).p( ACACIA, 0.02f, 10, 120 ).p( PALM, 0.03f, 6, 80 );
    W( b, BE::ERITREAN_HIGHLANDS, "ASMARA ROAD", "ERITREA", S::ALPINE ).land( 0xA8945E, 0x9C8A56, 0x9A8A62 ).verge( 0x8A7A60 )
        .ranges( 0x6A6A70, 0x9A9AA0, 8 ).hills( 40, 30, -30 ).drive( 2, 0.25f, 0.96f, 12 ).bends( 0.95f, 0.9f, 0.95f ).posts( 0 )
        .p( PRICKLY_PEAR, 0.08f, 3, 40 ).p( TERRACES, 0.02f, 30, 150 ).c( 0x9A8A50, 0x8A7A5A ).p( SHRUB, 0.15f, 2, 60 ).c( 0x7A8048 )
        .p( CAMEL, 0.01f, 10, 80 ).p( ROCK, 0.12f, 3, 60 ).p( EUCALYPTUS, 0.03f, 6, 80 );
    W( b, BE::LAKE_ASSAL, "LAKE ASSAL", "DJIBOUTI", S::DESERT ).sea( 1, 0, -6 ).water( 0x3AC8C0 ).land( 0xF0EEE8, 0xE6E4DC, 0x3A3634 )
        .verge( 0x3A3634 ).shine( 0.35f ).hills( 16, -2, 12 ).drive( 2, 0.1f, 0.96f, 12 ).bends( 0.35f, 0.3f, 0.3f ).posts( 0 )
        .p( LAVA_ROCK, 0.2f, 3, 80 ).p( HILLOCK, 0.02f, 3, 11, 0.3f, 0.5f ).c( 0xF4F2EC ).p( CAMEL, 0.006f, 10, 60 );
    W( b, BE::LAAS_GEEL, "LAAS GEEL", "SOMALIA", S::DESERT ).land( 0xC8A878, 0xBC9C70, 0xB8A070 ).verge( 0xB88A60 ).road( 0xA88A68, 0xA88A68 )
        .hills( 14, 4, 4 ).drive( 2, 0.08f, 0.88f, 12 ).bends( 0.35f, 0.3f, 0.25f ).posts( 0 )
        .p( GRANITE_DOME, 0.006f, 60, 250, 0.4f, 0.7f ).v( 1 ).p( ACACIA, 0.06f, 6, 150 ).p( CAMEL, 0.03f, 8, 120 ).p( SHRUB, 0.12f, 2, 60 ).c( 0x8A8A50 )
        .p( HUT, 0.006f, 8, 60 ).p( TERMITE_MOUND, 0.01f, 4, 60 );
    W( b, BE::NGORONGORO, "NGORONGORO", "TANZANIA", S::MORNING ).clouds( 0.45f ).land( 0x8AA850, 0x7E9C48, 0x7A9A58 ).ranges( 0x4A6A4A, 0x7A9A80, 9 )
        .hills( 40, -30, 20, 0.15f ).water( 0x8AA8B0 ).drive( 2, 0.15f, 0.9f, 12 ).bends( 0.6f, 0.5f, 0.6f ).road( 0xA88A68, 0xA88A68 ).posts( 0 )
        .p( ACACIA, 0.06f, 8, 150 ).p( ZEBRA, 0.06f, 10, 150 ).p( ELEPHANT, 0.01f, 20, 150 ).p( HUT, 0.006f, 10, 60 ).p( COW, 0.02f, 10, 80 ).v( 2 )
        .p( FLAMINGO, 0.02f, 40, 200 );

    // ---- Southern Africa and the islands -----------------------------------------------------------------------------
    W( b, BE::BAZARUTO, "BAZARUTO", "MOZAMBIQUE", S::TROPICAL ).sea( 1, 1, -2 ).water( 0x30C0C8 ).land( 0xF0E8D8, 0xE6DECE, 0x7AA058 )
        .verge( 0xF0E8D8 ).hills( 8, 0, 0 ).drive( 2, 0.1f, 0.94f, 12 ).bends( 0.3f, 0.3f, 0.2f ).road( 0xC8B898, 0xC8B898 ).sand().posts( 0 )
        .p( DUNE, 0.05f, 3, 11, 0.6f, 1.0f ).c( 0xF4ECDC ).p( PALM, 0.2f, 2, 11 ).p( SAILBOAT, 0.03f, 40, 200 ).c( 0xF0ECE0, 0xF0ECE0 ).floating()
        .p( HUT, 0.01f, 3, 11 );
    W( b, BE::LAKE_MALAWI, "LAKE MALAWI", "MALAWI", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x2A8AB0 ).land( 0x8AA050, 0x809448, 0x7A9A58 )
        .verge( 0xE0D4B8 ).hills( 20, 0, 14 ).drive( 2, 0.3f, 0.94f, 12 ).bends( 0.45f, 0.4f, 0.35f )
        .p( BAOBAB, 0.03f, 6, 120 ).p( BOULDER, 0.04f, 3, 60 ).p( HUT, 0.02f, 6, 60 ).p( PALM, 0.08f, 3, 40 )
        .p( BOAT, 0.03f, 40, 150 ).c( 0x6A4A30, 0xE8E0D0 ).floating().p( BANANA, 0.06f, 3, 40 );
    W( b, BE::GREAT_ZIMBABWE, "GREAT ZIMBABWE", "ZIMBABWE", S::GOLDEN ).land( 0x9AA058, 0x909450, 0x9A9A62 ).hills( 16, 4, 4 )
        .drive( 2, 0.25f, 0.95f, 12 ).bends( 0.4f, 0.35f, 0.4f )
        .landmark( Lm::GREAT_ZIMBABWE, 50, 90 ).p( KOPJE, 0.012f, 40, 250 ).p( BROADLEAF, 0.15f, 4, 120 ).c( 0xA86A3A ).p( BOULDER, 0.05f, 4, 80 )
        .p( HUT, 0.01f, 8, 60 ).p( ACACIA, 0.03f, 8, 120 );
    W( b, BE::SANI_PASS, "SANI PASS", "LESOTHO", S::ALPINE ).land( 0x8A9A5A, 0x7E8E52, 0x8A8A6A ).verge( 0x8A8478 ).road( 0x8A8070, 0x8A8070 )
        .ranges( 0x5A5E64, 0xD8DCE2, 12 ).hills( 60, 50, -40 ).drive( 2, 0.08f, 0.82f, 12 ).bends( 1.0f, 1.0f, 1.0f ).posts( 0 )
        .p( ROCK, 0.3f, 3, 60 ).p( SHRUB, 0.2f, 2, 50, 0.5f, 0.8f ).c( 0x8A8A50 ).p( RONDAVEL, 0.008f, 10, 80 ).c( 0x8A8278, 0x9A8A5A )
        .p( SHEEP, 0.03f, 8, 80 ).p( HORSE, 0.01f, 8, 60 );
    W( b, BE::EZULWINI, "EZULWINI VALLEY", "ESWATINI", S::MORNING ).land( 0x6EA048, 0x649440, 0x6E9450 ).fields( 0.35f, 0x8AC050, 0x6AA040, 0xA8B060 )
        .ranges( 0x4A6A50, 0x7A9A88, 8 ).hills( 30, 10, 10 ).drive( 2, 0.5f, 0.97f, 12 ).bends( 0.5f, 0.45f, 0.5f )
        .p( RONDAVEL, 0.03f, 8, 80 ).c( 0xE8E0D0, 0x9A8A5A ).p( HUT, 0.01f, 8, 60 ).p( COW, 0.04f, 8, 80 ).p( BROADLEAF, 0.1f, 4, 100 ).c( 0x4E8A3A )
        .p( EUCALYPTUS, 0.05f, 6, 100 );
    W( b, BE::CHAMAREL, "CHAMAREL", "MAURITIUS", S::TROPICAL ).sea( 0, 1, -30 ).water( 0x2AA8C0 ).land( 0x5E9A44, 0x56903E, 0x6A9A4E )
        .fields( 0.35f, 0x8AC050, 0x7AB048, 0x9AC060 ).hills( 30, 14, -10 ).drive( 2, 0.4f, 0.95f, 12 ).bends( 0.65f, 0.55f, 0.6f )
        .p( BADLAND, 0.01f, 20, 80, 0.4f, 0.6f ).c( 0xC87850, 0x8A5A8A ).p( WATERFALL, 0.003f, 60, 200, 0.6f, 0.8f ).v( 1 ).p( PALM, 0.1f, 3, 60 )
        .p( FLAT_HOUSE, 0.02f, 6, 60 ).c( 0xE8C890, 0xC83A2A ).pastel().p( JUNGLE_TREE, 0.08f, 10, 150 );
    W( b, BE::KARTHALA, "GRANDE COMORE", "COMOROS", S::TROPICAL ).sea( 0, 1, -6 ).water( 0x1E7AA0 ).land( 0x4E8A3A, 0x468034, 0x3A3634 )
        .verge( 0x2E2A28 ).hills( 24, 10, -4 ).drive( 2, 0.3f, 0.94f, 12 ).bends( 0.55f, 0.5f, 0.45f )
        .p( VOLCANO_CONE, 0.005f, 300, 600, 1.4f, 1.8f ).c( 0x3A3634, 0x4A4440 ).p( LAVA_ROCK, 0.12f, 3, 60 ).p( PALM, 0.2f, 2, 60 )
        .p( MOSQUE, 0.004f, 20, 100 ).c( 0xF4F2EC, 0xF0EEE6 ).p( FLAT_HOUSE, 0.03f, 6, 60 ).c( 0xF0ECE0, 0x8A8A8A );
    W( b, BE::CHAPMANS_PEAK, "CHAPMANS PEAK", "SOUTH AFRICA", S::NOON ).sea( 0, 1, -40 ).water( 0x1E5A8A ).land( 0x7A8A50, 0x708048, 0x8A8A60 )
        .verge( 0xB89A70 ).hills( 40, 40, -40 ).drive( 2, 0.4f, 0.98f, 12 ).bends( 0.8f, 0.7f, 0.5f ).tunnels( 0.15f ).rails()
        .p( SHRUB, 0.35f, 2, 60, 0.5f, 0.9f ).c( 0x6A7A48 ).p( FLOWER_BUSH, 0.06f, 2, 30, 0.5f, 0.8f ).c( 0xE88A3A )
        .p( PEAK, 0.006f, 150, 350, 0.7f, 1.0f ).c( 0x8A8070, 0x8A8070 ).p( ROCK, 0.1f, 3, 40 );
    W( b, BE::GARDEN_ROUTE, "GARDEN ROUTE", "SOUTH AFRICA", S::MORNING ).sea( 0, 1, -20 ).water( 0x1E6A8A ).land( 0x5E9A44, 0x56903E, 0x4E8048 )
        .hills( 30, 14, -10 ).drive( 2, 0.5f, 0.98f, 12 ).bends( 0.5f, 0.45f, 0.5f ).bridges( 0.2f, BR::ARCH, 0xC8C0B0 )
        .p( BROADLEAF, 0.35f, 4, 150 ).c( 0x3E7A34 ).p( TREE_FERN, 0.06f, 2, 30 ).p( FLAT_HOUSE, 0.01f, 8, 60 ).c( 0xF4F2EA, 0x4A4E58 )
        .p( LIGHTHOUSE, 0.003f, 30, 120 ).p( SAILBOAT, 0.01f, 60, 250 ).floating();
    W( b, BE::KRUGER, "KRUGER", "SOUTH AFRICA", S::GOLDEN ).land( 0x9AA050, 0x909448, 0x9A9A5E ).hills( 8, 2, 2 ).drive( 2, 0.2f, 0.97f, 12 )
        .bends( 0.3f, 0.3f, 0.2f ).posts( 0 )
        .p( ELEPHANT, 0.02f, 15, 150 ).p( GIRAFFE, 0.02f, 15, 150 ).p( ZEBRA, 0.05f, 10, 150 ).p( ACACIA, 0.08f, 6, 150 ).p( KOPJE, 0.006f, 40, 200 )
        .p( BAOBAB, 0.008f, 15, 150 ).p( GRASS_TUFT, 0.3f, 1, 40 ).p( WARNING_SIGN, 0.004f, 1, 1, 1, 1 ).v( 4 );
    W( b, BE::ETOSHA, "ETOSHA", "NAMIBIA", S::DESERT ).land( 0xE8E4D8, 0xDCD8CC, 0xE0DCD0 ).verge( 0xD8D2C4 ).shine( 0.15f ).hills( 2, 0, 0, 0.08f )
        .water( 0x8AA0A0 ).drive( 2, 0.12f, 0.92f, 12 ).bends( 0.2f, 0.25f, 0.05f ).road( 0xC8C0B0, 0xC8C0B0 ).posts( 0 )
        .p( ZEBRA, 0.05f, 10, 150 ).p( GIRAFFE, 0.015f, 15, 150 ).p( ELEPHANT, 0.015f, 20, 150 ).p( ACACIA, 0.02f, 8, 150 ).p( GRASS_TUFT, 0.15f, 1, 40 );

    // ---- The Middle East and the Caucasus ----------------------------------------------------------------------------
    W( b, BE::DOHA, "DOHA", "QATAR", S::DUSK ).sea( 0, 1, -3 ).water( 0x2A5A7A ).land( 0xD8C8A8, 0xCCBC9C, 0xD0BC98 ).drive( 4, 1.1f, 1.0f, 0 )
        .bends( 0.3f, 0.3f, 0.05f ).lamps( 30 ).posts( 0 ).gantries( 700 )
        .p( GLASS_TOWER, 0.15f, 20, 250 ).c( 0x4A6A8A ).p( SUPERTALL, 0.002f, 200, 400 ).p( DATE_PALM, 0.25f, 2, 8 ).p( MOSQUE, 0.003f, 30, 150 ).c( 0xE8DCC0, 0xD8C8A0 )
        .p( BOAT, 0.03f, 40, 150 ).c( 0x8A5A3A, 0xE8E0D0 ).floating();
    W( b, BE::KING_FAHD, "KING FAHD CAUSEWAY", "BAHRAIN", S::DESERT ).sea( 1, 1, -2.5f ).water( 0x30B8C0 ).land( 0xE0D4B8, 0xD4C8AC, 0xD8CCB0 )
        .drive( 4, 0.9f, 1.0f, 0 ).bends( 0.15f, 0.2f, 0.05f ).lamps( 40 ).posts( 0 ).bridges( 0.8f, BR::CAUSEWAY, 0xE0DCD4 )
        .p( TV_TOWER, 0.0f, 20, 40, 0.7f, 0.7f ).v( 0 ).once().p( DATE_PALM, 0.2f, 2, 11 ).p( SHIP, 0.004f, 80, 300 ).floating()
        .p( BOAT, 0.01f, 40, 150 ).c( 0xF0F0F0, 0x2A4A6A ).floating();
    W( b, BE::KUWAIT_CITY, "KUWAIT CITY", "KUWAIT", S::DESERT ).sea( 0, 1, -3 ).water( 0x2A7A9A ).land( 0xD8C8A0, 0xCCBC96, 0xD0BC98 )
        .drive( 4, 1.2f, 1.0f, 0 ).bends( 0.25f, 0.3f, 0.05f ).lamps( 30 ).posts( 0 )
        .landmark( Lm::KUWAIT_TOWERS, 25, 40 ).p( GLASS_TOWER, 0.1f, 20, 200 ).c( 0x7A8AA0 ).p( DATE_PALM, 0.2f, 2, 10 ).p( MOSQUE, 0.004f, 30, 150 ).c( 0xE8E0D0, 0x3A6A8A )
        .p( FLAT_HOUSE, 0.04f, 6, 40 ).c( 0xE8DCC0, 0xD8C8A8 );
    W( b, BE::MESOPOTAMIA, "MESOPOTAMIAN MARSHES", "IRAQ", S::HAZE ).sea( 0.7f, 0.7f, -1.5f ).water( 0x5A7A6A, 0.85f ).land( 0xB8B080, 0xACA476, 0xA8A07A )
        .hills( 0, 0, 0, 0.5f ).drive( 2, 0.1f, 0.93f, 11 ).bends( 0.25f, 0.25f, 0.0f ).posts( 0 )
        .p( REEDS, 0.5f, 2, 11 ).p( REED_HOUSE, 0.03f, 4, 11 ).p( BOAT, 0.04f, 36, 150 ).c( 0x3A2A20, 0x3A2A20 ).floating().p( COW, 0.03f, 3, 11 ).v( 0 )
        .p( DATE_PALM, 0.04f, 3, 11 );
    W( b, BE::NEGEV, "NEGEV", "ISRAEL", S::DESERT ).land( 0xC8A880, 0xBC9C76, 0xB8987A ).verge( 0xA87A5A ).hills( 30, -20, 10 )
        .drive( 2, 0.25f, 0.98f, 11 ).bends( 0.45f, 0.45f, 0.5f )
        .p( BADLAND, 0.02f, 40, 250, 0.8f, 1.4f ).c( 0xC8A07A, 0x9A5A4A ).p( BUTTE, 0.004f, 100, 300 ).p( ACACIA, 0.03f, 6, 100, 0.5f, 0.8f )
        .p( CAMEL, 0.008f, 10, 100 ).p( ROCK, 0.1f, 3, 60 );
    W( b, BE::BETHLEHEM, "BETHLEHEM HILLS", "PALESTINE", S::GOLDEN ).land( 0xA8A06A, 0x9C9462, 0xA89C74 ).verge( 0xD8CCB0 ).hills( 24, 10, 10 )
        .drive( 2, 0.6f, 0.97f, 11 ).bends( 0.55f, 0.5f, 0.6f )
        .p( OLIVE, 0.3f, 4, 120 ).p( STONE_WALL, 0.3f, 3, 3, 1, 1 ).p( TERRACES, 0.02f, 20, 120 ).c( 0x9A9A5A, 0xC8B898 )
        .p( FLAT_HOUSE, 0.05f, 6, 80 ).c( 0xE8DCC0, 0xD8C8A8 ).p( CHURCH, 0.004f, 30, 150 ).c( 0xE8DCC0, 0x6A6A6A ).p( CYPRESS, 0.04f, 4, 60 );
    W( b, BE::PALMYRA, "PALMYRA", "SYRIA", S::DESERT ).land( 0xD8B888, 0xCCAC80, 0xD0B080 ).hills( 8, 2, 2 ).drive( 2, 0.1f, 0.96f, 11 )
        .bends( 0.2f, 0.25f, 0.1f ).posts( 0 )
        .landmark( Lm::TRIUMPHAL_ARCH, 40, 70, 0.6f ).p( COLUMNS, 0.04f, 10, 120 ).p( RUIN, 0.02f, 10, 120 ).p( DATE_PALM, 0.06f, 4, 80 )
        .p( CAMEL, 0.015f, 10, 120 ).p( WATCHTOWER, 0.004f, 30, 150 ).c( 0xD8B888 ).v( 1 );
    W( b, BE::CYPRUS, "TROODOS", "CYPRUS", S::NOON ).land( 0x7A9050, 0x708648, 0x86905E ).verge( 0xB8A88A ).hills( 36, 16, 16 )
        .drive( 2, 0.4f, 0.97f, 19 ).bends( 0.75f, 0.65f, 0.75f )
        .p( PINE, 0.35f, 4, 150 ).p( CHURCH, 0.004f, 20, 120 ).c( 0xD8C8A8, 0x8A5A3A ).p( VINE_ROW, 0.25f, 8, 40, 1, 1 ).p( OLIVE, 0.08f, 6, 80 )
        .p( COTTAGE, 0.015f, 8, 60 ).c( 0xE0D4BC, 0xB0583A );
    W( b, BE::PAMUKKALE, "PAMUKKALE", "TURKEY", S::NOON ).land( 0xA8A868, 0x9E9E60, 0xF0F0EA ).hills( 20, 16, 0 ).drive( 2, 0.4f, 0.97f, 11 )
        .bends( 0.4f, 0.35f, 0.4f )
        .p( TERRACES, 0.04f, 20, 150, 1.0f, 1.4f ).c( 0xF4F4F0, 0xE8E8E2 ).p( COLUMNS, 0.01f, 10, 80 ).p( RUIN, 0.008f, 10, 80 ).p( POPLAR, 0.08f, 4, 60 )
        .p( FLAT_HOUSE, 0.02f, 8, 60 ).c( 0xF0ECE0, 0xB0583A ).p( BALLOON, 0.003f, 100, 300 );
    W( b, BE::ATHENS, "ATHENS", "GREECE", S::NOON ).land( 0xA0A068, 0x969660, 0xA8A074 ).drive( 3, 1.3f, 0.97f, 19 ).bends( 0.4f, 0.4f, 0.3f )
        .lamps( 35 ).posts( 0 )
        .landmark( Lm::ACROPOLIS, 160, 260 ).p( FLAT_HOUSE, 0.15f, 4, 40 ).c( 0xF0EADC, 0xD8D0C0 ).p( OLIVE, 0.06f, 4, 60 ).p( CYPRESS, 0.04f, 4, 60 )
        .p( PANEL_BLOCK, 0.03f, 20, 120 ).c( 0xE8E0D0 ).p( CHURCH, 0.004f, 20, 120 ).c( 0xE8D8C0, 0xB0583A );
    W( b, BE::ARARAT, "ARARAT PLAIN", "ARMENIA", S::MORNING ).land( 0x9AA458, 0x909850, 0xA09A66 ).fields( 0.4f, 0xC8B860, 0x8AA050, 0xA8905A )
        .hills( 10, 2, 2 ).drive( 2, 0.4f, 0.98f, 22 ).bends( 0.3f, 0.3f, 0.2f )
        .p( PEAK, 0.0f, 400, 500, 3.2f, 3.2f ).c( 0x6A5A50, 0xF4F6FA ).once().p( CHURCH, 0.004f, 30, 150 ).c( 0xC8A890, 0x8A7A70 )
        .p( BROADLEAF, 0.1f, 6, 100 ).c( 0x6A9A40 ).p( VINE_ROW, 0.3f, 6, 30, 1, 1 ).p( COTTAGE, 0.01f, 10, 80 ).c( 0xC89A80, 0x8A8A8A );
    W( b, BE::CASPIAN, "CASPIAN COAST", "AZERBAIJAN", S::HAZE ).sea( 0, 1, -3 ).water( 0x3A6A7A ).land( 0xB8A880, 0xACA076, 0xA89C7A )
        .drive( 2, 0.6f, 0.98f, 22 ).bends( 0.3f, 0.3f, 0.1f ).posts( 0 )
        .p( PUMPJACK, 0.05f, 10, 150 ).p( PIPELINE, 1.0f, 9, 9, 1, 1 ).p( GLASS_TOWER, 0.006f, 80, 250 ).c( 0x5A6A8A ).p( SHRUB, 0.1f, 2, 60 ).c( 0x8A8A5A )
        .p( FLAT_HOUSE, 0.01f, 10, 60 ).c( 0xE0D4BC, 0x8A8A8A );
    W( b, BE::DARVAZA, "DARVAZA", "TURKMENISTAN", S::NIGHT ).lamps( 0 ).land( 0x8A7A60, 0x806E56, 0x5A4E40 ).verge( 0x6A5A48 ).hills( 8, 2, 2 )
        .drive( 2, 0.04f, 0.86f, 22 ).bends( 0.3f, 0.3f, 0.2f ).road( 0x5A5450, 0x5A5450 ).sand().posts( 0 )
        .landmark( Lm::FIRE_CRATER, 80, 140 ).p( DUNE, 0.03f, 40, 300, 0.8f, 1.6f ).c( 0x8A7454 ).p( SHRUB, 0.12f, 2, 60 ).c( 0x5A5A40 ).p( CAMEL, 0.006f, 10, 80 );
    W( b, BE::BAND_E_AMIR, "BAND-E AMIR", "AFGHANISTAN", S::ALPINE ).sea( 1, 0, -10 ).water( 0x1A58C8 ).land( 0xC8A878, 0xBC9C70, 0xB89A70 )
        .verge( 0xF0EEE6 ).ranges( 0x8A7A6A, 0xD8D0C8, 9 ).hills( 30, 0, 24 ).drive( 2, 0.08f, 0.88f, 22 ).bends( 0.5f, 0.45f, 0.5f )
        .road( 0x9A8A70, 0x9A8A70 ).posts( 0 )
        .p( BUTTE, 0.006f, 80, 300 ).p( SHRUB, 0.1f, 2, 60 ).c( 0x8A8A50 ).p( ADOBE, 0.01f, 10, 80 ).c( 0xB89A70, 0xA88A60 ).p( SHEEP, 0.03f, 8, 80 );

    // ---- The Balkans, Central and Eastern Europe ---------------------------------------------------------------------
    W( b, BE::ALBANIAN_RIVIERA, "ALBANIAN RIVIERA", "ALBANIA", S::NOON ).sea( 0, 1, -40 ).water( 0x1E7AB0 ).land( 0x7E9050, 0x748648, 0x8A9060 )
        .verge( 0xC8C0B0 ).hills( 40, 30, -30 ).drive( 2, 0.4f, 0.97f, 19 ).bends( 0.85f, 0.8f, 0.85f )
        .p( OLIVE, 0.15f, 4, 100 ).p( DOME, 0.02f, 4, 40, 0.15f, 0.2f ).p( FLAT_HOUSE, 0.03f, 6, 60 ).c( 0xF0ECE0, 0xB0583A ).p( CYPRESS, 0.05f, 4, 60 )
        .p( SHRUB, 0.2f, 2, 60 ).c( 0x6A7A48 ).p( SAILBOAT, 0.008f, 60, 250 ).floating();
    W( b, BE::LAKE_OHRID, "LAKE OHRID", "NORTH MACEDONIA", S::GOLDEN ).sea( 0, 1, -6 ).water( 0x2A7A9A ).land( 0x7A9A50, 0x709048, 0x80925E )
        .hills( 30, 16, -6 ).drive( 2, 0.4f, 0.97f, 19 ).bends( 0.55f, 0.5f, 0.5f )
        .p( CHURCH, 0.006f, 8, 80 ).c( 0xB89878, 0x8A4A3A ).p( ROW_HOUSES, 0.05f, 6, 40 ).c( 0xF0ECE0, 0x8A4A3A ).p( BROADLEAF, 0.1f, 4, 80 ).c( 0x5A8A40 )
        .p( BOAT, 0.03f, 40, 150 ).c( 0x2A6AAA, 0xF0F0F0 ).pastel().floating().p( CASTLE, 0.002f, 80, 200 ).c( 0x9A8A78, 0x8A4A3A );
    W( b, BE::KOSOVO, "RUGOVA GORGE", "KOSOVO", S::MORNING ).land( 0x5E9446, 0x568A40, 0x5E8450 ).verge( 0x9A9690 ).sea( 0, 1, -18 ).water( 0x3A8A8A )
        .hills( 50, 60, -10 ).drive( 2, 0.3f, 0.95f, 19 ).bends( 0.7f, 0.65f, 0.6f ).tunnels( 0.3f )
        .p( SPRUCE, 0.35f, 4, 150 ).p( ROCK, 0.12f, 3, 50 ).p( COTTAGE, 0.008f, 10, 60 ).c( 0xE8E0D0, 0x8A4A3A ).p( SHEEP, 0.02f, 8, 60 );
    W( b, BE::MOSTAR, "MOSTAR", "BOSNIA", S::GOLDEN ).sea( 0, 1, -18 ).water( 0x3AA890 ).land( 0x8A9A58, 0x809050, 0x8E9466 ).verge( 0xD8D0C0 )
        .hills( 30, 20, -10 ).drive( 2, 0.5f, 0.97f, 19 ).bends( 0.5f, 0.45f, 0.5f ).bridges( 0.4f, BR::ARCH, 0xD8D0C0 )
        .p( MOSQUE, 0.006f, 20, 100 ).c( 0xE8E0D0, 0x8A8A90 ).p( ROW_HOUSES, 0.06f, 6, 40 ).c( 0xD8D0C0, 0x6A6A6A ).p( CHURCH, 0.003f, 30, 120 ).c( 0xE8E0D0, 0x6A6060 )
        .p( FLAT_HOUSE, 0.03f, 6, 60 ).c( 0xE8E0D0, 0xB0583A ).p( CYPRESS, 0.04f, 4, 60 );
    W( b, BE::LAKE_BLED, "LAKE BLED", "SLOVENIA", S::MORNING ).sea( 0, 1, -3 ).water( 0x2A7A7A ).land( 0x62A048, 0x589640, 0x5E8E50 )
        .ranges( 0x5A6878, 0xE0E6F0, 12 ).hills( 30, 16, 0 ).drive( 2, 0.4f, 0.98f, 19 ).bends( 0.5f, 0.45f, 0.4f )
        .landmark( Lm::ISLAND_CHURCH, 120, 180 ).floating().p( CASTLE, 0.003f, 60, 160 ).c( 0xD8D0C0, 0x8A4A3A ).p( SPRUCE, 0.3f, 4, 150 )
        .p( CHALET, 0.02f, 8, 60 ).c( 0xF0EADC, 0x6A4A3A ).p( BOAT, 0.02f, 40, 120 ).c( 0x8A5A3A, 0xF0F0F0 ).floating();
    W( b, BE::IRON_GATES, "IRON GATES", "SERBIA", S::HAZE ).sea( 0, 1, -8 ).water( 0x4A6A6A ).land( 0x5E8E46, 0x568440, 0x5A7E4A ).verge( 0x8A8478 )
        .hills( 50, 50, -8 ).drive( 2, 0.4f, 0.97f, 19 ).bends( 0.55f, 0.5f, 0.35f ).tunnels( 0.25f )
        .p( BROADLEAF, 0.35f, 4, 150 ).c( 0x4A7A3A ).p( CASTLE, 0.004f, 30, 120 ).c( 0xA8A090, 0x8A4A3A ).p( ROCK, 0.08f, 3, 50 )
        .p( SHIP, 0.003f, 80, 250, 0.6f, 0.8f ).floating().p( COTTAGE, 0.006f, 10, 60 ).c( 0xE8E0D0, 0xB0583A );
    W( b, BE::ORHEI, "ORHEIUL VECHI", "MOLDOVA", S::MORNING ).land( 0x7EA44E, 0x749848, 0x8A9A5E ).fields( 0.4f, 0xF0D030, 0x8AB04A, 0xB89A5A )
        .verge( 0xC8C0A8 ).hills( 24, -16, 14 ).drive( 2, 0.3f, 0.95f, 19 ).bends( 0.5f, 0.45f, 0.5f )
        .p( VINE_ROW, 0.4f, 6, 30, 1, 1 ).p( CHURCH, 0.004f, 30, 150 ).c( 0xF0ECE0, 0x6A7A9A ).p( COTTAGE, 0.03f, 6, 60 ).c( 0x7AA8D8, 0x8A8A8A ).pastel()
        .p( WAYSIDE_CROSS, 0.006f, 3, 10 ).p( BROADLEAF, 0.08f, 4, 80 ).c( 0x5A8A40 );
    W( b, BE::TRANSFAGARASAN, "TRANSFAGARASAN", "ROMANIA", S::ALPINE ).land( 0x6E9E4C, 0x649244, 0x7A9064 ).verge( 0x8A8680 ).ranges( 0x5A6476, 0xE8ECF4, 13 )
        .hills( 60, 40, -40 ).drive( 2, 0.4f, 0.97f, 19 ).bends( 1.0f, 0.95f, 1.0f ).tunnels( 0.06f ).rails()
        .p( PEAK, 0.006f, 150, 400, 1.0f, 1.4f ).c( 0x6A6C70, 0xF0F2F6 ).p( SPRUCE, 0.2f, 10, 150 ).p( ROCK, 0.15f, 3, 60 ).p( SHEEP, 0.04f, 8, 80 )
        .p( SNOWBANK, 0.06f, 1, 6, 0.5f, 0.9f );
    W( b, BE::SPIS, "SPIS CASTLE", "SLOVAKIA", S::MORNING ).land( 0x7AA44E, 0x6E9848, 0x86A05E ).fields( 0.5f, 0xD8C25A, 0x8AB04A, 0xB8A860 )
        .ranges( 0x6A7888, 0xD0D8E4, 8 ).hills( 20, 6, 6 ).drive( 2, 0.5f, 0.98f, 6 ).bends( 0.45f, 0.4f, 0.45f )
        .p( CASTLE, 0.0f, 150, 250, 1.8f, 1.8f ).c( 0xD8D0C0, 0x8A8A8A ).once().p( CHURCH, 0.004f, 30, 150 ).c( 0xF0ECE0, 0x5A5A60 )
        .p( COTTAGE, 0.02f, 8, 70 ).c( 0xF0ECE0, 0x8A4A3A ).p( BROADLEAF, 0.06f, 4, 80 ).c( 0x5A8A40 ).p( HAYBALE, 0.01f, 10, 60 );
    W( b, BE::BELOVEZHSKAYA, "BELOVEZHSKAYA PUSHCHA", "BELARUS", S::MIST ).land( 0x4E8040, 0x46763A, 0x3E6A3C ).hills( 4, 2, 2, 0.15f )
        .drive( 2, 0.15f, 0.96f, 22 ).bends( 0.35f, 0.3f, 0.15f )
        .p( SPRUCE, 0.4f, 3, 150 ).p( BROADLEAF, 0.25f, 3, 150, 1.0f, 1.4f ).c( 0x3E6A30 ).p( BISON, 0.012f, 10, 80 ).p( COTTAGE, 0.006f, 10, 60 ).c( 0x7AA0C8, 0x8A8A8A )
        .p( FERN, 0.2f, 1, 20 );
    W( b, BE::LAHEMAA, "LAHEMAA", "ESTONIA", S::MORNING ).sea( 0, 1, -2 ).water( 0x3A5A6A ).land( 0x6A9A4A, 0x608E44, 0x5E8450 ).verge( 0xD8D0B8 )
        .hills( 6, 2, 0, 0.2f ).drive( 2, 0.2f, 0.98f, 23 ).bends( 0.4f, 0.35f, 0.15f )
        .p( BOULDER, 0.06f, 4, 120, 1.0f, 2.0f ).p( PINE, 0.3f, 4, 150 ).p( BIRCH, 0.1f, 3, 80 ).p( CABIN, 0.01f, 3, 11 )
        .p( CHURCH, 0.003f, 30, 150 ).c( 0xF0ECE0, 0x5A5A60 );
    W( b, BE::GAUJA, "GAUJA VALLEY", "LATVIA", S::MORNING ).land( 0x5E9446, 0x568A40, 0x527E48 ).verge( 0xA85A3A ).sea( 0, 1, -10 ).water( 0x5A5A48 )
        .hills( 24, 14, -6 ).drive( 2, 0.3f, 0.98f, 23 ).bends( 0.5f, 0.45f, 0.5f )
        .p( CASTLE, 0.004f, 40, 160 ).c( 0xA84A30, 0x8A3A2A ).p( SPRUCE, 0.35f, 4, 150 ).p( BIRCH, 0.12f, 3, 80 ).p( COTTAGE, 0.008f, 10, 60 ).c( 0xC8A070, 0x5A5A60 );
    W( b, BE::ST_PETERSBURG, "SAINT PETERSBURG", "RUSSIA", S::DUSK ).sea( 0.5f, 0.5f, -3 ).water( 0x3A4A6A ).land( 0x6E8A58, 0x668250, 0x707A68 )
        .drive( 3, 1.1f, 0.97f, 0 ).bends( 0.3f, 0.3f, 0.0f ).posts( 0 ).bridges( 0.3f, BR::TRUSS, 0x3A4048 )
        .p( ROW_HOUSES, 0.2f, 3, 11 ).c( 0xE8C890, 0x5A6A5A ).pastel().p( ONION_CHURCH, 0.006f, 20, 150 ).c( 0xF0ECE0, 0xD8A84A )
        .p( CHURCH, 0.004f, 20, 150 ).c( 0x6AA8A0, 0xD8C8A0 ).p( BOAT, 0.02f, 36, 100 ).c( 0xF0F0F0, 0x2A3A5A ).floating();
    W( b, BE::MOSCOW, "MOSCOW", "RUSSIA", S::WINTER ).snow( 0.3f ).land( 0xE0E4EA, 0xD4DAE2, 0xC8D0DA ).verge( 0xC8CCD2 ).drive( 4, 1.6f, 0.9f, 1 )
        .bends( 0.3f, 0.3f, 0.1f ).lamps( 30 ).posts( 0 ).gantries( 800 )
        .landmark( Lm::ST_BASIL, 60, 100 ).p( PANEL_BLOCK, 0.1f, 15, 150 ).c( 0xD8D0C0 ).p( ONION_CHURCH, 0.004f, 30, 150 ).c( 0xF0ECE0, 0xD8A84A )
        .p( GLASS_TOWER, 0.03f, 60, 250 ).c( 0x5A7A9A ).p( SUPERTALL, 0.001f, 200, 400 ).p( SNOWBANK, 0.2f, 0.5f, 3, 0.8f, 1.4f );

    // ---- Russia and Central Asia -------------------------------------------------------------------------------------
    W( b, BE::CHUYSKY, "CHUYSKY TRAKT", "RUSSIA", S::ALPINE ).sea( 0, 1, -8 ).water( 0x4AB0B0 ).land( 0x7A9A50, 0x708E48, 0x7A8A60 )
        .ranges( 0x5A6476, 0xE8ECF4, 12 ).hills( 40, 24, -6 ).drive( 2, 0.25f, 0.97f, 22 ).bends( 0.55f, 0.5f, 0.55f )
        .p( SPRUCE, 0.25f, 4, 150 ).p( PEAK, 0.004f, 250, 500, 1.2f, 1.5f ).c( 0x6A6A70, 0xF0F2F6 ).p( HORSE, 0.03f, 10, 100 ).p( YURT, 0.004f, 20, 100 )
        .p( ROCK, 0.08f, 3, 50 );
    W( b, BE::ROAD_OF_BONES, "ROAD OF BONES", "RUSSIA", S::WINTER ).snow( 0.4f ).land( 0xE4E8EE, 0xD8DEE6, 0xCCD4DE ).verge( 0xB8BCC4 )
        .road( 0x8A8680, 0x8A8680 ).hills( 12, 4, 4 ).drive( 2, 0.05f, 0.78f, 22 ).bends( 0.35f, 0.35f, 0.3f ).posts( 0 )
        .p( SNOW_PINE, 0.15f, 6, 200, 0.5f, 0.9f ).p( DEAD_TREE, 0.04f, 6, 80 ).p( SNOWBANK, 0.2f, 0.5f, 3, 0.8f, 1.4f ).p( CABIN, 0.003f, 15, 80 )
        .p( PANEL_BLOCK, 0.001f, 40, 150 ).c( 0x9A9A9A, 0x6A6A6A, 0x1A1E26 );
    W( b, BE::CHARYN, "CHARYN CANYON", "KAZAKHSTAN", S::DESERT ).land( 0xB8A070, 0xAC9468, 0xB89A6E ).verge( 0xB86A40 ).hills( 24, -30, 8 )
        .drive( 2, 0.1f, 0.93f, 22 ).bends( 0.35f, 0.3f, 0.3f ).road( 0x9A8A70, 0x9A8A70 ).posts( 0 )
        .p( BADLAND, 0.04f, 20, 250, 0.8f, 1.5f ).c( 0xC86A40, 0x9A4A30 ).p( BUTTE, 0.004f, 100, 300 )
        .p( SHRUB, 0.1f, 2, 60 ).c( 0x8A8A50 ).p( HORSE, 0.006f, 10, 80 );

    // ---- South Asia --------------------------------------------------------------------------------------------------
    W( b, BE::SPITI, "SPITI VALLEY", "INDIA", S::ALPINE ).land( 0xB8A078, 0xAC9470, 0xA89A80 ).verge( 0x9A8A70 ).ranges( 0x8A7A70, 0xF0F0F4, 14 )
        .hills( 50, 30, -30 ).drive( 2, 0.1f, 0.88f, 10 ).bends( 0.75f, 0.7f, 0.7f ).road( 0x8A8070, 0x8A8070 ).posts( 0 )
        .p( FLAT_HOUSE, 0.01f, 30, 150 ).c( 0xF4F2EA, 0x6A3A2A ).p( STUPA, 0.008f, 10, 100 ).c( 0xF4F2EA, 0xD8A84A ).p( PRAYER_FLAGS, 0.03f, 4, 40 )
        .p( YAK, 0.02f, 10, 80 ).p( PEAK, 0.004f, 250, 500, 1.2f, 1.6f ).c( 0x7A6A60, 0xF4F6FA ).p( ROCK, 0.12f, 3, 60 );
    W( b, BE::MANALI, "MANALI HIGHWAY", "INDIA", S::ALPINE ).land( 0x6A9A48, 0x609040, 0x6E8E58 ).ranges( 0x5A6476, 0xF0F2F6, 14 )
        .hills( 50, 40, -30 ).drive( 2, 0.6f, 0.92f, 10 ).bends( 0.9f, 0.85f, 0.95f ).tunnels( 0.08f ).posts( 0 )
        .p( CEDAR, 0.3f, 4, 150 ).p( PEAK, 0.005f, 220, 450, 1.2f, 1.6f ).c( 0x6A6A70, 0xF4F6FA ).p( PRAYER_FLAGS, 0.02f, 4, 30 )
        .p( FLAT_HOUSE, 0.02f, 6, 60 ).c( 0xE8D8C0, 0x6A7A9A ).p( SNOWBANK, 0.05f, 1, 6 ).p( YAK, 0.01f, 10, 60 );
    W( b, BE::PUNJAB, "PUNJAB", "INDIA", S::HAZE ).land( 0x8AA850, 0x809C48, 0x9AA060 ).fields( 0.85f, 0xF0D030, 0xC8B850, 0x8AB04A )
        .hills( 2, 0, 0 ).drive( 2, 0.9f, 0.96f, 10 ).bends( 0.15f, 0.2f, 0.05f )
        .p( BROADLEAF, 0.08f, 4, 100 ).c( 0x5A8A3A ).p( COW, 0.03f, 6, 60 ).v( 0 ).p( FLAT_HOUSE, 0.02f, 10, 80 ).c( 0xE8D8B8, 0xC8B898 )
        .p( WATER_TOWER, 0.004f, 20, 120 ).p( HAYBALE, 0.01f, 8, 40 ).p( DOME, 0.002f, 60, 200, 0.6f, 0.8f );
    W( b, BE::HAMPI, "HAMPI", "INDIA", S::GOLDEN ).land( 0x9AA058, 0x909450, 0xA89A6A ).hills( 20, 6, 6 ).drive( 2, 0.4f, 0.94f, 10 )
        .bends( 0.45f, 0.4f, 0.35f )
        .p( BOULDER, 0.15f, 6, 200, 1.5f, 3.0f ).p( KOPJE, 0.015f, 30, 250 ).p( TEMPLE_TOWER, 0.006f, 30, 150 ).c( 0xA8906A ).v( 1 ).p( COLUMNS, 0.01f, 10, 80 )
        .p( BANANA, 0.12f, 3, 60 ).p( PALM, 0.1f, 3, 80 );
    W( b, BE::MEGHALAYA, "MEGHALAYA", "INDIA", S::MIST ).rain( 0.6f ).land( 0x4E8A3A, 0x468034, 0x4A7A44 ).hills( 40, 20, 14 )
        .drive( 2, 0.3f, 0.88f, 10 ).bends( 0.7f, 0.6f, 0.7f ).bridges( 0.15f, BR::COVERED, 0x6A5A40 )
        .p( WATERFALL, 0.006f, 60, 200, 0.8f, 1.2f ).v( 1 ).p( JUNGLE_TREE, 0.3f, 6, 150 ).p( TREE_FERN, 0.12f, 2, 40 ).p( BAMBOO, 0.15f, 2, 40 )
        .p( FLAT_HOUSE, 0.02f, 6, 60 ).c( 0xE8D8C0, 0x3A6AAA ).pastel();
    W( b, BE::SUNDARBANS, "SUNDARBANS", "BANGLADESH", S::HAZE ).sea( 0.7f, 0.7f, -1.5f ).water( 0x6A6A50, 0.8f ).land( 0x6A9040, 0x60863A, 0x5E8048 )
        .hills( 0, 0, 0, 0.4f ).drive( 2, 0.25f, 0.9f, 10 ).bends( 0.3f, 0.3f, 0.0f ).posts( 0 )
        .p( MANGROVE, 0.4f, 2, 11 ).p( BOAT, 0.05f, 36, 150 ).c( 0x6A4A30, 0xE8C040 ).floating().p( HUT, 0.02f, 3, 11 ).p( PALM, 0.08f, 3, 11 )
        .p( WARNING_SIGN, 0.004f, 1, 1, 1, 1 ).v( 4 );
    W( b, BE::SIGIRIYA, "SIGIRIYA", "SRI LANKA", S::TROPICAL ).land( 0x5E9440, 0x568A3A, 0x5A8448 ).fields( 0.3f, 0x8AC050, 0x7AB048, 0x9AC060 )
        .hills( 10, 4, 4, 0.15f ).drive( 2, 0.4f, 0.93f, 10 ).bends( 0.4f, 0.35f, 0.25f )
        .p( MONOLITH, 0.0f, 180, 260, 1.4f, 1.4f ).c( 0x8A5A48 ).once().p( JUNGLE_TREE, 0.25f, 6, 150 ).p( STUPA, 0.004f, 30, 150 ).c( 0xF8F6F2, 0xF0EEE6 )
        .p( ELEPHANT, 0.01f, 15, 120 ).p( PALM, 0.1f, 3, 60 );

    // ---- Southeast Asia ----------------------------------------------------------------------------------------------
    W( b, BE::BANGKOK, "BANGKOK", "THAILAND", S::NIGHT ).land( 0x5A6A58, 0x526250, 0x4A5A4A ).drive( 4, 1.7f, 0.96f, 0 ).bends( 0.3f, 0.3f, 0.1f )
        .lamps( 30 ).posts( 0 ).gantries( 600 )
        .p( GLASS_TOWER, 0.18f, 15, 250 ).c( 0x4A6A8A ).p( SUPERTALL, 0.002f, 200, 400 ).p( WAT, 0.004f, 20, 120 ).p( BILLBOARD, 0.03f, 4, 12 )
        .p( PALM, 0.08f, 2, 12 ).p( NEON_PILLAR, 0.01f, 4, 20 );
    W( b, BE::HA_GIANG, "HA GIANG LOOP", "VIETNAM", S::MIST ).land( 0x6A9A48, 0x609040, 0x6A8A58 ).verge( 0x9A9690 ).ranges( 0x4A5A5A, 0x8A9A9A, 9 )
        .hills( 50, 40, -40 ).drive( 2, 0.25f, 0.92f, 20 ).bends( 0.95f, 0.9f, 0.9f ).posts( 0 )
        .p( KARST, 0.02f, 60, 400, 0.6f, 1.2f ).c( 0x9A9890, 0x4A7A44 ).p( TERRACES, 0.03f, 20, 150 ).c( 0x8AB04A, 0x8A6A4A ).p( HUT, 0.015f, 8, 60 )
        .p( ROCK, 0.12f, 3, 50 ).p( BANANA, 0.05f, 3, 40 );
    W( b, BE::KUALA_LUMPUR, "KUALA LUMPUR", "MALAYSIA", S::HAZE ).land( 0x5E8A48, 0x568040, 0x5A7A4E ).drive( 4, 1.5f, 0.96f, 1 ).bends( 0.35f, 0.35f, 0.2f )
        .lamps( 30 ).posts( 0 ).gantries( 700 )
        .landmark( Lm::TWIN_TOWERS, 120, 200 ).p( GLASS_TOWER, 0.15f, 20, 250 ).c( 0x5A7A9A ).p( TV_TOWER, 0.002f, 150, 350 ).v( 0 ).p( PALM, 0.15f, 2, 30 )
        .p( MOSQUE, 0.003f, 30, 150 ).c( 0xF0ECE0, 0x3A6AAA );
    W( b, BE::BRUNEI, "BANDAR SERI BEGAWAN", "BRUNEI", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x4A7A6A ).land( 0x4E8A3A, 0x468034, 0x4A7A44 )
        .drive( 2, 0.4f, 0.96f, 20 ).bends( 0.35f, 0.35f, 0.1f ).lamps( 35 )
        .p( MOSQUE, 0.004f, 20, 100, 1.2f, 1.6f ).c( 0xF4F2EA, 0xD8A840 ).p( WATER_VILLA, 0.05f, 36, 150 ).floating().p( JUNGLE_TREE, 0.2f, 6, 150 )
        .p( PALM, 0.1f, 2, 30 ).p( BOAT, 0.03f, 36, 120 ).c( 0x3A5A8A, 0xE8E8E8 ).floating();
    W( b, BE::BOROBUDUR, "BOROBUDUR", "INDONESIA", S::MIST ).land( 0x5E9A40, 0x56903A, 0x5E8A48 ).fields( 0.5f, 0x8AC050, 0x6AA040, 0xA8B860 )
        .hills( 16, 4, 4, 0.2f ).drive( 2, 0.4f, 0.94f, 20 ).bends( 0.45f, 0.4f, 0.35f )
        .landmark( Lm::BOROBUDUR, 80, 140 ).p( VOLCANO_CONE, 0.004f, 300, 600, 1.4f, 1.8f ).c( 0x4A4A50, 0x6A6A6A ).p( PALM, 0.15f, 3, 80 )
        .p( BANANA, 0.1f, 3, 40 ).p( FLAT_HOUSE, 0.02f, 6, 60 ).c( 0xE8D8C0, 0xB0583A );
    W( b, BE::PALAWAN, "PALAWAN", "PHILIPPINES", S::TROPICAL ).sea( 1, 1, -2 ).water( 0x2AC0C0 ).land( 0xF0E8D8, 0xE6DECE, 0x4E8A3A )
        .verge( 0xF0E8D8 ).hills( 10, 6, 6 ).drive( 2, 0.25f, 0.95f, 20 ).bends( 0.45f, 0.4f, 0.3f )
        .p( KARST, 0.02f, 40, 300, 0.6f, 1.2f ).c( 0x6A6A60, 0x3A7A3A ).floating().p( PALM, 0.3f, 2, 11 ).p( HUT, 0.02f, 3, 11 )
        .p( BOAT, 0.05f, 36, 150 ).c( 0x2A8AC8, 0xF0F0F0 ).pastel().floating();
    W( b, BE::DILI, "DILI", "EAST TIMOR", S::NOON ).sea( 0, 1, -4 ).water( 0x1E8AB0 ).land( 0xA0A060, 0x969658, 0x8A9060 ).hills( 30, 20, -4 )
        .drive( 2, 0.4f, 0.95f, 20 ).bends( 0.5f, 0.45f, 0.4f )
        .landmark( Lm::OPEN_ARMS, 120, 200, 1.0f ).p( PALM, 0.12f, 2, 40 ).p( FLAT_HOUSE, 0.03f, 6, 60 ).c( 0xE8D8C0, 0xB0583A ).pastel()
        .p( SHRUB, 0.15f, 2, 60 ).c( 0x7A8A48 ).p( BOAT, 0.02f, 40, 150 ).c( 0x2A6AAA, 0xF0F0F0 ).floating();

    // ---- East Asia ---------------------------------------------------------------------------------------------------
    W( b, BE::TAROKO, "TAROKO GORGE", "TAIWAN", S::MIST ).sea( 0, 1, -24 ).water( 0x5AB0B0 ).land( 0x4E8A3A, 0x468034, 0x4A7A44 ).verge( 0xD8D8D0 )
        .hills( 60, 70, -10 ).drive( 2, 0.3f, 0.95f, 9 ).bends( 0.75f, 0.7f, 0.5f ).tunnels( 0.45f ).bridges( 0.25f, BR::SUSPENSION, 0xC83A2A )
        .p( JUNGLE_TREE, 0.2f, 6, 150 ).p( PINE, 0.1f, 6, 100 ).p( BOULDER, 0.06f, 4, 60 ).c( 0xE0E0D8 ).p( PAGODA, 0.003f, 30, 150 );
    W( b, BE::TIANMEN, "TIANMEN MOUNTAIN", "CHINA", S::MIST ).fog( 0.0035f, 0xD8DEE2 ).land( 0x4E8A3A, 0x468034, 0x4A7A44 ).verge( 0x8A8A80 )
        .hills( 60, 50, -40 ).drive( 2, 0.3f, 0.95f, 9 ).bends( 1.0f, 1.0f, 1.0f ).rails()
        .p( KARST, 0.03f, 40, 300, 1.0f, 1.8f ).c( 0x8A8A80, 0x4A7A40 ).p( ARCH, 0.0f, 200, 300, 2.0f, 2.0f ).c( 0x7A7A70 ).once().p( PINE, 0.25f, 4, 150 )
        .p( TEMPLE_GATE, 0.003f, 10, 60 );
    W( b, BE::ZHANGYE, "RAINBOW MOUNTAINS", "CHINA", S::DESERT ).land( 0xD8945E, 0xCC8856, 0xD8A070 ).verge( 0xC87A50 ).hills( 24, 10, 10 )
        .drive( 2, 0.2f, 0.97f, 9 ).bends( 0.45f, 0.4f, 0.45f )
        .p( BADLAND, 0.05f, 15, 250, 0.8f, 1.6f ).c( 0xD87A50, 0xE8C080 ).p( BADLAND, 0.04f, 15, 250, 0.8f, 1.6f ).c( 0xC85A6A, 0xE8D0A0 )
        .p( BADLAND, 0.03f, 15, 250, 0.8f, 1.6f ).c( 0xE8B060, 0x9A6AA0 ).p( SHRUB, 0.05f, 2, 40 ).c( 0x8A8A50 );
    W( b, BE::GUOLIANG, "GUOLIANG TUNNEL", "CHINA", S::MORNING ).land( 0x6A9448, 0x608A40, 0x6A8458 ).verge( 0xA89A88 ).hills( 60, 70, 60 )
        .drive( 2, 0.25f, 0.95f, 9 ).bends( 0.65f, 0.6f, 0.7f ).tunnels( 0.6f ).posts( 0 )
        .p( COTTAGE, 0.02f, 8, 60 ).c( 0xA89A88, 0x6A6A6A ).p( PINE, 0.2f, 4, 120 ).p( ROCK, 0.12f, 3, 50 ).p( LANTERN, 0.01f, 3, 8 );
    W( b, BE::HUANGSHAN, "HUANGSHAN", "CHINA", S::MIST ).fog( 0.004f, 0xE0E4E6 ).land( 0x4E8A3A, 0x468034, 0x4A7A44 ).verge( 0x9A9890 )
        .hills( 50, 30, 30 ).drive( 2, 0.3f, 0.95f, 9 ).bends( 0.75f, 0.7f, 0.75f )
        .p( KARST, 0.04f, 30, 250, 0.8f, 1.6f ).c( 0x9A9890, 0x3A6A3A ).p( PINE, 0.3f, 4, 120 ).p( HUIZHOU_HOUSE, 0.015f, 10, 80 ).p( TEA_ROW, 0.2f, 6, 30, 1, 1 );
    W( b, BE::BEIJING, "BEIJING", "CHINA", S::HAZE ).fog( 0.0024f ).land( 0x6A7A5A, 0x627252, 0x6A6A5E ).drive( 4, 1.7f, 0.97f, 1 ).bends( 0.25f, 0.3f, 0.05f )
        .lamps( 30 ).posts( 0 ).gantries( 700 )
        .p( PANEL_BLOCK, 0.12f, 15, 150 ).c( 0xD8C8B0 ).p( GLASS_TOWER, 0.08f, 30, 250 ).c( 0x6A8AA0 ).p( TEMPLE_GATE, 0.004f, 20, 100 )
        .p( PAGODA, 0.003f, 30, 150 ).p( LANTERN, 0.02f, 3, 8 ).p( BROADLEAF, 0.12f, 2, 12 ).c( 0x5A8A40 );
    W( b, BE::HARBIN, "HARBIN", "CHINA", S::NIGHT ).snow( 0.25f ).land( 0xD8DEE8, 0xCCD4DE, 0xC0C8D4 ).verge( 0xC8D0DA ).drive( 3, 0.9f, 0.84f, 9 )
        .bends( 0.3f, 0.3f, 0.05f ).posts( 0 )
        .landmark( Lm::ICE_CASTLE, 40, 70 ).p( ONION_CHURCH, 0.004f, 30, 120 ).c( 0xA85A40, 0x3A6A4A ).p( PANEL_BLOCK, 0.08f, 15, 150 ).c( 0xC8C0B0 )
        .p( SNOWBANK, 0.25f, 0.5f, 3, 0.8f, 1.4f ).p( LANTERN, 0.03f, 3, 8 );
    W( b, BE::KUMGANG, "KUMGANG MOUNTAINS", "NORTH KOREA", S::MORNING ).land( 0x5E9446, 0x568A40, 0x5E8450 ).verge( 0xC8C4B8 ).hills( 50, 30, 20 )
        .drive( 3, 0.03f, 0.97f, 9 ).bends( 0.5f, 0.45f, 0.5f ).posts( 0 )
        .p( KARST, 0.03f, 40, 300, 0.8f, 1.4f ).c( 0xC8C4B8, 0x3A6A3A ).p( PINE, 0.3f, 4, 150 ).p( TEMPLE_GATE, 0.002f, 20, 80 ).p( BILLBOARD, 0.003f, 5, 12 )
        .p( WATERFALL, 0.003f, 60, 200, 0.5f, 0.8f ).v( 1 );
    W( b, BE::SEOUL, "SEOUL", "SOUTH KOREA", S::DUSK ).sea( 0, 0.4f, -6 ).water( 0x3A5A7A ).land( 0x5E7A58, 0x567250, 0x5A6A5A ).ranges( 0x3A4A50, 0x6A7A88, 7 )
        .drive( 4, 1.5f, 0.97f, 8 ).bends( 0.3f, 0.3f, 0.15f ).lamps( 30 ).posts( 0 ).bridges( 0.2f, BR::BEAM )
        .p( PANEL_BLOCK, 0.12f, 15, 200 ).c( 0xE8E4DC ).p( GLASS_TOWER, 0.1f, 20, 250 ).c( 0x5A7A9A ).p( TV_TOWER, 0.002f, 150, 350 ).v( 0 )
        .p( HANOK, 0.01f, 8, 40 ).p( NEON_PILLAR, 0.01f, 4, 20 );
    W( b, BE::HAKONE, "HAKONE TURNPIKE", "JAPAN", S::MORNING ).land( 0x5E9446, 0x568A40, 0x5E8450 ).hills( 40, 30, -20 ).drive( 2, 0.5f, 0.98f, 8 )
        .bends( 0.85f, 0.75f, 0.85f ).rails()
        .p( VOLCANO_CONE, 0.0f, 450, 550, 2.6f, 2.6f ).c( 0x4A5A7A, 0xF4F6FA ).once().p( CEDAR, 0.35f, 4, 150 ).p( TORII, 0.003f, 6, 20 ).p( CHERRY, 0.04f, 4, 60 )
        .p( LANTERN, 0.01f, 3, 8 );
    W( b, BE::IROHAZAKA, "IROHAZAKA", "JAPAN", S::GOLDEN ).land( 0x7A8A48, 0x708040, 0x8A7A50 ).hills( 50, 40, -30 ).drive( 2, 0.4f, 0.98f, 8 )
        .bends( 1.0f, 1.0f, 0.95f ).rails()
        .p( AUTUMN_TREE, 0.4f, 3, 150 ).p( SPRUCE, 0.1f, 6, 150 ).p( WATERFALL, 0.003f, 80, 200, 0.6f, 0.9f ).v( 1 ).p( TORII, 0.003f, 6, 20 )
        .p( PAGODA, 0.002f, 40, 150 );
    W( b, BE::MIYAJIMA, "MIYAJIMA", "JAPAN", S::MORNING ).sea( 0, 1, -2 ).water( 0x3A7A8A ).land( 0x5E9446, 0x568A40, 0x5E8450 ).hills( 30, 20, 0 )
        .drive( 2, 0.4f, 0.98f, 8 ).bends( 0.45f, 0.4f, 0.3f ).lamps( 40 )
        .landmark( Lm::SEA_TORII, 60, 100 ).floating().p( LANTERN, 0.04f, 2, 6 ).p( PAGODA, 0.003f, 30, 120 ).p( CHERRY, 0.08f, 3, 60 )
        .p( PINE, 0.1f, 4, 80 ).p( WARNING_SIGN, 0.004f, 1, 1, 1, 1 ).v( 4 );
    W( b, BE::OKINAWA, "OKINAWA", "JAPAN", S::TROPICAL ).sea( 1, 1, -2 ).water( 0x30C8D0 ).land( 0xE8E0C8, 0xDCD4BC, 0x6A9A4A ).verge( 0xE8E0C8 )
        .hills( 6, 0, 0 ).drive( 2, 0.4f, 0.98f, 8 ).bends( 0.3f, 0.3f, 0.1f ).bridges( 0.5f, BR::CAUSEWAY, 0xE0E0DA )
        .p( COTTAGE, 0.04f, 3, 11 ).c( 0xE8E4D8, 0xC85A3A ).p( PALM, 0.25f, 2, 11 ).p( BANANA, 0.05f, 2, 11 ).p( SAILBOAT, 0.01f, 60, 250 ).floating();

    // ---- Micronesia, Melanesia, Australia, New Zealand, Polynesia ----------------------------------------------------
    W( b, BE::ROCK_ISLANDS, "ROCK ISLANDS", "PALAU", S::TROPICAL ).sea( 1, 1, -2 ).water( 0x30C8C0 ).land( 0x4E8A3A, 0x468034, 0x4A7A44 )
        .verge( 0xE8E0C8 ).hills( 8, 4, 4 ).drive( 2, 0.15f, 0.95f, 16 ).bends( 0.4f, 0.35f, 0.2f ).bridges( 0.3f, BR::BEAM )
        .p( KARST, 0.04f, 36, 250, 0.4f, 0.8f ).c( 0x8A8A80, 0x3A7A3A ).floating().p( PALM, 0.2f, 2, 11 ).p( JUNGLE_TREE, 0.1f, 3, 11 )
        .p( BOAT, 0.03f, 36, 150 ).c( 0xF0F0F0, 0x2A6AAA ).floating();
    W( b, BE::NAN_MADOL, "NAN MADOL", "MICRONESIA", S::MIST ).rain( 0.25f ).sea( 0.7f, 0.7f, -1.5f ).water( 0x3A7A7A ).land( 0x3E7A32, 0x38702C, 0x3E6A3A )
        .hills( 10, 4, 4 ).drive( 2, 0.1f, 0.9f, 16 ).bends( 0.4f, 0.35f, 0.2f )
        .p( BASALT_COLUMNS, 0.04f, 3, 11, 0.6f, 0.9f ).v( 1 ).p( RUIN, 0.02f, 3, 11 ).c( 0x3A3A3E ).p( MANGROVE, 0.2f, 2, 11 ).p( JUNGLE_TREE, 0.15f, 3, 11 )
        .p( PALM, 0.1f, 2, 11 );
    W( b, BE::MAJURO, "MAJURO", "MARSHALL ISLANDS", S::TROPICAL ).sea( 1, 1, -1 ).water( 0x30C0D0 ).land( 0xE8E0C8, 0xDCD4BC, 0x6A9A4A ).verge( 0xF0E8D8 )
        .hills( 0, 0, 0 ).drive( 2, 0.3f, 0.96f, 16 ).bends( 0.25f, 0.25f, 0.0f )
        .p( PALM, 0.35f, 2, 11 ).p( FLAT_HOUSE, 0.04f, 3, 11 ).c( 0xE8C890, 0x8A8A8A ).pastel().p( BOAT, 0.03f, 36, 150 ).c( 0x2A8AC8, 0xF0F0F0 ).pastel().floating();
    W( b, BE::TARAWA, "TARAWA", "KIRIBATI", S::TROPICAL ).sea( 1, 1, -1 ).water( 0x2AC8C8 ).land( 0xE8E0C8, 0xDCD4BC, 0x6A9A4A ).verge( 0xF0E8D8 )
        .hills( 0, 0, 0 ).drive( 2, 0.25f, 0.95f, 16 ).bends( 0.25f, 0.25f, 0.0f ).bridges( 0.4f, BR::CAUSEWAY, 0xD8D4C8 )
        .p( PALM, 0.4f, 2, 11 ).p( HUT, 0.04f, 3, 11 ).p( BOAT, 0.03f, 36, 150 ).c( 0x6A4A30, 0xE8E0D0 ).floating();
    W( b, BE::NAURU, "NAURU", "NAURU", S::NOON ).sea( 0, 1, -3 ).water( 0x1E8AC0 ).land( 0xB8B098, 0xACA48E, 0x8A8A70 ).verge( 0xD8D0C0 ).hills( 14, 8, -2 )
        .drive( 2, 0.3f, 0.96f, 16 ).bends( 0.35f, 0.35f, 0.2f )
        .p( PILLAR, 0.08f, 12, 150, 0.15f, 0.3f ).p( PALM, 0.15f, 2, 40 ).p( FLAT_HOUSE, 0.03f, 6, 60 ).c( 0xE8C890, 0x8A8A8A ).pastel().p( SHRUB, 0.1f, 2, 40 ).c( 0x6A8A48 );
    W( b, BE::PNG_HIGHLANDS, "HIGHLANDS", "PAPUA NEW GUINEA", S::MIST ).rain( 0.3f ).land( 0x4E8A3A, 0x468034, 0x4A7A44 ).verge( 0x8A5A3A )
        .road( 0x8A6A50, 0x8A6A50 ).hills( 46, 24, 20 ).drive( 2, 0.25f, 0.86f, 16 ).bends( 0.7f, 0.6f, 0.75f ).posts( 0 )
        .p( HUT, 0.03f, 6, 60 ).p( TREE_FERN, 0.1f, 2, 40 ).p( JUNGLE_TREE, 0.2f, 6, 150 ).p( BROADLEAF, 0.1f, 4, 80 ).c( 0x2E5E2A ).p( BANANA, 0.08f, 3, 40 );
    W( b, BE::MAROVO, "MAROVO LAGOON", "SOLOMON ISLANDS", S::TROPICAL ).sea( 1, 1, -2 ).water( 0x2AA8B0 ).land( 0x3E7A32, 0x38702C, 0x3E6A3A )
        .hills( 20, 10, 10 ).drive( 2, 0.1f, 0.94f, 16 ).bends( 0.45f, 0.4f, 0.3f )
        .p( WATER_VILLA, 0.04f, 36, 150 ).floating().p( BOAT, 0.04f, 36, 150 ).c( 0x6A4A30, 0xE8E0D0 ).floating().p( PALM, 0.25f, 2, 11 ).p( JUNGLE_TREE, 0.15f, 3, 11 );
    W( b, BE::YASUR, "MOUNT YASUR", "VANUATU", S::DUSK ).land( 0x3A3634, 0x34302E, 0x2E2A28 ).verge( 0x2A2624 ).road( 0x4A4642, 0x4A4642 )
        .hills( 20, 6, 6 ).drive( 2, 0.05f, 0.86f, 16 ).bends( 0.45f, 0.4f, 0.45f ).lamps( 0 ).posts( 0 )
        .p( VOLCANO_CONE, 0.0f, 250, 350, 1.4f, 1.4f ).c( 0x2E2A28, 0x3A3634 ).once().p( LAVA_ROCK, 0.25f, 3, 80 ).p( PALM, 0.06f, 4, 60 )
        .p( BROADLEAF, 0.05f, 6, 80, 1.2f, 1.6f ).c( 0x2E5E2A ).p( HUT, 0.006f, 8, 40 );
    W( b, BE::SYDNEY, "SYDNEY", "AUSTRALIA", S::MORNING ).sea( 0, 1, -4 ).water( 0x1E6A9A ).land( 0x6A8A50, 0x628048, 0x6A7A5A ).drive( 3, 1.3f, 0.98f, 21 )
        .bends( 0.4f, 0.4f, 0.3f ).lamps( 30 ).posts( 0 ).bridges( 0.4f, BR::ARCH, 0x6A7480 )
        .landmark( Lm::OPERA_HOUSE, 60, 100 ).floating().p( GLASS_TOWER, 0.08f, 20, 200 ).c( 0x6A8AA8 ).p( ROW_HOUSES, 0.06f, 4, 30 ).c( 0xE8D8B8, 0x6A3A2A ).pastel()
        .p( EUCALYPTUS, 0.08f, 3, 60 ).p( SAILBOAT, 0.03f, 40, 200 ).floating().p( TV_TOWER, 0.002f, 150, 300 ).v( 1 );
    W( b, BE::SURFERS_PARADISE, "GOLD COAST", "AUSTRALIA", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x2A9AC0 ).land( 0x6A9A50, 0x629048, 0x6A8A5A ).verge( 0xF0E4C8 )
        .drive( 3, 1.0f, 0.98f, 21 ).bends( 0.2f, 0.25f, 0.05f ).lamps( 35 ).posts( 0 )
        .p( GLASS_TOWER, 0.15f, 6, 60 ).c( 0xC8D8E0 ).p( SUPERTALL, 0.002f, 40, 120 ).p( PALM, 0.25f, 2, 11 ).p( PARASOL, 0.06f, 3, 11 ).pastel();
    W( b, BE::KIMBERLEY, "GIBB RIVER ROAD", "AUSTRALIA", S::DESERT ).land( 0xB8703E, 0xAC6838, 0xA8704A ).verge( 0xB86A3E ).road( 0xA85A36, 0xA85A36 )
        .hills( 16, 4, 4 ).drive( 2, 0.05f, 0.84f, 21 ).bends( 0.3f, 0.3f, 0.2f ).posts( 0 )
        .p( BAOBAB, 0.02f, 6, 150, 0.6f, 0.9f ).p( TERMITE_MOUND, 0.04f, 3, 80 ).p( BUTTE, 0.005f, 100, 300 ).p( KANGAROO, 0.02f, 8, 80 )
        .p( GRASS_TUFT, 0.2f, 1, 40 ).p( WARNING_SIGN, 0.004f, 1, 1, 1, 1 ).v( 3 );
    W( b, BE::NULLARBOR, "NULLARBOR PLAIN", "AUSTRALIA", S::DESERT ).sea( 0, 1, -60 ).water( 0x1E5A8A ).land( 0xB8A888, 0xAC9C7E, 0xA89A80 ).verge( 0xC8B890 )
        .hills( 2, 0, 0 ).drive( 2, 0.2f, 0.98f, 21 ).bends( 0.02f, 0.1f, 0.02f ).posts( 80 )
        .p( SHRUB, 0.25f, 2, 150, 0.3f, 0.5f ).c( 0x8A9A80 ).p( KANGAROO, 0.01f, 8, 80 ).p( WARNING_SIGN, 0.003f, 1, 1, 1, 1 ).v( 0 )
        .p( WARNING_SIGN, 0.003f, 1, 1, 1, 1 ).v( 1 ).p( WARNING_SIGN, 0.003f, 1, 1, 1, 1 ).v( 3 ).p( GAS_STATION, 0.001f, 8, 14 );
    W( b, BE::TASMANIA, "CRADLE MOUNTAIN", "AUSTRALIA", S::OVERCAST ).clouds( 0.6f ).land( 0x8A9A5A, 0x7E8E52, 0x6E8058 ).ranges( 0x3E4650, 0x6A7480, 9 )
        .hills( 30, 10, 10, 0.25f ).water( 0x3A4A5A ).drive( 2, 0.2f, 0.93f, 21 ).bends( 0.6f, 0.5f, 0.55f )
        .p( PEAK, 0.0f, 250, 350, 1.2f, 1.2f ).c( 0x5A5450, 0x6A6460 ).once().p( SHRUB, 0.3f, 2, 80 ).c( 0xA8A060 ).p( EUCALYPTUS, 0.15f, 4, 150 )
        .p( TREE_FERN, 0.06f, 2, 30 ).p( KANGAROO, 0.01f, 6, 60, 0.5f, 0.6f );
    W( b, BE::CROWN_RANGE, "CROWN RANGE", "NEW ZEALAND", S::ALPINE ).land( 0xB8A870, 0xAC9C68, 0x9A9A6A ).ranges( 0x4A5260, 0xD8DEE8, 12 )
        .hills( 50, 40, -30 ).drive( 2, 0.3f, 0.97f, 6 ).bends( 0.9f, 0.85f, 0.95f ).rails()
        .p( SHRUB, 0.4f, 2, 100, 0.5f, 0.8f ).c( 0xC8B070 ).p( LUPIN, 0.05f, 2, 30 ).p( SHEEP, 0.05f, 8, 100 ).p( PEAK, 0.005f, 220, 450, 1.0f, 1.4f ).c( 0x5A5E66, 0xF0F2F6 )
        .p( ROCK, 0.08f, 3, 50 );
    W( b, BE::CORAL_COAST, "CORAL COAST", "FIJI", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x2AB8C8 ).land( 0x5E9A44, 0x56903E, 0x5E8A4A ).verge( 0xF0E8D0 )
        .fields( 0.3f, 0x8AC050, 0x7AB048, 0x9AC060 ).hills( 16, 8, 0 ).drive( 2, 0.3f, 0.96f, 16 ).bends( 0.45f, 0.4f, 0.3f )
        .p( PALM, 0.3f, 2, 60 ).p( HUT, 0.03f, 4, 40 ).p( BOAT, 0.03f, 36, 150 ).c( 0xF0F0F0, 0x2A6AAA ).floating().p( JUNGLE_TREE, 0.08f, 10, 150 );
    W( b, BE::FUNAFUTI, "FUNAFUTI", "TUVALU", S::TROPICAL ).sea( 1, 1, -1 ).water( 0x2AC8D0 ).land( 0xE0D8C0, 0xD4CCB4, 0x6A9A4A ).verge( 0xF0E8D8 )
        .hills( 0, 0, 0 ).drive( 2, 0.15f, 0.96f, 16 ).bends( 0.1f, 0.15f, 0.0f )
        .p( PALM, 0.35f, 2, 11 ).p( FLAT_HOUSE, 0.03f, 3, 11 ).c( 0xE8C890, 0x8A8A8A ).pastel().p( BOAT, 0.02f, 36, 150 ).c( 0x6A4A30, 0xE8E0D0 ).floating();
    W( b, BE::LALOMANU, "LALOMANU", "SAMOA", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x2AC0C8 ).land( 0x5E9A44, 0x56903E, 0x5E8A4A ).verge( 0xF4ECDC )
        .hills( 24, 12, 0 ).drive( 2, 0.25f, 0.96f, 16 ).bends( 0.5f, 0.45f, 0.4f )
        .p( PALM, 0.3f, 2, 60 ).p( HUT, 0.04f, 3, 30 ).p( CHURCH, 0.006f, 10, 80 ).c( 0xF4F2EA, 0xC83A3A ).pastel().p( JUNGLE_TREE, 0.1f, 10, 150 )
        .p( BOAT, 0.02f, 36, 120 ).c( 0x2A8AC8, 0xF0F0F0 ).floating();
    W( b, BE::TONGA_BLOWHOLES, "MAPU A VAEA", "TONGA", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x1E8AB0 ).land( 0x5E9A44, 0x56903E, 0x5E8A4A ).verge( 0x6A6A60 )
        .hills( 6, 2, 0 ).drive( 2, 0.25f, 0.96f, 16 ).bends( 0.35f, 0.3f, 0.15f )
        .p( GEYSER, 0.03f, 4, 11, 0.5f, 0.8f ).v( 0 ).p( PALM, 0.3f, 2, 60 ).p( CHURCH, 0.004f, 10, 80 ).c( 0xF4F2EA, 0xC83A3A ).p( FLAT_HOUSE, 0.02f, 6, 60 ).c( 0xE8C890, 0x8A8A8A ).pastel();

    // ---- South America -----------------------------------------------------------------------------------------------
    W( b, BE::TORRES_DEL_PAINE, "TORRES DEL PAINE", "CHILE", S::ALPINE ).clouds( 0.5f ).sea( 0, 1, -6 ).water( 0x40A8B8 ).land( 0xA8A06A, 0x9C9462, 0x8A9060 )
        .ranges( 0x4A505A, 0xE8ECF2, 14 ).hills( 30, 6, -4 ).drive( 2, 0.12f, 0.88f, 15 ).bends( 0.45f, 0.4f, 0.4f ).road( 0x9A9080, 0x9A9080 ).posts( 0 )
        .p( PEAK, 0.0f, 300, 420, 2.2f, 2.2f ).c( 0x8A8680, 0xF0F0F2 ).once().p( PEAK, 0.004f, 250, 500, 1.0f, 1.4f ).c( 0x5A5450, 0xF0F2F6 )
        .p( LLAMA, 0.03f, 8, 100 ).p( SHRUB, 0.3f, 2, 80, 0.4f, 0.7f ).c( 0xA89A58 );
    W( b, BE::PERITO_MORENO, "PERITO MORENO", "ARGENTINA", S::ALPINE ).sea( 0, 1, -4 ).water( 0x7AA8C8 ).land( 0x6A8A48, 0x608040, 0x5E7A4A )
        .ranges( 0xB8C8D8, 0xF0F4FA, 10 ).hills( 30, 20, -2 ).drive( 2, 0.2f, 0.95f, 14 ).bends( 0.5f, 0.45f, 0.45f )
        .p( ICEBERG, 0.02f, 40, 250, 0.8f, 1.4f ).floating().p( ICE_SPIKE, 0.01f, 150, 300, 1.5f, 2.5f ).floating().p( BROADLEAF, 0.25f, 4, 150 ).c( 0x4A7A3A )
        .p( PEAK, 0.004f, 250, 500, 1.2f, 1.5f ).c( 0x6A6A70, 0xF4F6FA );
    W( b, BE::RUTA_40, "RUTA 40", "ARGENTINA", S::DESERT ).clouds( 0.25f ).land( 0xB8A878, 0xAC9C70, 0xA89A74 ).verge( 0xB8A888 ).ranges( 0x6A6A78, 0xD8DCE6, 7 )
        .hills( 8, 2, 2 ).drive( 2, 0.08f, 0.94f, 14 ).bends( 0.12f, 0.2f, 0.15f ).posts( 60 )
        .p( SHRUB, 0.35f, 2, 150, 0.3f, 0.6f ).c( 0xA89A58 ).p( LLAMA, 0.02f, 8, 100 ).p( FENCE, 0.3f, 8, 8, 1, 1 ).p( COTTAGE, 0.002f, 30, 150 ).c( 0xE8E0D0, 0xA83A2A )
        .p( WIND_PUMP, 0.003f, 20, 120 );
    W( b, BE::VALPARAISO, "VALPARAISO", "CHILE", S::MORNING ).sea( 0, 1, -6 ).water( 0x2A5A7A ).land( 0x7A8A58, 0x708050, 0x7A7A62 ).hills( 40, 30, -4 )
        .drive( 2, 1.0f, 0.97f, 15 ).bends( 0.65f, 0.65f, 0.75f ).lamps( 35 ).posts( 0 )
        .p( ROW_HOUSES, 0.2f, 4, 80 ).c( 0xE8A870, 0x8A8A8A ).pastel().p( CONTAINERS, 0.01f, 8, 11 ).p( CRANE, 0.004f, 8, 11 ).p( SHIP, 0.004f, 80, 250 ).floating()
        .p( PALM, 0.04f, 2, 20 );
    W( b, BE::CARACOLES, "PASO LOS CARACOLES", "CHILE", S::ALPINE ).land( 0x9A8A70, 0x8E7E66, 0x8A8070 ).verge( 0x8A8070 ).ranges( 0x5A5A64, 0xF0F2F6, 15 )
        .hills( 60, 50, -50 ).drive( 2, 0.5f, 0.95f, 15 ).bends( 1.0f, 1.0f, 1.0f ).tunnels( 0.2f ).rails()
        .p( PEAK, 0.008f, 150, 400, 1.3f, 1.8f ).c( 0x6A625A, 0xF4F6FA ).p( ROCK, 0.25f, 3, 60 ).p( SNOWBANK, 0.12f, 1, 8 ).p( BOULDER, 0.04f, 6, 80 );
    W( b, BE::BUENOS_AIRES, "BUENOS AIRES", "ARGENTINA", S::GOLDEN ).land( 0x6E8A58, 0x668250, 0x707A68 ).drive( 4, 1.5f, 0.98f, 14 ).bends( 0.2f, 0.25f, 0.0f )
        .lamps( 30 ).posts( 0 )
        .landmark( Lm::OBELISK, 30, 50, 0.4f ).p( ROW_HOUSES, 0.18f, 4, 30 ).c( 0xE0D8C8, 0x5A6470 ).p( BROADLEAF, 0.15f, 1.5f, 10 ).c( 0x8A6AC8 )
        .p( GLASS_TOWER, 0.04f, 30, 200 ).c( 0x7A9AB0 ).p( PANEL_BLOCK, 0.03f, 20, 120 ).c( 0xD8D0C0 );
    W( b, BE::PUNTA_DEL_ESTE, "PUNTA DEL ESTE", "URUGUAY", S::NOON ).sea( 0, 1, -2 ).water( 0x2A6A9A ).land( 0x7A9A58, 0x709050, 0x7A8A62 ).verge( 0xF0E8D0 )
        .drive( 2, 0.6f, 0.98f, 14 ).bends( 0.3f, 0.3f, 0.1f ).lamps( 40 )
        .landmark( Lm::HAND, 36, 50 ).floating().p( GLASS_TOWER, 0.06f, 15, 120 ).c( 0xE8E8E0 ).p( PINE, 0.15f, 4, 80 ).p( PARASOL, 0.04f, 3, 11 ).pastel()
        .p( SAILBOAT, 0.01f, 60, 250 ).floating();
    W( b, BE::CHACO, "GRAN CHACO", "PARAGUAY", S::HAZE ).land( 0x8A9A50, 0x809048, 0x8A9058 ).verge( 0xA86A48 ).road( 0x9A5A3A, 0x9A5A3A )
        .hills( 2, 0, 0, 0.1f ).drive( 2, 0.08f, 0.86f, 14 ).bends( 0.15f, 0.2f, 0.0f ).posts( 0 )
        .p( SHRUB, 0.35f, 2, 80 ).c( 0x6A7A40 ).p( PALM, 0.1f, 6, 150 ).p( BAOBAB, 0.01f, 8, 120, 0.4f, 0.6f ).p( COW, 0.03f, 8, 100 ).p( RANCH_FENCE, 0.3f, 8, 8, 1, 1 );
    W( b, BE::YUNGAS, "YUNGAS ROAD", "BOLIVIA", S::MIST ).fog( 0.004f, 0xD8DEE0 ).land( 0x3E7A32, 0x38702C, 0x3E6A3A ).verge( 0x6A5A48 )
        .road( 0x8A7A66, 0x8A7A66 ).hills( 60, 70, -120 ).drive( 2, 0.12f, 0.8f, 15 ).bends( 0.95f, 0.85f, 0.8f ).posts( 0 )
        .p( WATERFALL, 0.008f, 6, 12, 0.15f, 0.25f ).v( 1 ).p( JUNGLE_TREE, 0.3f, 6, 150 ).p( TREE_FERN, 0.15f, 2, 40 ).p( WAYSIDE_CROSS, 0.006f, 1, 3 ).v( 0 );
    W( b, BE::RAINBOW_MOUNTAIN, "VINICUNCA", "PERU", S::ALPINE ).land( 0x9A8A60, 0x8E7E58, 0xA88A6A ).ranges( 0x6A6A70, 0xF4F6FA, 12 ).hills( 40, 20, 20 )
        .drive( 2, 0.08f, 0.86f, 15 ).bends( 0.65f, 0.55f, 0.75f ).road( 0x8A7A66, 0x8A7A66 ).posts( 0 )
        .p( BADLAND, 0.05f, 20, 250, 0.8f, 1.5f ).c( 0xC8506A, 0xE8C070 ).p( BADLAND, 0.03f, 20, 250, 0.8f, 1.5f ).c( 0x8AA0A0, 0xD8805A )
        .p( LLAMA, 0.04f, 8, 100 ).p( STONE_WALL, 0.15f, 4, 4, 1, 1 ).p( PEAK, 0.003f, 250, 500, 1.4f, 1.8f ).c( 0x6A6A70, 0xF4F6FA );
    W( b, BE::NAZCA, "NAZCA DESERT", "PERU", S::DESERT ).land( 0xC89A70, 0xBC8E68, 0xB89070 ).verge( 0xB88A60 ).hills( 6, 2, 2 ).drive( 2, 0.3f, 0.98f, 15 )
        .bends( 0.1f, 0.15f, 0.05f )
        .p( WATCHTOWER, 0.0f, 6, 12, 0.8f, 0.8f ).c( 0xA8A8A8 ).v( 0 ).once().p( DUNE, 0.01f, 100, 400, 1.0f, 2.0f ).c( 0xD8A070 ).p( ROCK, 0.06f, 3, 60 )
        .p( SHRUB, 0.02f, 2, 40 ).c( 0x7A8A48 );
    W( b, BE::GALAPAGOS, "GALAPAGOS", "ECUADOR", S::NOON ).sea( 0, 1, -3 ).water( 0x1E7AA0 ).land( 0x6A6A50, 0x60604A, 0x4A4A44 ).verge( 0x3A3634 )
        .hills( 14, 6, 0 ).drive( 2, 0.1f, 0.95f, 15 ).bends( 0.35f, 0.3f, 0.2f )
        .p( TORTOISE, 0.03f, 4, 40, 0.9f, 1.3f ).p( PRICKLY_PEAR, 0.1f, 3, 60, 1.2f, 1.8f ).p( LAVA_ROCK, 0.15f, 3, 60 ).p( SHRUB, 0.15f, 2, 60 ).c( 0x7A8A48 )
        .p( FLAMINGO, 0.006f, 8, 11 );
    W( b, BE::CARTAGENA, "CARTAGENA", "COLOMBIA", S::TROPICAL ).sea( 0, 1, -3 ).water( 0x1E8AA0 ).land( 0x7A9A58, 0x709050, 0x7A8A62 ).drive( 2, 0.8f, 0.97f, 16 )
        .bends( 0.35f, 0.35f, 0.1f ).lamps( 35 ).posts( 0 )
        .p( FORT, 0.008f, 8, 60 ).c( 0xD8C8A8, 0xC8B898 ).v( 0 ).p( ROW_HOUSES, 0.18f, 3, 30 ).c( 0xE8C040, 0xB0583A ).pastel().p( PALM, 0.15f, 2, 20 )
        .p( GLASS_TOWER, 0.02f, 60, 200 ).c( 0xE0E8EE ).p( FLOWER_BUSH, 0.06f, 2, 15 ).c( 0xC8307A );
    W( b, BE::KAIETEUR, "KAIETEUR FALLS", "GUYANA", S::MIST ).land( 0x3E7A32, 0x38702C, 0x3E6A3A ).verge( 0x9A5A3A ).road( 0x9A5A3A, 0x9A5A3A )
        .hills( 30, 10, 10 ).drive( 2, 0.04f, 0.86f, 16 ).bends( 0.5f, 0.45f, 0.45f ).posts( 0 )
        .p( WATERFALL, 0.0f, 150, 250, 1.8f, 1.8f ).v( 1 ).once().p( TEPUI, 0.004f, 250, 500 ).c( 0x6A5A4A, 0x3E7A32 ).p( JUNGLE_TREE, 0.45f, 3, 150 )
        .p( FERN, 0.2f, 1, 20 );
    W( b, BE::SURINAME_RIVER, "SURINAME RIVER", "SURINAME", S::HAZE ).sea( 0, 1, -3 ).water( 0x6A5A40, 0.8f ).land( 0x3E7A32, 0x38702C, 0x3E6A3A )
        .hills( 8, 4, 0 ).drive( 2, 0.06f, 0.88f, 16 ).bends( 0.4f, 0.35f, 0.15f ).road( 0x9A6A4A, 0x9A6A4A ).posts( 0 )
        .p( JUNGLE_TREE, 0.45f, 3, 150 ).p( HUT, 0.02f, 4, 11 ).p( BOAT, 0.04f, 36, 150 ).c( 0x6A4A30, 0xD8C040 ).pastel().floating().p( PALM, 0.08f, 3, 11 );
    W( b, BE::RIO_DO_RASTRO, "SERRA DO RIO DO RASTRO", "BRAZIL", S::MIST ).fog( 0.003f ).land( 0x5E9A44, 0x56903E, 0x5E8A4A ).verge( 0x8A8278 )
        .hills( 60, 60, -60 ).drive( 2, 0.3f, 0.95f, 13 ).bends( 1.0f, 1.0f, 1.0f ).rails().lamps( 40 )
        .p( UMBRELLA_PINE, 0.15f, 4, 150, 1.0f, 1.4f ).p( COW, 0.03f, 8, 80 ).p( ROCK, 0.1f, 3, 50 ).p( WATERFALL, 0.003f, 30, 150, 0.4f, 0.6f ).v( 1 );
    W( b, BE::SAO_PAULO, "SAO PAULO", "BRAZIL", S::HAZE ).rain( 0.15f ).land( 0x5E7A50, 0x567248, 0x5A6A54 ).drive( 4, 1.8f, 0.95f, 13 ).bends( 0.3f, 0.3f, 0.15f )
        .lamps( 30 ).posts( 0 ).gantries( 600 )
        .p( GLASS_TOWER, 0.15f, 15, 250 ).c( 0x7A8A9A ).p( PANEL_BLOCK, 0.15f, 15, 200 ).c( 0xE0D8C8 ).p( FAVELA, 0.02f, 40, 150 ).p( BILLBOARD, 0.02f, 4, 12 )
        .p( BROADLEAF, 0.06f, 2, 12 ).c( 0x4A7A3A );
    W( b, BE::TRANSAMAZONICA, "TRANSAMAZONICA", "BRAZIL", S::HAZE ).land( 0x6A9A48, 0x609040, 0x3E7A32 ).verge( 0xA85A36 ).road( 0xA85A36, 0xA85A36 )
        .hills( 8, 2, 2 ).drive( 2, 0.12f, 0.82f, 13 ).bends( 0.2f, 0.25f, 0.3f ).posts( 0 )
        .p( JUNGLE_TREE, 0.3f, 40, 200 ).p( DEAD_TREE, 0.05f, 6, 60 ).p( COW, 0.04f, 8, 60 ).v( 2 ).p( RANCH_FENCE, 0.3f, 6, 6, 1, 1 ).p( HUT, 0.005f, 8, 40 );
    W( b, BE::HUMAHUACA, "QUEBRADA DE HUMAHUACA", "ARGENTINA", S::DESERT ).land( 0xB8A070, 0xAC9468, 0xA89A70 ).verge( 0xB87A5A ).hills( 36, 20, 20 )
        .drive( 2, 0.2f, 0.95f, 15 ).bends( 0.45f, 0.4f, 0.45f )
        .p( BADLAND, 0.04f, 20, 250, 0.8f, 1.5f ).c( 0xC87A60, 0x8A9A6A ).p( CACTUS, 0.1f, 3, 80 ).p( ADOBE, 0.02f, 8, 60 ).c( 0xB89A78, 0x9A8A6A )
        .p( CHURCH, 0.003f, 20, 100 ).c( 0xF0ECE0, 0xF0ECE0 ).p( LLAMA, 0.03f, 8, 80 ).p( POPLAR, 0.03f, 6, 60 );
    W( b, BE::IGUAZU, "IGUAZU FALLS", "ARGENTINA", S::HAZE ).fog( 0.0024f, 0xD0DCD8 ).sea( 0, 1, -20 ).water( 0x7A7A5A ).land( 0x3E7A32, 0x38702C, 0x3E6A3A )
        .hills( 20, 10, -10 ).drive( 2, 0.3f, 0.92f, 13 ).bends( 0.45f, 0.4f, 0.3f )
        .p( WATERFALL, 0.0f, 80, 160, 1.0f, 1.2f ).v( 0 ).once( 3 ).p( JUNGLE_TREE, 0.45f, 3, 150 ).p( PALM, 0.1f, 3, 60 ).p( FERN, 0.2f, 1, 20 );

    // ---- The Caribbean, Central America, Mexico ----------------------------------------------------------------------
    W( b, BE::MARACAS_BAY, "MARACAS BAY", "TRINIDAD AND TOBAGO", S::TROPICAL ).sea( 0, 1, -24 ).water( 0x1E8AA8 ).land( 0x3E8A32, 0x38802C, 0x3E7A3A )
        .hills( 40, 30, -20 ).drive( 2, 0.4f, 0.95f, 16 ).bends( 0.8f, 0.7f, 0.7f ).rails()
        .p( JUNGLE_TREE, 0.3f, 6, 150 ).p( BAMBOO, 0.15f, 2, 30 ).p( PALM, 0.15f, 2, 60 ).p( BOAT, 0.02f, 40, 150 ).c( 0x2A8AC8, 0xF0F0F0 ).pastel().floating();
    W( b, BE::GRAND_ANSE, "GRAND ANSE", "GRENADA", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x2AB8C8 ).land( 0x4E9A3A, 0x469034, 0x4E8A44 ).verge( 0xF4ECDC )
        .hills( 24, 14, 0 ).drive( 2, 0.4f, 0.96f, 16 ).bends( 0.5f, 0.45f, 0.45f )
        .p( BROADLEAF, 0.15f, 4, 100 ).c( 0x2E6E2A ).p( PALM, 0.25f, 2, 40 ).p( FLAT_HOUSE, 0.04f, 4, 60 ).c( 0xE8C890, 0xC83A2A ).pastel()
        .p( SAILBOAT, 0.02f, 40, 200 ).floating();
    W( b, BE::LA_SOUFRIERE, "LA SOUFRIERE", "SAINT VINCENT", S::MIST ).rain( 0.3f ).sea( 0, 1, -6 ).water( 0x1E6A8A ).land( 0x3E7A32, 0x38702C, 0x2E2A28 )
        .verge( 0x2A2624 ).hills( 40, 20, -4 ).drive( 2, 0.2f, 0.92f, 16 ).bends( 0.6f, 0.5f, 0.6f )
        .p( VOLCANO_CONE, 0.005f, 300, 600, 1.2f, 1.6f ).c( 0x3A4A3A, 0x6A6A60 ).p( BANANA, 0.2f, 3, 60 ).p( PALM, 0.1f, 2, 40 ).p( JUNGLE_TREE, 0.15f, 10, 150 );
    W( b, BE::BATHSHEBA, "BATHSHEBA", "BARBADOS", S::TROPICAL ).clouds( 0.45f ).sea( 0, 1, -3 ).water( 0x1E7AA0 ).land( 0x5E9A44, 0x56903E, 0x5E8A4A )
        .verge( 0xF0E4C8 ).hills( 20, 10, -2 ).drive( 2, 0.3f, 0.96f, 16 ).bends( 0.45f, 0.4f, 0.4f )
        .p( BOULDER, 0.06f, 4, 11, 2.0f, 3.5f ).p( PALM, 0.25f, 2, 60 ).p( COTTAGE, 0.03f, 4, 60 ).c( 0xE8C890, 0x8A8A8A ).pastel().p( FLOWER_BUSH, 0.06f, 2, 20 ).c( 0xC8307A );
    W( b, BE::PITONS, "THE PITONS", "SAINT LUCIA", S::TROPICAL ).sea( 0, 1, -10 ).water( 0x1E8AB0 ).land( 0x3E8A32, 0x38802C, 0x3E7A3A )
        .hills( 30, 20, -6 ).drive( 2, 0.3f, 0.95f, 16 ).bends( 0.7f, 0.6f, 0.65f )
        .p( VOLCANO_CONE, 0.0f, 220, 320, 1.0f, 1.0f ).c( 0x2E6A2E, 0x2E6A2E ).once( 2 ).p( BANANA, 0.2f, 3, 60 ).p( PALM, 0.12f, 2, 40 )
        .p( BOAT, 0.02f, 40, 150 ).c( 0xC83A2A, 0xF0F0F0 ).pastel().floating();
    W( b, BE::BOILING_LAKE, "MORNE TROIS PITONS", "DOMINICA", S::RAIN ).rain( 0.5f ).land( 0x3E7A32, 0x38702C, 0x3E6A3A ).hills( 46, 24, 20 )
        .drive( 2, 0.15f, 0.88f, 16 ).bends( 0.7f, 0.6f, 0.7f )
        .p( WATERFALL, 0.006f, 40, 200, 0.4f, 0.6f ).v( 1 ).p( TREE_FERN, 0.2f, 2, 40 ).p( JUNGLE_TREE, 0.35f, 4, 150 ).p( GEYSER, 0.003f, 30, 150, 0.6f, 0.9f )
        .p( BANANA, 0.05f, 3, 40 );
    W( b, BE::ENGLISH_HARBOUR, "ENGLISH HARBOUR", "ANTIGUA AND BARBUDA", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x2AB0C0 ).land( 0x7A9A50, 0x709048, 0x8A9A5A )
        .hills( 20, 10, -2 ).drive( 2, 0.3f, 0.96f, 16 ).bends( 0.45f, 0.4f, 0.35f )
        .p( COTTAGE, 0.04f, 4, 40 ).c( 0xC8B898, 0x8A5A4A ).p( SAILBOAT, 0.06f, 36, 150 ).floating().p( FORT, 0.004f, 30, 150 ).c( 0xC8B898, 0xB8A888 ).v( 0 )
        .p( PALM, 0.15f, 2, 40 ).p( SHRUB, 0.15f, 2, 60 ).c( 0x6A8A48 );
    W( b, BE::BRIMSTONE_HILL, "BRIMSTONE HILL", "SAINT KITTS AND NEVIS", S::TROPICAL ).sea( 0, 1, -6 ).water( 0x1E8AB0 ).land( 0x6A9A48, 0x609040, 0x5E8A4A )
        .fields( 0.45f, 0x8AC050, 0x7AB048, 0x9AC060 ).hills( 24, 16, -4 ).drive( 2, 0.3f, 0.96f, 16 ).bends( 0.45f, 0.4f, 0.4f )
        .p( FORT, 0.0f, 120, 200, 1.4f, 1.4f ).c( 0x8A8278, 0x7A7268 ).v( 0 ).once().p( ROUND_MILL, 0.008f, 20, 150 ).v( 1 ).p( PALM, 0.12f, 2, 60 )
        .p( VOLCANO_CONE, 0.003f, 300, 600, 1.0f, 1.3f ).c( 0x3A6A3A, 0x4A6A4A );
    W( b, BE::PUNTA_CANA, "PUNTA CANA", "DOMINICAN REPUBLIC", S::TROPICAL ).sea( 0, 1, -2 ).water( 0x30C0C8 ).land( 0x6A9A48, 0x609040, 0x5E8A4A ).verge( 0xF4ECDC )
        .fields( 0.3f, 0x8AC050, 0x7AB048, 0x9AC060 ).hills( 4, 0, 0 ).drive( 2, 0.5f, 0.98f, 16 ).bends( 0.25f, 0.25f, 0.05f )
        .p( PALM, 0.4f, 2, 60 ).p( FLAT_HOUSE, 0.04f, 6, 60 ).c( 0xF0ECE0, 0xC8A060 ).p( PARASOL, 0.04f, 3, 11 ).pastel().p( SAILBOAT, 0.01f, 60, 250 ).floating();
    W( b, BE::CITADELLE, "CITADELLE LAFERRIERE", "HAITI", S::MIST ).land( 0x4E8A3A, 0x468034, 0x4A7A44 ).verge( 0x8A6A50 ).hills( 50, 30, 30 )
        .drive( 2, 0.4f, 0.9f, 16 ).bends( 0.75f, 0.65f, 0.8f ).road( 0x7A7064, 0x7A7064 ).posts( 0 )
        .p( FORT, 0.0f, 180, 260, 2.0f, 2.0f ).c( 0x8A8278, 0x7A7268 ).v( 0 ).once().p( BANANA, 0.15f, 3, 60 ).p( HUT, 0.02f, 6, 40 )
        .p( BROADLEAF, 0.15f, 4, 100 ).c( 0x2E6E2A ).p( FLAT_HOUSE, 0.02f, 6, 60 ).c( 0xE8C890, 0x8A8A8A ).pastel();
    W( b, BE::EXUMA, "EXUMA CAYS", "BAHAMAS", S::TROPICAL ).sea( 1, 1, -1.5f ).water( 0x40E0E0, 0.98f ).land( 0xF4ECDC, 0xE8E0D0, 0x6A9A4A )
        .verge( 0xF4ECDC ).hills( 2, 0, 0 ).drive( 2, 0.15f, 0.97f, 16 ).bends( 0.3f, 0.3f, 0.05f ).bridges( 0.35f, BR::CAUSEWAY, 0xE8E4DC )
        .p( PALM, 0.3f, 2, 11 ).p( SAILBOAT, 0.03f, 40, 250 ).floating().p( BOAT, 0.02f, 36, 150 ).c( 0xF0F0F0, 0x2A8AC8 ).floating()
        .p( FLAT_HOUSE, 0.02f, 3, 11 ).c( 0xE8C890, 0xF0F0F0 ).pastel();
    W( b, BE::PANAMA_CANAL, "PANAMA CANAL", "PANAMA", S::HAZE ).sea( 1, 0, -6 ).water( 0x3A6A6A ).land( 0x4E8A3A, 0x468034, 0x4A7A44 ).hills( 16, 0, 8 )
        .drive( 3, 0.9f, 0.96f, 17 ).bends( 0.3f, 0.3f, 0.2f ).bridges( 0.35f, BR::CABLE_STAYED, 0xE8E8E8 )
        .p( SHIP, 0.01f, 60, 200, 1.0f, 1.4f ).floating().p( JUNGLE_TREE, 0.25f, 6, 150 ).p( CRANE, 0.003f, 10, 40 ).p( CONTAINERS, 0.008f, 10, 40 )
        .p( PALM, 0.08f, 2, 30 );
    W( b, BE::OMETEPE, "OMETEPE", "NICARAGUA", S::TROPICAL ).sea( 1, 0, -3 ).water( 0x4A7A8A ).land( 0x6A9A48, 0x609040, 0x5E8A4A ).hills( 10, 0, 6 )
        .drive( 2, 0.15f, 0.92f, 17 ).bends( 0.4f, 0.35f, 0.25f )
        .p( VOLCANO_CONE, 0.0f, 300, 420, 1.6f, 1.6f ).c( 0x4A6A40, 0x6A7A60 ).once( 2 ).p( BANANA, 0.15f, 3, 60 ).p( HORSE, 0.02f, 6, 60 ).p( HUT, 0.02f, 6, 40 )
        .p( BROADLEAF, 0.1f, 4, 100 ).c( 0x3E7A34 );
    W( b, BE::RUTA_DE_LAS_FLORES, "RUTA DE LAS FLORES", "EL SALVADOR", S::MORNING ).land( 0x4E8A3A, 0x468034, 0x4A7A44 ).hills( 34, 14, 14 )
        .drive( 2, 0.4f, 0.95f, 17 ).bends( 0.65f, 0.55f, 0.6f )
        .p( SHRUB, 0.3f, 2, 80, 0.6f, 0.9f ).c( 0x2E5A2A ).p( FLOWER_BUSH, 0.12f, 2, 20 ).c( 0xE8508A ).p( COTTAGE, 0.03f, 6, 60 ).c( 0x7AA8D8, 0xB0583A ).pastel()
        .p( VOLCANO_CONE, 0.004f, 300, 600, 1.2f, 1.5f ).c( 0x4A5A48, 0x6A7068 ).p( CHURCH, 0.003f, 20, 120 ).c( 0xF0ECE0, 0xF0ECE0 );
    W( b, BE::COPAN, "COPAN", "HONDURAS", S::HAZE ).land( 0x5E9440, 0x568A3A, 0x5A8448 ).hills( 20, 6, 6 ).drive( 2, 0.2f, 0.93f, 17 ).bends( 0.45f, 0.4f, 0.4f )
        .p( STEP_PYRAMID, 0.006f, 40, 200, 0.6f, 0.9f ).c( 0xA8A090 ).p( MENHIR, 0.01f, 8, 60, 0.6f, 0.8f ).p( JUNGLE_TREE, 0.25f, 6, 150 )
        .p( ADOBE, 0.015f, 8, 60 ).c( 0xF0ECE0, 0xB0583A ).p( HORSE, 0.01f, 8, 60 );
    W( b, BE::BLUE_HOLE, "BARRIER REEF", "BELIZE", S::TROPICAL ).sea( 1, 1, -1.5f ).water( 0x30C8D0 ).land( 0xF0E8D8, 0xE6DECE, 0x6A9A4A ).verge( 0xF0E8D8 )
        .hills( 2, 0, 0 ).drive( 2, 0.15f, 0.96f, 17 ).bends( 0.3f, 0.3f, 0.05f ).bridges( 0.3f, BR::CAUSEWAY, 0xE0DCD4 )
        .p( PALM, 0.3f, 2, 11 ).p( WATER_VILLA, 0.04f, 36, 150 ).floating().p( SAILBOAT, 0.02f, 40, 200 ).floating().p( MANGROVE, 0.08f, 2, 11 );
    W( b, BE::RIVIERA_MAYA, "TULUM", "MEXICO", S::TROPICAL ).sea( 0, 1, -8 ).water( 0x30C0C8 ).land( 0x5E9440, 0x568A3A, 0x5A8448 ).verge( 0xF0E8D8 )
        .hills( 6, 2, -6 ).drive( 2, 0.5f, 0.97f, 17 ).bends( 0.3f, 0.3f, 0.1f )
        .p( STEP_PYRAMID, 0.006f, 8, 11, 0.3f, 0.4f ).c( 0xC8C0A8 ).p( RUIN, 0.02f, 4, 11 ).p( PALM, 0.25f, 2, 40 ).p( JUNGLE_TREE, 0.15f, 10, 150 )
        .p( PARASOL, 0.02f, 3, 11 ).pastel();
    W( b, BE::OAXACA, "HIERVE EL AGUA", "MEXICO", S::NOON ).land( 0xA89A60, 0x9C8E58, 0xA8946A ).verge( 0xC8B898 ).hills( 34, 20, -20 )
        .drive( 2, 0.25f, 0.95f, 17 ).bends( 0.65f, 0.55f, 0.65f )
        .p( WATERFALL, 0.0f, 80, 140, 0.6f, 0.6f ).c( 0xD8D0C0, 0xE8E2D4 ).v( 1 ).once().p( AGAVE, 0.15f, 3, 60 ).p( CACTUS, 0.06f, 3, 80 )
        .p( ADOBE, 0.015f, 8, 60 ).c( 0xC8907A, 0x9A6A4A ).p( SHRUB, 0.15f, 2, 60 ).c( 0x7A8A48 );
    W( b, BE::MEXICO_CITY, "MEXICO CITY", "MEXICO", S::HAZE ).fog( 0.0022f ).land( 0x6A8A58, 0x628050, 0x6A7A62 ).ranges( 0x5A6476, 0xE8ECF4, 9 )
        .drive( 4, 1.7f, 0.96f, 17 ).bends( 0.25f, 0.3f, 0.1f ).lamps( 30 ).posts( 0 )
        .landmark( Lm::VICTORY_COLUMN, 25, 40 ).p( ROW_HOUSES, 0.15f, 4, 30 ).c( 0xE8A870, 0x8A8A8A ).pastel().p( GLASS_TOWER, 0.05f, 30, 200 ).c( 0x6A8AA0 )
        .p( BROADLEAF, 0.15f, 1.5f, 10 ).c( 0x8A6AC8 ).p( VOLCANO_CONE, 0.002f, 400, 600, 1.6f, 2.0f ).c( 0x5A5A68, 0xF4F6FA );
    W( b, BE::COPPER_CANYON, "COPPER CANYON", "MEXICO", S::GOLDEN ).land( 0x9A9058, 0x8E8450, 0x9A8A62 ).verge( 0xB87A50 ).hills( 60, 40, -90 )
        .drive( 2, 0.15f, 0.94f, 17 ).bends( 0.85f, 0.8f, 0.85f ).posts( 0 )
        .p( PINE, 0.25f, 4, 150 ).p( BUTTE, 0.004f, 150, 400 ).p( ADOBE, 0.008f, 10, 60 ).c( 0xB89A78, 0x8A6A4A ).p( ROCK, 0.12f, 3, 60 ).p( AGAVE, 0.05f, 3, 40 );

    // ---- The United States and Canada --------------------------------------------------------------------------------
    W( b, BE::LA_FREEWAY, "LOS ANGELES", "USA", S::HAZE ).fog( 0.0022f, 0xD8C8B0 ).land( 0x8A9A60, 0x809058, 0x9A9470 ).drive( 4, 1.8f, 1.0f, 0 )
        .bends( 0.25f, 0.25f, 0.15f ).gantries( 500 ).posts( 0 ).lamps( 40 )
        .p( WAX_PALM, 0.2f, 2, 20 ).p( GLASS_TOWER, 0.04f, 60, 250 ).c( 0x7A9AB0 ).p( BILLBOARD, 0.03f, 4, 12 ).p( FLAT_HOUSE, 0.08f, 6, 60 ).c( 0xF0E0C8, 0xC85A3A ).pastel()
        .p( SHRUB, 0.1f, 2, 60 ).c( 0x7A8A50 );
    W( b, BE::HOLLYWOOD, "HOLLYWOOD HILLS", "USA", S::GOLDEN ).land( 0x9A9A58, 0x909050, 0xA0986A ).hills( 30, 20, -10 ).drive( 2, 0.7f, 0.98f, 0 )
        .bends( 0.85f, 0.75f, 0.7f ).lamps( 50 )
        .landmark( Lm::HOLLYWOOD_SIGN, 150, 250 ).p( WAX_PALM, 0.12f, 2, 30 ).p( FLAT_HOUSE, 0.06f, 6, 60 ).c( 0xF4F2EA, 0x4A4A50 ).p( SHRUB, 0.25f, 2, 60 ).c( 0x7A8048 )
        .p( GLASS_TOWER, 0.006f, 150, 300 ).c( 0x7A9AB0 );
    W( b, BE::NAPA, "NAPA VALLEY", "USA", S::GOLDEN ).land( 0x9AA058, 0x909450, 0xA89A62 ).hills( 20, 6, 6 ).drive( 2, 0.5f, 1.0f, 24 ).bends( 0.35f, 0.3f, 0.3f )
        .p( VINE_ROW, 0.85f, 5, 12, 1, 1 ).p( VINE_ROW, 0.8f, 14, 21, 1, 1 ).p( VINE_ROW, 0.6f, 23, 40, 1, 1 ).p( OAK, 0.05f, 40, 150 )
        .p( ADOBE, 0.008f, 30, 120 ).c( 0xE8DCC0, 0xC85A3A );
    W( b, BE::SAN_FRANCISCO, "SAN FRANCISCO", "USA", S::MIST ).fog( 0.003f, 0xE0E4E8 ).sea( 0, 1, -8 ).water( 0x3A5A6A ).land( 0x6A8A58, 0x628050, 0x6A7A62 )
        .drive( 3, 1.1f, 0.98f, 0 ).bends( 0.35f, 0.4f, 1.0f ).lamps( 35 ).posts( 0 ).bridges( 0.5f, BR::SUSPENSION, 0xC0402C )
        .p( ROW_HOUSES, 0.2f, 3, 30 ).c( 0xE8C8A0, 0x5A5A60 ).pastel().p( GLASS_TOWER, 0.05f, 40, 250 ).c( 0x8AA0B0 ).p( CYPRESS, 0.04f, 4, 40 )
        .p( SAILBOAT, 0.02f, 40, 200 ).floating();
    W( b, BE::OREGON_COAST, "OREGON COAST", "USA", S::OVERCAST ).clouds( 0.7f ).sea( 0, 1, -10 ).water( 0x2A5068 ).land( 0x5E8A44, 0x56803E, 0x4E7A48 )
        .verge( 0xE0D8C4 ).hills( 24, 10, -6 ).drive( 2, 0.4f, 0.98f, 24 ).bends( 0.55f, 0.45f, 0.45f ).bridges( 0.2f, BR::ARCH, 0xD8D8D0 )
        .p( SEA_STACK, 0.02f, 40, 200, 0.8f, 1.4f ).c( 0x3A3A3E ).floating().p( SPRUCE, 0.45f, 4, 150 ).p( LIGHTHOUSE, 0.004f, 20, 100 )
        .p( COTTAGE, 0.01f, 8, 60 ).c( 0xC8B898, 0x4A4A50 );
    W( b, BE::SEATTLE, "SEATTLE", "USA", S::RAIN ).rain( 0.3f ).sea( 0, 1, -4 ).water( 0x2A4A5A ).land( 0x5E8A4A, 0x568044, 0x4E7A4A ).drive( 3, 1.3f, 0.95f, 1 )
        .bends( 0.35f, 0.35f, 0.35f ).lamps( 30 ).posts( 0 )
        .landmark( Lm::SPACE_NEEDLE, 60, 120 ).p( GLASS_TOWER, 0.1f, 20, 250 ).c( 0x6A8AA0 ).p( SPRUCE, 0.15f, 4, 100 )
        .p( VOLCANO_CONE, 0.002f, 450, 650, 2.2f, 2.6f ).c( 0x8A8A90, 0xF4F6FA ).p( SHIP, 0.004f, 80, 250 ).floating();
    W( b, BE::GOING_TO_THE_SUN, "GOING TO THE SUN ROAD", "USA", S::ALPINE ).land( 0x6A9A48, 0x609040, 0x6E8E58 ).verge( 0x8A8478 ).ranges( 0x4A5464, 0xE8ECF4, 14 )
        .hills( 60, 60, -50 ).drive( 2, 0.3f, 0.97f, 24 ).bends( 0.8f, 0.7f, 0.85f ).tunnels( 0.1f )
        .p( STONE_WALL, 0.5f, 2, 2, 1, 1 ).p( WATERFALL, 0.008f, 6, 12, 0.12f, 0.2f ).v( 1 ).p( PEAK, 0.008f, 150, 400, 1.2f, 1.6f ).c( 0x6A5A50, 0xF4F6FA )
        .p( SPRUCE, 0.2f, 10, 150 ).p( ROCK, 0.1f, 3, 50 );
    W( b, BE::TETONS, "GRAND TETON", "USA", S::MORNING ).land( 0x9AA068, 0x8E9460, 0x8A9468 ).ranges( 0x4A5464, 0xE8ECF4, 15 ).hills( 6, 2, 2 )
        .drive( 2, 0.3f, 1.0f, 24 ).bends( 0.25f, 0.25f, 0.2f )
        .p( PEAK, 0.008f, 200, 450, 1.4f, 1.8f ).c( 0x5A5E66, 0xF4F6FA ).p( BISON, 0.03f, 10, 120 ).p( BARN, 0.004f, 30, 150 ).p( SHRUB, 0.3f, 2, 80, 0.4f, 0.7f ).c( 0x9AA088 )
        .p( SPRUCE, 0.08f, 20, 150 ).p( RANCH_FENCE, 0.2f, 6, 6, 1, 1 );
    W( b, BE::BONNEVILLE, "BONNEVILLE SALT FLATS", "USA", S::NOON ).land( 0xF4F2EC, 0xEAE8E2, 0xE8E6E0 ).verge( 0xF0EEE8 ).shine( 0.4f ).ranges( 0x7A7A88, 0xC8CCD8, 6 )
        .hills( 0, 0, 0 ).drive( 2, 0.05f, 1.0f, 24 ).bends( 0.02f, 0.1f, 0.0f ).posts( 100 )
        .p( MAILBOX, 0.002f, 3, 6 ).p( ROCK, 0.004f, 20, 200 );
    W( b, BE::SEDONA, "SEDONA", "USA", S::GOLDEN ).land( 0xB8704A, 0xAC6844, 0xA8704E ).verge( 0xB86A44 ).hills( 20, 6, 6 ).drive( 2, 0.5f, 0.98f, 24 )
        .bends( 0.45f, 0.4f, 0.4f )
        .p( MONOLITH, 0.006f, 150, 400, 0.6f, 1.0f ).c( 0xB85A3A ).p( BUTTE, 0.006f, 100, 300 ).p( SHRUB, 0.3f, 2, 80 ).c( 0x5E7A48 ).p( AGAVE, 0.05f, 3, 40 )
        .p( ADOBE, 0.01f, 10, 60 ).c( 0xC8806A, 0x8A5A44 );
    W( b, BE::WHITE_SANDS, "WHITE SANDS", "USA", S::NOON ).land( 0xF4F2EC, 0xEAE8E0, 0xEEECE6 ).verge( 0xF4F2EC ).shine( 0.15f ).ranges( 0x6A6A80, 0xB8BCCC, 6 )
        .hills( 6, 2, 2 ).drive( 2, 0.15f, 0.9f, 24 ).bends( 0.3f, 0.3f, 0.3f ).road( 0xB8B0A2, 0xB8B0A2 ).sand().posts( 0 )
        .p( DUNE, 0.08f, 35, 300, 0.6f, 1.6f ).c( 0xF6F4EE ).p( AGAVE, 0.03f, 3, 60, 0.6f, 0.9f );
    W( b, BE::PIKES_PEAK, "PIKES PEAK", "USA", S::ALPINE ).land( 0x7A8A58, 0x6E7E50, 0x8A7A66 ).verge( 0xA87A5A ).ranges( 0x5A5E70, 0xE8ECF4, 9 )
        .hills( 50, 40, -40 ).drive( 2, 0.3f, 0.97f, 24 ).bends( 1.0f, 0.95f, 1.0f ).posts( 0 )
        .p( PINE, 0.2f, 4, 150 ).p( ROCK, 0.25f, 3, 60 ).c( 0xA87A5A ).p( BOULDER, 0.04f, 6, 80 ).p( SNOWBANK, 0.05f, 1, 6 );
    W( b, BE::MILLION_DOLLAR, "MILLION DOLLAR HIGHWAY", "USA", S::ALPINE ).land( 0x6A9A48, 0x609040, 0x6E8E58 ).verge( 0x9A6A4A ).ranges( 0x4A5464, 0xE8ECF4, 13 )
        .hills( 60, 50, -60 ).drive( 2, 0.25f, 0.95f, 24 ).bends( 0.9f, 0.85f, 0.95f ).posts( 0 )
        .p( PEAK, 0.008f, 150, 400, 1.2f, 1.6f ).c( 0xA85A3A, 0xF4F6FA ).p( SPRUCE, 0.3f, 6, 150 ).p( BARN, 0.003f, 20, 100 ).p( ROCK, 0.1f, 3, 50 );
    W( b, BE::MOUNT_RUSHMORE, "BLACK HILLS", "USA", S::MORNING ).land( 0x7A9A50, 0x709048, 0x6A8458 ).hills( 34, 14, 14 ).drive( 2, 0.4f, 0.98f, 24 )
        .bends( 0.7f, 0.6f, 0.7f ).tunnels( 0.2f ).bridges( 0.15f, BR::BEAM )
        .p( PINE, 0.4f, 4, 150 ).p( GRANITE_DOME, 0.006f, 80, 300, 0.4f, 0.7f ).v( 0 ).p( PILLAR, 0.02f, 30, 200 ).p( BISON, 0.02f, 10, 120 );
    W( b, BE::CHICAGO, "CHICAGO", "USA", S::OVERCAST ).sea( 1, 0, -3 ).water( 0x3A6A7A ).land( 0x6A7A62, 0x627258, 0x5A6A5A ).drive( 4, 1.5f, 0.98f, 1 )
        .bends( 0.2f, 0.25f, 0.05f ).lamps( 30 ).posts( 0 ).bridges( 0.3f, BR::TRUSS, 0x3A4A5A )
        .p( GLASS_TOWER, 0.2f, 10, 250 ).c( 0x3A4A5A ).p( SUPERTALL, 0.003f, 150, 350 ).p( ROW_HOUSES, 0.04f, 6, 30 ).c( 0xA86A4A, 0x4A4A50 ).p( BROADLEAF, 0.06f, 2, 12 ).c( 0x5A8A40 );
    W( b, BE::NEW_ORLEANS, "LAKE PONTCHARTRAIN", "USA", S::HAZE ).sea( 1, 1, -2 ).water( 0x5A7A7A, 0.9f ).land( 0x6A8A48, 0x608040, 0x5E7A4A )
        .hills( 0, 0, 0 ).drive( 2, 0.5f, 1.0f, 24 ).bends( 0.05f, 0.1f, 0.0f ).bridges( 0.9f, BR::CAUSEWAY, 0xC8C8C0 ).posts( 0 )
        .p( MOSS_TREE, 0.06f, 3, 11 ).p( BOAT, 0.01f, 40, 200 ).c( 0xF0F0F0, 0x2A4A6A ).floating().p( SAILBOAT, 0.005f, 60, 300 ).floating();
    W( b, BE::TAIL_OF_THE_DRAGON, "TAIL OF THE DRAGON", "USA", S::MORNING ).land( 0x5E8A44, 0x56803E, 0x4E7A48 ).hills( 30, 14, 10 ).drive( 2, 0.4f, 0.98f, 24 )
        .bends( 1.0f, 0.85f, 0.6f ).rails()
        .p( BROADLEAF, 0.5f, 3, 150 ).c( 0x3E7A34 ).p( SPRUCE, 0.1f, 6, 150 ).p( BILLBOARD, 0.004f, 4, 10 ).p( CABIN, 0.004f, 10, 60 );
    W( b, BE::BLUE_RIDGE, "BLUE RIDGE PARKWAY", "USA", S::HAZE ).fog( 0.0024f, 0x9AB0D0 ).land( 0x6A9A48, 0x609040, 0x5E8A4A ).ranges( 0x3A5A8A, 0x7A9AC8, 9 )
        .hills( 30, 14, -20 ).drive( 2, 0.3f, 0.98f, 24 ).bends( 0.6f, 0.5f, 0.6f ).bridges( 0.1f, BR::ARCH, 0x8A8278 ).posts( 0 )
        .p( BROADLEAF, 0.3f, 3, 150 ).c( 0x4E8A3A ).p( STONE_WALL, 0.2f, 2, 2, 1, 1 ).p( CABIN, 0.006f, 10, 60 ).p( RANCH_FENCE, 0.15f, 6, 6, 1, 1 )
        .p( FLOWER_BUSH, 0.05f, 2, 30 ).c( 0xE88AB0 );
    W( b, BE::WASHINGTON_DC, "WASHINGTON", "USA", S::MORNING ).land( 0x6A9A50, 0x609048, 0x6A8A5A ).drive( 3, 1.2f, 1.0f, 1 ).bends( 0.25f, 0.3f, 0.1f )
        .lamps( 30 ).posts( 0 ).bridges( 0.1f, BR::ARCH, 0xD8D4CC )
        .landmark( Lm::CAPITOL, 100, 160 ).landmark( Lm::OBELISK, 120, 200 ).p( CHERRY, 0.15f, 2, 30 ).p( COLUMNS, 0.01f, 10, 60 )
        .p( ROW_HOUSES, 0.06f, 6, 30 ).c( 0xA86A4A, 0x4A4A50 ).p( BROADLEAF, 0.1f, 4, 80 ).c( 0x5A8A40 );
    W( b, BE::HANA, "ROAD TO HANA", "HAWAII", S::TROPICAL ).sea( 0, 1, -20 ).water( 0x1E7AB0 ).land( 0x3E8A32, 0x38802C, 0x3E7A3A ).hills( 40, 30, -20 )
        .drive( 2, 0.4f, 0.95f, 16 ).bends( 0.95f, 0.9f, 0.6f ).bridges( 0.4f, BR::BEAM ).posts( 0 )
        .p( WATERFALL, 0.008f, 6, 12, 0.12f, 0.2f ).v( 1 ).p( JUNGLE_TREE, 0.3f, 4, 150 ).p( BANANA, 0.12f, 2, 30 ).p( EUCALYPTUS, 0.05f, 4, 60 )
        .p( TREE_FERN, 0.08f, 2, 30 );
    W( b, BE::DALTON, "DALTON HIGHWAY", "ALASKA", S::ARCTIC ).land( 0x8A9A58, 0x7E8E50, 0x8A8A60 ).verge( 0x8A8478 ).road( 0x8A8680, 0x8A8680 )
        .ranges( 0x8A92A0, 0xE0E6EE, 8 ).hills( 14, 4, 4 ).drive( 2, 0.08f, 0.84f, 24 ).bends( 0.25f, 0.25f, 0.35f ).posts( 0 )
        .p( PIPELINE, 1.0f, 11, 11, 1, 1 ).p( SPRUCE, 0.08f, 6, 150, 0.5f, 0.8f ).p( MOOSE, 0.01f, 10, 100 ).p( SHRUB, 0.2f, 2, 60 ).c( 0x8A7A48 )
        .p( WARNING_SIGN, 0.003f, 1, 1, 1, 1 ).v( 2 );
    W( b, BE::YUKON, "KLONDIKE HIGHWAY", "CANADA", S::ARCTIC ).sea( 0, 0.5f, -8 ).water( 0x3A6A7A ).land( 0x6A8A48, 0x608040, 0x5E7A4A )
        .ranges( 0x5A6476, 0xE8ECF4, 10 ).hills( 24, 8, -4 ).drive( 2, 0.12f, 0.95f, 24 ).bends( 0.4f, 0.35f, 0.4f )
        .p( SPRUCE, 0.4f, 4, 150 ).p( CABIN, 0.006f, 10, 60 ).p( MOOSE, 0.01f, 10, 100 ).p( BIRCH, 0.06f, 4, 60 );
    W( b, BE::ICEFIELDS, "ICEFIELDS PARKWAY", "CANADA", S::ALPINE ).sea( 0, 0.6f, -4 ).water( 0x2AB8C8 ).land( 0x5E9046, 0x56863E, 0x5E8050 )
        .ranges( 0x6A7A8A, 0xF2F4F8, 16 ).hills( 40, 14, 14, 0.2f ).drive( 2, 0.3f, 0.98f, 24 ).bends( 0.4f, 0.35f, 0.4f )
        .p( PEAK, 0.01f, 150, 450, 1.3f, 1.8f ).c( 0x7A7C82, 0xF4F6FA ).p( SPRUCE, 0.45f, 3, 150 ).p( MOOSE, 0.01f, 10, 80 ).p( WARNING_SIGN, 0.003f, 1, 1, 1, 1 ).v( 2 );
    W( b, BE::SEA_TO_SKY, "SEA TO SKY HIGHWAY", "CANADA", S::MIST ).sea( 0, 1, -20 ).water( 0x2A5A6A ).land( 0x4E8040, 0x46763A, 0x4A7044 )
        .ranges( 0x4A5464, 0xD8DEE8, 12 ).hills( 50, 40, -20 ).drive( 2, 0.7f, 0.98f, 24 ).bends( 0.65f, 0.55f, 0.55f ).rails()
        .p( GRANITE_DOME, 0.0f, 120, 200, 1.4f, 1.4f ).v( 0 ).once().p( SPRUCE, 0.45f, 4, 150 ).p( WATERFALL, 0.004f, 30, 120, 0.4f, 0.6f ).v( 1 )
        .p( SHIP, 0.002f, 100, 300, 0.6f, 0.8f ).floating();
    W( b, BE::NIAGARA, "NIAGARA FALLS", "CANADA", S::MORNING ).sea( 0, 1, -20 ).water( 0x4A8A9A ).land( 0x6A9A50, 0x609048, 0x6A8A5A )
        .drive( 3, 1.0f, 0.98f, 24 ).bends( 0.3f, 0.3f, 0.15f ).lamps( 35 ).posts( 0 )
        .p( WATERFALL, 0.0f, 50, 100, 1.2f, 1.2f ).v( 0 ).once( 2 ).floating().p( CASINO, 0.01f, 20, 120 ).p( GLASS_TOWER, 0.04f, 20, 150 ).c( 0x7A9AB0 )
        .p( TV_TOWER, 0.002f, 60, 200, 0.6f, 0.7f ).v( 0 ).p( BOAT, 0.01f, 40, 120 ).c( 0x2A5AAA, 0xF0F0F0 ).floating().p( BROADLEAF, 0.1f, 2, 30 ).c( 0x5A8A40 );
    W( b, BE::TORONTO, "TORONTO", "CANADA", S::MORNING ).sea( 1, 0, -3 ).water( 0x2A6A8A ).land( 0x6A7A62, 0x627258, 0x5A6A5A ).drive( 4, 1.5f, 0.98f, 1 )
        .bends( 0.2f, 0.25f, 0.05f ).lamps( 30 ).posts( 0 ).gantries( 600 )
        .p( TV_TOWER, 0.0f, 80, 120, 1.4f, 1.4f ).v( 0 ).once().p( GLASS_TOWER, 0.18f, 15, 250 ).c( 0x4A6A8A ).p( ROW_HOUSES, 0.04f, 6, 30 ).c( 0xA86A4A, 0x4A4A50 )
        .p( SAILBOAT, 0.01f, 40, 200 ).floating();
    W( b, BE::QUEBEC_CITY, "QUEBEC CITY", "CANADA", S::WINTER ).snow( 0.4f ).sea( 0, 1, -20 ).water( 0x5A6A7A ).land( 0xE0E4EA, 0xD4DAE2, 0xC8D0DA )
        .verge( 0xC8CCD2 ).hills( 20, 10, -10 ).drive( 2, 0.7f, 0.9f, 24 ).bends( 0.4f, 0.4f, 0.4f ).lamps( 30 ).posts( 0 )
        .p( CASTLE, 0.0f, 60, 100, 1.4f, 1.4f ).c( 0xA85A3A, 0x3A6A5A ).once().p( ROW_HOUSES, 0.15f, 4, 30 ).c( 0xC8B8A0, 0x3A6A5A ).pastel()
        .p( CHURCH, 0.004f, 20, 120 ).c( 0xC8C0B0, 0x6A8AA0 ).p( SNOWBANK, 0.2f, 0.5f, 3, 0.8f, 1.4f );
    W( b, BE::CABOT_TRAIL, "CABOT TRAIL", "CANADA", S::GOLDEN ).sea( 0, 1, -40 ).water( 0x1E4A6A ).land( 0x7A8A48, 0x708040, 0x8A7A50 ).hills( 40, 30, -30 )
        .drive( 2, 0.3f, 0.98f, 24 ).bends( 0.75f, 0.65f, 0.8f )
        .p( AUTUMN_TREE, 0.4f, 3, 150 ).p( SPRUCE, 0.15f, 6, 150 ).p( LIGHTHOUSE, 0.003f, 20, 100 ).p( COTTAGE, 0.01f, 8, 60 ).c( 0xF2F0EA, 0xA83A2A )
        .p( CHURCH, 0.003f, 20, 100 ).c( 0xF4F2EA, 0x4A4A50 );

    // ---- Scandinavia and Iceland -------------------------------------------------------------------------------------
    W( b, BE::NORTH_CAPE, "NORTH CAPE", "NORWAY", S::ARCTIC ).sun( 4, 0 ).sea( 0, 1, -30 ).water( 0x1E3A5A ).land( 0x8A9060, 0x7E8458, 0x6E7460 )
        .verge( 0x6A6A68 ).hills( 24, 10, -20 ).drive( 2, 0.12f, 0.92f, 23 ).bends( 0.45f, 0.4f, 0.5f ).tunnels( 0.25f )
        .p( REINDEER, 0.04f, 8, 100 ).p( ROCK, 0.2f, 3, 60 ).p( SHRUB, 0.2f, 2, 50, 0.4f, 0.6f ).c( 0x8A8050 ).p( CABIN, 0.003f, 10, 60 );
    W( b, BE::TROLLSTIGEN, "TROLLSTIGEN", "NORWAY", S::ALPINE ).land( 0x6A9A50, 0x608E48, 0x6A8458 ).verge( 0x6A6A6A ).ranges( 0x3A4048, 0xC8D0DC, 14 )
        .hills( 60, 60, -50 ).drive( 2, 0.3f, 0.96f, 23 ).bends( 1.0f, 1.0f, 1.0f ).bridges( 0.1f, BR::ARCH, 0x6A6A68 )
        .p( WATERFALL, 0.0f, 60, 120, 1.0f, 1.0f ).v( 1 ).once().p( PEAK, 0.01f, 150, 350, 1.0f, 1.4f ).c( 0x4A4E54, 0xE8ECF2 ).p( STONE_WALL, 0.5f, 2, 2, 1, 1 )
        .p( ROCK, 0.2f, 3, 60 ).p( SNOWBANK, 0.06f, 1, 6 );
    W( b, BE::ATLANTIC_ROAD, "ATLANTIC ROAD", "NORWAY", S::OVERCAST ).clouds( 0.85f ).rain( 0.15f ).sea( 1, 1, -3 ).water( 0x2A4458 ).land( 0x7A8A58, 0x6E7E50, 0x6A7458 )
        .verge( 0x5A5A58 ).hills( 4, 0, 0 ).drive( 2, 0.3f, 0.95f, 23 ).bends( 0.45f, 0.4f, 0.4f ).bridges( 0.7f, BR::BEAM ).rails()
        .p( ROCK, 0.06f, 36, 150, 1.5f, 3.0f ).floating().p( CABIN, 0.02f, 3, 11 ).p( BOAT, 0.01f, 40, 150 ).c( 0xC83A2A, 0xF0F0F0 ).floating();
    W( b, BE::ORESUND, "ORESUND BRIDGE", "SWEDEN", S::MORNING ).clouds( 0.5f ).sea( 1, 1, -4 ).water( 0x2A5A7A ).land( 0x7AAA50, 0x6E9E48, 0x86A05E )
        .drive( 4, 0.9f, 1.0f, 23 ).bends( 0.1f, 0.15f, 0.1f ).bridges( 0.8f, BR::CABLE_STAYED, 0xD8DCE0 ).tunnels( 0.15f ).posts( 0 )
        .p( WINDMILL, 0.01f, 80, 300, 1.2f, 1.6f ).floating().p( SHIP, 0.004f, 100, 300 ).floating().p( SAILBOAT, 0.006f, 60, 250 ).floating();
    W( b, BE::JOKULSARLON, "JOKULSARLON", "ICELAND", S::ARCTIC ).sea( 1, 1, -2 ).water( 0x3A6A8A ).land( 0x3A3A3C, 0x343436, 0x5A5A58 ).verge( 0x2E2E30 )
        .ranges( 0xC8D4E0, 0xF0F4F8, 10 ).hills( 4, 0, 0 ).drive( 2, 0.2f, 0.92f, 23 ).bends( 0.3f, 0.3f, 0.1f ).bridges( 0.4f, BR::SUSPENSION, 0x6A6A6E )
        .p( ICEBERG, 0.04f, 36, 250, 0.4f, 1.0f ).floating().p( ROCK, 0.05f, 3, 11 );
}

} // namespace sunstreak
