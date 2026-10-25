/**
 * @file Track.h
 * @brief The endless road: a stream of short segments generated ahead of the car from a seed.
 *
 * The track is a centre line made of 5 m segments, each with a curvature, a grade and a bank. The generator plans
 * features (straights, bends, S-bends, crests, roadworks) the way Lotus III's RECS did from its parameters: how curvy,
 * how sharp, how hilly, how many obstacles. Every 15 km a new stage begins with its own biome; checkpoints come every
 * 2.5 km. The heading is kept within +-80 degrees of north, so the road never folds back onto itself.
 *
 * Distances along the road (s) are doubles; the lateral offset x is positive to the right.
 *
 * @author Rene Weller
 */
#pragma once

#include "Biome.h"
#include "MathUtil.h"
#include "Rng.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace sunstreak
{

constexpr float  SegmentLength = 5.0f;         ///< metres per segment
constexpr float  LaneWidth = 3.7f;             ///< metres per lane
constexpr float  RumbleWidth = 0.9f;           ///< the kerb outside the road edge
constexpr float  ShoulderWidth = 2.2f;         ///< gravel beside the kerb
constexpr double StageLength = 15000.0;        ///< one biome
constexpr double CheckpointSpacing = 2500.0;   ///< metres between checkpoints
constexpr double BlendLength = 500.0;          ///< the crossfade between two biomes
constexpr double StartLine = 40.0;             ///< where the start gantry stands

/** @brief Flags of a segment. */
enum SegmentFlagE : uint16_t
{
    SEG_CHECKPOINT = 1 << 0,   ///< a checkpoint gate stands here
    SEG_ROADWORKS = 1 << 1,    ///< lanes in m_blockedLanes are closed
    SEG_CHEVRON_L = 1 << 2,    ///< warning signs on the left (a right-hand bend)
    SEG_CHEVRON_R = 1 << 3,    ///< warning signs on the right (a left-hand bend)
    SEG_START = 1 << 4,        ///< the start gantry
    SEG_JUMP = 1 << 5,         ///< a crest that throws the car (warning sign before it)
    SEG_RAIL_L = 1 << 6,       ///< guard rail on the left
    SEG_RAIL_R = 1 << 7,       ///< guard rail on the right
    SEG_PUDDLE = 1 << 8,       ///< a puddle on the road
    SEG_NO_DASH = 1 << 9,      ///< no lane dashes (the lane count changes here)
    SEG_FINISH = 1 << 10,      ///< the finish line of a race
    SEG_TUNNEL = 1 << 11,      ///< inside a tunnel
    SEG_BRIDGE = 1 << 12,      ///< on a bridge over a valley
    SEG_FORK = 1 << 13,        ///< near a fork of the world tour: flat land, no tunnels, bridges, crests or roadworks
    SEG_MEDIAN = 1 << 14,      ///< a barrier down the middle: two carriageways to two places
    SEG_SUMMIT = 1 << 15,      ///< the top of a pass (its board stands here)
};

constexpr double ForkAt = StageLength - 2600.0;   ///< where in its stage a fork's carriageways part (the choice; the border is 2.6 km on)
constexpr double ForkWiden = 300.0;     ///< the road widens to two carriageways before it
constexpr double ForkMedian = 1300.0;   ///< then they run side by side, beyond the sight of the choice
constexpr double ForkPeel = 300.0;      ///< then the other one turns away and the road narrows again

constexpr double CrossingQuay = 350.0;  ///< the quay of a ferry or a car train stands this far before its sea (and after)
constexpr double CrossingReach = 700.0; ///< how far before and after its sea a crossing shapes the road
constexpr float  CrossingTime = 9.0f;   ///< seconds the crossing takes (the clock stands still)
constexpr float  SeaDipDepth = 12.0f;   ///< how far the road sinks below the land to an undersea tunnel
constexpr double SeaRamp = 320.0;       ///< the ramp down to an undersea tunnel's portal, and up from the other

/** @brief A sea crossed aboard a ferry or a car train: the car drives aboard at the quay and off at the far one. */
struct Crossing
{
    CrossingE m_type = CrossingE::NONE;   ///< none, ferry or car train
    BiomeE    m_sea{};                    ///< the sea crossed
    double    m_board = 0.0;              ///< where the car drives aboard (before the sea's stage)
    double    m_land = 0.0;               ///< where it drives off (after it)
    double    m_length = 0.0;             ///< the vessel's length
};

/** @brief A fork of the world tour: the road splits, each carriageway towards another country or state. */
struct Fork
{
    bool   m_exists = false;            ///< this stage has one
    double m_at = 0.0;                  ///< where the barrier between the carriageways begins (absolute)
    BiomeE m_left = BiomeE::MEADOW;     ///< where the left carriageway leads
    BiomeE m_right = BiomeE::MEADOW;    ///< where the right one leads
    int    m_chosen = 0;                ///< -1 left, +1 right, 0 not yet
};

/** @brief What the Track Lab (and a race) asks of the generator; -1 leaves a value to the biomes. */
constexpr int SceneryMixed = -1;   ///< TrackParams::m_biome: every stage another biome, all of them shuffled
constexpr int SceneryWorld = -2;   ///< TrackParams::m_biome: the world tour, stage after stage along the route

/** @brief The settings of a track the Track Lab can change (-1: as the biomes and the stages say). */
struct TrackParams
{
    int    m_curvy = -1;              ///< 0..9 how often the road bends
    int    m_sharp = -1;              ///< 0..9 how tight
    int    m_hilly = -1;              ///< 0..9 how much it climbs
    int    m_traffic = -1;            ///< 0..9 traffic density
    int    m_obstacles = -1;          ///< 0..9 roadworks and puddles
    int    m_biome = -1;              ///< one biome all the way, or SceneryMixed / SceneryWorld
    double m_length = 0.0;            ///< the finish (m), 0 endless
    double m_startLine = StartLine;   ///< where the start gantry stands
    std::vector<uint16_t> m_stages;   ///< a rally's places, stage by stage (empty: as m_biome says; not in track codes)
};

/** @brief One 5 m piece of road. */
struct Segment
{
    Vec3d    m_pos;               ///< centre line at the segment's start
    float    m_heading = 0;       ///< yaw at the start (0 north, positive turned right)
    float    m_curve = 0;         ///< curvature over the segment (1/m, positive bends right)
    float    m_grade = 0;         ///< rise per metre over the segment
    float    m_bank = 0;          ///< roll at the start (positive: the right side lower)
    float    m_halfWidth = 0;     ///< half the paved width at the start (lanes only)
    uint8_t  m_lanes = 3;         ///< lanes
    uint16_t m_biome = 0;         ///< the biome blended from
    uint16_t m_biomeTo = 0;       ///< the biome blended to
    uint8_t  m_blockedLanes = 0;  ///< bit per closed lane (roadworks)
    float    m_blend = 0;         ///< 0..1 between m_biome and m_biomeTo
    uint16_t m_flags = 0;         ///< SegmentFlagE
    int      m_stage = 0;         ///< the stage this segment belongs to
    float    m_flat = 0.0f;       ///< 0..1 how flat the land beside it lies (a fork's surroundings)
    float    m_trench = 0.0f;     ///< metres the land stands over the road in the cutting to an undersea tunnel
    float    m_reachL = 0.0f;     ///< in a serpentine: how far left of the centre line the land is this segment's (0: all)
    float    m_reachR = 0.0f;     ///< and right (the rest belongs to the legs above and below)
    uint8_t  m_hairpin = 0;       ///< the number on the board of the hairpin that begins here (0: none)
    int8_t   m_gallery = 0;       ///< under an avalanche gallery: the side its back wall stands on (-1 left, 1 right; 0 none)
    float    m_runway = 0.0f;     ///< beside a landing plane's runway: how flat its land lies, signed by its side (-1..1)
};

/**
 * @brief A pass climbed in hairpins: the land beside all its legs is one mountainside, rising along the climb to a
 *        ridge at the summit and falling beyond, so that it lies as high whichever leg it is seen from.
 */
struct Serpentine
{
    long   m_first = 0;      ///< its first segment
    long   m_last = 0;       ///< and its last
    Vec3d  m_origin;         ///< where it begins
    float  m_axisX = 0.0f;   ///< the way it climbs (horizontal unit vector)
    float  m_axisZ = 1.0f;   ///< (north part)
    float  m_up = 0.0f;      ///< the mountainside's rise per metre along the axis up to the summit
    float  m_down = 0.0f;    ///< its fall per metre beyond
    float  m_summit = 0.0f;  ///< the summit's distance along the axis
    float  m_top = 0.0f;     ///< and its height over the origin
    float  m_end = 0.0f;     ///< the last segment's distance along the axis

    /**
     * @brief The mountainside's height at a point.
     * @param[in] x  east
     * @param[in] z  north
     * @return its height (world)
     */
    double height( double x, double z ) const
    {
        const float d = std::clamp( static_cast<float>( ( x - m_origin.m_x ) * m_axisX + ( z - m_origin.m_z ) * m_axisZ ), -120.0f, m_end + 120.0f );
        return m_origin.m_y + static_cast<double>( std::min( m_up * d, m_top - m_down * ( d - m_summit ) ) );
    }
};

/** @brief The road at a point: position and orientation, interpolated. */
struct TrackFrame
{
    Vec3d   m_pos;              ///< centre line
    float   m_heading = 0;      ///< yaw
    float   m_grade = 0;        ///< rise per metre
    float   m_bank = 0;         ///< roll
    float   m_curve = 0;        ///< curvature
    float   m_halfWidth = 0;    ///< half the paved width
    Vector3 m_forward{};        ///< along the road, with the grade
    Vector3 m_right{};          ///< horizontal right
    Vector3 m_up{};             ///< road normal, with the bank
};

/** @brief The biomes on both sides of a crossfade. */
struct BiomeMix
{
    BiomeE m_from = BiomeE::MEADOW;   ///< blended from
    BiomeE m_to = BiomeE::MEADOW;     ///< blended to
    float  m_t = 0.0f;                ///< 0..1
};

/** @brief The generated road. */
class Track
{
public:
    /**
     * @brief Starts a track.
     * @param[in] seed        the seed (equal seeds, equal tracks)
     * @param[in] params      overrides (Track Lab, races)
     * @param[in] startBiome  the first stage's biome, -1 to let the seed choose
     */
    explicit Track( uint64_t seed, const TrackParams &params = TrackParams(), int startBiome = -1 );

    /**
     * @brief What the track was asked for.
     * @return its parameters
     */
    const TrackParams &params() const { return m_params; }

    /**
     * @brief Generates segments until s is covered (plus the next segment for interpolation).
     * @param[in] s  the distance to reach
     */
    void generateTo( double s );

    /**
     * @brief Forgets segments behind a distance.
     * @param[in] s  segments ending before it are dropped
     */
    void discardBefore( double s );

    /**
     * @brief The first segment still held.
     * @return its absolute index
     */
    long firstIndex() const { return m_first; }

    /**
     * @brief One past the last generated segment.
     * @return its absolute index
     */
    long endIndex() const { return m_first + static_cast<long>( m_segments.size() ); }

    /**
     * @brief A segment by absolute index (clamped into what is held).
     * @param[in] index  the index
     * @return the segment
     */
    const Segment &segment( long index ) const;

    /**
     * @brief The serpentine a segment belongs to.
     * @param[in] index  the segment
     * @return it, or nullptr
     */
    const Serpentine *serpentineAt( long index ) const;

    /**
     * @brief The road at a distance.
     * @param[in] s  the distance along the road
     * @return position and orientation
     */
    TrackFrame frameAt( double s ) const;

    /**
     * @brief A point on or above the road surface.
     * @param[in] s        the distance along the road
     * @param[in] lateral  metres to the right of the centre line
     * @param[in] height   metres above the surface
     * @return the world point
     */
    Vec3d pointAt( double s, float lateral, float height ) const;

    /**
     * @brief The road surface's height (bank included) at a point.
     * @param[in] s        the distance
     * @param[in] lateral  metres to the right
     * @return the world height
     */
    double surfaceHeight( double s, float lateral ) const;

    /**
     * @brief Which biomes are blended at a distance.
     * @param[in] s  the distance
     * @return the blend
     */
    BiomeMix biomeMix( double s ) const;

    /**
     * @brief The biome of a stage.
     * @param[in] stage  the stage number (0 first)
     * @return its biome
     */
    BiomeE stageBiome( int stage ) const;

    /**
     * @brief The world tour's fork in a stage (copies of the track share the choices).
     * @param[in] stage  the stage
     * @return the fork (m_exists false outside the world tour and where the route goes on alone)
     */
    Fork fork( int stage ) const;

    /**
     * @brief Takes one carriageway of a fork; the stages after it follow.
     * @param[in] stage  the stage of the fork
     * @param[in] side   -1 left, +1 right
     */
    void choose( int stage, int side );

    /**
     * @brief Whether a distance lies in a fork's zone (with a margin).
     * @param[in] s       the distance
     * @param[in] margin  metres added before and after
     * @return true if so
     */
    bool inForkZone( double s, double margin ) const;

    /**
     * @brief How far the road has sunk below its own line for an undersea tunnel.
     * @param[in]  s       distance along the road
     * @param[out] trench  the part of it in a cutting on land (0 on the sea floor)
     * @return metres (0..SeaDipDepth)
     */
    float seaDip( double s, float *trench = nullptr ) const;

    /**
     * @brief The ferry or car train near a distance: the stage it lies in or one of its neighbours crosses a sea aboard.
     * @param[in] s  distance along the road
     * @return the crossing (m_type NONE: none near)
     */
    Crossing crossingAt( double s ) const;

    /**
     * @brief The choices made at the world tour's forks so far.
     * @return per stage after the detour: -1 left, +1 right, 0 none
     */
    std::vector<int8_t> tourChoices() const;

    /**
     * @brief Takes over the choices of another drive of the same tour (a replay).
     * @param[in] choices  as tourChoices() gave them
     */
    void setTourChoices( const std::vector<int8_t> &choices );

    /**
     * @brief The track's seed.
     * @return the seed
     */
    uint64_t seed() const { return m_seed; }

    /**
     * @brief The difficulty of the road at a distance.
     * @param[in] s  the distance
     * @return 0 (the start) rising to 1 (35 km on)
     */
    static float difficulty( double s );

private:
    /**
     * @brief Whether a distance lies on or near the ramps of an undersea tunnel (no tunnels, bridges or jumps there).
     * @param[in] s       distance along the road
     * @param[in] margin  metres to keep clear beyond the ramps
     * @return true near a portal
     */
    bool nearSeaPortal( double s, double margin ) const;

    /** @brief Plans the next feature into the pending queues: its shape, then tunnels and bridges. */
    void planFeature();

    /** @brief Plans the next feature's shape (straight, bend, S-bend, crest, roadworks). */
    void planShape();

    /**
     * @brief Plans a stage's pass in hairpins, if it has one and this is where it begins: the climb, the summit, the
     *        descent, and how far each segment's land reaches between the legs.
     * @param[in] s  where the next segment begins
     * @return true if planned
     */
    bool planSerpentine( double s );

    /**
     * @brief Plans a humped bridge over the sea, now and then where the place has them.
     * @param[in] s  where the next segment begins
     * @return true if planned
     */
    bool planHump( double s );

    /**
     * @brief Turns a planned feature into a tunnel or a bridge now and then.
     * @param[in] from  the first pending segment of the feature
     */
    void decorate( size_t from );

    /** @brief Appends one segment from the pending queues. */
    void appendSegment();

    /**
     * @brief The rolling-hill grade at a distance.
     * @param[in] s          the distance
     * @param[in] hilliness  0..1
     * @return the grade
     */
    float hillGrade( double s, float hilliness ) const;

    /**
     * @brief Blended road style at a distance.
     * @param[in] s       the distance
     * @param[out] curvy  how curvy
     * @param[out] sharp  how sharp
     * @param[out] hilly  how hilly
     */
    void styleAt( double s, float &curvy, float &sharp, float &hilly ) const;

    uint64_t                     m_seed;                ///< the seed
    TrackParams                  m_params;              ///< overrides
    Rng                          m_rng;                 ///< the feature planner's generator
    mutable std::vector<uint16_t> m_stageBiomes;        ///< biome per stage, extended on demand (world tour: a detour first)
    int                          m_routeStart = 0;      ///< the world tour's first place (index in worldRoute())
    std::deque<Segment>          m_segments;            ///< the segments held
    long                         m_first = 0;           ///< absolute index of m_segments.front()
    std::deque<float>            m_pendingCurve;        ///< planned curvature per future segment
    std::deque<float>            m_pendingGrade;        ///< planned extra grade (crests)
    std::deque<uint16_t>         m_pendingFlags;        ///< planned flags
    std::deque<uint8_t>          m_pendingBlocked;      ///< planned closed lanes

    /** @brief What a planned segment of a serpentine carries besides its shape. */
    struct Bend
    {
        bool    m_fixed = false;   ///< its grade is the whole grade (no rolling hills, no pull back to sea level)
        uint8_t m_hairpin = 0;     ///< Segment::m_hairpin
        float   m_reachL = 0.0f;   ///< Segment::m_reachL
        float   m_reachR = 0.0f;   ///< Segment::m_reachR
        int8_t  m_gallery = 0;     ///< Segment::m_gallery
    };
    std::deque<Bend>             m_pendingBend;         ///< planned serpentine details
    std::vector<Serpentine>      m_serpentines;         ///< the serpentines planned and still held
    int                          m_serpentineStage = -1; ///< the last stage whose pass was decided
    int                          m_humpStage = -1;      ///< the stage of the humps counted in m_humpsLeft
    int                          m_humpsLeft = 0;       ///< humped bridges still to come in it
    Vec3d                        m_genPos;              ///< where the next segment starts
    float                        m_genHeading = 0.0f;   ///< its heading
    float                        m_hillPhase[3]{};      ///< phases of the rolling hills
    long                         m_lastJump = -1000;    ///< segment of the last crest
    long                         m_lastWorks = -1000;   ///< segment of the last roadworks
    long                         m_lastSpecial = -1000; ///< segment of the last tunnel or bridge

    /** @brief The world tour's way: the route position of every stage and the choices at its forks (shared by copies). */
    struct Tour
    {
        std::mutex          m_mutex;    ///< the main thread chooses, the world's worker reads
        std::vector<int>    m_pos;      ///< route position per stage after the detour (as far as known)
        std::vector<int>    m_sea;      ///< per stage: the sea crossed on the way to m_pos (-1: a place on land)
        std::vector<int8_t> m_choice;   ///< per stage: -1 left, +1 right, 0 not yet
    };
    std::shared_ptr<Tour> m_tour;       ///< the world tour's way (null in other modes)

    /**
     * @brief The route position of a world tour stage (after the detour), following the choices made.
     * @param[in] stage  the stage counted after the detour
     * @return the position in worldRoute()
     */
    int tourPos( int stage ) const;
};

/**
 * @brief Where a segment's puddle lies (if it has one).
 * @param[in] track  the track (its seed)
 * @param[in] index  the segment
 * @return the puddle's lateral centre
 */
float puddleLateral( const Track &track, long index );

/**
 * @brief The centre of a lane.
 * @param[in] lane       the lane (0 leftmost)
 * @param[in] lanes      lanes on the road
 * @param[in] halfWidth  half the paved width
 * @return its lateral offset
 */
inline float laneCenter( int lane, int lanes, float halfWidth )
{
    const float w = 2.0f * halfWidth / static_cast<float>( lanes );
    return -halfWidth + ( static_cast<float>( lane ) + 0.5f ) * w;
}

/**
 * @brief The lane a lateral offset lies in.
 * @param[in] x          the offset
 * @param[in] lanes      lanes on the road
 * @param[in] halfWidth  half the paved width
 * @return the lane, clamped to the road
 */
inline int laneOf( float x, int lanes, float halfWidth )
{
    const float w = 2.0f * halfWidth / static_cast<float>( lanes );
    return std::clamp( static_cast<int>( std::floor( ( x + halfWidth ) / w ) ), 0, lanes - 1 );
}

} // namespace sunstreak
