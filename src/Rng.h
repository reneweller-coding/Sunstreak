/**
 * @file Rng.h
 * @brief Deterministic random numbers: a small generator for sequences and stateless hashes for placement.
 *
 * The same seed must give the same track on every machine, so nothing here touches std::random_device or the
 * standard distributions (whose results differ between libraries).
 *
 * @author Rene Weller
 */
#pragma once

#include <cstdint>

namespace sunstreak
{

/**
 * @brief SplitMix64's finaliser: a good 64-bit mix.
 * @param[in] x  the input
 * @return the scrambled value
 */
inline uint64_t hash64( uint64_t x )
{
    x += 0x9E3779B97F4A7C15ull;
    x = ( x ^ ( x >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
    x = ( x ^ ( x >> 27 ) ) * 0x94D049BB133111EBull;
    return x ^ ( x >> 31 );
}

/**
 * @brief A stateless random number in [0, 1) for a pair of keys.
 * @param[in] a  the first key (a seed)
 * @param[in] b  the second key (an index)
 * @return the number
 */
inline float hashFloat( uint64_t a, uint64_t b )
{
    return static_cast<float>( hash64( a ^ hash64( b ) ) >> 40 ) * ( 1.0f / 16777216.0f );
}

/** @brief xoshiro-style 64-bit generator seeded through SplitMix64. */
class Rng
{
public:
    /**
     * @brief Seeds the generator.
     * @param[in] seed  any value; equal seeds give equal sequences
     */
    explicit Rng( uint64_t seed = 1 ) : m_state( hash64( seed ) | 1ull ) {}

    /**
     * @brief The next raw value.
     * @return 64 random bits
     */
    uint64_t next()
    {
        m_state ^= m_state << 13;
        m_state ^= m_state >> 7;
        m_state ^= m_state << 17;
        return hash64( m_state );
    }

    /**
     * @brief A uniform float.
     * @return a number in [0, 1)
     */
    float uniform() { return static_cast<float>( next() >> 40 ) * ( 1.0f / 16777216.0f ); }

    /**
     * @brief A uniform float in a range.
     * @param[in] lo  the lower bound
     * @param[in] hi  the upper bound
     * @return a number in [lo, hi)
     */
    float range( float lo, float hi ) { return lo + ( hi - lo ) * uniform(); }

    /**
     * @brief A uniform integer in a closed range.
     * @param[in] lo  the smallest value
     * @param[in] hi  the largest value
     * @return a number in [lo, hi]
     */
    int rangeInt( int lo, int hi ) { return lo + static_cast<int>( next() % static_cast<uint64_t>( hi - lo + 1 ) ); }

    /**
     * @brief A coin with a given chance.
     * @param[in] p  the probability of true
     * @return true with probability p
     */
    bool chance( float p ) { return uniform() < p; }

    /**
     * @brief Plus or minus one.
     * @return -1 or +1 with equal chance
     */
    float sign() { return ( next() & 1 ) ? 1.0f : -1.0f; }

private:
    uint64_t m_state;   ///< the generator's state, never zero
};

} // namespace sunstreak
