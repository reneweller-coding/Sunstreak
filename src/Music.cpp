/**
 * @file Music.cpp
 * @brief The twenty-six radio stations: styles, scales, rhythms, instruments, arrangements, echo and reverb.
 * @author reneweller
 */
#include "Music.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace sunstreak
{

namespace
{

using namespace dsp;

constexpr int StringLength = 4096;   ///< samples in a plucked string's delay line (down to 12 Hz)

/** @brief Drum kits: their patterns and sounds. */
enum class DrumsE
{
    FOUR, SYNTHWAVE, TRANCE, BREAKS, DNB, CHIP, FUNK, TECHNO, DISCO, HALFTIME, BHANGRA, AFRO, SAMBA, HABANERA, SOCA,
    POLKA, GALLOP, ROCK, LOFI, COUNT
};

/** @brief Hand percussion families. */
enum class PercE
{
    NONE, TAIKO, TABLA, DARBUKA, AFRO, SAMBA, ANDES, CARIB, CELTIC, BALKAN, GAMELAN, CLAPSTICKS, STEPPE, SHAKER, COUNT
};

/** @brief Bass lines. */
enum class BassE
{
    OCTAVE,    ///< sixteenths jumping octaves
    OFFBEAT,   ///< offbeat eighths
    ROLLING,   ///< gallop between the kicks
    REESE,     ///< long detuned notes
    CHIP,      ///< pulse arpeggio bass
    FUNKY,     ///< slap-ish syncopation
    BIGBEAT,   ///< distorted riff
    ACID,      ///< resonant line with accents and slides
    SUB,       ///< deep sine with glides
    POLKA,     ///< oom-pah: root and fifth
    SAMBA,     ///< surdo-like two-feel
    HABANERA,  ///< dotted tango figure
    COUNTRY,   ///< root, fifth, a walk-up
    GALLOP,    ///< horse-riding triplet feel
};

/** @brief How the chords are played. */
enum class ChordE
{
    SAW,       ///< warm saw pad
    GATED,     ///< trance gate
    STABS,     ///< electric piano stabs
    STRINGS,   ///< slow strings
    NONE,      ///< none
    STRUM,     ///< strummed strings (guitar, charango, bouzouki)
    SKANK,     ///< organ on the offbeats
    COMP,      ///< electric piano comping with sevenths
    BELLS,     ///< interlocking bells (gamelan)
    BRASS,     ///< brass or string stabs
    PLUCKS,    ///< picked chord tones (highlife, charango)
};

/** @brief Drones under everything. */
enum class DroneE
{
    NONE, TANPURA, DIDGERIDOO, BAGPIPE, THROAT, HUM
};

/** @brief Arpeggios. */
enum class ArpE
{
    NONE, UP, CHIP
};

/** @brief Scales and modes. */
enum class ScaleE
{
    MINOR, MAJOR, DORIAN, PHRYGIAN, MIXOLYDIAN, LYDIAN, HARMONIC, HIJAZ, PENTA_MINOR, PENTA_MAJOR, BLUES, HIRAJOSHI, IN_SEN,
    BHAIRAV, PELOG, GYPSY, COUNT
};

/** @brief Melodic instruments. */
enum InstE : int
{
    SQUARE, SUPERSAW, PULSE, EPIANO, BRASS, PLUCK, GUITAR, TWANG, SITAR, KOTO, OUD, CHARANGO, STEEL_DRUM, MARIMBA, KALIMBA,
    GAMELAN, VIBES, ACCORDION, BANDONEON, HARMONICA, FLUTE, PANFLUTE, NEY, WHISTLE, FIDDLE, ERHU, ORGAN, DIST_GUITAR,
    TRUMPET, CLARINET, INST_COUNT
};

/** @brief The parts of an arrangement. */
enum LayerE : uint16_t
{
    L_DRUMS = 1, L_BASS = 2, L_CHORDS = 4, L_LEAD = 8, L_COUNTER = 16, L_PERC = 32, L_DRONE = 64, L_ARP = 128
};

/** @brief What an instrument is like. */
struct InstInfo
{
    int   m_base;        ///< MIDI note of the melody's degree 0 in C
    bool  m_sustained;   ///< held while the key is down (else it rings out)
    float m_attackMs;    ///< attack
    float m_releaseMs;   ///< release
    float m_level;       ///< loudness trim
    bool  m_slides;      ///< glides between connected notes
};

const InstInfo Insts[INST_COUNT] = {
    { 69, true, 6, 60, 0.8f, false },    { 69, true, 8, 90, 0.75f, true },    { 69, true, 4, 40, 0.6f, false },
    { 64, false, 1, 1, 0.9f, false },    { 64, true, 25, 80, 0.8f, false },   { 69, false, 1, 1, 0.8f, false },
    { 60, false, 1, 1, 1.1f, false },    { 59, false, 1, 1, 1.0f, false },    { 60, false, 1, 1, 1.1f, true },
    { 64, false, 1, 1, 1.0f, false },    { 57, false, 1, 1, 1.2f, false },    { 67, false, 1, 1, 0.9f, false },
    { 67, false, 1, 1, 0.8f, false },    { 67, false, 1, 1, 1.0f, false },    { 72, false, 1, 1, 0.9f, false },
    { 67, false, 1, 1, 0.75f, false },   { 67, false, 1, 1, 0.8f, false },    { 64, true, 25, 70, 0.75f, false },
    { 60, true, 40, 90, 0.85f, false },  { 67, true, 15, 60, 0.75f, true },   { 72, true, 30, 80, 0.85f, false },
    { 72, true, 20, 90, 0.9f, false },   { 67, true, 60, 120, 0.95f, true },  { 79, true, 10, 50, 0.6f, false },
    { 67, true, 50, 90, 0.75f, true },   { 67, true, 60, 120, 0.8f, true },   { 64, true, 4, 40, 0.55f, false },
    { 59, true, 6, 80, 0.55f, true },    { 64, true, 20, 70, 0.75f, false },  { 62, true, 25, 80, 0.85f, false },
};

/** @brief A scale: its steps within the octave. */
struct ScaleDef
{
    int m_n;          ///< notes per octave
    int m_steps[7];   ///< semitones above the tonic
};

const ScaleDef Scales[static_cast<int>( ScaleE::COUNT )] = {
    { 7, { 0, 2, 3, 5, 7, 8, 10 } }, { 7, { 0, 2, 4, 5, 7, 9, 11 } }, { 7, { 0, 2, 3, 5, 7, 9, 10 } },
    { 7, { 0, 1, 3, 5, 7, 8, 10 } }, { 7, { 0, 2, 4, 5, 7, 9, 10 } }, { 7, { 0, 2, 4, 6, 7, 9, 11 } },
    { 7, { 0, 2, 3, 5, 7, 8, 11 } }, { 7, { 0, 1, 4, 5, 7, 8, 10 } }, { 5, { 0, 3, 5, 7, 10 } },
    { 5, { 0, 2, 4, 7, 9 } },        { 6, { 0, 3, 5, 6, 7, 10 } },    { 5, { 0, 2, 3, 7, 8 } },
    { 5, { 0, 1, 5, 7, 10 } },       { 7, { 0, 1, 4, 5, 7, 8, 11 } }, { 5, { 0, 1, 3, 7, 8 } },
    { 7, { 0, 2, 3, 6, 7, 8, 11 } },
};

/** @brief Chord progressions in scale degrees (eight chords; four-chord ones repeat). */
const int8_t PopProgs[][8] = {
    { 0, 5, 3, 4, 0, 5, 3, 4 }, { 0, 3, 4, 4, 0, 3, 4, 0 }, { 0, 4, 5, 3, 0, 4, 5, 3 }, { 5, 3, 0, 4, 5, 3, 0, 4 },
    { 0, 0, 3, 3, 4, 4, 0, 0 }, { 1, 4, 0, 0, 1, 4, 0, 5 }, { 0, 3, 0, 4, 0, 3, 4, 0 }, { 0, 2, 3, 4, 0, 2, 3, 4 },
    { 0, 6, 5, 4, 0, 6, 5, 4 }, { 0, 5, 1, 4, 0, 5, 1, 4 }, { 3, 4, 0, 5, 3, 4, 2, 5 }, { 0, 4, 3, 3, 0, 4, 1, 4 },
};
const int8_t ModalProgs[][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 1, 0, 1, 0, 1, 0, 6 }, { 0, 6, 0, 6, 0, 6, 5, 6 }, { 0, 0, 6, 6, 0, 0, 5, 5 },
    { 0, 3, 0, 3, 0, 3, 0, 4 }, { 0, 0, 1, 0, 0, 0, 6, 0 },
};

/** @brief Melody rhythms: one bar, 'x' a note, '-' held, '.' rest. */
const char *const Rhythms[] = {
    "x.x.x.x.x.x.x.x.", "x..x..x.x..x..x.", "x-x-x.x.x-x-x...", "x---x---x-x-x---", "x-------x---x---",
    "xxx.xxx.xxx.x.x.", "x.xxx.xxx.xxx.xx", "x..x..x...x.x...", "x.x...x.x...x.x.", "..x...x...x.x.x.",
    "x-----x-x-----x-", "x.x.x---x.x.x---", "x.......x.x.x...", "x-x.x-x.x.x-x...",
};

/**
 * @brief A bit for a rhythm template.
 * @param[in] i  the template
 * @return its bit
 */
constexpr uint16_t R( int i )
{
    return static_cast<uint16_t>( 1u << i );
}

constexpr uint16_t RPop = R( 0 ) | R( 3 ) | R( 8 ) | R( 9 ) | R( 11 );   ///< pop melodies
constexpr uint16_t RFast = R( 0 ) | R( 5 ) | R( 6 );                       ///< runs and reels
constexpr uint16_t RSlow = R( 3 ) | R( 4 ) | R( 10 ) | R( 12 );            ///< long notes
constexpr uint16_t RLatin = R( 1 ) | R( 7 ) | R( 8 ) | R( 13 );            ///< clave and habanera
constexpr uint16_t RSwing = R( 2 ) | R( 11 ) | R( 13 );                    ///< swung

/** @brief A drum kit's pattern: one character per sixteenth, '1'..'9' velocity, 'x' open hat. */
struct Kit
{
    const char *m_kick;    ///< kick
    const char *m_snare;   ///< snare or clap
    const char *m_hat;     ///< closed hat
    const char *m_open;    ///< open hat
};

const Kit Kits[static_cast<int>( DrumsE::COUNT )][2] = {
    { { "9...9...9...9...", "....9.......9...", "3.6.3.6.3.6.3.6.", "..x...x...x...x." },
      { "9...9...9...9..5", "....9.......9...", "5454545454545454", "..x...x...x...x." } },
    { { "9.......9.6.....", "....9.......9...", "5.4.5.4.5.4.5.4.", "................" },
      { "9.....6.9.......", "....9.......9...", "5.4.5.4.5.4.5.4.", "..............x." } },
    { { "9...9...9...9...", "....7.......7...", "4353435343534353", "..x...x...x...x." },
      { "9...9...9...9...", "................", "..5...5...5...5.", "..x...x...x...x." } },
    { { "9.7.......9..6..", "....9..3.3..9..4", "6.4.6.4.6.4.6.4.", "..............x." },
      { "9.....7...9.....", "....9.......9.4.", "6464646464646464", "......x.......x." } },
    { { "9.........9.....", "....9..2.2..9..3", "6363636363636363", "..............x." },
      { "9.....9...9.....", "....9.......9..4", "6.6.6.6.6.6.6.6.", "..............x." } },
    { { "9...9...9...9...", "....9.......9.7.", "5.5.5.5.5.5.5.5.", "................" },
      { "9.....9.9.......", "....9.......9...", "5555555555555555", "................" } },
    { { "9..6..9...9..6..", "....9..3....9.3.", "5454545454545454", "......x.......x." },
      { "9.6...9.7.....6.", "....9.......9...", "5.5.5.5.5.5.5.5.", "..x.......x....." } },
    { { "9...9...9...9...", "....7.......7...", "3373337333733373", "..x...x...x...x." },
      { "9...9...9...9...", "........7.......", "..7...7...7...7.", "..............x." } },
    { { "9...9...9...9...", "....8.......8...", "5555555555555555", "..x...x...x...x." },
      { "9...9...9...9...", "....8.......8...", "4.6.4.6.4.6.4.6.", "..x...x...x...x." } },
    { { "9.........6.....", "........9.......", "5.4.5.4.5.4.5.4.", "................" },
      { "9......6..9.....", "........9.......", "5.5.5.5.5.5.5.5.", "..............x." } },
    { { "9..7..9.9..7....", "....9.......9...", "5.5.5.5.5.5.5.5.", "......x.......x." },
      { "9..9..9.9..9..9.", "....9.......9...", "5454545454545454", "................" } },
    { { "9...9...9...9...", "......7.....7...", "5.5.5.5.5.5.5.5.", "..x...x...x...x." },
      { "9..6..9.9..6....", "....7.......7...", "5455545554555455", "..x.......x....." } },
    { { "9.......9.......", "....5.......5...", "5555555555555555", "................" },
      { "9..6....9..6....", "..5...5...5...5.", "5.5.5.5.5.5.5.5.", "................" } },
    { { "9..6..9.9..6..9.", "....8.......8...", "4.4.4.4.4.4.4.4.", "................" },
      { "9..7....9..7....", "....9.......9...", "..5...5...5...5.", "................" } },
    { { "9...9...9...9...", "..6...6...6...6.", "5.5.5.5.5.5.5.5.", "................" },
      { "9...9...9...9...", "...6..6....6..6.", "5454545454545454", "..x.......x....." } },
    { { "9.......9.......", "....9.......9...", "..5...5...5...5.", "................" },
      { "9...9...9...9...", "..7...7...7...7.", "................", "................" } },
    { { "9.99....9.99....", "....9.......9...", "5.5.5.5.5.5.5.5.", "................" },
      { "9.9.9...9.9.9...", "....9.......9...", "..5...5...5...5.", "................" } },
    { { "9.....9.9.......", "....9.......9...", "5.5.5.5.5.5.5.5.", "..............x." },
      { "9.9.....9.9.....", "....9.......9...", "5555555555555555", "................" } },
    { { "9.....6...9.....", "....8.......8...", "4.3.4.3.4.3.4.3.", "................" },
      { "9.......9.6.....", "....8.......8...", "4.4.4.4.4.4.4.4.", "..............x." } },
};

/** @brief Hand percussion: five lanes and their sounds. */
struct Perc
{
    const char *m_lanes[2][5];   ///< two variants: low drum, high drum, shaker, bell, click
    float       m_lowF;          ///< low drum pitch
    float       m_lowDecay;      ///< its decay
    float       m_lowBend;       ///< its pitch drop (negative: a rise, the tabla's bayan)
    float       m_highF;         ///< high drum pitch
    float       m_highDecay;     ///< its decay
    float       m_highClick;     ///< how much of it is a slap rather than a ring
    int         m_bell;          ///< 0 cowbell, 1 agogo, 2 gong, 3 triangle
    int         m_click;         ///< 0 claves, 1 rim, 2 castanets, 3 clapsticks, 4 tambourine
};

const Perc Percs[static_cast<int>( PercE::COUNT )] = {
    { { { "", "", "", "", "" }, { "", "", "", "", "" } }, 100, 10, 1, 400, 20, 0.3f, 0, 0 },
    { { { "9.......9...6.9.", "", "", "", "..6...6...6...6." }, { "9..9....9...9.9.", "", "", "", "....6.......6..." } }, 62, 5, 1.6f, 300, 20, 0.5f, 0, 0 },
    { { { "9..6....9..6.6..", "5.75.75.5.75.75.", "", "", "" }, { "9.....6.9..6....", "5575.57555755575", "", "", "" } }, 95, 7, -0.35f, 360, 9, 0.15f, 0, 0 },
    { { { "9.....9...9.....", "..7.7...7.7.7.77", "", "", "" }, { "9..9......9.....", "..77.7.7..7.7.7.", "", "", "" } }, 110, 10, 0.6f, 520, 28, 0.7f, 0, 0 },
    { { { "9......69.....6.", "..7.7.7...7.7.7.", "5353535353535353", "9..7..9...7.9...", "" },
        { "9..6....9..6..6.", "....7..7....7..7", "5555555555555555", "9.7.9.7.9.7.9.7.", "" } }, 85, 9, 0.8f, 420, 18, 0.6f, 0, 0 },
    { { { "6.......9.......", "9.99.9.99.9.9.9.", "7575757575757575", "9.9..9.9.9.9..9.", "" },
        { "6...6...9.......", "9.9.99.99.9.99.9", "7575757575757575", "9..9.9..9..9.9..", "" } }, 58, 4, 1.2f, 700, 30, 0.8f, 1, 0 },
    { { { "9.....9.9.......", "", "5.5.5.5.5.5.5.5.", "", "" }, { "9.......9.6.....", "", "5555555555555555", "", "" } }, 70, 6, 1.0f, 400, 20, 0.3f, 0, 0 },
    { { { "", "..5...5...5...5.", "5555555555555555", "9.......9.......", "9..9..9...9.9..." },
        { "", "..5.5...5.5.5...", "5.5.5.5.5.5.5.5.", "9...9...9...9...", "..9..9....9..9.." } }, 100, 10, 1, 450, 22, 0.4f, 0, 0 },
    { { { "9.5.7.5.9.5.7.5.", "", "", "", "" }, { "9.56.5.59.56.5.5", "", "", "", "" } }, 90, 14, 0.5f, 400, 20, 0.3f, 0, 0 },
    { { { "9.......9.......", "..7...7...7...7.", "", "", "....9.......9..." }, { "9...9...9...9...", "...7...7...7...7", "", "", "" } },
      80, 8, 0.8f, 600, 30, 0.8f, 0, 1 },
    { { { "9..6..9.....6...", "....7..7....7.7.", "", "9...............", "..5...5...5...5." },
        { "9.....6.9..6....", "..7.7.....7.7...", "", "9.......6.......", "5.5.5.5.5.5.5.5." } }, 105, 9, 0.4f, 480, 16, 0.4f, 2, 1 },
    { { { "", "", "", "", "9...9...9...9..." }, { "", "", "", "", "9.9.9.9.9.9.9.9." } }, 100, 10, 1, 400, 20, 0.3f, 0, 3 },
    { { { "9.77....9.77....", "", "5.5.5.5.5.5.5.5.", "", "" }, { "9.7.7...9.7.7...", "", "5555555555555555", "", "" } }, 85, 10, 0.6f, 400, 20, 0.3f, 0, 0 },
    { { { "", "", "5353535353535353", "", "....6.......6..." }, { "", "", "5.5.5.5.5.5.5.5.", "", "....6.......6..." } }, 100, 10, 1, 400, 20, 0.3f, 3, 4 },
};

/** @brief A section of an arrangement template. */
struct SectionDef
{
    int      m_bars;     ///< length
    uint16_t m_layers;   ///< parts
    int      m_melody;   ///< melody A or B
    bool     m_riser;    ///< sweeps up to the next
};

constexpr uint16_t L_ALL = L_DRUMS | L_BASS | L_CHORDS | L_LEAD | L_COUNTER | L_PERC | L_DRONE | L_ARP;   ///< everything
constexpr uint16_t L_GROOVE = L_DRUMS | L_BASS | L_CHORDS | L_PERC | L_DRONE | L_ARP;                    ///< all but the tunes

const SectionDef Arrangements[4][10] = {
    { { 4, L_CHORDS | L_DRONE | L_PERC, 0, false }, { 4, L_GROOVE, 0, false }, { 8, L_GROOVE | L_LEAD, 0, false },
      { 8, L_ALL, 1, false }, { 4, L_CHORDS | L_LEAD | L_DRONE, 0, false }, { 4, L_GROOVE & ~L_DRONE, 0, true } },
    { { 4, L_CHORDS | L_DRONE, 0, false }, { 4, L_CHORDS | L_DRONE | L_PERC | L_ARP, 0, false }, { 8, L_GROOVE, 0, false },
      { 8, L_GROOVE | L_LEAD, 0, false }, { 8, L_ALL, 1, false }, { 4, L_CHORDS | L_COUNTER | L_DRONE, 1, false },
      { 4, L_DRUMS | L_BASS | L_CHORDS | L_PERC, 0, true }, { 8, L_ALL, 0, false } },
    { { 8, L_CHORDS | L_DRONE | L_PERC, 0, false }, { 8, L_GROOVE, 0, false }, { 8, L_GROOVE | L_LEAD, 0, false },
      { 8, L_ALL, 0, false }, { 8, L_ALL, 1, false }, { 8, L_CHORDS | L_LEAD | L_DRONE, 1, false },
      { 4, L_DRUMS | L_BASS | L_CHORDS | L_PERC | L_ARP, 0, true }, { 8, L_ALL, 1, false }, { 4, L_GROOVE, 0, false } },
    { { 4, L_CHORDS | L_DRONE | L_LEAD, 0, false }, { 8, L_GROOVE | L_LEAD, 0, false }, { 8, L_GROOVE | L_LEAD | L_COUNTER, 1, false },
      { 8, L_ALL, 0, false }, { 4, L_CHORDS | L_LEAD | L_DRONE, 1, false } },
};
const int ArrangementSections[4] = { 6, 8, 9, 5 };

/** @brief A station's style. */
struct Style
{
    const char *m_name = "";                     ///< station name
    float       m_bpmMin = 120, m_bpmMax = 125;  ///< tempo range
    DrumsE      m_drums = DrumsE::FOUR;          ///< drum kit
    PercE       m_perc = PercE::NONE;            ///< hand percussion
    BassE       m_bass = BassE::OCTAVE;          ///< bass
    ChordE      m_chords = ChordE::SAW;          ///< chords
    DroneE      m_drone = DroneE::NONE;          ///< drone
    ArpE        m_arp = ArpE::NONE;              ///< arpeggio
    int         m_leads[3] = { SQUARE, -1, -1 }; ///< lead instruments to pick from
    int         m_counter = -1;                  ///< counter line instrument (-1: another of the leads)
    int         m_chordInst = EPIANO;            ///< instrument of strums, comps, bells, stabs
    ScaleE      m_scales[4] = { ScaleE::MINOR, ScaleE::COUNT, ScaleE::COUNT, ScaleE::COUNT };   ///< scales to pick from
    float       m_modal = 0.1f;                  ///< chance of a vamp instead of a progression
    float       m_sevenths = 0.0f;               ///< chance of seventh chords
    uint16_t    m_rhythms = RPop;                ///< melody rhythms to pick from
    int         m_arrangement = -1;              ///< arrangement template (-1: one of the first three)
    float       m_swing = 0.0f;                  ///< swing
    float       m_sidechain = 0.3f;              ///< pumping
    float       m_reverb = 0.3f;                 ///< reverb send
    float       m_echo = 0.25f;                  ///< echo send
    float       m_kick = 0.9f;                   ///< kick level
    float       m_snare = 0.6f;                  ///< snare level
    float       m_hat = 0.11f;                   ///< hat level
    float       m_percLevel = 0.3f;              ///< hand percussion level
    float       m_bassLevel = 0.38f;             ///< bass level
    float       m_leadLevel = 0.08f;             ///< lead level
    float       m_chordLevel = 0.07f;            ///< chord level
    float       m_droneLevel = 0.06f;            ///< drone level
    float       m_arpLevel = 0.07f;              ///< arpeggio level
    float       m_trim = 1.0f;                   ///< overall level
};

/**
 * @brief Starts a style.
 * @param[in] name  station name
 * @param[in] lo    slowest tempo
 * @param[in] hi    fastest tempo
 * @param[in] d     drums
 * @param[in] p     hand percussion
 * @param[in] b     bass
 * @param[in] c     chords
 * @param[in] leads lead instruments
 * @param[in] sc    scales
 * @return the style (levels at their defaults)
 */
Style style( const char *name, float lo, float hi, DrumsE d, PercE p, BassE b, ChordE c, std::initializer_list<int> leads,
             std::initializer_list<ScaleE> sc )
{
    Style s;
    s.m_name = name;
    s.m_bpmMin = lo;
    s.m_bpmMax = hi;
    s.m_drums = d;
    s.m_perc = p;
    s.m_bass = b;
    s.m_chords = c;
    int i = 0;
    for( int l : leads ) s.m_leads[i++] = l;
    i = 0;
    for( ScaleE e : sc ) s.m_scales[i++] = e;
    return s;
}

/**
 * @brief Builds the stations.
 * @return all of them
 */
std::array<Style, StationCount> buildStyles()
{
    using S = ScaleE;
    std::array<Style, StationCount> st;
    Style *s = nullptr;

    s = &( st[0] = style( "SUNSET FM", 100, 112, DrumsE::SYNTHWAVE, PercE::NONE, BassE::OCTAVE, ChordE::SAW, { SUPERSAW, BRASS, SQUARE },
                          { S::MINOR, S::DORIAN, S::MAJOR } ) );
    s->m_arp = ArpE::UP, s->m_rhythms = RPop | RSlow, s->m_sidechain = 0.35f, s->m_reverb = 0.45f, s->m_echo = 0.35f;
    s->m_snare = 0.65f, s->m_hat = 0.1f, s->m_leadLevel = 0.07f, s->m_chordLevel = 0.05f, s->m_arpLevel = 0.1f;

    s = &( st[1] = style( "TURBO HOUSE", 122, 128, DrumsE::FOUR, PercE::SHAKER, BassE::OFFBEAT, ChordE::STABS, { SQUARE, EPIANO, ORGAN },
                          { S::MINOR, S::DORIAN, S::MIXOLYDIAN } ) );
    s->m_modal = 0.2f, s->m_rhythms = RPop | RLatin, s->m_sidechain = 0.5f, s->m_reverb = 0.25f, s->m_kick = 0.95f, s->m_snare = 0.55f;
    s->m_hat = 0.14f, s->m_bassLevel = 0.42f, s->m_chordLevel = 0.09f, s->m_percLevel = 0.15f;

    s = &( st[2] = style( "AMIGA TECHNO", 134, 142, DrumsE::TECHNO, PercE::NONE, BassE::ACID, ChordE::STRINGS, { PULSE, SQUARE },
                          { S::MINOR, S::PHRYGIAN, S::HARMONIC } ) );
    s->m_arp = ArpE::CHIP, s->m_modal = 0.4f, s->m_rhythms = RFast | RPop, s->m_reverb = 0.15f, s->m_kick = 0.95f, s->m_snare = 0.5f;
    s->m_hat = 0.12f, s->m_bassLevel = 0.36f, s->m_chordLevel = 0.035f;

    s = &( st[3] = style( "TRANCE GATE", 136, 140, DrumsE::TRANCE, PercE::NONE, BassE::ROLLING, ChordE::GATED, { SUPERSAW, PLUCK },
                          { S::MINOR, S::HARMONIC, S::DORIAN } ) );
    s->m_arp = ArpE::UP, s->m_rhythms = RPop | RFast, s->m_sidechain = 0.55f, s->m_reverb = 0.45f, s->m_echo = 0.4f, s->m_kick = 0.95f;
    s->m_snare = 0.5f, s->m_hat = 0.12f, s->m_bassLevel = 0.36f, s->m_leadLevel = 0.07f, s->m_chordLevel = 0.05f, s->m_arpLevel = 0.09f;

    s = &( st[4] = style( "JUNGLE PRESSURE", 168, 176, DrumsE::DNB, PercE::NONE, BassE::REESE, ChordE::STRINGS, { EPIANO, VIBES, FLUTE },
                          { S::MINOR, S::DORIAN } ) );
    s->m_modal = 0.3f, s->m_rhythms = RSlow | RPop, s->m_sidechain = 0.2f, s->m_reverb = 0.35f, s->m_echo = 0.3f, s->m_hat = 0.11f;
    s->m_bassLevel = 0.4f, s->m_leadLevel = 0.1f, s->m_chordLevel = 0.05f;

    s = &( st[5] = style( "8-BIT RACER", 148, 160, DrumsE::CHIP, PercE::NONE, BassE::CHIP, ChordE::NONE, { PULSE },
                          { S::MAJOR, S::MINOR, S::PENTA_MAJOR } ) );
    s->m_arp = ArpE::CHIP, s->m_rhythms = RFast | RPop, s->m_sidechain = 0.0f, s->m_reverb = 0.05f, s->m_echo = 0.15f, s->m_kick = 0.7f;
    s->m_snare = 0.4f, s->m_hat = 0.09f, s->m_bassLevel = 0.26f, s->m_leadLevel = 0.07f, s->m_arpLevel = 0.06f;

    s = &( st[6] = style( "FUSION DRIVE", 110, 118, DrumsE::FUNK, PercE::NONE, BassE::FUNKY, ChordE::STABS, { BRASS, EPIANO, ORGAN },
                          { S::DORIAN, S::MIXOLYDIAN, S::MAJOR } ) );
    s->m_counter = TRUMPET, s->m_sevenths = 0.7f, s->m_modal = 0.2f, s->m_rhythms = RPop | RLatin | RSwing, s->m_swing = 0.1f;
    s->m_sidechain = 0.1f, s->m_reverb = 0.25f, s->m_echo = 0.2f, s->m_kick = 0.85f, s->m_bassLevel = 0.4f, s->m_chordLevel = 0.08f;

    s = &( st[7] = style( "BIG BEAT", 124, 132, DrumsE::BREAKS, PercE::NONE, BassE::BIGBEAT, ChordE::NONE, { BRASS, DIST_GUITAR, ORGAN },
                          { S::MINOR, S::BLUES } ) );
    s->m_modal = 0.5f, s->m_swing = 0.06f, s->m_sidechain = 0.2f, s->m_reverb = 0.2f, s->m_kick = 1.0f, s->m_snare = 0.75f, s->m_hat = 0.12f;
    s->m_bassLevel = 0.4f;

    s = &( st[8] = style( "CITY POP FM", 108, 118, DrumsE::DISCO, PercE::NONE, BassE::FUNKY, ChordE::COMP, { EPIANO, SQUARE, TRUMPET },
                          { S::MAJOR, S::LYDIAN, S::DORIAN } ) );
    s->m_counter = VIBES, s->m_chordInst = EPIANO, s->m_sevenths = 1.0f, s->m_modal = 0.05f, s->m_rhythms = RPop | RSwing;
    s->m_sidechain = 0.15f, s->m_reverb = 0.3f, s->m_snare = 0.55f, s->m_hat = 0.1f, s->m_chordLevel = 0.06f;

    s = &( st[9] = style( "KOTO WAVE", 88, 104, DrumsE::HALFTIME, PercE::TAIKO, BassE::SUB, ChordE::STRINGS, { KOTO, NEY, FLUTE },
                          { S::HIRAJOSHI, S::IN_SEN, S::PENTA_MINOR } ) );
    s->m_counter = KOTO, s->m_modal = 0.5f, s->m_rhythms = RSlow | RPop, s->m_reverb = 0.45f, s->m_echo = 0.35f, s->m_percLevel = 0.4f;
    s->m_chordLevel = 0.045f;

    s = &( st[10] = style( "BOLLYWOOD EXPRESS", 100, 124, DrumsE::BHANGRA, PercE::TABLA, BassE::SUB, ChordE::STRINGS, { SITAR, FLUTE, SQUARE },
                           { S::BHAIRAV, S::HARMONIC, S::MIXOLYDIAN } ) );
    s->m_drone = DroneE::TANPURA, s->m_counter = SITAR, s->m_modal = 0.6f, s->m_rhythms = RSlow | RPop | RFast, s->m_reverb = 0.35f;
    s->m_echo = 0.3f, s->m_percLevel = 0.45f, s->m_chordLevel = 0.045f, s->m_droneLevel = 0.05f;

    s = &( st[11] = style( "DESERT CARAVAN", 96, 118, DrumsE::HALFTIME, PercE::DARBUKA, BassE::SUB, ChordE::STRINGS, { OUD, NEY, CLARINET },
                           { S::HIJAZ, S::PHRYGIAN, S::HARMONIC } ) );
    s->m_drone = DroneE::HUM, s->m_counter = OUD, s->m_modal = 0.7f, s->m_rhythms = RSlow | RPop | RFast, s->m_reverb = 0.4f;
    s->m_percLevel = 0.45f, s->m_chordLevel = 0.04f;

    s = &( st[12] = style( "AFRO HIGHWAY", 112, 124, DrumsE::AFRO, PercE::AFRO, BassE::FUNKY, ChordE::PLUCKS, { KALIMBA, MARIMBA, TRUMPET },
                           { S::MAJOR, S::MIXOLYDIAN, S::PENTA_MAJOR, S::DORIAN } ) );
    s->m_chordInst = GUITAR, s->m_counter = MARIMBA, s->m_modal = 0.3f, s->m_rhythms = RLatin | RPop, s->m_sidechain = 0.2f;
    s->m_percLevel = 0.4f, s->m_chordLevel = 0.06f;

    s = &( st[13] = style( "SAMBA AVENIDA", 96, 108, DrumsE::SAMBA, PercE::SAMBA, BassE::SAMBA, ChordE::STRUM, { GUITAR, FLUTE, TRUMPET },
                           { S::MAJOR, S::DORIAN, S::LYDIAN, S::MINOR } ) );
    s->m_chordInst = GUITAR, s->m_sevenths = 0.8f, s->m_rhythms = RLatin | RSwing, s->m_sidechain = 0.0f, s->m_kick = 0.7f;
    s->m_snare = 0.4f, s->m_percLevel = 0.45f, s->m_chordLevel = 0.06f;

    s = &( st[14] = style( "TANGO TURBO", 112, 122, DrumsE::HABANERA, PercE::NONE, BassE::HABANERA, ChordE::BRASS, { BANDONEON, FIDDLE, ACCORDION },
                           { S::HARMONIC, S::MINOR } ) );
    s->m_chordInst = FIDDLE, s->m_counter = FIDDLE, s->m_rhythms = RLatin | RSlow, s->m_sidechain = 0.15f, s->m_reverb = 0.35f;
    s->m_chordLevel = 0.06f;

    s = &( st[15] = style( "ANDES WIND", 92, 108, DrumsE::HALFTIME, PercE::ANDES, BassE::OCTAVE, ChordE::STRUM, { PANFLUTE, FLUTE, CHARANGO },
                           { S::PENTA_MINOR, S::MINOR, S::HARMONIC } ) );
    s->m_chordInst = CHARANGO, s->m_modal = 0.3f, s->m_rhythms = RFast | RPop, s->m_reverb = 0.4f, s->m_percLevel = 0.45f;
    s->m_chordLevel = 0.05f, s->m_kick = 0.75f;

    s = &( st[16] = style( "CARIBBEAN STEEL", 100, 120, DrumsE::SOCA, PercE::CARIB, BassE::FUNKY, ChordE::SKANK, { STEEL_DRUM, TRUMPET, MARIMBA },
                           { S::MAJOR, S::MIXOLYDIAN, S::PENTA_MAJOR } ) );
    s->m_counter = STEEL_DRUM, s->m_modal = 0.2f, s->m_rhythms = RLatin | RPop, s->m_percLevel = 0.4f, s->m_chordLevel = 0.06f;

    s = &( st[17] = style( "MARIACHI MOTOR", 112, 132, DrumsE::POLKA, PercE::NONE, BassE::POLKA, ChordE::STRUM, { TRUMPET, ACCORDION, FIDDLE },
                           { S::MAJOR, S::HARMONIC, S::MIXOLYDIAN } ) );
    s->m_chordInst = GUITAR, s->m_counter = TRUMPET, s->m_rhythms = RPop | RFast, s->m_sidechain = 0.0f, s->m_reverb = 0.3f;
    s->m_kick = 0.75f, s->m_snare = 0.45f, s->m_chordLevel = 0.06f;

    s = &( st[18] = style( "CELTIC COAST", 112, 128, DrumsE::FOUR, PercE::CELTIC, BassE::ROLLING, ChordE::STRUM, { WHISTLE, FIDDLE, FLUTE },
                           { S::DORIAN, S::MIXOLYDIAN, S::MAJOR, S::PENTA_MAJOR } ) );
    s->m_chordInst = TWANG, s->m_drone = DroneE::BAGPIPE, s->m_modal = 0.3f, s->m_rhythms = RFast, s->m_swing = 0.08f;
    s->m_sidechain = 0.25f, s->m_percLevel = 0.4f, s->m_chordLevel = 0.05f, s->m_droneLevel = 0.035f;

    s = &( st[19] = style( "BALKAN BRASS", 130, 146, DrumsE::POLKA, PercE::BALKAN, BassE::POLKA, ChordE::BRASS, { TRUMPET, CLARINET, ACCORDION },
                           { S::GYPSY, S::HARMONIC, S::HIJAZ, S::MINOR } ) );
    s->m_chordInst = BRASS, s->m_counter = CLARINET, s->m_modal = 0.2f, s->m_rhythms = RFast, s->m_sidechain = 0.0f, s->m_kick = 0.8f;
    s->m_percLevel = 0.4f, s->m_chordLevel = 0.06f;

    s = &( st[20] = style( "GAMELAN DRIVE", 100, 120, DrumsE::FOUR, PercE::GAMELAN, BassE::SUB, ChordE::BELLS, { GAMELAN, VIBES, FLUTE },
                           { S::PELOG, S::PENTA_MAJOR } ) );
    s->m_chordInst = GAMELAN, s->m_modal = 0.6f, s->m_rhythms = RPop | RSlow, s->m_reverb = 0.4f, s->m_percLevel = 0.4f;
    s->m_chordLevel = 0.05f;

    s = &( st[21] = style( "OUTBACK DRONE", 110, 124, DrumsE::BREAKS, PercE::CLAPSTICKS, BassE::SUB, ChordE::SAW, { PLUCK, TWANG, FLUTE },
                           { S::MINOR, S::DORIAN, S::PENTA_MINOR } ) );
    s->m_drone = DroneE::DIDGERIDOO, s->m_modal = 0.6f, s->m_rhythms = RPop | RSlow, s->m_percLevel = 0.35f, s->m_chordLevel = 0.04f;
    s->m_droneLevel = 0.09f;

    s = &( st[22] = style( "STEPPE RIDER", 104, 124, DrumsE::GALLOP, PercE::STEPPE, BassE::GALLOP, ChordE::STRINGS, { ERHU, FLUTE, OUD },
                           { S::PENTA_MINOR, S::PENTA_MAJOR, S::DORIAN } ) );
    s->m_drone = DroneE::THROAT, s->m_modal = 0.6f, s->m_rhythms = RFast | RSlow, s->m_reverb = 0.4f, s->m_percLevel = 0.4f;
    s->m_chordLevel = 0.04f, s->m_droneLevel = 0.06f;

    s = &( st[23] = style( "NORDIC NIGHT", 122, 136, DrumsE::TRANCE, PercE::CELTIC, BassE::ROLLING, ChordE::GATED, { FIDDLE, FLUTE, VIBES },
                           { S::MINOR, S::DORIAN } ) );
    s->m_drone = DroneE::HUM, s->m_arp = ArpE::UP, s->m_modal = 0.2f, s->m_rhythms = RPop | RFast, s->m_sidechain = 0.45f;
    s->m_reverb = 0.5f, s->m_echo = 0.35f, s->m_percLevel = 0.3f, s->m_chordLevel = 0.05f;

    s = &( st[24] = style( "ROUTE 66 RADIO", 112, 140, DrumsE::ROCK, PercE::SHAKER, BassE::COUNTRY, ChordE::STRUM, { TWANG, HARMONICA, DIST_GUITAR },
                           { S::MAJOR, S::MIXOLYDIAN, S::BLUES, S::PENTA_MAJOR } ) );
    s->m_chordInst = TWANG, s->m_modal = 0.2f, s->m_rhythms = RPop | RSwing, s->m_sidechain = 0.0f, s->m_kick = 0.85f, s->m_snare = 0.7f;
    s->m_percLevel = 0.25f, s->m_chordLevel = 0.055f;

    s = &( st[25] = style( "LO-FI CRUISE", 72, 88, DrumsE::LOFI, PercE::SHAKER, BassE::SUB, ChordE::COMP, { EPIANO, VIBES, GUITAR },
                           { S::DORIAN, S::MINOR, S::MAJOR } ) );
    s->m_chordInst = EPIANO, s->m_sevenths = 1.0f, s->m_rhythms = RSlow | RSwing, s->m_arrangement = 3, s->m_swing = 0.14f;
    s->m_sidechain = 0.15f, s->m_reverb = 0.35f, s->m_echo = 0.3f, s->m_kick = 0.8f, s->m_snare = 0.5f, s->m_hat = 0.08f;
    s->m_percLevel = 0.15f, s->m_chordLevel = 0.06f;
    return st;
}

/**
 * @brief The stations.
 * @return their styles
 */
const std::array<Style, StationCount> &styles()
{
    static const auto s = buildStyles();
    return s;
}

/**
 * @brief A pattern character as a velocity.
 * @param[in] p  the pattern (may be empty)
 * @param[in] i  the step
 * @return 0 (rest) .. 1
 */
float vel( const char *p, int i )
{
    if( !p || !p[0] ) return 0.0f;
    const char c = p[i & 15];
    return c >= '1' && c <= '9' ? static_cast<float>( c - '0' ) / 9.0f : ( c == 'x' ? 1.0f : 0.0f );
}

/**
 * @brief A smoothing coefficient for a time constant.
 * @param[in] ms  milliseconds
 * @return per-sample coefficient
 */
float coef( float ms )
{
    return 1.0f - std::exp( -1000.0f / ( std::max( 0.1f, ms ) * SampleRate ) );
}

/**
 * @brief Writes a pluck into a string's delay line.
 * @param[out] v       the voice
 * @param[in]  amp     loudness
 * @param[in]  bright  0..1 how much of the noise's top end stays
 */
void excite( Tone &v, float amp, float bright )
{
    const int period = std::min( StringLength - 2, static_cast<int>( SampleRate / std::max( 20.0f, v.m_f ) ) + 2 );
    float     l = 0.0f;
    for( int k = 1; k <= period; ++k )
    {
        l += bright * ( v.m_noise.next() - l );
        const size_t i = ( v.m_write + StringLength - static_cast<size_t>( k ) ) % StringLength;
        v.m_string[i] = v.m_string[i] * 0.3f + l * amp;
    }
}

/**
 * @brief One sample of a plucked string (Karplus-Strong with a fractional delay).
 * @param[in,out] v      the voice
 * @param[in]     decay  loss per period
 * @return its sample
 */
float pluckString( Tone &v, float decay )
{
    const float period = std::clamp( SampleRate / std::max( 20.0f, v.m_f ) - 0.5f, 2.0f, static_cast<float>( StringLength - 2 ) );
    float       rp = static_cast<float>( v.m_write ) - period;
    if( rp < 0.0f ) rp += static_cast<float>( StringLength );
    const size_t i0 = static_cast<size_t>( rp ) % StringLength;
    const size_t i1 = ( i0 + 1 ) % StringLength;
    const float  fr = rp - std::floor( rp );
    const float  y = v.m_string[i0] * ( 1.0f - fr ) + v.m_string[i1] * fr;
    v.m_string[v.m_write] = 0.5f * ( y + v.m_last ) * decay;
    v.m_last = y;
    v.m_write = ( v.m_write + 1 ) % StringLength;
    return y;
}

/**
 * @brief Advances a phase and returns its sine.
 * @param[in,out] ph  the phase (0..1)
 * @param[in]     f   frequency
 * @return the sine
 */
float osc( float &ph, float f )
{
    const float y = std::sin( Pi2 * ph );
    ph += f / SampleRate;
    ph -= std::floor( ph );
    return y;
}

} // namespace

