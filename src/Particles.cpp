/**
 * @file Particles.cpp
 * @brief The particle pool.
 * @author Rene Weller
 */
#include "Particles.h"

#include <algorithm>
#include <cstddef>

namespace sunstreak
{

namespace
{

constexpr size_t MaxParticles = 2500;   ///< pool size

}   // namespace

void Particles::emit( const Particle &p )
{
    if ( m_items.size() >= MaxParticles ) m_items.erase( m_items.begin() );
    m_items.push_back( p );
}

void Particles::update( float dt )
{
    for ( Particle &p : m_items )
    {
        p.m_life += dt;
        p.m_vel.y -= p.m_gravity * dt;
        const float damp = std::exp( -p.m_drag * dt );
        p.m_vel = Vector3Scale( p.m_vel, damp );
        p.m_pos.m_x += static_cast<double>( p.m_vel.x * dt );
        p.m_pos.m_y += static_cast<double>( p.m_vel.y * dt );
        p.m_pos.m_z += static_cast<double>( p.m_vel.z * dt );
        p.m_size += p.m_grow * dt;
    }
    std::erase_if( m_items, []( const Particle &p ) { return p.m_life >= p.m_maxLife; } );
}

}   // namespace sunstreak
