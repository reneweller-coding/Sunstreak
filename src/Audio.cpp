/**
 * @file Audio.cpp
 * @brief Live synthesis: engine, tyres, wind, effects and the generative soundtrack.
 * @author reneweller
 */
#include "Audio.h"

#include "Music.h"

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

using namespace dsp;

constexpr int   VoiceCount = 24;            ///< simultaneous effects
constexpr float MasterGain = 0.35f;         ///< overall level before the soft clip (about -9 dB)

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

/** @brief The synthesiser shared with the audio thread. */
struct Synth
{
    std::atomic<float> m_rpm{ 0.1f }, m_throttle{ 0 }, m_speed{ 0 }, m_slip{ 0 }, m_offroad{ 0 }, m_rumble{ 0 };
    std::atomic<bool>  m_active{ false }, m_boost{ false }, m_limiter{ false }, m_shifting{ false };
    std::atomic<int>   m_cylinders{ 8 };
    std::atomic<float> m_idle{ 900.0f }, m_redline{ 7800.0f }, m_bright{ 1.0f };
    std::atomic<float> m_sfxVol{ 0.9f }, m_musicVol{ 0.7f };
    std::atomic<int>   m_musicCmd{ 0 };      ///< 1 start, 2 stop
    std::atomic<uint64_t> m_musicSeed{ 1 };    ///< the track to start
    std::atomic<int>   m_musicStation{ 0 };  ///< its station
    std::atomic<float> m_tension{ 0.0f }, m_drive{ 0.0f };   ///< the game's mood
    std::atomic<float> m_aSurf{ 0 }, m_aSurfPan{ 0 }, m_aWind{ 0 }, m_aJungle{ 0 }, m_aCity{ 0 }, m_aRain{ 0 }, m_aTunnel{ 0 }, m_aBridge{ 0 };
    std::atomic<float> m_aUnder{ 0 };

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
    // The ambience (audio thread).
    float  a_surf = 0, a_pan = 0, a_wind = 0, a_jungle = 0, a_city = 0, a_rain = 0, a_tunnel = 0, a_bridge = 0, a_time = 0;
    float  a_joint = 0, a_thump = 0, a_thump2 = 0, a_thumpDelay = -1, a_horn = 0, a_hornF = 420, a_hornIn = 6, a_bird = 0, a_birdF = 3000,
           a_birdIn = 2, a_birdPh = 0, a_insectPh = 0, a_amPh = 0, a_droplet = 0;
    float  a_under = 0, a_whale = 0, a_whaleIn = 4, a_whaleDur = 1, a_whalePh = 0, a_whaleF0 = 200, a_whaleF1 = 300;
    int    a_coef = 0;
    Svf    a_surfLo, a_surfHi, a_windBp, a_rainHp, a_cityLo, a_hornLo, a_thumpLo, a_seaLo, a_mufL, a_mufR;
    Noise  a_n;
    std::array<std::array<float, 3600>, 4> a_comb{};   ///< tunnel echo: four combs
    std::array<float, 4>                   a_combDamp{};
    int                                    a_combPos = 0;

