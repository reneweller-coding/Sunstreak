/**
 * @file Audio.cpp
 * @brief Live synthesis: engine, tyres, wind, effects and the generative soundtrack.
 * @author Rene Weller
 */
#include "Audio.h"

#include "MathUtil.h"
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

constexpr int   VoiceCount = 24;      ///< simultaneous effects
constexpr float MasterGain = 0.35f;   ///< overall level before the soft clip (about -9 dB)

/** @brief A posted effect. */
struct Event
{
    SfxE  m_type = SfxE::TICK;   ///< which
    float m_strength = 1.0f;     ///< 0..1
    float m_pan = 0.0f;          ///< -1..1
    float m_pitch = 1.0f;        ///< Doppler shift
};

/** @brief One sounding effect. */
struct Voice
{
    bool  m_on = false;          ///< sounding
    SfxE  m_type = SfxE::TICK;   ///< which
    float m_t = 0;               ///< seconds since start
    float m_strength = 1;        ///< 0..1
    float m_pan = 0;             ///< -1..1
    float m_pitch = 1;           ///< Doppler shift
    float m_a = 0;               ///< per-type state (a pitch, a sweep)
    float m_b = 0;               ///< more per-type state
    float m_phase = 0;           ///< oscillator
    Svf   m_f;                   ///< filter
    Noise m_n;                   ///< noise
};

/** @brief The synthesiser shared with the audio thread. */
struct Synth
{
    // Written by the game thread, read by the audio thread.
    std::atomic<float>    m_rpm{ 0.1f };          ///< engine revs, 0..1 of idle to redline
    std::atomic<float>    m_throttle{ 0 };        ///< throttle 0..1
    std::atomic<float>    m_speed{ 0 };           ///< the car's speed (m/s)
    std::atomic<float>    m_slip{ 0 };            ///< tyre slip 0..1
    std::atomic<float>    m_offroad{ 0 };         ///< 0..1 off the road
    std::atomic<float>    m_rumble{ 0 };          ///< 0..1 on the kerbs
    std::atomic<bool>     m_active{ false };      ///< the engine runs (a drive is on)
    std::atomic<bool>     m_boost{ false };       ///< boosting
    std::atomic<bool>     m_limiter{ false };     ///< at the rev limiter
    std::atomic<bool>     m_shifting{ false };    ///< a gear change under way
    std::atomic<int>      m_cylinders{ 8 };       ///< the engine's cylinders
    std::atomic<float>    m_idle{ 900.0f };       ///< its idle revs
    std::atomic<float>    m_redline{ 7800.0f };   ///< its redline
    std::atomic<float>    m_bright{ 1.0f };       ///< the exhaust's brightness
    std::atomic<float>    m_sfxVol{ 0.9f };       ///< effects volume 0..1
    std::atomic<float>    m_musicVol{ 0.7f };     ///< music volume 0..1
    std::atomic<int>      m_musicCmd{ 0 };        ///< 1 start, 2 stop
    std::atomic<uint64_t> m_musicSeed{ 1 };       ///< the track to start
    std::atomic<int>      m_musicStation{ 0 };    ///< its station
    std::atomic<float>    m_tension{ 0.0f };      ///< the game's tension 0..1 (the music's mood)
    std::atomic<float>    m_drive{ 0.0f };        ///< how hard the car is driven 0..1
    std::atomic<float>    m_aSurf{ 0 };           ///< surf on the shore 0..1
    std::atomic<float>    m_aSurfPan{ 0 };        ///< where the surf is, -1..1
    std::atomic<float>    m_aWind{ 0 };           ///< wind 0..1
    std::atomic<float>    m_aJungle{ 0 };         ///< jungle 0..1
    std::atomic<float>    m_aCity{ 0 };           ///< city 0..1
    std::atomic<float>    m_aRain{ 0 };           ///< rain 0..1
    std::atomic<float>    m_aTunnel{ 0 };         ///< in a tunnel 0..1
    std::atomic<float>    m_aBridge{ 0 };         ///< on a bridge 0..1
    std::atomic<float>    m_aUnder{ 0 };          ///< under the sea 0..1
    std::atomic<float>    m_aSiren{ 0 };          ///< a siren's loudness 0..1
    std::atomic<float>    m_aSirenPan{ 0 };       ///< where it is, -1..1
    std::atomic<int>      m_aSirenKind{ 0 };      ///< 0 the long wail, 1 the two tones of Europe
    std::atomic<float>    m_aSirenPitch{ 1 };     ///< its Doppler shift

    std::array<Event, 64> m_queue;       ///< effects posted by the game
    std::atomic<uint32_t> m_head{ 0 };   ///< written by the game thread
    std::atomic<uint32_t> m_tail{ 0 };   ///< read by the audio thread

    // The engine, the tyres and the wind (audio thread).
    float m_engRpm = 0.1f;            ///< the revs, smoothed
    float m_engThr = 0;               ///< the throttle, smoothed
    float m_engSpeed = 0;             ///< the speed, smoothed
    float m_engSlip = 0;              ///< the slip, smoothed
    float m_engOff = 0;               ///< off the road, smoothed
    float m_engRumble = 0;            ///< on the kerbs, smoothed
    float m_engActive = 0;            ///< the engine's presence, faded in and out
    float m_engBoost = 0;             ///< the boost, smoothed
    float m_engSpool = 0;             ///< the turbo's spool 0..1
    float m_engPhase = 0;             ///< phase within the firing pulse
    float m_engSub = 0;               ///< the sub-octave's phase
    float m_engPulseAmp = 1;          ///< the current pulse's strength
    float m_engUneven = 1;            ///< the current pulse's weight (a cross-plane V8's burble)
    float m_engPop = 0;               ///< the backfire's pops left
    float m_engShift = 1;             ///< the gain dip of a gear change
    float m_engPrevThr = 0;           ///< the last sample's throttle (a lift-off pops)
    float m_engWhistle = 0;           ///< the turbo whistle's phase
    float m_engSqueal = 0;            ///< the tyre squeal's phase
    float m_engVib = 0;               ///< the squeal's vibrato phase
    float m_engRumblePh = 0;          ///< the kerb rumble's phase
    float m_engLimPh = 0;             ///< the rev limiter's cut phase
    int   m_engCount = 0;             ///< voices started (picks the next one and its noise seed)
    int   m_engCoef = 0;              ///< samples since the filters were set
    bool  m_engWasShifting = false;   ///< the last sample was in a gear change
    Svf   m_engBody;                  ///< the engine body's resonance
    Svf   m_engIntake;                ///< the intake's resonance
    Svf   m_engWind;                  ///< the wind noise
    Svf   m_engSquealF;               ///< the squeal's band
    Svf   m_engGravel;                ///< the gravel's band
    Svf   m_engRumbleF;               ///< the kerb rumble's low-pass
    Svf   m_engHiss;                  ///< the boost's hiss
    Noise m_engN;                     ///< noise
    float m_engWidth[64] = {};        ///< the short delay that widens the engine
    int   m_engWi = 0;                ///< its write position

