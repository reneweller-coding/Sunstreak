/**
 * @file Game.h
 * @brief The game: title with an attract-mode drive, the endless run against the clock, pause, results, settings,
 *        the chase camera, particles, sound cues and the HUD.
 *
 * The endless run works like Lotus III's time challenge: the clock counts down, every checkpoint (2.5 km) adds
 * time, and the run ends when the clock reaches zero. Every 5 km a new stage with a new biome begins; the roads get
 * curvier, the traffic denser, the time bonus tighter. A run's track comes from its code, so a good run can be
 * driven again.
 *
 * @author reneweller
 */
#pragma once

#include "Audio.h"
#include "Car.h"
#include "Config.h"
#include "Input.h"
#include "Models.h"
#include "Particles.h"
#include "Renderer.h"
#include "Track.h"
#include "Traffic.h"
#include "World.h"

#include <memory>
#include <string>
#include <vector>

namespace sunstreak
{

/** @brief The items of the settings menu, in order. */
enum class SettingE
{
    DISPLAY,      ///< window or fullscreen
    RESOLUTION,   ///< 3D resolution
    SHADOWS,      ///< sun shadows
    VSYNC,        ///< vertical sync
    FRAME_LIMIT,  ///< frame rate cap
    DIFFICULTY,   ///< easy, normal, hard
    GEARS,        ///< automatic or manual
    UNITS,        ///< km/h or mph
    CAMERA,       ///< chase, far, bumper
    BLUR,         ///< speed blur
    VIBRATION,    ///< rumble
    MUSIC,        ///< music volume
    EFFECTS,      ///< effects volume
    RADIO,        ///< radio station
    BACK,         ///< leave
    COUNT         ///< number of items
};

/** @brief Where the game is. */
enum class StateE
{
    TITLE,       ///< title menu over a demo drive
    SETTINGS,    ///< settings menu
    COUNTDOWN,   ///< 3, 2, 1, GO
    RACE,        ///< driving against the clock
    PAUSED,      ///< pause menu
    GAMEOVER,    ///< results
};

/** @brief A message in the middle of the screen. */
struct Popup
{
    std::string m_text;       ///< the big line
    std::string m_sub;        ///< the small line
    Color       m_color{};    ///< colour of the big line
    float       m_age = 0;    ///< seconds shown
    float       m_life = 2;   ///< seconds to show
    float       m_size = 1;   ///< size factor
};

/**
 * @brief A seed as a track code (eight characters, Crockford's base 32).
 * @param[in] seed  the seed (40 bits used)
 * @return "XXXX-XXXX"
 */
std::string trackCode( uint64_t seed );

/** @brief The game. */
class Game
{
public:
    /**
     * @brief Prepares the game.
     * @param[in,out] settings  the player's settings (saved on change)
     * @param[in]     options   the command line
     */
    Game( Settings &settings, const LaunchOptions &options );

    /** @brief Loads models, shaders and sound (needs the window). */
    void init();

    /** @brief Frees everything. */
    void shutdown();

    /**
     * @brief Runs one frame: input, simulation, drawing.
     * @param[in] dt  seconds since the last frame
     */
    void frame( float dt );

    /**
     * @brief Whether the player chose to quit.
     * @return true to close
     */
    bool wantsQuit() const { return m_quit; }

private:
    /** @brief Starts the attract-mode drive behind the title. */
    void startDemo();

    /**
     * @brief Starts a run.
     * @param[in] seed  the track's seed
     */
    void startRun( uint64_t seed );

    /**
     * @brief Sets up a track and puts the car on its start.
     * @param[in] seed       the seed
     * @param[in] startS     where the car starts
     */
    void setupTrack( uint64_t seed, double startS );

    /**
     * @brief Advances the world: track, chunks, car, traffic, rules, camera, particles, sound.
     * @param[in] dt         seconds
     * @param[in] autopilot  the car drives itself
     */
    void simulate( float dt, bool autopilot );

