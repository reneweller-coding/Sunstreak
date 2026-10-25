/**
 * @file Passenger.h
 * @brief Hitchhikers and courier jobs: now and then someone waits by the road. Stop beside them: a hitchhiker rides along
 *        to the next place and thanks you in their language; a courier hands you a fragile parcel for the next place (hit
 *        nothing on the way); a paramedic an organ for the hospital at the next checkpoint (hurry); a police officer asks
 *        you to catch the car that was just stolen before it reaches the next checkpoint.
 * @author Rene Weller
 */
#pragma once

#include "Track.h"

#include <cstdint>
#include <string>

namespace sunstreak
{

constexpr double HitchAt = 1500.0;     ///< a stage's hitchhiker waits somewhere from here on (if it has one)
constexpr float  HitchPace = 38.0f;    ///< the pace (m/s) the time to deliver them is reckoned at
constexpr float  HitchSlack = 15.0f;   ///< and the seconds on top
constexpr float  HitchStop = 12.0f;    ///< slower than this (m/s) beside them, they get in
constexpr double HitchBefore = 25.0;   ///< from so far before them
constexpr double HitchPast = 15.0;     ///< to so far past them

/** @brief What the one by the road wants. */
enum class JobE : uint8_t
{
    HITCHHIKER,   ///< a ride to the next place
    FRAGILE,      ///< a fragile parcel for the next place: hit nothing
    ORGAN,        ///< an organ for the hospital at the next checkpoint: hurry
    THIEF,        ///< a stolen car to catch before the next checkpoint
    COUNT         ///< number of jobs
};

constexpr float OrganPace = 50.0f;   ///< the pace (m/s) the organ's time is reckoned at

/** @brief What the hitchhiker is doing. */
enum class PassengerStateE : uint8_t
{
    NONE,      ///< nobody near
    WAITING,   ///< by the road, thumb out
    ABOARD,    ///< riding along to the next place
};

/** @brief What happened this update. */
struct PassengerEvents
{
    bool m_pickedUp = false;    ///< they got in
    bool m_delivered = false;   ///< they got there in time
    bool m_late = false;        ///< too late: they got out at the roadside, a little cross
    bool m_thief =
        false;   ///< the officer's thief drives off: the chase begins (m_destS is where it must be caught by)
};

/**
 * @brief Where a stage's hitchhiker waits, if one does (two stages in three, on a straight stretch away from tunnels,
 *        bridges, forks and crossings, on the side the country drives on).
 * @param[in]  track  the road
 * @param[in]  stage  the stage
 * @param[out] s      the spot
 * @param[out] side   1 right of the road, -1 left
 * @return true if one waits in that stage
 */
bool stageHitchhiker( const Track &track, int stage, double &s, float &side );

/**
 * @brief "Hello" in a country's language (plain letters).
 * @param[in] country  its name (nullptr: none)
 * @return the greeting
 */
const char *helloIn( const char *country );

/**
 * @brief "Thank you" in a country's language (plain letters).
 * @param[in] country  its name (nullptr: none)
 * @return the thanks
 */
const char *thanksIn( const char *country );

/** @brief The hitchhiker of the moment. */
class Passenger
{
public:
    /** @brief Nobody waiting, nobody aboard. */
    void reset();

    /**
     * @brief Waits, gets in, rides along, gets out.
     * @param[in]  dt       the step in seconds
     * @param[in]  track    the road
     * @param[in]  s        the car's distance
     * @param[in]  x        its lateral offset
     * @param[in]  speed    its speed (m/s)
     * @param[in]  enabled  hitchhikers may wait (not in races, not in the demo, not once time is up)
     * @param[out] ev       what happened
     */
    void update( float dt, const Track &track, double s, float x, float speed, bool enabled, PassengerEvents &ev );

    /**
     * @brief The car hit something: a fragile parcel aboard breaks.
     * @return true if one did
     */
    bool breakCargo();

    PassengerStateE m_state = PassengerStateE::NONE;   ///< what they are doing
    JobE            m_job = JobE::HITCHHIKER;          ///< what they want
    double          m_s = 0.0;                         ///< where they wait
    float           m_x = 0.0f;                        ///< their lateral offset there
    float           m_side = 1.0f;                     ///< 1 right of the road, -1 left
    int             m_look = 0;                        ///< which of them (Models::m_hitchhikers)
    std::string     m_from;                            ///< their country (they greet and thank in its language)
    BiomeE          m_to = BiomeE::MEADOW;             ///< the place they want to go
    double          m_destS = 0.0;                     ///< where that is on the road
    float           m_timeLeft = 0.0f;                 ///< seconds left to get them there
    int             m_lastStage = -1;                  ///< the last stage whose hitchhiker came along
};

}   // namespace sunstreak
