/**
 * @file Police.cpp
 * @brief The patrol car of the world tour: where it waits, when it gives chase, how it drives and when it gives up.
 * @author reneweller
 */
#include "Police.h"

#include "Data.h"
#include "Local.h"
#include "Models.h"
#include "Traffic.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace sunstreak
{

int policeModel( const Models &models, const BiomeDef &place )
{
    return serviceModel( models, place, "POLICE_CAR" );
}

void Police::reset()
{
    *this = Police{};
}

void Police::update( float dt, const Track &track, double s, float x, float speed, float top, const Traffic &traffic, const Models &models,
                     bool enabled, PoliceEvents &ev )
{
    ev = PoliceEvents{};
    const double before = m_lastS;
    m_lastS = s;
    switch( m_state )
    {
    case PoliceStateE::NONE:
    {
        if( !enabled ) return;
        // The trap of this stage or the next, once the road there is laid.
        const int stage = static_cast<int>( s / StageLength );
        for( int st = stage; st <= stage + 1; ++st )
        {
            if( st <= m_doneStage ) continue;
            const double base = policeBase( track, st );
            if( base < 0.0 || s < base - PoliceSpawn ) continue;
            double at = 0.0;
            float  side = 1.0f;
            const BiomeDef &place = biomeDef( track.stageBiome( st ) );
            const int       model = policeModel( models, place );
            m_doneStage = st;
            if( model < 0 || !stagePolice( track, st, at, side ) || at < s + 150.0 ) continue;
            const Segment &seg = track.segment( static_cast<long>( at / SegmentLength ) );
            m_state = PoliceStateE::PARKED;
            m_trapStage = st;
            m_trapS = at;
            m_s = at;
            m_x = side * ( seg.m_halfWidth + 0.6f );   // half on the kerb: the verge beyond may rise
            m_speed = 0.0f;
            m_yaw = 0.0f;
            m_model = model;
            m_chaseTime = 0.0f;
            m_beside = 0.0f;
            m_siren = place.m_country && countryInfo( place.m_country ).m_fleet == FleetE::EUROPE ? 1 : 0;
            return;
        }
        return;
    }
    case PoliceStateE::PARKED:
    {
        // Past it: too fast and it sets off (the same allowance as a speed camera's).
        if( enabled && before < m_trapS && s >= m_trapS )
        {
            const BiomeDef &place = biomeDef( track.stageBiome( m_trapStage ) );
            const bool      mph = roadCustoms( place.m_country ).m_mph;
            const int       limit = speedLimit( track.segment( static_cast<long>( m_trapS / SegmentLength ) ), mph );
            const float     allowed = static_cast<float>( limit ) * ( mph ? 0.44704f : 1.0f / 3.6f ) * 1.1f;
            if( speed > allowed )
            {
                m_state = PoliceStateE::CHASE;
                ev.m_chase = true;
                return;
            }
        }
        if( s > m_trapS + 400.0 || !enabled ) m_state = PoliceStateE::NONE;
        return;
    }
    case PoliceStateE::CHASE:
    {
        m_chaseTime += dt;
        const double gap = s - m_s;
        if( !enabled || gap > PoliceEscape || m_chaseTime > PoliceGiveUp )
        {
            ev.m_escaped = enabled;
            m_state = PoliceStateE::STOPPED;
            return;
        }
        // Flat out, a little slower than the car can go; close behind, level with it.
        float want = top * PolicePace;
        if( gap < 30.0 ) want = std::max( 0.0f, speed + std::clamp( static_cast<float>( gap ) * 0.35f, -6.0f, 6.0f ) );
        const float accel = m_speed < want ? 9.0f : 14.0f;
        m_speed += std::clamp( want - m_speed, -accel * dt, accel * dt );
        m_s += static_cast<double>( m_speed * dt );

        // Behind the car in its lane, weaving round the traffic; close up, beside it on the side towards the middle.
        const Segment &seg = track.segment( static_cast<long>( m_s / SegmentLength ) );
        const float    edge = std::max( 1.0f, seg.m_halfWidth - 1.0f );
        float          tx = gap < 20.0 ? x + ( x > 0.0f ? -2.9f : 2.9f ) : x;
        for( const TrafficCar &c : traffic.m_cars )
        {
            const double ds = c.m_s - m_s;
            if( c.m_animal || c.m_gone || ds < -2.0 || ds > 28.0 || std::abs( c.m_x - tx ) > 2.4f ) continue;
            tx = c.m_x > 0.0f ? c.m_x - 3.4f : c.m_x + 3.4f;
        }
        tx = std::clamp( tx, -edge, edge );
        const float vx = std::clamp( ( tx - m_x ) * 1.6f, -5.5f, 5.5f );
        m_x += vx * dt;
        m_yaw = std::atan2( vx, std::max( m_speed, 2.0f ) );

        // Caught: level with the car for a moment, or the car stands still with the police right behind.
        const bool level = std::abs( gap ) < 4.5 && std::abs( m_x - x ) < 3.8f;
        const bool still = speed < 3.0f && gap < 25.0;
        m_beside = level || still ? m_beside + dt : std::max( 0.0f, m_beside - dt );
        if( m_beside > 1.2f )
        {
            ev.m_caught = true;
            m_state = PoliceStateE::STOPPED;
        }
        return;
    }
    case PoliceStateE::STOPPED:
    {
        // Pulls over to the verge and is left behind.
        m_speed = std::max( 0.0f, m_speed - 10.0f * dt );
        m_s += static_cast<double>( m_speed * dt );
        const Segment &seg = track.segment( static_cast<long>( m_s / SegmentLength ) );
        const float    verge = ( m_x < 0.0f ? -1.0f : 1.0f ) * ( seg.m_halfWidth + RumbleWidth + 1.0f );
        const float    vx = std::clamp( ( verge - m_x ) * 1.2f, -3.0f, 3.0f ) * std::min( 1.0f, m_speed / 5.0f );
        m_x += vx * dt;
        m_yaw = std::atan2( vx, std::max( m_speed, 2.0f ) );
        if( std::abs( s - m_s ) > 700.0 ) m_state = PoliceStateE::NONE;
        return;
    }
    }
}

} // namespace sunstreak
