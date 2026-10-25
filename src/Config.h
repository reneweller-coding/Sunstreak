/**
 * @file Config.h
 * @brief The player's settings and records, the command line, and where they are kept.
 *
 * Settings live in a plain key=value file: %APPDATA%/Sunstreak/settings.ini on Windows,
 * $XDG_CONFIG_HOME/sunstreak (or ~/.config/sunstreak) elsewhere. The game starts windowed on Windows and fullscreen
 * on a Steam Deck unless the file or the command line says otherwise.
 *
 * @author Rene Weller
 */
#pragma once

#include <cstdint>
#include <string>

namespace sunstreak
{

/** @brief How the game shows its picture. */
enum class DisplayModeE
{
    WINDOWED,     ///< a resizable window
    FULLSCREEN,   ///< borderless, the size of the monitor
};

/** @brief The unit of the speedometer. */
enum class UnitsE
{
    KMH,   ///< kilometres per hour
    MPH,   ///< miles per hour (as in the original)
};

/** @brief Everything the settings menu changes, and the records. */
struct Settings
{
    DisplayModeE m_display = DisplayModeE::WINDOWED;   ///< window or fullscreen
    int          m_windowWidth = 1600;                 ///< the window's width in windowed mode
    int          m_windowHeight = 900;                 ///< and its height
    bool         m_vsync = true;                       ///< wait for the vertical blank
    float        m_renderScale = 1.0f;                 ///< the 3D picture's resolution against the window's
    float        m_musicVolume = 0.6f;                 ///< 0..1
    float        m_sfxVolume = 0.8f;                   ///< 0..1
    UnitsE       m_units = UnitsE::KMH;                ///< speedometer unit
    bool         m_manualGears = false;                ///< shift by hand
    int          m_camera = 0;                         ///< 0 chase, 1 far chase, 2 bumper, 3 cockpit
    bool         m_speedBlur = true;                   ///< radial blur at speed
    int          m_station = -1;                       ///< radio: -1 automatic (by biome), else a station
    int          m_difficulty = 1;                     ///< 0 easy, 1 normal, 2 hard
    bool         m_shadows = true;                     ///< real-time sun shadows
    int          m_fpsLimit = 0;                       ///< frame rate cap (0: none beyond vsync)
    bool         m_vibration = true;                   ///< gamepad rumble
    int          m_scenery = 0;                        ///< endless runs: 0 the world tour, 1 every biome mixed
    int          m_timeOfDay = 0;                      ///< 0 as the place, 1 the tour clock (time zones), 2 dawn, 3 day, 4 dusk, 5 night
    int          m_weather = 0;                        ///< 0 weather fronts pass, 1 as the place
    int          m_seasons = 0;                        ///< 0 as today, 1 as the tour goes on, 2 off
    bool         m_police = true;                      ///< patrol cars wait at the roadside and chase the car too fast past them
    int          m_steerAssist = 0;                    ///< 0 off, 1 light, 2 strong: helps keep the road and miss the traffic
    bool         m_brakeAssist = false;                ///< brakes for bends and obstacles taken too fast
    bool         m_colourBlind = false;                ///< the HUD's red and green become orange and blue
    int          m_textSize = 0;                       ///< 0 100 %, 1 115 %, 2 130 %
    int          m_profile = 0;                        ///< 0 custom, 1 battery, 2 balanced, 3 quality
    int          m_car = 0;                            ///< the chosen car
    int          m_paint = 0;                          ///< its paint
    int          m_rims = 0;                           ///< its rims
    int          m_decal = 0;                          ///< its decal
    int          m_decalColor = 1;                     ///< the decal's colour (a paint)
    std::string  m_homeCountry;                        ///< the country of the car's number plate (empty: none)
    std::string  m_plateText = "SUN 26";               ///< what the plate says
    int64_t      m_bestScore = 0;                      ///< the highest score of an endless run
    double       m_bestDistance = 0.0;                 ///< the longest endless run in metres
};

/** @brief What the command line asked for (testing and screenshots included). */
struct LaunchOptions
{
    int         m_forceDisplay = -1;   ///< -1 as the settings say, 0 windowed, 1 fullscreen
    uint64_t    m_seed = 0;            ///< 0: a new random track
    int         m_startBiome = -1;     ///< -1 as the seed says, else the first stage's biome
    int         m_scenery = -3;        ///< endless runs: a biome, -1 all mixed, -2 the world tour (-3: as the settings say)
    bool        m_autopilot = false;   ///< drive by itself (demo and screenshots)
    bool        m_lookBack = false;    ///< look back all the time (screenshots)
    bool        m_skipMenu = false;    ///< start a run at once
    float       m_quitAfter = 0.0f;    ///< seconds of play after which the game quits (0: never)
    std::string m_screenshot;          ///< save the last frame there before quitting
    std::string m_bench;               ///< measure the frames (no vsync, no limit) and write the times there at the end
    float       m_startDistance = 0;   ///< teleport this far along the track at the start (testing)
    int         m_width = 0;           ///< a window size for this run only (0: the settings')
    int         m_height = 0;          ///< its height
    bool        m_mute = false;        ///< no sound at all (test runs)
    std::string m_audioTest;           ///< render a test drive's sound to this WAV and quit
    int         m_find = 0;            ///< start before the first 1 tunnel, 2 bridge, 3/4 landmark, 5 fork, 6 sea portal, 7 in it, 8 its far portal, 9 ferry (testing)
    int         m_weather = -1;        ///< a weather front all the way (WeatherE, testing; -1: as it comes)
    int         m_timeOfDay = -1;      ///< the time of day for this start (Settings::m_timeOfDay; -1: the settings')
    int         m_rally = -1;          ///< start this rally (0-based; -1: none)
    int         m_journey = -1;        ///< start this journey (0-based; -1: none)
    bool        m_resume = false;      ///< continue the saved endless run
    bool        m_daily = false;       ///< start the daily challenge
    bool        m_travel = false;      ///< start travelling (the world tour without a clock)
    std::string m_action;              ///< testing: replay, photo, reload (the data files) or save (and quit) after m_actionAt seconds of racing
    float       m_actionAt = 0.0f;     ///< when
    int         m_camera = -1;         ///< a camera for this run only (-1: the settings')
    std::string m_code;                ///< start this track code at once
    bool        m_championship = false;///< start a championship at once
    std::string m_screen;              ///< open a menu screen at once: lab, records, code (screenshots)
    std::string m_shape;               ///< the landmark the viewer opens on (--screen viewer)
    bool        m_stats = false;       ///< testing: start with the F3 performance overlay shown
    int         m_shapeTour = 0;       ///< testing: the viewer saves this many models from there on, one picture each, named after them after the --screenshot prefix
    float       m_season = -1.0f;      ///< testing: the time of year (0: 1 January), -1 today's
};

/**
 * @brief The folder for settings and records (created on first use).
 * @return the path, ending without a separator
 */
std::string configDirectory();

/**
 * @brief Reads the settings file; missing keys keep their defaults.
 * @param[out] settings  the settings to fill
 * @return true if a file was read
 */
bool loadSettings( Settings &settings );

/**
 * @brief Writes the settings file.
 * @param[in] settings  what to write
 * @return true on success
 */
bool saveSettings( const Settings &settings );

/**
 * @brief Parses the command line.
 * @param[in] argc  the argument count
 * @param[in] argv  the arguments
 * @return the options
 */
LaunchOptions parseCommandLine( int argc, char **argv );

/**
 * @brief Whether this machine looks like a Steam Deck (or runs in its gamescope session).
 * @return true on a Deck
 */
bool runningOnSteamDeck();

} // namespace sunstreak
