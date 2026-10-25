/**
 * @file Profiler.h
 * @brief GPU time per render pass, read from OpenGL timestamp queries a few frames late (so nothing waits for them).
 * @author Rene Weller
 */
#pragma once

namespace sunstreak
{

/** @brief Timestamps between the render passes of a frame, averaged. */
class GpuTimer
{
public:
    /** @brief The marks of a frame, in order; a pass's time runs from the mark before to its own. */
    enum PassE
    {
        START,     ///< the frame's 3D picture begins
        SHADOWS,   ///< the sun's shadow map
        SCENE,     ///< sky, world, vehicles, particles
        MIRROR,    ///< the reflections
        BLOOM,     ///< bright pass and blurs
        POST,      ///< composite and FXAA
        HUD,       ///< the 2D layer on top
        MARKS      ///< number of marks
    };

    /** @brief Fetches the query functions (needs the GL context; without them the timer stays off). */
    void init();

    /**
     * @brief Sends what is batched and stamps the time.
     * @param[in] at  the mark
     */
    void mark( PassE at );

    /** @brief Closes the frame and reads the one stamped a few frames ago. */
    void endFrame();

    /**
     * @brief A pass's time, smoothed.
     * @param[in] pass  the pass (the mark it ends with)
     * @return milliseconds
     */
    float ms( PassE pass ) const { return m_ms[pass]; }

    /**
     * @brief A pass's average since resetTotals().
     * @param[in] pass  the pass
     * @return milliseconds
     */
    double average( PassE pass ) const { return m_count ? m_sum[pass] / m_count : 0.0; }

    /** @brief Starts the averages anew. */
    void resetTotals();

    bool m_ok = false;   ///< the queries work

private:
    static constexpr int Ring = 4;   ///< frames in flight before a result is read

    unsigned m_query[Ring][MARKS]{};   ///< the query objects
    bool     m_set[Ring][MARKS]{};     ///< stamped this frame
    int      m_frame = 0;              ///< frames closed
    float    m_ms[MARKS]{};            ///< smoothed pass times
    double   m_sum[MARKS]{};           ///< summed pass times
    int      m_count = 0;              ///< frames summed
};

} // namespace sunstreak
