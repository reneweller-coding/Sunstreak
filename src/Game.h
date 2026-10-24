/**
 * @file Game.h
 * @brief The game: title with an attract-mode drive, the endless run against the clock, the championship against
 *        rivals, the Track Lab, records and ghosts, pause, results, settings, the chase camera, particles, sound cues
 *        and the HUD.
 *
 * The endless run works like Lotus III's time challenge: the clock counts down, every checkpoint (2.5 km) adds
 * time, and the run ends when the clock reaches zero. Every 5 km a new stage with a new biome begins; the roads get
 * curvier, the traffic denser, the time bonus tighter. The championship is five races from the back of a grid of
 * twelve. The Track Lab builds a track from sliders and gives its code; a code typed in drives that track again.
 *
 * Game.cpp holds the flow and the simulation, GameUi.cpp the menus, the screens and the HUD.
 *
 * @author reneweller
 */
#pragma once

#include "Audio.h"
#include "Haptics.h"
#include "Car.h"
#include "Config.h"
#include "Input.h"
#include "Models.h"
#include "Particles.h"
#include "Race.h"
#include "Records.h"
#include "Renderer.h"
#include "Track.h"
#include "Traffic.h"
#include "World.h"

#include <future>
#include <map>
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
    SCENERY,      ///< endless runs: the world tour or all mixed
    TIME_OF_DAY,  ///< as the place, the tour clock, or a fixed hour
    WEATHER,      ///< weather fronts or the place's own weather
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

/** @brief The items of the title menu, in order. */
enum class TitleE
{
    ENDLESS,       ///< endless run
    CHAMPIONSHIP,  ///< five races
    TRACK_LAB,     ///< build or enter a track
    GARAGE,        ///< car, paint, rims, decals
    PASSPORT,      ///< world map, stamps, postcards
    RECORDS,       ///< top ten and progress
    SETTINGS,      ///< settings
    QUIT,          ///< leave the game
    COUNT          ///< number of items
};

/** @brief The items of the garage, in order. */
enum class GarageE
{
    CAR,           ///< which car
    PAINT,         ///< its paint
    RIMS,          ///< rim finish
    DECAL,         ///< stripes, roundels, chequers
    DECAL_COLOR,   ///< the decal's colour
    BACK,          ///< leave
    COUNT          ///< number of items
};

/** @brief The pages of the passport. */
enum class PassportE
{
    MAP,        ///< the world tour on a map
    STAMPS,     ///< a stamp per country
    POSTCARDS,  ///< a picture per place
    COUNT       ///< number of pages
};

/** @brief The items of the Track Lab, in order. */
enum class LabE
{
    MODE,        ///< time attack or race
    SCENERY,     ///< mixed or one biome
    CURVES,      ///< how curvy
    SHARPNESS,   ///< how tight
    HILLS,       ///< how hilly
    TRAFFIC,     ///< traffic density
    OBSTACLES,   ///< roadworks and puddles
    LENGTH,      ///< endless or a finish
    SEED,        ///< the random part
    CODE,        ///< type a code in
    START,       ///< drive it
    BACK,        ///< leave
    COUNT        ///< number of items
};

/** @brief Where the game is. */
enum class StateE
{
    TITLE,       ///< title menu over a demo drive
    SETTINGS,    ///< settings menu
    COUNTDOWN,   ///< 3, 2, 1, GO
    RACE,        ///< driving
    PAUSED,      ///< pause menu
    GAMEOVER,    ///< results of a time attack
    RESULTS,     ///< results of a race, then the standings
    LAB,         ///< the Track Lab
    CODE_ENTRY,  ///< typing a track code
    INITIALS,    ///< entering a name into the top ten
    RECORDS,     ///< the records screen
    GARAGE,      ///< dressing the car
    PASSPORT,    ///< map, stamps and postcards
};

/** @brief What is being played. */
enum class ModeE
{
    ENDLESS,       ///< the endless time attack
    CHAMPIONSHIP,  ///< five races against rivals
    CUSTOM,        ///< a Track Lab track (time attack or a single race)
};

/** @brief A weather front passing over the road. */
enum class WeatherE
{
    NONE,        ///< clear
    RAIN,        ///< a shower
    STORM,       ///< a thunderstorm
    FOG,         ///< a fog bank
    SNOW,        ///< a snow shower
    SANDSTORM,   ///< blowing sand
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

    /** @brief Loads models, shaders, sound and records (needs the window). */
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
    // ---- Flow (Game.cpp) ----------------------------------------------------------------------------------

    /**
     * @brief The track settings of an endless run (the world tour or all mixed).
     * @return them
     */
    TrackParams endlessParams() const;

    /** @brief Starts the attract-mode drive behind the title. */
    void startDemo();

