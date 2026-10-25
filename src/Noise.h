/**
 * @file Noise.h
 * @brief Smooth value noise for the terrain beside the road.
 * @author Rene Weller
 */
#pragma once

#include "MathUtil.h"
#include "Rng.h"

namespace sunstreak
{

/**
 * @brief The random value at a lattice point.
 * @param[in] x     lattice x
 * @param[in] y     lattice y
 * @param[in] seed  the noise's seed
 * @return -1..1
 */
inline float latticeValue( int64_t x, int64_t y, uint64_t seed )
{
    return hashFloat( seed, static_cast<uint64_t>( x ) * 0x9E3779B1ull + static_cast<uint64_t>( y ) * 0x85EBCA77ull ) * 2.0f - 1.0f;
}

/**
 * @brief Smoothly interpolated value noise.
 * @param[in] x     position x
 * @param[in] y     position y
 * @param[in] seed  the noise's seed
 * @return -1..1
 */
inline float valueNoise( double x, double y, uint64_t seed )
{
    const double  fx = std::floor( x ), fy = std::floor( y );
    const int64_t ix = static_cast<int64_t>( fx ), iy = static_cast<int64_t>( fy );
    float         tx = static_cast<float>( x - fx ), ty = static_cast<float>( y - fy );
    tx = tx * tx * ( 3.0f - 2.0f * tx );
    ty = ty * ty * ( 3.0f - 2.0f * ty );
    const float a = latticeValue( ix, iy, seed ), b = latticeValue( ix + 1, iy, seed );
    const float c = latticeValue( ix, iy + 1, seed ), d = latticeValue( ix + 1, iy + 1, seed );
    return lerpf( lerpf( a, b, tx ), lerpf( c, d, tx ), ty );
}

/**
 * @brief Three octaves of value noise.
 * @param[in] x     position x
 * @param[in] y     position y
 * @param[in] seed  the noise's seed
 * @return roughly -1..1
 */
inline float fbm( double x, double y, uint64_t seed )
{
    return 0.57f * valueNoise( x, y, seed ) + 0.29f * valueNoise( x * 2.03, y * 2.03, seed + 1 ) +
           0.14f * valueNoise( x * 4.1, y * 4.1, seed + 2 );
}

} // namespace sunstreak
