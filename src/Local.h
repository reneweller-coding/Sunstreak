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
};

constexpr double CameraAt = 650.0;   ///< where in its stage a speed camera stands (or the first clear spot after)
constexpr double LimitAt = 300.0;    ///< and the speed limit before it
constexpr double TollAt = 1300.0;    ///< where in its stage a toll plaza stands (or the first clear spot after)

/**
 * @brief A country's road customs.
 * @param[in] country  its name (nullptr: none)
 * @return them (the plain ones for none)
 */
const RoadCustoms &roadCustoms( const char *country );

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

} // namespace sunstreak