    /**
     * @brief Starts a time attack or a single race on a track.
     * @param[in] setup  the track (default parameters: the endless run)
     * @param[in] mode   endless or custom
     */
    void startTrack( const TrackSetup &setup, ModeE mode );

    /** @brief Starts a new championship at round one. */
    void startChampionship();

    /**
     * @brief Starts a round of the championship.
     * @param[in] round  0..RoundCount-1
     */
    void startRound( int round );

    /** @brief Restarts what is being played from its start. */
    void restart();

    /**
     * @brief Sets up a track and puts the car on it.
     * @param[in] seed    the seed
     * @param[in] startS  where the car starts
     * @param[in] params  the generator's parameters
     */
    void setupTrack( uint64_t seed, double startS, const TrackParams &params );

    /** @brief Resets the run's counters and starts the countdown. */
    void beginCountdown();

    /**
     * @brief Advances the world: track, chunks, car, traffic, rivals, rules, camera, particles, sound.
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
     * @brief The rules of a time attack in one step.
     * @param[in] h  seconds
     */
    void timeAttackRules( float h );

    /** @brief The player crossed the finish line. */
    void crossFinish();

    /** @brief A time attack ended (time up, or the finish of a fixed-length track). */
    void endTimeAttack();

    /**
     * @brief The demo driver.
     * @return controls that follow the road and avoid traffic and rivals
     */
    CarInput autopilot() const;

    /** @brief Turns this frame's collected events into sound, particles, popups and score. */
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
     * @brief Counts kilometres, discovered biomes and what they unlock.
     * @param[in] ds  metres driven this frame
     */
    void progress( double ds );

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

    /** @brief Saves the settings and applies volumes. */
    void settingsChanged();

    /** @brief Saves the records. */
    void saveRecords() const;

    /**
     * @brief The weather front over a point of the road: a few per hundred kilometres, of the kind the place's climate
     *        brings, rising and falling over a kilometre or two.
     * @param[in]  s     distance along the road
     * @param[out] type  the front's kind
     * @return its strength 0..1
     */
    float frontAt( double s, WeatherE &type ) const;

    /**
     * @brief The local hour the sky shows (the tour clock: the time zone of the place; or the fixed hour chosen).
     * @return 0..24, or -1 for the place's own light
     */
    float wantedHour() const;

    /**
     * @brief The air and light at a point of the road: the place, blended between stages, at the hour, in the weather.
     * @param[in] s  distance along the road
     * @return the environment
     */
    Environment sceneEnvironment( double s ) const;

    /** @brief Dresses the player's car as the garage says and keeps the choice. */
    void applyLivery();

    /**
     * @brief Adds career points.
     * @param[in] points  how many
     */
    void earn( int64_t points );

    /**
     * @brief Saves the picture just rendered as the current place's postcard (before the HUD is drawn).
     * @param[in] biome  the place
     */
    void takePostcard( BiomeE biome );

    /** @brief Frees the postcard pictures the passport loaded. */
    void unloadPostcards();

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

    // ---- Screens (GameUi.cpp) ------------------------------------------------------------------------------

    /**
     * @brief Handles the menu of the current state.
     * @param[in] m  menu input
     */
    void menus( const MenuInput &m );

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
     * @brief A Track Lab item's value as text.
     * @param[in] item  the item
     * @return the value
     */
    std::string labValue( LabE item ) const;

    /**
     * @brief Changes a Track Lab value.
     * @param[in] item  the item
     * @param[in] dir   -1 or +1
     */
    void changeLab( LabE item, int dir );

    /** @brief Draws the frame. */
    void draw();

    /** @brief Draws the HUD of a run or a race. */
    void drawHud();

    /**
     * @brief Draws the minimap of the road ahead.
     * @param[in] x  left
     * @param[in] y  top
     * @param[in] w  width
     * @param[in] h  height
     */
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

    /** @brief Draws the results of a time attack. */
    void drawResults();

    /** @brief Draws a race's result, the standings, or the championship's end. */
    void drawRaceResults();

    /** @brief Draws the Track Lab. */
    void drawLab();

    /** @brief Draws the code entry. */
    void drawCodeEntry();

    /** @brief Draws the initials entry. */
    void drawInitials();

    /** @brief Draws the records screen. */
    void drawRecords();

    /** @brief Draws the garage. */
    void drawGarage();

    /** @brief Draws the passport. */
    void drawPassport();

    /**
     * @brief Draws the world tour on a map: the route, the places seen, the slices of the globe.
     * @param[in] x  left
     * @param[in] y  top
     * @param[in] w  width
     * @param[in] h  height
     */
    void drawWorldMap( float x, float y, float w, float h );

    /**
     * @brief Draws a page of country stamps.
     * @param[in] x  left
     * @param[in] y  top
     * @param[in] w  width
     * @param[in] h  height
     */
    void drawStamps( float x, float y, float w, float h );

