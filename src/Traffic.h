/**
 * @file Traffic.h
 * @brief Everything else on the road: traffic to overtake, roadworks cones to dodge.
 *
 * Traffic drives in lanes at its own pace, changes lanes around slower cars and closed lanes, and is spawned ahead
 * beyond the fog. Passing a car counts as an overtake; passing it close and fast is a near miss and fills the boost
 * tank. Cones fly when hit. Now and then an ambulance or a fire engine of the country comes along with its siren on,
 * from behind when the player is slower, else ahead: the traffic pulls over to let it through.
 *
 * With oncoming traffic on (Traffic::m_oncoming), a road of an even number of lanes is one of two directions, as its
 * markings show it: the half across the middle carries what comes the other way. Each direction keeps to its half;
 * what comes the other way makes room at its edge, flashes its lights and sounds its horn at a car in its lane.
 *
 * @author Rene Weller
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
    double  m_s = 0;            ///< distance along the road
    float   m_x = 0;            ///< lateral offset
    float   m_speed = 0;        ///< m/s
    float   m_cruise = 0;       ///< the speed it likes
    int     m_lane = 0;         ///< target lane
    int     m_model = 0;        ///< index into Models::m_traffic
    float   m_vx = 0;           ///< lateral speed
    float   m_yaw = 0;          ///< drawn yaw against the road
    float   m_hitTimer = 0;     ///< seconds since a hit (wobble)
    float   m_length = 4;       ///< metres
    float   m_width = 1.8f;     ///< metres
    bool    m_passed = false;   ///< the player is ahead of it
    bool    m_braking = false;  ///< slowing hard (its brake lights)
    int8_t  m_signal = 0;       ///< indicating: -1 to the left, 1 to the right, 0 not
    float   m_signalWait = 0;   ///< seconds it still indicates before it moves over
    int     m_laneFrom = 0;     ///< the lane it keeps to until then
    bool    m_animal = false;   ///< an animal crossing the road (m_vx carries it across)
    float   m_wait = 0.0f;      ///< seconds it stands still in the road
    bool    m_waited = false;   ///< it has stood already
    bool    m_gone = false;     ///< took the other carriageway of a fork and has left the road
    bool    m_swim = false;     ///< swims round an undersea tunnel (never touches the road)
    uint8_t m_plate = 0;        ///< its number plates (PlateE)
    uint8_t m_lanes = 0;        ///< the road's lanes when it last looked (0: not yet)
    float   m_lift = 0.0f;      ///< height above the road
    float   m_orbit = 0.0f;     ///< radius of its circle over the road (0: swims straight)
    float   m_orbitA = 0.0f;    ///< where on the circle (radians)
    float   m_orbitW = 0.0f;    ///< how fast round it (radians per second, the sign its sense)
    double  m_orbitS = 0.0;     ///< the circle's centre along the road
    uint8_t m_siren =
        0;   ///< an ambulance or a fire engine on a run: 1 its siren wails, 2 the two tones of Europe (0: none)
    bool  m_swarm = false;   ///< one of a swarm of scooters (they ride side by side and never wait for each other)
    float m_xOff = 0.0f;     ///< where in its lane it rides (a swarm's scooters)
    bool  m_chaos = false;   ///< drives where the lines are a suggestion: weaves, changes lanes on a whim, honks
    float m_weave = 0.0f;    ///< the phase of its weaving
    float m_honk = 0.0f;     ///< seconds until it may honk again
    float m_yield = 0.0f;    ///< seconds it keeps pulling over for a siren behind
    bool  m_oncoming = false;   ///< comes the other way, on the other half of a road of two directions (m_s falls)
    float m_dodge = 0.0f;       ///< seconds it keeps to its edge of the road to leave room in the middle
    float m_flash = 0.0f;       ///< seconds its headlights still flash at the player in its lane
    int8_t m_overtake = 0;   ///< dense oncoming traffic: 1 waits for a chance to overtake, 2 out in the player's half

    /**
     * @brief Its speed along the road.
     * @return m/s, negative for what comes the other way
     */
    float along() const { return m_oncoming ? -m_speed : m_speed; }
};

/** @brief A cone or an arrow board. */
struct Obstacle
{
    double  m_s = 0;             ///< distance along the road
    float   m_x = 0;             ///< lateral offset
    int     m_kind = 0;          ///< 0 cone, 1 board arrow left, 2 board arrow right
    bool    m_knocked = false;   ///< flying
    Vec3d   m_pos;               ///< world position while flying
    Vector3 m_vel{};             ///< world velocity while flying
    Vector3 m_axis{ 1, 0, 0 };   ///< tumble axis
    float   m_angle = 0;         ///< tumble angle
    float   m_spinRate = 0;      ///< rad/s
    float   m_life = 0;          ///< seconds flying
    float   m_groundY = 0;       ///< height to bounce on, relative to m_pos at launch
};