const char *stationName( int station )
{
    return styles()[static_cast<size_t>( std::clamp( station, 0, StationCount - 1 ) )].m_name;
}

std::string trackTitle( uint64_t seed )
{
    static const char *adjectives[40] = { "MIDNIGHT", "CHROME",   "NEON",     "VELVET",  "SOLAR",   "ELECTRIC", "CRYSTAL", "GOLDEN",
                                          "RADIANT",  "PHANTOM",  "SILVER",   "CRIMSON", "TURBO",   "DIGITAL",  "COSMIC",  "SAPPHIRE",
                                          "FADING",   "INFINITE", "LUNAR",    "BURNING", "ENDLESS", "HYPER",    "AMBER",   "ULTRA",
                                          "DUSTY",    "SALTY",    "MONSOON",  "COPPER",  "JADE",    "SCARLET",  "DISTANT", "WILD",
                                          "SPICED",   "HIGHLAND", "TROPICAL", "SILENT",  "NORTHERN", "SOUTHERN", "DESERT", "OCEAN" };
    static const char *nouns[40] = { "HIGHWAY",   "HORIZON",  "MIRAGE",   "PULSE",   "DRIFT",    "RUNNER",   "SKYLINE",  "OVERDRIVE",
                                     "AFTERGLOW", "VELOCITY", "SUNRISE",  "STREAK",  "CIRCUIT",  "PARADISE", "FRONTIER", "HEARTBEAT",
                                     "LIGHTS",    "ECHOES",   "MOTION",   "SIGNAL",  "CORONA",   "COASTLINE", "APEX",    "TWILIGHT",
                                     "CARAVAN",   "MONSOON",  "BAZAAR",   "LAGOON",  "PASSAGE",  "CROSSING", "BORDER",   "SERPENTINE",
                                     "HARBOUR",   "PLATEAU",  "CANYON",   "LANTERN", "MERIDIAN", "TRADEWIND", "AVENUE",  "JOURNEY" };
    const uint64_t h = hash64( seed * 0x9E37ull + 3 );
    return std::string( adjectives[h % 40] ) + " " + nouns[( h >> 8 ) % 40];
}

