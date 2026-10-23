/**
 * @file Race.h
 * @brief The championship: rival drivers, a race from the grid to the finish, points and standings.
 *
 * Like Lotus III's races the player starts at the back of a grid of twelve. The rivals drive in road coordinates,
 * brake for bends by the same physics as the player, slipstream, change lanes to pass and are held together by a
 * gentle rubber band. Five rounds, points 10-8-6-5-4-3-2-1; a finish outside the top eight does not qualify.
 *
 * @author reneweller
 */
#pragma once

#include "Car.h"
#include "Rng.h"
#include "Track.h"

#include <array>
#include <string>
#include <vector>

namespace sunstreak
{

constexpr int RivalCount = 11;      ///< rivals on the grid (twelve cars with the player)
constexpr int RoundCount = 5;       ///< races in a championship
constexpr int QualifyPlace = 8;     ///< the worst place that qualifies for the next round

/** @brief A rival driver and the car on the track. */
struct Rival
{
    int    m_driver = 0;         ///< index into the championship's drivers
    double m_s = 0;              ///< distance along the road
    float  m_x = 0;              ///< lateral offset
    float  m_speed = 0;          ///< m/s
    float  m_vx = 0;             ///< lateral speed
    float  m_yaw = 0;            ///< drawn yaw
    int    m_lane = 0;           ///< target lane
    float  m_launch = 0;         ///< reaction at the start (s)
    bool   m_finished = false;   ///< crossed the line
    float  m_finishTime = 0;     ///< race time at the line
    float  m_hitTimer = 0;       ///< wobble after contact
};

/** @brief A driver of the championship. */
struct Driver
{
    std::string m_name;          ///< e.g. "K. VOSS"
    int         m_car = 0;       ///< car model
    int         m_paint = 0;     ///< paint
    float       m_skill = 1.0f;  ///< pace factor
    int         m_points = 0;    ///< championship points
};

/** @brief One line of a race result. */
struct ResultLine
{
    int   m_driver = -1;         ///< -1 the player, else a driver
    float m_time = 0;            ///< finish time (s), or a projection for those still running
    int   m_points = 0;          ///< points scored
};

/** @brief What happened between the player and the rivals this step. */
struct RaceEvents
{
    float m_hit = 0.0f;          ///< contact strength
    float m_whoosh = 0.0f;       ///< a rival rushing past
    float m_whooshPan = 0.0f;    ///< its side
    int   m_passed = 0;          ///< rivals the player passed (net)
};

/** @brief A championship and its current race. */
class Race
{
public:
    /**
     * @brief Starts a championship: drivers, their cars and skills.
     * @param[in] seed  the championship's seed
     */
    void newChampionship( uint64_t seed );

    /**
     * @brief The track parameters of a round.
     * @param[in] round  0..RoundCount-1
     * @return length, biome and start line of that race
     */
    TrackParams roundParams( int round ) const;

    /**
     * @brief The seed of a round's track.
     * @param[in] round  0..RoundCount-1
     * @return the seed
     */
    uint64_t roundSeed( int round ) const;

    /**
     * @brief Puts the field on the grid, the player at the back.
     * @param[in]     track   the race's track
     * @param[in,out] player  the player's car (placed)
     * @param[in]     pace    rivals' pace factor (difficulty)
     */
    void grid( const Track &track, PlayerCar &player, float pace );

    /**
     * @brief Advances the rivals and handles contact with the player.
     * @param[in]     dt       seconds
     * @param[in]     raceTime seconds since the start
     * @param[in]     track    the road
     * @param[in,out] player   the player's car
     * @param[out]    ev       events (accumulated)
     * @param[in]     started  the lights went green
     */
    void update( float dt, float raceTime, const Track &track, PlayerCar &player, RaceEvents &ev, bool started );

    /**
     * @brief The player's place now.
     * @param[in] player  the player's car
     * @return 1..RivalCount+1
     */
    int position( const PlayerCar &player ) const;

    /**
     * @brief Closes the race: the order, the points (added to the drivers and the player).
     * @param[in] playerTime  the player's finish time
     * @return the result, winner first
     */
    std::vector<ResultLine> finish( float playerTime );

    /**
     * @brief The championship table.
     * @return driver indices (-1 the player) with points, leader first
     */
    std::vector<std::pair<int, int>> standings() const;

    std::vector<Rival>              m_rivals;          ///< the field
    std::array<Driver, RivalCount>  m_drivers{};       ///< the championship's drivers
    int                             m_playerPoints = 0;///< the player's points
    int                             m_round = 0;       ///< current round
    uint64_t                        m_seed = 1;        ///< championship seed
    double                          m_finishS = 0.0;   ///< the finish line of the current race

private:
    /**
     * @brief The speed a rival may carry into the bends ahead.
     * @param[in] track  the road
     * @param[in] r      the rival
     * @return m/s
     */
    float cornerSpeed( const Track &track, const Rival &r ) const;

    float m_pace = 1.0f;   ///< difficulty factor
    Rng   m_rng;           ///< lane choices and launches
};

/**
 * @brief Points for a place.
 * @param[in] place  1..12
 * @return 10, 8, 6, 5, 4, 3, 2, 1 or 0
 */
int pointsFor( int place );

} // namespace sunstreak