    /**
     * @brief Renders the place's sounds and the tunnel's echo of the engine.
     * @param[in]  engine  the dry engine and tyres
     * @param[in]  gain    the effects volume
     * @param[out] l       left
     * @param[out] r       right
     */
    void renderAmbience( float engine, float gain, float &l, float &r )
    {
        const float glide = 0.00004f;
        a_surf += ( m_aSurf.load( std::memory_order_relaxed ) - a_surf ) * glide;
        a_pan += ( m_aSurfPan.load( std::memory_order_relaxed ) - a_pan ) * glide;
        a_wind += ( m_aWind.load( std::memory_order_relaxed ) - a_wind ) * glide;
        a_jungle += ( m_aJungle.load( std::memory_order_relaxed ) - a_jungle ) * glide;
        a_city += ( m_aCity.load( std::memory_order_relaxed ) - a_city ) * glide;
        a_rain += ( m_aRain.load( std::memory_order_relaxed ) - a_rain ) * glide;
        a_tunnel += ( m_aTunnel.load( std::memory_order_relaxed ) - a_tunnel ) * 0.0002f;
        a_bridge += ( m_aBridge.load( std::memory_order_relaxed ) - a_bridge ) * 0.0004f;
        a_under += ( m_aUnder.load( std::memory_order_relaxed ) - a_under ) * 0.0002f;
        a_time += 1.0f / SampleRate;
        if( a_time > 3600.0f ) a_time -= 3600.0f;
        const float dt = 1.0f / SampleRate;
        float       mono = 0.0f, side = 0.0f;

        // Surf: swells every seven seconds, a rush and a hiss as they break.
        if( a_surf > 0.001f )
        {
            const float swell = 0.5f - 0.5f * std::cos( Pi2 * a_time / 7.3f ), crash = swell * swell * swell;
            if( ( a_coef & 31 ) == 0 )
            {
                a_surfLo.set( 280.0f + 900.0f * crash, 0.7f );
                a_surfHi.set( 2600.0f, 0.8f );
            }
            const float wave = a_surfLo.low( a_n.next() ) * ( 0.3f + 0.9f * crash ) + a_surfHi.band( a_n.next() ) * crash * 0.35f;
            side += wave * a_surf * 0.8f;
        }
        // Wind: a band of noise that gusts.
        if( a_wind > 0.001f )
        {
            const float gust = 0.55f + 0.3f * std::sin( Pi2 * a_time / 5.1f ) + 0.15f * std::sin( Pi2 * a_time / 1.7f );
            if( ( a_coef & 31 ) == 0 ) a_windBp.set( 380.0f + 420.0f * gust, 1.6f );
            mono += a_windBp.band( a_n.next() ) * gust * a_wind * 0.6f;
        }
        // Rain: a hiss and drops.
        if( a_rain > 0.001f )
        {
            if( ( a_coef & 31 ) == 0 ) a_rainHp.set( 4200.0f, 0.6f );
            if( a_n.unit() < 0.0016f * a_rain ) a_droplet = 1.0f;
            a_droplet *= 0.993f;
            mono += ( a_rainHp.band( a_n.next() ) * 0.5f + a_n.next() * a_droplet * 0.25f ) * a_rain * 0.6f;
        }
        // The city: a low rumble and now and then a horn far off.
        if( a_city > 0.001f )
        {
            if( ( a_coef & 31 ) == 0 )
            {
                a_cityLo.set( 160.0f, 0.7f );
                a_hornLo.set( 1300.0f, 0.7f );
            }
            float y = a_cityLo.low( a_n.next() ) * 0.9f;
            a_hornIn -= dt;
            if( a_hornIn <= 0.0f )
            {
                a_horn = 0.35f + 0.25f * a_n.unit();
                a_hornF = 380.0f + 160.0f * a_n.unit();
                a_hornIn = 4.0f + 9.0f * a_n.unit();
            }
            if( a_horn > 0.0f )
            {
                a_horn -= dt;
                const float sq = std::sin( Pi2 * a_hornF * a_time ) + std::sin( Pi2 * a_hornF * 1.26f * a_time );
                y += a_hornLo.low( sq ) * 0.18f * std::min( 1.0f, a_horn * 8.0f );
            }
            mono += y * a_city * 1.1f;
        }
        // The jungle: a chorus of insects and birdsong.
        if( a_jungle > 0.001f )
        {
            a_insectPh += 4700.0f * dt;
            if( a_insectPh >= 1.0f ) a_insectPh -= 1.0f;
            a_amPh += 38.0f * dt;
            if( a_amPh >= 1.0f ) a_amPh -= 1.0f;
            const float chorus = std::max( 0.0f, std::sin( Pi2 * a_time / 3.7f ) ) * 0.7f + 0.3f;
            float       y = std::sin( Pi2 * a_insectPh ) * ( a_amPh < 0.35f ? 1.0f : 0.0f ) * chorus * 0.12f;
            a_birdIn -= dt;
            if( a_birdIn <= 0.0f )
            {
                a_bird = 0.42f;
                a_birdF = 2200.0f + 1800.0f * a_n.unit();
                a_birdIn = 1.2f + 3.5f * a_n.unit();
            }
            if( a_bird > 0.0f )
            {
                a_bird -= dt;
                const float chirp = std::fmod( 0.42f - a_bird, 0.14f ) / 0.14f;   // three falling chirps
                a_birdPh += ( a_birdF * ( 1.3f - 0.5f * chirp ) ) * dt;
                if( a_birdPh >= 1.0f ) a_birdPh -= 1.0f;
                y += std::sin( Pi2 * a_birdPh ) * std::sin( 3.14159f * chirp ) * 0.3f;
            }
            side -= y * a_jungle * 1.4f;   // the far side from the sea
        }
        // A bridge: the joints, front and rear axle.
        if( a_bridge > 0.01f )
        {
            const float speed = m_speed.load( std::memory_order_relaxed );
            a_joint += speed / 16.0f * dt;
            if( a_joint >= 1.0f )
            {
                a_joint -= 1.0f;
                a_thump = 1.0f;
                a_thumpDelay = speed > 1.0f ? 2.7f / speed : -1.0f;
            }
            if( a_thumpDelay > 0.0f )
            {
                a_thumpDelay -= dt;
                if( a_thumpDelay <= 0.0f ) a_thump2 = 0.8f;
            }
            if( ( a_coef & 31 ) == 0 ) a_thumpLo.set( 140.0f, 0.9f );
            const float hit = a_thump + a_thump2;
            mono += a_thumpLo.low( a_n.next() * hit * 4.0f ) * a_bridge * 0.8f;
            a_thump *= 0.9985f;
            a_thump2 *= 0.9985f;
        }
        // Under the sea: a deep hum, and now and then a whale far off, calling in a long glide.
        float whale = 0.0f;
        if( a_under > 0.001f )
        {
            if( ( a_coef & 31 ) == 0 ) a_seaLo.set( 70.0f, 0.8f );
            mono += a_seaLo.low( a_n.next() ) * a_under * 0.45f;
            a_whaleIn -= dt;
            if( a_whaleIn <= 0.0f && a_whale <= 0.0f )
            {
                a_whaleDur = 2.2f + 2.5f * a_n.unit();
                a_whale = a_whaleDur;
                a_whaleF0 = 130.0f + 170.0f * a_n.unit();
                a_whaleF1 = a_whaleF0 * ( 0.6f + 1.1f * a_n.unit() );
                a_whaleIn = 6.0f + 12.0f * a_n.unit();
            }
            if( a_whale > 0.0f )
            {
                a_whale -= dt;
                const float t = 1.0f - a_whale / a_whaleDur;
                const float f = a_whaleF0 + ( a_whaleF1 - a_whaleF0 ) * ( 0.5f - 0.5f * std::cos( 3.14159f * t ) ) + 14.0f * std::sin( Pi2 * 5.0f * a_time );
                a_whalePh += f * dt;
                if( a_whalePh >= 1.0f ) a_whalePh -= 1.0f;
                const float env = std::sin( 3.14159f * t );
                whale = ( std::sin( Pi2 * a_whalePh ) + 0.35f * std::sin( 2.0f * Pi2 * a_whalePh ) + 0.15f * std::sin( 3.0f * Pi2 * a_whalePh ) ) *
                        env * env * a_under * 0.16f;
                mono += whale;
            }
        }
        ++a_coef;

        // The tunnel: the engine thrown back by the walls (and a whale's call by the sea).
        float echo = 0.0f;
        if( a_tunnel > 0.001f )
        {
            static const int lengths[4] = { 2711, 3049, 3307, 3559 };
            for( size_t k = 0; k < 4; ++k )
            {
                float      &slot = a_comb[k][static_cast<size_t>( a_combPos % lengths[k] )];
                const float out = slot;
                a_combDamp[k] += ( out - a_combDamp[k] ) * 0.35f;
                slot = engine + whale * 2.0f + a_combDamp[k] * 0.74f;
                echo += out;
            }
            echo *= 0.25f * a_tunnel * 0.9f;
        }
        else if( a_combPos % 4096 == 0 )
        {
            for( auto &c : a_comb ) c.fill( 0.0f );
        }
        ++a_combPos;
        if( a_combPos >= 3559 * 3307 ) a_combPos = 0;
        const float pan = clampPan( a_pan );
        l += ( mono + side * ( 1.0f - pan ) * 0.7f ) * gain + echo * 0.9f;
        r += ( mono + side * ( 1.0f + pan ) * 0.7f ) * gain + echo * 1.0f;
    }