    /**
     * @brief One physics step.
     * @param[in] h   seconds
     * @param[in] in  the controls
     */
    void step( float h, const CarInput &in );

    /**
     * @brief The demo driver.
     * @return controls that follow the road and avoid traffic
     */
    CarInput autopilot() const;

    /**
     * @brief Turns this frame's collected events into sound, particles, popups and score.
     */
    void handleEvents();

    /**
     * @brief Moves the camera.
     * @param[in] dt  seconds
     */
    void updateCamera( float dt );

    /**
     * @brief Emits smoke, dust, spray and flames.
     * @param[in] dt  seconds
     */
    void emitParticles( float dt );

    /**
     * @brief Emits a burst of sparks.
     * @param[in] at     where
     * @param[in] count  how many
     * @param[in] speed  how fast
     */
    void sparks( const Vec3d &at, int count, float speed );

    /** @brief Feeds the engine sound. */
    void updateAudio();

    /**
     * @brief Shows a message.
     * @param[in] text   big line
     * @param[in] sub    small line
     * @param[in] color  colour
     * @param[in] size   size factor
     * @param[in] life   seconds
     */
    void popup( const std::string &text, const std::string &sub, Color color, float size = 1.0f, float life = 2.2f );

    /** @brief Switches between window and fullscreen. */
    void toggleFullscreen();

    /**
     * @brief Handles the menu of the current state.
     * @param[in] m  menu input
     */
    void menus( const MenuInput &m );

    /** @brief Draws the frame. */
    void draw();

    /** @brief Draws the HUD of a run. */
    void drawHud();

    /** @brief Draws the minimap of the road ahead. */
    void drawMinimap( float x, float y, float w, float h );

    /**
     * @brief Draws a menu.
     * @param[in] title  heading
     * @param[in] items  labels
     * @param[in] values value texts (empty: none)
     * @param[in] y      top of the menu
     */
    void drawMenu( const char *title, const std::vector<std::string> &items, const std::vector<std::string> &values, float y );

    /** @brief Draws the title screen. */
    void drawTitle();

    /** @brief Draws the results. */
    void drawResults();

    /** @brief Saves the settings and applies volumes. */
    void settingsChanged();

    /**
     * @brief A settings item's label.
     * @param[in] item  the item
     * @return its label
     */
    const char *settingLabel( SettingE item ) const;

    /**
     * @brief A settings item's value as text.
     * @param[in] item  the item
     * @return the value
     */
    std::string settingValue( SettingE item ) const;

    /**
     * @brief Changes a setting.
     * @param[in] item  the item
     * @param[in] dir   -1 or +1
     */
    void changeSetting( SettingE item, int dir );

    /**
     * @brief The station the radio should play now (the biome's in automatic mode).
     * @return 0..StationCount-1, StationCount for the music files
     */
    int wantedStation() const;

    /**
     * @brief Starts a track and shows what plays.
     * @param[in] seed     the track's seed
     * @param[in] station  the station
     */
    void playMusic( uint64_t seed, int station );

    /** @brief Steps the radio to the next station (automatic, the eight, the music files). */
    void nextStation();

    /**
     * @brief The radio setting as text.
     * @return "AUTO", a station's name or "MY MUSIC"
     */
    std::string radioName() const;

    Settings     &m_settings;      ///< the player's settings
    Tuning        m_tuningBase;    ///< tuning.ini before the difficulty
    LaunchOptions m_options;       ///< the command line

    Models                 m_models;       ///< meshes
    Renderer               m_renderer;     ///< drawing
    World                  m_world;        ///< chunks
    std::unique_ptr<Track> m_track;        ///< the road
    PlayerCar              m_car;          ///< the player
    Traffic                m_traffic;      ///< the others
    Particles              m_particles;    ///< effects
    Audio                  m_audio;        ///< sound
    Input                  m_input;        ///< controls
    bool                   m_audioOk = false;   ///< sound works

