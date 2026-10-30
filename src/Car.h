/**
 * @file Car.h
 * @brief The player's car: arcade physics in track space.
 *
 * Like the classic pseudo-3D racers, the car lives in road coordinates: distance along the road (s), offset from the
 * centre (x), speed. Steering moves it sideways, bends push it outwards with the square of the speed, the land
 * beside the road slows it down. Crests can throw it into the air. Gears, a rev limiter and a boost tank drive the
 * engine sound.
 *
 * @author Rene Weller
 */
#pragma once

#include "Models.h"
#include "Track.h"

#include <string>

namespace sunstreak
{

constexpr int   GearCount = 6;            ///< forward gears
constexpr float Gravity = 9.81f * 1.7f;   ///< arcade gravity (shorter, punchier jumps)

/** @brief The handling's knobs: tuning.ini sets them, the difficulty scales them. */
struct Tuning
{
    float m_steer = 15.0f;         ///< sideways speed at full lock (m/s)
    float m_centrifugal = 0.42f;   ///< outward push per curvature and speed squared
    float m_accel = 11.5f;         ///< acceleration in first gear (m/s^2)
    float m_brake = 24.0f;         ///< braking (m/s^2)
    float m_boost = 6.5f;          ///< extra acceleration while boosting (m/s^2)
    float m_grip = 1.0f;           ///< grip factor
    float m_crashKeep = 0.22f;     ///< share of the speed a crash leaves
    float m_time = 1.0f;           ///< time bonus factor (endless)
    float m_traffic = 1.0f;        ///< traffic density factor
    float m_rivals = 1.0f;         ///< rivals' pace factor (championship)
};

/** @brief Difficulty levels. */
enum class DifficultyE
{
    EASY,     ///< more time, more grip, gentler crashes, slower rivals
    NORMAL,   ///< as tuned
    HARD,     ///< less time, more traffic, faster rivals
    COUNT     ///< number of levels
};

/**
 * @brief The handling in effect (tuning.ini scaled by the difficulty).
 * @return the tuning, writable
 */
Tuning &tuning();

/**
 * @brief Scales a tuning by a difficulty.
 * @param[in] base        tuning.ini's values
 * @param[in] difficulty  0 easy, 1 normal, 2 hard
 * @return the tuning to use
 */
Tuning applyDifficulty( const Tuning &base, int difficulty );

/**
 * @brief Reads tuning.ini, writing one with the defaults and explanations if there is none.
 * @param[in] path  the file
 * @return the values (defaults where missing)
 */
Tuning loadTuning( const std::string &path );

/**
 * @brief The fastest sideways speed steering can give at a speed (before grip).
 * @param[in] speed     m/s
 * @param[in] topSpeed  the car's top speed
 * @return m/s
 */
inline float steerLimit( float speed, float topSpeed )
{
    return tuning().m_steer * smoothstepf( 0.0f, 12.0f, speed ) * ( 1.0f - 0.18f * speed / topSpeed );
}

/** @brief A position and an orientation in the world. */
struct Pose
{
    Vec3d   m_pos;         ///< world position
    Vector3 m_left{};      ///< model +x
    Vector3 m_up{};        ///< model +y
    Vector3 m_forward{};   ///< model +z
};

/**
 * @brief The pose of something on the road.
 * @param[in] track  the road
 * @param[in] s      distance along it
 * @param[in] x      lateral offset
 * @param[in] yaw    turned against the road (rad, positive right)
 * @return the pose on the surface
 */
Pose roadPose( const Track &track, double s, float x, float yaw );

/** @brief What the driver does. */
struct CarInput
{
    float m_throttle = 0.0f;     ///< 0..1
    float m_brake = 0.0f;        ///< 0..1
    float m_steer = 0.0f;        ///< -1 left .. 1 right
    bool  m_boost = false;       ///< boost held
    bool  m_shiftUp = false;     ///< pressed this frame (manual gears)
    bool  m_shiftDown = false;   ///< pressed this frame
};

/** @brief The player's car. */
class PlayerCar
{
public:
    /**
     * @brief Places the car at a standstill.
     * @param[in] spec   which car
     * @param[in] s      distance along the road
     * @param[in] x      lateral offset
     * @param[in] track  the track (its height there)
     */
    void reset( const CarSpec &spec, double s, float x, const Track &track );

    /**
     * @brief Advances the car by one step.
     * @param[in] in      the driver's input
     * @param[in] track   the road
     * @param[in] dt      the step in seconds
     * @param[in] manual  manual gears
     */
    void update( const CarInput &in, const Track &track, float dt, bool manual );

