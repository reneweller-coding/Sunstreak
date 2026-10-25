/**
 * @file ScriptNames.h
 * @brief The places' names in their own script for the place signs (data/script-names.txt): strokes traced from the
 *        Noto fonts by tools/make_signs.py from the places' local lines, so the game needs no font of its own for
 *        Cyrillic, Arabic, Devanagari, the CJK scripts and the rest.
 * @author Rene Weller
 */
#pragma once

#include "Biome.h"

#include <raylib.h>

#include <utility>
#include <vector>

namespace sunstreak
{

/** @brief A name in its own script, as strokes. */
struct ScriptName
{
    float                                    m_width = 0.0f;   ///< its width in ems
    float                                    m_low = 0.0f;     ///< its lowest stroke (ems above the baseline; below it negative)
    float                                    m_high = 0.0f;    ///< and its highest
    std::vector<std::pair<Vector2, Vector2>> m_lines;          ///< the strokes in ems, x from the left, y up from the baseline
};

/**
 * @brief A place's name in its own script.
 * @param[in] place  the place
 * @return it, or nullptr where the place writes in Latin letters
 */
const ScriptName *scriptName( BiomeE place );

/**
 * @brief The lines of data/script-names.txt that could not be read or name no place.
 * @return their number
 */
int scriptNameErrors();

} // namespace sunstreak
