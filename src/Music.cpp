/**
 * @file Music.cpp
 * @brief The eight radio stations: styles, patterns, instruments, arrangement, echo and reverb.
 * @author reneweller
 */
#include "Music.h"

#include "Rng.h"

namespace sunstreak
{

namespace
{

using namespace dsp;

/** @brief Drum pattern families. */
enum class DrumsE
{
    FOUR,        ///< four on the floor (house)
    SYNTHWAVE,   ///< half-time kick, big snare
    TRANCE,      ///< four on the floor, rolling hats
    BREAKS,      ///< big beat breakbeat
    DNB,         ///< drum and bass
    CHIP,        ///< chiptune noise drums
    FUNK,        ///< syncopated funk
    TECHNO,      ///< techno with offbeat hats
};

/** @brief Bass lines. */
enum class BassE
{
    OCTAVE,      ///< sixteenths jumping octaves
    OFFBEAT,     ///< offbeat eighths
    ROLLING,     ///< gallop between the kicks
    REESE,       ///< long detuned notes
    CHIP,        ///< pulse arpeggio bass
    FUNKY,       ///< slap-ish syncopation
    BIGBEAT,     ///< distorted riff
    ACID,        ///< resonant line with accents and slides
};

/** @brief Lead instruments. */
enum class LeadE
{
    SQUARE,      ///< soft square
    SUPERSAW,    ///< three detuned saws
    PULSE,       ///< thin pulse (tracker, chiptune)
    EPIANO,      ///< FM electric piano
    BRASS,       ///< filtered saw brass
    PLUCK,       ///< plucked saw
};

/** @brief Pads and chords. */
enum class PadE
{
    SAW,         ///< warm saw pad
    GATED,       ///< trance gate
    STABS,       ///< electric piano stabs
    STRINGS,     ///< slow strings
    NONE,        ///< none
};

/** @brief A station's style. */
struct Style
{
    const char *m_name;         ///< station name
    float       m_bpmMin;       ///< slowest tempo
    float       m_bpmMax;       ///< fastest tempo
    DrumsE      m_drums;        ///< drums
    BassE       m_bass;         ///< bass
    LeadE       m_lead;         ///< lead
    PadE        m_pad;          ///< chords
    bool        m_arp;          ///< sixteenth arpeggio
    bool        m_chipArp;      ///< tracker-style fast chord arpeggio
    float       m_majorChance;  ///< share of tracks in major
    float       m_swing;        ///< swing
    float       m_sidechain;    ///< pumping
    float       m_reverb;       ///< reverb send
    float       m_echo;         ///< echo send
    float       m_duty;         ///< pulse width of a PULSE lead
    float       m_kick;         ///< kick level
    float       m_snare;        ///< snare level
    float       m_hat;          ///< hat level
    float       m_bassLevel;    ///< bass level
    float       m_leadLevel;    ///< lead level
    float       m_padLevel;     ///< pad level
    float       m_arpLevel;     ///< arpeggio level
    float       m_density;      ///< share of eighths the lead plays
};

const Style Styles[StationCount] = {
    { "SUNSET FM", 100, 112, DrumsE::SYNTHWAVE, BassE::OCTAVE, LeadE::SUPERSAW, PadE::SAW, true, false, 0.1f, 0.0f, 0.35f, 0.45f,
      0.35f, 0.5f, 0.9f, 0.65f, 0.1f, 0.38f, 0.07f, 0.05f, 0.1f, 0.6f },
    { "TURBO HOUSE", 122, 128, DrumsE::FOUR, BassE::OFFBEAT, LeadE::SQUARE, PadE::STABS, false, false, 0.5f, 0.0f, 0.5f, 0.25f,
      0.3f, 0.5f, 0.95f, 0.55f, 0.14f, 0.42f, 0.08f, 0.09f, 0.1f, 0.55f },
    { "AMIGA TECHNO", 134, 142, DrumsE::TECHNO, BassE::ACID, LeadE::PULSE, PadE::STRINGS, false, true, 0.2f, 0.0f, 0.3f, 0.15f,
      0.25f, 0.25f, 0.95f, 0.5f, 0.12f, 0.36f, 0.08f, 0.035f, 0.07f, 0.65f },
    { "TRANCE GATE", 136, 140, DrumsE::TRANCE, BassE::ROLLING, LeadE::SUPERSAW, PadE::GATED, true, false, 0.1f, 0.0f, 0.55f, 0.45f,
      0.4f, 0.5f, 0.95f, 0.5f, 0.12f, 0.36f, 0.07f, 0.05f, 0.09f, 0.7f },
    { "JUNGLE PRESSURE", 168, 176, DrumsE::DNB, BassE::REESE, LeadE::EPIANO, PadE::STRINGS, false, false, 0.2f, 0.0f, 0.2f, 0.35f,
      0.3f, 0.5f, 0.9f, 0.6f, 0.11f, 0.4f, 0.1f, 0.05f, 0.08f, 0.35f },
    { "8-BIT RACER", 148, 160, DrumsE::CHIP, BassE::CHIP, LeadE::PULSE, PadE::NONE, false, true, 0.7f, 0.0f, 0.0f, 0.05f,
      0.15f, 0.125f, 0.7f, 0.4f, 0.09f, 0.26f, 0.07f, 0.0f, 0.06f, 0.75f },
    { "FUSION DRIVE", 110, 118, DrumsE::FUNK, BassE::FUNKY, LeadE::BRASS, PadE::STABS, false, false, 0.8f, 0.1f, 0.1f, 0.25f,
      0.2f, 0.5f, 0.85f, 0.6f, 0.11f, 0.4f, 0.08f, 0.1f, 0.08f, 0.6f },
    { "BIG BEAT", 124, 132, DrumsE::BREAKS, BassE::BIGBEAT, LeadE::BRASS, PadE::NONE, false, false, 0.2f, 0.06f, 0.2f, 0.2f,
      0.25f, 0.5f, 1.0f, 0.75f, 0.12f, 0.4f, 0.08f, 0.0f, 0.08f, 0.45f },
};

/** @brief A drum pattern: one character per sixteenth, '1'..'9' velocity, '.' rest, 'x' open hat. */
struct DrumPattern
{
    const char *m_kick;   ///< kick
    const char *m_snare;  ///< snare or clap
    const char *m_hat;    ///< closed hat
    const char *m_open;   ///< open hat
};

const DrumPattern Patterns[8] = {
    { "9...9...9...9...", "....9.......9...", "3.6.3.6.3.6.3.6.", "..x...x...x...x." },
    { "9.......9.6.....", "....9.......9...", "5.4.5.4.5.4.5.4.", "................" },
    { "9...9...9...9...", "....7.......7...", "4353435343534353", "..x...x...x...x." },
    { "9.7.......9..6..", "....9..3.3..9..4", "6.4.6.4.6.4.6.4.", "..............x." },
    { "9.........9.....", "....9..2.2..9..3", "6363636363636363", "..............x." },
    { "9...9...9...9...", "....9.......9.7.", "5.5.5.5.5.5.5.5.", "................" },
    { "9..6..9...9..6..", "....9..3....9.3.", "5454545454545454", "......x.......x." },
    { "9...9...9...9...", "....7.......7...", "3373337333733373", "..x...x...x...x." },
};

/**
 * @brief A pattern character as a velocity.
 * @param[in] c  the character
 * @return 0 (rest) .. 1
 */
float vel( char c )
{
    return c >= '1' && c <= '9' ? static_cast<float>( c - '0' ) / 9.0f : ( c == 'x' ? 1.0f : 0.0f );
}

const int MinorScale[10] = { 0, 2, 3, 5, 7, 8, 10, 12, 14, 15 };   ///< natural minor over an octave and a half
const int MajorScale[10] = { 0, 2, 4, 5, 7, 9, 11, 12, 14, 16 };   ///< major

} // namespace

const char *stationName( int station )
{
    return Styles[std::clamp( station, 0, StationCount - 1 )].m_name;
}

std::string trackTitle( uint64_t seed )
{
    static const char *adjectives[24] = { "MIDNIGHT", "CHROME",  "NEON",    "VELVET",  "SOLAR",    "ELECTRIC", "CRYSTAL", "GOLDEN",
                                          "RADIANT",  "PHANTOM", "SILVER",  "CRIMSON", "TURBO",    "DIGITAL",  "COSMIC",  "SAPPHIRE",
                                          "FADING",   "INFINITE", "LUNAR",  "BURNING", "ENDLESS",  "HYPER",    "AMBER",   "ULTRA" };
    static const char *nouns[24] = { "HIGHWAY",   "HORIZON", "MIRAGE",   "PULSE",    "DRIFT",   "RUNNER", "SKYLINE",  "OVERDRIVE",
                                     "AFTERGLOW", "VELOCITY", "SUNRISE", "STREAK",   "CIRCUIT", "PARADISE", "FRONTIER", "HEARTBEAT",
                                     "LIGHTS",    "ECHOES",  "MOTION",   "SIGNAL",   "CORONA",  "COASTLINE", "APEX",    "TWILIGHT" };
    const uint64_t h = hash64( seed * 0x9E37ull + 3 );
    return std::string( adjectives[h % 24] ) + " " + nouns[( h >> 8 ) % 24];
}

MusicSynth::MusicSynth()
{
    m_delayL.assign( static_cast<size_t>( SampleRate ), 0.0f );
    m_delayR.assign( static_cast<size_t>( SampleRate ), 0.0f );
    static const int combs[4] = { 1687, 1601, 2053, 2251 };
    static const int aps[2] = { 556, 441 };
    for( int i = 0; i < 8; ++i ) m_comb[i].assign( static_cast<size_t>( combs[i % 4] + ( i >= 4 ? 23 : 0 ) ), 0.0f );
    for( int i = 0; i < 4; ++i ) m_ap[i].assign( static_cast<size_t>( aps[i % 2] + ( i >= 2 ? 23 : 0 ) ), 0.0f );
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

void MusicSynth::start( uint64_t seed, int station )
{
    m_station = std::clamp( station, 0, StationCount - 1 );
    const Style &st = Styles[m_station];
    Rng          r( seed * 2654435761ull + 17 );
    m_bpm = st.m_bpmMin + ( st.m_bpmMax - st.m_bpmMin ) * r.uniform();
    m_bpm = std::round( m_bpm );
    m_swing = st.m_swing;
    m_key = r.rangeInt( -5, 6 );
    m_major = r.chance( st.m_majorChance );
    static const int  minorProgs[5][4] = { { 0, 8, 3, 10 }, { 0, 5, 8, 7 }, { 0, 10, 8, 10 }, { 8, 10, 0, 0 }, { 0, 3, 10, 5 } };
    static const bool minorQual[5][4] = { { true, false, false, false }, { true, true, false, false }, { true, false, false, false },
                                          { false, false, true, true }, { true, false, false, true } };
    static const int  majorProgs[5][4] = { { 0, 7, 9, 5 }, { 0, 9, 5, 7 }, { 2, 7, 0, 9 }, { 5, 7, 4, 9 }, { 0, 5, 2, 7 } };
    static const bool majorQual[5][4] = { { false, false, true, false }, { false, true, false, false }, { true, false, false, true },
                                          { false, false, true, true }, { false, false, true, false } };
    const int p = r.rangeInt( 0, 4 );
    for( int i = 0; i < 4; ++i )
    {
        m_prog[i] = m_major ? majorProgs[p][i] : minorProgs[p][i];
        m_minorChord[i] = m_major ? majorQual[p][i] : minorQual[p][i];
    }
    m_variant = r.rangeInt( 0, 2 );
    // Four bars of melody in eighths: a motif, repeated with a varied answer.
    int deg = r.rangeInt( 3, 6 );
    for( int i = 0; i < 16; ++i )
    {
        if( i % 4 != 0 && !r.chance( st.m_density ) )
        {
            m_lead[i] = -1;
            continue;
        }
        deg = std::clamp( deg + r.rangeInt( -2, 2 ), 0, 9 );
        m_lead[i] = deg;
    }
    for( int i = 16; i < 32; ++i ) m_lead[i] = ( i >= 28 ) ? ( i % 2 ? -1 : std::clamp( m_lead[i - 16] + r.rangeInt( -1, 2 ), 0, 9 ) ) : m_lead[i - 16];
    for( int i = 0; i < 16; ++i )
    {
        static const int acidNotes[8] = { 0, 0, 12, 0, 3, 7, 10, 15 };
        m_acid[i] = r.chance( 0.75f ) ? acidNotes[r.rangeInt( 0, 7 )] : -1;
        m_accent[i] = r.chance( 0.3f );
        m_slide[i] = r.chance( 0.2f );
    }
    m_step = -1;
    m_clock = 1e9;
    m_on = true;
    m_pending = false;
    m_snareF.set( m_station == 5 ? 4000.0f : 1900.0f, 0.7f );
    m_hatF.set( 7500.0f, 0.7f );
    m_kickClick.set( 3000.0f, 0.7f );
    m_padF1.set( st.m_pad == PadE::GATED ? 2400.0f : st.m_pad == PadE::STRINGS ? 850.0f : 1100.0f, 0.8f );
    m_arpF1.set( 2600.0f, 0.9f );
    m_leadF1.set( st.m_lead == LeadE::SUPERSAW ? 4800.0f : 3200.0f, 0.8f );
    m_stabF1.set( 3000.0f, 0.7f );
}

void MusicSynth::bassStep( int s16, int root )
{
    const Style &st = Styles[m_station];
    bool         hit = false;
    int          note = root;
    float        len = 0.9f;
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
    {
        static const char *pat = "x..x.xx...x.x.x.";
        static const char *oct = "......u.......u.";
        hit = pat[s16] == 'x';
        note = root + ( oct[s16] == 'u' ? 12 : 0 );
        len = 1.2f;
        break;
    }
    case BassE::BIGBEAT:
    {
        static const char *pat = "x...x..x..x...x.";
        hit = pat[s16] == 'x';
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
    }
    if( !hit ) return;
    m_bassTarget = mtof( static_cast<float>( 33 + note ) );
    m_bassF = m_bassTarget;
    m_bassT = 0.0f;
    const double stepLen = static_cast<double>( SampleRate ) * 60.0 / static_cast<double>( m_bpm ) / 4.0;
    m_bassGate = len * static_cast<float>( stepLen / SampleRate );
}

void MusicSynth::onStep()
{
    const Style       &st = Styles[m_station];
    const DrumPattern &dp = Patterns[static_cast<int>( st.m_drums )];
    const int          s16 = m_step % 16;
    const int          bar = ( m_step / 16 ) % 32;
    const bool         groove = bar >= 4;
    const bool         breakdown = bar >= 24 && bar < 28;
    const bool         build = bar >= 28;
    const bool         kickOn = groove && !breakdown;
    const bool         bassOn = groove && !breakdown;
    const bool         arpOn = ( bar >= 8 && bar < 24 ) || breakdown || build;
    const bool         leadOn = bar >= 12 && bar < 24;
    const bool         snareOn = bar >= 8 && !breakdown;
    const int          ci = bar % 4;
    const int          root = m_key + m_prog[ci];
    const int          third = m_minorChord[ci] ? 3 : 4;

    if( s16 == 0 )
    {
        m_padF[0] = mtof( static_cast<float>( 57 + root ) );
        m_padF[1] = mtof( static_cast<float>( 57 + root + third ) );
        m_padF[2] = mtof( static_cast<float>( 57 + root + 7 ) );
        m_chipChord[0] = mtof( static_cast<float>( 69 + root ) );
        m_chipChord[1] = mtof( static_cast<float>( 69 + root + third ) );
        m_chipChord[2] = mtof( static_cast<float>( 69 + root + 7 ) );
    }
    if( kickOn && vel( dp.m_kick[s16] ) > 0.0f )
    {
        m_kickT = 0.0f;
        m_kickPh = 0.0f;
        m_kickVel = vel( dp.m_kick[s16] );
    }
    if( snareOn && vel( dp.m_snare[s16] ) > 0.0f )
    {
        m_snareT = 0.0f;
        m_snareVel = vel( dp.m_snare[s16] );
    }
    if( build && bar == 31 && s16 % 2 == 0 )
    {
        m_snareT = 0.0f;
        m_snareVel = 0.4f + 0.04f * static_cast<float>( s16 );
    }
    if( groove || st.m_drums == DrumsE::CHIP )
    {
        if( vel( dp.m_open[s16] ) > 0.0f )
        {
            m_hatT = 0.0f;
            m_hatOpen = true;
            m_hatVel = 0.8f;
        }
        else if( vel( dp.m_hat[s16] ) > 0.0f )
        {
            m_hatT = 0.0f;
            m_hatOpen = false;
            m_hatVel = vel( dp.m_hat[s16] );
        }
    }
    if( bassOn ) bassStep( s16, root );

    if( st.m_pad == PadE::STABS && ( groove || breakdown ) )
    {
        static const char *house = "..x...x...x..x..";
        static const char *funk = "x..x..x...x.x...";
        const char        *pat = st.m_drums == DrumsE::FUNK ? funk : house;
        if( pat[s16] == 'x' ) m_stabT = 0.0f;
    }
    if( st.m_arp && arpOn )
    {
        static const int pats[3][8] = { { 0, 1, 2, 3, 2, 1, 2, 3 }, { 0, 2, 1, 3, 0, 2, 3, 2 }, { 3, 2, 1, 0, 1, 2, 3, 2 } };
        const int        tones[4] = { 0, third, 7, 12 };
        m_arpF = mtof( static_cast<float>( 69 + root + tones[pats[m_variant][s16 % 8]] ) );
        m_arpT = 0.0f;
    }
    if( st.m_chipArp && arpOn && s16 % 2 == 0 ) m_arpT = 0.0f;
    if( leadOn && s16 % 2 == 0 )
    {
        const int *scale = m_major ? MajorScale : MinorScale;
        const int  idx = ( ( bar - 12 ) % 4 ) * 8 + s16 / 2;
        const int  note = m_lead[idx % 32];
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

float MusicSynth::renderDrums( float &reverbSend )
{
    const Style &st = Styles[m_station];
    const float  dt = 1.0f / SampleRate;
    float        out = 0.0f;
    reverbSend = 0.0f;
    if( m_kickT < 1.0f )
    {
        if( st.m_drums == DrumsE::CHIP )
        {
            m_kickPh += ( 40.0f + 160.0f * std::exp( -m_kickT * 40.0f ) ) * dt;
            out += ( m_kickPh - std::floor( m_kickPh ) < 0.5f ? 1.0f : -1.0f ) * std::exp( -m_kickT * 14.0f ) * 0.5f * st.m_kick;
        }
        else
        {
            m_kickPh += ( 46.0f + 150.0f * std::exp( -m_kickT * 30.0f ) ) * dt;
            float k = std::sin( Pi2 * m_kickPh ) * std::exp( -m_kickT * ( st.m_drums == DrumsE::DNB ? 7.0f : 5.5f ) );
            if( st.m_drums == DrumsE::BREAKS || st.m_drums == DrumsE::DNB )
                k += m_kickClick.high( m_n.next() ) * std::exp( -m_kickT * 120.0f ) * 0.5f;
            out += k * st.m_kick * m_kickVel;
        }
        m_kickT += dt;
    }
    if( m_snareT < 0.9f )
    {
        float s = 0.0f;
        if( st.m_drums == DrumsE::CHIP )
        {
            s = m_snareF.high( m_n.next() ) * std::exp( -m_snareT * 22.0f );
        }
        else
        {
            const float decay = st.m_drums == DrumsE::SYNTHWAVE ? 9.0f : 15.0f;
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
        out += m_hatF.high( m_n.next() ) * std::exp( -m_hatT * ( m_hatOpen ? 13.0f : 60.0f ) ) * st.m_hat * m_hatVel *
               ( m_hatOpen ? 1.4f : 1.0f );
        m_hatT += dt;
    }
    return out;
}

float MusicSynth::renderBass()
{
    const Style &st = Styles[m_station];
    const float  dt = 1.0f / SampleRate;
    if( m_bassT > 4.0f ) return 0.0f;
    float y = 0.0f;
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

float MusicSynth::renderLead()
{
    const Style &st = Styles[m_station];
    const float  dt = 1.0f / SampleRate;
    const bool   percussive = st.m_lead == LeadE::EPIANO || st.m_lead == LeadE::PLUCK;
    const float  target = m_leadGate ? ( m_leadT < 0.05f ? 1.0f : ( percussive ? 0.9f : 0.65f ) ) : 0.0f;
    m_leadEnv += ( target - m_leadEnv ) * ( target > m_leadEnv ? 0.01f : 0.002f );
    if( m_leadEnv < 1e-4f ) return 0.0f;
    const float t = m_leadT;
    const float vib = t > 0.15f ? 1.0f + 0.004f * std::sin( Pi2 * 5.5f * t ) : 1.0f;
    float       y = 0.0f;
    switch( st.m_lead )
    {
    case LeadE::SQUARE: y = m_leadF1.low( pulse( m_leadPh[0], m_leadF * vib ) ); break;
    case LeadE::SUPERSAW:
        y = m_leadF1.low( ( saw( m_leadPh[0], m_leadF * 0.992f * vib ) + saw( m_leadPh[1], m_leadF * vib ) +
                            saw( m_leadPh[2], m_leadF * 1.009f * vib ) ) * 0.45f );
        break;
    case LeadE::PULSE: y = pulse( m_leadPh[0], m_leadF * ( t > 0.1f ? 1.0f + 0.006f * std::sin( Pi2 * 6.0f * t ) : 1.0f ), st.m_duty ) * 0.7f; break;
    case LeadE::EPIANO:
    {
        const float index = 2.2f * std::exp( -t * 5.0f ) + 0.3f;
        const float mod = std::sin( Pi2 * m_leadPh[1] ) * index;
        m_leadPh[1] += m_leadF / SampleRate;
        if( m_leadPh[1] >= 1.0f ) m_leadPh[1] -= 1.0f;
        y = std::sin( Pi2 * m_leadPh[0] + mod ) * std::exp( -t * 2.2f ) + std::sin( Pi2 * m_leadPh[0] * 4.0f ) * std::exp( -t * 12.0f ) * 0.15f;
        m_leadPh[0] += m_leadF / SampleRate;
        if( m_leadPh[0] >= 1.0f ) m_leadPh[0] -= 1.0f;
        break;
    }
    case LeadE::BRASS:
        m_leadF1.set( 500.0f + 3200.0f * ( 1.0f - std::exp( -t * 14.0f ) ) * ( 0.6f + 0.4f * std::exp( -t * 2.0f ) ), 0.9f );
        y = m_leadF1.low( ( saw( m_leadPh[0], m_leadF * 0.996f * vib ) + saw( m_leadPh[1], m_leadF * 1.004f * vib ) ) * 0.6f );
        break;
    case LeadE::PLUCK:
        m_leadF1.set( 300.0f + 5000.0f * std::exp( -t * 16.0f ), 0.9f );
        y = m_leadF1.low( saw( m_leadPh[0], m_leadF ) ) * std::exp( -t * 4.0f );
        break;
    }
    m_leadT += dt;
    return y * m_leadEnv * st.m_leadLevel;
}

float MusicSynth::renderPad()
{
    const Style &st = Styles[m_station];
    const float  dt = 1.0f / SampleRate;
    float        y = 0.0f;
    if( st.m_pad == PadE::STABS )
    {
        if( m_stabT < 1.0f )
        {
            const float decay = st.m_drums == DrumsE::FUNK ? 7.0f : 5.0f;
            for( int i = 0; i < 3; ++i )
            {
                const float f = m_padF[i] * 2.0f;
                const float mod = std::sin( Pi2 * m_stabPh[i * 2 + 1] ) * ( 1.6f * std::exp( -m_stabT * 8.0f ) + 0.2f );
                y += std::sin( Pi2 * m_stabPh[i * 2] + mod );
                m_stabPh[i * 2] += f / SampleRate;
                m_stabPh[i * 2 + 1] += f / SampleRate;
                if( m_stabPh[i * 2] >= 1.0f ) m_stabPh[i * 2] -= 1.0f;
                if( m_stabPh[i * 2 + 1] >= 1.0f ) m_stabPh[i * 2 + 1] -= 1.0f;
            }
            y *= std::exp( -m_stabT * decay ) * 0.33f;
            m_stabT += dt;
        }
        return y * st.m_padLevel;
    }
    if( st.m_pad == PadE::NONE ) return 0.0f;
    const float attack = st.m_pad == PadE::STRINGS ? 0.000015f : 0.00003f;
    m_padEnv += ( 1.0f - m_padEnv ) * attack;
    for( int i = 0; i < 3; ++i )
    {
        y += saw( m_padPh[i * 2], m_padF[i] * 1.004f );
        y += saw( m_padPh[i * 2 + 1], m_padF[i] * 0.996f );
    }
    y = m_padF1.low( y ) * m_padEnv;
    if( st.m_pad == PadE::GATED )
    {
        static const char *gate = "x.xx.x.xx.x.xx.x";
        const bool         open = gate[( m_step < 0 ? 0 : m_step ) % 16] == 'x';
        m_gateEnv += ( ( open ? 1.0f : 0.0f ) - m_gateEnv ) * ( open ? 0.02f : 0.004f );
        y *= m_gateEnv * 1.4f;
    }
    return y * st.m_padLevel;
}

void MusicSynth::reverb( float in, float &l, float &r )
{
    float outL = 0.0f, outR = 0.0f;
    for( int i = 0; i < 8; ++i )
    {
        std::vector<float> &buf = m_comb[i];
        const float y = buf[m_combI[i]];
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
    const Style &st = Styles[m_station];

    // Sixteenths with swing on the odd ones.
    const double stepLen = static_cast<double>( SampleRate ) * 60.0 / static_cast<double>( m_bpm ) / 4.0;
    const double thisLen = stepLen * ( ( m_step & 1 ) ? 1.0 - m_swing : 1.0 + m_swing );
    m_clock += 1.0;
    if( m_clock >= thisLen )
    {
        m_clock = m_clock >= 1e8 ? 0.0 : m_clock - thisLen;
        ++m_step;
        onStep();
    }

    float       reverbSend = 0.0f;
    const float drums = renderDrums( reverbSend );
    const float duck = 1.0f - st.m_sidechain * std::exp( -m_kickT * 8.0f );
    const float bass = renderBass() * duck;
    const float pad = renderPad() * duck;
    const float lead = renderLead();
    float       arp = 0.0f;
    if( m_arpT < 1.0f )
    {
        if( st.m_chipArp )
        {
            // Tracker arpeggio: the chord's notes cycled every third of a sixteenth.
            const int k = static_cast<int>( m_clock / ( stepLen / 3.0 ) ) % 3;
            arp = pulse( m_arpPh, m_chipChord[k], 0.25f ) * std::exp( -m_arpT * 6.0f ) * 0.8f;
        }
        else
        {
            m_arpF1.set( 300.0f + 4000.0f * std::exp( -m_arpT * 20.0f ), 0.9f );
            arp = m_arpF1.low( saw( m_arpPh, m_arpF ) ) * std::exp( -m_arpT * 9.0f );
        }
        arp *= st.m_arpLevel * duck;
        m_arpT += 1.0f / SampleRate;
    }

    // Ping-pong echo (three sixteenths) and reverb.
    const float  echoIn = ( arp + lead ) * st.m_echo;
    const size_t n = m_delayL.size();
    const size_t len = std::min( n - 1, static_cast<size_t>( stepLen * 3.0 ) );
    const size_t rd = ( m_di + n - len ) % n;
    const float  dl = m_delayL[rd], dr = m_delayR[rd];
    m_delayL[m_di] = echoIn + dr * 0.38f;
    m_delayR[m_di] = dl * 0.38f;
    m_di = ( m_di + 1 ) % n;
    float rl = 0.0f, rr = 0.0f;
    reverb( ( reverbSend + pad * 0.6f + lead * 0.5f + arp * 0.3f ) * st.m_reverb, rl, rr );

    const float dry = drums + bass + pad + lead + arp;
    const float g = m_gain * 0.5f;
    l = ( dry + dl * 0.6f + rl ) * g;
    r = ( dry + dr * 0.6f + rr ) * g;
}

} // namespace sunstreak
