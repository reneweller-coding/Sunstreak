/**
 * @file Haptics.h
 * @brief Gamepad rumble through SDL2: raylib's GLFW reads the pads but cannot make them vibrate.
 *
 * Only SDL's game controller subsystem runs (no window, no sound). Hits are pulses that fade out, kerbs and gravel a
 * steady buzz that is set every frame. On the Steam Deck, Steam Input passes the rumble to the Deck's motors.
 *
 * @author Rene Weller
 */
#pragma once

namespace sunstreak
{

/** @brief Rumbles the first game controller SDL finds. */
class Haptics
{
public:
    /** @brief Closes the pad and SDL. */
    ~Haptics();

    /**
     * @brief Starts SDL's game controller subsystem.
     * @return true if it runs
     */
    bool init();

    /**
     * @brief Adds a pulse that fades out.
     * @param[in] low      0..1, the heavy motor
     * @param[in] high     0..1, the light motor
     * @param[in] seconds  its length
     */
    void pulse( float low, float high, float seconds );

    /**
     * @brief Sets the steady buzz for this frame (it ends unless set again).
     * @param[in] low   0..1
     * @param[in] high  0..1
     */
    void steady( float low, float high );

    /**
     * @brief Finds pads that were plugged in and sends the strength.
     * @param[in] dt       seconds since the last frame
     * @param[in] enabled  the VIBRATION setting and a running race
     */
    void update( float dt, bool enabled );

private:
    /** @brief A fading pulse. */
    struct Pulse
    {
        float m_low = 0.0f;      ///< heavy motor at the start
        float m_high = 0.0f;     ///< light motor at the start
        float m_left = 0.0f;     ///< seconds left
        float m_length = 1.0f;   ///< its length
    };

    bool  m_ok = false;                       ///< SDL runs
    void *m_pad = nullptr;                    ///< the SDL_GameController
    float m_scan = 0.0f;                      ///< seconds until the next look for a pad
    Pulse m_pulses[8];                        ///< fading pulses
    float m_steadyLow = 0.0f;                 ///< this frame's buzz, the low motor
    float m_steadyHigh = 0.0f;                ///< and the high one
    int   m_sentLow = 0;                      ///< what the low motor was told last
    int   m_sentHigh = 0;                     ///< and the high one
    float m_resend = 0.0f;                    ///< seconds until the strength is sent again
};

} // namespace sunstreak