MusicSynth::MusicSynth()
{
    m_delayL.assign( static_cast<size_t>( SampleRate ), 0.0f );
    m_delayR.assign( static_cast<size_t>( SampleRate ), 0.0f );
    static const int combs[4] = { 1687, 1601, 2053, 2251 };
    static const int aps[2] = { 556, 441 };
    for( int i = 0; i < 8; ++i ) m_comb[i].assign( static_cast<size_t>( combs[i % 4] + ( i >= 4 ? 23 : 0 ) ), 0.0f );
    for( int i = 0; i < 4; ++i ) m_ap[i].assign( static_cast<size_t>( aps[i % 2] + ( i >= 2 ? 23 : 0 ) ), 0.0f );
    for( Tone *v : { &m_lead, &m_counterVoice, &m_arp, &m_chordVoice[0], &m_chordVoice[1], &m_chordVoice[2], &m_chordVoice[3], &m_tanpura } )
        v->m_string.assign( StringLength, 0.0f );
    for( float &d : m_strumDelay ) d = -1.0f;
}

void MusicSynth::request( uint64_t seed, int station )
{
    if( !m_on || m_gain < 0.01f )
    {
        start( seed, station );
        return;
    }
    m_pending = true;
    m_pendingSeed = seed;
    m_pendingStation = station;
}

