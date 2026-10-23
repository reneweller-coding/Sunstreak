/**
 * @file Records.h
 * @brief What the game remembers: track codes, the top-ten table, ghosts of the best runs, the kilometres driven and
 *        what they unlock, the biomes discovered.
 *
 * A track code holds everything the Track Lab sets (seed, curves, sharpness, hills, traffic, obstacles, scenery,
 * length, race or time attack) in twelve characters of Crockford's base 32: XXXX-XXXX-XXXX.
 *
 * @author reneweller
 */
#pragma once

#include "Car.h"
#include "Track.h"

#include <bitset>
#include <cstdint>
#include <string>
#include <vector>

namespace sunstreak
{

/** @brief A track as the Track Lab defines it. */
struct TrackSetup
{
    uint64_t    m_seed = 1;       ///< 29 bits used
    TrackParams m_params;         ///< overrides (-1 automatic)
    bool        m_race = false;   ///< against rivals instead of the clock
};

/**
 * @brief A track setup as a code.
 * @param[in] setup  the setup
 * @return "XXXX-XXXX-XXXX"
 */
std::string encodeTrack( const TrackSetup &setup );

/**
 * @brief Reads a code (dashes, spaces and case ignored; O, I and L read as 0, 1, 1).
 * @param[in]  code   the text
 * @param[out] setup  the setup if valid
 * @return true if the code is valid
 */
bool decodeTrack( const std::string &code, TrackSetup &setup );

/** @brief A line of the top-ten table. */
struct ScoreEntry
{
    std::string m_initials;        ///< three letters
    int64_t     m_score = 0;       ///< points
    double      m_distance = 0.0;  ///< metres
    std::string m_code;            ///< the track
};

/** @brief The game's long memory. */
struct Records
{
    std::vector<ScoreEntry> m_top;              ///< endless top ten, best first
    double                  m_totalKm = 0.0;    ///< kilometres driven in all modes
    int                     m_titles = 0;       ///< championships won
    int                     m_bestPlace = 0;    ///< best championship place (0: none)
    std::bitset<256>        m_seen;             ///< a bit per biome driven through

    /**
     * @brief Reads the records file.
     * @param[in] path  the file
     */
    void load( const std::string &path );

    /**
     * @brief Writes the records file.
     * @param[in] path  the file
     */
    void save( const std::string &path ) const;

    /**
     * @brief Where a score would enter the table.
     * @param[in] score  the score
     * @return 0..9, or -1 if it does not
     */
    int rankOf( int64_t score ) const;

    /**
     * @brief Enters a score.
     * @param[in] entry  the line
     */
    void insert( const ScoreEntry &entry );

    /**
     * @brief Whether a car may be driven.
     * @param[in] car  the car
     * @return true if unlocked
     */
    bool carUnlocked( int car ) const;

    /**
     * @brief Whether a paint may be used.
     * @param[in] paint  the paint
     * @return true if unlocked
     */
    bool paintUnlocked( int paint ) const;

    /**
     * @brief What unlocks a car.
     * @param[in] car  the car
     * @return e.g. "30 KM"
     */
    static std::string carRequirement( int car );

    /**
     * @brief What unlocks a paint.
     * @param[in] paint  the paint
     * @return e.g. "15 KM" or "WIN A CHAMPIONSHIP"
     */
    static std::string paintRequirement( int paint );

    /**
     * @brief Counts the biomes discovered.
     * @return 0..BiomeE::COUNT
     */
    int seenCount() const;

    /**
     * @brief How many countries the biomes driven through belong to.
     * @return the number
     */
    int countriesSeen() const;
};

/** @brief One sample of a recorded drive. */
struct GhostSample
{
    float  m_t = 0;     ///< seconds since the start
    double m_s = 0;     ///< distance along the road
    float  m_x = 0;     ///< lateral offset
    float  m_yaw = 0;   ///< drawn yaw
};

/** @brief The ghost: the best drive on a track, recorded and replayed. */
class Ghost
{
public:
    /** @brief Starts recording a drive. */
    void startRecording();

    /**
     * @brief Records the car (ten times a second).
     * @param[in] t    seconds since the start
     * @param[in] car  the player's car
     */
    void record( float t, const PlayerCar &car );

    /**
     * @brief Loads the best drive of a track for playback.
     * @param[in] dir   the ghosts folder
     * @param[in] code  the track
     * @return true if there is one
     */
    bool load( const std::string &dir, const std::string &code );

    /**
     * @brief Keeps the recording if it beats the stored one.
     * @param[in] dir     the ghosts folder
     * @param[in] code    the track
     * @param[in] metric  the drive's result (higher is better: a score, or minus a time)
     * @param[in] car     the car driven
     * @param[in] paint   its paint
     * @return true if it was the best and was saved
     */
    bool saveIfBest( const std::string &dir, const std::string &code, double metric, int car, int paint );

    /**
     * @brief Where the ghost is at a time.
     * @param[in]  t    seconds since the start
     * @param[out] s    distance
     * @param[out] x    lateral offset
     * @param[out] yaw  yaw
     * @return false if there is no ghost or it has finished
     */
    bool at( float t, double &s, float &x, float &yaw ) const;

    int    m_car = 0;          ///< the ghost's car
    int    m_paint = 1;        ///< and paint
    double m_metric = -1e300;  ///< the stored drive's result

private:
    std::vector<GhostSample> m_recording;   ///< this drive
    std::vector<GhostSample> m_best;        ///< the stored drive
    float                    m_nextSample = 0.0f;   ///< time of the next sample
};

} // namespace sunstreak
