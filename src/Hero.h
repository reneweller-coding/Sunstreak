/**
 * @file Hero.h
 * @brief The local hero: now and then a driver of the country pulls up beside the car before a checkpoint and challenges
 *        it to the next one, in their own language. Get there first and they grumble; let them win and they taunt you.
 * @author reneweller
 */
#pragma once

#include "Car.h"
#include "Track.h"

#include <cstdint>
#include <string>

namespace sunstreak
{

class Traffic;

constexpr double HeroApproach = 700.0;   ///< the hero turns up behind the car this far before the checkpoint
constexpr double HeroGone = 600.0;       ///< after the duel, this far away they are gone
constexpr int    HeroBonus = 5000;       ///< points for beating them (and five seconds where the clock runs)

/** @brief What the hero is doing. */
enum class HeroStateE : uint8_t
{
    NONE,     ///< nobody
    CLOSING,  ///< coming up beside the car before the checkpoint
    DUEL,     ///< racing it to the next checkpoint
    AFTER,    ///< it is decided: driving off
};

/** @brief What happened this update. */
struct HeroEvents
{
    bool m_start = false;   ///< the duel began at the checkpoint
    bool m_won = false;     ///< the car got to the next one first
    bool m_lost = false;    ///< the hero did
};

/**
 * @brief Where a stage's duel begins, if it has one (one stage in three; a checkpoint well inside the stage, with no
 *        crossing or fork before the next one).
 * @param[in]  track       the road
 * @param[in]  stage       the stage
 * @param[out] checkpoint  the checkpoint the duel starts at
 * @return true if the stage has a duel
 */
bool stageHero( const Track &track, int stage, double &checkpoint );

/** @brief The local hero of the moment. */
class LocalHero
{
public:
    /** @brief Nobody. */
    void reset();

    /**
     * @brief Turns up, races, drives off.
     * @param[in]     dt       the step in seconds
     * @param[in]     track    the road
     * @param[in,out] player   the car (pushed on contact)
     * @param[in]     traffic  the traffic to get past
     * @param[in]     enabled  a duel may begin (not in races or the demo, not in a police chase, not once time is up)
     * @param[out]    ev       what happened
     */
    void update( float dt, const Track &track, PlayerCar &player, const Traffic &traffic, bool enabled, HeroEvents &ev );

    HeroStateE  m_state = HeroStateE::NONE;   ///< what they are doing
    double      m_s = 0.0;                    ///< where along the road
    float       m_x = 0.0f;                   ///< lateral offset
    float       m_speed = 0.0f;               ///< m/s
    float       m_yaw = 0.0f;                 ///< drawn yaw
    int         m_lane = 0;                   ///< the lane they aim for
    int         m_car = 0;                    ///< their car (a player car model)
    int         m_paint = 0;                  ///< its paint
    std::string m_name;                       ///< their first name
    std::string m_country;                    ///< their country (they speak its language)
    double      m_start = 0.0;                ///< the checkpoint the duel starts at
    double      m_goal = 0.0;                 ///< and the one it ends at
    bool        m_heroFirst = false;          ///< they won
    int         m_lastStage = -1;             ///< the last stage whose hero came along

private:
    float m_hitTimer = 0.0f;   ///< seconds since a bump (no second shove at once)
};

} // namespace sunstreak