int MusicSynth::semitones( int degree ) const
{
    const ScaleDef &sc = Scales[m_scale];
    const int       oct = degree >= 0 ? degree / sc.m_n : -( ( -degree + sc.m_n - 1 ) / sc.m_n );
    return sc.m_steps[degree - oct * sc.m_n] + 12 * oct;
}

void MusicSynth::compose( Rng &r, int8_t *out, int rhythm, int low, int high )
{
    const char *bar = Rhythms[rhythm];
    const int   n = Scales[m_scale].m_n;
    int         deg = r.rangeInt( low + 1, std::max( low + 1, ( low + high ) / 2 + 1 ) );
    const int   shift = r.rangeInt( -1, 2 );
    const bool  echoBar = r.chance( 0.5f );
    for( int b = 0; b < 4; ++b )
        for( int s = 0; s < 16; ++s )
        {
            const int i = b * 16 + s;
            char      c = bar[s];
            if( b == 3 && s >= 8 ) c = s == 8 ? 'x' : ( s < 14 ? '-' : '.' );   // a long last note
            if( c == '.' )
            {
                out[i] = -1;
                continue;
            }
            if( c == '-' )
            {
                out[i] = -2;
                continue;
            }
            if( b == 2 && s < 8 )
            { // The third bar answers the first, moved.
                out[i] = out[i - 32] >= 0 ? static_cast<int8_t>( std::clamp( out[i - 32] + shift, low, high ) ) : out[i - 32];
                continue;
            }
            if( b == 1 && s < 8 && echoBar )
            {
                out[i] = out[i - 16];
                continue;
            }
            int step = r.rangeInt( -2, 2 );
            if( r.chance( 0.15f ) ) step = r.chance( 0.5f ) ? 3 : -3;
            if( step == 0 && r.chance( 0.6f ) ) step = r.chance( 0.5f ) ? 1 : -1;
            deg = std::clamp( deg + step, low, high );
            if( b == 3 && s == 8 ) deg = std::clamp( r.chance( 0.5f ) ? 0 : n, low, high );
            out[i] = static_cast<int8_t>( deg );
        }
}

void MusicSynth::start( uint64_t seed, int station )
{
    m_station = std::clamp( station, 0, StationCount - 1 );
    const Style &st = styles()[static_cast<size_t>( m_station )];
    Rng          r( seed * 2654435761ull + 17 );
    m_bpm = std::round( st.m_bpmMin + ( st.m_bpmMax - st.m_bpmMin ) * r.uniform() );
    m_swing = st.m_swing;
    m_key = r.rangeInt( -5, 6 );
    int scales = 0;
    while( scales < 4 && st.m_scales[scales] != ScaleE::COUNT ) ++scales;
    m_scale = static_cast<int>( st.m_scales[r.rangeInt( 0, std::max( 0, scales - 1 ) )] );
    // The progression: a vamp, or a progression of four or eight chords, a bar or two each.
    const bool    modal = r.chance( st.m_modal );
    const int8_t *prog = modal ? ModalProgs[r.rangeInt( 0, 5 )] : PopProgs[r.rangeInt( 0, 11 )];
    m_progLen = r.chance( 0.5f ) ? 8 : 4;
    for( int i = 0; i < 8; ++i ) m_prog[i] = prog[i];
    m_barsPerChord = m_bpm < 100.0f || r.chance( 0.3f ) ? 1 : ( r.chance( 0.5f ) ? 2 : 1 );
    m_sevenths = r.chance( st.m_sevenths );
    // Instruments: one of the leads, the counter line another (or the style's).
    int leads = 0;
    while( leads < 3 && st.m_leads[leads] >= 0 ) ++leads;
    const int li = r.rangeInt( 0, leads - 1 );
    m_leadInst = st.m_leads[li];
    m_counterInst = st.m_counter >= 0 ? st.m_counter : ( leads > 1 ? st.m_leads[( li + 1 ) % leads] : -1 );
    m_drumKit = static_cast<int>( st.m_drums );
    m_drumVariant = r.rangeInt( 0, 1 );
    m_percVariant = r.rangeInt( 0, 1 );
    m_variant = r.rangeInt( 0, 2 );
    // Two melodies with their own rhythms, a counter line.
    int rhythms[16], nr = 0;
    for( int i = 0; i < 14; ++i )
        if( st.m_rhythms & R( i ) ) rhythms[nr++] = i;
    const int n = Scales[m_scale].m_n;
    const int ra = rhythms[r.rangeInt( 0, nr - 1 )];
    const int rb = rhythms[r.rangeInt( 0, nr - 1 )];
    compose( r, m_melody[0], ra, 0, n + 3 );
    compose( r, m_melody[1], rb, 2, n + 5 );
    const int counterKind = r.rangeInt( 0, 2 );
    if( counterKind == 0 )
        for( int i = 0; i < 64; ++i ) m_counter[i] = m_melody[0][i] >= 0 ? static_cast<int8_t>( m_melody[0][i] - 2 ) : m_melody[0][i];
    else
    {
        compose( r, m_counter, counterKind == 1 ? 12 : 4, -2, n );
        if( counterKind == 1 ) // Answers in the lead's rests.
            for( int i = 0; i < 64; ++i )
                if( m_melody[0][i] >= 0 || m_melody[0][i] == -2 ) m_counter[i] = -1;
    }
    for( int i = 0; i < 16; ++i )
    {
        static const int acidNotes[8] = { 0, 0, 12, 0, 3, 7, 10, 15 };
        m_acid[i] = r.chance( 0.75f ) ? acidNotes[r.rangeInt( 0, 7 )] : -1;
        m_accent[i] = r.chance( 0.3f );
        m_slide[i] = r.chance( 0.2f );
    }
    // The arrangement.
    const int a = st.m_arrangement >= 0 ? st.m_arrangement : r.rangeInt( 0, 2 );
    m_sectionCount = ArrangementSections[a];
    m_totalBars = 0;
    for( int i = 0; i < m_sectionCount; ++i )
    {
        const SectionDef &d = Arrangements[a][i];
        m_sections[static_cast<size_t>( i )] = Section{ d.m_bars, d.m_layers, d.m_melody, d.m_riser };
        m_totalBars += d.m_bars;
    }
    m_step = -1;
    m_clock = 1e9;
    m_on = true;
    m_pending = false;
    m_snareF.set( st.m_drums == DrumsE::CHIP ? 4000.0f : st.m_drums == DrumsE::LOFI ? 1300.0f : 1900.0f, 0.7f );
    m_hatF.set( st.m_drums == DrumsE::LOFI ? 5500.0f : 7500.0f, 0.7f );
    m_kickClick.set( 3000.0f, 0.7f );
    m_crashF.set( 6000.0f, 0.6f );
    m_padF1.set( st.m_chords == ChordE::GATED ? 2400.0f : st.m_chords == ChordE::STRINGS ? 850.0f : 1100.0f, 0.8f );
    const Perc &pc = Percs[static_cast<int>( st.m_perc )];
    m_pF[1].set( pc.m_highF * 2.2f, 1.2f );
    m_pF[2].set( 7000.0f, 0.8f );
    m_pF[3].set( 2200.0f, 2.0f );
    m_pF[4].set( pc.m_click == 2 ? 3000.0f : 1500.0f, 1.5f );
    m_pF[0].set( 600.0f, 0.8f );
    m_lead.m_gate = m_counterVoice.m_gate = false;
}

