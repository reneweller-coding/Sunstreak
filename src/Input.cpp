/**
 * @file Input.cpp
 * @brief Device reading.
 * @author reneweller
 */
#include "Input.h"

namespace sunstreak
{

void Input::update()
{
    m_pad = -1;
    for( int i = 0; i < 4; ++i )
    {
        if( IsGamepadAvailable( i ) )
        {
            m_pad = i;
            break;
        }
    }
    m_gamepad = m_pad >= 0;

    CarInput c;
    const bool up = IsKeyDown( KEY_UP ) || IsKeyDown( KEY_W );
    const bool down = IsKeyDown( KEY_DOWN ) || IsKeyDown( KEY_S );
    const bool left = IsKeyDown( KEY_LEFT ) || IsKeyDown( KEY_A );
    const bool right = IsKeyDown( KEY_RIGHT ) || IsKeyDown( KEY_D );
    c.m_throttle = up ? 1.0f : 0.0f;
    c.m_brake = down ? 1.0f : 0.0f;
    c.m_steer = ( right ? 1.0f : 0.0f ) - ( left ? 1.0f : 0.0f );
    c.m_boost = IsKeyDown( KEY_SPACE ) || IsKeyDown( KEY_LEFT_SHIFT ) || IsKeyDown( KEY_RIGHT_SHIFT );
    c.m_shiftUp = IsKeyPressed( KEY_E );
    c.m_shiftDown = IsKeyPressed( KEY_Q );

    MenuInput m;
    m.m_up = IsKeyPressed( KEY_UP ) || IsKeyPressed( KEY_W );
    m.m_down = IsKeyPressed( KEY_DOWN ) || IsKeyPressed( KEY_S );
    m.m_left = IsKeyPressed( KEY_LEFT ) || IsKeyPressed( KEY_A );
    m.m_right = IsKeyPressed( KEY_RIGHT ) || IsKeyPressed( KEY_D );
    m.m_accept = IsKeyPressed( KEY_ENTER ) || IsKeyPressed( KEY_KP_ENTER ) || IsKeyPressed( KEY_SPACE );
    m.m_back = IsKeyPressed( KEY_ESCAPE ) || IsKeyPressed( KEY_BACKSPACE );
    m_pause = IsKeyPressed( KEY_ESCAPE ) || IsKeyPressed( KEY_P );
    m_camera = IsKeyPressed( KEY_C );
    m_stats = IsKeyPressed( KEY_F3 );
    m_nextStation = IsKeyPressed( KEY_M );
    m_nextTrack = IsKeyPressed( KEY_N );
    m_reload = IsKeyPressed( KEY_F5 );
    m_viewer = IsKeyPressed( KEY_F6 );
    const bool alt = IsKeyDown( KEY_LEFT_ALT ) || IsKeyDown( KEY_RIGHT_ALT );
    m_fullscreen = IsKeyPressed( KEY_F11 ) || ( alt && IsKeyPressed( KEY_ENTER ) );
    if( alt && IsKeyPressed( KEY_ENTER ) ) m.m_accept = false;

    if( m_pad >= 0 )
    {
        const int p = m_pad;
        float     sx = GetGamepadAxisMovement( p, GAMEPAD_AXIS_LEFT_X );
        const float dead = 0.14f;
        sx = std::abs( sx ) < dead ? 0.0f : ( sx - std::copysign( dead, sx ) ) / ( 1.0f - dead );
        sx = std::copysign( std::pow( std::abs( sx ), 1.4f ), sx );
        if( IsGamepadButtonDown( p, GAMEPAD_BUTTON_LEFT_FACE_LEFT ) ) sx = -1.0f;
        if( IsGamepadButtonDown( p, GAMEPAD_BUTTON_LEFT_FACE_RIGHT ) ) sx = 1.0f;
        if( std::abs( sx ) > std::abs( c.m_steer ) ) c.m_steer = sx;

        // Triggers rest at -1; until one has been seen at rest its value is not trusted (some drivers start at 0).
        const float rt = GetGamepadAxisMovement( p, GAMEPAD_AXIS_RIGHT_TRIGGER );
        const float lt = GetGamepadAxisMovement( p, GAMEPAD_AXIS_LEFT_TRIGGER );
        if( rt < -0.5f ) m_rtSeen = true;
        if( lt < -0.5f ) m_ltSeen = true;
        const float throttle = m_rtSeen ? clampf( ( rt + 1.0f ) * 0.5f, 0.0f, 1.0f ) : 0.0f;
        const float brake = m_ltSeen ? clampf( ( lt + 1.0f ) * 0.5f, 0.0f, 1.0f ) : 0.0f;
        c.m_throttle = std::max( { c.m_throttle, throttle > 0.05f ? throttle : 0.0f,
                                   IsGamepadButtonDown( p, GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) ? 1.0f : 0.0f } );
        c.m_brake = std::max( { c.m_brake, brake > 0.05f ? brake : 0.0f,
                                IsGamepadButtonDown( p, GAMEPAD_BUTTON_RIGHT_FACE_LEFT ) ? 1.0f : 0.0f } );
        c.m_boost = c.m_boost || IsGamepadButtonDown( p, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT );
        c.m_shiftUp = c.m_shiftUp || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_RIGHT_TRIGGER_1 );
        c.m_shiftDown = c.m_shiftDown || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_LEFT_TRIGGER_1 );

        m.m_up = m.m_up || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_LEFT_FACE_UP );
        m.m_down = m.m_down || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_LEFT_FACE_DOWN );
        m.m_left = m.m_left || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_LEFT_FACE_LEFT );
        m.m_right = m.m_right || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_LEFT_FACE_RIGHT );
        m.m_accept = m.m_accept || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_RIGHT_FACE_DOWN );
        m.m_back = m.m_back || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT );
        m_pause = m_pause || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_MIDDLE_RIGHT );
        m_camera = m_camera || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_RIGHT_FACE_UP );
        m_nextStation = m_nextStation || IsGamepadButtonPressed( p, GAMEPAD_BUTTON_MIDDLE_LEFT );

        // The stick moves through menus too, with a repeat.
        const float sy = GetGamepadAxisMovement( p, GAMEPAD_AXIS_LEFT_Y );
        const float smx = GetGamepadAxisMovement( p, GAMEPAD_AXIS_LEFT_X );
        int         dir = 0;
        if( sy < -0.6f ) dir = 1;
        else if( sy > 0.6f ) dir = 2;
        else if( smx < -0.6f ) dir = 3;
        else if( smx > 0.6f ) dir = 4;
        m_repeat -= GetFrameTime();
        if( dir != 0 && ( dir != m_stickDir || m_repeat <= 0.0f ) )
        {
            m_repeat = dir != m_stickDir ? 0.4f : 0.15f;
            m.m_up = m.m_up || dir == 1;
            m.m_down = m.m_down || dir == 2;
            m.m_left = m.m_left || dir == 3;
            m.m_right = m.m_right || dir == 4;
        }
        m_stickDir = dir;
    }
    m_car = c;
    m_menu = m;
}

} // namespace sunstreak