    /**
     * @brief Draws a page of postcards.
     * @param[in] x  left
     * @param[in] y  top
     * @param[in] w  width
     * @param[in] h  height
     */
    void drawPostcards( float x, float y, float w, float h );

    /**
     * @brief A garage item's value as text.
     * @param[in] item  the item
     * @return the value, with (LOCKED) where it is
     */
    std::string garageValue( GarageE item ) const;

    /**
     * @brief Changes a garage item.
     * @param[in] item  the item
     * @param[in] dir   -1 or +1
     */
    void changeGarage( GarageE item, int dir );

    /** @brief Draws the messages in the middle of the screen. */
    void drawPopups();

    // ---- State -------------------------------------------------------------------------------------------

    Settings     &m_settings;      ///< the player's settings
    Tuning        m_tuningBase;    ///< tuning.ini before the difficulty
    LaunchOptions m_options;       ///< the command line
    Records       m_records;       ///< top ten, kilometres, unlocks
    Ghost         m_ghost;         ///< the best drive of the current track

    Models                 m_models;       ///< meshes
    Renderer               m_renderer;     ///< drawing
    World                  m_world;        ///< chunks
    std::unique_ptr<Track> m_track;        ///< the road
    PlayerCar              m_car;          ///< the player
    Traffic                m_traffic;      ///< the others
    Race                   m_race;         ///< rivals and the championship
    Particles              m_particles;    ///< effects
    Audio                  m_audio;        ///< sound
    Input                  m_input;        ///< controls
    Haptics                m_haptics;      ///< rumble
    bool                   m_showStats = false;   ///< the F3 overlay
    float                  m_frameMsPeak = 0.0f;  ///< the slowest frame lately
    bool                   m_audioOk = false;   ///< sound works

    StateE m_state = StateE::TITLE;   ///< where the game is
    ModeE  m_mode = ModeE::ENDLESS;   ///< what is being played
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

    // The track being driven.
    TrackSetup  m_setup;              ///< its setup
    std::string m_code;               ///< its code
    uint64_t    m_seed = 1;           ///< its seed

    // A time attack.
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
    bool     m_newGhost = false;      ///< the run became the track's ghost
    int      m_rank = -1;             ///< place in the top ten (-1 none)
    bool     m_finishedTrack = false; ///< a fixed-length time attack reached its finish
    int      m_lastWarning = 99;      ///< last second a warning beeped
    int      m_lastCount = 4;         ///< last countdown number shown

    // A race.
    float                   m_raceTime = 0.0f;    ///< seconds since the start
    bool                    m_finished = false;   ///< the player crossed the line
    float                   m_finishedFor = 0.0f; ///< seconds since
    int                     m_place = 0;          ///< the player's final place
    std::vector<ResultLine> m_results;            ///< the race's result
    int                     m_resultPage = 0;     ///< 0 result, 1 standings, 2 championship end
    RaceEvents              m_raceEv;             ///< rival events this frame

    // Track Lab, code entry, initials.
    TrackSetup  m_lab;                ///< the lab's setup
    std::string m_codeText;           ///< the code being typed (12 characters)
    int         m_codeCursor = 0;     ///< the character being edited
    std::string m_codeError;          ///< why a code was refused
    std::string m_initials = "AAA";   ///< the name being entered
    int         m_initialCursor = 0;  ///< the letter being edited

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

    // Time of day and weather.
    double m_clockStart = 9.0;        ///< tour clock: the universal hour at the run's start
    float  m_hour = -1.0f;            ///< the local hour the sky shows (smoothed, -1: the place's own)
    long   m_frontShown = -1;         ///< the stage whose front was announced

    // Weather and effects.
    float m_flash = 0.0f;             ///< lightning brightness
    float m_thunderIn = -1.0f;        ///< seconds until thunder (-1: none)

    std::vector<Popup> m_popups;      ///< messages

    // Garage, passport, postcards, points.
    int64_t                    m_earned = 0;            ///< career points of this run
    int                        m_passportPage = 0;      ///< the passport's page (PassportE)
    int                        m_passportSheet = 0;     ///< which sheet of stamps or postcards
    int                        m_postcardWanted = -1;   ///< the place whose postcard is due (-1: none)
    double                     m_postcardAt = 0.0;      ///< taken once the car passes this distance
    std::map<int, Texture2D>   m_postcardTex;           ///< pictures loaded for the passport
    std::future<void>          m_postcardJob;           ///< the last postcard being written

    // The radio.
    int         m_playingStation = -1;  ///< the station that plays
    uint64_t    m_trackSeed = 0;        ///< the track that plays
    int         m_musicStage = -1;      ///< the stage the radio last looked at
    std::string m_nowPlaying;           ///< "STATION - TITLE"
    float       m_nowTimer = 99.0f;     ///< seconds since the track started (the toast fades)
};

} // namespace sunstreak