void MusicSynth::noteOn( Tone &v, int inst, float f, float vel, bool slide )
{
    const bool sounding = v.m_gate && v.m_inst == inst && v.m_t < 8.0f;
    v.m_inst = inst;
    v.m_vel = vel;
    v.m_gate = true;
    if( slide && sounding && Insts[inst].m_slides )
    {
        v.m_target = f;
        if( inst == SITAR ) v.m_t = std::min( v.m_t, 0.5f );
        return;
    }
    v.m_target = v.m_f = f;
    v.m_t = 0.0f;
    switch( inst )
    {
    case GUITAR: excite( v, 0.9f, 0.45f ); break;
    case TWANG: excite( v, 0.8f, 0.85f ); break;
    case SITAR: excite( v, 0.9f, 0.8f ); break;
    case KOTO: excite( v, 0.85f, 0.9f ); break;
    case OUD: excite( v, 1.0f, 0.3f ); break;
    case CHARANGO: excite( v, 0.8f, 0.85f ); break;
    case SQUARE: v.m_f1.set( 3200.0f, 0.8f ); break;
    case SUPERSAW: v.m_f1.set( 4800.0f, 0.8f ); break;
    case ACCORDION: v.m_f1.set( 2800.0f, 0.7f ); break;
    case BANDONEON: v.m_f1.set( 1600.0f, 0.8f ); break;
    case HARMONICA:
        v.m_f1.set( 1800.0f, 0.8f );
        v.m_f2.set( 1200.0f, 1.5f );
        break;
    case FLUTE:
    case WHISTLE: v.m_f1.set( f * 2.0f, 3.0f ); break;
    case PANFLUTE: v.m_f1.set( f * 2.0f, 2.0f ); break;
    case NEY: v.m_f1.set( f * 1.5f, 1.5f ); break;
    case FIDDLE:
        v.m_f1.set( 3800.0f, 0.7f );
        v.m_f2.set( 1000.0f, 1.2f );
        break;
    case ERHU:
        v.m_f1.set( 2400.0f, 0.7f );
        v.m_f2.set( 1300.0f, 1.6f );
        break;
    case DIST_GUITAR: v.m_f1.set( 3000.0f, 0.8f ); break;
    case CLARINET: v.m_f1.set( 2200.0f, 0.7f ); break;
    default:
        for( float &p : v.m_ph ) p = 0.0f;
        break;
    }
}

float MusicSynth::render( Tone &v )
{
    const InstInfo &in = Insts[v.m_inst];
    if( v.m_t > 10.0f && v.m_env < 1e-4f ) return 0.0f;
    const float dt = 1.0f / SampleRate;
    v.m_f += ( v.m_target - v.m_f ) * 0.0015f;
    const float t = v.m_t;
    v.m_t += dt;
    if( in.m_sustained )
    {
        static const auto rates = []
        {
            std::array<float, INST_COUNT * 2> a{};
            for( int i = 0; i < INST_COUNT; ++i )
            {
                a[static_cast<size_t>( i * 2 )] = coef( Insts[i].m_attackMs );
                a[static_cast<size_t>( i * 2 + 1 )] = coef( Insts[i].m_releaseMs );
            }
            return a;
        }();
        const float target = v.m_gate ? 1.0f : 0.0f;
        v.m_env += ( target - v.m_env ) * rates[static_cast<size_t>( v.m_inst * 2 + ( target > v.m_env ? 0 : 1 ) )];
        if( !v.m_gate && v.m_env < 1e-4f ) return 0.0f;
    }
    const float f = v.m_f;
    const float vib = t > 0.25f ? 1.0f + 0.005f * std::sin( Pi2 * 5.2f * t ) : 1.0f;
    float       y = 0.0f;
    switch( v.m_inst )
    {
    case SQUARE: y = v.m_f1.low( pulse( v.m_ph[0], f * vib ) ); break;
    case SUPERSAW:
        y = v.m_f1.low( ( saw( v.m_ph[0], f * 0.992f * vib ) + saw( v.m_ph[1], f * vib ) + saw( v.m_ph[2], f * 1.009f * vib ) ) * 0.45f );
        break;
    case PULSE: y = pulse( v.m_ph[0], f * ( t > 0.1f ? 1.0f + 0.006f * std::sin( Pi2 * 6.0f * t ) : 1.0f ), 0.25f ) * 0.7f; break;
    case EPIANO:
    {
        const float index = 2.2f * std::exp( -t * 5.0f ) + 0.3f;
        const float mod = osc( v.m_ph[1], f ) * index;
        y = std::sin( Pi2 * v.m_ph[0] + mod ) * std::exp( -t * 2.2f ) + std::sin( Pi2 * v.m_ph[0] * 4.0f ) * std::exp( -t * 12.0f ) * 0.15f;
        v.m_ph[0] += f / SampleRate;
        v.m_ph[0] -= std::floor( v.m_ph[0] );
        break;
    }
    case BRASS:
        v.m_f1.set( 500.0f + 3200.0f * ( 1.0f - std::exp( -t * 14.0f ) ) * ( 0.6f + 0.4f * std::exp( -t * 2.0f ) ), 0.9f );
        y = v.m_f1.low( ( saw( v.m_ph[0], f * 0.996f * vib ) + saw( v.m_ph[1], f * 1.004f * vib ) ) * 0.6f );
        break;
    case PLUCK:
        v.m_f1.set( 300.0f + 5000.0f * std::exp( -t * 16.0f ), 0.9f );
        y = v.m_f1.low( saw( v.m_ph[0], f ) ) * std::exp( -t * 4.0f );
        break;
    case GUITAR: y = pluckString( v, v.m_gate ? 0.9975f : 0.993f ); break;
    case TWANG: y = pluckString( v, v.m_gate ? 0.9985f : 0.994f ); break;
    case SITAR:
    {
        // The jawari bridge: a buzzing edge on every pluck.
        const float s = pluckString( v, v.m_gate ? 0.9988f : 0.995f );
        y = s + 0.3f * std::tanh( s * 8.0f ) * std::exp( -t * 3.0f );
        break;
    }
    case KOTO: y = pluckString( v, v.m_gate ? 0.9965f : 0.992f ); break;
    case OUD: y = pluckString( v, v.m_gate ? 0.9968f : 0.992f ); break;
    case CHARANGO:
        // Tremolo picking while the note is held.
        if( v.m_gate && t > 0.07f && std::fmod( t, 0.075f ) < dt ) excite( v, 0.45f, 0.8f );
        y = pluckString( v, v.m_gate ? 0.995f : 0.99f );
        break;
    case STEEL_DRUM:
    {
        const float index = 2.0f * std::exp( -t * 7.0f ) + 0.3f;
        const float mod = osc( v.m_ph[1], f ) * index;
        y = std::sin( Pi2 * v.m_ph[0] + mod ) * std::exp( -t * 2.6f ) + osc( v.m_ph[2], f * 2.0f ) * 0.25f * std::exp( -t * 4.0f );
        v.m_ph[0] += f / SampleRate;
        v.m_ph[0] -= std::floor( v.m_ph[0] );
        break;
    }
    case MARIMBA:
    {
        const float mod = osc( v.m_ph[1], f * 4.0f ) * 1.6f * std::exp( -t * 30.0f );
        y = std::sin( Pi2 * v.m_ph[0] + mod ) * std::exp( -t * 7.0f );
        v.m_ph[0] += f / SampleRate;
        v.m_ph[0] -= std::floor( v.m_ph[0] );
        break;
    }
    case KALIMBA:
        y = osc( v.m_ph[0], f ) * std::exp( -t * 3.5f ) + osc( v.m_ph[1], f * 5.6f ) * 0.35f * std::exp( -t * 20.0f ) +
            v.m_noise.next() * 0.2f * std::exp( -t * 400.0f );
        break;
    case GAMELAN:
    {
        // Two bronze keys a few cents apart: the shimmer (ombak) of a gamelan.
        const float index = 1.8f * std::exp( -t * 1.2f ) + 0.2f;
        const float m1 = osc( v.m_ph[2], f * 1.414f ) * index;
        const float m2 = osc( v.m_ph[3], f * 1.006f * 1.414f ) * index;
        y = ( std::sin( Pi2 * v.m_ph[0] + m1 ) + std::sin( Pi2 * v.m_ph[1] + m2 ) ) * 0.5f * std::exp( -t * 0.9f );
        v.m_ph[0] += f / SampleRate;
        v.m_ph[1] += f * 1.006f / SampleRate;
        v.m_ph[0] -= std::floor( v.m_ph[0] );
        v.m_ph[1] -= std::floor( v.m_ph[1] );
        break;
    }
    case VIBES:
        y = ( osc( v.m_ph[0], f ) + osc( v.m_ph[1], f * 4.0f ) * 0.25f * std::exp( -t * 6.0f ) ) * std::exp( -t * 1.1f ) *
            ( 1.0f + 0.3f * std::sin( Pi2 * 5.5f * t ) );
        break;
    case ACCORDION: y = v.m_f1.low( ( pulse( v.m_ph[0], f, 0.42f ) + pulse( v.m_ph[1], f * 1.0045f, 0.42f ) ) * 0.5f ); break;
    case BANDONEON: y = v.m_f1.low( ( pulse( v.m_ph[0], f, 0.48f ) + pulse( v.m_ph[1], f * 0.997f, 0.48f ) ) * 0.6f ); break;
    case HARMONICA:
    {
        const float x = pulse( v.m_ph[0], f * ( 1.0f - 0.04f * std::exp( -t * 14.0f ) ) * vib, 0.3f );
        y = v.m_f1.low( x ) * 0.6f + v.m_f2.band( x ) * 0.5f;
        break;
    }
    case FLUTE:
        y = osc( v.m_ph[0], f * vib ) + osc( v.m_ph[1], f * 2.0f * vib ) * 0.22f + v.m_f1.band( v.m_noise.next() ) * 0.12f +
            v.m_noise.next() * 0.3f * std::exp( -t * 40.0f );
        break;
    case PANFLUTE:
    {
        const float fp = f * ( 1.0f - 0.02f * std::exp( -t * 20.0f ) );
        y = osc( v.m_ph[0], fp ) + osc( v.m_ph[1], fp * 2.0f ) * 0.12f + v.m_f1.band( v.m_noise.next() ) * 0.22f +
            v.m_noise.next() * 0.5f * std::exp( -t * 30.0f );
        break;
    }
    case NEY:
    {
        const float vn = t > 0.3f ? 1.0f + 0.006f * std::sin( Pi2 * 4.0f * t ) : 1.0f;
        y = osc( v.m_ph[0], f * vn ) + osc( v.m_ph[1], f * 3.0f * vn ) * 0.1f + v.m_f1.band( v.m_noise.next() ) * 0.3f;
        break;
    }
    case WHISTLE:
    {
        // The "cut": a grace note a step above starts the note.
        const float fw = t < 0.03f ? f * 1.122f : f * ( t > 0.2f ? 1.0f + 0.006f * std::sin( Pi2 * 6.0f * t ) : 1.0f );
        y = osc( v.m_ph[0], fw ) + osc( v.m_ph[1], fw * 2.0f ) * 0.08f + v.m_f1.band( v.m_noise.next() ) * 0.06f;
        break;
    }
    case FIDDLE:
    {
        const float x = saw( v.m_ph[0], f * ( t > 0.2f ? 1.0f + 0.005f * std::sin( Pi2 * 6.0f * t ) : 1.0f ) );
        y = v.m_f1.low( x ) * 0.6f + v.m_f2.band( x ) * 0.6f + v.m_noise.next() * 0.02f;
        break;
    }
    case ERHU:
    {
        const float x = saw( v.m_ph[0], f * ( t > 0.15f ? 1.0f + 0.01f * std::sin( Pi2 * 6.0f * t ) : 1.0f ) );
        y = v.m_f2.band( x ) * 0.9f + v.m_f1.low( x ) * 0.4f;
        break;
    }
    case ORGAN:
        y = ( osc( v.m_ph[0], f ) * 0.8f + osc( v.m_ph[1], f * 2.0f ) * 0.6f + osc( v.m_ph[2], f * 3.0f ) * 0.3f +
              osc( v.m_ph[3], f * 4.0f ) * 0.2f ) * ( 1.0f + 0.15f * std::sin( Pi2 * 6.5f * t ) ) * 0.6f +
            v.m_noise.next() * 0.3f * std::exp( -t * 300.0f );
        break;
    case DIST_GUITAR:
    {
        const float x = saw( v.m_ph[0], f ) + saw( v.m_ph[1], f * 1.006f ) + 0.5f * saw( v.m_ph[2], f * 0.5f );
        y = v.m_f1.low( std::tanh( x * 4.0f ) ) * 0.6f;
        break;
    }
    case TRUMPET:
    {
        const float fs = f * ( 1.0f - 0.03f * std::exp( -t * 25.0f ) ) * ( t > 0.2f ? 1.0f + 0.006f * std::sin( Pi2 * 5.5f * t ) : 1.0f );
        v.m_f1.set( 900.0f + 4200.0f * ( 1.0f - std::exp( -t * 18.0f ) ) * ( 0.7f + 0.3f * std::exp( -t * 3.0f ) ), 1.0f );
        y = v.m_f1.low( saw( v.m_ph[0], fs ) + saw( v.m_ph[1], fs * 1.003f ) ) * 0.6f;
        break;
    }
    case CLARINET: y = v.m_f1.low( pulse( v.m_ph[0], f * vib, 0.5f ) ) * 0.8f + v.m_noise.next() * 0.01f; break;
    default: break;
    }
    return y * v.m_vel * ( in.m_sustained ? v.m_env : 1.0f ) * in.m_level;
}

