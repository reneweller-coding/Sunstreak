/**
 * @file Dsp.h
 * @brief The small signal-processing kit the sound and the music share: filters, noise, band-limited oscillators.
 * @author Rene Weller
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sunstreak::dsp
{

constexpr float SampleRate = 48000.0f;   ///< output rate
constexpr float Pi2 = 6.28318530718f;    ///< two pi

/** @brief Topology-preserving state-variable filter (Zavalishin). */
struct Svf
{
    float m_ic1 = 0;   ///< first integrator's state
    float m_ic2 = 0;   ///< second integrator's state
    float m_a1 = 1;    ///< coefficient a1
    float m_a2 = 0;    ///< coefficient a2
    float m_a3 = 0;    ///< coefficient a3
    float m_k = 1;     ///< damping (2 - 2 resonance)

    /**
     * @brief Sets cutoff and resonance.
     * @param[in] fc  cutoff in Hz
     * @param[in] q   quality
     */
    void set( float fc, float q )
    {
        const float g = std::tan( 3.14159265f * std::clamp( fc, 10.0f, SampleRate * 0.45f ) / SampleRate );
        m_k = 1.0f / q;
        m_a1 = 1.0f / ( 1.0f + g * ( g + m_k ) );
        m_a2 = g * m_a1;
        m_a3 = g * m_a2;
    }

    /**
     * @brief Filters one sample.
     * @param[in]  v0    input
     * @param[out] low   low-pass output
     * @param[out] band  band-pass output
     * @return the high-pass output
     */
    float process( float v0, float &low, float &band )
    {
        const float v3 = v0 - m_ic2;
        const float v1 = m_a1 * m_ic1 + m_a2 * v3;
        const float v2 = m_ic2 + m_a2 * m_ic1 + m_a3 * v3;
        m_ic1 = 2.0f * v1 - m_ic1;
        m_ic2 = 2.0f * v2 - m_ic2;
        low = v2;
        band = v1;
        return v0 - m_k * v1 - v2;
    }

    /**
     * @brief Low-pass only.
     * @param[in] v0  input
     * @return low-pass output
     */
    float low( float v0 )
    {
        float l, b;
        process( v0, l, b );
        return l;
    }

    /**
     * @brief Band-pass only.
     * @param[in] v0  input
     * @return band-pass output
     */
    float band( float v0 )
    {
        float l, b;
        process( v0, l, b );
        return b;
    }

    /**
     * @brief High-pass only.
     * @param[in] v0  input
     * @return high-pass output
     */
    float high( float v0 )
    {
        float l, b;
        return process( v0, l, b );
    }
};

/** @brief White noise. */
struct Noise
{
    uint32_t m_s = 0x2468ACEu;   ///< state

    /**
     * @brief The next sample.
     * @return -1..1
     */
    float next()
    {
        m_s ^= m_s << 13;
        m_s ^= m_s >> 17;
        m_s ^= m_s << 5;
        return static_cast<float>( m_s & 0xFFFFFF ) / 8388608.0f - 1.0f;
    }

    /**
     * @brief A uniform number.
     * @return 0..1
     */
    float unit() { return next() * 0.5f + 0.5f; }
};

/**
 * @brief PolyBLEP correction for a discontinuity.
 * @param[in] t   phase 0..1
 * @param[in] dt  phase increment
 * @return the correction
 */
inline float polyBlep( float t, float dt )
{
    if ( t < dt )
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if ( t > 1.0f - dt )
    {
        t = ( t - 1.0f ) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

/**
 * @brief A band-limited sawtooth step.
 * @param[in,out] phase  0..1
 * @param[in]     freq   Hz
 * @return -1..1
 */
inline float saw( float &phase, float freq )
{
    const float dt = freq / SampleRate;
    const float v = 2.0f * phase - 1.0f - polyBlep( phase, dt );
    phase += dt;
    if ( phase >= 1.0f ) phase -= 1.0f;
    return v;
}

/**
 * @brief A band-limited pulse step.
 * @param[in,out] phase  0..1
 * @param[in]     freq   Hz
 * @param[in]     duty   pulse width (0.5 square)
 * @return -1..1
 */
inline float pulse( float &phase, float freq, float duty = 0.5f )
{
    const float dt = freq / SampleRate;
    float       v = phase < duty ? 1.0f : -1.0f;
    v += polyBlep( phase, dt );
    float t2 = phase + 1.0f - duty;
    if ( t2 >= 1.0f ) t2 -= 1.0f;
    v -= polyBlep( t2, dt );
    phase += dt;
    if ( phase >= 1.0f ) phase -= 1.0f;
    return v;
}

/**
 * @brief A sine step.
 * @param[in,out] phase  0..1
 * @param[in]     freq   Hz
 * @return -1..1
 */
inline float sine( float &phase, float freq )
{
    const float v = std::sin( Pi2 * phase );
    phase += freq / SampleRate;
    if ( phase >= 1.0f ) phase -= 1.0f;
    return v;
}

/**
 * @brief MIDI note to Hz.
 * @param[in] m  the note
 * @return the frequency
 */
inline float mtof( float m )
{
    return 440.0f * std::pow( 2.0f, ( m - 69.0f ) / 12.0f );
}

}   // namespace sunstreak::dsp