    /**
     * @brief Clamps a pan position.
     * @param[in] p  the position
     * @return -1..1
     */
    static float clampPan( float p ) { return std::min( 1.0f, std::max( -1.0f, p ) ); }

    std::array<Voice, VoiceCount> m_voices;
    std::array<Event, 8>          m_onBeat{};     ///< cues waiting for the beat
    int                           m_onBeatCount = 0;   ///< how many
    float                         m_onBeatWait = 0.0f; ///< seconds the oldest waits
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
        case SfxE::VICTORY:
        {
            // A rising arpeggio into a held major chord, slightly overdriven like brass.
            dur = 2.6f;
            static const float notes[4] = { 60, 64, 67, 72 };
            const int   i = std::min( 3, static_cast<int>( t / 0.11f ) );
            const float lt = t - 0.11f * static_cast<float>( i );
            float       s = std::sin( Pi2 * mtof( notes[i] ) * t );
            if( i == 3 ) s += std::sin( Pi2 * mtof( 76.0f ) * t ) + std::sin( Pi2 * mtof( 79.0f ) * t ) + 0.5f * std::sin( Pi2 * mtof( 48.0f ) * t );
            y = std::tanh( s * 1.6f ) * std::min( 1.0f, lt * 60.0f ) * std::exp( -lt * ( i == 3 ? 1.4f : 10.0f ) ) * 0.24f;
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

        const int   cyl = m_cylinders.load( std::memory_order_relaxed );
        const float idle = m_idle.load( std::memory_order_relaxed ), red = m_redline.load( std::memory_order_relaxed );
        const float bright = m_bright.load( std::memory_order_relaxed );
        const float fire = ( idle + e_rpm * ( red - idle ) ) / 60.0f * static_cast<float>( cyl ) * 0.5f;
        if( ( e_coef++ & 31 ) == 0 )
        {
            e_body.set( ( 250.0f + 2600.0f * ( 0.25f + 0.75f * load ) * ( 0.35f + e_rpm ) ) * bright, 0.9f );
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
            e_uneven = cyl != 8 ? 1.0f : k == 1 ? 1.3f : k == 3 ? 0.85f : 1.0f;
            if( e_thr < 0.1f && e_rpm > 0.55f && e_n.unit() < 0.02f * e_rpm ) e_pop = 1.0f;
        }
        const float decay = ( 4.0f + 10.0f * ( 1.0f - load * 0.8f ) ) * ( cyl >= 12 ? 1.6f : cyl <= 4 ? 0.8f : 1.0f );
        float       pulse = std::exp( -e_phase * decay ) * e_pulseAmp * e_uneven - ( 1.0f - std::exp( -decay ) ) / decay;
        e_sub += fire * 0.5f / SampleRate;
        if( e_sub >= 1.0f ) e_sub -= 1.0f;
        const float sub = cyl <= 4 ? 0.45f : cyl >= 12 ? 0.12f : 0.3f;
        const float x = pulse * 0.9f + std::sin( Pi2 * e_sub ) * sub * ( 1.2f - e_rpm );
        float       lo, bd, ilo, ibd;
        e_body.process( x, lo, bd );
        e_intake.process( x, ilo, ibd );
        float y = lo + ibd * ( cyl <= 4 ? 0.55f : 0.3f ) * ( 0.3f + load );
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
            const Event   &e = m_queue[t % m_queue.size()];
            // Cues flagged by a negative pan wait for the beat (the pan is restored).
            if( e.m_pan < -1.5f && m_music.audible() && m_onBeatCount < static_cast<int>( m_onBeat.size() ) )
            {
                if( m_onBeatCount == 0 ) m_onBeatWait = 0.0f;
                m_onBeat[static_cast<size_t>( m_onBeatCount++ )] = Event{ e.m_type, e.m_strength, 0.0f };
            }
            else
            {
                startVoice( Event{ e.m_type, e.m_strength, e.m_pan < -1.5f ? 0.0f : e.m_pan } );
            }
            m_tail.store( t + 1, std::memory_order_release );
        }
        m_music.setMood( m_tension.load( std::memory_order_relaxed ), m_drive.load( std::memory_order_relaxed ) );
        const int cmd = m_musicCmd.exchange( 0 );
        if( cmd == 1 ) m_music.request( m_musicSeed.load(), m_musicStation.load() );
        if( cmd == 2 ) m_music.stop();
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
            renderAmbience( engine, sfx, l, r );
            for( Voice &v : m_voices )
            {
                if( !v.m_on ) continue;
                const float s = renderVoice( v ) * sfx;
                l += s * std::min( 1.0f, 1.0f - v.m_pan );
                r += s * std::min( 1.0f, 1.0f + v.m_pan );
            }
            // Under the sea everything but the music comes through glass and water.
            if( a_under > 0.01f )
            {
                if( ( e_wi & 31 ) == 0 )
                {
                    const float fc = 18000.0f * std::pow( 650.0f / 18000.0f, a_under );
                    a_mufL.set( fc, 0.6f );
                    a_mufR.set( fc, 0.6f );
                }
                l = a_mufL.low( l ) * ( 1.0f + 0.3f * a_under );
                r = a_mufR.low( r ) * ( 1.0f + 0.3f * a_under );
            }
            float ml, mr;
            m_music.tick( ml, mr );
            if( m_onBeatCount > 0 )
            {
                m_onBeatWait += 1.0f / SampleRate;
                if( m_music.m_beatNow || m_onBeatWait > 0.7f )
                {
                    for( int k = 0; k < m_onBeatCount; ++k ) startVoice( m_onBeat[static_cast<size_t>( k )] );
                    m_onBeatCount = 0;
                }
            }
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

/**
 * @brief Clamps to 0..1.
 * @param[in] v  the value
 * @return v in [0, 1]
 */
float clampf01( float v )
{
    return std::min( 1.0f, std::max( 0.0f, v ) );
}

} // namespace

