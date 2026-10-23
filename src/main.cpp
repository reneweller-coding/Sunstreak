/**
 * @file main.cpp
 * @brief Entry point: settings, window (windowed by default on Windows, fullscreen on a Steam Deck), game loop.
 *
 * Command line: --windowed, --fullscreen, --seed N, --biome 0..37, --play (skip the title), --autopilot,
 * --distance M (start M metres down the road), --size WxH, --quit-after S, --screenshot file.png, --mute,
 * --audio-test file.wav (renders a scripted drive's sound offline and quits)
 *
 * @author reneweller
 */
#include "Config.h"
#include "Game.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

namespace
{

/**
 * @brief Draws the window icon: a striped sunset sun.
 * @return the image (the caller unloads it)
 */
Image makeIcon()
{
    Image img = GenImageColor( 64, 64, BLANK );
    for( int y = 0; y < 64; ++y )
    {
        const float yc = static_cast<float>( y ) - 31.5f;
        if( y > 34 && ( y / 4 ) % 2 == 1 ) continue;
        const float hw = std::sqrt( std::max( 0.0f, 30.0f * 30.0f - yc * yc ) );
        const float t = static_cast<float>( y ) / 63.0f;
        const Color c = { 255, static_cast<unsigned char>( 214 - 170 * t ), static_cast<unsigned char>( 80 + 60 * t ), 255 };
        ImageDrawRectangle( &img, static_cast<int>( 32.0f - hw ), y, static_cast<int>( hw * 2.0f ), 1, c );
    }
    return img;
}

} // namespace

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
    if( !options.m_audioTest.empty() )
    {
        bool       ok = Audio::renderTest( options.m_audioTest, 22.0f );
        const auto dot = options.m_audioTest.rfind( '.' );
        const auto stem = options.m_audioTest.substr( 0, dot );
        for( int s = 0; s < StationCount; ++s ) ok = Audio::renderTest( stem + "-station" + std::to_string( s ) + ".wav", 50.0f, s ) && ok;
        ok = Audio::renderTest( stem + "-adaptive.wav", 40.0f, StationCount ) && ok;
        return ok ? 0 : 1;
    }
    Settings            settings;
    const bool          hadFile = loadSettings( settings );
    if( !hadFile && runningOnSteamDeck() )
    {
        // The Deck: fullscreen, 60 frames, no shadow pass (the APU's budget goes to the frame rate).
        settings.m_display = DisplayModeE::FULLSCREEN;
        settings.m_fpsLimit = 60;
        settings.m_shadows = false;
    }
    if( options.m_forceDisplay >= 0 ) settings.m_display = options.m_forceDisplay ? DisplayModeE::FULLSCREEN : DisplayModeE::WINDOWED;

    SetTraceLogLevel( LOG_WARNING );
    unsigned flags = FLAG_WINDOW_RESIZABLE;
    if( settings.m_vsync ) flags |= FLAG_VSYNC_HINT;
    SetConfigFlags( flags );
    int w = options.m_width ? options.m_width : settings.m_windowWidth;
    int h = options.m_height ? options.m_height : settings.m_windowHeight;
    InitWindow( w, h, "Sunstreak" );
    const int     monitor = GetCurrentMonitor();
    const int     mw = GetMonitorWidth( monitor ), mh = GetMonitorHeight( monitor );
    const Vector2 mpos = GetMonitorPosition( monitor );
    if( !options.m_width && mw > 0 && ( w > mw * 9 / 10 || h > mh * 9 / 10 ) )
    {
        w = mw * 4 / 5;
        h = mh * 4 / 5;
        SetWindowSize( w, h );
    }
    if( mw > 0 ) SetWindowPosition( static_cast<int>( mpos.x ) + ( mw - w ) / 2, static_cast<int>( mpos.y ) + ( mh - h ) / 2 );
    SetWindowMinSize( 640, 360 );
    SetExitKey( KEY_NULL );
    Image icon = makeIcon();
    SetWindowIcon( icon );
    UnloadImage( icon );
    if( settings.m_display == DisplayModeE::FULLSCREEN ) ToggleBorderlessWindowed();
    SetTargetFPS( settings.m_fpsLimit ? settings.m_fpsLimit : ( settings.m_vsync ? 0 : 240 ) );

    {
        Game game( settings, options );
        game.init();
        while( !WindowShouldClose() && !game.wantsQuit() )
        {
            const float dt = std::min( GetFrameTime(), 0.05f );
            game.frame( dt );
        }
        if( !options.m_width && !IsWindowState( FLAG_BORDERLESS_WINDOWED_MODE ) )
        {
            settings.m_windowWidth = GetScreenWidth();
            settings.m_windowHeight = GetScreenHeight();
        }
        if( !options.m_width ) saveSettings( settings );
        game.shutdown();
    }
    CloseWindow();
    return 0;
}
