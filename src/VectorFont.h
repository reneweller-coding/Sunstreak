/**
 * @file VectorFont.h
 * @brief A slanted stroke font for the HUD and the menus, drawn with lines: sharp at any resolution, no font file.
 * @author reneweller
 */
#pragma once

#include <raylib.h>

#include <string_view>

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