bool Audio::renderTest( const std::string &path, float seconds, int station )
{
    auto           synth = std::make_unique<Synth>();
    const unsigned frames = static_cast<unsigned>( seconds * SampleRate );
    std::vector<float> buf( static_cast<size_t>( frames ) * 2 );
    // Stations past the last one test the adaptive soundtrack: tension from 15 to 32 s, cues held for the beat.
    const bool adaptive = station >= StationCount;
    const int  style = std::max( 0, station ) % StationCount;
    synth->m_music.request( 4711 + static_cast<uint64_t>( style ), style );
    const bool drive = station == -1;
    const bool ambience = station == -2;
    bool       cued[2] = {};
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
        if( ambience )
        { // Five seconds of each: surf, wind, jungle, city, rain, a bridge, a tunnel with the engine.
            const int k = static_cast<int>( t / 5.0f );
            synth->m_aSurf = k == 0 ? 1.0f : 0.0f;
            synth->m_aSurfPan = 0.8f;
            synth->m_aWind = k == 1 ? 1.0f : 0.0f;
            synth->m_aJungle = k == 2 ? 1.0f : 0.0f;
            synth->m_aCity = k == 3 ? 1.0f : 0.0f;
            synth->m_aRain = k == 4 ? 1.0f : 0.0f;
            synth->m_aBridge = k == 5 ? 1.0f : 0.0f;
            synth->m_aTunnel = k == 6 ? 1.0f : 0.0f;
            if( k >= 5 )
            {
                synth->m_active = true;
                synth->m_rpm = 0.7f;
                synth->m_throttle = 0.8f;
                synth->m_speed = 45.0f;
            }
            synth->m_musicVol = 0.0f;
            synth->render( buf.data() + static_cast<size_t>( f ) * 2, std::min( block, frames - f ) );
            continue;
        }
        synth->m_active = drive;
        synth->m_rpm = rpm;
        synth->m_throttle = thr;
        synth->m_speed = speed;
        synth->m_shifting = shifting;
        synth->m_boost = t > 12.0f && t < 14.0f;
        synth->m_slip = t > 17.0f && t < 18.0f ? 0.8f : 0.0f;
        synth->m_rumble = t > 19.0f ? 1.0f : 0.0f;
        if( adaptive )
        {
            synth->m_tension = t > 15.0f && t < 32.0f ? 1.0f : 0.0f;
            synth->m_drive = 0.5f + 0.5f * std::sin( t * 0.3f );
            for( int k = 0; k < 2; ++k )
                if( !cued[k] && t >= ( k == 0 ? 8.0f : 33.0f ) )
                {
                    const uint32_t h = synth->m_head.load();
                    synth->m_queue[h % synth->m_queue.size()] = Event{ k == 0 ? SfxE::CHECKPOINT : SfxE::VICTORY, 1.0f, -2.0f };
                    synth->m_head.store( h + 1 );
                    cued[k] = true;
                }
        }
        while( drive && cue < std::size( cues ) && cues[cue].m_t <= t )
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
    g_synth->m_cylinders = s.m_cylinders;
    g_synth->m_idle = s.m_idle;
    g_synth->m_redline = s.m_redline;
    g_synth->m_bright = s.m_bright;
}

