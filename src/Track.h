/**
 * @file Track.h
 * @brief The endless road: a stream of short segments generated ahead of the car from a seed.
 *
 * The track is a centre line made of 5 m segments, each with a curvature, a grade and a bank. The generator plans
 * features (straights, bends, S-bends, crests, roadworks) the way Lotus III's RECS did from its parameters: how curvy,
 * how sharp, how hilly, how many obstacles. Every 5 km a new stage begins with its own biome; checkpoints come every
 * 2.5 km. The heading is kept within +-80 degrees of north, so the road never folds back onto itself.
 *
 * Distances along the road (s) are doubles; the lateral offset x is positive to the right.
 *
 * @author reneweller
 */
#pragma once

#include "Biome.h"
#include "MathUtil.h"
#include "Rng.h"

#include <cstdint>
#include <deque>
#include <vector>

namespace sunstreak
{

constexpr float  SegmentLength = 5.0f;         ///< metres per segment
constexpr float  LaneWidth = 3.7f;             ///< metres per lane
constexpr float  RumbleWidth = 0.9f;           ///< the kerb outside the road edge
constexpr float  ShoulderWidth = 2.2f;         ///< gravel beside the kerb
constexpr double StageLength = 5000.0;         ///< one biome
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
    uint8_t  m_biome = 0;         ///< the biome blended from
    uint8_t  m_biomeTo = 0;       ///< the biome blended to
    uint8_t  m_blockedLanes = 0;  ///< bit per closed lane (roadworks)
    float    m_blend = 0;         ///< 0..1 between m_biome and m_biomeTo
    uint16_t m_flags = 0;         ///< SegmentFlagE
    int      m_stage = 0;         ///< the stage this segment belongs to
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
     * @param[in] startBiome  the first stage's biome, -1 to let the seed choose
     */
    explicit Track( uint64_t seed, int startBiome = -1 );

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
     * @brief The track's seed.
     * @return the seed
     */
    uint64_t seed() const { return m_seed; }

    /**
     * @brief The difficulty of a stage.
     * @param[in] stage  the stage number
     * @return 0 (first stage) rising to 1
     */
    static float difficulty( int stage );

private:
    /** @brief Plans the next feature into the pending queues. */
    void planFeature();

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
    Rng                          m_rng;                 ///< the feature planner's generator
    mutable std::vector<uint8_t> m_stageBiomes;         ///< biome per stage, extended on demand
    std::deque<Segment>          m_segments;            ///< the segments held
    long                         m_first = 0;           ///< absolute index of m_segments.front()
    std::deque<float>            m_pendingCurve;        ///< planned curvature per future segment
    std::deque<float>            m_pendingGrade;        ///< planned extra grade (crests)
    std::deque<uint16_t>         m_pendingFlags;        ///< planned flags
    std::deque<uint8_t>          m_pendingBlocked;      ///< planned closed lanes
    Vec3d                        m_genPos;              ///< where the next segment starts
    float                        m_genHeading = 0.0f;   ///< its heading
    float                        m_hillPhase[3]{};      ///< phases of the rolling hills
    long                         m_lastJump = -1000;    ///< segment of the last crest
    long                         m_lastWorks = -1000;   ///< segment of the last roadworks
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
