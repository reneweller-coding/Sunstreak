/**
 * @file TestMain.cpp
 * @brief Core tests without a window: the generator's determinism and bounds, the stage order, the car's physics,
 *        the track code.
 * @author reneweller
 */
#include "Achievements.h"
#include "Car.h"
#include "Climate.h"
#include "Data.h"
#include "Game.h"
#include "Hero.h"
#include "Passenger.h"
#include "Phrases.h"
#include "Police.h"
#include "Race.h"
#include "Records.h"
#include "ScriptNames.h"
#include "Spectacle.h"
#include "Track.h"
#include "Train.h"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace
{

int g_failures = 0;   ///< failed checks

/**
 * @brief Records a check.
 * @param[in] ok    the result
 * @param[in] what  what was checked
 */
void check( bool ok, const char *what )
{
    std::printf( "%s  %s\n", ok ? "ok  " : "FAIL", what );
    if( !ok ) ++g_failures;
}

} // namespace

/**
 * @brief Runs the tests.
 * @return the number of failures
 */
int main()
{
    using namespace sunstreak;

    { // Equal seeds give equal tracks, different seeds different ones.
        Track a( 12345 ), b( 12345 ), c( 54321 );
        a.generateTo( 20000.0 );
        b.generateTo( 20000.0 );
        c.generateTo( 20000.0 );
        const Segment &sa = a.segment( 3500 ), &sb = b.segment( 3500 ), &sc = c.segment( 3500 );
        check( sa.m_pos.m_x == sb.m_pos.m_x && sa.m_pos.m_z == sb.m_pos.m_z && sa.m_curve == sb.m_curve, "same seed, same track" );
        check( std::abs( sa.m_pos.m_x - sc.m_pos.m_x ) > 1.0, "different seed, different track" );
    }
    { // 300 km: the heading stays near north, the height bounded, checkpoints where they belong.
        Track t( 777 );
        float maxHeading = 0.0f, maxGrade = 0.0f;
        double minY = 0.0, maxY = 0.0;
        int checkpoints = 0, jumps = 0, works = 0;
        for( double s = 0.0; s < 300000.0; s += 5000.0 )
        {
            t.generateTo( s + 5000.0 );
            for( long i = t.firstIndex(); i < t.endIndex(); ++i )
            {
                const Segment &g = t.segment( i );
                maxHeading = std::max( maxHeading, std::abs( g.m_heading ) );
                maxGrade = std::max( maxGrade, std::abs( g.m_grade ) );
                minY = std::min( minY, g.m_pos.m_y );
                maxY = std::max( maxY, g.m_pos.m_y );
            }
            for( long i = static_cast<long>( s / SegmentLength ); i < static_cast<long>( ( s + 5000.0 ) / SegmentLength ); ++i )
            {
                const Segment &g = t.segment( i );
                if( g.m_flags & SEG_CHECKPOINT ) ++checkpoints;
                if( g.m_flags & SEG_JUMP ) ++jumps;
                if( ( g.m_flags & SEG_ROADWORKS ) && !( t.segment( i - 1 ).m_flags & SEG_ROADWORKS ) ) ++works;
            }
            t.discardBefore( s );
        }
        std::printf( "      heading %.2f rad, grade %.3f, height %.0f..%.0f m, %d checkpoints, %d jumps, %d roadworks\n", maxHeading,
                     maxGrade, minY, maxY, checkpoints, jumps, works );
        check( maxHeading < 1.45f, "heading within +-83 degrees" );
        check( maxGrade < 0.3f, "grades below 30 % (crest ramps included)" );
        check( maxY - minY < 600.0, "height bounded" );
        check( checkpoints == 119, "a checkpoint every 2.5 km" );
        check( jumps >= 2 && works > 5, "crests and roadworks appear" );   // fewer crests where passes climb in hairpins
    }
    { // Stages never repeat a biome back to back.
        Track t( 99 );
        bool  ok = true;
        for( int s = 1; s < 64; ++s ) ok = ok && t.stageBiome( s ) != t.stageBiome( s - 1 );
        check( ok, "no biome twice in a row" );
        Track forced( 99, TrackParams(), static_cast<int>( BiomeE::NEON ) );
        check( forced.stageBiome( 0 ) == BiomeE::NEON, "a requested first biome" );
        TrackParams world;
        world.m_biome = SceneryWorld;
        Track       tour( 1234, world );
        const auto &route = worldRoute();
        const int   first = routeIndex( tour.stageBiome( 0 ) );
        bool        follows = first >= 0;
        // Along the route where no fork is taken, a stage on the sea floor between landmasses.
        for( int s = 1, k = 1; k < static_cast<int>( route.size() ) + 3; ++s )
        {
            const BiomeE b = tour.stageBiome( s );
            if( biomeDef( b ).m_underwater ) continue;
            follows = follows && b == route[static_cast<size_t>( first + k ) % route.size()];
            ++k;
        }
        check( follows, "the world tour follows the route" );
        Track paris( 1234, world, static_cast<int>( BiomeE::PARIS ) );
        check( paris.stageBiome( 0 ) == BiomeE::PARIS && paris.stageBiome( 1 ) == route[static_cast<size_t>( routeIndex( BiomeE::PARIS ) + 1 )],
               "the world tour from a chosen place" );
    }
    { // The car reaches its speed on a straight and stops again.
        Track     t( 4242 );
        PlayerCar car;
        car.reset( carSpec( 0 ), StartLine, 0.0f, t );
        CarInput in;
        in.m_throttle = 1.0f;
        float t100 = -1.0f;
        for( int i = 0; i < 120 * 12; ++i )
        {
            t.generateTo( car.m_s + 500.0 );
            in.m_steer = clampf( -car.m_x * 0.3f + car.m_drift / std::max( 1.0f, steerLimit( car.m_speed, car.m_spec.m_topSpeed ) ), -1.0f, 1.0f );
            car.update( in, t, 1.0f / 120.0f, false );
            if( t100 < 0.0f && car.m_speed * 3.6f >= 100.0f ) t100 = static_cast<float>( i ) / 120.0f;
        }
        std::printf( "      0-100 km/h %.1f s, after 12 s %.0f km/h in gear %d\n", t100, car.m_speed * 3.6f, car.m_gear + 1 );
        check( t100 > 2.0f && t100 < 6.0f, "0-100 km/h in 2..6 s" );
        check( car.m_speed * 3.6f > 180.0f, "over 180 km/h after 12 s" );
        in.m_throttle = 0.0f;
        in.m_brake = 1.0f;
        for( int i = 0; i < 120 * 8; ++i ) car.update( in, t, 1.0f / 120.0f, false );
        check( car.m_speed < 0.1f, "brakes to a stop" );
    }
    { // Track codes carry every Track Lab value and come back unchanged.
        TrackSetup a;
        a.m_seed = 0x1234567;
        a.m_params.m_curvy = 7;
        a.m_params.m_sharp = -1;
        a.m_params.m_hilly = 0;
        a.m_params.m_traffic = 9;
        a.m_params.m_obstacles = 3;
        a.m_params.m_biome = 25;
        a.m_params.m_length = 12500.0;
        a.m_race = true;
        a.m_params.m_biome = static_cast<int>( BiomeE::COUNT ) - 1;
        const std::string code = encodeTrack( a );
        TrackSetup        b;
        const bool        ok = decodeTrack( code, b );
        check( code.size() == 17 && code[5] == '-' && code[11] == '-', "track code format" );
        check( ok && b.m_seed == a.m_seed && b.m_params.m_curvy == 7 && b.m_params.m_sharp == -1 && b.m_params.m_hilly == 0 &&
                   b.m_params.m_traffic == 9 && b.m_params.m_obstacles == 3 && b.m_params.m_biome == a.m_params.m_biome &&
                   b.m_params.m_length == 12500.0 &&
                   b.m_race,
               "track code round trip" );
        TrackSetup c;
        check( !decodeTrack( "NOT-A-CODE", c ), "a broken code is refused" );
        std::string typo = code;
        typo[2] = typo[2] == '7' ? '8' : '7';
        check( !decodeTrack( typo, c ), "a mistyped code is refused" );
        TrackSetup w;
        w.m_seed = 4711;
        w.m_params.m_biome = SceneryWorld;
        check( decodeTrack( encodeTrack( w ), c ) && c.m_params.m_biome == SceneryWorld && c.m_seed == 4711, "the world tour in a code" );
        if( biomeCount() > 510 )
        { // A place past the nine-bit field: sixteen symbols.
            TrackSetup far = a;
            far.m_params.m_biome = biomeCount() - 1;
            const std::string longCode = encodeTrack( far );
            check( longCode.size() == 19 && longCode[4] == '-' && decodeTrack( longCode, c ) && c.m_params.m_biome == biomeCount() - 1 &&
                       c.m_seed == far.m_seed && c.m_params.m_length == far.m_params.m_length && c.m_race == far.m_race,
                   "long codes for the new places" );
        }
        // A code from before the world tour: twelve symbols, the biome in six bits.
        uint64_t v = 0x1234567ull;
        for( int i = 0; i < 5; ++i ) v = ( v << 4 ) | 15u;
        v = ( v << 6 ) | 9u;
        v = ( v << 4 ) | 2u;
        v = ( v << 1 ) | 1u;
        std::string legacy;
        for( int i = 11; i >= 0; --i ) legacy += "0123456789ABCDEFGHJKMNPQRSTVWXYZ"[( v >> ( i * 5 ) ) & 31];
        check( decodeTrack( legacy, c ) && c.m_seed == 0x1234567ull && c.m_params.m_biome == 9 && c.m_race && c.m_params.m_length == 5000.0,
               "old codes still work" );
    }
    { // The rivals finish a race; the points add up.
        Race race;
        race.newChampionship( 77 );
        Track     t( race.roundSeed( 0 ), race.roundParams( 0 ) );
        PlayerCar car;
        car.reset( carSpec( 0 ), 0.0, 0.0f, t );
        race.grid( t, car, 1.0f );
        RaceEvents ev;
        float      time = 0.0f;
        for( int i = 0; i < 120 * 400 && time < 400.0f; ++i )
        {
            t.generateTo( car.m_s + 2000.0 );
            for( const Rival &r : race.m_rivals ) t.generateTo( r.m_s + 2000.0 );
            race.update( 1.0f / 120.0f, time, t, car, ev, true );
            time += 1.0f / 120.0f;
        }
        int finished = 0;
        for( const Rival &r : race.m_rivals ) finished += r.m_finished ? 1 : 0;
        std::printf( "      %d of %d rivals finished %.0f m in %.0f s\n", finished, RivalCount, race.m_finishS, time );
        check( finished == RivalCount, "the rivals reach the finish" );
        const auto result = race.finish( time );
        int        sum = 0;
        for( const ResultLine &l : result ) sum += l.m_points;
        check( result.size() == static_cast<size_t>( RivalCount + 1 ) && sum == 39, "points 10+8+6+5+4+3+2+1" );
    }
    { // Every biome is complete; the first bag of stages visits all of them.
        bool ok = true;
        for( int b = 0; b < biomeCount(); ++b )
        {
            const BiomeDef &d = biomeDef( static_cast<BiomeE>( b ) );
            ok = ok && d.m_name && d.m_lanes >= 2 && d.m_lanes <= 4 && d.m_station >= 0 && d.m_station < StationCount &&
                 d.m_grip > 0.5f && d.m_env.m_fogDensity > 0.0f && !d.m_props.empty();
        }
        check( ok, "all biomes defined" );
        // The route visits every place with a country once, and only those.
        int  onRoute = 0;
        bool once = true;
        for( int b = 0; b < biomeCount(); ++b )
        {
            const bool placed = biomeDef( static_cast<BiomeE>( b ) ).m_country != nullptr;
            int        n = 0;
            for( BiomeE r : worldRoute() ) n += static_cast<int>( r ) == b ? 1 : 0;
            once = once && n == ( placed ? 1 : 0 );
            onRoute += n;
        }
        std::printf( "      %d places on the world tour, %d countries\n", onRoute, countryCount() );
        check( once && onRoute >= 450 && countryCount() >= 200, "the world tour visits every place once" );
        // Every place on the route is on the map, and the route goes round the globe.
        bool                          located = true;
        std::bitset<LongitudeSectors> sectors;
        for( BiomeE r : worldRoute() )
        {
            GeoPoint g;
            located = located && placeLocation( r, g ) && g.m_lat > -60.0f && g.m_lat < 80.0f && g.m_lon >= -180.0f && g.m_lon <= 180.0f;
            if( located ) sectors.set( static_cast<size_t>( longitudeSector( g.m_lon ) ) );
        }
        check( located && sectors.all() && longitudeSector( -180.0f ) == 0 && longitudeSector( 180.0f ) == 0, "every place on the map, round the globe" );
        // Career points unlock the garage step by step; the globe wants countries, not points.
        Records career;
        career.m_points = 60000;
        check( career.rimsUnlocked( 3 ) && !career.rimsUnlocked( 4 ) && career.paintUnlocked( 6 ) && !career.paintUnlocked( 8 ) &&
                   !career.decalUnlocked( DecalCount - 1 ) && !career.paintUnlocked( PaintCount - 1 ) && Records::pointsText( 25000 ) == "25 000 POINTS",
               "garage unlocks" );
        // The places seen are kept by name, so new places in the data never move them.
        Records written, read;
        written.m_seen.set( static_cast<size_t>( BiomeE::PARIS ) );
        written.m_seen.set( static_cast<size_t>( biomeCount() - 1 ) );
        written.m_postcards.set( static_cast<size_t>( BiomeE::KYOTO ) );
        const std::string file = ( std::filesystem::temp_directory_path() / "sunstreak_test_records.txt" ).string();
        written.save( file );
        read.load( file );
        std::filesystem::remove( file );
        check( read.m_seen == written.m_seen && read.m_postcards == written.m_postcards, "records keep places by name" );
        // The tour clock: noon is bright and starless, midnight dark with stars, whatever the place wrote.
        const Environment &paris = biomeDef( BiomeE::PARIS ).m_env;
        const Environment  noon = environmentAtHour( paris, 12.5f ), midnight = environmentAtHour( paris, 0.5f );
        check( noon.m_sunIntensity > midnight.m_sunIntensity && midnight.m_stars > 0.3f && noon.m_stars < 0.05f &&
                   midnight.m_headlights > 0.8f && noon.m_rain == paris.m_rain,
               "the hour sets the light" );
        check( biomeDef( BiomeE::KIMBERLEY ).m_unpaved == 1 && biomeDef( BiomeE::WHITE_SANDS ).m_unpaved == 2 && biomeDef( BiomeE::PARIS ).m_unpaved == 0 &&
                   carSpec( 3 ).m_dirt > 1.0f && carSpec( 0 ).m_dirt < 1.0f,
               "gravel and sand, and a car for them" );
        check( countryInfo( "JAPAN" ).m_left && !countryInfo( "FRANCE" ).m_left && countryInfo( "USA" ).m_lines == 1 && countryIndex( "GERMANY" ) >= 0 &&
                   countryTableSize() >= 204,
               "the rules of the road by country" );
        Track t( 31337 );
        std::vector<bool> seen( static_cast<size_t>( biomeCount() ) );
        for( int s = 0; s < biomeCount(); ++s ) seen[static_cast<size_t>( t.stageBiome( s ) )] = true;
        bool all = true;
        for( bool v : seen ) all = all && v;
        check( all, "one bag of stages visits every biome" );
        { // The world tour forks at borders: two carriageways and a barrier, and the side taken decides the next place.
            TrackParams tour;
            tour.m_biome = SceneryWorld;
            Track world( 99, tour, static_cast<int>( BiomeE::CLIFFS ) );
            int   forks = 0, stage = -1;
            for( int st = 0; st < 40; ++st )
                if( world.fork( st ).m_exists )
                {
                    ++forks;
                    if( stage < 0 ) stage = st;
                }
            bool split = false, next = false;
            if( stage >= 0 )
            {
                const Fork f = world.fork( stage );
                world.generateTo( f.m_at + 200.0 );
                const Segment &mid = world.segment( static_cast<long>( ( f.m_at + 100.0 ) / SegmentLength ) );
                split = ( mid.m_flags & SEG_MEDIAN ) && mid.m_lanes >= 4 && f.m_left != f.m_right;
                world.choose( stage, 1 );
                const int after = biomeDef( world.stageBiome( stage + 1 ) ).m_underwater ? stage + 2 : stage + 1;
                next = world.stageBiome( after ) == f.m_right && world.fork( stage ).m_chosen == 1;
                std::string a, b;
                forkNames( f.m_left, f.m_right, world.stageBiome( stage ), a, b );
                next = next && !a.empty() && a != b;
            }
            // Every fork's two signs read differently.
            const auto &route = worldRoute();
            int         named = 0, total = 0;
            for( int pos = 0; pos < static_cast<int>( route.size() ); ++pos )
            {
                const RouteFork rf = routeFork( pos );
                if( rf.m_other < 0 ) continue;
                std::string a, b;
                forkNames( route[static_cast<size_t>( rf.m_main )], route[static_cast<size_t>( rf.m_other )], route[static_cast<size_t>( pos )], a, b );
                ++total;
                named += !a.empty() && a != b ? 1 : 0;
            }
            std::printf( "      %d of %d forks round the world signed apart\n", named, total );
            split = split && named == total;
            std::printf( "      %d forks in the first 40 stages from the white cliffs\n", forks );
            check( forks >= 3 && split && next, "forks at the borders" );
        }
        { // Between landmasses the tour drives a stage on the sea floor: the white cliffs to Brittany under the Channel.
            const auto &route = worldRoute();
            int         crossings = 0, seas = 0;
            for( size_t i = 0; i < route.size(); ++i )
            {
                const int sea = seaCrossing( route[i], route[( i + 1 ) % route.size()] );
                crossings += sea >= 0 ? 1 : 0;
                seas += sea >= 0 && biomeDef( static_cast<BiomeE>( sea ) ).m_underwater ? 1 : 0;
            }
            TrackParams tour;
            tour.m_biome = SceneryWorld;
            Track       world( 99, tour, static_cast<int>( BiomeE::CLIFFS ) );
            const bool  channel = biomeDef( world.stageBiome( 1 ) ).m_underwater && world.stageBiome( 2 ) == route[static_cast<size_t>( routeIndex( BiomeE::CLIFFS ) + 1 )] &&
                                 !world.fork( 1 ).m_exists;
            std::printf( "      %d sea crossings round the world; stage 2 from the cliffs: %s\n", crossings, placeName( world.stageBiome( 1 ) ).c_str() );
            check( crossings >= 20 && seas == crossings && channel, "under the sea between landmasses" );
        }
        { // Some seas are crossed aboard: the Channel on a car train, a few straits on ferries.
            const auto &route = worldRoute();
            int         ferries = 0, trains = 0;
            for( size_t i = 0; i < route.size(); ++i )
            {
                const int sea = seaCrossing( route[i], route[( i + 1 ) % route.size()] );
                if( sea < 0 ) continue;
                const CrossingE how = biomeDef( static_cast<BiomeE>( sea ) ).m_crossing;
                ferries += how == CrossingE::FERRY ? 1 : 0;
                trains += how == CrossingE::TRAIN ? 1 : 0;
            }
            TrackParams tour;
            tour.m_biome = SceneryWorld;
            Track world( 99, tour, static_cast<int>( BiomeE::CLIFFS ) );
            world.generateTo( 2.0 * StageLength + 800.0 );
            const Crossing xc = world.crossingAt( StageLength - 100.0 );
            const Segment &deck = world.segment( static_cast<long>( ( xc.m_board + 20.0 ) / SegmentLength ) );
            std::printf( "      %d ferries and %d car trains round the world\n", ferries, trains );
            check( trains >= 1 && ferries >= 3 && xc.m_type == CrossingE::TRAIN && deck.m_lanes == 2 && std::abs( deck.m_curve ) < 1e-6f,
                   "ferries and car trains" );
        }
        std::printf( "      %d biomes, %d data errors\n", biomeCount(), dataErrors() );
        check( dataErrors() == 0 && biomeCount() >= static_cast<int>( BiomeE::COUNT ) && biomeId( "TUSCANY" ) == static_cast<int>( BiomeE::TUSCANY ) &&
                   biomeKey( BiomeE::PARIS ) == "PARIS",
               "the data files read cleanly" );
        int shapeErrors = 0;
        bool shapesBuilt = true;
        for( const DataShape &shape : dataShapes() )
        {
            MeshBuilder m;
            shapeErrors += buildDataShape( m, shape );
            shapesBuilt = shapesBuilt && m.triangleCount() > 0;
            if( m.triangleCount() == 0 ) std::printf( "      empty landmark %s\n", shape.m_name.c_str() );
        }
        std::printf( "      %d landmarks from the data, %d broken lines\n", static_cast<int>( dataShapes().size() ), shapeErrors );
        check( shapeErrors == 0 && shapesBuilt, "the landmarks of the data build" );
        { // Festivals on their dates: Diwali in India in early November, the Christmas markets in Germany, none in summer.
            const int diwali = festivalAt( static_cast<BiomeE>( biomeId( "AGRA" ) ), dateFromText( "11-02" ) );
            const int market = festivalAt( BiomeE::BAVARIA, dateFromText( "12-10" ) );
            const int wiesn = festivalAt( BiomeE::BAVARIA, dateFromText( "09-25" ) );
            const int none = festivalAt( BiomeE::BAVARIA, dateFromText( "07-10" ) );
            std::printf( "      %d festivals, %d broken lines\n", static_cast<int>( festivals().size() ), festivalErrors() );
            check( festivalErrors() == 0 && diwali >= 0 && festivals()[static_cast<size_t>( diwali )].m_name == "DIWALI" && market >= 0 &&
                       festivals()[static_cast<size_t>( market )].m_name == "CHRISTMAS MARKET" && wiesn >= 0 &&
                       festivals()[static_cast<size_t>( wiesn )].m_name == "OKTOBERFEST" && none < 0,
                   "festivals on their dates" );
        }
        { // The real climate: snow and bare trees in Bavaria in January but not in July, autumn and spring there, the seasons
          // of Patagonia turned round, none in Agra, the monsoon of Mumbai.
            auto look = []( const char *place, const char *day ) { return seasonAt( static_cast<BiomeE>( biomeId( place ) ), dateFromText( day ) ); };
            const SeasonLook bavJan = look( "BAVARIA", "01-15" ), bavJul = look( "BAVARIA", "07-15" ), bavOct = look( "BAVARIA", "10-20" );
            const SeasonLook bavApr = look( "BAVARIA", "04-25" ), patJan = look( "PATAGONIA", "01-15" ), patJul = look( "PATAGONIA", "07-15" );
            const SeasonLook agra = look( "AGRA", "01-15" );
            const PlaceClimate *mumbai = placeClimate( static_cast<BiomeE>( biomeId( "MUMBAI" ) ) );
            const float         monsoon = mumbai ? climatePrecipitation( *mumbai, dateFromText( "07-15" ) ) : 0.0f;
            const float         dry = mumbai ? climatePrecipitation( *mumbai, dateFromText( "01-15" ) ) : 1000.0f;
            std::printf( "      Bavaria January snow %.2f bare %.2f, July snow %.2f, October autumn %.2f, April spring %.2f; Patagonia January %.2f July %.2f; "
                         "Mumbai %.0f mm in July, %.0f in January; %d broken lines\n",
                         bavJan.m_winter, bavJan.m_bare, bavJul.m_winter, bavOct.m_autumn, bavApr.m_spring, patJan.m_winter, patJul.m_winter, monsoon, dry,
                         climateErrors() );
            check( climateErrors() == 0 && bavJan.m_winter > 0.7f && bavJan.m_bare > 0.7f && bavJul.m_winter == 0.0f && bavOct.m_autumn > 0.3f &&
                       bavApr.m_spring > 0.2f && patJan.m_winter == 0.0f && patJul.m_winter > 0.2f && agra.m_winter == 0.0f && agra.m_autumn == 0.0f &&
                       agra.m_spring == 0.0f && agra.m_bare == 0.0f && monsoon > 300.0f && dry < 5.0f,
                   "the seasons and the rain follow the real climate" );
        }
        { // The air of Delhi: thick in a winter night, thin on a monsoon afternoon; no change without a date or an hour.
            const BiomeE delhi = static_cast<BiomeE>( biomeId( "DELHI" ) );
            const PlaceSmog *air = placeSmog( delhi );
            const float      winter = smogAt( delhi, dateFromText( "12-15" ), 22.0f ), monsoon = smogAt( delhi, dateFromText( "08-15" ), 15.0f );
            const float      plain = smogAt( delhi, -1.0f, -1.0f ), base = smogOf( biomeDef( delhi ) );
            std::printf( "      Delhi smog: %.2f on a December night, %.2f on an August afternoon, %.2f without date and hour; %d broken lines\n", winter,
                         monsoon, plain, smogProfileErrors() );
            check( smogProfileErrors() == 0 && air && winter > monsoon + 0.3f && plain == base && smogAt( BiomeE::BAVARIA, 0.5f, 12.0f ) <= 1.0f,
                   "the smog follows the month and the hour" );
        }
        { // What OpenStreetMap shows: poles of overhead lines beside some country roads, more gravel and concrete where measured.
            int poles = 0, wood = 0, concrete = 0, steel = 0, gravelly = 0, slabs = 0;
            for( int i = 0; i < biomeCount(); ++i )
            {
                const BiomeDef &d = biomeDef( static_cast<BiomeE>( i ) );
                poles += d.m_poleKind > 0 ? 1 : 0;
                wood += d.m_poleKind == 1 ? 1 : 0;
                concrete += d.m_poleKind == 2 ? 1 : 0;
                steel += d.m_poleKind == 3 ? 1 : 0;
                gravelly += d.m_gravel >= 0.3f ? 1 : 0;
                slabs += d.m_concrete ? 1 : 0;
            }
            std::printf( "      roadside: %d places with poles (%d wood, %d concrete, %d steel), %d with gravel from 30 %%, %d on concrete; %d broken lines\n", poles, wood,
                         concrete, steel, gravelly, slabs, roadsideErrors() );
            check( roadsideErrors() == 0 && poles >= 20 && wood > 0 && concrete > 0 && steel > 0, "the roadside data reaches the places" );
        }
        { // Road customs: miles and boards in the USA, cameras in Germany, EU plates in France, toll plazas in Italy.
            const RoadCustoms &usa = roadCustoms( "USA" ), &germany = roadCustoms( "GERMANY" ), &france = roadCustoms( "FRANCE" );
            int tolls = 0, cameras = 0;
            for( uint64_t seed = 1; seed <= 8; ++seed )
            {
                TrackParams italy, bavaria;
                italy.m_biome = biomeId( "TUSCANY" );
                bavaria.m_biome = static_cast<int>( BiomeE::BAVARIA );
                Track a( seed, italy ), b( seed, bavaria );
                a.generateTo( 3000.0 );
                b.generateTo( 3000.0 );
                double s = 0.0;
                int    limit = 0;
                bool   mph = false;
                tolls += stageToll( a, 0, s ) ? 1 : 0;
                if( stageCamera( b, 0, s, limit, mph ) )
                {
                    ++cameras;
                    if( cameras == 1 ) std::printf( "      the first speed camera: drive %d, %.0f m, %d km/h\n", static_cast<int>( seed ), s, limit );
                }
            }
            std::printf( "      road customs: %d broken lines; of 8 drives %d with a toll plaza, %d with a speed camera\n", roadErrors(), tolls,
                         cameras );
            check( roadErrors() == 0 && tolls >= 3 && cameras >= 3 && usa.m_mph && usa.m_board && germany.m_cameras && !germany.m_mph && france.m_plate == PlateE::EU &&
                       roadCustoms( "ITALY" ).m_tolls && !roadCustoms( nullptr ).m_tolls && france.m_lamp == LampSodium &&
                       roadCustoms( "JAPAN" ).m_lamp == LampCool && germany.m_lamp == LampWarm && germany.m_post == PostBand && usa.m_post == PostFlex &&
                       roadCustoms( "AUSTRALIA" ).m_post == PostRedWhite && france.m_post == PostPlain,
                   "road customs by country" );
        }
        { // The roads of the countries: gravel and potholes in Chad, none in Switzerland; narrow lanes in Japan, concrete somewhere.
            auto placeOf = []( const char *country, bool rural )
            {
                for( int i = 0; i < biomeCount(); ++i )
                {
                    const BiomeDef &d = biomeDef( static_cast<BiomeE>( i ) );
                    if( d.m_country && std::strcmp( d.m_country, country ) == 0 && !d.m_unpaved && ( !rural || d.m_gravel > 0.0f ) ) return i;
                }
                return -1;
            };
            int concrete = 0, studs = 0;
            for( int i = 0; i < biomeCount(); ++i )
            {
                concrete += biomeDef( static_cast<BiomeE>( i ) ).m_concrete ? 1 : 0;
                studs += biomeDef( static_cast<BiomeE>( i ) ).m_studs ? 1 : 0;
            }
            auto drive = [&]( int place, float &gravel, int &holes )
            {
                gravel = 0.0f;
                holes = 0;
                if( place < 0 ) return;
                TrackParams p;
                p.m_biome = place;
                Track t( 7, p );
                t.generateTo( StageLength );
                const long n = static_cast<long>( StageLength / SegmentLength ) - 2;
                float      x = 0.0f, r = 0.0f;
                for( long j = 0; j < n; ++j )
                {
                    gravel += gravelAt( t, static_cast<double>( j ) * SegmentLength ) / static_cast<float>( n );
                    holes += potholeAt( t, j, x, r ) ? 1 : 0;
                }
            };
            const int chad = placeOf( "CHAD", true ), swiss = placeOf( "SWITZERLAND", false ), japan = placeOf( "JAPAN", false );
            float     chadGravel = 0.0f, swissGravel = 0.0f;
            int       chadHoles = 0, swissHoles = 0;
            drive( chad, chadGravel, chadHoles );
            drive( swiss, swissGravel, swissHoles );
            std::printf( "      Chad: %.0f %% gravel, %d potholes; Switzerland: %.0f %%, %d; %d places of concrete, %d with road studs\n",
                         chadGravel * 100.0f, chadHoles, swissGravel * 100.0f, swissHoles, concrete, studs );
            check( chad >= 0 && chadGravel > 0.2f && chadHoles > 0 && swiss >= 0 && swissGravel == 0.0f && swissHoles == 0 && japan >= 0 &&
                       std::abs( biomeDef( static_cast<BiomeE>( japan ) ).m_laneWidth - 3.0f ) < 0.01f && concrete > 10 && studs > 10 &&
                       countryInfo( "THAILAND" ).m_lines == 1 && countryInfo( "UAE" ).m_lines == 3,
                   "the roads of the countries" );
        }
        { // Spectacles: balloons over Cappadocia in most of its stages, the rules all known.
            TrackParams p;
            p.m_biome = biomeId( "CAPPADOCIA" );
            Track cappadocia( 17, p );
            int   balloons = 0;
            for( int st = 0; st < 20; ++st )
            {
                SkyE   kind = SkyE::COUNT;
                double at = 0.0;
                balloons += stageSpectacle( cappadocia, st, kind, at ) && kind == SkyE::BALLOONS ? 1 : 0;
            }
            std::printf( "      balloons over Cappadocia in %d of 20 stages; %d rules, %d errors\n", balloons, static_cast<int>( skyRules().size() ), skyErrors() );
            check( balloons >= 12 && skyErrors() == 0 && skyRules().size() == 5, "spectacles over the road" );
        }
        { // Fast trains: half of Japan's stages; flat out the car beats the train, dawdling it loses.
            TrackParams p;
            p.m_biome = biomeId( "TOKYO" );
            Track tokyo( 13, p );
            int   races = 0;
            for( int st = 0; st < 20; ++st )
            {
                TrainStretch ts;
                races += stageTrain( tokyo, st, ts ) ? 1 : 0;
            }
            auto race = [&]( float pace, bool &won, bool &lost )
            {
                won = lost = false;
                TrainStretch t;
                int          st = 0;
                while( st < 20 && !stageTrain( tokyo, st, t ) ) ++st;
                PlayerCar car;
                car.reset( carSpec( 0 ), StartLine, 0.0f, tokyo );
                car.m_s = t.m_from - 240.0;
                TrainRace train;
                for( int k = 0; k < 60 * 300 && !won && !lost; ++k )
                {
                    tokyo.generateTo( car.m_s + 2000.0 );
                    car.m_speed = train.m_running ? pace : 45.0f;
                    TrainEvents ev;
                    train.update( 1.0f / 60.0f, tokyo, car, true, ev );
                    won = won || ev.m_won;
                    lost = lost || ev.m_lost;
                    car.m_s += static_cast<double>( car.m_speed ) / 60.0;
                }
            };
            bool fastWon = false, fastLost = false, slowWon = false, slowLost = false;
            race( carSpec( 0 ).m_topSpeed, fastWon, fastLost );
            race( 40.0f, slowWon, slowLost );
            std::printf( "      %d train races in 20 stages of Tokyo; flat out won %d, at 144 km/h lost %d; %d trains, %d errors\n", races, fastWon, slowLost,
                         static_cast<int>( trainLines().size() ), trainErrors() );
            check( races >= 4 && races <= 16 && fastWon && !fastLost && slowLost && !slowWon && trainErrors() == 0 && trainLines().size() >= 15,
                   "racing the train" );
        }
        { // The local hero: one stage in three; a fast car beats them to the checkpoint, a dawdling one loses.
            TrackParams world;
            world.m_biome = SceneryWorld;
            Track tour( 9, world );
            int   duels = 0;
            for( int st = 0; st < 60; ++st )
            {
                double cp = 0.0;
                tour.generateTo( static_cast<double>( st + 1 ) * StageLength + 3000.0 );
                duels += stageHero( tour, st, cp ) ? 1 : 0;
            }
            auto duel = [&]( float pace, bool &won, bool &lost )
            {
                won = lost = false;
                PlayerCar car;
                car.reset( carSpec( 0 ), StartLine, 0.0f, tour );
                int    st = 0;
                double cp = 0.0;
                while( st < 60 && !stageHero( tour, st, cp ) ) ++st;
                car.m_s = cp - 650.0;
                Traffic   none;
                LocalHero hero;
                for( int k = 0; k < 60 * 400 && !won && !lost; ++k )
                {
                    tour.generateTo( car.m_s + 3000.0 );
                    car.m_speed = hero.m_state == HeroStateE::DUEL ? pace : 40.0f;
                    HeroEvents ev;
                    hero.update( 1.0f / 60.0f, tour, car, none, true, ev );
                    won = won || ev.m_won;
                    lost = lost || ev.m_lost;
                    car.m_s += static_cast<double>( car.m_speed ) / 60.0;
                }
            };
            bool fastWon = false, fastLost = false, slowWon = false, slowLost = false;
            duel( carSpec( 0 ).m_topSpeed, fastWon, fastLost );
            duel( 25.0f, slowWon, slowLost );
            const Phrases &jp = phrasesOf( "JAPAN" );
            std::printf( "      %d duels in 60 stages; flat out won %d, at 90 km/h lost %d; Japan: %s / %s; %d phrase errors\n", duels, fastWon, slowLost,
                         jp.m_challenge.c_str(), jp.m_taunt.c_str(), phraseErrors() );
            check( duels >= 10 && duels <= 32 && fastWon && !fastLost && slowLost && !slowWon && phraseErrors() == 0 && jp.m_taunt == "OSOI!",
                   "the local hero's duel" );
        }
        { // Achievements: none for a new driver, GETAWAY and MAKE WAY when earned, kept in the records file.
            Records r;
            const size_t fresh = earnAchievements( r ).size();
            r.m_escapes = 1;
            r.m_gaveWay = 10;
            r.m_escapedFrom = { "POLICE_CAR_USA" };
            const std::vector<size_t> now = earnAchievements( r );
            const std::string         path = ( std::filesystem::temp_directory_path() / "sunstreak-achievements-test.txt" ).string();
            r.save( path );
            Records back;
            back.load( path );
            std::filesystem::remove( path );
            const bool getaway = std::find( back.m_achieved.begin(), back.m_achieved.end(), "escape1" ) != back.m_achieved.end();
            const bool makeway = std::find( back.m_achieved.begin(), back.m_achieved.end(), "makeway" ) != back.m_achieved.end();
            std::printf( "      %d achievements; new driver %d, then %d earned; %d police liveries to escape\n", static_cast<int>( achievements().size() ),
                         static_cast<int>( fresh ), static_cast<int>( now.size() ), policeLiveries() );
            check( fresh == 0 && now.size() == 2 && getaway && makeway && back.m_escapes == 1 && back.m_gaveWay == 10 && back.m_escapedFrom.size() == 1 &&
                       achievementProgress( back, 13 ).m_need == policeLiveries() && earnAchievements( back ).empty(),
                   "achievements earned and kept" );
        }
        { // Hitchhikers: one stage in three; stopping beside one, they get in; at pace they get there, crawling they get out.
            TrackParams world;
            world.m_biome = SceneryWorld;
            Track tour( 9, world );
            int   waiting = 0;
            for( int st = 0; st < 60; ++st )
            {
                double s = 0.0;
                float  side = 0.0f;
                waiting += stageHitchhiker( tour, st, s, side ) ? 1 : 0;
            }
            auto ride = [&]( float pace, bool &picked, bool &delivered, bool &late )
            {
                picked = delivered = late = false;
                int st = 0;
                double spot = 0.0;
                float  side = 0.0f;
                while( st < 60 && !stageHitchhiker( tour, st, spot, side ) ) ++st;
                Passenger p;
                double    s = std::max( 50.0, spot - 800.0 );
                for( int k = 0; k < 60 * 900 && !delivered && !late; ++k )
                {
                    tour.generateTo( s + 2000.0 );
                    // Drive up, stop beside them, then on at the pace.
                    const bool  near = p.m_state == PassengerStateE::WAITING && std::abs( s - p.m_s ) < 30.0;
                    const float speed = p.m_state == PassengerStateE::ABOARD ? pace : near ? 5.0f : 40.0f;
                    PassengerEvents ev;
                    p.update( 1.0f / 60.0f, tour, s, p.m_state == PassengerStateE::WAITING ? p.m_x - p.m_side * 2.0f : 0.0f, speed, true, ev );
                    picked = picked || ev.m_pickedUp;
                    delivered = delivered || ev.m_delivered;
                    late = late || ev.m_late;
                    s += static_cast<double>( speed ) / 60.0;
                }
            };
            bool fastPicked = false, fastDelivered = false, fastLate = false, slowPicked = false, slowDelivered = false, slowLate = false;
            ride( 50.0f, fastPicked, fastDelivered, fastLate );
            ride( 12.0f, slowPicked, slowDelivered, slowLate );
            std::printf( "      %d hitchhikers in 60 stages; at 180 km/h delivered %d, at 43 km/h too late %d; %s / %s in Japan\n", waiting, fastDelivered, slowLate,
                         helloIn( "JAPAN" ), thanksIn( "JAPAN" ) );
            check( waiting >= 10 && waiting <= 32 && fastPicked && fastDelivered && !fastLate && slowPicked && slowLate && !slowDelivered &&
                       std::string( helloIn( "JAPAN" ) ) == "KONNICHIWA" && std::string( thanksIn( "PERU" ) ) == "GRACIAS",
                   "hitchhikers: picked up, delivered or too late" );
        }
        { // Rain: puddles gather, and at speed the tyres float in them; slower, or in the dry, they do not.
            Track road( 21 );
            road.generateTo( 6000.0 );
            int   puddles = 0;
            float x = 0.0f;
            for( long j = 2; j < 1000; ++j ) puddles += rainPuddleAt( road, j, x ) ? 1 : 0;
            auto drive = [&]( float rain, float speed )
            {
                PlayerCar car;
                car.reset( carSpec( 0 ), StartLine, 0.0f, road );
                car.m_rain = rain;
                CarInput in;
                in.m_throttle = 1.0f;
                bool floated = false;
                for( long j = 40; j < 1000 && !floated; ++j )
                {
                    if( !rainPuddleAt( road, j, x ) ) continue;
                    car.m_s = ( static_cast<double>( j ) + 0.5 ) * SegmentLength;
                    car.m_x = x;
                    car.m_speed = speed;
                    car.m_aquaplane = 0.0f;
                    car.update( in, road, 1.0f / 120.0f, false );
                    floated = car.m_eventAquaplane && car.m_wetGrip < 0.2f;
                }
                return floated;
            };
            const bool fast = drive( 1.0f, 40.0f ), slow = drive( 1.0f, 20.0f ), dry = drive( 0.0f, 40.0f );
            std::printf( "      %d puddles in 5 km of rain; at 144 km/h aquaplaning %d, at 72 km/h %d, in the dry %d\n", puddles, fast, slow, dry );
            check( puddles >= 6 && puddles <= 30 && fast && !slow && !dry, "puddles in the rain and aquaplaning" );
        }
        { // Dirty air: Delhi's own index, Helsinki's, the countries' and the estimated ones; smog and litter where it is high.
            auto place = [&]( const char *id ) -> const BiomeDef & { return biomeDef( static_cast<BiomeE>( std::max( 0, biomeId( id ) ) ) ); };
            auto litter = [&]( const BiomeDef &d )
            {
                int n = 0;
                for( const PropRule &r : d.m_props ) n += r.m_soft ? 1 : 0;
                return n;
            };
            const BiomeDef &delhi = place( "DELHI" ), &helsinki = place( "HELSINKI" ), &rajasthan = place( "RAJASTHAN" );
            int             known = 0, countries = 0;
            for( int c = 0; c < countryTableSize(); ++c )
            {
                ++countries;
                known += roadCustoms( countryAt( c ).m_name ).m_pollution >= 0.0f ? 1 : 0;
            }
            Environment clean{};
            clean.m_fogDensity = 0.001f;
            clean.m_fogColor = clean.m_skyHorizon = Color{ 200, 210, 230, 255 };
            clean.m_saturation = 1.0f;
            const Environment dirty = withSmog( clean, smogOf( delhi ) );
            std::printf( "      pollution: Delhi %.0f, Helsinki %.0f, Rajasthan %.0f; %d of %d countries; litter rules %d / %d; haze %.4f\n", delhi.m_pollution,
                         helsinki.m_pollution, rajasthan.m_pollution, known, countries, litter( delhi ), litter( helsinki ), dirty.m_fogDensity );
            check( std::abs( delhi.m_pollution - 90.0f ) < 1.0f && helsinki.m_pollution < 15.0f && std::abs( rajasthan.m_pollution - 73.0f ) < 3.0f &&
                       known == countries && litter( delhi ) >= 6 && litter( helsinki ) == 0 && smogOf( helsinki ) == 0.0f &&
                       dirty.m_fogDensity > clean.m_fogDensity * 1.5f && dirty.m_fogColor.b < dirty.m_fogColor.r && dirty.m_saturation < 0.9f,
                   "dirty air: haze and litter by Numbeo's pollution index" );
        }
        { // The police: traps in some stages; too fast past one, a chase that ends caught when the car stops, escaped when it runs.
            Models models;   // no meshes: only which patrol car a place has
            models.m_policeModel.assign( dataShapes().size(), -1 );
            int usaShape = -1, hkShape = -1, policeCars = 0;
            for( size_t i = 0; i < dataShapes().size(); ++i )
            {
                if( dataShapes()[i].m_name == "POLICE_CAR_USA" ) usaShape = static_cast<int>( i );
                if( dataShapes()[i].m_name == "POLICE_CAR_HONG_KONG" ) hkShape = static_cast<int>( i );
                policeCars += dataShapes()[i].m_name.rfind( "POLICE_CAR", 0 ) == 0 ? 1 : 0;
                if( dataShapes()[i].m_name.rfind( "POLICE_CAR", 0 ) == 0 ) models.m_policeModel[i] = static_cast<int>( i );
            }
            const int  route66 = biomeId( "ROUTE_66" );
            const int  hongKong = biomeId( "HONG_KONG" );
            const bool usa = route66 >= 0 && policeModel( models, biomeDef( static_cast<BiomeE>( route66 ) ) ) == usaShape;
            const bool hk = hongKong >= 0 && policeModel( models, biomeDef( static_cast<BiomeE>( hongKong ) ) ) == hkShape;
            TrackParams world;
            world.m_biome = SceneryWorld;
            Track tour( 3, world );
            int   traps = 0;
            for( int st = 0; st < 100; ++st ) traps += policeBase( tour, st ) >= 0.0 ? 1 : 0;
            auto chase = [&]( float after, bool &chased, bool &caught, bool &escaped )
            {
                chased = caught = escaped = false;
                TrackParams p;
                p.m_biome = route66;
                Track   t( 11, p );
                Traffic none;
                int     st = 0;
                while( st < 60 && policeBase( t, st ) < 0.0 ) ++st;
                double s = std::max( 50.0, policeBase( t, st ) - 1200.0 );
                Police police;
                for( int k = 0; k < 60 * 240 && !caught && !escaped; ++k )
                {
                    const float  speed = police.m_state == PoliceStateE::CHASE && police.m_chaseTime > 3.0f ? after : 50.0f;
                    PoliceEvents ev;
                    t.generateTo( s + 2000.0 );
                    police.update( 1.0f / 60.0f, t, s, 1.5f, speed, 55.0f, none, models, true, ev );
                    chased = chased || ev.m_chase;
                    caught = caught || ev.m_caught;
                    escaped = escaped || ev.m_escaped;
                    s += static_cast<double>( speed ) / 60.0;
                }
            };
            bool runChased = false, runCaught = false, runEscaped = false, stopChased = false, stopCaught = false, stopEscaped = false;
            chase( 55.0f, runChased, runCaught, runEscaped );
            chase( 2.0f, stopChased, stopCaught, stopEscaped );
            std::printf( "      %d police cars; %d traps in 100 stages; flat out: chased %d, escaped %d; stopping: caught %d\n", policeCars, traps, runChased, runEscaped, stopCaught );
            check( usa && hk && policeCars >= 40 && traps >= 4 && traps <= 16 && runChased && runEscaped && !runCaught && stopChased && stopCaught && !stopEscaped,
                   "the police of the world tour" );
        }
        { // An ambulance or a fire engine on its run: from behind past a slow car, the traffic pulling over for it.
            Models       models;   // no meshes: sizes only, an everyday car and lorry at the end
            const size_t shapes = dataShapes().size();
            models.m_traffic.resize( shapes + 2 );
            models.m_traffic[shapes + 1].m_truck = true;
            models.m_policeModel.assign( shapes, -1 );
            int services = 0;
            for( size_t i = 0; i < shapes; ++i )
            {
                const std::string &name = dataShapes()[i].m_name;
                const bool service = name.rfind( "AMBULANCE", 0 ) == 0 || name.rfind( "FIRE_ENGINE", 0 ) == 0 || name.rfind( "POLICE_CAR", 0 ) == 0;
                models.m_traffic[i].m_animal = service ? PropKindE::COUNT : PropKindE::LANDMARK;   // the rest never drives
                models.m_traffic[i].m_police = service;
                models.m_traffic[i].m_length = 5.2f;
                models.m_traffic[i].m_width = 2.0f;
                if( service ) models.m_policeModel[i] = static_cast<int>( i );
                services += name.rfind( "AMBULANCE", 0 ) == 0 || name.rfind( "FIRE_ENGINE", 0 ) == 0 ? 1 : 0;
            }
            TrackParams p;
            p.m_biome = biomeId( "ROUTE_66" );
            Track     road( 5, p );
            PlayerCar car;
            car.reset( carSpec( 0 ), StartLine, 0.0f, road );
            Traffic traffic;
            traffic.reset( 77, car.m_s );
            std::string    who;
            int            gaveWay = 0, yielded = 0, after = 0;
            for( int k = 0; k < 60 * 400 && after < 60 * 40; ++k )
            {
                after += gaveWay > 0 ? 1 : 0;   // then on for 40 s more, the siren ahead among the traffic
                road.generateTo( car.m_s + 2000.0 );
                const Segment &seg = road.segment( static_cast<long>( car.m_s / SegmentLength ) );
                car.m_speed = 20.0f;
                car.m_x = laneCenter( seg.m_lanes - 1, seg.m_lanes, seg.m_halfWidth ) + 0.4f;
                TrafficEvents ev;
                traffic.update( 1.0f / 60.0f, road, car, models, ev );
                car.m_s += 20.0 / 60.0;
                gaveWay += ev.m_gaveWay;
                for( const TrafficCar &c : traffic.m_cars )
                {
                    if( c.m_siren && who.empty() ) who = dataShapes()[static_cast<size_t>( c.m_model )].m_name;
                    yielded += c.m_yield > 1.9f ? 1 : 0;
                }
            }
            std::printf( "      %d ambulances and fire engines; on Route 66 %s came by, %d frames of cars pulling over\n", services, who.c_str(), yielded );
            check( services >= 40 && ( who == "AMBULANCE_USA" || who == "FIRE_ENGINE_USA" ) && gaveWay == 1 && yielded > 0,
                   "ambulances and fire engines on their runs" );
        }
        { // Journeys: every one from a place of the route to another, London to Paris a short one.
            int  shortest = 1 << 30, longest = 0;
            bool known = !journeys().empty();
            for( const Journey &j : journeys() )
            {
                const int n = routeStages( j.m_from, j.m_to );
                known = known && n > 0;
                shortest = std::min( shortest, n );
                longest = std::max( longest, n );
            }
            std::printf( "      %d journeys, %d to %d stages on the main way\n", static_cast<int>( journeys().size() ), shortest, longest );
            check( known && journeys().size() >= 8 && journeys()[0].m_from == static_cast<BiomeE>( biomeId( "LONDON" ) ) && shortest >= 3 &&
                       routeStages( journeys()[0].m_from, journeys()[0].m_to ) < 12,
                   "journeys from place to place" );
        }
        { // F5: the files read again give the same places, the same route and the same shapes.
            const int         places = biomeCount(), shapes = static_cast<int>( dataShapes().size() ), route = static_cast<int>( worldRoute().size() );
            const std::string from = reloadData( "" );
            resetDataErrors();
            std::printf( "      read again from %s\n", from.empty() ? "the built-in copies" : from.c_str() );
            check( dataErrors() == 0 && biomeCount() == places && static_cast<int>( dataShapes().size() ) == shapes &&
                       static_cast<int>( worldRoute().size() ) == route && biomeId( "TUSCANY" ) == static_cast<int>( BiomeE::TUSCANY ),
                   "the data files read again" );
        }
    }
    check( trackTitle( 42 ) == trackTitle( 42 ) && trackTitle( 42 ).size() > 6, "track titles" );
    { // Every place that writes in its own script has its name traced for the signs (tools/make_signs.py).
        int locals = 0, traced = 0;
        for( int b = 0; b < biomeCount(); ++b )
            if( biomeDef( static_cast<BiomeE>( b ) ).m_local )
            {
                ++locals;
                traced += scriptName( static_cast<BiomeE>( b ) ) != nullptr;
            }
        std::printf( "      %d places in their own script, %d traced\n", locals, traced );
        check( locals > 200 && traced == locals && scriptNameErrors() == 0, "place names in their own script" );
    }

    std::printf( "%d failure(s)\n", g_failures );
    return g_failures;
}