    // The ambience (audio thread).
    float m_ambSurf = 0;                                  ///< surf, smoothed
    float m_ambPan = 0;                                   ///< where the surf is, smoothed
    float m_ambWind = 0;                                  ///< wind, smoothed
    float m_ambJungle = 0;                                ///< jungle, smoothed
    float m_ambCity = 0;                                  ///< city, smoothed
    float m_ambRain = 0;                                  ///< rain, smoothed
    float m_ambTunnel = 0;                                ///< in a tunnel, smoothed
    float m_ambBridge = 0;                                ///< on a bridge, smoothed
    float m_ambTime = 0;                                  ///< seconds running (the gusts' clock)
    float m_ambJoint = 0;                                 ///< the way to the next expansion joint of a bridge, 0..1
    float m_ambThump = 0;                                 ///< the front axle's thump on it
    float m_ambThump2 = 0;                                ///< the rear axle's
    float m_ambThumpDelay = -1;                           ///< seconds until the rear axle gets there (-1: none waiting)
    float m_ambHorn = 0;                                  ///< a far horn of the city, its envelope
    float m_ambHornF = 420;                               ///< its pitch (Hz)
    float m_ambHornIn = 6;                                ///< seconds to the next one
    float m_ambBird = 0;                                  ///< a bird's call, its envelope
    float m_ambBirdF = 3000;                              ///< its pitch (Hz)
    float m_ambBirdIn = 2;                                ///< seconds to the next call
    float m_ambBirdPh = 0;                                ///< its phase
    float m_ambInsectPh = 0;                              ///< the insects' phase
    float m_ambAmPh = 0;                                  ///< their tremolo's phase
    float m_ambDroplet = 0;                               ///< a raindrop on the roof, its envelope
    float m_ambUnder = 0;                                 ///< under the sea, smoothed
    float m_ambWhale = 0;                                 ///< a whale's song, seconds left
    float m_ambWhaleIn = 4;                               ///< seconds to the next one
    float m_ambWhaleDur = 1;                              ///< how long it sings
    float m_ambWhalePh = 0;                               ///< its phase
    float m_ambWhaleF0 = 200;                             ///< its pitch at the start (Hz)
    float m_ambWhaleF1 = 300;                             ///< and at the end
    float m_ambSiren = 0;                                 ///< the siren's loudness, smoothed
    float m_ambSirenPan = 0;                              ///< where it is, smoothed
    float m_ambSirenT = 0;                                ///< seconds into its cycle
    float m_ambSirenPh = 0;                               ///< its phase
    float m_ambSirenPitch = 1;                            ///< its Doppler shift, smoothed
    int   m_ambCoef = 0;                                  ///< samples since the filters were set
    Svf   m_ambSurfLo;                                    ///< the surf's rumble
    Svf   m_ambSurfHi;                                    ///< its hiss
    Svf   m_ambWindBp;                                    ///< the wind's band
    Svf   m_ambRainHp;                                    ///< the rain's hiss
    Svf   m_ambCityLo;                                    ///< the city's hum
    Svf   m_ambHornLo;                                    ///< the far horn's muffling
    Svf   m_ambThumpLo;                                   ///< the joints' thump
    Svf   m_ambSeaLo;                                     ///< the sea's rumble under water
    Svf   m_ambMufL;                                      ///< the muffling under water, left
    Svf   m_ambMufR;                                      ///< and right
    Noise m_ambN;                                         ///< noise
    std::array<std::array<float, 3600>, 4> m_ambComb{};   ///< tunnel echo: four combs
    std::array<float, 4>                   m_ambCombDamp{};    ///< their damping filters' state
    int                                    m_ambCombPos = 0;   ///< their write position

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
        m_ambSurf += ( m_aSurf.load( std::memory_order_relaxed ) - m_ambSurf ) * glide;
        m_ambPan += ( m_aSurfPan.load( std::memory_order_relaxed ) - m_ambPan ) * glide;
        m_ambWind += ( m_aWind.load( std::memory_order_relaxed ) - m_ambWind ) * glide;
        m_ambJungle += ( m_aJungle.load( std::memory_order_relaxed ) - m_ambJungle ) * glide;
        m_ambCity += ( m_aCity.load( std::memory_order_relaxed ) - m_ambCity ) * glide;
        m_ambRain += ( m_aRain.load( std::memory_order_relaxed ) - m_ambRain ) * glide;
        m_ambTunnel += ( m_aTunnel.load( std::memory_order_relaxed ) - m_ambTunnel ) * 0.0002f;
        m_ambBridge += ( m_aBridge.load( std::memory_order_relaxed ) - m_ambBridge ) * 0.0004f;
        m_ambUnder += ( m_aUnder.load( std::memory_order_relaxed ) - m_ambUnder ) * 0.0002f;
        m_ambTime += 1.0f / SampleRate;
        if ( m_ambTime > 3600.0f ) m_ambTime -= 3600.0f;
        const float dt = 1.0f / SampleRate;
        float       mono = 0.0f, side = 0.0f;

