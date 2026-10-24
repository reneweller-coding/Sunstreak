/**
 * @file TestMain.cpp
 * @brief Core tests without a window: the generator's determinism and bounds, the stage order, the car's physics,
 *        the track code.
 * @author reneweller
 */
#include "Car.h"
#include "Game.h"
#include "Race.h"
#include "Records.h"
#include "Track.h"

#include <cstdio>
#include <cmath>
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
        check( jumps > 2 && works > 5, "crests and roadworks appear" );
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
        for( int s = 1; s < static_cast<int>( route.size() ) + 3; ++s )
            follows = follows && tour.stageBiome( s ) == route[static_cast<size_t>( first + s ) % route.size()];
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
        std::printf( "      %d biomes, %d data errors\n", biomeCount(), dataErrors() );
        check( dataErrors() == 0 && biomeCount() >= static_cast<int>( BiomeE::COUNT ) && biomeId( "TUSCANY" ) == static_cast<int>( BiomeE::TUSCANY ) &&
                   biomeKey( BiomeE::PARIS ) == "PARIS",
               "the data files read cleanly" );
    }
    check( trackTitle( 42 ) == trackTitle( 42 ) && trackTitle( 42 ).size() > 6, "track titles" );

    std::printf( "%d failure(s)\n", g_failures );
    return g_failures;
}
