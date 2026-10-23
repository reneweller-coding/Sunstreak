/**
 * @file Audio.cpp
 * @brief Live synthesis: engine, tyres, wind, effects and the generative soundtrack.
 * @author reneweller
 */
#include "Audio.h"

#include "Rng.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <memory>

namespace sunstreak
{

namespace
{

constexpr float SampleRate = 48000.0f;      ///< output rate
constexpr float Pi2 = 6.28318530718f;       ///< two pi
constexpr int   VoiceCount = 24;            ///< simultaneous effects
constexpr float MasterGain = 0.35f;         ///< overall level before the soft clip (about -9 dB)

/** @brief Topology-preserving state-variable filter (Zavalishin). */
struct Svf
{
    float m_ic1 = 0, m_ic2 = 0;                    ///< integrator states
    float m_a1 = 1, m_a2 = 0, m_a3 = 0, m_k = 1;   ///< coefficients

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
float polyBlep( float t, float dt )
{
    if( t < dt )
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if( t > 1.0f - dt )
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
float saw( float &phase, float freq )
{
    const float dt = freq / SampleRate;
    float       v = 2.0f * phase - 1.0f - polyBlep( phase, dt );
    phase += dt;
    if( phase >= 1.0f ) phase -= 1.0f;
    return v;
}

/**
 * @brief A band-limited square step.
 * @param[in,out] phase  0..1
 * @param[in]     freq   Hz
 * @return -1..1
 */
float square( float &phase, float freq )
{
    const float dt = freq / SampleRate;
    float       v = phase < 0.5f ? 1.0f : -1.0f;
    v += polyBlep( phase, dt );
    float t2 = phase + 0.5f;
    if( t2 >= 1.0f ) t2 -= 1.0f;
    v -= polyBlep( t2, dt );
    phase += dt;
    if( phase >= 1.0f ) phase -= 1.0f;
    return v;
}

/**
 * @brief MIDI note to Hz.
 * @param[in] m  the note
 * @return the frequency
 */
float mtof( float m )
{
    return 440.0f * std::pow( 2.0f, ( m - 69.0f ) / 12.0f );
}

/** @brief A posted effect. */
struct Event
{
    SfxE  m_type = SfxE::TICK;   ///< which
    float m_strength = 1.0f;     ///< 0..1
    float m_pan = 0.0f;          ///< -1..1
};

/** @brief One sounding effect. */
struct Voice
{
    bool  m_on = false;          ///< sounding
    SfxE  m_type = SfxE::TICK;   ///< which
    float m_t = 0;               ///< seconds since start
    float m_strength = 1;        ///< 0..1
    float m_pan = 0;             ///< -1..1
    float m_a = 0, m_b = 0;      ///< per-type state
    float m_phase = 0;           ///< oscillator
    Svf   m_f;                   ///< filter
    Noise m_n;                   ///< noise
};

/** @brief The generated soundtrack: a small arcade house/synth piece per seed. */
struct MusicSynth
{
    bool   m_on = false;          ///< playing
    float  m_gain = 0.0f;         ///< fade
    float  m_bpm = 132.0f;        ///< tempo
    int    m_key = 0;             ///< transposition
    int    m_prog[4] = {};        ///< chord roots (semitones)
    bool   m_minor[4] = {};       ///< chord qualities
    int    m_bassStyle = 0;       ///< bass pattern
    int    m_arpStyle = 0;        ///< arpeggio pattern
    int    m_lead[16] = {};       ///< two bars of eighths: scale step or -1
    double m_clock = 0.0;         ///< samples into the current step
    int    m_step = 0;            ///< sixteenth counter
    float  m_kickT = 9, m_kickPh = 0, m_snareT = 9, m_snareAmp = 1, m_hatT = 9;
    bool   m_hatOpen = false;
    float  m_bassT = 9, m_bassF = 55, m_bassPh = 0;
    float  m_arpT = 9, m_arpF = 440, m_arpPh = 0;
    float  m_leadT = 9, m_leadF = 880, m_leadPh = 0, m_leadEnv = 0;
    bool   m_leadGate = false;
    float  m_padF[3] = { 110, 130, 165 }, m_padPh[6] = {}, m_padEnv = 0;
    bool   m_padOn = false;
    Svf    m_snareF, m_hatF, m_bassFl, m_arpFl, m_padFl, m_leadFl;
    Noise  m_n;
    std::vector<float> m_delayL, m_delayR;   ///< ping-pong echo
    size_t m_di = 0;                          ///< echo write index

    /**
     * @brief Composes a new piece.
     * @param[in] seed  its seed
     */
    void start( uint64_t seed )
    {
        Rng r( seed * 2654435761ull + 17 );
        m_bpm = 126.0f + 4.0f * static_cast<float>( r.rangeInt( 0, 4 ) );
        m_key = r.rangeInt( -5, 6 );
        static const int progs[5][4] = { { 0, 8, 3, 10 }, { 0, 5, 8, 7 }, { 0, 10, 8, 10 }, { 8, 10, 0, 0 }, { 0, 3, 10, 5 } };
        static const bool minors[5][4] = { { true, false, false, false }, { true, true, false, false }, { true, false, false, false },
                                           { false, false, true, true }, { true, false, false, true } };
        const int p = r.rangeInt( 0, 4 );
        for( int i = 0; i < 4; ++i )
        {
            m_prog[i] = progs[p][i];
            m_minor[i] = minors[p][i];
        }
        m_bassStyle = r.rangeInt( 0, 2 );
        m_arpStyle = r.rangeInt( 0, 2 );
        int deg = r.rangeInt( 2, 5 );
        for( int i = 0; i < 16; ++i )
        {
            if( i % 4 != 0 && r.chance( 0.3f ) )
            {
                m_lead[i] = -1;
                continue;
            }
            deg = std::clamp( deg + r.rangeInt( -2, 2 ), 0, 9 );
            m_lead[i] = deg;
        }
        m_step = 0;
        m_clock = 0.0;
        m_on = true;
        m_delayL.assign( static_cast<size_t>( SampleRate ), 0.0f );
        m_delayR.assign( static_cast<size_t>( SampleRate ), 0.0f );
        m_snareF.set( 1900.0f, 0.7f );
        m_hatF.set( 7500.0f, 0.7f );
        m_padFl.set( 1100.0f, 0.8f );
        m_arpFl.set( 2600.0f, 0.9f );
        m_leadFl.set( 3200.0f, 0.8f );
    }

    /** @brief Triggers what happens on the current sixteenth. */
    void onStep()
    {
        const int s16 = m_step % 16;
        const int bar = ( m_step / 16 ) % 32;
        const bool groove = bar >= 4;
        const bool breakdown = bar >= 24 && bar < 28;
        const bool build = bar >= 28;
        const bool kickOn = groove && !breakdown;
        const bool bassOn = groove && !breakdown;
        const bool arpOn = ( bar >= 8 && bar < 24 ) || breakdown || build;
        const bool leadOn = bar >= 12 && bar < 24;
        const bool snareOn = bar >= 8 && !breakdown;
        const int  ci = bar % 4;
        const int  root = m_key + m_prog[ci];
        const int  third = m_minor[ci] ? 3 : 4;
        if( s16 == 0 )
        {
            m_padF[0] = mtof( static_cast<float>( 57 + root ) );
            m_padF[1] = mtof( static_cast<float>( 57 + root + third ) );
            m_padF[2] = mtof( static_cast<float>( 57 + root + 7 ) );
            m_padOn = true;
        }
        if( kickOn && s16 % 4 == 0 )
        {
            m_kickT = 0.0f;
            m_kickPh = 0.0f;
        }
        if( snareOn && ( s16 == 4 || s16 == 12 ) )
        {
            m_snareT = 0.0f;
            m_snareAmp = 1.0f;
        }
        if( build && bar == 31 && s16 % 2 == 0 )
        {
            m_snareT = 0.0f;
            m_snareAmp = 0.4f + 0.04f * static_cast<float>( s16 );
        }
        if( groove )
        {
            if( s16 % 4 == 2 )
            {
                m_hatT = 0.0f;
                m_hatOpen = true;
            }
            else if( s16 % 2 == 1 )
            {
                m_hatT = 0.0f;
                m_hatOpen = false;
            }
        }
        if( bassOn )
        {
            bool hit = false;
            bool up = false;
            switch( m_bassStyle )
            {
            case 0: hit = true; up = s16 % 2 == 1; break;
            case 1: hit = s16 % 4 == 2 || s16 % 4 == 3; up = s16 % 4 == 3; break;
            default: hit = s16 % 2 == 0 || s16 % 8 == 7; up = s16 % 8 == 4; break;
            }
            if( hit )
            {
                m_bassF = mtof( static_cast<float>( 33 + root + ( up ? 12 : 0 ) ) );
                m_bassT = 0.0f;
            }
        }
        if( arpOn )
        {
            static const int pats[3][8] = { { 0, 1, 2, 3, 2, 1, 2, 3 }, { 0, 2, 1, 3, 0, 2, 3, 2 }, { 3, 2, 1, 0, 1, 2, 3, 2 } };
            const int tones[4] = { 0, third, 7, 12 };
            m_arpF = mtof( static_cast<float>( 69 + root + tones[pats[m_arpStyle][s16 % 8]] ) );
            m_arpT = 0.0f;
        }
        if( leadOn && s16 % 2 == 0 )
        {
            static const int scale[10] = { 0, 2, 3, 5, 7, 8, 10, 12, 14, 15 };
            const int idx = ( ( bar % 2 ) * 8 + s16 / 2 ) % 16;
            int       note = m_lead[idx];
            if( bar % 4 == 3 && idx >= 12 ) note = idx % 2 ? -1 : 4;   // a turnaround every four bars
            if( note >= 0 )
            {
                m_leadF = mtof( static_cast<float>( 69 + m_key + scale[note] ) );
                m_leadT = 0.0f;
                m_leadGate = true;
            }
            else
            {
                m_leadGate = false;
            }
        }
        else if( !leadOn )
        {
            m_leadGate = false;
        }
    }

    /**
     * @brief Renders one stereo sample.
     * @param[out] l  left
     * @param[out] r  right
     */
    void tick( float &l, float &r )
    {
        l = r = 0.0f;
        if( !m_on && m_gain < 1e-4f ) return;
        m_gain += ( ( m_on ? 1.0f : 0.0f ) - m_gain ) * 0.00004f;
        const double stepLen = static_cast<double>( SampleRate ) * 60.0 / static_cast<double>( m_bpm ) / 4.0;
        m_clock += 1.0;
        if( m_clock >= stepLen )
        {
            m_clock -= stepLen;
            ++m_step;
            onStep();
        }
        const float dt = 1.0f / SampleRate;
        float       drums = 0.0f, tonal = 0.0f, echo = 0.0f;
        if( m_kickT < 1.0f )
        {
            m_kickPh += ( 48.0f + 150.0f * std::exp( -m_kickT * 30.0f ) ) * dt;
            drums += std::sin( Pi2 * m_kickPh ) * std::exp( -m_kickT * 5.5f ) * 0.95f;
            m_kickT += dt;
        }
        const float duck = 1.0f - 0.55f * std::exp( -m_kickT * 8.0f );
        if( m_snareT < 0.6f )
        {
            drums += ( m_snareF.band( m_n.next() ) * 1.6f * std::exp( -m_snareT * 15.0f ) +
                       std::sin( Pi2 * 185.0f * m_snareT ) * std::exp( -m_snareT * 24.0f ) * 0.35f ) * m_snareAmp * 0.55f;
            m_snareT += dt;
        }
        if( m_hatT < 0.5f )
        {
            float lo, bd;
            const float hp = m_hatF.process( m_n.next(), lo, bd );
            drums += hp * std::exp( -m_hatT * ( m_hatOpen ? 13.0f : 60.0f ) ) * ( m_hatOpen ? 0.16f : 0.1f );
            m_hatT += dt;
        }
        if( m_bassT < 1.0f )
        {
            m_bassFl.set( 180.0f + 1700.0f * std::exp( -m_bassT * 14.0f ), 1.2f );
            tonal += m_bassFl.low( saw( m_bassPh, m_bassF ) ) * std::exp( -m_bassT * 2.5f ) * 0.42f * duck;
            m_bassT += dt;
        }
        if( m_arpT < 0.8f )
        {
            const float a = m_arpFl.low( square( m_arpPh, m_arpF ) ) * std::exp( -m_arpT * 9.0f ) * 0.12f * duck;
            tonal += a;
            echo += a;
            m_arpT += dt;
        }
        m_padEnv += ( ( m_padOn ? 1.0f : 0.0f ) - m_padEnv ) * 0.00003f;
        if( m_padEnv > 1e-4f )
        {
            float p = 0.0f;
            for( int i = 0; i < 3; ++i )
            {
                p += saw( m_padPh[i * 2], m_padF[i] * 1.004f );
                p += saw( m_padPh[i * 2 + 1], m_padF[i] * 0.996f );
            }
            tonal += m_padFl.low( p ) * m_padEnv * 0.045f * duck;
        }
        m_leadEnv += ( ( m_leadGate ? ( m_leadT < 0.05f ? 1.0f : 0.65f ) : 0.0f ) - m_leadEnv ) * 0.004f;
        if( m_leadEnv > 1e-4f )
        {
            const float vib = m_leadT > 0.15f ? 1.0f + 0.004f * std::sin( Pi2 * 5.5f * m_leadT ) : 1.0f;
            const float a = m_leadFl.low( square( m_leadPh, m_leadF * vib ) ) * m_leadEnv * 0.09f;
            tonal += a;
            echo += a * 0.8f;
            m_leadT += dt;
        }
        // Ping-pong echo, three sixteenths.
        const size_t n = m_delayL.size();
        const size_t len = std::min( n - 1, static_cast<size_t>( stepLen * 3.0 ) );
        const size_t rd = ( m_di + n - len ) % n;
        const float  dl = m_delayL[rd], dr = m_delayR[rd];
        m_delayL[m_di] = echo + dr * 0.38f;
        m_delayR[m_di] = dl * 0.38f;
        m_di = ( m_di + 1 ) % n;
        const float g = m_gain * 0.5f;
        l = ( drums + tonal + dl * 0.6f ) * g;
        r = ( drums + tonal + dr * 0.6f ) * g;
    }
};

/** @brief The synthesiser shared with the audio thread. */
struct Synth
{
    std::atomic<float> m_rpm{ 0.1f }, m_throttle{ 0 }, m_speed{ 0 }, m_slip{ 0 }, m_offroad{ 0 }, m_rumble{ 0 };
    std::atomic<bool>  m_active{ false }, m_boost{ false }, m_limiter{ false }, m_shifting{ false };
    std::atomic<float> m_sfxVol{ 0.9f }, m_musicVol{ 0.7f };
    std::atomic<int>   m_musicCmd{ 0 };      ///< 1 start, 2 stop
    std::atomic<uint64_t> m_musicSeed{ 1 };

    std::array<Event, 64>  m_queue;          ///< effects posted by the game
    std::atomic<uint32_t>  m_head{ 0 };      ///< written by the game thread
    std::atomic<uint32_t>  m_tail{ 0 };      ///< read by the audio thread

    // Audio-thread state.
    float  e_rpm = 0.1f, e_thr = 0, e_speed = 0, e_slip = 0, e_off = 0, e_rumble = 0, e_active = 0, e_boost = 0, e_spool = 0;
    float  e_phase = 0, e_sub = 0, e_pulseAmp = 1, e_uneven = 1, e_pop = 0, e_shift = 1, e_prevThr = 0;
    float  e_whistle = 0, e_squeal = 0, e_vib = 0, e_rumblePh = 0, e_limPh = 0;
    int    e_count = 0, e_coef = 0;
    bool   e_wasShifting = false;
    Svf    e_body, e_intake, e_wind, e_squealF, e_gravel, e_rumbleF, e_hiss;
    Noise  e_n;
    float  e_width[64] = {};
    int    e_wi = 0;
    std::array<Voice, VoiceCount> m_voices;
    MusicSynth m_music;

    /**
     * @brief Starts an effect voice.
     * @param[in] e  the effect
     */
    void startVoice( const Event &e )
    {
        Voice *v = nullptr;
        for( Voice &c : m_voices )
            if( !c.m_on )
            {
                v = &c;
                break;
            }
        if( !v ) v = &m_voices[static_cast<size_t>( e_count ) % m_voices.size()];
        *v = Voice{};
        v->m_on = true;
        v->m_type = e.m_type;
        v->m_strength = e.m_strength;
        v->m_pan = e.m_pan;
        v->m_n.m_s = 0x9E37u + static_cast<uint32_t>( e_count * 7919 );
        if( e.m_type == SfxE::BACKFIRE ) v->m_a = 0.0f;
    }

    /**
     * @brief Renders one effect voice.
     * @param[in,out] v  the voice
     * @return its mono sample
     */
    float renderVoice( Voice &v )
    {
        const float t = v.m_t;
        float       y = 0.0f, dur = 1.0f;
        auto        tone = [&]( float f, float decay ) { return std::sin( Pi2 * f * t ) * std::exp( -t * decay ); };
        switch( v.m_type )
        {
        case SfxE::SHIFT:
            dur = 0.12f;
            v.m_f.set( 300.0f, 0.7f );
            y = v.m_f.low( v.m_n.next() ) * std::exp( -t * 40.0f ) * 1.2f;
            break;
        case SfxE::BACKFIRE:
            dur = 0.45f;
            if( t >= v.m_a )
            {
                v.m_b = t;
                v.m_a = t + 0.03f + 0.07f * v.m_n.unit();
            }
            v.m_f.set( 1300.0f, 0.6f );
            y = ( v.m_f.low( v.m_n.next() ) * 2.2f + std::sin( Pi2 * 90.0f * ( t - v.m_b ) ) * 0.6f ) * std::exp( -( t - v.m_b ) * 70.0f ) * 0.7f;
            break;
        case SfxE::BLOWOFF:
            dur = 0.5f;
            v.m_f.set( 3800.0f - 1500.0f * t, 1.4f );
            y = v.m_f.band( v.m_n.next() ) * std::exp( -t * 7.0f ) * 0.5f;
            break;
        case SfxE::CRASH:
            dur = 1.0f;
            v.m_f.set( 900.0f, 0.7f );
            y = tone( 55.0f, 6.0f ) * 0.9f + v.m_f.low( v.m_n.next() ) * std::exp( -t * 5.0f ) * 1.4f +
                v.m_n.next() * std::exp( -t * 14.0f ) * 0.25f;
            break;
        case SfxE::BUMP:
            dur = 0.35f;
            v.m_f.set( 600.0f, 0.7f );
            y = tone( 70.0f, 12.0f ) * 0.8f + v.m_f.low( v.m_n.next() ) * std::exp( -t * 18.0f ) * 0.9f;
            break;
        case SfxE::CONE:
            dur = 0.18f;
            v.m_f.set( 1500.0f, 2.0f );
            y = tone( 420.0f, 35.0f ) * 0.6f + v.m_f.band( v.m_n.next() ) * std::exp( -t * 45.0f ) * 0.9f;
            break;
        case SfxE::WHOOSH:
        {
            dur = 0.6f;
            v.m_f.set( 2600.0f * std::exp( -t * 3.5f ) + 260.0f, 1.1f );
            const float env = std::sin( 3.14159f * std::min( 1.0f, t / dur ) );
            y = v.m_f.band( v.m_n.next() ) * env * env * 1.4f;
            break;
        }
        case SfxE::LAND:
            dur = 0.4f;
            v.m_f.set( 500.0f, 0.7f );
            y = tone( 50.0f, 10.0f ) * 0.9f + v.m_f.low( v.m_n.next() ) * std::exp( -t * 14.0f ) * 0.7f;
            break;
        case SfxE::CHECKPOINT:
        {
            dur = 0.9f;
            static const float notes[4] = { 72, 76, 79, 84 };
            const int i = std::min( 3, static_cast<int>( t / 0.08f ) );
            const float lt = t - static_cast<float>( i ) * 0.08f;
            y = std::tanh( std::sin( Pi2 * mtof( notes[i] ) * t ) * 2.5f ) * std::exp( -lt * ( i == 3 ? 5.0f : 14.0f ) ) * 0.3f;
            break;
        }
        case SfxE::NEAR_MISS:
            dur = 0.25f;
            v.m_phase += ( 700.0f + 1600.0f * t / 0.25f ) / SampleRate;
            y = std::sin( Pi2 * v.m_phase ) * std::exp( -t * 9.0f ) * 0.3f;
            break;
        case SfxE::BEEP: dur = 0.16f; y = std::tanh( std::sin( Pi2 * 880.0f * t ) * 3.0f ) * 0.22f; break;
        case SfxE::GO: dur = 0.5f; y = std::tanh( std::sin( Pi2 * 1760.0f * t ) * 3.0f ) * 0.22f * std::exp( -t * 3.0f ); break;
        case SfxE::TICK: dur = 0.03f; y = std::sin( Pi2 * 1600.0f * t ) * 0.18f; break;
        case SfxE::SELECT:
            dur = 0.14f;
            y = std::sin( Pi2 * ( t < 0.06f ? 880.0f : 1320.0f ) * t ) * 0.22f;
            break;
        case SfxE::WARNING: dur = 0.09f; y = std::tanh( std::sin( Pi2 * 1200.0f * t ) * 3.0f ) * 0.18f; break;
        case SfxE::BOOST:
            dur = 0.7f;
            v.m_f.set( 400.0f + 3600.0f * t / 0.7f, 1.0f );
            y = v.m_f.band( v.m_n.next() ) * std::sin( 3.14159f * t / 0.7f ) * 1.2f + tone( 60.0f, 8.0f ) * 0.4f;
            break;
        case SfxE::STAGE:
            dur = 1.4f;
            y = ( std::sin( Pi2 * 523.25f * t ) + std::sin( Pi2 * 659.25f * t ) + std::sin( Pi2 * 783.99f * t ) ) *
                std::min( 1.0f, t * 8.0f ) * std::exp( -t * 2.2f ) * 0.09f;
            break;
        case SfxE::GAMEOVER:
        {
            dur = 1.6f;
            static const float notes[4] = { 72, 67, 63, 60 };
            const int i = std::min( 3, static_cast<int>( t / 0.22f ) );
            y = std::tanh( std::sin( Pi2 * mtof( notes[i] ) * t ) * 2.0f ) * std::exp( -( t - 0.22f * i ) * 4.0f ) * 0.22f;
            break;
        }
        default: break;
        }
        v.m_t += 1.0f / SampleRate;
        if( v.m_t > dur ) v.m_on = false;
        return y * v.m_strength;
    }

    /**
     * @brief Renders one engine sample.
     * @return the mono engine, tyre and environment sound
     */
    float renderEngine()
    {
        const float rpmT = m_rpm.load( std::memory_order_relaxed );
        const float thrT = m_throttle.load( std::memory_order_relaxed );
        e_rpm += ( rpmT - e_rpm ) * 0.0012f;
        e_thr += ( thrT - e_thr ) * 0.003f;
        e_speed += ( m_speed.load( std::memory_order_relaxed ) - e_speed ) * 0.001f;
        e_slip += ( m_slip.load( std::memory_order_relaxed ) - e_slip ) * 0.002f;
        e_off += ( m_offroad.load( std::memory_order_relaxed ) - e_off ) * 0.002f;
        e_rumble += ( m_rumble.load( std::memory_order_relaxed ) - e_rumble ) * 0.004f;
        e_active += ( ( m_active.load( std::memory_order_relaxed ) ? 1.0f : 0.0f ) - e_active ) * 0.0005f;
        e_boost += ( ( m_boost.load( std::memory_order_relaxed ) ? 1.0f : 0.0f ) - e_boost ) * 0.0008f;
        const float load = std::min( 1.0f, e_thr + 0.3f * e_boost );
        const float spoolTarget = e_thr * std::min( 1.0f, e_rpm * 1.2f );
        e_spool += ( spoolTarget - e_spool ) * ( spoolTarget > e_spool ? 0.00012f : 0.0004f );
        if( e_prevThr > 0.7f && e_thr < 0.45f && e_spool > 0.4f && e_active > 0.5f )
        {
            startVoice( Event{ SfxE::BLOWOFF, e_spool * 0.9f, 0.0f } );
            e_spool *= 0.5f;
        }
        e_prevThr = e_thr;

        const float fire = ( 900.0f + e_rpm * 6900.0f ) / 60.0f * 4.0f;
        if( ( e_coef++ & 31 ) == 0 )
        {
            e_body.set( 250.0f + 2600.0f * ( 0.25f + 0.75f * load ) * ( 0.35f + e_rpm ), 0.9f );
            e_intake.set( std::min( fire * 2.0f, 8000.0f ), 4.0f );
            e_wind.set( 250.0f + e_speed * 18.0f, 0.6f );
            e_squealF.set( 1800.0f, 6.0f );
            e_gravel.set( 900.0f, 0.6f );
            e_rumbleF.set( 350.0f, 0.8f );
            e_hiss.set( 5200.0f, 0.7f );
        }

        // Firing pulses of a cross-plane V8: uneven, slightly random.
        e_phase += fire / SampleRate;
        if( e_phase >= 1.0f )
        {
            e_phase -= 1.0f;
            ++e_count;
            e_pulseAmp = 0.8f + 0.35f * e_n.unit();
            const int k = e_count & 3;
            e_uneven = k == 1 ? 1.3f : k == 3 ? 0.85f : 1.0f;
            if( e_thr < 0.1f && e_rpm > 0.55f && e_n.unit() < 0.02f * e_rpm ) e_pop = 1.0f;
        }
        const float decay = 4.0f + 10.0f * ( 1.0f - load * 0.8f );
        float       pulse = std::exp( -e_phase * decay ) * e_pulseAmp * e_uneven - ( 1.0f - std::exp( -decay ) ) / decay;
        e_sub += fire * 0.5f / SampleRate;
        if( e_sub >= 1.0f ) e_sub -= 1.0f;
        const float x = pulse * 0.9f + std::sin( Pi2 * e_sub ) * 0.3f * ( 1.2f - e_rpm );
        float       lo, bd, ilo, ibd;
        e_body.process( x, lo, bd );
        e_intake.process( x, ilo, ibd );
        float y = lo + ibd * 0.3f * ( 0.3f + load );
        y = std::tanh( y * ( 1.2f + 2.5f * load ) ) * ( 0.35f + 0.35f * load + 0.15f * e_rpm );
        if( e_pop > 0.001f )
        {
            y += ( e_n.next() * 0.8f + 0.4f ) * e_pop * 0.6f;
            e_pop *= 0.9965f;
        }
        if( m_limiter.load( std::memory_order_relaxed ) )
        {
            e_limPh += 26.0f / SampleRate;
            if( e_limPh >= 1.0f ) e_limPh -= 1.0f;
            if( e_limPh > 0.55f ) y *= 0.25f;
        }
        const bool shifting = m_shifting.load( std::memory_order_relaxed );
        if( shifting && !e_wasShifting ) e_shift = 0.15f;
        e_wasShifting = shifting;
        e_shift += ( 1.0f - e_shift ) * 0.0006f;
        y *= e_shift;

        e_whistle += ( 1800.0f + 5200.0f * e_spool ) / SampleRate;
        if( e_whistle >= 1.0f ) e_whistle -= 1.0f;
        float engine = ( y + std::sin( Pi2 * e_whistle ) * 0.03f * e_spool * e_spool ) * e_active;
        engine += e_hiss.band( e_n.next() ) * 0.09f * e_boost * e_active;

        // Tyres, wind, kerbs, gravel.
        e_vib += 7.0f / SampleRate;
        if( e_vib >= 1.0f ) e_vib -= 1.0f;
        e_squeal += ( 1050.0f + 40.0f * std::sin( Pi2 * e_vib ) ) / SampleRate;
        if( e_squeal >= 1.0f ) e_squeal -= 1.0f;
        float env = ( std::sin( Pi2 * e_squeal ) * 0.5f + e_squealF.band( e_n.next() ) * 1.6f ) * e_slip * 0.2f;
        const float wind = e_speed / 85.0f;
        env += e_wind.low( e_n.next() ) * wind * wind * 0.5f;
        e_rumblePh += ( 45.0f + e_speed * 1.4f ) / SampleRate;
        if( e_rumblePh >= 1.0f ) e_rumblePh -= 1.0f;
        env += e_rumbleF.low( e_rumblePh < 0.5f ? 1.0f : -1.0f ) * e_rumble * 0.2f * std::min( 1.0f, e_speed / 15.0f );
        const float impulse = e_n.unit() < e_speed * 0.0008f ? e_n.next() * 2.0f : 0.0f;
        env += e_gravel.band( impulse + e_n.next() * 0.15f ) * e_off * 0.6f;
        return engine + env;
    }

    /**
     * @brief Fills the stream's buffer.
     * @param[out] out     interleaved stereo floats
     * @param[in]  frames  frames to render
     */
    void render( float *out, unsigned frames )
    {
        while( m_tail.load( std::memory_order_acquire ) != m_head.load( std::memory_order_acquire ) )
        {
            const uint32_t t = m_tail.load( std::memory_order_relaxed );
            startVoice( m_queue[t % m_queue.size()] );
            m_tail.store( t + 1, std::memory_order_release );
        }
        const int cmd = m_musicCmd.exchange( 0 );
        if( cmd == 1 ) m_music.start( m_musicSeed.load() );
        if( cmd == 2 ) m_music.m_on = false;
        const float sfx = m_sfxVol.load( std::memory_order_relaxed );
        const float mus = m_musicVol.load( std::memory_order_relaxed );
        for( unsigned i = 0; i < frames; ++i )
        {
            const float engine = renderEngine() * sfx;
            e_width[e_wi & 63] = engine;
            const float delayed = e_width[( e_wi - 19 ) & 63];
            ++e_wi;
            float l = engine * 0.9f + delayed * 0.1f;
            float r = engine * 0.75f + delayed * 0.25f;
            for( Voice &v : m_voices )
            {
                if( !v.m_on ) continue;
                const float s = renderVoice( v ) * sfx;
                l += s * std::min( 1.0f, 1.0f - v.m_pan );
                r += s * std::min( 1.0f, 1.0f + v.m_pan );
            }
            float ml, mr;
            m_music.tick( ml, mr );
            l += ml * mus;
            r += mr * mus;
            out[i * 2] = std::tanh( l * MasterGain );
            out[i * 2 + 1] = std::tanh( r * MasterGain );
        }
    }
};

Synth *g_synth = nullptr;   ///< the one synthesiser the callback renders

/**
 * @brief raylib's audio callback.
 * @param[out] buffer  interleaved floats
 * @param[in]  frames  frames wanted
 */
void audioCallback( void *buffer, unsigned int frames )
{
    if( g_synth ) g_synth->render( static_cast<float *>( buffer ), frames );
}

} // namespace

bool Audio::renderTest( const std::string &path, float seconds )
{
    auto           synth = std::make_unique<Synth>();
    const unsigned frames = static_cast<unsigned>( seconds * SampleRate );
    std::vector<float> buf( static_cast<size_t>( frames ) * 2 );
    synth->m_music.start( 4711 );
    // Gear by gear: seconds per gear while the revs climb from 0.45 to 0.97.
    static const float gearTime[6] = { 1.4f, 1.8f, 2.2f, 2.6f, 3.0f, 3.4f };
    struct Cue
    {
        float m_t;
        SfxE  m_type;
        float m_pan;
    };
    static const Cue cues[] = { { 6.0f, SfxE::WHOOSH, -0.7f }, { 9.0f, SfxE::CHECKPOINT, 0.0f }, { 11.0f, SfxE::NEAR_MISS, 0.0f },
                                { 12.0f, SfxE::BOOST, 0.0f },  { 16.2f, SfxE::BACKFIRE, 0.0f }, { 18.0f, SfxE::CRASH, 0.0f } };
    size_t     cue = 0;
    const unsigned block = 512;
    for( unsigned f = 0; f < frames; f += block )
    {
        const float t = static_cast<float>( f ) / SampleRate;
        float       rpm = 0.12f, thr = 0.0f, speed = 0.0f;
        bool        shifting = false;
        if( t < 2.0f )
        {
            thr = t > 1.0f && t < 1.5f ? 1.0f : 0.0f;
            rpm = thr > 0.0f ? 0.8f : 0.12f;
        }
        else if( t < 16.0f )
        {
            float tt = t - 2.0f;
            int   g = 0;
            while( g < 5 && tt > gearTime[g] )
            {
                tt -= gearTime[g];
                ++g;
            }
            const float k = std::min( 1.0f, tt / gearTime[g] );
            rpm = g == 0 ? 0.2f + 0.77f * k : 0.45f + 0.52f * k;
            shifting = g > 0 && tt < 0.16f;
            thr = 1.0f;
            speed = 80.0f * std::min( 1.0f, ( t - 2.0f ) / 14.0f );
        }
        else
        {
            rpm = std::max( 0.5f, 0.97f - ( t - 16.0f ) * 0.08f );
            thr = t > 18.5f ? 0.6f : 0.0f;
            speed = 75.0f;
        }
        synth->m_active = true;
        synth->m_rpm = rpm;
        synth->m_throttle = thr;
        synth->m_speed = speed;
        synth->m_shifting = shifting;
        synth->m_boost = t > 12.0f && t < 14.0f;
        synth->m_slip = t > 17.0f && t < 18.0f ? 0.8f : 0.0f;
        synth->m_rumble = t > 19.0f ? 1.0f : 0.0f;
        while( cue < std::size( cues ) && cues[cue].m_t <= t )
        {
            synth->startVoice( Event{ cues[cue].m_type, 1.0f, cues[cue].m_pan } );
            ++cue;
        }
        synth->render( buf.data() + static_cast<size_t>( f ) * 2, std::min( block, frames - f ) );
    }
    Wave w{};
    w.frameCount = frames;
    w.sampleRate = static_cast<unsigned>( SampleRate );
    w.sampleSize = 32;
    w.channels = 2;
    w.data = buf.data();
    const bool ok = ExportWave( w, path.c_str() );

    // Peak and RMS per second, in dBFS.
    FILE *txt = std::fopen( ( path + ".txt" ).c_str(), "w" );
    if( txt )
    {
        float allPeak = 0.0f;
        for( unsigned s = 0; s * SampleRate < frames; ++s )
        {
            const unsigned a = static_cast<unsigned>( s * SampleRate ), b = std::min( frames, static_cast<unsigned>( ( s + 1 ) * SampleRate ) );
            double sum = 0.0;
            float  peak = 0.0f;
            for( unsigned i = a * 2; i < b * 2; ++i )
            {
                peak = std::max( peak, std::abs( buf[i] ) );
                sum += static_cast<double>( buf[i] ) * buf[i];
            }
            allPeak = std::max( allPeak, peak );
            const double rms = std::sqrt( sum / std::max( 1u, ( b - a ) * 2 ) );
            std::fprintf( txt, "%2u s  peak %6.1f dBFS  rms %6.1f dBFS\n", s, 20.0 * std::log10( std::max( 1e-6f, peak ) ),
                          20.0 * std::log10( std::max( 1e-9, rms ) ) );
        }
        std::fprintf( txt, "overall peak %.1f dBFS\n", 20.0 * std::log10( std::max( 1e-6f, allPeak ) ) );
        std::fclose( txt );
    }
    return ok;
}

bool Audio::init( const std::vector<std::string> &musicDirs )
{
    InitAudioDevice();
    if( !IsAudioDeviceReady() ) return false;
    g_synth = new Synth();
    SetAudioStreamBufferSizeDefault( 1024 );
    m_stream = LoadAudioStream( static_cast<unsigned>( SampleRate ), 32, 2 );
    SetAudioStreamCallback( m_stream, audioCallback );
    PlayAudioStream( m_stream );
    m_ok = true;
    for( const std::string &dir : musicDirs )
    {
        std::error_code ec;
        if( !std::filesystem::is_directory( dir, ec ) ) continue;
        for( const auto &entry : std::filesystem::directory_iterator( dir, ec ) )
        {
            std::string ext = entry.path().extension().string();
            std::transform( ext.begin(), ext.end(), ext.begin(), []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
            if( ext == ".ogg" || ext == ".mp3" || ext == ".flac" || ext == ".wav" || ext == ".xm" || ext == ".mod" )
                m_files.push_back( entry.path().string() );
        }
    }
    std::sort( m_files.begin(), m_files.end() );
    return true;
}

void Audio::shutdown()
{
    if( !m_ok ) return;
    if( m_musicLoaded ) UnloadMusicStream( m_music );
    m_musicLoaded = false;
    UnloadAudioStream( m_stream );
    CloseAudioDevice();
    delete g_synth;
    g_synth = nullptr;
    m_ok = false;
}

void Audio::setEngine( const EngineState &s )
{
    if( !g_synth ) return;
    g_synth->m_active = s.m_active;
    g_synth->m_rpm = s.m_rpm;
    g_synth->m_throttle = s.m_throttle;
    g_synth->m_speed = s.m_speed;
    g_synth->m_slip = s.m_slip;
    g_synth->m_offroad = s.m_offroad;
    g_synth->m_rumble = s.m_rumble;
    g_synth->m_boost = s.m_boost;
    g_synth->m_limiter = s.m_limiter;
    g_synth->m_shifting = s.m_shifting;
}

void Audio::play( SfxE type, float strength, float pan )
{
    if( !g_synth ) return;
    const uint32_t h = g_synth->m_head.load( std::memory_order_relaxed );
    if( h - g_synth->m_tail.load( std::memory_order_acquire ) >= g_synth->m_queue.size() ) return;
    g_synth->m_queue[h % g_synth->m_queue.size()] = Event{ type, std::clamp( strength, 0.0f, 1.5f ), std::clamp( pan, -1.0f, 1.0f ) };
    g_synth->m_head.store( h + 1, std::memory_order_release );
}

void Audio::setVolumes( float music, float sfx )
{
    m_musicVolume = music;
    if( g_synth )
    {
        g_synth->m_musicVol = music;
        g_synth->m_sfxVol = sfx;
    }
    if( m_musicLoaded ) SetMusicVolume( m_music, music * 0.8f );
}

void Audio::startMusic( uint64_t seed )
{
    if( !m_ok ) return;
    m_musicOn = true;
    if( !m_files.empty() )
    {
        if( m_musicLoaded ) UnloadMusicStream( m_music );
        m_music = LoadMusicStream( m_files[m_fileIndex % m_files.size()].c_str() );
        m_fileIndex = ( m_fileIndex + 1 ) % m_files.size();
        m_musicLoaded = IsMusicValid( m_music );
        if( m_musicLoaded )
        {
            m_music.looping = false;
            SetMusicVolume( m_music, m_musicVolume * 0.8f );
            PlayMusicStream( m_music );
        }
        return;
    }
    g_synth->m_musicSeed = seed;
    g_synth->m_musicCmd = 1;
}

void Audio::stopMusic()
{
    m_musicOn = false;
    if( m_musicLoaded ) StopMusicStream( m_music );
    if( g_synth ) g_synth->m_musicCmd = 2;
}

void Audio::update()
{
    if( !m_musicLoaded ) return;
    UpdateMusicStream( m_music );
    if( m_musicOn && !IsMusicStreamPlaying( m_music ) ) startMusic( 0 );
}

} // namespace sunstreak