void MusicSynth::bassStep( int s16, int root, int fifth )
{
    const Style &st = styles()[static_cast<size_t>( m_station )];
    bool         hit = false;
    int          note = root;
    float        len = 0.9f;
    auto         pat = [&]( const char *p, int lo = 0 )
    {
        const char c = p[s16];
        hit = c != '.';
        note = c == 'f' ? root + fifth - 12 : c == 'u' ? root + 12 : c == 'w' ? root - 1 : root + lo;
    };
    switch( st.m_bass )
    {
    case BassE::OCTAVE:
        hit = m_variant == 1 ? s16 % 2 == 0 : true;
        note = root + ( ( m_variant == 1 ? s16 / 2 : s16 ) % 2 ? 12 : 0 );
        break;
    case BassE::OFFBEAT:
        hit = s16 % 4 == 2 || ( m_variant == 2 && s16 % 8 == 7 );
        note = root + ( s16 % 8 == 7 ? 12 : 0 );
        break;
    case BassE::ROLLING: hit = s16 % 4 != 0; break;
    case BassE::REESE:
        hit = s16 == 0 || s16 == 8 || ( m_variant == 1 && s16 == 11 );
        len = s16 == 11 ? 1.5f : 6.5f;
        break;
    case BassE::CHIP:
    {
        static const int arp[4] = { 0, 12, 7, 12 };
        hit = s16 % 2 == 0;
        note = root + arp[( s16 / 2 ) % 4];
        len = 1.6f;
        break;
    }
    case BassE::FUNKY:
        pat( m_variant == 0 ? "x..x.xu...x.x.u." : m_variant == 1 ? "x.x...xu..x..fx." : "x..xx..ux.x.x..f" );
        len = 1.2f;
        break;
    case BassE::BIGBEAT:
    {
        static const char *p = "x...x..x..x...x.";
        hit = p[s16] == 'x';
        note = root + ( s16 == 7 ? 3 : s16 == 14 ? 5 : 0 );
        len = 1.5f;
        break;
    }
    case BassE::ACID:
        hit = m_acid[s16] >= 0;
        note = root + std::max( 0, m_acid[s16] );
        m_bassAccent = m_accent[s16];
        if( hit && m_slide[s16] && m_bassGate > 0.0f )
        {
            m_bassTarget = mtof( static_cast<float>( 33 + note ) );
            m_bassGate = 1.0f;
            return;
        }
        len = 0.7f;
        break;
    case BassE::SUB:
        pat( m_variant == 0 ? "x.....x...x....." : m_variant == 1 ? "x..x......x..f.." : "x.......x.f....." );
        len = 3.0f;
        break;
    case BassE::POLKA: pat( m_bpm > 128.0f ? "x...f...x...f..." : "x.......f......." ); len = 1.4f; break;
    case BassE::SAMBA: pat( "x.....f.x.....f." ); len = 1.8f; break;
    case BassE::HABANERA: pat( "x..f..x.x..f..x." ); len = 1.2f; break;
    case BassE::COUNTRY: pat( m_variant == 2 ? "x.......f...w.w." : "x...f...x...f..." ); len = 1.6f; break;
    case BassE::GALLOP: pat( "x.xx....x.xx...." ); len = 0.9f; break;
    }
    if( !hit ) return;
    m_bassTarget = mtof( static_cast<float>( 33 + note ) );
    if( st.m_bass != BassE::SUB || m_bassT > 1.0f ) m_bassF = m_bassTarget;
    m_bassT = 0.0f;
    const double stepLen = static_cast<double>( SampleRate ) * 60.0 / static_cast<double>( m_bpm ) / 4.0;
    m_bassGate = len * static_cast<float>( stepLen / SampleRate );
}

void MusicSynth::onStep()
{
    const Style &st = styles()[static_cast<size_t>( m_station )];
    const int    s16 = m_step % 16;
    const int    bar = ( m_step / 16 ) % std::max( 1, m_totalBars );

    if( s16 == 0 )
    { // Where in the arrangement, which chord.
        int b = bar, section = 0;
        while( section < m_sectionCount - 1 && b >= m_sections[static_cast<size_t>( section )].m_bars )
        {
            b -= m_sections[static_cast<size_t>( section )].m_bars;
            ++section;
        }
        const Section &sec = m_sections[static_cast<size_t>( section )];
        m_layers = sec.m_layers;
        m_sectionBar = b;
        m_sectionLen = sec.m_bars;
        m_melodyIndex = sec.m_melody;
        m_riser = sec.m_riser;
        if( b == 0 ) m_riserT = 0.0f;
        if( b == 0 && section > 0 && ( m_layers & L_DRUMS ) ) m_crashT = 0.0f;

        const int ci = ( bar / m_barsPerChord ) % m_progLen;
        const int deg = m_prog[ci];
        const int rootSemis = semitones( deg );
        m_root = ( ( rootSemis % 12 ) + 12 ) % 12;
        // The chord's tones from the scale: a third if there is one, else a fourth; a fifth; a seventh.
        const ScaleDef &sc = Scales[m_scale];
        auto            inScale = [&]( int semis )
        {
            const int x = ( ( m_root + semis ) % 12 + 12 ) % 12;
            for( int i = 0; i < sc.m_n; ++i )
                if( sc.m_steps[i] == x ) return true;
            return false;
        };
        const int third = inScale( 3 ) && !inScale( 4 ) ? 3 : inScale( 4 ) ? 4 : inScale( 3 ) ? 3 : 5;
        const int fifth = inScale( 7 ) ? 7 : inScale( 6 ) ? 6 : 8;
        const int seventh = inScale( 10 ) ? 10 : inScale( 11 ) ? 11 : 12;
        m_chordTones[0] = 0;
        m_chordTones[1] = third;
        m_chordTones[2] = fifth;
        m_chordTones[3] = m_sevenths ? seventh : 12;
        for( int i = 0; i < 4; ++i )
        {
            int m = 55 + m_key + m_root + m_chordTones[i];
            while( m > 72 ) m -= 12;
            while( m < 52 ) m += 12;
            m_padF[i] = mtof( static_cast<float>( m ) );
            m_chordF[i] = mtof( static_cast<float>( m + 7 > 72 ? m : m + 12 ) );
        }
        for( int i = 0; i < 3; ++i ) m_chipChord[i] = mtof( static_cast<float>( 69 + m_key + m_root + m_chordTones[i] ) );
    }
    const bool lastBar = m_sectionBar == m_sectionLen - 1;
    const int  root = m_key + m_root;

    // Drums, with fills at the end of a section.
    if( m_layers & L_DRUMS )
    {
        const Kit &k = Kits[m_drumKit][m_drumVariant];
        if( lastBar && s16 >= 12 && ( m_sectionBar > 0 || m_sectionLen == 1 ) )
        {
            if( m_variant == 1 )
            {
                m_snareT = 0.0f;
                m_snareVel = 0.5f + 0.12f * static_cast<float>( s16 - 12 );
            }
            else
            {
                m_tomT = 0.0f;
                m_tomF = 220.0f - 30.0f * static_cast<float>( s16 - 12 );
                m_tomPh = 0.0f;
            }
        }
        else
        {
            if( vel( k.m_kick, s16 ) > 0.0f )
            {
                m_kickT = 0.0f;
                m_kickPh = 0.0f;
                m_kickVel = vel( k.m_kick, s16 );
            }
            if( vel( k.m_snare, s16 ) > 0.0f )
            {
                m_snareT = 0.0f;
                m_snareVel = vel( k.m_snare, s16 );
            }
        }
        if( vel( k.m_open, s16 ) > 0.0f )
        {
            m_hatT = 0.0f;
            m_hatOpen = true;
            m_hatVel = 0.8f;
        }
        else if( vel( k.m_hat, s16 ) > 0.0f )
        {
            m_hatT = 0.0f;
            m_hatOpen = false;
            m_hatVel = vel( k.m_hat, s16 );
        }
    }
    if( m_layers & L_PERC )
    {
        const Perc &pc = Percs[static_cast<int>( st.m_perc )];
        for( int lane = 0; lane < 5; ++lane )
        {
            const float v = vel( pc.m_lanes[m_percVariant][lane], s16 );
            if( v <= 0.0f ) continue;
            m_pT[lane] = 0.0f;
            m_pVel[lane] = v;
            m_pPh[lane] = 0.0f;
            m_pAlt[lane] = s16 % 4 == 0 ? 0.0f : 1.0f;
        }
    }
    if( m_layers & L_BASS ) bassStep( s16, root, m_chordTones[2] );

    // Chords.
    if( m_layers & L_CHORDS )
    {
        const char *p = nullptr;
        switch( st.m_chords )
        {
        case ChordE::STABS:
            if( ( st.m_drums == DrumsE::FUNK ? "x..x..x...x.x..." : "..x...x...x..x.." )[s16] == 'x' ) m_stabT = 0.0f;
            break;
        case ChordE::SKANK:
            if( ( m_variant == 0 ? "....x.......x..." : "..x...x...x...x." )[s16] == 'x' ) m_stabT = 0.0f;
            break;
        case ChordE::COMP: p = m_variant == 0 ? "x.....x...x....." : m_variant == 1 ? "x..x....x..x...." : "..x.....x.....x."; break;
        case ChordE::STRUM:
            p = st.m_drums == DrumsE::SAMBA      ? "x..x..x...x.x..."
                : st.m_drums == DrumsE::POLKA    ? "..x...x...x...x."
                : st.m_drums == DrumsE::ROCK     ? "x.x.x.x.x.x.x.x."
                : m_variant == 0                 ? "x.x.xx.xx.x.xx.x"
                                                 : "x...x.x.x...x.x.";
            break;
        case ChordE::BRASS: p = st.m_drums == DrumsE::POLKA ? "..x...x...x...x." : "x...x...x.x.x..."; break;
        default: break;
        }
        if( p && p[s16] == 'x' )
        {
            if( st.m_chords == ChordE::STRUM )
            { // Down on the beats, up between them.
                const bool down = s16 % 4 == 0;
                for( int i = 0; i < 4; ++i ) m_strumDelay[i] = ( down ? i : 3 - i ) * 0.011f;
            }
            else
            {
                for( int i = 0; i < 4; ++i ) noteOn( m_chordVoice[i], st.m_chordInst, m_chordF[i], 0.7f );
            }
        }
        if( st.m_chords == ChordE::BELLS && s16 % 2 == 0 )
        { // Interlocking: each eighth another chord tone, an octave above.
            static const int order[8] = { 0, 2, 1, 3, 2, 0, 3, 1 };
            const int        i = order[( s16 / 2 + m_variant ) % 8];
            noteOn( m_chordVoice[i], st.m_chordInst, m_chordF[i] * 2.0f, s16 % 4 == 0 ? 0.8f : 0.55f );
        }
        if( st.m_chords == ChordE::PLUCKS && ( "x.xxx.xxx.xxx.x." )[s16] == 'x' )
        {
            static const int order[4] = { 0, 2, 1, 3 };
            const int        i = order[s16 % 4];
            noteOn( m_chordVoice[i], st.m_chordInst, m_chordF[i] * ( s16 % 8 >= 4 ? 2.0f : 1.0f ), 0.6f );
        }
    }

    // Arpeggios.
    if( ( m_layers & L_ARP ) && st.m_arp == ArpE::UP )
    {
        static const int pats[3][8] = { { 0, 1, 2, 3, 2, 1, 2, 3 }, { 0, 2, 1, 3, 0, 2, 3, 2 }, { 3, 2, 1, 0, 1, 2, 3, 2 } };
        const int        tones[4] = { 0, m_chordTones[1], m_chordTones[2], 12 };
        noteOn( m_arp, PLUCK, mtof( static_cast<float>( 69 + root + tones[pats[m_variant][s16 % 8]] ) ), 1.0f );
    }
    if( ( m_layers & L_ARP ) && st.m_arp == ArpE::CHIP && s16 % 2 == 0 ) m_chipT = 0.0f;

    // Tension: sixteenth hats and a high alarm on the eighths, the root and its fifth.
    if( m_tension > 0.05f )
    {
        m_tHatT = 0.0f;
        m_tHatF.set( 7500.0f, 0.9f );
        if( s16 % 2 == 0 )
        {
            m_tBlipT = 0.0f;
            m_tBlipF = mtof( static_cast<float>( 81 + root + ( s16 % 4 == 0 ? 0 : 7 ) ) );
        }
    }

    // The lead and the counter line.
    auto play = [&]( Tone &v, int inst, const int8_t *line, float octave )
    {
        const int idx = ( m_sectionBar % 4 ) * 16 + s16;
        const int d = line[idx];
        if( d >= 0 )
        {
            const int   prev = line[( idx + 63 ) % 64];
            const float f = mtof( static_cast<float>( Insts[inst].m_base + m_key + semitones( d ) ) + octave );
            noteOn( v, inst, f, s16 % 4 == 0 ? 1.0f : 0.8f, prev >= 0 || prev == -2 );
        }
        else if( d == -1 )
        {
            v.m_gate = false;
        }
    };
    if( m_layers & L_LEAD ) play( m_lead, m_leadInst, m_melody[m_melodyIndex], 0.0f );
    else m_lead.m_gate = false;
    if( ( m_layers & L_COUNTER ) && m_counterInst >= 0 ) play( m_counterVoice, m_counterInst, m_counter, m_counterInst == m_leadInst ? -12.0f : 0.0f );
    else m_counterVoice.m_gate = false;

    // The tanpura: Pa, Sa, Sa, low Sa, every bar.
    if( ( m_layers & L_DRONE ) && st.m_drone == DroneE::TANPURA && s16 % 4 == 0 )
    {
        static const int notes[4] = { -5, 12, 12, 0 };
        noteOn( m_tanpura, SITAR, mtof( static_cast<float>( 48 + m_key + notes[s16 / 4] ) ), 0.6f );
    }
}

