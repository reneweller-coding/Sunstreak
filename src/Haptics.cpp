/**
 * @file Haptics.cpp
 * @brief Gamepad rumble through SDL2.
 * @author Rene Weller
 */
#include "Haptics.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <algorithm>

namespace sunstreak
{

Haptics::~Haptics()
{
    if ( m_pad ) SDL_GameControllerClose( static_cast<SDL_GameController *>( m_pad ) );
    if ( m_ok ) SDL_Quit();
}

bool Haptics::init()
{
    SDL_SetMainReady();
    // Without a window of SDL's own the pads must be read on SDL's thread and while the game is in the background.
    SDL_SetHint( SDL_HINT_JOYSTICK_THREAD, "1" );
    SDL_SetHint( SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1" );
    m_ok = SDL_Init( SDL_INIT_GAMECONTROLLER ) == 0;
    return m_ok;
}

void Haptics::pulse( float low, float high, float seconds )
{
    Pulse *slot = &m_pulses[0];
    for ( Pulse &p : m_pulses )
        if ( p.m_left < slot->m_left ) slot = &p;
    *slot = Pulse{ std::clamp( low, 0.0f, 1.0f ), std::clamp( high, 0.0f, 1.0f ), seconds, std::max( 0.01f, seconds ) };
}

void Haptics::steady( float low, float high )
{
    m_steadyLow = std::max( m_steadyLow, std::clamp( low, 0.0f, 1.0f ) );
    m_steadyHigh = std::max( m_steadyHigh, std::clamp( high, 0.0f, 1.0f ) );
}

void Haptics::update( float dt, bool enabled )
{
    float low = m_steadyLow, high = m_steadyHigh;
    m_steadyLow = m_steadyHigh = 0.0f;
    for ( Pulse &p : m_pulses )
    {
        if ( p.m_left <= 0.0f ) continue;
        const float k = p.m_left / p.m_length;
        low = std::max( low, p.m_low * k );
        high = std::max( high, p.m_high * k );
        p.m_left -= dt;
        if ( !enabled ) p.m_left = 0.0f;
    }
    if ( !enabled ) low = high = 0.0f;
    if ( !m_ok ) return;

    // Pads come and go: drop a lost one, look for a new one once a second.
    SDL_PumpEvents();
    SDL_FlushEvents( SDL_FIRSTEVENT, SDL_LASTEVENT );
    auto *pad = static_cast<SDL_GameController *>( m_pad );
    if ( pad && !SDL_GameControllerGetAttached( pad ) )
    {
        SDL_GameControllerClose( pad );
        pad = nullptr;
    }
    m_scan -= dt;
    if ( !pad && m_scan <= 0.0f )
    {
        m_scan = 1.0f;
        for ( int i = 0; i < SDL_NumJoysticks() && !pad; ++i )
            if ( SDL_IsGameController( i ) ) pad = SDL_GameControllerOpen( i );
        m_sentLow = m_sentHigh = -1;
    }
    m_pad = pad;
    if ( !pad ) return;

    // In steps of 1/32, sent when it changes and refreshed while it lasts.
    const int l = static_cast<int>( low * 32.0f + 0.5f ), h = static_cast<int>( high * 32.0f + 0.5f );
    m_resend -= dt;
    if ( l != m_sentLow || h != m_sentHigh || ( ( l || h ) && m_resend <= 0.0f ) )
    {
        SDL_GameControllerRumble( pad, static_cast<Uint16>( std::min( 65535, l * 2048 ) ),
                                  static_cast<Uint16>( std::min( 65535, h * 2048 ) ), l || h ? 200 : 0 );
        m_sentLow = l;
        m_sentHigh = h;
        m_resend = 0.1f;
    }
}

}   // namespace sunstreak
