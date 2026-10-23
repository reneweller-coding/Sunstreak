/**
 * @file MathUtil.h
 * @brief Small numeric helpers the whole game shares: clamping, easing, colours and a double-precision point.
 *
 * The track runs for hundreds of kilometres, so world positions are kept in doubles (Vec3d) and turned into floats
 * only relative to a floating origin near the camera.
 *
 * @author reneweller
 */
#pragma once

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sunstreak
{

constexpr float Tau = 6.28318530718f;   ///< a full turn in radians

/**
 * @brief Clamps a value into a range.
 * @param[in] v   the value
 * @param[in] lo  the lower bound
 * @param[in] hi  the upper bound
 * @return v limited to [lo, hi]
 */
inline float clampf( float v, float lo, float hi )
{
    return std::min( std::max( v, lo ), hi );
}

/**
 * @brief Linear interpolation.
 * @param[in] a  the value at t = 0
 * @param[in] b  the value at t = 1
 * @param[in] t  the position between them
 * @return a + (b - a) t
 */
inline float lerpf( float a, float b, float t )
{
    return a + ( b - a ) * t;
}

/**
 * @brief Hermite step between two edges.
 * @param[in] e0  where the step starts (result 0)
 * @param[in] e1  where it ends (result 1)
 * @param[in] x   the input
 * @return 0..1, smooth at both ends
 */
inline float smoothstepf( float e0, float e1, float x )
{
    const float t = clampf( ( x - e0 ) / ( e1 - e0 ), 0.0f, 1.0f );
    return t * t * ( 3.0f - 2.0f * t );
}

/**
 * @brief Moves a value towards a target with a frame-rate independent exponential.
 * @param[in] current  the value now
 * @param[in] target   where it heads
 * @param[in] rate     how fast (1/s): after 1/rate seconds about 63 % of the way is done
 * @param[in] dt       the time step in seconds
 * @return the new value
 */
inline float approach( float current, float target, float rate, float dt )
{
    return current + ( target - current ) * ( 1.0f - std::exp( -rate * dt ) );
}

/**
 * @brief Blends two colours channel by channel.
 * @param[in] a  the colour at t = 0
 * @param[in] b  the colour at t = 1
 * @param[in] t  the position between them
 * @return the blended colour
 */
inline Color lerpColor( Color a, Color b, float t )
{
    t = clampf( t, 0.0f, 1.0f );
    return Color{ static_cast<unsigned char>( a.r + ( b.r - a.r ) * t ),
                  static_cast<unsigned char>( a.g + ( b.g - a.g ) * t ),
                  static_cast<unsigned char>( a.b + ( b.b - a.b ) * t ),
                  static_cast<unsigned char>( a.a + ( b.a - a.a ) * t ) };
}

/**
 * @brief Scales a colour's brightness (alpha untouched).
 * @param[in] c  the colour
 * @param[in] f  the factor
 * @return the scaled colour, clamped to 255
 */
inline Color scaleColor( Color c, float f )
{
    auto ch = []( unsigned char v, float k ) { return static_cast<unsigned char>( clampf( v * k, 0.0f, 255.0f ) ); };
    return Color{ ch( c.r, f ), ch( c.g, f ), ch( c.b, f ), c.a };
}

/**
 * @brief A colour as linear 0..1 floats (the shaders light in linear space).
 * @param[in] c  the sRGB colour
 * @return its linear RGB
 */
inline Vector3 linearRgb( Color c )
{
    auto lin = []( unsigned char v ) { return std::pow( v / 255.0f, 2.2f ); };
    return Vector3{ lin( c.r ), lin( c.g ), lin( c.b ) };
}

/** @brief A point in double precision: world positions along an endless track. */
struct Vec3d
{
    double m_x = 0.0;   ///< east
    double m_y = 0.0;   ///< up
    double m_z = 0.0;   ///< north
};

/**
 * @brief A world point relative to an origin, as floats for the GPU.
 * @param[in] p       the world point
 * @param[in] origin  the floating origin
 * @return p - origin
 */
inline Vector3 relativeTo( const Vec3d &p, const Vec3d &origin )
{
    return Vector3{ static_cast<float>( p.m_x - origin.m_x ), static_cast<float>( p.m_y - origin.m_y ),
                    static_cast<float>( p.m_z - origin.m_z ) };
}

/**
 * @brief The horizontal forward direction for a heading (0 = north, positive = turned right).
 * @param[in] heading  the yaw in radians
 * @return the unit vector
 */
inline Vector3 headingForward( float heading )
{
    return Vector3{ -std::sin( heading ), 0.0f, std::cos( heading ) };
}

/**
 * @brief The horizontal right direction for a heading.
 * @param[in] heading  the yaw in radians
 * @return the unit vector (forward x up)
 */
inline Vector3 headingRight( float heading )
{
    return Vector3{ -std::cos( heading ), 0.0f, -std::sin( heading ) };
}

/**
 * @brief A model matrix from an orthonormal basis and a position.
 *
 * Models are built with +x to the LEFT, +y up and +z forward, a right-handed frame, so no transform ever mirrors.
 *
 * @param[in] left     where the model's +x goes
 * @param[in] up       where its +y goes
 * @param[in] forward  where its +z goes
 * @param[in] pos      where its origin goes
 * @return the matrix
 */
inline Matrix basisMatrix( Vector3 left, Vector3 up, Vector3 forward, Vector3 pos )
{
    Matrix m = { 0 };
    m.m0 = left.x;
    m.m1 = left.y;
    m.m2 = left.z;
    m.m4 = up.x;
    m.m5 = up.y;
    m.m6 = up.z;
    m.m8 = forward.x;
    m.m9 = forward.y;
    m.m10 = forward.z;
    m.m12 = pos.x;
    m.m13 = pos.y;
    m.m14 = pos.z;
    m.m15 = 1.0f;
    return m;
}

} // namespace sunstreak
