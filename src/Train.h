/**
 * @file Train.h
 * @brief The fast trains: in some stages of their countries a viaduct runs beside the road for a few kilometres, and the
 *        country's train races the car along it (data/trains.txt). Pass its nose before the stretch ends to win.
 * @author reneweller
 */
#pragma once

#include "Car.h"
#include "Track.h"

#include <cstdint>
#include <string>
#include <vector>

namespace sunstreak
{

constexpr float  RailOffset = 14.0f;     ///< the viaduct's middle, this far beyond the road's outer edge
constexpr float  RailHeight = 5.5f;      ///< the rails this high over the road
constexpr float  CoachLength = 25.6f;    ///< a coach (and the power car)
constexpr double RailBefore = 400.0;     ///< the viaduct begins this far before the race
constexpr double RailAfter = 900.0;      ///< and ends this far after it
constexpr double PortalLength = 30.0;    ///< the stone portals at its ends
constexpr float  TrainPace = 0.93f;      ///< the train's top speed against the car's, at most
constexpr int    TrainBonus = 4000;      ///< points for beating it (and five seconds where the clock runs)

/** @brief A train of data/trains.txt. */
struct TrainLine
{
    std::string              m_name;        ///< its shapes are TRAIN_NAME and TRAIN_NAME_COACH
    std::vector<std::string> m_countries;   ///< where it runs
    float                    m_speed = 80.0f;   ///< top speed (m/s)
    int                      m_coaches = 8;     ///< coaches behind the power car
};

/**
 * @brief The trains, read again after a reload.
 * @return them
 */
const std::vector<TrainLine> &trainLines();

/**
 * @brief How many lines of data/trains.txt were wrong.
 * @return the count
 */
int trainErrors();

/** @brief Where a stage's train races, if it has one. */
struct TrainStretch
{
    double m_from = 0.0;   ///< the race begins (the viaduct RailBefore earlier)
    double m_to = 0.0;     ///< and ends (the viaduct RailAfter later)
    float  m_side = 1.0f;  ///< 1 right of the road, -1 left
    int    m_line = -1;    ///< which train (trainLines())
};

/**
 * @brief A stage's train race: one stage in two of a country with a train, 4 km in the middle of the stage (away from
 *        its fork), not in crossings, under the sea or in neon places.
 * @param[in]  track  the road
 * @param[in]  stage  the stage
 * @param[out] out    the stretch
 * @return true if the stage has one
 */
bool stageTrain( const Track &track, int stage, TrainStretch &out );

/**
 * @brief Whether the viaduct runs beside a point of the road.
 * @param[in]  track  the road
 * @param[in]  s      the distance
 * @param[out] side   its side (1 right, -1 left)
 * @return true if it does
 */
bool viaductAt( const Track &track, double s, float &side );

/**
 * @brief Where a point of the rails lies (as the world's chunks build the viaduct: the road's bank only across the road).
 * @param[in] track  the road
 * @param[in] s      the distance
 * @param[in] side   the viaduct's side
 * @return the point on the rails
 */
Vec3d railPoint( const Track &track, double s, float side );

/** @brief What happened this update. */
struct TrainEvents
{
    bool m_start = false;   ///< the race began
    bool m_won = false;     ///< the car passed the nose in time
    bool m_lost = false;    ///< the train got to the end first
};

/** @brief The race against the train. */
class TrainRace
{
public:
    /** @brief No train. */
    void reset();

    /**
     * @brief Starts, runs and ends the race.
     * @param[in]  dt       the step in seconds
     * @param[in]  track    the road
     * @param[in]  player   the car
     * @param[in]  enabled  a race may begin (not in races, the demo, or once time is up)
     * @param[out] ev       what happened
     */
    void update( float dt, const Track &track, const PlayerCar &player, bool enabled, TrainEvents &ev );

    /** @brief The race is on or the train still in sight. */
    bool active() const { return m_running || m_after; }

    bool         m_running = false;   ///< the race is on
    bool         m_after = false;     ///< decided, the train runs on
    double       m_nose = 0.0;        ///< the power car's front
    float        m_speed = 0.0f;      ///< m/s
    float        m_top = 0.0f;        ///< its top speed in this race
    TrainStretch m_stretch;           ///< where
    int          m_lastStage = -1;    ///< the last stage whose train ran
};

} // namespace sunstreak
