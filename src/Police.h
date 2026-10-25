/**
 * @file Police.h
 * @brief The police of the world tour: now and then a patrol car of the country waits at the roadside. Past it too fast,
 *        it gives chase with its lights and siren until it pulls up beside the car (caught: a fine in seconds) or the
 *        car is far enough ahead (escaped: points).
 * @author Rene Weller
 */
#pragma once

#include "Track.h"

#include <cstdint>

namespace sunstreak
{

class Models;
class Traffic;

constexpr double PoliceSpawn = 900.0;   ///< the patrol car appears this far before its spot
constexpr double PoliceEscape = 400.0;  ///< this far ahead of it, the car has got away
constexpr float  PoliceGiveUp = 90.0f;  ///< and after so many seconds of chasing in any case
constexpr float  PoliceFine = 5.0f;     ///< seconds a catch costs
constexpr float  PolicePace = 0.9f;     ///< its top speed against the car's

/** @brief What the patrol car is doing. */
enum class PoliceStateE : uint8_t
{
    NONE,     ///< no patrol car near
    PARKED,   ///< waiting at the roadside
    CHASE,    ///< lights and siren, after the car
    STOPPED,  ///< caught it, or gave up: pulls over and is left behind
};

/** @brief What happened this update. */
struct PoliceEvents
{
    bool m_chase = false;     ///< it set off after the car
    bool m_caught = false;    ///< it pulled up beside the car
    bool m_escaped = false;   ///< the car got away
};

/** @brief The patrol car near the player. */
class Police
{
public:
    /** @brief Forgets the patrol car (a new run, a crossing). */
    void reset();

    /**
     * @brief Places, wakes and drives the patrol car.
     * @param[in]  dt       seconds
     * @param[in]  track    the road
     * @param[in]  s        the player's distance
     * @param[in]  x        the player's lateral offset
     * @param[in]  speed    the player's speed (m/s)
     * @param[in]  top      the player's top speed (m/s): the patrol car's is PolicePace of it
     * @param[in]  traffic  the traffic it weaves through
     * @param[in]  models   the meshes (which patrol car the country drives)
     * @param[in]  enabled  traps wait on this road (the world tour outside races)
     * @param[out] ev       what happened
     */
    void update( float dt, const Track &track, double s, float x, float speed, float top, const Traffic &traffic, const Models &models,
                 bool enabled, PoliceEvents &ev );

    /**
     * @brief How far the car is ahead of the patrol car.
     * @param[in] s  the player's distance
     * @return metres (negative: the patrol car is ahead)
     */
    double gap( double s ) const { return s - m_s; }

    PoliceStateE m_state = PoliceStateE::NONE;   ///< what it is doing
    double       m_s = 0.0;                      ///< where along the road
    float        m_x = 0.0f;                     ///< lateral offset
    float        m_speed = 0.0f;                 ///< m/s
    float        m_yaw = 0.0f;                   ///< against the road
    int          m_model = -1;                   ///< its traffic model
    int          m_siren = 0;                    ///< 0 a wail, 1 the two tones of Europe
    float        m_chaseTime = 0.0f;             ///< seconds since it set off
    float        m_beside = 0.0f;                ///< seconds it has been level with the car

private:
    int    m_trapStage = -1;     ///< the stage of the trap it belongs to
    double m_trapS = 0.0;        ///< where it waits
    int    m_doneStage = -1;     ///< the last stage whose trap is spent
    double m_lastS = 0.0;        ///< the player's distance last update
};

/**
 * @brief The patrol car a place drives: the POLICE_CAR_* shape at home in its region or country, else the plain POLICE_CAR.
 * @param[in] models  the meshes
 * @param[in] place   the place
 * @return its traffic model, or -1 if there is none
 */
int policeModel( const Models &models, const BiomeDef &place );

} // namespace sunstreak