    /**
     * @brief A heavy hit: the car spins and loses most of its speed.
     * @param[in] keep  share of the speed kept
     * @param[in] push  lateral shove (m)
     */
    void crash( float keep, float push );

    /**
     * @brief The speed in the chosen unit.
     * @param[in] mph  miles per hour instead of km/h
     * @return the speed
     */
    float displaySpeed( bool mph ) const { return m_speed * ( mph ? 2.23694f : 3.6f ); }

    /**
     * @brief The engine speed in revolutions per minute.
     * @return rpm
     */
    float rpm() const { return m_spec.m_idle + m_rpm * ( m_spec.m_redline - m_spec.m_idle ); }

    /**
     * @brief Where the car is and how it is turned, pitched and rolled.
     * @param[in] track  the road
     * @return its pose
     */
    Pose pose( const Track &track ) const;

    // State. Public on purpose: the game, the renderer and the audio read it every frame.
    CarSpec m_spec{};               ///< which car
    double  m_s = 0.0;              ///< distance along the road
    float   m_x = 0.0f;             ///< lateral offset (right positive)
    double  m_y = 0.0;              ///< world height of the car's base
    float   m_vy = 0.0f;            ///< vertical speed
    float   m_speed = 0.0f;         ///< m/s along the road
    float   m_vx = 0.0f;            ///< lateral speed from steering
    float   m_drift = 0.0f;         ///< lateral speed from the bend (outwards)
    float   m_steer = 0.0f;         ///< smoothed steering
    int     m_gear = 0;             ///< 0..GearCount-1
    float   m_rpm = 0.1f;           ///< 0..1.15 of the redline
    float   m_shiftTimer = 0.0f;    ///< seconds left in a shift
    float   m_boostTank = 0.6f;     ///< 0..1
    bool    m_boosting = false;     ///< boost active this step
    float   m_throttle = 0.0f;      ///< smoothed throttle (sound)
    bool    m_airborne = false;     ///< off the ground
    float   m_airTime = 0.0f;       ///< seconds in the air
    float   m_slip = 0.0f;          ///< 0..1 tyre squeal
    float   m_offroad = 0.0f;       ///< 0..1 off the tarmac
    float   m_rumble = 0.0f;        ///< 0..1 on the kerb
    float   m_yawVisual = 0.0f;     ///< drawn yaw against the road (rad)
    float   m_spin = 0.0f;          ///< crash spin angle (rad)
    float   m_crashTimer = 0.0f;    ///< seconds of crash left
    float   m_shake = 0.0f;         ///< camera shake 0..1
    float   m_wetGrip = 1.0f;       ///< grip of the surface under the car
    float   m_weatherGrip = 1.0f;   ///< what the weather leaves of the grip (rain, snow, sand on the road)
    float   m_rain =
        0.0f;   ///< 0..1 how hard it rains: puddles gather from 0.35, and the tyres float in them over 100 km/h
    float   m_aquaplane = 0.0f;     ///< 0..1 the tyres float on water
    bool    m_monsoon = false;      ///< a monsoon falls: the road floods in stretches (floodAt)
    bool    m_inFlood = false;      ///< driving through a flood (fountains, a drag on the speed)
    uint8_t m_unpaved = 0;          ///< the road under the car: 0 asphalt, 1 gravel, 2 sand
    long    m_lastPothole = -1;     ///< the segment of the last pothole it hit
    bool    m_frozen = false;       ///< held at the start (the engine still revs)
    bool    m_reverse = false;      ///< in reverse: m_speed is then the speed backwards
    float   m_reverseHold = 0.0f;   ///< seconds the brake has been held at a standstill (reverse after ReverseDelay)
    double  m_farthest = 0.0;       ///< the farthest the car has come (reverse goes at most ReverseReach behind it)
    bool    m_coasting = false;     ///< time is up: no throttle any more

    // Events of the last update, for sound and particles.
    bool  m_eventShift = false;       ///< a gear change
    bool  m_eventLanded = false;      ///< touched down
    float m_landImpact = 0.0f;        ///< how hard (m/s)
    bool  m_eventBackfire = false;    ///< lifted off at high revs
    bool  m_eventAquaplane = false;   ///< the tyres began to float

private:
    /**
     * @brief The top speed of a gear.
     * @param[in] gear  the gear
     * @return m/s at the redline
     */
    float gearTop( int gear ) const;

    double m_prevRoadY = 0.0;    ///< the road's height last step
    float  m_lastThrottle = 0;   ///< raw throttle last step (backfires)
};

}   // namespace sunstreak
