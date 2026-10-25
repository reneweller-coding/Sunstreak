/**
 * @file Passenger.cpp
 * @brief The hitchhikers: where they wait, when they get in, when they get out, and their words.
 * @author Rene Weller
 */
#include "Passenger.h"

#include "Local.h"
#include "Phrases.h"
#include "Rng.h"

#include <cmath>

namespace sunstreak
{

bool stageHitchhiker( const Track &track, int stage, double &s, float &side )
{
    const BiomeDef &place = biomeDef( track.stageBiome( stage ) );
    if ( stage < 0 || place.m_underwater || place.m_surface.m_lineGlow > 0.0f ) return false;
    const uint64_t h = hash64( track.seed() * 2029 + static_cast<uint64_t>( stage ) * 13 + 3 );
    if ( h % 3 != 0 ) return false;   // one stage in three
    side = place.m_country && countryInfo( place.m_country ).m_left ? -1.0f : 1.0f;
    const double base =
        static_cast<double>( stage ) * StageLength + HitchAt + static_cast<double>( ( h >> 8 ) % 10000 );
    for ( int k = 0; k < 8; ++k )
    {
        s = base + 120.0 * k;
        bool clear = track.crossingAt( s ).m_type == CrossingE::NONE && !track.inForkZone( s, 150.0 );
        for ( double d = -60.0; d <= 60.0 && clear; d += SegmentLength )
            clear = !( track.segment( static_cast<long>( ( s + d ) / SegmentLength ) ).m_flags &
                       ( SEG_TUNNEL | SEG_BRIDGE | SEG_JUMP | SEG_ROADWORKS ) );
        if ( clear ) return true;
    }
    return false;
}

const char *helloIn( const char *country )
{
    return phrasesOf( country ).m_hello.c_str();
}

const char *thanksIn( const char *country )
{
    return phrasesOf( country ).m_thanks.c_str();
}

void Passenger::reset()
{
    *this = Passenger{};
}

bool Passenger::breakCargo()
{
    if ( m_state != PassengerStateE::ABOARD || m_job != JobE::FRAGILE ) return false;
    m_state = PassengerStateE::NONE;
    return true;
}

void Passenger::update( float dt, const Track &track, double s, float x, float speed, bool enabled,
                        PassengerEvents &ev )
{
    ev = PassengerEvents{};
    switch ( m_state )
    {
    case PassengerStateE::NONE:
    {
        if ( !enabled ) return;
        // This stage's or the next one's, once the road there is laid and the spot is still ahead.
        const int stage = static_cast<int>( s / StageLength );
        for ( int st = stage; st <= stage + 1; ++st )
        {
            double hs = 0.0;
            float  side = 1.0f;
            if ( st <= m_lastStage || !stageHitchhiker( track, st, hs, side ) || hs < s + 150.0 || hs > s + 900.0 )
                continue;
            const Segment &seg = track.segment( static_cast<long>( hs / SegmentLength ) );
            const uint64_t h = hash64( track.seed() * 31 + static_cast<uint64_t>( st ) );
            m_lastStage = st;
            m_state = PassengerStateE::WAITING;
            m_s = hs;
            m_side = side;
            m_x = side * ( seg.m_halfWidth + RumbleWidth + 1.2f );
            m_look = static_cast<int>( h % 1000 );
            // Mostly hitchhikers; now and then a courier, a paramedic or a police officer.
            const int job = static_cast<int>( ( h >> 20 ) % 20 );
            m_job = job < 11 ? JobE::HITCHHIKER : job < 14 ? JobE::FRAGILE : job < 17 ? JobE::ORGAN : JobE::THIEF;
            const BiomeDef &place = biomeDef( track.stageBiome( st ) );
            m_from = place.m_country ? place.m_country : "";
            break;
        }
        return;
    }
    case PassengerStateE::WAITING:
    {
        if ( s > m_s + 60.0 || !enabled )
        {
            m_state = PassengerStateE::NONE;   // passed by
            return;
        }
        if ( std::abs( s - m_s ) < 9.0 && std::abs( x - m_x ) < 5.0f && speed < HitchStop )
        {
            // The organ and the thief go to the next checkpoint at least 1.5 km on, the rest to the next place.
            const double checkpoint = std::ceil( ( s + 1500.0 ) / CheckpointSpacing ) * CheckpointSpacing;
            if ( m_job == JobE::THIEF )
            {
                m_destS = checkpoint;
                m_state = PassengerStateE::NONE;
                ev.m_thief = true;
                return;
            }
            if ( m_job == JobE::ORGAN )
            {
                m_destS = checkpoint;
                m_to = track.stageBiome( static_cast<int>( checkpoint / StageLength ) );
                m_timeLeft = static_cast<float>( ( m_destS - s ) / OrganPace ) + 6.0f;
            }
            else
            {
                const int next = static_cast<int>( m_s / StageLength ) + 1;
                m_destS = static_cast<double>( next ) * StageLength + 800.0;
                m_to = track.stageBiome( next );
                m_timeLeft = static_cast<float>( ( m_destS - s ) / HitchPace ) + HitchSlack;
            }
            m_state = PassengerStateE::ABOARD;
            ev.m_pickedUp = true;
        }
        return;
    }
    case PassengerStateE::ABOARD:
    {
        if ( s >= m_destS )
        {
            m_state = PassengerStateE::NONE;
            ev.m_delivered = true;
            return;
        }
        m_timeLeft -= dt;
        if ( m_timeLeft <= 0.0f || !enabled )
        {
            m_timeLeft = 0.0f;
            m_state = PassengerStateE::NONE;
            ev.m_late = true;
        }
        return;
    }
    }
}

}   // namespace sunstreak
