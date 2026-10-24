/**
 * @file VectorFont.h
 * @brief A slanted stroke font for the HUD and the menus, drawn with lines: sharp at any resolution, no font file.
 * @author reneweller
 */
#pragma once

#include <raylib.h>

#include <string_view>
#include <utility>
#include <vector>

namespace sunstreak
{

/** @brief Horizontal alignment of a text. */
enum class AlignE
{
    LEFT,     ///< x is the left edge
    CENTER,   ///< x is the centre
    RIGHT,    ///< x is the right edge
};

/**
 * @brief The width of a text.
 * @param[in] text  the text (upper case; lower case is drawn as upper case)
 * @param[in] size  cap height in pixels
 * @return the width in pixels
 */
float textWidth( std::string_view text, float size );

/**
 * @brief Scales every text the UI draws (larger text for reading at a distance).
 * @param[in] scale  1 normal
 */
void setTextScale( float scale );

/**
 * @brief The UI's text scale.
 * @return 1 normal
 */
float textScale();

/**
 * @brief The strokes of a text, upright, for building it into a sign: cap height 1, the baseline at y 0, x to the right.
 * @param[in]  text   the text (upper case)
 * @param[out] lines  segments, as pairs of points
 * @return the text's width
 */
float textStrokes( std::string_view text, std::vector<std::pair<Vector2, Vector2>> &lines );

/**
 * @brief Draws a text.
 * @param[in] text    the text
 * @param[in] x       horizontal anchor (see align)
 * @param[in] y       top of the capitals
 * @param[in] size    cap height in pixels
 * @param[in] color   stroke colour
 * @param[in] align   alignment
 * @param[in] weight  stroke width as a share of the size (0.12 regular)
 */
void drawText( std::string_view text, float x, float y, float size, Color color, AlignE align = AlignE::LEFT,
               float weight = 0.13f );

/**
 * @brief Draws a text with a dark drop shadow and a soft glow, the HUD's style.
 * @param[in] text   the text
 * @param[in] x      horizontal anchor
 * @param[in] y      top of the capitals
 * @param[in] size   cap height in pixels
 * @param[in] color  stroke colour
 * @param[in] align  alignment
 * @param[in] glow   0..1 glow strength
 */
void drawTextFx( std::string_view text, float x, float y, float size, Color color, AlignE align = AlignE::LEFT,
                 float glow = 0.5f );

} // namespace sunstreak
