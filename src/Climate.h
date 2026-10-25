/**
 * @file Climate.h
 * @brief The climate of the places (data/climate.txt): twelve monthly temperatures and rain sums each, from which the
 *        seasons and the showers of a place follow; and how their air changes with the month and the hour
 *        (data/smog.txt).
 * @author reneweller
 */
#pragma once

#include "Biome.h"

namespace sunstreak
{

/** @brief A place's climate: the monthly means of thirty years. */
struct PlaceClimate
{
    bool  m_known = false;   ///< the data file has a line for the place
    float m_t[12] = {};      ///< mean temperature per month (deg C, January first)
    float m_p[12] = {};      ///< precipitation per month (mm)
    float m_cold = 0.0f;     ///< the coldest month's temperature
    float m_rain = 0.0f;     ///< the year's precipitation (mm)
};

/**
 * @brief A place's climate.
 * @param[in] place  the biome
 * @return its climate, or nullptr if the data has none (the seas, the fantasy places)
 */
const PlaceClimate *placeClimate( BiomeE place );

/**
 * @brief The temperature at a time of year: the months' means joined smoothly round the year.
 * @param[in] c      the climate
 * @param[in] phase  the time of year (0: 1 January; values outside 0..1 wrap)
 * @return degrees Celsius
 */
float climateTemperature( const PlaceClimate &c, float phase );

/**
 * @brief The precipitation of the month at a time of year, the months joined smoothly round the year.
 * @param[in] c      the climate
 * @param[in] phase  the time of year (0: 1 January; values outside 0..1 wrap)
 * @return millimetres per month
 */
float climatePrecipitation( const PlaceClimate &c, float phase );

/**
 * @brief How many lines of data/climate.txt were wrong.
 * @return the count
 */
int climateErrors();

/** @brief How the air of a place changes with the month and the hour of the day (data/smog.txt): factors averaging 1. */
struct PlaceSmog
{
    bool  m_known = false;   ///< the data file has a line for the place
    float m_month[12] = {};  ///< per month (January first): 2 is twice as thick as the year's mean
    float m_hour[24] = {};   ///< per hour of the local day (midnight first)
};

/**
 * @brief A place's air profile.
 * @param[in] place  the biome
 * @return its profile, or nullptr if the data has none
 */
const PlaceSmog *placeSmog( BiomeE place );

/**
 * @brief How thick the air is at a time of year and a time of day compared with the year's mean.
 * @param[in] s      the profile
 * @param[in] phase  the time of year (0: 1 January), or below 0 for no seasonal change
 * @param[in] hour   the local hour (0..24), or below 0 for no daily change
 * @return a factor, 0.2 .. 3
 */
float smogFactor( const PlaceSmog &s, float phase, float hour );

/**
 * @brief How many lines of data/smog.txt were wrong.
 * @return the count
 */
int smogProfileErrors();

} // namespace sunstreak