float MusicSynth::renderDrums( float &reverbSend )
{
    const Style &st = styles()[static_cast<size_t>( m_station )];
    const float  dt = 1.0f / SampleRate;
    const bool   chip = st.m_drums == DrumsE::CHIP;
    float        out = 0.0f;
    reverbSend = 0.0f;
    if( m_kickT < 1.0f )
    {
        if( chip )
        {
            m_kickPh += ( 40.0f + 160.0f * std::exp( -m_kickT * 40.0f ) ) * dt;
            out += ( m_kickPh - std::floor( m_kickPh ) < 0.5f ? 1.0f : -1.0f ) * std::exp( -m_kickT * 14.0f ) * 0.5f * st.m_kick;
        }
        else
        {
            m_kickPh += ( 46.0f + 150.0f * std::exp( -m_kickT * 30.0f ) ) * dt;
            float k = std::sin( Pi2 * m_kickPh ) * std::exp( -m_kickT * ( st.m_drums == DrumsE::DNB ? 7.0f : 5.5f ) );
            if( st.m_drums == DrumsE::BREAKS || st.m_drums == DrumsE::DNB || st.m_drums == DrumsE::ROCK )
                k += m_kickClick.high( m_n.next() ) * std::exp( -m_kickT * 120.0f ) * 0.5f;
            out += k * st.m_kick * m_kickVel;
        }
        m_kickT += dt;
    }
    if( m_snareT < 0.9f )
    {
        float s = 0.0f;
        if( chip )
        {
            s = m_snareF.high( m_n.next() ) * std::exp( -m_snareT * 22.0f );
        }
        else
        {
            const float decay = st.m_drums == DrumsE::SYNTHWAVE ? 9.0f : st.m_drums == DrumsE::LOFI ? 18.0f : 15.0f;
            s = m_snareF.band( m_n.next() ) * 1.6f * std::exp( -m_snareT * decay ) +
                std::sin( Pi2 * 185.0f * m_snareT ) * std::exp( -m_snareT * 24.0f ) * 0.35f;
        }
        s *= m_snareVel * st.m_snare * 0.6f;
        out += s;
        reverbSend += s * ( st.m_drums == DrumsE::SYNTHWAVE ? 1.0f : 0.4f );
        m_snareT += dt;
    }
    if( m_hatT < 0.5f )
    {
        out += m_hatF.high( m_n.next() ) * std::exp( -m_hatT * ( m_hatOpen ? 13.0f : 60.0f ) ) * st.m_hat * m_hatVel * ( m_hatOpen ? 1.4f : 1.0f );
        m_hatT += dt;
    }
    if( m_tomT < 0.6f )
    {
        out += osc( m_tomPh, m_tomF * ( 1.0f + 0.5f * std::exp( -m_tomT * 25.0f ) ) ) * std::exp( -m_tomT * 9.0f ) * st.m_kick * 0.5f;
        m_tomT += dt;
    }
    if( m_crashT < 2.5f && !chip )
    {
        const float c = m_crashF.high( m_n.next() ) * std::exp( -m_crashT * 1.8f ) * st.m_hat * 1.3f;
        out += c;
        reverbSend += c * 0.5f;
        m_crashT += dt;
    }
    return out;
}

float MusicSynth::renderPerc()
{
    const Style &st = styles()[static_cast<size_t>( m_station )];
    if( st.m_perc == PercE::NONE ) return 0.0f;
    const Perc &pc = Percs[static_cast<int>( st.m_perc )];
    const float dt = 1.0f / SampleRate;
    float       out = 0.0f;
    if( m_pT[0] < 1.2f )
    { // Low drum: a pitched thump (the bayan rises, the others fall).
        const float t = m_pT[0];
        const float f = pc.m_lowF * ( 1.0f + pc.m_lowBend * std::exp( -t * ( pc.m_lowBend < 0.0f ? 10.0f : 30.0f ) ) );
        out += ( osc( m_pPh[0], f ) + m_pF[0].low( m_n.next() ) * std::exp( -t * 40.0f ) * 0.5f ) * std::exp( -t * pc.m_lowDecay ) * m_pVel[0];
        m_pT[0] += dt;
    }
    if( m_pT[1] < 1.0f )
    { // High drum: a ring and a slap.
        const float t = m_pT[1];
        const float f = pc.m_highF * ( m_pAlt[1] > 0.5f ? 1.12f : 1.0f );
        out += ( osc( m_pPh[1], f ) * ( 1.0f - pc.m_highClick ) + m_pF[1].band( m_n.next() ) * pc.m_highClick * 2.0f ) * std::exp( -t * pc.m_highDecay ) *
               m_pVel[1] * 0.8f;
        m_pT[1] += dt;
    }
    if( m_pT[2] < 0.2f )
    {
        out += m_pF[2].high( m_n.next() ) * std::exp( -m_pT[2] * 45.0f ) * m_pVel[2] * 0.5f;
        m_pT[2] += dt;
    }
    if( m_pT[3] < 2.0f )
    {
        const float t = m_pT[3];
        float       b = 0.0f;
        switch( pc.m_bell )
        {
        case 0: b = m_pF[3].band( pulse( m_pPh[3], 540.0f ) + pulse( m_pAlt[3], 800.0f ) ) * std::exp( -t * 12.0f ) * 1.5f; break;
        case 1: b = osc( m_pPh[3], m_pAlt[3] > 0.5f ? 700.0f : 900.0f ) * std::exp( -t * 9.0f ); break;
        case 2: b = ( osc( m_pPh[3], 82.0f ) + osc( m_pAlt[3], 82.0f * 1.52f ) * 0.4f ) * std::exp( -t * 0.9f ) * 1.2f; break;
        default: b = osc( m_pPh[3], 4000.0f ) * std::exp( -t * 3.0f ) * 0.3f; break;
        }
        out += b * m_pVel[3] * 0.6f;
        m_pT[3] += dt;
    }
    if( m_pT[4] < 0.3f )
    {
        const float t = m_pT[4];
        float       c = 0.0f;
        switch( pc.m_click )
        {
        case 0: c = osc( m_pPh[4], 2500.0f ) * std::exp( -t * 60.0f ); break;
        case 1: c = m_pF[4].band( m_n.next() ) * std::exp( -t * 80.0f ) * 2.0f + osc( m_pPh[4], 900.0f ) * std::exp( -t * 70.0f ) * 0.4f; break;
        case 2: c = m_pF[4].band( m_n.next() ) * ( std::exp( -t * 150.0f ) + ( t > 0.02f ? std::exp( -( t - 0.02f ) * 150.0f ) : 0.0f ) ) * 2.0f; break;
        case 3: c = osc( m_pPh[4], 1800.0f ) * std::exp( -t * 50.0f ) + m_pF[4].band( m_n.next() ) * std::exp( -t * 90.0f ); break;
        default: c = m_pF[2].high( m_n.next() ) * std::exp( -t * 18.0f ) * ( 0.6f + 0.4f * std::sin( Pi2 * 60.0f * t ) ); break;
        }
        out += c * m_pVel[4] * 0.6f;
        m_pT[4] += dt;
    }
    return out * st.m_percLevel;
}

float MusicSynth::renderDrone()
{
    const Style &st = styles()[static_cast<size_t>( m_station )];
    if( st.m_drone == DroneE::NONE || !( m_layers & L_DRONE ) ) return st.m_drone == DroneE::TANPURA ? render( m_tanpura ) * st.m_droneLevel : 0.0f;
    const float dt = 1.0f / SampleRate;
    m_droneT += dt;
    const float base = mtof( static_cast<float>( 36 + m_key ) );
    float       y = 0.0f;
    switch( st.m_drone )
    {
    case DroneE::TANPURA: y = render( m_tanpura ); break;
    case DroneE::DIDGERIDOO:
    {
        // A low buzz through a mouth that opens and closes with the beat.
        const float rate = m_bpm / 60.0f * 0.5f;
        const float x = saw( m_dronePh[0], base * 1.06f ) * ( 1.0f + 0.2f * std::sin( Pi2 * 31.0f * m_droneT ) );
        m_droneF1.set( 420.0f + 260.0f * std::sin( Pi2 * rate * m_droneT ), 3.0f );
        m_droneF2.set( 1100.0f, 2.0f );
        y = ( m_droneF1.band( x ) + 0.5f * m_droneF2.band( x ) ) * 1.2f + osc( m_dronePh[1], base * 1.06f ) * 0.3f;
        break;
    }
    case DroneE::BAGPIPE:
        m_droneF1.set( 1800.0f, 0.7f );
        y = m_droneF1.low( pulse( m_dronePh[0], base * 2.0f, 0.3f ) + pulse( m_dronePh[1], base * 3.0f, 0.3f ) * 0.6f ) * 0.5f;
        break;
    case DroneE::THROAT:
    {
        // One voice, two notes: the fundamental and a whistling overtone that wanders.
        const float x = saw( m_dronePh[0], base * 1.5f );
        const float h = 8.0f + 3.0f * std::sin( Pi2 * 0.11f * m_droneT ) + 1.5f * std::sin( Pi2 * 0.37f * m_droneT );
        m_droneF1.set( base * 1.5f * h, 14.0f );
        m_droneF2.set( 500.0f, 0.7f );
        y = m_droneF1.band( x ) * 1.6f + m_droneF2.low( x ) * 0.4f;
        break;
    }
    case DroneE::HUM:
        y = ( osc( m_dronePh[0], base * 2.0f ) + osc( m_dronePh[1], base * 3.0f ) * 0.6f ) * ( 0.7f + 0.3f * std::sin( Pi2 * 0.2f * m_droneT ) );
        break;
    default: break;
    }
    return y * st.m_droneLevel;
}

