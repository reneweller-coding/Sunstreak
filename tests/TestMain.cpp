/**
 * @file TestMain.cpp
 * @brief Core tests without a window: the generator's determinism and bounds, the stage order, the car's physics,
 *        the track code.
 * @author reneweller
 */
#include "Car.h"
#include "Game.h"
#include "Track.h"

#include <cstdio>
#include <cmath>

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
        check( jumps > 5 && works > 5, "crests and roadworks appear" );
    }
    { // Stages never repeat a biome back to back.
        Track t( 99 );
        bool  ok = true;
        for( int s = 1; s < 64; ++s ) ok = ok && t.stageBiome( s ) != t.stageBiome( s - 1 );
        check( ok, "no biome twice in a row" );
        Track forced( 99, static_cast<int>( BiomeE::NEON ) );
        check( forced.stageBiome( 0 ) == BiomeE::NEON, "a requested first biome" );
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
    check( trackCode( 0x123456789Aull ).size() == 9 && trackCode( 0x123456789Aull )[4] == '-', "track code format" );

    std::printf( "%d failure(s)\n", g_failures );
    return g_failures;
}
