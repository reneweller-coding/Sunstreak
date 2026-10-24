/**
 * @file Local.h
 * @brief The customs of a country's roads (data/roads.txt): speed limits in km/h or mph on round signs or white boards,
 *        toll plazas, speed cameras, number plates; and where a stage puts its camera and its toll plaza.
 * @author reneweller
 */
#pragma once

#include "Track.h"

#include <cstdint>

namespace sunstreak
{

/** @brief Number plates. */
enum class PlateE : uint8_t
{
    PLAIN,    ///< plain white
    EU,       ///< white with a blue band
    UK,       ///< yellow behind
    US,       ///< white with a coloured top
    AMBER,    ///< yellow
    JP,       ///< small, white with green letters
    DARK,     ///< black with white letters
    COUNT     ///< number of styles
};

/** @brief How a country's roads are run. */
struct RoadCustoms
{
    bool   m_mph = false;              ///< speed limits in miles per hour
    bool   m_board = false;            ///< speed limits on a white board
    bool   m_tolls = false;            ///< toll plazas
    bool   m_cameras = false;          ///< speed cameras
    PlateE m_plate = PlateE::PLAIN;    ///< number plates
    float  m_quality = 4.5f;           ///< 1..7 how good its roads are
    float  m_width = 3.5f;             ///< metres per lane
    bool   m_shoulders = false;        ///< a paved shoulder beyond the edge line
    bool   m_kerbs = false;            ///< its towns paint their kerbs
    Color  m_kerbA{};                  ///< in this colour
    Color  m_kerbB{};                  ///< and this one
    bool   m_studs = false;            ///< reflecting studs along the lines
    float  m_concrete = 0.0f;          ///< 0..1 of its places paved with concrete slabs
    int    m_concreteLanes = 0;        ///< only roads of so many lanes or more
    float  m_gravel = 0.0f;            ///< 0..1 of a country road on gravel or dirt
    float  m_traffic = 0.0f;           ///< Numbeo's traffic index (0: not known)
    uint8_t m_styles = 0;              ///< how its traffic drives (Style* bits)
    float  m_pollution = -1.0f;        ///< Numbeo's pollution index 0..100 (-1: not known)
};

constexpr uint8_t StyleScooters = 1;     ///< swarms of scooters
constexpr uint8_t StyleChaos = 2;        ///< weaving, lanes changed on a whim, horns
constexpr uint8_t StyleTractors = 4;     ///< tractors on the country roads
constexpr uint8_t StyleRoadTrains = 8;   ///< road trains in the outback

constexpr double CameraAt = 650.0;   ///< where in its stage a speed camera stands (or the first clear spot after)
constexpr double LimitAt = 300.0;    ///< and the speed limit before it
constexpr double TollAt = 1300.0;    ///< where in its stage a toll plaza stands (or the first clear spot after)
constexpr double PoliceAt = 2500.0;  ///< a stage's patrol car waits somewhere from here on (if it has one)

/**
 * @brief A country's road customs.
 * @param[in] country  its name (nullptr: none)
 * @return them (the plain ones for none)
 */
const RoadCustoms &roadCustoms( const char *country );

/**
 * @brief How much smog a place's air holds.
 * @param[in] place  the place
 * @return 0 (clean, an index of 30 or less) .. 1 (an index of 90 or more)
 */
float smogOf( const BiomeDef &place );

/**
 * @brief The air of a polluted place: a thicker haze turning brown, a paler sky, paler colours, a weaker, yellower sun.
 * @param[in] e    the air as it would be
 * @param[in] smog 0..1 (smogOf)
 * @return the smoggy air
 */
Environment withSmog( Environment e, float smog );

/**
 * @brief How many lines of data/roads.txt were wrong.
 * @return the count
 */
int roadErrors();

/**
 * @brief The speed limit on a road, as its signs give it.
 * @param[in] seg  the segment
 * @param[in] mph  in miles per hour
 * @return the number on the sign
 */
int speedLimit( const Segment &seg, bool mph );

/**
 * @brief The speed camera of a stage, if it has one.
 * @param[in]  track  the road
 * @param[in]  stage  the stage
 * @param[out] s      where it stands
 * @param[out] limit  the limit it checks (in the country's unit)
 * @param[out] mph    whether in miles per hour
 * @return true if the stage has one
 */
bool stageCamera( const Track &track, int stage, double &s, int &limit, bool &mph );

/**
 * @brief The toll plaza of a stage, if it has one.
 * @param[in]  track  the road
 * @param[in]  stage  the stage
 * @param[out] s      where it stands
 * @return true if the stage has one
 */
bool stageToll( const Track &track, int stage, double &s );

/**
 * @brief Where a stage's patrol car would wait, before the spot is checked (that needs the road laid there).
 * @param[in] track  the road
 * @param[in] stage  the stage
 * @return the distance, or -1 if the stage has none (one stage in ten has one, in a country)
 */
double policeBase( const Track &track, int stage );

/**
 * @brief The patrol car of a stage: the first clear spot from policeBase on (no tunnel, bridge, fork, gravel).
 * @param[in]  track  the road (laid to 500 m past policeBase)
 * @param[in]  stage  the stage
 * @param[out] s      where it waits
 * @param[out] side   on which verge (1 right, -1 left: the driver's side of the road)
 * @return true if it has one
 */
bool stagePolice( const Track &track, int stage, double &s, float &side );

/**
 * @brief Gives the places the roads of their countries: the lane width, how worn, gravel stretches, concrete slabs,
 *        road studs and the edges (a paved shoulder, the verge or the painted kerbs of the towns).
 * @param[in,out] table  every biome; those without a country keep their racing kerbs
 */
void applyRoadCustoms( std::vector<BiomeDef> &table );

/**
 * @brief How much of the road at a distance is gravel: the stretches where a poor country's asphalt ends.
 * @param[in] track  the road
 * @param[in] s      the distance
 * @return 0 asphalt .. 1 gravel (a place of gravel all along says so itself)
 */
float gravelAt( const Track &track, double s );

/**
 * @brief A pothole in a worn road.
 * @param[in]  track  the road
 * @param[in]  j      the segment
 * @param[out] x      its lateral offset
 * @param[out] r      its radius
 * @return true if the segment has one
 */
bool potholeAt( const Track &track, long j, float &x, float &r );

} // namespace sunstreak