    StateE m_state = StateE::TITLE;   ///< where the game is
    StateE m_settingsReturn = StateE::TITLE;   ///< where the settings menu returns to
    StateE m_pauseReturn = StateE::RACE;       ///< where the pause menu returns to
    int    m_returnIndex = 0;                  ///< menu item to restore after the settings
    float  m_stateTime = 0.0f;        ///< seconds in this state
    float  m_time = 0.0f;             ///< seconds since start
    float  m_dt = 0.0f;               ///< this frame's time
    int    m_menuIndex = 0;           ///< selected menu item
    int    m_carIndex = 0;            ///< chosen car
    int    m_paintIndex = 0;          ///< chosen paint
    bool   m_quit = false;            ///< close the game
    bool   m_demo = true;             ///< the run is the title's demo

    // The run.
    uint64_t m_seed = 1;              ///< the track's seed
    float    m_timeLeft = 0.0f;       ///< seconds on the clock
    bool     m_timeUp = false;        ///< the clock reached zero
    float    m_timeUpFor = 0.0f;      ///< seconds since then
    double   m_score = 0.0;           ///< points
    double   m_lastS = 0.0;           ///< distance at the last step (scoring)
    double   m_startS = 0.0;          ///< where the run began
    double   m_nextCheckpoint = 0.0;  ///< distance of the next checkpoint
    int      m_checkpoints = 0;       ///< checkpoints passed
    int      m_stage = 0;             ///< current stage
    int      m_overtakes = 0;         ///< cars passed
    int      m_nearMisses = 0;        ///< close passes
    int      m_combo = 1;             ///< score multiplier
    float    m_comboTimer = 0.0f;     ///< seconds until the combo resets
    float    m_topSpeed = 0.0f;       ///< fastest speed of the run (m/s)
    bool     m_newBest = false;       ///< the run beat the record
    int      m_lastWarning = 99;      ///< last second a warning beeped
    int      m_lastCount = 4;         ///< last countdown number shown

    // Collected during the physics steps.
    TrafficEvents m_ev;               ///< traffic events this frame
    float         m_crash = 0.0f;     ///< crash strength this frame
    bool          m_scraping = false; ///< touching a guard rail
    bool          m_wasScraping = false;   ///< ... last frame
    float         m_scrapeSide = 0.0f;     ///< which side
    bool          m_landed = false;   ///< touched down this frame
    float         m_landImpact = 0.0f;///< how hard
    bool          m_shifted = false;  ///< changed gear this frame
    bool          m_backfire = false; ///< lifted off at high revs this frame
    bool          m_wasBoosting = false;   ///< boosting last frame
    std::vector<const Collider *> m_colliders;   ///< scratch list

    // Camera.
    CameraView m_cam;                 ///< the view
    float      m_camX = 0.0f;         ///< smoothed lateral offset
    double     m_camY = 0.0;          ///< smoothed height
    float      m_fov = 60.0f;         ///< smoothed field of view
    bool       m_camInit = false;     ///< the camera has a position
    float      m_blur = 0.0f;         ///< smoothed speed blur
    float      m_aberration = 0.0f;   ///< smoothed boost fringes

    // Weather and effects.
    float m_flash = 0.0f;             ///< lightning brightness
    float m_thunderIn = -1.0f;        ///< seconds until thunder (-1: none)
    float m_emitAcc = 0.0f;           ///< particle emission accumulator

    std::vector<Popup> m_popups;      ///< messages

    // The radio.
    int         m_playingStation = -1;  ///< the station that plays
    uint64_t    m_trackSeed = 0;        ///< the track that plays
    int         m_musicStage = -1;      ///< the stage the radio last looked at
    std::string m_nowPlaying;           ///< "STATION - TITLE"
    float       m_nowTimer = 99.0f;     ///< seconds since the track started (the toast fades)
};

} // namespace sunstreak
