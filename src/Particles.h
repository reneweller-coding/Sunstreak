/**
 * @file Particles.h
 * @brief Short-lived sprites: tyre smoke, dust, sparks, splashes, exhaust flames.
 * @author reneweller
 */
#pragma once

#include "MathUtil.h"
#include "Rng.h"

#include <vector>

namespace sunstreak
{

/** @brief One sprite. */
struct Particle
{
    Vec3d   m_pos;              ///< world position
    Vector3 m_vel{};            ///< world velocity
    float   m_life = 0;         ///< seconds lived
    float   m_maxLife = 1;      ///< seconds to live
    float   m_size = 0.5f;      ///< radius at birth
    float   m_grow = 0;         ///< radius growth per second
    Color   m_color{};          ///< colour (alpha fades)
    float   m_glow = 0;         ///< brightness multiplier above 1 for bloom
    float   m_gravity = 0;      ///< m/s^2 downwards
    float   m_drag = 0;         ///< velocity damping per second
    bool    m_streak = false;   ///< drawn stretched along its velocity
};

/** @brief The particle pool. */
class Particles
{
public:
    /**
     * @brief Adds a particle (the oldest is dropped when full).
     * @param[in] p  the particle
     */
    void emit( const Particle &p );

    /**
     * @brief Moves and ages the particles.
     * @param[in] dt  seconds
     */
    void update( float dt );

    /** @brief Drops all particles. */
    void clear() { m_items.clear(); }

    /**
     * @brief The live particles.
     * @return the particles
     */
    const std::vector<Particle> &items() const { return m_items; }

    Rng m_rng{ 4242 };   ///< for emitters' jitter

private:
    std::vector<Particle> m_items;   ///< live particles
};

} // namespace sunstreak
