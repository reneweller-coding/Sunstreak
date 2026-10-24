/**
 * @file Traffic.h
 * @brief Everything else on the road: traffic to overtake, roadworks cones to dodge.
 *
 * Traffic drives in lanes at its own pace, changes lanes around slower cars and closed lanes, and is spawned ahead
 * beyond the fog. Passing a car counts as an overtake; passing it close and fast is a near miss and fills the boost
 * tank. Cones fly when hit.
 *
 * @author reneweller
 */
#pragma once

#include "Car.h"
#include "Models.h"
#include "Rng.h"
#include "Track.h"

#include <vector>

namespace sunstreak
{

/** @brief One traffic vehicle. */
struct TrafficCar
{
    double m_s = 0;            ///< distance along the road
    float  m_x = 0;            ///< lateral offset
    float  m_speed = 0;        ///< m/s
    float  m_cruise = 0;       ///< the speed it likes
    int    m_lane = 0;         ///< target lane
    int    m_model = 0;        ///< index into Models::m_traffic
    float  m_vx = 0;           ///< lateral speed
    float  m_yaw = 0;          ///< drawn yaw against the road
    float  m_hitTimer = 0;     ///< seconds since a hit (wobble)
    float  m_length = 4;       ///< metres
    float  m_width = 1.8f;     ///< metres
    bool   m_passed = false;   ///< the player is ahead of it
    bool   m_animal = false;   ///< an animal crossing the road (m_vx carries it across)
    float  m_wait = 0.0f;      ///< seconds it stands still in the road
    bool   m_waited = false;   ///< it has stood already
};

/** @brief A cone or an arrow board. */
struct Obstacle
{
    double  m_s = 0;            ///< distance along the road
    float   m_x = 0;            ///< lateral offset
    int     m_kind = 0;         ///< 0 cone, 1 board arrow left, 2 board arrow right
    bool    m_knocked = false;  ///< flying
    Vec3d   m_pos;              ///< world position while flying
    Vector3 m_vel{};            ///< world velocity while flying
    Vector3 m_axis{ 1, 0, 0 };  ///< tumble axis
    float   m_angle = 0;        ///< tumble angle
    float   m_spinRate = 0;     ///< rad/s
    float   m_life = 0;         ///< seconds flying
    float   m_groundY = 0;      ///< height to bounce on, relative to m_pos at launch
};

/** @brief What happened between the player and the others this step. */
struct TrafficEvents
{
    int   m_overtakes = 0;         ///< cars passed
    int   m_nearMisses = 0;        ///< cars passed close and fast
    float m_whooshPan = 0.0f;      ///< -1 left .. 1 right, the side of a car rushing past
    float m_whoosh = 0.0f;         ///< 0..1 its strength
    float m_hit = 0.0f;            ///< 0..1 a collision's strength
    int   m_cones = 0;             ///< cones hit
    int   m_animal = -1;           ///< an animal (PropKindE) about to cross ahead (-1: none)
};

/** @brief Traffic and roadworks. */
class Traffic
{
public:
    /**
     * @brief Clears the road.
     * @param[in] seed     the seed of this run's traffic
     * @param[in] startS   where the player starts
     */
    void reset( uint64_t seed, double startS );

    /**
     * @brief Spawns, drives and collides.
     * @param[in]     dt      the step in seconds
     * @param[in]     track   the road
     * @param[in,out] player  the player's car (pushed on contact)
     * @param[in]     models  vehicle sizes
     * @param[out]    ev      what happened (accumulated)
     */
    void update( float dt, const Track &track, PlayerCar &player, const Models &models, TrafficEvents &ev );

    std::vector<TrafficCar> m_cars;        ///< the vehicles
    std::vector<Obstacle>   m_obstacles;   ///< cones and boards
    float                   m_density = 1.0f;   ///< the track's traffic factor (0: none)

private:
    /**
     * @brief Places cones for roadworks segments not seen yet.
     * @param[in] track  the road
     * @param[in] upTo   scan until this distance
     */
    void scanRoadworks( const Track &track, double upTo );

    /**
     * @brief Whether a lane is free around a distance.
     * @param[in] lane  the lane
     * @param[in] s     the distance
     * @param[in] self  the car asking (ignored)
     * @return true if no car is within 18 m behind or 28 m ahead
     */
    bool laneFree( int lane, double s, const TrafficCar *self ) const;

    Rng    m_rng;               ///< spawning
    double m_spawnS = 0.0;      ///< the next spawn point
    double m_animalS = 0.0;     ///< where the next animals may cross
    long   m_scanned = 0;       ///< segments checked for roadworks
};

} // namespace sunstreak
