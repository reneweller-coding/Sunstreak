/**
 * @file UiCommon.h
 * @brief What the game's flow and its screens share: colours, text formatting, panels, seeds, the blended biome.
 * @author reneweller
 */
#pragma once

#include "Biome.h"
#include "Rng.h"
#include "Track.h"

#include <raylib.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace sunstreak::ui
{

constexpr Color Accent = { 255, 150, 40, 255 };   ///< sunset orange
constexpr Color Cyan = { 70, 220, 255, 255 };     ///< boost, near misses
constexpr Color Green = { 90, 255, 150, 255 };    ///< checkpoints
constexpr Color Red = { 255, 60, 60, 255 };       ///< warnings
constexpr Color Ink = { 245, 246, 250, 255 };     ///< text
constexpr Color Dim = { 190, 196, 210, 255 };     ///< secondary text
constexpr Color Gold = { 255, 210, 80, 255 };     ///< first place

/**
 * @brief printf into a std::string.
 * @param[in] f  the format
 * @return the text
 */
inline std::string format( const char *f, ... )
{
    char    buf[256];
    va_list args;
    va_start( args, f );
    std::vsnprintf( buf, sizeof( buf ), f, args );
    va_end( args );
    return buf;
}

/**
 * @brief A time as minutes, seconds and tenths.
 * @param[in] seconds  the time
 * @return "1:23.4"
 */
inline std::string clockText( float seconds )
{
    const int tenths = static_cast<int>( seconds * 10.0f + 0.5f );
    return format( "%d:%02d.%d", tenths / 600, ( tenths / 10 ) % 60, tenths % 10 );
}

/**
 * @brief A fresh random seed (29 bits, never zero: it fits a track code).
 * @return the seed
 */
inline uint64_t randomSeed()
{
    const auto t = static_cast<uint64_t>( std::chrono::high_resolution_clock::now().time_since_epoch().count() );
    return ( hash64( t ) & 0x1FFFFFFFull ) | 1ull;
}

/**
 * @brief The blended environment at a distance.
 * @param[in] track  the road
 * @param[in] s      the distance
 * @return light and air
 */
inline Environment environmentAt( const Track &track, double s )
{
    const BiomeMix m = track.biomeMix( s );
    return blendEnvironment( biomeDef( m.m_from ).m_env, biomeDef( m.m_to ).m_env, m.m_t );
}

/**
 * @brief The blended surface at a distance.
 * @param[in] track  the road
 * @param[in] s      the distance
 * @return colours
 */
inline Surface surfaceAt( const Track &track, double s )
{
    const BiomeMix m = track.biomeMix( s );
    return blendSurface( biomeDef( m.m_from ).m_surface, biomeDef( m.m_to ).m_surface, m.m_t );
}

/**
 * @brief Draws a rounded translucent panel.
 * @param[in] x      left
 * @param[in] y      top
 * @param[in] w      width
 * @param[in] h      height
 * @param[in] alpha  opacity 0..255
 */
inline void panel( float x, float y, float w, float h, unsigned char alpha = 170 )
{
    DrawRectangleRounded( Rectangle{ x, y, w, h }, 0.08f, 8, Color{ 8, 10, 22, alpha } );
    DrawRectangleRoundedLinesEx( Rectangle{ x, y, w, h }, 0.08f, 8, 1.5f, Color{ 255, 255, 255, 40 } );
}

/**
 * @brief The UI's unit: 1 at 720 lines, smaller in narrow windows.
 * @return pixels per unit
 */
inline float unit()
{
    return std::min( static_cast<float>( GetScreenHeight() ) / 720.0f, static_cast<float>( GetScreenWidth() ) / 1150.0f );
}

} // namespace sunstreak::ui