/** @brief What happened between the player and the others this step. */
struct TrafficEvents
{
    int   m_overtakes = 0;        ///< cars passed
    int   m_nearMisses = 0;       ///< cars passed close and fast
    float m_whooshPan = 0.0f;     ///< -1 left .. 1 right, the side of a car rushing past
    float m_whoosh = 0.0f;        ///< 0..1 its strength
    float m_hit = 0.0f;           ///< 0..1 a collision's strength
    int   m_cones = 0;            ///< cones hit
    int   m_animal = -1;          ///< an animal (PropKindE) about to cross ahead (-1: none)
    int   m_gaveWay = 0;          ///< a siren passed the player, who had kept out of its way
    float m_horn = 0.0f;          ///< 0..1 a horn, the loudest this step
    float m_hornPan = 0.0f;       ///< -1 left .. 1 right: where
    float m_hornPitch = 1.0f;     ///< its Doppler shift
    float m_whooshPitch = 1.0f;   ///< the rush's pitch: higher the faster the car goes past
    int   m_closeCalls = 0;       ///< oncoming cars missed close and fast
    int   m_met = 0;              ///< oncoming cars that went by
    int   m_headOn = 0;           ///< crashes into what came the other way
    float m_gust = 0.0f;          ///< 0..1 the blast of a lorry or a bus going by the other way
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

    /**
     * @brief The player flashes the headlights: slower vehicles ahead in the player's lane make way to the slow side
     *        for a while (where the lines are a suggestion, only some of them do).
     * @param[in] player  the player's car
     * @return how many made way
     */
    int flash( const PlayerCar &player );

    /**
     * @brief The player sounds the horn: animals in the road ahead stop dawdling and get out of the way; where the lines
     *        are a suggestion, a car near by answers.
     * @param[in]  player    the player's car
     * @param[out] answered  a car near by honks back
     * @param[out] pan       where it is, -1 left .. 1 right
     * @return how many animals were startled
     */
    int honk( const PlayerCar &player, bool &answered, float &pan );

    /**
     * @brief The lanes of a stretch that belong to the traffic coming the other way: the half across the middle of a
     *        road of an even number of lanes (the left one, or the right one where they drive on the left). A fork's
     *        two carriageways both lead away, and a road of an odd number of lanes is one carriageway.
     * @param[in]  track  the road
     * @param[in]  seg    the stretch
     * @param[out] first  the first of them (0 leftmost)
     * @param[out] last   the last of them (before the first if there are none)
     * @return how many (0: a road of one direction, or oncoming traffic is off)
     */
    int oncomingLanes( const Track &track, const Segment &seg, int &first, int &last ) const;

    /** @brief An ambulance or a fire engine comes along at once, close ahead or behind (testing). */
    void sirenNow()
    {
        m_sirenS = 0.0;
        m_sirenAhead = 200.0;
    }

    /**
     * @brief Three vehicles come the other way at once (testing; where the road has two directions). With dense
     *        oncoming traffic the last of them overtakes the others.
     * @param[in] ahead  how far ahead they appear (m)
     */
    void oncomingNow( double ahead = 190.0 )
    {
        m_oncomingDue = 0.0;
        m_oncomingAhead = ahead;
    }

    std::vector<TrafficCar> m_cars;             ///< the vehicles
    std::vector<Obstacle>   m_obstacles;        ///< cones and boards
    float                   m_density = 1.0f;   ///< the track's traffic factor (0: none)
    int                     m_oncoming = 0;   ///< traffic the other way on roads of two directions: 0 off, 1 light,
                                              ///< 2 normal, 3 dense (the setting)

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

    /**
     * @brief Whether a stretch is a road of two directions all the way, the same half the oncoming one, with no ferry
     *        or tunnel train on it and no roadworks at its far end: only then something comes the other way along it.
     * @param[in] track  the road
     * @param[in] from   from here
     * @param[in] to     to here
     * @return true if it is
     */
    bool twoWay( const Track &track, double from, double to ) const;

    Rng    m_rng;                  ///< spawning
    double m_spawnS = 0.0;         ///< the next spawn point
    double m_animalS = 0.0;        ///< where the next animals may cross
    double m_swimS = 0.0;          ///< where the next creatures swim by
    long   m_scanned = 0;          ///< segments checked for roadworks
    double m_sirenS = 0.0;         ///< where the next ambulance or fire engine comes along
    double m_sirenAhead = 900.0;   ///< how far ahead it appears when the player is faster
    float  m_clock = 0.0f;         ///< seconds driven (the weaving)
    Rng    m_rngOncoming;          ///< spawning what comes the other way (its own, so the rest stays as it is)
    double m_oncomingDue = 0.0;    ///< metres of the oncoming stream until its next vehicle
    double m_oncomingAhead = 1050.0;   ///< how far ahead it appears (beyond the fog; closer only for testing)
    float  m_spawnThin = 1.0f;     ///< how much further apart to spawn where only half the lanes go the player's way
};

/** @brief How far an ambulance's or a fire engine's siren carries (metres). */
constexpr double SirenRange = 600.0;

}   // namespace sunstreak