void Audio::setAmbience( const Ambience &a )
{
    if( !g_synth ) return;
    g_synth->m_aSurf.store( a.m_surf, std::memory_order_relaxed );
    g_synth->m_aSurfPan.store( a.m_surfPan, std::memory_order_relaxed );
    g_synth->m_aWind.store( a.m_wind, std::memory_order_relaxed );
    g_synth->m_aJungle.store( a.m_jungle, std::memory_order_relaxed );
    g_synth->m_aCity.store( a.m_city, std::memory_order_relaxed );
    g_synth->m_aRain.store( a.m_rain, std::memory_order_relaxed );
    g_synth->m_aTunnel.store( a.m_tunnel, std::memory_order_relaxed );
    g_synth->m_aBridge.store( a.m_bridge, std::memory_order_relaxed );
    g_synth->m_aUnder.store( a.m_underwater, std::memory_order_relaxed );
}

void Audio::play( SfxE type, float strength, float pan )
{
    if( !g_synth ) return;
    const uint32_t h = g_synth->m_head.load( std::memory_order_relaxed );
    if( h - g_synth->m_tail.load( std::memory_order_acquire ) >= g_synth->m_queue.size() ) return;
    g_synth->m_queue[h % g_synth->m_queue.size()] = Event{ type, std::clamp( strength, 0.0f, 1.5f ), std::clamp( pan, -1.0f, 1.0f ) };
    g_synth->m_head.store( h + 1, std::memory_order_release );
}

