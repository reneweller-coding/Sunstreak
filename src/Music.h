/**
 * @file Music.h
 * @brief The generative soundtrack: twenty-six radio stations, each a style, each track composed from a seed.
 *
 * Eight genre stations (synthwave, house, acid techno, trance, drum and bass, chiptune, funk fusion, big beat), sixteen
 * that carry a region's colours (city pop, koto, Bollywood, desert maqam, afrobeat, samba, tango, the Andes, Caribbean
 * steel, mariachi, Celtic, Balkan brass, gamelan, the outback, the steppe, the north), Route 66 and lo-fi. A track picks
 * its tempo, key, scale or mode, chord progression, two melodies with their rhythms, one of the style's lead
 * instruments and an arrangement of 32 to 64 bars with fills, crashes and risers; the arrangement loops until the next
 * track. A request crossfades to a new track.
 *
 * Runs on the audio thread; nothing here allocates after the constructor.
 *
 * @author reneweller
 */
#pragma once

#include "Dsp.h"
#include "Rng.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sunstreak
{

constexpr int StationCount = 26;   ///< generated stations

/** @name Stations with a region (the biomes' AUTO choice) */
///@{
constexpr int StationCityPop = 8;      ///< Japan, Korea: city pop
constexpr int StationKoto = 9;         ///< East Asia: koto, taiko, pentatonic
constexpr int StationBollywood = 10;   ///< South Asia: sitar, tabla, tanpura
constexpr int StationDesert = 11;      ///< North Africa, the Middle East: oud, darbuka, Hijaz
constexpr int StationAfro = 12;        ///< Africa: congas, kalimba, highlife guitar
constexpr int StationSamba = 13;       ///< Brazil: surdo, agogo, nylon guitar
constexpr int StationTango = 14;       ///< the Southern Cone: bandoneon, habanera
constexpr int StationAndes = 15;       ///< the Andes: pan flute, charango, bombo
constexpr int StationCaribbean = 16;   ///< the Caribbean: steel drums, skank organ
constexpr int StationMariachi = 17;    ///< Mexico, Central America: trumpets, accordion
constexpr int StationCeltic = 18;      ///< Ireland, Scotland, Wales, Brittany: whistle, fiddle, bodhran
constexpr int StationBalkan = 19;      ///< the Balkans: brass, clarinet, fast two-step
constexpr int StationGamelan = 20;     ///< Southeast Asia: gamelan, kendang, pelog
constexpr int StationOutback = 21;     ///< Australia: didgeridoo, clapsticks, breaks
constexpr int StationSteppe = 22;      ///< Central Asia, Mongolia: gallop, fiddle, throat drone
constexpr int StationNordic = 23;      ///< Scandinavia, the Baltic, Iceland: fiddle, frame drum, wide pads
constexpr int StationRoute66 = 24;     ///< North America on the open road: twang, harmonica, train beat
constexpr int StationLofi = 25;        ///< lo-fi beats
///@}

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

/** @brief A melodic voice: a synth, a plucked string, a bell, a reed, a flute, a bow (Music.cpp). */
struct Tone
{
    int                m_inst = 0;          ///< the instrument (InstE)
    float              m_f = 440.0f;        ///< current frequency
    float              m_target = 440.0f;   ///< where a slide goes
    float              m_ph[4] = {};        ///< oscillator phases
    float              m_t = 9.0f;          ///< seconds since the note began
    float              m_env = 0.0f;        ///< amplitude envelope
    float              m_vel = 1.0f;        ///< velocity
    bool               m_gate = false;      ///< key held
    std::vector<float> m_string;            ///< Karplus-Strong delay line
    size_t             m_write = 0;         ///< its write index
    float              m_last = 0.0f;       ///< loop filter state
    dsp::Svf           m_f1, m_f2;          ///< filters
    dsp::Noise         m_noise;             ///< breath, bow, pluck excitation
};

/** @brief The synthesiser of the soundtrack. */
class MusicSynth
{
public:
    /** @brief Allocates the delay lines, strings and reverb. */
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
    /** @brief A part of the arrangement. */
    struct Section
    {
        int      m_bars = 4;       ///< length
        uint16_t m_layers = 0;     ///< which parts play (LayerE bits)
        int      m_melody = 0;     ///< melody A or B
        bool     m_riser = false;  ///< a noise sweep towards the next section
    };

    /**
     * @brief Composes a track at once.
     * @param[in] seed     its seed
     * @param[in] station  its station
     */
    void start( uint64_t seed, int station );

    /**
     * @brief Writes one four-bar melody.
     * @param[in,out] rng     the composition's random numbers
     * @param[out]    out     64 sixteenths: scale degree, -1 rest, -2 hold
     * @param[in]     rhythm  rhythm template
     * @param[in]     low     lowest degree
     * @param[in]     high    highest degree
     */
    void compose( Rng &rng, int8_t *out, int rhythm, int low, int high );

    /** @brief Triggers what happens on the current sixteenth. */
    void onStep();

    /**
     * @brief Triggers the bass on a step.
     * @param[in] s16   the step in the bar
     * @param[in] root  the chord's root (semitones)
     * @param[in] fifth the chord's fifth above the root (semitones)
     */
    void bassStep( int s16, int root, int fifth );

    /**
     * @brief The semitones of a scale degree above the key.
     * @param[in] degree  any degree (negative: below)
     * @return semitones
     */
    int semitones( int degree ) const;

    /**
     * @brief Starts a note on a melodic voice.
     * @param[out] v      the voice
     * @param[in]  inst   the instrument
     * @param[in]  f      frequency
     * @param[in]  vel    velocity
     * @param[in]  slide  glide from the previous note instead of starting fresh
     */
    void noteOn( Tone &v, int inst, float f, float vel, bool slide = false );

    /**
     * @brief Renders a melodic voice.
     * @param[in,out] v  the voice
     * @return its sample
     */
    float render( Tone &v );

    /**
     * @brief Renders the bass.
     * @return its sample
     */
    float renderBass();

    /**
     * @brief Renders the chords (pad, stabs, skank, strums, bells).
     * @return its sample
     */
    float renderChords();

    /**
     * @brief Renders the drum kit.
     * @param[out] reverbSend  the snare's share for the reverb
     * @return the drums' sample
     */
    float renderDrums( float &reverbSend );

    /**
     * @brief Renders the hand percussion.
     * @return its sample
     */
    float renderPerc();

    /**
     * @brief Renders the drone.
     * @return its sample
     */
    float renderDrone();

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
    int      m_scale = 0;              ///< ScaleE
    int      m_prog[8] = {};           ///< chord progression in scale degrees
    int      m_progLen = 4;            ///< chords in the progression
    int      m_barsPerChord = 1;       ///< bars each chord lasts
    bool     m_sevenths = false;       ///< chords with their seventh
    int      m_leadInst = 0;           ///< the track's lead instrument
    int      m_counterInst = 0;        ///< the counter line's instrument
    int      m_drumKit = 0;            ///< DrumsE
    int      m_drumVariant = 0;        ///< which of the kit's patterns
    int      m_percVariant = 0;        ///< which of the percussion's patterns
    int      m_variant = 0;            ///< bass and arpeggio variant
    int8_t   m_melody[2][64] = {};     ///< two four-bar melodies in sixteenths
    int8_t   m_counter[64] = {};       ///< the counter line
    int      m_acid[16] = {};          ///< acid line: semitone or -1
    bool     m_accent[16] = {};        ///< acid accents
    bool     m_slide[16] = {};         ///< acid slides
    std::array<Section, 10> m_sections{};   ///< the arrangement
    int      m_sectionCount = 0;       ///< sections used
    int      m_totalBars = 32;         ///< the arrangement's length
    double   m_clock = 0.0;            ///< samples into the current step
    int      m_step = 0;               ///< sixteenth counter
    uint16_t m_layers = 0;             ///< what plays in the current bar
    int      m_root = 0;               ///< current chord root (semitones)
    int      m_chordTones[4] = {};     ///< current chord (semitones above the key)
    int      m_sectionBar = 0;         ///< bar within the current section
    int      m_sectionLen = 4;         ///< current section's bars
    int      m_melodyIndex = 0;        ///< current section's melody
    bool     m_riser = false;          ///< the current section rises

    // Drum kit.
    float m_kickT = 9, m_kickPh = 0, m_kickVel = 1;
    float m_snareT = 9, m_snareVel = 1;
    float m_hatT = 9, m_hatVel = 1;
    bool  m_hatOpen = false;
    float m_crashT = 9;                ///< crash cymbal
    float m_riserT = 9;                ///< noise sweep
    float m_tomT = 9, m_tomF = 120, m_tomPh = 0;   ///< fill toms

    // Hand percussion: low drum, high drum, shaker, bell, click.
    float m_pT[5] = { 9, 9, 9, 9, 9 };  ///< time since each lane's hit
    float m_pVel[5] = {};              ///< their velocity
    float m_pPh[5] = {};               ///< their phase
    float m_pAlt[5] = {};              ///< alternating pitch (agogo, tabla)

    // Bass.
    float m_bassT = 9, m_bassF = 55, m_bassTarget = 55, m_bassPh[3] = {}, m_bassGate = 0;
    bool  m_bassAccent = false;

    // Melodic voices: lead, counter, arpeggio, four chord voices.
    Tone  m_lead, m_counterVoice, m_arp;
    Tone  m_chordVoice[4];
    float m_padF[4] = { 220, 262, 330, 392 }, m_padPh[8] = {}, m_padEnv = 0, m_gateEnv = 1;
    float m_stabT = 9;                 ///< chord stab or skank
    float m_strumDelay[4] = {};        ///< strum offsets of the chord voices still to come (s)
    float m_chordF[4] = {};            ///< their frequencies
    float m_chipChord[3] = {};         ///< chip arpeggio chord (Hz)
    float m_chipPh = 0.0f, m_chipT = 9.0f;   ///< chip arpeggio

    // Drone.
    float m_droneT = 0.0f, m_dronePh[4] = {};
    Tone  m_tanpura;                   ///< the tanpura's string

    // Mood.
    float m_tension = 0.0f;          ///< smoothed tension
    float m_tensionTarget = 0.0f;    ///< the game's tension
    float m_drive = 0.0f;            ///< the game's speed
    float m_tHatT = 9.0f;            ///< tension hat
    float m_tBlipT = 9.0f, m_tBlipF = 880.0f, m_tBlipPh = 0.0f;   ///< tension alarm blip

    dsp::Svf   m_snareF, m_hatF, m_bassF1, m_padF1, m_kickClick, m_crashF, m_riserF, m_droneF1, m_droneF2;
    dsp::Svf   m_pF[5];                ///< percussion filters
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
