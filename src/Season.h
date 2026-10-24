/**
 * @file Season.h
 * @brief The seasons: leaves turning in autumn, snow lying in winter, blossom in spring, by today's date or as the tour
 *        goes on; turned round south of the equator, none in the tropics, the deserts and the fantasy biomes.
 * @author reneweller
 */
#pragma once

#include "Biome.h"
#include "MeshBuilder.h"

namespace sunstreak
{

/** @brief How the seasons run in a drive. */
struct SeasonClock
{
    int   m_mode = 0;        ///< 0 as today, 1 as the tour goes on (a year every 400 km), 2 off
    float m_today = 0.0f;    ///< the time of year at the start (0: 1 January)

    /**
     * @brief The time of year at a distance.
     * @param[in] s  distance along the road
     * @return 0..1 (0: 1 January), or -1 without seasons
     */
    float phaseAt( double s ) const;

    /**
     * @brief The date at a distance, for the festivals: as phaseAt(), but today's when the seasons are off.
     * @param[in] s  distance along the road
     * @return 0..1 (0: 1 January)
     */
    float dateAt( double s ) const;
};

/**
 * @brief The time of year today.
 * @return 0..1 (0: 1 January)
 */
float todayPhase();

/**
 * @brief What the season does at a place.
 * @param[in] place  the biome
 * @param[in] phase  the time of year in the north (0: 1 January), -1 for none
 * @return its look (all zero where seasons do not reach)
 */
SeasonLook seasonAt( BiomeE place, float phase );

/**
 * @brief Colours the ground for the season: stubble and fallen leaves, snow, fresh green.
 * @param[in,out] s     the colours
 * @param[in]     look  the season
 */
void seasonSurface( Surface &s, const SeasonLook &look );

} // namespace sunstreak