void Audio::playOnBeat( SfxE type, float strength )
{
    if( !g_synth ) return;
    const uint32_t h = g_synth->m_head.load( std::memory_order_relaxed );
    if( h - g_synth->m_tail.load( std::memory_order_acquire ) >= g_synth->m_queue.size() ) return;
    g_synth->m_queue[h % g_synth->m_queue.size()] = Event{ type, std::clamp( strength, 0.0f, 1.5f ), -2.0f };
    g_synth->m_head.store( h + 1, std::memory_order_release );
}

void Audio::setMood( float tension, float drive )
{
    if( !g_synth ) return;
    g_synth->m_tension = clampf01( tension );
    g_synth->m_drive = clampf01( drive );
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

void Audio::startMusic( uint64_t seed, int station )
{
    if( !m_ok ) return;
    m_musicOn = true;
    m_fileMode = station >= StationCount && !m_files.empty();
    if( m_fileMode )
    {
        g_synth->m_musicCmd = 2;
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
    if( m_musicLoaded ) StopMusicStream( m_music );
    g_synth->m_musicSeed = seed;
    g_synth->m_musicStation = std::clamp( station, 0, StationCount - 1 );
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
    if( m_musicOn && m_fileMode && !IsMusicStreamPlaying( m_music ) ) startMusic( 0, StationCount );
}

} // namespace sunstreak
