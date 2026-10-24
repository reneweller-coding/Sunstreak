/**
 * @file Records.h
 * @brief What the game remembers: track codes, the top-ten table, ghosts of the best runs, the kilometres driven and
 *        what they unlock, the biomes discovered.
 *
 * A track code holds everything the Track Lab sets (seed, curves, sharpness, hills, traffic, obstacles, scenery,
 * length, race or time attack) in fifteen characters of Crockford's base 32: XXXXX-XXXXX-XXXXX; a place numbered 510
 * or higher takes sixteen: XXXX-XXXX-XXXX-XXXX.
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
    uint64_t    m_seed = 1;         ///< 29 bits used
    TrackParams m_params;           ///< overrides (-1 automatic)
    bool        m_race = false;     ///< against rivals instead of the clock
    int         m_startBiome = -1;  ///< where a continued world tour begins (not in track codes; -1: as the seed says)
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
    std::bitset<MaxBiomes>  m_seen;             ///< a bit per biome driven through (saved by name)
    std::bitset<MaxBiomes>  m_postcards;        ///< a bit per place with a postcard in the album (saved by name)
    int64_t                 m_points = 0;       ///< career points: scores, race results, discoveries (the garage's currency)
    int                     m_dailyDate = 0;    ///< the day of the daily challenge driven last (YYYYMMDD)
    int64_t                 m_dailyBest = 0;    ///< its best score
    int                     m_dailyDays = 0;    ///< days with a daily challenge driven
    std::vector<double>     m_rallyBest;        ///< the best time per rally in seconds (0: not finished)
    std::vector<std::string> m_journey;         ///< the travel journal: the world tour's places in the order driven (IDs)
    std::string             m_tourAt;           ///< where the endless world tour stopped (an ID; empty: never driven)

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
     * @brief Whether a rim finish may be used.
     * @param[in] rims  the finish
     * @return true if unlocked
     */
    bool rimsUnlocked( int rims ) const;

    /**
     * @brief Whether a decal may be used.
     * @param[in] decal  the decal
     * @return true if unlocked
     */
    bool decalUnlocked( int decal ) const;

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
     * @brief What unlocks a rim finish.
     * @param[in] rims  the finish
     * @return e.g. "10 000 POINTS"
     */
    static std::string rimsRequirement( int rims );

    /**
     * @brief What unlocks a decal.
     * @param[in] decal  the decal
     * @return e.g. "20 000 POINTS"
     */
    static std::string decalRequirement( int decal );

    /**
     * @brief A number of career points as text.
     * @param[in] points  the number
     * @return e.g. "25 000 POINTS"
     */
    static std::string pointsText( int64_t points );

    /**
     * @brief How many slices of the globe hold a place driven through.
     * @return 0..LongitudeSectors
     */
    int sectorsSeen() const;

    /**
     * @brief Whether the player has driven round the world (a place in every slice of the globe).
     * @return true once all are seen
     */
    bool circumnavigated() const { return sectorsSeen() == LongitudeSectors; }

    /**
     * @brief Counts the biomes discovered.
     * @return 0..biomeCount()
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
