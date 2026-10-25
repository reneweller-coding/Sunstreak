/**
 * @file Local.h
 * @brief The customs of a country's roads (data/roads.txt): speed limits in km/h or mph on round signs or white boards,
 *        toll plazas, speed cameras, number plates; and where a stage puts its camera and its toll plaza.
 * @author Rene Weller
 */
#pragma once

#include "Track.h"

#include <cstdint>

namespace sunstreak
{

/** @brief Number plates. */
enum class PlateE : uint8_t
{
    PLAIN,    ///< plain white
    EU,       ///< white with a blue band
    UK,       ///< yellow behind
    US,       ///< white with a coloured top
    AMBER,    ///< yellow
    JP,       ///< small, white with green letters
    DARK,     ///< black with white letters
    COUNT     ///< number of styles
};

/** @brief How a country's roads are run. */
struct RoadCustoms
{
    bool   m_mph = false;              ///< speed limits in miles per hour
    bool   m_board = false;            ///< speed limits on a white board
    bool   m_tolls = false;            ///< toll plazas
    bool   m_cameras = false;          ///< speed cameras
    PlateE m_plate = PlateE::PLAIN;    ///< number plates
    float  m_quality = 4.5f;           ///< 1..7 how good its roads are
    float  m_width = 3.5f;             ///< metres per lane
    bool   m_shoulders = false;        ///< a paved shoulder beyond the edge line
    bool   m_kerbs = false;            ///< its towns paint their kerbs
    Color  m_kerbA{};                  ///< in this colour
    Color  m_kerbB{};                  ///< and this one
    bool   m_studs = false;            ///< reflecting studs along the lines
    float  m_concrete = 0.0f;          ///< 0..1 of its places paved with concrete slabs
    int    m_concreteLanes = 0;        ///< only roads of so many lanes or more
    float  m_gravel = 0.0f;            ///< 0..1 of a country road on gravel or dirt
    float  m_traffic = 0.0f;           ///< Numbeo's traffic index (0: not known)
    uint8_t m_styles = 0;              ///< how its traffic drives (Style* bits)
    float  m_pollution = -1.0f;        ///< Numbeo's pollution index 0..100 (-1: not known)
    uint8_t m_lamp = 0;                ///< the light of its street lamps (Lamp* below)
    uint8_t m_post = 0;                ///< its delineator posts (Post* below)
};

constexpr uint8_t PostPlain = 0;         ///< white with an orange cap
constexpr uint8_t PostBand = 1;          ///< white with a black band and a reflector (Central Europe)
constexpr uint8_t PostFlex = 2;          ///< slim and flexible, a white reflector strip on the traffic's side, a yellow one across (North America)
constexpr uint8_t PostRedWhite = 3;      ///< white, a red reflector on the traffic's side, a white one across (Australia, New Zealand)

constexpr uint8_t LampWarm = 0;          ///< warm white LEDs
constexpr uint8_t LampSodium = 1;        ///< orange sodium vapour
constexpr uint8_t LampCool = 2;          ///< cool white LEDs and tubes

constexpr uint8_t StyleScooters = 1;     ///< swarms of scooters
constexpr uint8_t StyleChaos = 2;        ///< weaving, lanes changed on a whim, horns
constexpr uint8_t StyleTractors = 4;     ///< tractors on the country roads
constexpr uint8_t StyleRoadTrains = 8;   ///< road trains in the outback

constexpr double CameraAt = 650.0;   ///< where in its stage a speed camera stands (or the first clear spot after)
constexpr double LimitAt = 300.0;    ///< and the speed limit before it
constexpr double TollAt = 1300.0;    ///< where in its stage a toll plaza stands (or the first clear spot after)
constexpr double PoliceAt = 2500.0;  ///< a stage's patrol car waits somewhere from here on (if it has one)

/**
 * @brief A country's road customs.
 * @param[in] country  its name (nullptr: none)
 * @return them (the plain ones for none)
 */
const RoadCustoms &roadCustoms( const char *country );

/**
 * @brief Where a puddle gathers on the road when it rains (about one segment in sixty, away from tunnels, bridges,
 *        forks, jumps, roadworks and the road's own puddles).
 * @param[in]  track  the road
 * @param[in]  index  the segment
 * @param[out] x      the puddle's lateral centre
 * @return true if one gathers there
 */
bool rainPuddleAt( const Track &track, long index, float &x );

/**
 * @brief Where the monsoon floods the road: 40 m across the whole road in one 80 m block in four (not in tunnels, on
 *        bridges, forks or crossings).
 * @param[in] track  the road
 * @param[in] index  the segment
 * @return true if flooded there
 */
bool floodAt( const Track &track, long index );

/**
 * @brief How much smog a place's air holds.
 * @param[in] place  the place
 * @return 0 (clean, an index of 30 or less) .. 1 (an index of 90 or more)
 */
float smogOf( const BiomeDef &place );

/**
 * @brief How much smog a place's air holds at a time of year and of day: its index (smogOf) times how thick the air is
 *        then (data/smog.txt: thicker in the heating season, thinner in the monsoon, thickest at night), where the data
 *        knows the place.
 * @param[in] place  the biome
 * @param[in] phase  the time of year (0: 1 January), or below 0 for no seasonal change
 * @param[in] hour   the local hour (0..24), or below 0 for no daily change
 * @return 0 (clean) .. 1
 */
float smogAt( BiomeE place, float phase, float hour );

/**
 * @brief The air of a polluted place: a thicker haze turning brown, a paler sky, paler colours, a weaker, yellower sun.
 * @param[in] e    the air as it would be
 * @param[in] smog 0..1 (smogOf)
 * @return the smoggy air
 */
Environment withSmog( Environment e, float smog );

constexpr int PlateLength = 8;   ///< characters on the player's number plate

/**
 * @brief A country's letters in the blue band of a European plate.
 * @param[in] country  its name
 * @return them (empty: none)
 */
std::string plateCode( const char *country );

/**
 * @brief The country the computer's region setting names (for the home plate the first time).
 * @return its name (empty: not known)
 */
std::string countryFromLocale();

/**
 * @brief How many lines of data/roads.txt were wrong.
 * @return the count
 */
int roadErrors();

/**
 * @brief The speed limit on a road, as its signs give it.
 * @param[in] seg  the segment
 * @param[in] mph  in miles per hour
 * @return the number on the sign
 */
int speedLimit( const Segment &seg, bool mph );

/**
 * @brief The speed camera of a stage, if it has one.
 * @param[in]  track  the road
 * @param[in]  stage  the stage
 * @param[out] s      where it stands
 * @param[out] limit  the limit it checks (in the country's unit)
 * @param[out] mph    whether in miles per hour
 * @return true if the stage has one
 */
bool stageCamera( const Track &track, int stage, double &s, int &limit, bool &mph );

/**
 * @brief The toll plaza of a stage, if it has one.
 * @param[in]  track  the road
 * @param[in]  stage  the stage
 * @param[out] s      where it stands
 * @return true if the stage has one
 */
bool stageToll( const Track &track, int stage, double &s );

/**
 * @brief Where a stage's patrol car would wait, before the spot is checked (that needs the road laid there).
 * @param[in] track  the road
 * @param[in] stage  the stage
 * @return the distance, or -1 if the stage has none (one stage in ten has one, in a country)
 */
double policeBase( const Track &track, int stage );

/**
 * @brief The patrol car of a stage: the first clear spot from policeBase on (no tunnel, bridge, fork, gravel).
 * @param[in]  track  the road (laid to 500 m past policeBase)
 * @param[in]  stage  the stage
 * @param[out] s      where it waits
 * @param[out] side   on which verge (1 right, -1 left: the driver's side of the road)
 * @return true if it has one
 */
bool stagePolice( const Track &track, int stage, double &s, float &side );

/**
 * @brief Gives the places the roads of their countries: the lane width, how worn, gravel stretches, concrete slabs,
 *        road studs and the edges (a paved shoulder, the verge or the painted kerbs of the towns).
 * @param[in,out] table  every biome; those without a country keep their racing kerbs
 */
void applyRoadCustoms( std::vector<BiomeDef> &table );

/** @brief What OpenStreetMap shows round a place (data/roadside.txt). */
struct PlaceRoadside
{
    uint8_t m_pole = 0;          ///< the poles of its overhead lines: 0 none known, 1 wood, 2 concrete, 3 steel
    int8_t  m_lamp = -1;         ///< its street lamps: -1 not known, 0 LEDs, 1 sodium
    float   m_unpaved = -1.0f;   ///< 0..1 the share of its main roads on gravel or dirt (-1: not known)
    float   m_concrete = -1.0f;  ///< 0..1 the share on concrete slabs (-1: not known)
};

/**
 * @brief Gives the places what OpenStreetMap shows round them: the poles of overhead lines beside their roads, the
 *        light of their street lamps, more gravel and concrete where their roads have it. Run after applyRoadCustoms.
 * @param[in,out] table  every biome
 */
void applyRoadside( std::vector<BiomeDef> &table );

/**
 * @brief How many lines of data/roadside.txt were wrong (a line that does not parse, a place that does not exist).
 * @return the count
 */
int roadsideErrors();

/**
 * @brief How much of the road at a distance is gravel: the stretches where a poor country's asphalt ends.
 * @param[in] track  the road
 * @param[in] s      the distance
 * @return 0 asphalt .. 1 gravel (a place of gravel all along says so itself)
 */
float gravelAt( const Track &track, double s );

/**
 * @brief A pothole in a worn road.
 * @param[in]  track  the road
 * @param[in]  j      the segment
 * @param[out] x      its lateral offset
 * @param[out] r      its radius
 * @return true if the segment has one
 */
bool potholeAt( const Track &track, long j, float &x, float &r );

} // namespace sunstreak
