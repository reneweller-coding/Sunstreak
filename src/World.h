/**
 * @file World.h
 * @brief The scenery around the road, baked in 80 m chunks as the car approaches and dropped behind it.
 *
 * A chunk holds two meshes -- road and land, and the props -- in coordinates relative to the chunk's first
 * segment, so they stay precise however far the run goes. Solid props near the road become colliders.
 *
 * After the first chunks of a track a worker thread bakes the triangles on its own copy of the track (the generator
 * is deterministic, so the copy yields the same road); the main thread only uploads what is finished.
 *
 * @author Rene Weller
 */
#pragma once

#include "Models.h"
#include "Season.h"
#include "Track.h"

#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace sunstreak
{

constexpr int    ChunkSegments = 16;                            ///< segments per chunk
constexpr double ChunkLength = ChunkSegments * SegmentLength;   ///< metres per chunk
constexpr double ViewDistance = 1150.0;                         ///< how far ahead chunks exist
constexpr double KeepBehind = 320.0;                            ///< how far behind chunks stay (looking back)
constexpr double LodDistance = 320.0;                           ///< chunks further ahead show their scenery from afar
constexpr float  LodLateral = 130.0f;                           ///< scenery further from the road is always far

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
    long                  m_index = 0;      ///< chunk number (first segment / ChunkSegments)
    Vec3d                 m_origin;         ///< world position of the first segment's centre
    Mesh                  m_ground{};       ///< road and land
    Mesh                  m_props{};        ///< everything standing that belongs to the road (signs, gantries, bridges)
    Mesh                  m_scenery{};      ///< the scattered props: trees, rocks, houses, towers
    Mesh                  m_sceneryFar{};   ///< the same as seen from afar (LOD)
    Mesh                  m_glass{};        ///< see-through panes (undersea tunnels), drawn after everything solid
    std::vector<Collider> m_colliders;      ///< solid props near the road
};

/** @brief A chunk's triangles before they reach the GPU. */
struct BakedChunk
{
    long                  m_index = 0;    ///< chunk number
    Vec3d                 m_origin;       ///< world position of the first segment's centre
    MeshBuilder           m_ground;       ///< road and land
    MeshBuilder           m_props;        ///< everything standing that belongs to the road
    MeshBuilder           m_scenery;      ///< the scattered props
    MeshBuilder           m_sceneryFar;   ///< the same from afar
    MeshBuilder           m_glass;        ///< see-through panes
    std::vector<Collider> m_colliders;    ///< solid props near the road
    float                 m_ms = 0.0f;    ///< how long the baking took
};

/**
 * @brief Whether a chunk is near enough to show its scenery up close.
 * @param[in] c  the chunk
 * @param[in] s  where the car is along the road
 * @return true up close, false from afar
 */
inline bool upClose( const Chunk &c, double s )
{
    return static_cast<double>( c.m_index ) * ChunkLength < s + LodDistance;
}

/**
 * @brief Where one of a stage's landmarks stands (a prop rule with PropRule::m_perStage).
 * @param[in]  seed   the track's seed
 * @param[in]  rule   the rule's index in its biome
 * @param[in]  k      which of the rule's spots
 * @param[in]  stage  the stage
 * @param[out] right  whether it stands on the right (before the water decides)
 * @return the segment within the stage
 */
long landmarkSpot( uint64_t seed, size_t rule, int k, long stage, bool &right );

/** @brief The chunks around the car. */
class World
{
public:
    World() = default;
    World( const World & ) = delete;
    World &operator=( const World & ) = delete;

    /** @brief Stops the worker. */
    ~World();

    /**
     * @brief Connects the world to the models.
     * @param[in] models  the prop prototypes (must outlive the world)
     */
    void init( const Models *models ) { m_models = models; }

    /** @brief Drops every chunk and lets the worker forget its track (call before a new track). */
    void clear();

    /**
     * @brief Builds chunks ahead and drops those behind. The first call after clear() bakes everything wanted at once
     *        (up to maxBuilds); later calls hand the baking to the worker and upload at most maxBuilds finished chunks.
     * @param[in,out] track      the track (generated further when needed)
     * @param[in]     s          the camera's distance along the road
     * @param[in]     maxBuilds  chunks to upload at most this call
     */
    void update( Track &track, double s, int maxBuilds );

    /**
     * @brief How far behind a point chunks are kept and drawn: in a pass in hairpins the legs below lie beside the road,
     *        so they stay (up to 1.5 km back) while it lasts and a little after.
     * @param[in] track  the track
     * @param[in] s      the distance along the road
     * @return metres behind
     */
    static double keptBehind( const Track &track, double s );

    /**
     * @brief The chunks held, nearest first.
     * @return the chunks
     */
    const std::deque<Chunk> &chunks() const { return m_chunks; }

    float  m_buildMs = 0.0f;       ///< milliseconds the main thread spent on chunks in the last update
    float  m_buildPeakMs = 0.0f;   ///< the slowest update lately (decays)
    float  m_buildMaxMs = 0.0f;    ///< the slowest single chunk
    double m_buildTotalMs = 0.0;   ///< all chunk builds together (worker time)
    int    m_buildCount = 0;       ///< chunks built
    int    m_syncBuilds = 0;       ///< chunks the main thread had to bake itself (the worker fell behind)

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
     * @brief Bakes one chunk (thread-safe: reads only the models and the given track).
     * @param[in,out] track  the track (generated further when needed)
     * @param[in,out] c      the chunk (m_index set by the caller)
     */
    void buildChunk( Track &track, BakedChunk &c ) const;

public:
    SeasonClock
        m_seasons;   ///< how the seasons run (set while the worker stands still: before building, after clear())

private:
    /**
     * @brief Uploads a baked chunk and appends it.
     * @param[in,out] baked  the triangles (moved from)
     */
    void upload( BakedChunk &baked );

    /** @brief The worker: bakes the requested chunks on its own track. */
    void work();

    const Models     *m_models = nullptr;   ///< prop prototypes
    std::deque<Chunk> m_chunks;             ///< ordered by index

    std::thread                m_thread;           ///< the worker
    std::mutex                 m_mutex;            ///< guards everything below
    std::condition_variable    m_wake;             ///< jobs arrived, or quit
    std::condition_variable    m_idle;             ///< the worker finished a chunk
    std::deque<long>           m_jobs;             ///< chunks to bake
    std::deque<BakedChunk>     m_done;             ///< baked, waiting for the upload
    std::unique_ptr<Track>     m_workerTrack;      ///< the worker's copy of the track (none: bake on the main thread)
    bool                       m_busy = false;     ///< the worker is baking
    bool                       m_quit = false;     ///< the worker should end
    uint64_t                   m_generation = 0;   ///< bumped by clear(): older results are dropped
    long                       m_requested = -1;   ///< the last chunk asked for
    std::map<long, BakedChunk> m_ready;            ///< baked chunks that arrived before their predecessors
};

}   // namespace sunstreak
