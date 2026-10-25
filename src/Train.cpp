/**
 * @file Train.cpp
 * @brief The trains of data/trains.txt, their stretches and their races.
 * @author Rene Weller
 */
#include "Train.h"

#include "Data.h"
#include "Rng.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace sunstreak
{

namespace
{

int g_trainErrors = 0;   ///< wrong lines in the last read

} // namespace

const std::vector<TrainLine> &trainLines()
{
    static DataCache<std::vector<TrainLine>> cache;
    return cache.get( []
    {
        g_trainErrors = 0;
        std::vector<TrainLine> out;
        for( const std::string &line : dataLines( dataText( "trains.txt" ) ) )
        {
            const std::vector<std::string> f = splitLine( line, '|' );
            if( f.size() != 4 || f[0].empty() )
            {
                ++g_trainErrors;
                continue;
            }
            TrainLine t;
            t.m_name = f[0];
            for( const std::string &c : splitLine( f[1], ',' ) )
            {
                if( c.empty() ) continue;
                if( !countryInfo( c.c_str() ).m_name || std::string( countryInfo( c.c_str() ).m_name ) != c ) ++g_trainErrors;
                t.m_countries.push_back( c );
            }
            t.m_speed = static_cast<float>( std::atof( f[2].c_str() ) ) / 3.6f;
            t.m_coaches = std::clamp( std::atoi( f[3].c_str() ), 1, 16 );
            if( t.m_speed <= 0.0f || t.m_countries.empty() ) ++g_trainErrors;
            out.push_back( std::move( t ) );
        }
        return out;
    } );
}

int trainErrors()
{
    trainLines();
    return g_trainErrors;
}

bool stageTrain( const Track &track, int stage, TrainStretch &out )
{
    if( stage < 0 ) return false;
    const BiomeDef &place = biomeDef( track.stageBiome( stage ) );
    if( !place.m_country || place.m_underwater || place.m_crossing != CrossingE::NONE || place.m_surface.m_lineGlow > 0.0f ) return false;
    if( mayClimbHairpins( place ) || place.m_humps > 0 ) return false;   // a viaduct beside would fold over the legs, or stay low beside a hump
    const auto &lines = trainLines();
    int         line = -1;
    for( size_t i = 0; i < lines.size() && line < 0; ++i )
        if( std::find( lines[i].m_countries.begin(), lines[i].m_countries.end(), place.m_country ) != lines[i].m_countries.end() ) line = static_cast<int>( i );
    if( line < 0 ) return false;
    const uint64_t h = hash64( track.seed() * 6151 + static_cast<uint64_t>( stage ) * 23 + 7 );
    if( h % 2 != 0 ) return false;   // one stage in two
    out.m_line = line;
    out.m_from = static_cast<double>( stage ) * StageLength + 3000.0 + static_cast<double>( ( h >> 8 ) % 2000 );
    out.m_to = out.m_from + 4000.0;
    // Away from the traffic of the slow side: the side of the oncoming lanes there would be, the left where they keep right.
    out.m_side = countryInfo( place.m_country ).m_left ? 1.0f : -1.0f;
    return true;
}

bool viaductAt( const Track &track, double s, float &side )
{
    TrainStretch t;
    if( !stageTrain( track, static_cast<int>( s / StageLength ), t ) || s < t.m_from - RailBefore || s >= t.m_to + RailAfter ) return false;
    side = t.m_side;
    return true;
}

Vec3d railPoint( const Track &track, double s, float side )
{
    const TrackFrame f = track.frameAt( s );
    const Segment   &seg = track.segment( static_cast<long>( s / SegmentLength ) );
    const float      edge = seg.m_halfWidth + RumbleWidth + ShoulderWidth;
    const float      l = side * ( edge + RailOffset );
    const float      banked = clampf( l, -seg.m_halfWidth - RumbleWidth, seg.m_halfWidth + RumbleWidth );
    Vec3d            p = track.pointAt( s, l, 0.0f );
    p.m_y += static_cast<double>( ( l - banked ) * std::tan( f.m_bank ) + RailHeight );
    return p;
}

void TrainRace::reset()
{
    *this = TrainRace{};
}

void TrainRace::update( float dt, const Track &track, const PlayerCar &player, bool enabled, TrainEvents &ev )
{
    ev = TrainEvents{};
    if( !m_running && !m_after )
    {
        if( !enabled ) return;
        // When the car comes to the viaduct, the train runs just ahead of it, as fast as the car is.
        const int stage = static_cast<int>( player.m_s / StageLength );
        if( stage <= m_lastStage || !stageTrain( track, stage, m_stretch ) ) return;
        if( player.m_s < m_stretch.m_from - 250.0 || player.m_s > m_stretch.m_from ) return;
        m_lastStage = stage;
        m_running = true;
        m_nose = player.m_s + 60.0;
        m_speed = std::max( player.m_speed, 20.0f );
        m_top = std::min( trainLines()[static_cast<size_t>( m_stretch.m_line )].m_speed, player.m_spec.m_topSpeed * TrainPace );
        ev.m_start = true;
    }
    m_speed = approach( m_speed, m_top, 0.35f, dt );
    m_nose += static_cast<double>( m_speed ) * dt;
    if( m_running )
    {
        // Whoever is first at the end of the stretch.
        const bool carFirst = player.m_s >= m_stretch.m_to && player.m_s > m_nose, trainFirst = m_nose >= m_stretch.m_to && m_nose >= player.m_s;
        if( carFirst || trainFirst )
        {
            m_running = false;
            m_after = true;
            ( carFirst ? ev.m_won : ev.m_lost ) = true;
        }
        if( !enabled && !carFirst && !trainFirst )
        {
            m_running = false;
            m_after = true;
        }
    }
    else if( m_after )
    {
        const double tail = m_nose - static_cast<double>( CoachLength ) * ( trainLines()[static_cast<size_t>( m_stretch.m_line )].m_coaches + 1 );
        if( tail > m_stretch.m_to + RailAfter - 50.0 || std::abs( m_nose - player.m_s ) > 1500.0 ) m_after = false;
    }
}

} // namespace sunstreak
