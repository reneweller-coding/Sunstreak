/**
 * @file Input.h
 * @brief Keyboard and gamepad (the Steam Deck's controls arrive as a gamepad) mapped to driving and menu actions.
 *
 * Driving: throttle RT / A / Up / W, brake LT / X / Down / S, steer left stick / d-pad / arrows / A D,
 * boost B / Space / Shift, gears LB RB / Q E, camera Y / C, pause Start / Esc / P,
 * radio station M / View, next track N.
 *
 * @author reneweller
 */
#pragma once

#include "Car.h"

namespace sunstreak
{

/** @brief Menu actions of one frame. */
struct MenuInput
{
    bool m_up = false;       ///< move up
    bool m_down = false;     ///< move down
    bool m_left = false;     ///< change value down
    bool m_right = false;    ///< change value up
    bool m_accept = false;   ///< confirm
    bool m_back = false;     ///< back / cancel
};

/** @brief The input devices. */
class Input
{
public:
    /** @brief Reads the devices (once per frame). */
    void update();

    /**
     * @brief The driving controls.
     * @return the car input
     */
    CarInput car() const { return m_car; }

    /**
     * @brief The menu controls.
     * @return the menu input
     */
    MenuInput menu() const { return m_menu; }

    bool m_pause = false;        ///< pause pressed
    bool m_camera = false;       ///< camera switch pressed
    bool m_stats = false;        ///< F3: the performance overlay
    bool m_fullscreen = false;   ///< F11 or Alt+Enter pressed
    bool m_nextStation = false;  ///< M or the View button: the next radio station
    bool m_nextTrack = false;    ///< N: the next track
    bool m_gamepad = false;      ///< a gamepad is connected

private:
    CarInput  m_car;             ///< driving
    MenuInput m_menu;            ///< menus
    int       m_pad = -1;        ///< the gamepad in use
    bool      m_rtSeen = false;  ///< the right trigger has reported its rest position
    bool      m_ltSeen = false;  ///< the left trigger too
    float     m_repeat = 0.0f;   ///< stick repeat timer for menus
    int       m_stickDir = 0;    ///< last stick direction in menus
};

} // namespace sunstreak
