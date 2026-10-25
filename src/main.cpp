/**
 * @file main.cpp
 * @brief Entry point: settings, window (windowed by default on Windows, fullscreen on a Steam Deck), game loop.
 *
 * Command line: --windowed, --fullscreen, --seed N, --biome 0..37, --play (skip the title), --autopilot,
 * --distance M (start M metres down the road), --size WxH, --quit-after S, --screenshot file.png, --mute,
 * --audio-test file.wav (renders a scripted drive's sound offline and quits)
 *
 * @author Rene Weller
 */
#include "Config.h"
#include "Game.h"
#include "IconPng.h"

#include <raylib.h>

#include <algorithm>

/**
 * @brief Starts the game.
 * @param[in] argc  argument count
 * @param[in] argv  arguments
 * @return 0
 */
int main( int argc, char **argv )
{
    using namespace sunstreak;
    const LaunchOptions options = parseCommandLine( argc, argv );
    if ( !options.m_audioTest.empty() )
    {
        bool       ok = Audio::renderTest( options.m_audioTest, 22.0f );
        const auto dot = options.m_audioTest.rfind( '.' );
        const auto stem = options.m_audioTest.substr( 0, dot );
        for ( int s = 0; s < StationCount; ++s )
            ok = Audio::renderTest( stem + "-station" + std::to_string( s ) + ".wav", 50.0f, s ) && ok;
        ok = Audio::renderTest( stem + "-adaptive.wav", 40.0f, StationCount ) && ok;
        ok = Audio::renderTest( stem + "-ambience.wav", 35.0f, -2 ) && ok;
        return ok ? 0 : 1;
    }
    Settings   settings;
    const bool hadFile = loadSettings( settings );
    if ( !hadFile && runningOnSteamDeck() )
    {
        // The Deck: fullscreen, the balanced profile (60 frames, no shadow pass), the text a size larger for its screen.
        settings.m_display = DisplayModeE::FULLSCREEN;
        settings.m_fpsLimit = 60;
        settings.m_shadows = false;
        settings.m_renderScale = 0.85f;
        settings.m_profile = 2;
        settings.m_textSize = 1;
    }
    if ( options.m_forceDisplay >= 0 )
        settings.m_display = options.m_forceDisplay ? DisplayModeE::FULLSCREEN : DisplayModeE::WINDOWED;
    if ( !options.m_bench.empty() )
    {
        // Measuring: as fast as it goes.
        settings.m_vsync = false;
        settings.m_fpsLimit = 0;
    }

    SetTraceLogLevel( options.m_quitAfter > 0.0f ? LOG_INFO : LOG_WARNING );   // test runs report their numbers
    unsigned flags = FLAG_WINDOW_RESIZABLE;
    if ( settings.m_vsync ) flags |= FLAG_VSYNC_HINT;
    SetConfigFlags( flags );
    int w = options.m_width ? options.m_width : settings.m_windowWidth;
    int h = options.m_height ? options.m_height : settings.m_windowHeight;
    InitWindow( w, h, "Sunstreak" );
    const int     monitor = GetCurrentMonitor();
    const int     mw = GetMonitorWidth( monitor ), mh = GetMonitorHeight( monitor );
    const Vector2 mpos = GetMonitorPosition( monitor );
    if ( !options.m_width && mw > 0 && ( w > mw * 9 / 10 || h > mh * 9 / 10 ) )
    {
        w = mw * 4 / 5;
        h = mh * 4 / 5;
        SetWindowSize( w, h );
    }
    if ( mw > 0 )
        SetWindowPosition( static_cast<int>( mpos.x ) + ( mw - w ) / 2, static_cast<int>( mpos.y ) + ( mh - h ) / 2 );
    SetWindowMinSize( 640, 360 );
    SetExitKey( KEY_NULL );
    // The icon: the road running into the setting sun (tools/make_icon.py draws it; the program file has it from res/).
    Image icon = LoadImageFromMemory( ".png", IconPng, static_cast<int>( sizeof( IconPng ) ) );
    if ( icon.data ) SetWindowIcon( icon );
    UnloadImage( icon );
    if ( settings.m_display == DisplayModeE::FULLSCREEN ) ToggleBorderlessWindowed();
    SetTargetFPS( !options.m_bench.empty() ? 0
                  : settings.m_fpsLimit    ? settings.m_fpsLimit
                                           : ( settings.m_vsync ? 0 : 240 ) );

    {
        Game game( settings, options );
        game.init();
        while ( !WindowShouldClose() && !game.wantsQuit() )
        {
            const float dt = std::min( GetFrameTime(), 0.05f );
            game.frame( dt );
        }
        if ( !options.m_width && !IsWindowState( FLAG_BORDERLESS_WINDOWED_MODE ) )
        {
            settings.m_windowWidth = GetScreenWidth();
            settings.m_windowHeight = GetScreenHeight();
        }
        if ( !options.m_width ) saveSettings( settings );
        game.shutdown();
    }
    CloseWindow();
    return 0;
}
