/**
 * @file Config.cpp
 * @brief Settings file, command line and the Steam Deck check.
 * @author reneweller
 */
#include "Config.h"

#include "Biome.h"
#include "Festival.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sunstreak
{

namespace
{

/**
 * @brief An environment variable or an empty string.
 * @param[in] name  the variable
 * @return its value
 */
std::string envOr( const char *name )
{
    const char *v = std::getenv( name );
    return v ? std::string( v ) : std::string();
}

/**
 * @brief The settings file's path.
 * @return configDirectory() + settings.ini
 */
std::string settingsPath()
{
    return configDirectory() + "/settings.ini";
}

} // namespace

std::string configDirectory()
{
#ifdef _WIN32
    std::string base = envOr( "APPDATA" );
    if( base.empty() ) base = ".";
    std::string dir = base + "/Sunstreak";
#else
    std::string base = envOr( "XDG_CONFIG_HOME" );
    if( base.empty() )
    {
        const std::string home = envOr( "HOME" );
        base = home.empty() ? std::string( "." ) : home + "/.config";
    }
    std::string dir = base + "/sunstreak";
#endif
    std::error_code ec;
    std::filesystem::create_directories( dir, ec );
    return dir;
}

bool loadSettings( Settings &settings )
{
    std::ifstream in( settingsPath() );
    if( !in ) return false;
    std::string line;
    while( std::getline( in, line ) )
    {
        const auto eq = line.find( '=' );
        if( eq == std::string::npos || line[0] == '#' ) continue;
        const std::string key = line.substr( 0, eq );
        const std::string val = line.substr( eq + 1 );
        try
        {
            if( key == "display" ) settings.m_display = val == "fullscreen" ? DisplayModeE::FULLSCREEN : DisplayModeE::WINDOWED;
            else if( key == "window_width" ) settings.m_windowWidth = std::max( 640, std::stoi( val ) );
            else if( key == "window_height" ) settings.m_windowHeight = std::max( 360, std::stoi( val ) );
            else if( key == "vsync" ) settings.m_vsync = val == "1";
            else if( key == "render_scale" ) settings.m_renderScale = std::clamp( std::stof( val ), 0.5f, 2.0f );
            else if( key == "music_volume" ) settings.m_musicVolume = std::clamp( std::stof( val ), 0.0f, 1.0f );
            else if( key == "sfx_volume" ) settings.m_sfxVolume = std::clamp( std::stof( val ), 0.0f, 1.0f );
            else if( key == "units" ) settings.m_units = val == "mph" ? UnitsE::MPH : UnitsE::KMH;
            else if( key == "manual_gears" ) settings.m_manualGears = val == "1";
            else if( key == "camera" ) settings.m_camera = std::clamp( std::stoi( val ), 0, 2 );
            else if( key == "speed_blur" ) settings.m_speedBlur = val == "1";
            else if( key == "radio" ) settings.m_station = std::clamp( std::stoi( val ), -1, 64 );
            else if( key == "difficulty" ) settings.m_difficulty = std::clamp( std::stoi( val ), 0, 2 );
            else if( key == "shadows" ) settings.m_shadows = val == "1";
            else if( key == "fps_limit" ) settings.m_fpsLimit = std::clamp( std::stoi( val ), 0, 240 );
            else if( key == "vibration" ) settings.m_vibration = val == "1";
            else if( key == "scenery" ) settings.m_scenery = std::clamp( std::stoi( val ), 0, 1 );
            else if( key == "time_of_day" ) settings.m_timeOfDay = std::clamp( std::stoi( val ), 0, 5 );
            else if( key == "weather" ) settings.m_weather = std::clamp( std::stoi( val ), 0, 1 );
            else if( key == "seasons" ) settings.m_seasons = std::clamp( std::stoi( val ), 0, 2 );
            else if( key == "steer_assist" ) settings.m_steerAssist = std::clamp( std::stoi( val ), 0, 2 );
            else if( key == "brake_assist" ) settings.m_brakeAssist = val == "1";
            else if( key == "colour_blind" ) settings.m_colourBlind = val == "1";
            else if( key == "text_size" ) settings.m_textSize = std::clamp( std::stoi( val ), 0, 2 );
            else if( key == "profile" ) settings.m_profile = std::clamp( std::stoi( val ), 0, 3 );
            else if( key == "car" ) settings.m_car = std::max( 0, std::stoi( val ) );
            else if( key == "paint" ) settings.m_paint = std::max( 0, std::stoi( val ) );
            else if( key == "rims" ) settings.m_rims = std::max( 0, std::stoi( val ) );
            else if( key == "decal" ) settings.m_decal = std::max( 0, std::stoi( val ) );
            else if( key == "decal_color" ) settings.m_decalColor = std::max( 0, std::stoi( val ) );
            else if( key == "best_score" ) settings.m_bestScore = std::stoll( val );
            else if( key == "best_distance" ) settings.m_bestDistance = std::stod( val );
        }
        catch( ... )
        {
            // A broken value keeps its default.
        }
    }
    return true;
}

bool saveSettings( const Settings &settings )
{
    std::ofstream out( settingsPath() );
    if( !out ) return false;
    out << "# Sunstreak settings\n";
    out << "display=" << ( settings.m_display == DisplayModeE::FULLSCREEN ? "fullscreen" : "windowed" ) << "\n";
    out << "window_width=" << settings.m_windowWidth << "\n";
    out << "window_height=" << settings.m_windowHeight << "\n";
    out << "vsync=" << ( settings.m_vsync ? 1 : 0 ) << "\n";
    out << "render_scale=" << settings.m_renderScale << "\n";
    out << "music_volume=" << settings.m_musicVolume << "\n";
    out << "sfx_volume=" << settings.m_sfxVolume << "\n";
    out << "units=" << ( settings.m_units == UnitsE::MPH ? "mph" : "kmh" ) << "\n";
    out << "manual_gears=" << ( settings.m_manualGears ? 1 : 0 ) << "\n";
    out << "camera=" << settings.m_camera << "\n";
    out << "speed_blur=" << ( settings.m_speedBlur ? 1 : 0 ) << "\n";
    out << "radio=" << settings.m_station << "\n";
    out << "difficulty=" << settings.m_difficulty << "\n";
    out << "shadows=" << ( settings.m_shadows ? 1 : 0 ) << "\n";
    out << "fps_limit=" << settings.m_fpsLimit << "\n";
    out << "vibration=" << ( settings.m_vibration ? 1 : 0 ) << "\n";
    out << "scenery=" << settings.m_scenery << "\n";
    out << "time_of_day=" << settings.m_timeOfDay << "\n";
    out << "weather=" << settings.m_weather << "\n";
    out << "seasons=" << settings.m_seasons << "\n";
    out << "steer_assist=" << settings.m_steerAssist << "\n";
    out << "brake_assist=" << ( settings.m_brakeAssist ? 1 : 0 ) << "\n";
    out << "colour_blind=" << ( settings.m_colourBlind ? 1 : 0 ) << "\n";
    out << "text_size=" << settings.m_textSize << "\n";
    out << "profile=" << settings.m_profile << "\n";
    out << "car=" << settings.m_car << "\n";
    out << "paint=" << settings.m_paint << "\n";
    out << "rims=" << settings.m_rims << "\n";
    out << "decal=" << settings.m_decal << "\n";
    out << "decal_color=" << settings.m_decalColor << "\n";
    out << "best_score=" << settings.m_bestScore << "\n";
    out << "best_distance=" << settings.m_bestDistance << "\n";
    return static_cast<bool>( out );
}

LaunchOptions parseCommandLine( int argc, char **argv )
{
    LaunchOptions o;
    for( int i = 1; i < argc; ++i )
    {
        const std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? std::string( argv[++i] ) : std::string(); };
        if( a == "--fullscreen" ) o.m_forceDisplay = 1;
        else if( a == "--windowed" ) o.m_forceDisplay = 0;
        else if( a == "--seed" ) o.m_seed = std::strtoull( next().c_str(), nullptr, 0 );
        else if( a == "--biome" )
        {
            // A number, or a place's ID.
            const std::string v = next();
            o.m_startBiome = !v.empty() && std::isdigit( static_cast<unsigned char>( v[0] ) ) ? std::atoi( v.c_str() ) : biomeId( v );
        }
        else if( a == "--scenery" )
        {
            const std::string what = next();
            // A number, or a place's ID from the data files.
            const bool number = !what.empty() && std::isdigit( static_cast<unsigned char>( what[0] ) );
            o.m_scenery = what == "world" ? -2 : what == "mixed" ? -1 : number ? std::max( 0, std::atoi( what.c_str() ) ) : std::max( 0, biomeId( what ) );
        }
        else if( a == "--weather" )
        {
            static const char *kinds[] = { "clear", "rain", "storm", "fog", "snow", "sand" };
            const std::string  what = next();
            for( int k = 0; k < 6; ++k )
                if( what == kinds[k] ) o.m_weather = k;
        }
        else if( a == "--time" )
        {
            static const char *times[] = { "place", "clock", "dawn", "day", "dusk", "night" };
            const std::string  what = next();
            for( int k = 0; k < 6; ++k )
                if( what == times[k] ) o.m_timeOfDay = k;
        }
        else if( a == "--rally" ) o.m_rally = std::atoi( next().c_str() ) - 1;
        else if( a == "--daily" ) o.m_daily = true;
        else if( a == "--action" )
        {
            o.m_action = next();
            o.m_actionAt = static_cast<float>( std::atof( next().c_str() ) );
        }
        else if( a == "--autopilot" ) o.m_autopilot = true;
        else if( a == "--play" ) o.m_skipMenu = true;
        else if( a == "--mute" ) o.m_mute = true;
        else if( a == "--audio-test" ) o.m_audioTest = next();
        else if( a == "--find" )
        {
            const std::string what = next();
            o.m_find = what == "tunnel" ? 1 : what == "bridge" ? 2 : what == "landmark" ? 3 : what == "landmark2" ? 4 : what == "fork" ? 5 : what == "sea" ? 6 : what == "seain" ? 7 : what == "seaout" ? 8 : what == "ferry" ? 9 : 0;
        }
        else if( a == "--camera" ) o.m_camera = std::clamp( std::atoi( next().c_str() ), 0, 2 );
        else if( a == "--code" ) o.m_code = next();
        else if( a == "--championship" ) o.m_championship = true;
        else if( a == "--screen" ) o.m_screen = next();
        else if( a == "--shape" ) o.m_shape = next();
        else if( a == "--date" ) o.m_season = dateFromText( next() );
        else if( a == "--season" )
        {
            const std::string what = next();
            o.m_season = what == "winter" ? 0.04f : what == "spring" ? 0.31f : what == "summer" ? 0.55f : what == "autumn" ? 0.8f : -1.0f;
        }
        else if( a == "--quit-after" ) o.m_quitAfter = static_cast<float>( std::atof( next().c_str() ) );
        else if( a == "--screenshot" ) o.m_screenshot = next();
        else if( a == "--distance" ) o.m_startDistance = static_cast<float>( std::atof( next().c_str() ) );
        else if( a == "--size" )
        {
            std::istringstream s( next() );
            char x = 0;
            s >> o.m_width >> x >> o.m_height;
        }
    }
    return o;
}

bool runningOnSteamDeck()
{
#ifdef _WIN32
    return false;
#else
    if( envOr( "SteamDeck" ) == "1" || !envOr( "GAMESCOPE_WAYLAND_DISPLAY" ).empty() ) return true;
    std::ifstream board( "/sys/devices/virtual/dmi/id/board_vendor" );
    std::string vendor;
    std::getline( board, vendor );
    std::ifstream product( "/sys/devices/virtual/dmi/id/product_name" );
    std::string name;
    std::getline( product, name );
    return vendor.find( "Valve" ) != std::string::npos && ( name == "Jupiter" || name == "Galileo" );
#endif
}

} // namespace sunstreak
