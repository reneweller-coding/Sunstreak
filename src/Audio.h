/**
 * @file Audio.h
 * @brief All sound, synthesised live: a V8 engine with turbo, tyres, wind, kerbs, gravel, effects, and a
 *        generative arcade soundtrack. Music files in a "music" folder replace the soundtrack if present.
 *
 * The game thread sets the engine's state and posts effects; the synthesis runs in raylib's audio callback, which
 * reads the state through atomics and the effects through a lock-free queue.
 *
 * @author reneweller
 */
#pragma once

#include "Music.h"

#include <raylib.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sunstreak
{

/** @brief Sound effects. */
enum class SfxE : uint8_t
{
    SHIFT,        ///< gear change clunk
    BACKFIRE,     ///< exhaust pops on lift-off
    BLOWOFF,      ///< turbo blow-off valve
    CRASH,        ///< heavy hit
    BUMP,         ///< light hit
    CONE,         ///< plastic cone
    WHOOSH,       ///< a car rushing past
    LAND,         ///< touching down after a jump
    CHECKPOINT,   ///< extended time
    NEAR_MISS,    ///< close pass
    BEEP,         ///< countdown
    GO,           ///< start
    TICK,         ///< menu move
    SELECT,       ///< menu confirm
    WARNING,      ///< little time left
    BOOST,        ///< boost kicks in
    STAGE,        ///< a new stage
    GAMEOVER,     ///< time is up
    VICTORY,      ///< a podium finish
    COUNT         ///< number of effects
};

/** @brief What the engine and the tyres are doing. */
struct EngineState
{
    bool  m_active = false;    ///< a car is running
    float m_rpm = 0.1f;        ///< 0..1.15 of the redline
    float m_throttle = 0.0f;   ///< 0..1
    float m_speed = 0.0f;      ///< m/s
    float m_slip = 0.0f;       ///< 0..1 squeal
    float m_offroad = 0.0f;    ///< 0..1 gravel
    float m_rumble = 0.0f;     ///< 0..1 kerb
    bool  m_boost = false;     ///< boosting
    bool  m_limiter = false;   ///< on the rev limiter
    bool  m_shifting = false;  ///< in a gear change (ignition cut)
    int   m_cylinders = 8;     ///< engine layout: 4, 8 (cross-plane V8) or 12
    float m_idle = 900.0f;     ///< idle rpm
    float m_redline = 7800.0f; ///< redline rpm
    float m_bright = 1.0f;     ///< tone brightness
};

/** @brief The sounds of the place around the car (set once per frame; the synthesis glides between them). */
struct Ambience
{
    float m_surf = 0.0f;      ///< 0..1 waves on a shore
    float m_surfPan = 0.0f;   ///< -1 left .. 1 right: where the water is
    float m_wind = 0.0f;      ///< 0..1 wind over open land
    float m_jungle = 0.0f;    ///< 0..1 insects and birds
    float m_city = 0.0f;      ///< 0..1 traffic hum and distant horns
    float m_rain = 0.0f;      ///< 0..1 rain on the car
    float m_tunnel = 0.0f;    ///< 0..1 inside a tunnel: the engine echoes
    float m_bridge = 0.0f;    ///< 0..1 on a bridge: the expansion joints thump
};

/** @brief The sound system. */
class Audio
{
public:
    /**
     * @brief Opens the audio device and starts the synthesis.
     * @param[in] musicDirs  folders to look for music files in
     * @return true if sound works
     */
    bool init( const std::vector<std::string> &musicDirs );

    /** @brief Stops everything. */
    void shutdown();

    /**
     * @brief Sets the engine's state (once per frame).
     * @param[in] s  the state
     */
    void setEngine( const EngineState &s );

    /**
     * @brief Sets the sounds of the place (once per frame).
     * @param[in] a  the ambience
     */
    void setAmbience( const Ambience &a );

    /**
     * @brief Plays an effect.
     * @param[in] type      which effect
     * @param[in] strength  0..1
     * @param[in] pan       -1 left .. 1 right
     */
    void play( SfxE type, float strength = 1.0f, float pan = 0.0f );

    /**
     * @brief Plays a musical cue on the soundtrack's next beat (at once if no generated music plays).
     * @param[in] type      which effect
     * @param[in] strength  0..1
     */
    void playOnBeat( SfxE type, float strength = 1.0f );

    /**
     * @brief Tells the soundtrack the game's mood.
     * @param[in] tension  0..1
     * @param[in] drive    0..1
     */
    void setMood( float tension, float drive );

    /**
     * @brief Sets the volumes.
     * @param[in] music  0..1
     * @param[in] sfx    0..1
     */
    void setVolumes( float music, float sfx );

    /**
     * @brief Starts a track: a generated one on a radio station (crossfading), or the next music file.
     * @param[in] seed     the track's seed
     * @param[in] station  0..StationCount-1 a generated station, StationCount the music files
     */
    void startMusic( uint64_t seed, int station );

    /** @brief Stops the soundtrack. */
    void stopMusic();

    /**
     * @brief Keeps file music streaming (once per frame).
     */
    void update();

    /**
     * @brief Whether music files replace the generated soundtrack.
     * @return true if files were found
     */
    bool hasFileMusic() const { return !m_files.empty(); }

    /**
     * @brief Renders a scripted drive (idle, full throttle through the gears, boost, lift-off, effects, music) into
     *        a WAV file without opening a device, and writes peak and RMS per second beside it -- levels are
     *        checked there, never through the speakers.
     * @param[in] path     the WAV file
     * @param[in] seconds  its length
     * @param[in] station  -1 the scripted drive with music, -2 the ambiences one after the other, else that station's music
     *                     alone
     * @return true if written
     */
    static bool renderTest( const std::string &path, float seconds, int station = -1 );

private:
    bool                     m_ok = false;          ///< the device is open
    AudioStream              m_stream{};            ///< the synthesis stream
    std::vector<std::string> m_files;               ///< music files found
    size_t                   m_fileIndex = 0;       ///< the next file
    Music                    m_music{};             ///< the playing file
    bool                     m_musicLoaded = false; ///< m_music is valid
    bool                     m_musicOn = false;     ///< the soundtrack should play
    bool                     m_fileMode = false;    ///< music files instead of the radio
    float                    m_musicVolume = 0.7f;  ///< 0..1
};

} // namespace sunstreak
