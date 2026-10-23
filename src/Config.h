/**
 * @file Config.h
 * @brief The player's settings and records, the command line, and where they are kept.
 *
 * Settings live in a plain key=value file: %APPDATA%\Sunstreak\settings.ini on Windows,
 * $XDG_CONFIG_HOME/sunstreak (or ~/.config/sunstreak) elsewhere. The game starts windowed on Windows and fullscreen
 * on a Steam Deck unless the file or the command line says otherwise.
 *
 * @author reneweller
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
    int          m_camera = 0;                         ///< 0 chase, 1 far chase, 2 bumper
    bool         m_speedBlur = true;                   ///< radial blur at speed
    int          m_station = -1;                       ///< radio: -1 automatic (by biome), else a station
    int          m_difficulty = 1;                     ///< 0 easy, 1 normal, 2 hard
    bool         m_shadows = true;                     ///< real-time sun shadows
    int          m_fpsLimit = 0;                       ///< frame rate cap (0: none beyond vsync)
    bool         m_vibration = true;                   ///< gamepad rumble
    int64_t      m_bestScore = 0;                      ///< the highest score of an endless run
    double       m_bestDistance = 0.0;                 ///< the longest endless run in metres
};

/** @brief What the command line asked for (testing and screenshots included). */
struct LaunchOptions
{
    int         m_forceDisplay = -1;   ///< -1 as the settings say, 0 windowed, 1 fullscreen
    uint64_t    m_seed = 0;            ///< 0: a new random track
    int         m_startBiome = -1;     ///< -1 as the seed says, else the first stage's biome
    bool        m_autopilot = false;   ///< drive by itself (demo and screenshots)
    bool        m_skipMenu = false;    ///< start a run at once
    float       m_quitAfter = 0.0f;    ///< seconds of play after which the game quits (0: never)
    std::string m_screenshot;          ///< save the last frame there before quitting
    float       m_startDistance = 0;   ///< teleport this far along the track at the start (testing)
    int         m_width = 0;           ///< a window size for this run only (0: the settings')
    int         m_height = 0;          ///< its height
    bool        m_mute = false;        ///< no sound at all (test runs)
    std::string m_audioTest;           ///< render a test drive's sound to this WAV and quit
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
