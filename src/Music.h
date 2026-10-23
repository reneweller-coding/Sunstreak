/**
 * @file Music.h
 * @brief The generative soundtrack: eight radio stations, each a style, each track composed from a seed.
 *
 * Stations: Sunset FM (synthwave), Turbo House, Amiga Techno (acid, tracker leads), Trance Gate, Jungle Pressure
 * (drum and bass), 8-Bit Racer (chiptune), Fusion Drive (funk fusion), Big Beat (breaks). A track is 32 bars that
 * loop with an arrangement (intro, groove, full, breakdown, build); a request crossfades to a new track.
 *
 * Runs on the audio thread; nothing here allocates after start().
 *
 * @author reneweller
 */
#pragma once

#include "Dsp.h"

#include <cstdint>
#include <string>
#include <vector>

namespace sunstreak
{

constexpr int StationCount = 8;   ///< generated stations

/**
 * @brief A station's name.
 * @param[in] station  0..StationCount-1
 * @return its name
 */
const char *stationName( int station );

/**
 * @brief A track's title, made from its seed.
 * @param[in] seed  the track's seed
 * @return two words, e.g. "CHROME HORIZON"
 */
std::string trackTitle( uint64_t seed );

/** @brief The synthesiser of the soundtrack. */
class MusicSynth
{
public:
    /** @brief Allocates the delay and reverb lines. */
    MusicSynth();

    /**
     * @brief Crossfades to a new track: the current one fades out, then the new one starts.
     * @param[in] seed     the track's seed
     * @param[in] station  its station (style)
     */
    void request( uint64_t seed, int station );

    /** @brief Fades out and stops. */
    void stop() { m_pending = false; m_on = false; }

    /**
     * @brief Sets the game's mood: tension adds a driving layer, drive opens the pads.
     * @param[in] tension  0..1 (little time left, the last kilometre of a race)
     * @param[in] drive    0..1 (speed)
     */
    void setMood( float tension, float drive )
    {
        m_tensionTarget = tension;
        m_drive = drive;
    }

    /**
     * @brief Whether a track is audible.
     * @return true while playing or fading
     */
    bool audible() const { return m_gain > 0.05f; }

    bool m_beatNow = false;   ///< set for the one sample on which a beat began

    /**
     * @brief Renders one stereo sample.
     * @param[out] l  left
     * @param[out] r  right
     */
    void tick( float &l, float &r );

private:
    /**
     * @brief Composes a track at once.
     * @param[in] seed     its seed
     * @param[in] station  its station
     */
    void start( uint64_t seed, int station );

    /** @brief Triggers what happens on the current sixteenth. */
    void onStep();

    /**
     * @brief Triggers the bass on a step.
     * @param[in] s16   the step in the bar
     * @param[in] root  the chord's root (semitones)
     */
    void bassStep( int s16, int root );

    /**
     * @brief Renders the bass.
     * @return its sample
     */
    float renderBass();

    /**
     * @brief Renders the lead.
     * @return its sample
     */
    float renderLead();

    /**
     * @brief Renders the pad or the chord stabs.
     * @return its sample
     */
    float renderPad();

    /**
     * @brief Renders the drums.
     * @param[out] reverbSend  the snare's share for the reverb
     * @return the drums' sample
     */
    float renderDrums( float &reverbSend );

    /**
     * @brief The small Schroeder reverb.
     * @param[in]  in  mono input
     * @param[out] l   left output
     * @param[out] r   right output
     */
    void reverb( float in, float &l, float &r );

    // Arrangement and style.
    bool     m_on = false;             ///< playing (or fading in)
    bool     m_pending = false;        ///< a new track waits for the fade-out
    uint64_t m_pendingSeed = 0;        ///< its seed
    int      m_pendingStation = 0;     ///< its station
    float    m_gain = 0.0f;            ///< fade
    int      m_station = 0;            ///< current style
    float    m_bpm = 120.0f;           ///< tempo
    float    m_swing = 0.0f;           ///< swing of the odd sixteenths (0..0.3)
    int      m_key = 0;                ///< transposition
    bool     m_major = false;          ///< major key
    int      m_prog[4] = {};           ///< chord roots
    bool     m_minorChord[4] = {};     ///< chord qualities
    int      m_lead[32] = {};          ///< four bars of eighths: scale step or -1
    int      m_acid[16] = {};          ///< acid line: semitone or -1
    bool     m_accent[16] = {};        ///< acid accents
    bool     m_slide[16] = {};         ///< acid slides
    int      m_variant = 0;            ///< pattern variant
    double   m_clock = 0.0;            ///< samples into the current step
    int      m_step = 0;               ///< sixteenth counter

    // Voices.
    float m_kickT = 9, m_kickPh = 0, m_kickVel = 1;
    float m_snareT = 9, m_snareVel = 1;
    float m_hatT = 9, m_hatVel = 1;
    bool  m_hatOpen = false;
    float m_bassT = 9, m_bassF = 55, m_bassTarget = 55, m_bassPh[3] = {}, m_bassVel = 1, m_bassGate = 0;
    bool  m_bassAccent = false;
    float m_leadT = 9, m_leadF = 440, m_leadPh[3] = {}, m_leadEnv = 0;
    bool  m_leadGate = false;
    float m_arpT = 9, m_arpF = 440, m_arpPh = 0;
    float m_padF[3] = { 220, 262, 330 }, m_padPh[6] = {}, m_padEnv = 0, m_gateEnv = 1;
    float m_stabT = 9, m_stabPh[6] = {};
    float m_chipChord[3] = {};       ///< chip arpeggio chord (Hz)
    int   m_chipIndex = 0;           ///< which note of it sounds
    float m_tension = 0.0f;          ///< smoothed tension
    float m_tensionTarget = 0.0f;    ///< the game's tension
    float m_drive = 0.0f;            ///< the game's speed
    float m_tHatT = 9.0f;            ///< tension hat
    float m_tBlipT = 9.0f, m_tBlipF = 880.0f, m_tBlipPh = 0.0f;   ///< tension alarm blip

    dsp::Svf   m_snareF, m_hatF, m_bassF1, m_bassF2, m_leadF1, m_arpF1, m_padF1, m_stabF1, m_kickClick;
    dsp::Noise m_n;
    dsp::Svf   m_tHatF;   ///< the tension hats' filter
    dsp::Noise m_tN;      ///< their own noise, so the groove stays as it was

    std::vector<float> m_delayL, m_delayR;   ///< ping-pong echo
    size_t             m_di = 0;             ///< echo write index
    std::vector<float> m_comb[8];            ///< reverb combs (4 per side)
    size_t             m_combI[8] = {};      ///< their indices
    float              m_combLp[8] = {};     ///< their damping states
    std::vector<float> m_ap[4];              ///< reverb all-passes (2 per side)
    size_t             m_apI[4] = {};        ///< their indices
};

} // namespace sunstreak