        // Surf: swells every seven seconds, a rush and a hiss as they break.
        if ( m_ambSurf > 0.001f )
        {
            const float swell = 0.5f - 0.5f * std::cos( Pi2 * m_ambTime / 7.3f ), crash = swell * swell * swell;
            if ( ( m_ambCoef & 31 ) == 0 )
            {
                m_ambSurfLo.set( 280.0f + 900.0f * crash, 0.7f );
                m_ambSurfHi.set( 2600.0f, 0.8f );
            }
            const float wave = m_ambSurfLo.low( m_ambN.next() ) * ( 0.3f + 0.9f * crash ) +
                               m_ambSurfHi.band( m_ambN.next() ) * crash * 0.35f;
            side += wave * m_ambSurf * 0.8f;
        }
        // Wind: a band of noise that gusts.
        if ( m_ambWind > 0.001f )
        {
            const float gust =
                0.55f + 0.3f * std::sin( Pi2 * m_ambTime / 5.1f ) + 0.15f * std::sin( Pi2 * m_ambTime / 1.7f );
            if ( ( m_ambCoef & 31 ) == 0 ) m_ambWindBp.set( 380.0f + 420.0f * gust, 1.6f );
            mono += m_ambWindBp.band( m_ambN.next() ) * gust * m_ambWind * 0.6f;
        }
        // Rain: a hiss and drops.
        if ( m_ambRain > 0.001f )
        {
            if ( ( m_ambCoef & 31 ) == 0 ) m_ambRainHp.set( 4200.0f, 0.6f );
            if ( m_ambN.unit() < 0.0016f * m_ambRain ) m_ambDroplet = 1.0f;
            m_ambDroplet *= 0.993f;
            mono +=
                ( m_ambRainHp.band( m_ambN.next() ) * 0.5f + m_ambN.next() * m_ambDroplet * 0.25f ) * m_ambRain * 0.6f;
        }
        // The city: a low rumble and now and then a horn far off.
        if ( m_ambCity > 0.001f )
        {
            if ( ( m_ambCoef & 31 ) == 0 )
            {
                m_ambCityLo.set( 160.0f, 0.7f );
                m_ambHornLo.set( 1300.0f, 0.7f );
            }
            float y = m_ambCityLo.low( m_ambN.next() ) * 0.9f;
            m_ambHornIn -= dt;
            if ( m_ambHornIn <= 0.0f )
            {
                m_ambHorn = 0.35f + 0.25f * m_ambN.unit();
                m_ambHornF = 380.0f + 160.0f * m_ambN.unit();
                m_ambHornIn = 4.0f + 9.0f * m_ambN.unit();
            }
            if ( m_ambHorn > 0.0f )
            {
                m_ambHorn -= dt;
                const float sq =
                    std::sin( Pi2 * m_ambHornF * m_ambTime ) + std::sin( Pi2 * m_ambHornF * 1.26f * m_ambTime );
                y += m_ambHornLo.low( sq ) * 0.18f * std::min( 1.0f, m_ambHorn * 8.0f );
            }
            mono += y * m_ambCity * 1.1f;
        }
        // The jungle: a chorus of insects and birdsong.
        if ( m_ambJungle > 0.001f )
        {
            m_ambInsectPh += 4700.0f * dt;
            if ( m_ambInsectPh >= 1.0f ) m_ambInsectPh -= 1.0f;
            m_ambAmPh += 38.0f * dt;
            if ( m_ambAmPh >= 1.0f ) m_ambAmPh -= 1.0f;
            const float chorus = std::max( 0.0f, std::sin( Pi2 * m_ambTime / 3.7f ) ) * 0.7f + 0.3f;
            float       y = std::sin( Pi2 * m_ambInsectPh ) * ( m_ambAmPh < 0.35f ? 1.0f : 0.0f ) * chorus * 0.12f;
            m_ambBirdIn -= dt;
            if ( m_ambBirdIn <= 0.0f )
            {
                m_ambBird = 0.42f;
                m_ambBirdF = 2200.0f + 1800.0f * m_ambN.unit();
                m_ambBirdIn = 1.2f + 3.5f * m_ambN.unit();
            }
            if ( m_ambBird > 0.0f )
            {
                m_ambBird -= dt;
                const float chirp = std::fmod( 0.42f - m_ambBird, 0.14f ) / 0.14f;   // three falling chirps
                m_ambBirdPh += ( m_ambBirdF * ( 1.3f - 0.5f * chirp ) ) * dt;
                if ( m_ambBirdPh >= 1.0f ) m_ambBirdPh -= 1.0f;
                y += std::sin( Pi2 * m_ambBirdPh ) * std::sin( 3.14159f * chirp ) * 0.3f;
            }
            side -= y * m_ambJungle * 1.4f;   // the far side from the sea
        }
        // A bridge: the joints, front and rear axle.
        if ( m_ambBridge > 0.01f )
        {
            const float speed = m_speed.load( std::memory_order_relaxed );
            m_ambJoint += speed / 16.0f * dt;
            if ( m_ambJoint >= 1.0f )
            {
                m_ambJoint -= 1.0f;
                m_ambThump = 1.0f;
                m_ambThumpDelay = speed > 1.0f ? 2.7f / speed : -1.0f;
            }
            if ( m_ambThumpDelay > 0.0f )
            {
                m_ambThumpDelay -= dt;
                if ( m_ambThumpDelay <= 0.0f ) m_ambThump2 = 0.8f;
            }
            if ( ( m_ambCoef & 31 ) == 0 ) m_ambThumpLo.set( 140.0f, 0.9f );
            const float hit = m_ambThump + m_ambThump2;
            mono += m_ambThumpLo.low( m_ambN.next() * hit * 4.0f ) * m_ambBridge * 0.8f;
            m_ambThump *= 0.9985f;
            m_ambThump2 *= 0.9985f;
        }
        // Under the sea: a deep hum, and now and then a whale far off, calling in a long glide.
        float whale = 0.0f;
        if ( m_ambUnder > 0.001f )
        {
            if ( ( m_ambCoef & 31 ) == 0 ) m_ambSeaLo.set( 70.0f, 0.8f );
            mono += m_ambSeaLo.low( m_ambN.next() ) * m_ambUnder * 0.45f;
            m_ambWhaleIn -= dt;
            if ( m_ambWhaleIn <= 0.0f && m_ambWhale <= 0.0f )
            {
                m_ambWhaleDur = 2.2f + 2.5f * m_ambN.unit();
                m_ambWhale = m_ambWhaleDur;
                m_ambWhaleF0 = 130.0f + 170.0f * m_ambN.unit();
                m_ambWhaleF1 = m_ambWhaleF0 * ( 0.6f + 1.1f * m_ambN.unit() );
                m_ambWhaleIn = 6.0f + 12.0f * m_ambN.unit();
            }
            if ( m_ambWhale > 0.0f )
            {
                m_ambWhale -= dt;
                const float t = 1.0f - m_ambWhale / m_ambWhaleDur;
                const float f = m_ambWhaleF0 +
                                ( m_ambWhaleF1 - m_ambWhaleF0 ) * ( 0.5f - 0.5f * std::cos( 3.14159f * t ) ) +
                                14.0f * std::sin( Pi2 * 5.0f * m_ambTime );
                m_ambWhalePh += f * dt;
                if ( m_ambWhalePh >= 1.0f ) m_ambWhalePh -= 1.0f;
                const float env = std::sin( 3.14159f * t );
                whale = ( std::sin( Pi2 * m_ambWhalePh ) + 0.35f * std::sin( 2.0f * Pi2 * m_ambWhalePh ) +
                          0.15f * std::sin( 3.0f * Pi2 * m_ambWhalePh ) ) *
                        env * env * m_ambUnder * 0.16f;
                mono += whale;
            }
        }
        // A police siren: the long wail of the Americas and Asia, the two tones of Europe; louder as it nears.
        m_ambSiren += ( m_aSiren.load( std::memory_order_relaxed ) - m_ambSiren ) * 0.0005f;
        m_ambSirenPan += ( m_aSirenPan.load( std::memory_order_relaxed ) - m_ambSirenPan ) * 0.0005f;
        float siren = 0.0f;
        if ( m_ambSiren > 0.001f )
        {
            m_ambSirenT += dt;
            if ( m_ambSirenT > 360.0f ) m_ambSirenT -= 360.0f;
            m_ambSirenPitch += ( m_aSirenPitch.load( std::memory_order_relaxed ) - m_ambSirenPitch ) * 0.0005f;
            const float f = ( m_aSirenKind.load( std::memory_order_relaxed ) == 1
                                  ? ( std::fmod( m_ambSirenT, 1.2f ) < 0.6f ? 440.0f : 587.0f )
                                  : 700.0f + 650.0f * ( 0.5f - 0.5f * std::cos( Pi2 * m_ambSirenT / 3.6f ) ) ) *
                            m_ambSirenPitch;
            m_ambSirenPh += f * dt;
            if ( m_ambSirenPh >= 1.0f ) m_ambSirenPh -= 1.0f;
            siren = ( std::sin( Pi2 * m_ambSirenPh ) + 0.4f * std::sin( 2.0f * Pi2 * m_ambSirenPh ) +
                      0.2f * std::sin( 3.0f * Pi2 * m_ambSirenPh ) ) *
                    m_ambSiren * 0.2f;
        }
        else
        {
            m_ambSirenT = 0.0f;
        }
        ++m_ambCoef;

