/**
 * @file Festival.h
 * @brief The festivals of data/festivals.txt: when they come, where, and what they bring.
 * @author reneweller
 */
#pragma once

#include "Biome.h"

#include <string>
#include <vector>

namespace sunstreak
{

/** @brief A festival: its days, its places, and what it brings to the road. */
struct Festival
{
    std::string              m_name;         ///< what it is called
    std::string              m_what;         ///< a line about it
    int                      m_from = 0;     ///< its first day (0: 1 January)
    int                      m_to = 0;       ///< its last day
    std::vector<std::string> m_where;        ///< countries, COUNTRY/REGION, or * (everywhere)
    float                    m_north = 0.0f; ///< every place at least this far north as well (0: none)
    std::vector<Color>       m_bunting;      ///< flags on strings across the road
    std::vector<Color>       m_lanterns;     ///< glowing lanterns on strings across the road
    std::vector<Color>       m_lights;       ///< fairy lights on strings across the road
    std::vector<Color>       m_lamps;        ///< little lamps along both edges
    std::vector<Color>       m_stalls;       ///< market stalls beside the road (roof, walls)
    std::vector<Color>       m_tents;        ///< beer tents beside the road (roof, stripes)
    std::vector<Color>       m_confetti;     ///< colour in the air
    std::vector<Color>       m_fireworks;    ///< fireworks at night
    bool                     m_trees = false;    ///< Christmas trees
    bool                     m_aurora = false;   ///< the northern lights at night
};

/**
 * @brief The festivals, read from the data (again after a reload).
 * @return them, in the file's order
 */
const std::vector<Festival> &festivals();

/**
 * @brief How many lines of the festivals file were wrong.
 * @return the count
 */
int festivalErrors();

/**
 * @brief The festival a place celebrates at a time of year.
 * @param[in] place  the biome
 * @param[in] phase  the time of year (0: 1 January), -1 for none
 * @return its index in festivals(), or -1
 */
int festivalAt( BiomeE place, float phase );

/**
 * @brief A date as the time of year.
 * @param[in] mmdd  "MM-DD"
 * @return 0..1 (0: 1 January), or -1 if it is no date
 */
float dateFromText( const std::string &mmdd );

} // namespace sunstreak
