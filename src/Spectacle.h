/**
 * @file Spectacle.h
 * @brief Spectacles over and beside the road (data/spectacles.txt): an airliner coming in low across it, fighter jets
 *        roaring along it, hot-air balloons, a container ship, an airship. One at a time, in some stages of their places.
 * @author Rene Weller
 */
#pragma once

#include "Car.h"
#include "Track.h"

#include <cstdint>
#include <string>
#include <vector>

namespace sunstreak
{

/** @brief What there is to see. */
enum class SkyE : uint8_t
{
    PLANE,      ///< an airliner on its final approach, low across the road
    JETS,       ///< two fighter jets along the road overhead
    BALLOONS,   ///< hot-air balloons drifting over the land
    SHIP,       ///< a container ship on the water beside the road
    ZEPPELIN,   ///< an airship drifting high
    COUNT       ///< number of kinds
};

/** @brief A line of data/spectacles.txt. */
struct SkyRule
{
    SkyE                     m_kind = SkyE::PLANE;   ///< what
    std::vector<std::string> m_where;                ///< countries, COUNTRY:REGION, place:ID
    float                    m_share = 0.5f;         ///< of their stages
};

/**
 * @brief The rules, read again after a reload.
 * @return them
 */
const std::vector<SkyRule> &skyRules();

/**
 * @brief How many lines of data/spectacles.txt were wrong (an unknown kind, country, region or place).
 * @return the count
 */
int skyErrors();

/**
 * @brief A stage's spectacle, if it has one.
 * @param[in]  track  the road
 * @param[in]  stage  the stage
 * @param[out] kind   what
 * @param[out] at     where along the road it begins
 * @return true if the stage has one
 */
bool stageSpectacle( const Track &track, int stage, SkyE &kind, double &at );

constexpr double RunwayAhead =
    600.0;                            ///< the landing plane crosses the road this far beyond where its spectacle begins
constexpr float RunwayNear = 70.0f;   ///< the runway begins this far beside the road's edge
constexpr float RunwayLength = 600.0f;   ///< and runs this far away from it
constexpr float RunwayHalf = 22.0f;      ///< half its width

/**
 * @brief Where a stage's landing plane crosses the road, if the stage has one: there the world lays its runway.
 * @param[in]  track  the road
 * @param[in]  stage  the stage
 * @param[out] s      where along the road it crosses
 * @param[out] side   the side the runway lies on (-1 left, 1 right; the plane comes from the other)
 * @return true if the stage has a landing plane
 */
bool stageRunway( const Track &track, int stage, double &s, float &side );

/** @brief One thing in the sky or on the water, in road terms. */
struct SkyThing
{
    int    m_model = 0;        ///< which SKY_ shape (Models::m_sky)
    double m_s = 0.0;          ///< along the road
    float  m_lateral = 0.0f;   ///< right of the road's middle
    float  m_height = 0.0f;    ///< over the road
    float  m_turn = 0.0f;      ///< its heading against the road's (0 along it, +pi/2 across to the right)
};

/** @brief The models a spectacle draws, by kind (indices into Models::m_sky, which follows this order). */
enum SkyModelE
{
    SKY_AIRLINER,
    SKY_JET,
    SKY_BALLOON_1,
    SKY_BALLOON_2,
    SKY_BALLOON_3,
    SKY_BALLOON_4,
    SKY_SHIP,
    SKY_ZEPPELIN,
    SKY_MODELS
};

/**
 * @brief The shape name of a model.
 * @param[in] model  the model
 * @return e.g. "SKY_AIRLINER"
 */
const char *skyModelName( int model );

/** @brief What happened this update. */
struct SkyEvents
{
    bool  m_roar = false;   ///< engines coming over (the jet sound)
    float m_pan = 0.0f;     ///< from where
};

/** @brief The spectacle of the moment. */
class Spectacle
{
public:
    /** @brief Nothing to see. */
    void reset();

    /**
     * @brief Begins, moves and ends the spectacle.
     * @param[in]  dt       the step in seconds
     * @param[in]  track    the road
     * @param[in]  player   the car
     * @param[in]  enabled  one may begin (not in the demo's menus)
     * @param[out] ev       what happened
     */
    void update( float dt, const Track &track, const PlayerCar &player, bool enabled, SkyEvents &ev );

    std::vector<SkyThing> m_things;               ///< what is there
    SkyE                  m_kind = SkyE::COUNT;   ///< what (COUNT: nothing)
    int                   m_lastStage = -1;       ///< the last stage looked at
    double                m_at = -1.0;            ///< where the next one begins (-1: none waiting)
    SkyE                  m_next = SkyE::COUNT;   ///< and what it is
    float                 m_dir = 1.0f;           ///< which way across or which side
};

}   // namespace sunstreak