        // The tunnel: the engine thrown back by the walls (and a whale's call by the sea).
        float echo = 0.0f;
        if ( m_ambTunnel > 0.001f )
        {
            static const int lengths[4] = { 2711, 3049, 3307, 3559 };
            for ( size_t k = 0; k < 4; ++k )
            {
                float      &slot = m_ambComb[k][static_cast<size_t>( m_ambCombPos % lengths[k] )];
                const float out = slot;
                m_ambCombDamp[k] += ( out - m_ambCombDamp[k] ) * 0.35f;
                slot = engine + whale * 2.0f + m_ambCombDamp[k] * 0.74f;
                echo += out;
            }
            echo *= 0.25f * m_ambTunnel * 0.9f;
        }
        else if ( m_ambCombPos % 4096 == 0 )
        {
            for ( auto &c : m_ambComb ) c.fill( 0.0f );
        }
        ++m_ambCombPos;
        if ( m_ambCombPos >= 3559 * 3307 ) m_ambCombPos = 0;
        const float pan = clampPan( m_ambPan );
        const float sp = clampPan( m_ambSirenPan );
        l += ( mono + side * ( 1.0f - pan ) * 0.7f + siren * ( 1.0f - 0.6f * sp ) ) * gain + echo * 0.9f;
        r += ( mono + side * ( 1.0f + pan ) * 0.7f + siren * ( 1.0f + 0.6f * sp ) ) * gain + echo * 1.0f;
    }

    /**
     * @brief Clamps a pan position.
     * @param[in] p  the position
     * @return -1..1
     */
    static float clampPan( float p ) { return std::min( 1.0f, std::max( -1.0f, p ) ); }

    std::array<Voice, VoiceCount> m_voices;              ///< the effect voices
    std::array<Event, 8>          m_onBeat{};            ///< cues waiting for the beat
    int                           m_onBeatCount = 0;     ///< how many
    float                         m_onBeatWait = 0.0f;   ///< seconds the oldest waits
    MusicSynth                    m_music;               ///< the radio

    /**
     * @brief Starts an effect voice.
     * @param[in] e  the effect
     */
    void startVoice( const Event &e )
    {
        Voice *v = nullptr;
        for ( Voice &c : m_voices )
            if ( !c.m_on )
            {
                v = &c;
                break;
            }
        if ( !v ) v = &m_voices[static_cast<size_t>( m_engCount ) % m_voices.size()];
        *v = Voice{};
        v->m_on = true;
        v->m_type = e.m_type;
        v->m_strength = e.m_strength;
        v->m_pan = e.m_pan;
        v->m_pitch = e.m_pitch;
        v->m_n.m_s = 0x9E37u + static_cast<uint32_t>( m_engCount * 7919 );
        if ( e.m_type == SfxE::BACKFIRE ) v->m_a = 0.0f;
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
        switch ( v.m_type )
        {
        case SfxE::SHIFT:
            dur = 0.12f;
            v.m_f.set( 300.0f, 0.7f );
            y = v.m_f.low( v.m_n.next() ) * std::exp( -t * 40.0f ) * 1.2f;
            break;
        case SfxE::BACKFIRE:
            dur = 0.45f;
            if ( t >= v.m_a )
            {
                v.m_b = t;
                v.m_a = t + 0.03f + 0.07f * v.m_n.unit();
            }
            v.m_f.set( 1300.0f, 0.6f );
            y = ( v.m_f.low( v.m_n.next() ) * 2.2f + std::sin( Pi2 * 90.0f * ( t - v.m_b ) ) * 0.6f ) *
                std::exp( -( t - v.m_b ) * 70.0f ) * 0.7f;
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
            v.m_f.set( ( 2600.0f * std::exp( -t * 3.5f ) + 260.0f ) * v.m_pitch, 1.1f );
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
            const int          i = std::min( 3, static_cast<int>( t / 0.08f ) );
            const float        lt = t - static_cast<float>( i ) * 0.08f;
            y = std::tanh( std::sin( Pi2 * mtof( notes[i] ) * t ) * 2.5f ) *
                std::exp( -lt * ( i == 3 ? 5.0f : 14.0f ) ) * 0.3f;
            break;
        }
        case SfxE::NEAR_MISS:
            dur = 0.25f;
            v.m_phase += ( 700.0f + 1600.0f * t / 0.25f ) / SampleRate;
            y = std::sin( Pi2 * v.m_phase ) * std::exp( -t * 9.0f ) * 0.3f;
            break;
        case SfxE::BEEP:
            dur = 0.16f;
            y = std::tanh( std::sin( Pi2 * 880.0f * t ) * 3.0f ) * 0.22f;
            break;
        case SfxE::GO:
            dur = 0.5f;
            y = std::tanh( std::sin( Pi2 * 1760.0f * t ) * 3.0f ) * 0.22f * std::exp( -t * 3.0f );
            break;
        case SfxE::TICK:
            dur = 0.03f;
            y = std::sin( Pi2 * 1600.0f * t ) * 0.18f;
            break;
        case SfxE::SELECT:
            dur = 0.14f;
            y = std::sin( Pi2 * ( t < 0.06f ? 880.0f : 1320.0f ) * t ) * 0.22f;
            break;
        case SfxE::WARNING:
            dur = 0.09f;
            y = std::tanh( std::sin( Pi2 * 1200.0f * t ) * 3.0f ) * 0.18f;
            break;
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
            const int          i = std::min( 3, static_cast<int>( t / 0.22f ) );
            y = std::tanh( std::sin( Pi2 * mtof( notes[i] ) * t ) * 2.0f ) * std::exp( -( t - 0.22f * i ) * 4.0f ) *
                0.22f;
            break;
        }
        case SfxE::HORN:
        {
            // Two reedy notes a third apart, pitched a little differently each time.
            dur = 0.32f;
            if ( v.m_a == 0.0f ) v.m_a = 0.85f + 0.3f * v.m_n.unit();
            const float f = 400.0f * v.m_a * v.m_pitch;
            const float env = std::min( 1.0f, t * 80.0f ) * std::min( 1.0f, ( dur - t ) * 30.0f );
            y = std::tanh( ( std::sin( Pi2 * f * t ) + std::sin( Pi2 * f * 1.26f * t ) ) * 2.2f ) * env * 0.16f;
            break;
        }
        case SfxE::JET:
        {
            // A rising, then falling roar: noise whose band slides down as it passes, a rumble under it.
            dur = 7.0f;
            const float env = smoothstepf( 0.0f, 3.2f, t ) * ( 1.0f - smoothstepf( 3.6f, 7.0f, t ) );
            v.m_f.set( 1100.0f - 700.0f * smoothstepf( 2.6f, 4.4f, t ), 0.8f );
            y = ( v.m_f.band( v.m_n.next() ) * 1.6f + std::sin( Pi2 * 48.0f * t ) * 0.25f ) * env;
            break;
        }
        case SfxE::JINGLE:
        {
            // Three sung notes, an "ah" through two formants with a little vibrato, a bell on the last.
            dur = 1.6f;
            static const float tunes[4][3] = { { 67, 72, 76 }, { 72, 67, 79 }, { 64, 67, 72 }, { 69, 72, 76 } };
            const int          which = static_cast<int>( v.m_strength * 10.0f + 0.5f ) & 3;
            const int          i = std::min( 2, static_cast<int>( t / 0.2f ) );
            const float        lt = t - 0.2f * static_cast<float>( i );
            const float        f = mtof( tunes[which][i] ) * ( 1.0f + 0.012f * std::sin( Pi2 * 5.5f * t ) );
            v.m_phase += f / SampleRate;
            if ( v.m_phase >= 1.0f ) v.m_phase -= 1.0f;
            float sung = 0.0f;
            for ( int k = 1; k <= 8; ++k )
            {
                const float h = f * static_cast<float>( k );
                const float w = std::exp( -std::pow( ( h - 750.0f ) / 380.0f, 2.0f ) ) +
                                0.6f * std::exp( -std::pow( ( h - 1150.0f ) / 420.0f, 2.0f ) ) +
                                0.15f / static_cast<float>( k );
                sung += std::sin( Pi2 * v.m_phase * static_cast<float>( k ) ) * w;
            }
            const float bell = i == 2 ? std::sin( Pi2 * f * 2.0f * t ) * std::exp( -lt * 3.0f ) * 0.4f : 0.0f;
            y = ( sung * 0.35f + bell ) * std::min( 1.0f, lt * 25.0f ) * std::exp( -lt * ( i == 2 ? 2.2f : 7.0f ) ) *
                0.2f;
            break;
        }
        case SfxE::VICTORY:
        {
            // A rising arpeggio into a held major chord, slightly overdriven like brass.
            dur = 2.6f;
            static const float notes[4] = { 60, 64, 67, 72 };
            const int          i = std::min( 3, static_cast<int>( t / 0.11f ) );
            const float        lt = t - 0.11f * static_cast<float>( i );
            float              s = std::sin( Pi2 * mtof( notes[i] ) * t );
            if ( i == 3 )
                s += std::sin( Pi2 * mtof( 76.0f ) * t ) + std::sin( Pi2 * mtof( 79.0f ) * t ) +
                     0.5f * std::sin( Pi2 * mtof( 48.0f ) * t );
            y = std::tanh( s * 1.6f ) * std::min( 1.0f, lt * 60.0f ) * std::exp( -lt * ( i == 3 ? 1.4f : 10.0f ) ) *
                0.24f;
            break;
        }
        default: break;
        }
        v.m_t += 1.0f / SampleRate;
        if ( v.m_t > dur ) v.m_on = false;
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
        m_engRpm += ( rpmT - m_engRpm ) * 0.0012f;
        m_engThr += ( thrT - m_engThr ) * 0.003f;
        m_engSpeed += ( m_speed.load( std::memory_order_relaxed ) - m_engSpeed ) * 0.001f;
        m_engSlip += ( m_slip.load( std::memory_order_relaxed ) - m_engSlip ) * 0.002f;
        m_engOff += ( m_offroad.load( std::memory_order_relaxed ) - m_engOff ) * 0.002f;
        m_engRumble += ( m_rumble.load( std::memory_order_relaxed ) - m_engRumble ) * 0.004f;
        m_engActive += ( ( m_active.load( std::memory_order_relaxed ) ? 1.0f : 0.0f ) - m_engActive ) * 0.0005f;
        m_engBoost += ( ( m_boost.load( std::memory_order_relaxed ) ? 1.0f : 0.0f ) - m_engBoost ) * 0.0008f;
        const float load = std::min( 1.0f, m_engThr + 0.3f * m_engBoost );
        const float spoolTarget = m_engThr * std::min( 1.0f, m_engRpm * 1.2f );
        m_engSpool += ( spoolTarget - m_engSpool ) * ( spoolTarget > m_engSpool ? 0.00012f : 0.0004f );
        if ( m_engPrevThr > 0.7f && m_engThr < 0.45f && m_engSpool > 0.4f && m_engActive > 0.5f )
        {
            startVoice( Event{ SfxE::BLOWOFF, m_engSpool * 0.9f, 0.0f } );
            m_engSpool *= 0.5f;
        }
        m_engPrevThr = m_engThr;

        const int   cyl = m_cylinders.load( std::memory_order_relaxed );
        const float idle = m_idle.load( std::memory_order_relaxed ), red = m_redline.load( std::memory_order_relaxed );
        const float bright = m_bright.load( std::memory_order_relaxed );
        const float fire = ( idle + m_engRpm * ( red - idle ) ) / 60.0f * static_cast<float>( cyl ) * 0.5f;
        if ( ( m_engCoef++ & 31 ) == 0 )
        {
            m_engBody.set( ( 250.0f + 2600.0f * ( 0.25f + 0.75f * load ) * ( 0.35f + m_engRpm ) ) * bright, 0.9f );
            m_engIntake.set( std::min( fire * 2.0f, 8000.0f ), 4.0f );
            m_engWind.set( 250.0f + m_engSpeed * 18.0f, 0.6f );
            m_engSquealF.set( 1800.0f, 6.0f );
            m_engGravel.set( 900.0f, 0.6f );
            m_engRumbleF.set( 350.0f, 0.8f );
            m_engHiss.set( 5200.0f, 0.7f );
        }

        // Firing pulses of a cross-plane V8: uneven, slightly random.
        m_engPhase += fire / SampleRate;
        if ( m_engPhase >= 1.0f )
        {
            m_engPhase -= 1.0f;
            ++m_engCount;
            m_engPulseAmp = 0.8f + 0.35f * m_engN.unit();
            const int k = m_engCount & 3;
            m_engUneven = cyl != 8 ? 1.0f : k == 1 ? 1.3f : k == 3 ? 0.85f : 1.0f;
            if ( m_engThr < 0.1f && m_engRpm > 0.55f && m_engN.unit() < 0.02f * m_engRpm ) m_engPop = 1.0f;
        }
        const float decay = ( 4.0f + 10.0f * ( 1.0f - load * 0.8f ) ) * ( cyl >= 12 ? 1.6f : cyl <= 4 ? 0.8f : 1.0f );
        float       pulse =
            std::exp( -m_engPhase * decay ) * m_engPulseAmp * m_engUneven - ( 1.0f - std::exp( -decay ) ) / decay;
        m_engSub += fire * 0.5f / SampleRate;
        if ( m_engSub >= 1.0f ) m_engSub -= 1.0f;
        const float sub = cyl <= 4 ? 0.45f : cyl >= 12 ? 0.12f : 0.3f;
        const float x = pulse * 0.9f + std::sin( Pi2 * m_engSub ) * sub * ( 1.2f - m_engRpm );
        float       lo, bd, ilo, ibd;
        m_engBody.process( x, lo, bd );
        m_engIntake.process( x, ilo, ibd );
        float y = lo + ibd * ( cyl <= 4 ? 0.55f : 0.3f ) * ( 0.3f + load );
        y = std::tanh( y * ( 1.2f + 2.5f * load ) ) * ( 0.35f + 0.35f * load + 0.15f * m_engRpm );
        if ( m_engPop > 0.001f )
        {
            y += ( m_engN.next() * 0.8f + 0.4f ) * m_engPop * 0.6f;
            m_engPop *= 0.9965f;
        }
        if ( m_limiter.load( std::memory_order_relaxed ) )
        {
            m_engLimPh += 26.0f / SampleRate;
            if ( m_engLimPh >= 1.0f ) m_engLimPh -= 1.0f;
            if ( m_engLimPh > 0.55f ) y *= 0.25f;
        }
        const bool shifting = m_shifting.load( std::memory_order_relaxed );
        if ( shifting && !m_engWasShifting ) m_engShift = 0.15f;
        m_engWasShifting = shifting;
        m_engShift += ( 1.0f - m_engShift ) * 0.0006f;
        y *= m_engShift;

        m_engWhistle += ( 1800.0f + 5200.0f * m_engSpool ) / SampleRate;
        if ( m_engWhistle >= 1.0f ) m_engWhistle -= 1.0f;
        float engine = ( y + std::sin( Pi2 * m_engWhistle ) * 0.03f * m_engSpool * m_engSpool ) * m_engActive;
        engine += m_engHiss.band( m_engN.next() ) * 0.09f * m_engBoost * m_engActive;

        // Tyres, wind, kerbs, gravel.
        m_engVib += 7.0f / SampleRate;
        if ( m_engVib >= 1.0f ) m_engVib -= 1.0f;
        m_engSqueal += ( 1050.0f + 40.0f * std::sin( Pi2 * m_engVib ) ) / SampleRate;
        if ( m_engSqueal >= 1.0f ) m_engSqueal -= 1.0f;
        float env =
            ( std::sin( Pi2 * m_engSqueal ) * 0.5f + m_engSquealF.band( m_engN.next() ) * 1.6f ) * m_engSlip * 0.2f;
        const float wind = m_engSpeed / 85.0f;
        env += m_engWind.low( m_engN.next() ) * wind * wind * 0.5f;
        m_engRumblePh += ( 45.0f + m_engSpeed * 1.4f ) / SampleRate;
        if ( m_engRumblePh >= 1.0f ) m_engRumblePh -= 1.0f;
        env += m_engRumbleF.low( m_engRumblePh < 0.5f ? 1.0f : -1.0f ) * m_engRumble * 0.2f *
               std::min( 1.0f, m_engSpeed / 15.0f );
        const float impulse = m_engN.unit() < m_engSpeed * 0.0008f ? m_engN.next() * 2.0f : 0.0f;
        env += m_engGravel.band( impulse + m_engN.next() * 0.15f ) * m_engOff * 0.6f;
        return engine + env;
    }

    /**
     * @brief Fills the stream's buffer.
     * @param[out] out     interleaved stereo floats
     * @param[in]  frames  frames to render
     */
    void render( float *out, unsigned frames )
    {
        while ( m_tail.load( std::memory_order_acquire ) != m_head.load( std::memory_order_acquire ) )
        {
            const uint32_t t = m_tail.load( std::memory_order_relaxed );
            const Event   &e = m_queue[t % m_queue.size()];
            // Cues flagged by a negative pan wait for the beat (the pan is restored).
            if ( e.m_pan < -1.5f && m_music.audible() && m_onBeatCount < static_cast<int>( m_onBeat.size() ) )
            {
                if ( m_onBeatCount == 0 ) m_onBeatWait = 0.0f;
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
        if ( cmd == 1 ) m_music.request( m_musicSeed.load(), m_musicStation.load() );
        if ( cmd == 2 ) m_music.stop();
        const float sfx = m_sfxVol.load( std::memory_order_relaxed );
        const float mus = m_musicVol.load( std::memory_order_relaxed );
        for ( unsigned i = 0; i < frames; ++i )
        {
            const float engine = renderEngine() * sfx;
            m_engWidth[m_engWi & 63] = engine;
            const float delayed = m_engWidth[( m_engWi - 19 ) & 63];
            ++m_engWi;
            float l = engine * 0.9f + delayed * 0.1f;
            float r = engine * 0.75f + delayed * 0.25f;
            renderAmbience( engine, sfx, l, r );
            for ( Voice &v : m_voices )
            {
                if ( !v.m_on ) continue;
                const float s = renderVoice( v ) * sfx;
                l += s * std::min( 1.0f, 1.0f - v.m_pan );
                r += s * std::min( 1.0f, 1.0f + v.m_pan );
            }
            // Under the sea everything but the music comes through glass and water.
            if ( m_ambUnder > 0.01f )
            {
                if ( ( m_engWi & 31 ) == 0 )
                {
                    const float fc = 18000.0f * std::pow( 650.0f / 18000.0f, m_ambUnder );
                    m_ambMufL.set( fc, 0.6f );
                    m_ambMufR.set( fc, 0.6f );
                }
                l = m_ambMufL.low( l ) * ( 1.0f + 0.3f * m_ambUnder );
                r = m_ambMufR.low( r ) * ( 1.0f + 0.3f * m_ambUnder );
            }
            float ml, mr;
            m_music.tick( ml, mr );
            if ( m_onBeatCount > 0 )
            {
                m_onBeatWait += 1.0f / SampleRate;
                if ( m_music.m_beatNow || m_onBeatWait > 0.7f )
                {
                    for ( int k = 0; k < m_onBeatCount; ++k ) startVoice( m_onBeat[static_cast<size_t>( k )] );
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
    if ( g_synth ) g_synth->render( static_cast<float *>( buffer ), frames );
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

}   // namespace

bool Audio::renderTest( const std::string &path, float seconds, int station )
{
    auto               synth = std::make_unique<Synth>();
    const unsigned     frames = static_cast<unsigned>( seconds * SampleRate );
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
    static const Cue cues[] = { { 6.0f, SfxE::WHOOSH, -0.7f },    { 9.0f, SfxE::CHECKPOINT, 0.0f },
                                { 11.0f, SfxE::NEAR_MISS, 0.0f }, { 12.0f, SfxE::BOOST, 0.0f },
                                { 16.2f, SfxE::BACKFIRE, 0.0f },  { 18.0f, SfxE::CRASH, 0.0f } };
    size_t           cue = 0;
    const unsigned   block = 512;
    for ( unsigned f = 0; f < frames; f += block )
    {
        const float t = static_cast<float>( f ) / SampleRate;
        float       rpm = 0.12f, thr = 0.0f, speed = 0.0f;
        bool        shifting = false;
        if ( t < 2.0f )
        {
            thr = t > 1.0f && t < 1.5f ? 1.0f : 0.0f;
            rpm = thr > 0.0f ? 0.8f : 0.12f;
        }
        else if ( t < 16.0f )
        {
            float tt = t - 2.0f;
            int   g = 0;
            while ( g < 5 && tt > gearTime[g] )
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
        if ( ambience )
        {   // Five seconds of each: surf, wind, jungle, city, rain, a bridge, a tunnel with the engine.
            const int k = static_cast<int>( t / 5.0f );
            synth->m_aSurf = k == 0 ? 1.0f : 0.0f;
            synth->m_aSurfPan = 0.8f;
            synth->m_aWind = k == 1 ? 1.0f : 0.0f;
            synth->m_aJungle = k == 2 ? 1.0f : 0.0f;
            synth->m_aCity = k == 3 ? 1.0f : 0.0f;
            synth->m_aRain = k == 4 ? 1.0f : 0.0f;
            synth->m_aBridge = k == 5 ? 1.0f : 0.0f;
            synth->m_aTunnel = k == 6 ? 1.0f : 0.0f;
            if ( k >= 5 )
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
        if ( adaptive )
        {
            synth->m_tension = t > 15.0f && t < 32.0f ? 1.0f : 0.0f;
            synth->m_drive = 0.5f + 0.5f * std::sin( t * 0.3f );
            for ( int k = 0; k < 2; ++k )
                if ( !cued[k] && t >= ( k == 0 ? 8.0f : 33.0f ) )
                {
                    const uint32_t h = synth->m_head.load();
                    synth->m_queue[h % synth->m_queue.size()] =
                        Event{ k == 0 ? SfxE::CHECKPOINT : SfxE::VICTORY, 1.0f, -2.0f };
                    synth->m_head.store( h + 1 );
                    cued[k] = true;
                }
        }
        while ( drive && cue < std::size( cues ) && cues[cue].m_t <= t )
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
    if ( txt )
    {
        float allPeak = 0.0f;
        for ( unsigned s = 0; s * SampleRate < frames; ++s )
        {
            const unsigned a = static_cast<unsigned>( s * SampleRate ),
                           b = std::min( frames, static_cast<unsigned>( ( s + 1 ) * SampleRate ) );
            double         sum = 0.0;
            float          peak = 0.0f;
            for ( unsigned i = a * 2; i < b * 2; ++i )
            {
                peak = std::max( peak, std::abs( buf[i] ) );
                sum += static_cast<double>( buf[i] ) * buf[i];
            }
            allPeak = std::max( allPeak, peak );
            const double rms = std::sqrt( sum / std::max( 1u, ( b - a ) * 2 ) );
            std::fprintf( txt, "%2u s  peak %6.1f dBFS  rms %6.1f dBFS\n", s,
                          20.0 * std::log10( std::max( 1e-6f, peak ) ), 20.0 * std::log10( std::max( 1e-9, rms ) ) );
        }
        std::fprintf( txt, "overall peak %.1f dBFS\n", 20.0 * std::log10( std::max( 1e-6f, allPeak ) ) );
        std::fclose( txt );
    }
    return ok;
}

bool Audio::init( const std::vector<std::string> &musicDirs )
{
    InitAudioDevice();
    if ( !IsAudioDeviceReady() ) return false;
    g_synth = new Synth();
    SetAudioStreamBufferSizeDefault( 1024 );
    m_stream = LoadAudioStream( static_cast<unsigned>( SampleRate ), 32, 2 );
    SetAudioStreamCallback( m_stream, audioCallback );
    PlayAudioStream( m_stream );
    m_ok = true;
    for ( const std::string &dir : musicDirs )
    {
        std::error_code ec;
        if ( !std::filesystem::is_directory( dir, ec ) ) continue;
        for ( const auto &entry : std::filesystem::directory_iterator( dir, ec ) )
        {
            std::string ext = entry.path().extension().string();
            std::transform( ext.begin(), ext.end(), ext.begin(),
                            []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
            if ( ext == ".ogg" || ext == ".mp3" || ext == ".flac" || ext == ".wav" || ext == ".xm" || ext == ".mod" )
                m_files.push_back( entry.path().string() );
        }
    }
    std::sort( m_files.begin(), m_files.end() );
    return true;
}

void Audio::shutdown()
{
    if ( !m_ok ) return;
    if ( m_musicLoaded ) UnloadMusicStream( m_music );
    m_musicLoaded = false;
    UnloadAudioStream( m_stream );
    CloseAudioDevice();
    delete g_synth;
    g_synth = nullptr;
    m_ok = false;
}

void Audio::setEngine( const EngineState &s )
{
    if ( !g_synth ) return;
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
    if ( !g_synth ) return;
    g_synth->m_aSurf.store( a.m_surf, std::memory_order_relaxed );
    g_synth->m_aSurfPan.store( a.m_surfPan, std::memory_order_relaxed );
    g_synth->m_aWind.store( a.m_wind, std::memory_order_relaxed );
    g_synth->m_aJungle.store( a.m_jungle, std::memory_order_relaxed );
    g_synth->m_aCity.store( a.m_city, std::memory_order_relaxed );
    g_synth->m_aRain.store( a.m_rain, std::memory_order_relaxed );
    g_synth->m_aTunnel.store( a.m_tunnel, std::memory_order_relaxed );
    g_synth->m_aBridge.store( a.m_bridge, std::memory_order_relaxed );
    g_synth->m_aUnder.store( a.m_underwater, std::memory_order_relaxed );
    g_synth->m_aSiren.store( a.m_siren, std::memory_order_relaxed );
    g_synth->m_aSirenPan.store( a.m_sirenPan, std::memory_order_relaxed );
    g_synth->m_aSirenKind.store( a.m_sirenKind, std::memory_order_relaxed );
    g_synth->m_aSirenPitch.store( a.m_sirenPitch, std::memory_order_relaxed );
}

void Audio::play( SfxE type, float strength, float pan, float pitch )
{
    if ( !g_synth ) return;
    const uint32_t h = g_synth->m_head.load( std::memory_order_relaxed );
    if ( h - g_synth->m_tail.load( std::memory_order_acquire ) >= g_synth->m_queue.size() ) return;
    g_synth->m_queue[h % g_synth->m_queue.size()] =
        Event{ type, std::clamp( strength, 0.0f, 1.5f ), std::clamp( pan, -1.0f, 1.0f ),
               std::clamp( pitch, 0.5f, 2.0f ) };
    g_synth->m_head.store( h + 1, std::memory_order_release );
}

void Audio::playOnBeat( SfxE type, float strength )
{
    if ( !g_synth ) return;
    const uint32_t h = g_synth->m_head.load( std::memory_order_relaxed );
    if ( h - g_synth->m_tail.load( std::memory_order_acquire ) >= g_synth->m_queue.size() ) return;
    g_synth->m_queue[h % g_synth->m_queue.size()] = Event{ type, std::clamp( strength, 0.0f, 1.5f ), -2.0f };
    g_synth->m_head.store( h + 1, std::memory_order_release );
}

void Audio::setMood( float tension, float drive )
{
    if ( !g_synth ) return;
    g_synth->m_tension = clampf01( tension );
    g_synth->m_drive = clampf01( drive );
}

void Audio::setVolumes( float music, float sfx )
{
    m_musicVolume = music;
    if ( g_synth )
    {
        g_synth->m_musicVol = music;
        g_synth->m_sfxVol = sfx;
    }
    if ( m_musicLoaded ) SetMusicVolume( m_music, music * 0.8f );
}

void Audio::startMusic( uint64_t seed, int station )
{
    if ( !m_ok ) return;
    m_musicOn = true;
    m_fileMode = station >= StationCount && !m_files.empty();
    if ( m_fileMode )
    {
        g_synth->m_musicCmd = 2;
        if ( m_musicLoaded ) UnloadMusicStream( m_music );
        m_music = LoadMusicStream( m_files[m_fileIndex % m_files.size()].c_str() );
        m_fileIndex = ( m_fileIndex + 1 ) % m_files.size();
        m_musicLoaded = IsMusicValid( m_music );
        if ( m_musicLoaded )
        {
            m_music.looping = false;
            SetMusicVolume( m_music, m_musicVolume * 0.8f );
            PlayMusicStream( m_music );
        }
        return;
    }
    if ( m_musicLoaded ) StopMusicStream( m_music );
    g_synth->m_musicSeed = seed;
    g_synth->m_musicStation = std::clamp( station, 0, StationCount - 1 );
    g_synth->m_musicCmd = 1;
}

void Audio::stopMusic()
{
    m_musicOn = false;
    if ( m_musicLoaded ) StopMusicStream( m_music );
    if ( g_synth ) g_synth->m_musicCmd = 2;
}

void Audio::update()
{
    if ( !m_musicLoaded ) return;
    UpdateMusicStream( m_music );
    if ( m_musicOn && m_fileMode && !IsMusicStreamPlaying( m_music ) ) startMusic( 0, StationCount );
}

}   // namespace sunstreak