float MusicSynth::renderBass()
{
    const Style &st = styles()[static_cast<size_t>( m_station )];
    const float  dt = 1.0f / SampleRate;
    if( m_bassT > 4.0f ) return 0.0f;
    float       y = 0.0f;
    const float t = m_bassT;
    switch( st.m_bass )
    {
    case BassE::REESE:
    {
        const float a = saw( m_bassPh[0], m_bassF * 1.006f ) + saw( m_bassPh[1], m_bassF * 0.994f );
        m_bassF1.set( 380.0f + 220.0f * std::sin( Pi2 * 0.3f * static_cast<float>( m_step ) * 0.125f ), 1.4f );
        y = ( m_bassF1.low( a * 0.5f ) + sine( m_bassPh[2], m_bassF * 0.5f ) * 0.6f ) * ( t < m_bassGate ? 1.0f : std::exp( -( t - m_bassGate ) * 20.0f ) );
        break;
    }
    case BassE::CHIP: y = pulse( m_bassPh[0], m_bassF, 0.25f ) * ( t < m_bassGate ? 0.6f : 0.0f ); break;
    case BassE::FUNKY:
        m_bassF1.set( 400.0f + 2600.0f * std::exp( -t * 30.0f ), 1.5f );
        y = m_bassF1.low( pulse( m_bassPh[0], m_bassF ) ) * std::exp( -t * 6.0f );
        break;
    case BassE::BIGBEAT:
        m_bassF1.set( 1100.0f, 0.9f );
        y = m_bassF1.low( std::tanh( saw( m_bassPh[0], m_bassF ) * 4.0f ) ) * std::exp( -t * 3.0f ) * 0.8f;
        break;
    case BassE::ACID:
    {
        m_bassF += ( m_bassTarget - m_bassF ) * 0.0025f;
        m_bassF1.set( 220.0f + ( m_bassAccent ? 2800.0f : 1300.0f ) * std::exp( -t * 9.0f ), 6.5f );
        const float gate = t < m_bassGate ? 1.0f : std::exp( -( t - m_bassGate ) * 40.0f );
        y = std::tanh( m_bassF1.low( saw( m_bassPh[0], m_bassF ) ) * 1.6f ) * gate * ( m_bassAccent ? 1.0f : 0.75f );
        break;
    }
    case BassE::SUB:
    {
        // A deep sine that glides into its note, slightly driven.
        m_bassF += ( m_bassTarget - m_bassF ) * 0.002f;
        const float gate = t < m_bassGate ? 1.0f : std::exp( -( t - m_bassGate ) * 12.0f );
        y = std::tanh( sine( m_bassPh[0], m_bassF ) * 1.6f ) * gate * std::exp( -t * 0.6f ) * 0.9f;
        break;
    }
    case BassE::POLKA:
    case BassE::SAMBA:
    case BassE::HABANERA:
    case BassE::COUNTRY:
    case BassE::GALLOP:
    {
        // An acoustic bass: a round body and a short pluck.
        m_bassF1.set( 200.0f + 900.0f * std::exp( -t * 25.0f ), 1.0f );
        const float gate = t < m_bassGate ? 1.0f : std::exp( -( t - m_bassGate ) * 30.0f );
        y = ( sine( m_bassPh[0], m_bassF ) * 0.8f + m_bassF1.low( saw( m_bassPh[1], m_bassF ) ) * 0.5f ) * std::exp( -t * 3.0f ) * gate;
        break;
    }
    default:
    {
        const float decay = st.m_bass == BassE::ROLLING ? 9.0f : 2.5f;
        m_bassF1.set( 180.0f + 1700.0f * std::exp( -t * 14.0f ), 1.2f );
        y = m_bassF1.low( saw( m_bassPh[0], m_bassF ) ) * std::exp( -t * decay );
        break;
    }
    }
    m_bassT += dt;
    return y * st.m_bassLevel;
}

float MusicSynth::renderChords()
{
    const Style &st = styles()[static_cast<size_t>( m_station )];
    const float  dt = 1.0f / SampleRate;
    float        y = 0.0f;
    switch( st.m_chords )
    {
    case ChordE::NONE: return 0.0f;
    case ChordE::STABS:
        if( m_stabT < 1.0f )
        {
            const float decay = st.m_drums == DrumsE::FUNK ? 7.0f : 5.0f;
            for( int i = 0; i < 3; ++i )
            {
                const float f = m_padF[i] * 2.0f;
                const float mod = osc( m_padPh[i * 2 + 1], f ) * ( 1.6f * std::exp( -m_stabT * 8.0f ) + 0.2f );
                y += std::sin( Pi2 * m_padPh[i * 2] + mod );
                m_padPh[i * 2] += f / SampleRate;
                m_padPh[i * 2] -= std::floor( m_padPh[i * 2] );
            }
            y *= std::exp( -m_stabT * decay ) * 0.33f;
            m_stabT += dt;
        }
        return y * st.m_chordLevel;
    case ChordE::SKANK:
        if( m_stabT < 0.4f )
        {
            for( int i = 0; i < 3; ++i ) y += osc( m_padPh[i * 2], m_padF[i] * 2.0f ) + 0.5f * osc( m_padPh[i * 2 + 1], m_padF[i] * 4.0f );
            y *= std::exp( -m_stabT * 14.0f ) * 0.3f;
            m_stabT += dt;
        }
        return y * st.m_chordLevel;
    case ChordE::SAW:
    case ChordE::GATED:
    case ChordE::STRINGS:
    {
        const float attack = st.m_chords == ChordE::STRINGS ? 0.000015f : 0.00003f;
        m_padEnv += ( 1.0f - m_padEnv ) * attack;
        const int voices = m_sevenths ? 4 : 3;
        for( int i = 0; i < voices; ++i )
        {
            y += saw( m_padPh[i * 2], m_padF[i] * 1.004f );
            y += saw( m_padPh[i * 2 + 1], m_padF[i] * 0.996f );
        }
        if( ( m_step & 3 ) == 0 && m_clock < 1.0 )
            m_padF1.set( ( st.m_chords == ChordE::GATED ? 2400.0f : st.m_chords == ChordE::STRINGS ? 850.0f : 1100.0f ) * ( 0.75f + 0.6f * m_drive ), 0.8f );
        y = m_padF1.low( y ) * m_padEnv;
        if( st.m_chords == ChordE::GATED )
        {
            static const char *gate = "x.xx.x.xx.x.xx.x";
            const bool         open = gate[( m_step < 0 ? 0 : m_step ) % 16] == 'x';
            m_gateEnv += ( ( open ? 1.0f : 0.0f ) - m_gateEnv ) * ( open ? 0.02f : 0.004f );
            y *= m_gateEnv * 1.4f;
        }
        return y * st.m_chordLevel;
    }
    default:
        // Voices: strums wait for their offsets, the rest ring.
        for( int i = 0; i < 4; ++i )
        {
            if( m_strumDelay[i] >= 0.0f )
            {
                m_strumDelay[i] -= dt;
                if( m_strumDelay[i] < 0.0f ) noteOn( m_chordVoice[i], st.m_chordInst, m_chordF[i], 0.6f );
            }
            y += render( m_chordVoice[i] );
        }
        if( st.m_chords == ChordE::COMP || st.m_chords == ChordE::BRASS )
            for( Tone &v : m_chordVoice )
                if( v.m_t > 0.2f ) v.m_gate = false;
        return y * st.m_chordLevel * 2.5f;
    }
}

void MusicSynth::reverb( float in, float &l, float &r )
{
    float outL = 0.0f, outR = 0.0f;
    for( int i = 0; i < 8; ++i )
    {
        std::vector<float> &buf = m_comb[i];
        const float         y = buf[m_combI[i]];
        m_combLp[i] = y * 0.75f + m_combLp[i] * 0.25f;
        buf[m_combI[i]] = in + m_combLp[i] * 0.8f;
        m_combI[i] = ( m_combI[i] + 1 ) % buf.size();
        ( i < 4 ? outL : outR ) += y;
    }
    for( int i = 0; i < 4; ++i )
    {
        std::vector<float> &buf = m_ap[i];
        float              &v = i < 2 ? outL : outR;
        const float         b = buf[m_apI[i]];
        buf[m_apI[i]] = v + b * 0.5f;
        v = b - v * 0.5f;
        m_apI[i] = ( m_apI[i] + 1 ) % buf.size();
    }
    l = outL * 0.25f;
    r = outR * 0.25f;
}

void MusicSynth::tick( float &l, float &r )
{
    l = r = 0.0f;
    if( m_pending )
    {
        m_gain -= 0.00008f;
        if( m_gain <= 0.01f ) start( m_pendingSeed, m_pendingStation );
    }
    else
    {
        m_gain += ( ( m_on ? 1.0f : 0.0f ) - m_gain ) * 0.00004f;
    }
    if( m_gain < 1e-4f && !m_on ) return;
    const Style &st = styles()[static_cast<size_t>( m_station )];

    // Sixteenths with swing on the odd ones.
    const double stepLen = static_cast<double>( SampleRate ) * 60.0 / static_cast<double>( m_bpm ) / 4.0;
    const double thisLen = stepLen * ( ( m_step & 1 ) ? 1.0 - m_swing : 1.0 + m_swing );
    m_clock += 1.0;
    if( m_clock >= thisLen )
    {
        m_clock = m_clock >= 1e8 ? 0.0 : m_clock - thisLen;
        ++m_step;
        onStep();
        m_beatNow = m_step % 4 == 0;
    }
    else
    {
        m_beatNow = false;
    }
    m_tension += ( m_tensionTarget - m_tension ) * 0.00002f;

    float       reverbSend = 0.0f;
    const float drums = renderDrums( reverbSend );
    const float perc = renderPerc();
    const float duck = 1.0f - st.m_sidechain * std::exp( -m_kickT * 8.0f );
    const float bass = renderBass() * duck;
    const float chords = renderChords() * duck;
    const float drone = renderDrone() * ( 0.5f + 0.5f * duck );
    const float lead = render( m_lead ) * st.m_leadLevel;
    const float counter = render( m_counterVoice ) * st.m_leadLevel * 0.6f;
    float       arp = 0.0f;
    if( st.m_arp == ArpE::UP ) arp = render( m_arp ) * st.m_arpLevel * duck;
    if( st.m_arp == ArpE::CHIP && m_chipT < 1.0f )
    { // Tracker arpeggio: the chord's notes cycled every third of a sixteenth.
        const int k = static_cast<int>( m_clock / ( stepLen / 3.0 ) ) % 3;
        arp = pulse( m_chipPh, m_chipChord[k], 0.25f ) * std::exp( -m_chipT * 6.0f ) * 0.8f * st.m_arpLevel * duck;
        m_chipT += 1.0f / SampleRate;
    }
    // A noise sweep up through a rising section.
    float riser = 0.0f;
    if( m_riser )
    {
        const double secLen = stepLen * 16.0 * m_sectionLen;
        m_riserT += 1.0f;
        const float p = std::min( 1.0f, static_cast<float>( m_riserT / secLen ) );
        m_riserF.set( 300.0f + 7000.0f * p * p, 1.5f );
        riser = m_riserF.band( m_n.next() ) * p * p * st.m_hat * 1.2f;
    }

    // Ping-pong echo (three sixteenths) and reverb.
    const float  echoIn = ( arp + lead + counter ) * st.m_echo;
    const size_t n = m_delayL.size();
    const size_t len = std::min( n - 1, static_cast<size_t>( stepLen * 3.0 ) );
    const size_t rd = ( m_di + n - len ) % n;
    const float  dl = m_delayL[rd], dr = m_delayR[rd];
    m_delayL[m_di] = echoIn + dr * 0.38f;
    m_delayR[m_di] = dl * 0.38f;
    m_di = ( m_di + 1 ) % n;
    float rl = 0.0f, rr = 0.0f;
    reverb( ( reverbSend + chords * 0.6f + lead * 0.5f + counter * 0.5f + arp * 0.3f + perc * 0.2f + drone * 0.3f + riser * 0.5f ) * st.m_reverb, rl, rr );

    float tense = 0.0f;
    if( m_tension > 0.01f )
    {
        if( m_tHatT < 0.2f )
        {
            tense += m_tHatF.high( m_tN.next() ) * std::exp( -m_tHatT * 60.0f ) * 0.45f;
            m_tHatT += 1.0f / SampleRate;
        }
        if( m_tBlipT < 0.3f )
        {
            tense += pulse( m_tBlipPh, m_tBlipF, 0.25f ) * std::exp( -m_tBlipT * 14.0f ) * 0.4f;
            m_tBlipT += 1.0f / SampleRate;
        }
        tense *= m_tension;
    }
    // A little stereo: the counter line and the percussion lean apart.
    const float mid = drums + bass + chords + lead + arp + drone + riser + tense;
    const float g = m_gain * 0.5f * st.m_trim;
    l = ( mid + counter * 0.7f + perc * 1.15f + dl * 0.6f + rl ) * g;
    r = ( mid + counter * 1.3f + perc * 0.85f + dr * 0.6f + rr ) * g;
}

} // namespace sunstreak
