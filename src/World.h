/**
 * @file World.h
 * @brief The scenery around the road, baked in 80 m chunks as the car approaches and dropped behind it.
 *
 * A chunk holds two meshes -- road and land, and the props -- in coordinates relative to the chunk's first
 * segment, so they stay precise however far the run goes. Solid props near the road become colliders.
 *
 * @author reneweller
 */
#pragma once

#include "Models.h"
#include "Track.h"

#include <deque>
#include <vector>

namespace sunstreak
{

constexpr int    ChunkSegments = 16;                                ///< segments per chunk
constexpr double ChunkLength = ChunkSegments * SegmentLength;       ///< metres per chunk
constexpr double ViewDistance = 1150.0;                             ///< how far ahead chunks exist

/** @brief Something solid beside the road. */
struct Collider
{
    double m_s = 0;        ///< distance along the road
    float  m_x = 0;        ///< lateral offset
    float  m_radius = 0;   ///< size
};

/** @brief One baked piece of the world. */
struct Chunk
{
    long                  m_index = 0;   ///< chunk number (first segment / ChunkSegments)
    Vec3d                 m_origin;      ///< world position of the first segment's centre
    Mesh                  m_ground{};    ///< road and land
    Mesh                  m_props{};     ///< everything standing
    std::vector<Collider> m_colliders;   ///< solid props near the road
};

/** @brief The chunks around the car. */
class World
{
public:
    /**
     * @brief Connects the world to the models.
     * @param[in] models  the prop prototypes (must outlive the world)
     */
    void init( const Models *models ) { m_models = models; }

    /** @brief Drops every chunk. */
    void clear();

    /**
     * @brief Builds chunks ahead and drops those behind.
     * @param[in,out] track      the track (generated further when needed)
     * @param[in]     s          the camera's distance along the road
     * @param[in]     maxBuilds  chunks to build at most this call
     */
    void update( Track &track, double s, int maxBuilds );

    /**
     * @brief The chunks held, nearest first.
     * @return the chunks
     */
    const std::deque<Chunk> &chunks() const { return m_chunks; }

    float  m_buildMs = 0.0f;        ///< milliseconds spent building chunks in the last update
    float  m_buildPeakMs = 0.0f;    ///< the slowest update lately (decays)
    float  m_buildMaxMs = 0.0f;     ///< the slowest single chunk
    double m_buildTotalMs = 0.0;    ///< all chunk builds together
    int    m_buildCount = 0;        ///< chunks built

    /**
     * @brief The colliders near a distance.
     * @param[in]  s      the distance
     * @param[in]  range  metres before and after
     * @param[out] out    pointers to the colliders found (appended)
     */
    void collidersNear( double s, double range, std::vector<const Collider *> &out ) const;

    /**
     * @brief The terrain height beside the road, relative to the road's edge.
     * @param[in] track    the track
     * @param[in] index    the segment
     * @param[in] lateral  signed metres from the centre line
     * @param[in] terrain  the land's parameters there
     * @param[in] edge     the distance from the centre to the shoulder's outer edge
     * @return metres above the road's height at that segment
     */
    static float terrainHeight( const Track &track, long index, float lateral, const Terrain &terrain, float edge );

private:
    /**
     * @brief Bakes one chunk.
     * @param[in,out] track  the track
     * @param[out]    c      the chunk
     */
    void buildChunk( Track &track, Chunk &c );

    const Models     *m_models = nullptr;   ///< prop prototypes
    std::deque<Chunk> m_chunks;             ///< ordered by index
};

} // namespace sunstreak
