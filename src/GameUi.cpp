/**
 * @file GameUi.cpp
 * @brief The game's screens: menus, settings, Track Lab, code and initials entry, records, results, standings, HUD.
 * @author reneweller
 */
#include "Game.h"

#include "UiCommon.h"
#include "VectorFont.h"

#include <rlgl.h>

#include <cctype>
#include <cstring>

namespace sunstreak
{

using namespace ui;

namespace
{

/** @brief One piece of a hint line: text, or a button drawn as a glyph. */
struct HintPiece
{
    std::string m_text;    ///< the words, or the button's name
    bool        m_glyph;   ///< a button
};

/**
 * @brief Splits a hint like "[A] SELECT   [B] BACK" into words and buttons.
 * @param[in] text  the hint
 * @return its pieces
 */
std::vector<HintPiece> hintPieces( std::string_view text )
{
    std::vector<HintPiece> out;
    size_t                 i = 0;
    while( i < text.size() )
    {
        if( text[i] == '[' )
        {
            const size_t end = text.find( ']', i );
            if( end == std::string_view::npos ) break;
            out.push_back( { std::string( text.substr( i + 1, end - i - 1 ) ), true } );
            i = end + 1;
        }
        else
        {
            const size_t end = std::min( text.find( '[', i ), text.size() );
            out.push_back( { std::string( text.substr( i, end - i ) ), false } );
            i = end;
        }
    }
    return out;
}

/**
 * @brief The width of a button glyph.
 * @param[in] name  the button
 * @param[in] size  the hint's text size
 * @return pixels
 */
float glyphWidth( const std::string &name, float size )
{
    if( name == "A" || name == "B" || name == "X" || name == "Y" || name == "STICK" ) return size * 1.6f;
    if( name == "DPAD" ) return size * 1.7f;
    return textWidth( name, size * 0.78f ) + size * 1.0f;
}

/**
 * @brief Draws a button glyph: the face buttons in their colours, shoulders and keys as labelled caps, the d-pad as a
 *        cross, the stick as a ring.
 * @param[in] name  the button
 * @param[in] x     left
 * @param[in] y     top of the hint's capitals
 * @param[in] size  the hint's text size
 * @param[in] a     opacity
 */
void drawGlyph( const std::string &name, float x, float y, float size, unsigned char a )
{
    const float   w = glyphWidth( name, size ), cy = y + size * 0.5f;
    const Vector2 c = { x + w * 0.5f, cy };
    if( name == "A" || name == "B" || name == "X" || name == "Y" )
    {
        const Color fill = name == "A" ? Color{ 90, 170, 70, a } : name == "B" ? Color{ 205, 60, 50, a } : name == "X" ? Color{ 60, 110, 205, a } : Color{ 225, 185, 40, a };
        DrawCircleV( c, size * 0.78f, fill );
        drawText( name, c.x, cy - size * 0.36f, size * 0.72f, name == "Y" ? Color{ 30, 30, 30, a } : Color{ 255, 255, 255, a }, AlignE::CENTER, 0.16f );
        return;
    }
    if( name == "DPAD" )
    {
        const float s = size * 0.8f, t = size * 0.28f;
        DrawRectangleRec( Rectangle{ c.x - s, c.y - t, s * 2.0f, t * 2.0f }, Color{ 220, 224, 235, a } );
        DrawRectangleRec( Rectangle{ c.x - t, c.y - s, t * 2.0f, s * 2.0f }, Color{ 220, 224, 235, a } );
        return;
    }
    if( name == "STICK" )
    {
        DrawRing( c, size * 0.55f, size * 0.78f, 0.0f, 360.0f, 24, Color{ 220, 224, 235, a } );
        DrawCircleV( c, size * 0.3f, Color{ 220, 224, 235, a } );
        return;
    }
    const Rectangle r = { x, y - size * 0.3f, w, size * 1.6f };
    DrawRectangleRounded( r, 0.4f, 6, Color{ 60, 64, 78, a } );
    DrawRectangleRoundedLinesEx( r, 0.4f, 6, 1.2f, Color{ 220, 224, 235, a } );
    drawText( name, c.x, cy - size * 0.39f, size * 0.78f, Color{ 245, 246, 250, a }, AlignE::CENTER, 0.14f );
}

} // namespace

void ui::drawHint( std::string_view text, float x, float y, float size, Color color, AlignE align )
{
    const std::vector<HintPiece> pieces = hintPieces( text );
    const float                  ts = textScale();
    float                        width = 0.0f;
    for( const HintPiece &p : pieces ) width += p.m_glyph ? glyphWidth( p.m_text, size * ts ) + size * 0.35f : textWidth( p.m_text, size );
    float pen = align == AlignE::CENTER ? x - width * 0.5f : align == AlignE::RIGHT ? x - width : x;
    for( const HintPiece &p : pieces )
    {
        if( p.m_glyph )
        {
            drawGlyph( p.m_text, pen, y, size * ts, color.a );
            pen += glyphWidth( p.m_text, size * ts ) + size * 0.35f;
        }
        else
        {
            drawText( p.m_text, pen, y, size, color, AlignE::LEFT );
            pen += textWidth( p.m_text, size );
        }
    }
}


const char *Game::settingLabel( SettingE item ) const
{
    static const char *labels[] = { "PROFILE",      "DISPLAY",      "3D RESOLUTION", "SHADOWS",      "VSYNC",     "FRAME LIMIT", "DIFFICULTY",
                                    "SCENERY",      "TIME OF DAY",   "WEATHER",      "SEASONS",      "POLICE",       "STEER ASSIST", "BRAKE ASSIST", "COLOUR BLIND",
                                    "TEXT SIZE",    "GEARS",         "SPEEDOMETER",  "CAMERA",    "SPEED BLUR",  "VIBRATION",
                                    "MUSIC",        "EFFECTS",       "RADIO",        "RESET SETTINGS", "ERASE PROGRESS", "BACK" };
    return labels[static_cast<int>( item )];
}

std::string Game::settingValue( SettingE item ) const
{
    auto bar = []( float v )
    {
        std::string s;
        const int   n = static_cast<int>( std::round( v * 10.0f ) );
        for( int i = 0; i < 10; ++i ) s += i < n ? '#' : '-';
        return s;
    };
    static const char *cams[] = { "CHASE", "FAR CHASE", "BUMPER", "COCKPIT" };
    static const char *levels[] = { "EASY", "NORMAL", "HARD" };
    switch( item )
    {
    case SettingE::PROFILE:
    {
        static const char *profiles[] = { "CUSTOM", "BATTERY  40 FPS", "BALANCED  60 FPS", "QUALITY" };
        return profiles[std::clamp( m_settings.m_profile, 0, 3 )];
    }
    case SettingE::DISPLAY: return m_settings.m_display == DisplayModeE::FULLSCREEN ? "FULLSCREEN" : "WINDOW";
    case SettingE::RESOLUTION: return format( "%d%%", static_cast<int>( m_settings.m_renderScale * 100.0f + 0.5f ) );
    case SettingE::SHADOWS: return m_settings.m_shadows ? "ON" : "OFF";
    case SettingE::VSYNC: return m_settings.m_vsync ? "ON" : "OFF";
    case SettingE::FRAME_LIMIT: return m_settings.m_fpsLimit ? format( "%d FPS", m_settings.m_fpsLimit ) : std::string( "NONE" );
    case SettingE::DIFFICULTY: return levels[std::clamp( m_settings.m_difficulty, 0, 2 )];
    case SettingE::SCENERY: return m_settings.m_scenery ? "ALL MIXED" : "WORLD TOUR";
    case SettingE::TIME_OF_DAY:
    {
        static const char *times[] = { "AS THE PLACE", "TOUR CLOCK", "DAWN", "DAY", "DUSK", "NIGHT" };
        return times[std::clamp( m_settings.m_timeOfDay, 0, 5 )];
    }
    case SettingE::WEATHER: return m_settings.m_weather ? "AS THE PLACE" : "WEATHER FRONTS";
    case SettingE::SEASONS:
    {
        static const char *seasons[] = { "AS TODAY", "AS YOU TRAVEL", "OFF" };
        return seasons[std::clamp( m_settings.m_seasons, 0, 2 )];
    }
    case SettingE::POLICE: return m_settings.m_police ? "ON" : "OFF";
    case SettingE::STEER_ASSIST:
    {
        static const char *levels3[] = { "OFF", "LIGHT", "STRONG" };
        return levels3[std::clamp( m_settings.m_steerAssist, 0, 2 )];
    }
    case SettingE::BRAKE_ASSIST: return m_settings.m_brakeAssist ? "ON" : "OFF";
    case SettingE::COLOUR_BLIND: return m_settings.m_colourBlind ? "ON" : "OFF";
    case SettingE::TEXT_SIZE:
    {
        static const char *sizes[] = { "100%", "115%", "130%" };
        return sizes[std::clamp( m_settings.m_textSize, 0, 2 )];
    }
    case SettingE::GEARS: return m_settings.m_manualGears ? "MANUAL" : "AUTOMATIC";
    case SettingE::UNITS: return m_settings.m_units == UnitsE::MPH ? "MPH" : "KM/H";
    case SettingE::CAMERA: return cams[m_settings.m_camera];
    case SettingE::BLUR: return m_settings.m_speedBlur ? "ON" : "OFF";
    case SettingE::VIBRATION: return m_settings.m_vibration ? "ON" : "OFF";
    case SettingE::MUSIC: return bar( m_settings.m_musicVolume );
    case SettingE::EFFECTS: return bar( m_settings.m_sfxVolume );
    case SettingE::RADIO: return radioName();
    case SettingE::RESET: return m_armed == static_cast<int>( item ) ? "PRESS AGAIN TO RESET" : "ALL TO DEFAULTS";
    case SettingE::ERASE: return m_armed == static_cast<int>( item ) ? "PRESS AGAIN TO ERASE" : "RECORDS, PLACES, PHOTOS";
    default: return "";
    }
}

void Game::changeSetting( SettingE item, int dir )
{
    // Touching what a profile sets makes it a custom one.
    if( item == SettingE::RESOLUTION || item == SettingE::SHADOWS || item == SettingE::VSYNC || item == SettingE::FRAME_LIMIT ) m_settings.m_profile = 0;
    switch( item )
    {
    case SettingE::PROFILE:
    {
        // Battery: 40 frames at 70 % without shadows; balanced: 60 at 85 %; quality: as fast as the display, full
        // resolution, shadows.
        m_settings.m_profile = ( m_settings.m_profile + 4 + dir ) % 4;
        static const int   fps[4] = { 0, 40, 60, 0 };
        static const float scale[4] = { 1.0f, 0.7f, 0.85f, 1.0f };
        if( m_settings.m_profile > 0 )
        {
            m_settings.m_fpsLimit = fps[m_settings.m_profile];
            m_settings.m_renderScale = scale[m_settings.m_profile];
            m_settings.m_shadows = m_settings.m_profile == 3;
            m_settings.m_vsync = true;
            SetWindowState( FLAG_VSYNC_HINT );
            SetTargetFPS( m_settings.m_fpsLimit ? m_settings.m_fpsLimit : 0 );
        }
        break;
    }
    case SettingE::DISPLAY: toggleFullscreen(); break;
    case SettingE::RESOLUTION:
    {
        static const float scales[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
        int                idx = 2;
        for( int i = 0; i < 6; ++i )
            if( std::abs( scales[i] - m_settings.m_renderScale ) < 0.01f ) idx = i;
        m_settings.m_renderScale = scales[std::clamp( idx + dir, 0, 5 )];
        break;
    }
    case SettingE::SHADOWS: m_settings.m_shadows = !m_settings.m_shadows; break;
    case SettingE::VSYNC:
        m_settings.m_vsync = !m_settings.m_vsync;
        if( m_settings.m_vsync ) SetWindowState( FLAG_VSYNC_HINT );
        else ClearWindowState( FLAG_VSYNC_HINT );
        break;
    case SettingE::FRAME_LIMIT:
    {
        static const int limits[] = { 0, 30, 40, 60, 90, 120 };
        int              idx = 0;
        for( int i = 0; i < 6; ++i )
            if( limits[i] == m_settings.m_fpsLimit ) idx = i;
        m_settings.m_fpsLimit = limits[( idx + 6 + dir ) % 6];
        SetTargetFPS( m_settings.m_fpsLimit ? m_settings.m_fpsLimit : ( m_settings.m_vsync ? 0 : 240 ) );
        break;
    }
    case SettingE::DIFFICULTY:
        m_settings.m_difficulty = std::clamp( m_settings.m_difficulty + dir, 0, 2 );
        tuning() = applyDifficulty( m_tuningBase, m_settings.m_difficulty );
        break;
    case SettingE::SCENERY: m_settings.m_scenery = 1 - m_settings.m_scenery; break;
    case SettingE::TIME_OF_DAY: m_settings.m_timeOfDay = ( m_settings.m_timeOfDay + 6 + dir ) % 6; break;
    case SettingE::WEATHER: m_settings.m_weather = 1 - m_settings.m_weather; break;
    case SettingE::SEASONS:
        // The chunks carry the season in their colours: build them again.
        m_settings.m_seasons = ( m_settings.m_seasons + 3 + dir ) % 3;
        m_world.clear();
        m_world.m_seasons = seasonClock();
        break;
    case SettingE::POLICE: m_settings.m_police = !m_settings.m_police; break;
    case SettingE::STEER_ASSIST: m_settings.m_steerAssist = ( m_settings.m_steerAssist + 3 + dir ) % 3; break;
    case SettingE::BRAKE_ASSIST: m_settings.m_brakeAssist = !m_settings.m_brakeAssist; break;
    case SettingE::COLOUR_BLIND:
        m_settings.m_colourBlind = !m_settings.m_colourBlind;
        setColourBlind( m_settings.m_colourBlind );
        break;
    case SettingE::TEXT_SIZE:
        m_settings.m_textSize = ( m_settings.m_textSize + 3 + dir ) % 3;
        setTextScale( 1.0f + 0.15f * static_cast<float>( m_settings.m_textSize ) );
        break;
    case SettingE::GEARS: m_settings.m_manualGears = !m_settings.m_manualGears; break;
    case SettingE::UNITS: m_settings.m_units = m_settings.m_units == UnitsE::KMH ? UnitsE::MPH : UnitsE::KMH; break;
    case SettingE::CAMERA: m_settings.m_camera = ( m_settings.m_camera + 4 + dir ) % 4; break;
    case SettingE::BLUR: m_settings.m_speedBlur = !m_settings.m_speedBlur; break;
    case SettingE::VIBRATION: m_settings.m_vibration = !m_settings.m_vibration; break;
    case SettingE::MUSIC: m_settings.m_musicVolume = clampf( m_settings.m_musicVolume + 0.1f * dir, 0.0f, 1.0f ); break;
    case SettingE::EFFECTS: m_settings.m_sfxVolume = clampf( m_settings.m_sfxVolume + 0.1f * dir, 0.0f, 1.0f ); break;
    case SettingE::RADIO:
    {
        const int last = m_audio.hasFileMusic() ? StationCount : StationCount - 1;
        int       s = m_settings.m_station + dir;
        if( s > last ) s = -1;
        if( s < -1 ) s = last;
        m_settings.m_station = s;
        playMusic( hash64( m_trackSeed + 101 ), wantedStation() );
        break;
    }
    default: break;
    }
}

std::string Game::labValue( LabE item ) const
{
    const TrackParams &p = m_lab.m_params;
    auto level = []( int v ) { return v < 0 ? std::string( "AUTO" ) : format( "%d", v ); };
    switch( item )
    {
    case LabE::MODE: return m_lab.m_race ? "RACE  (11 RIVALS)" : "TIME ATTACK";
    case LabE::SCENERY:
        return p.m_biome == SceneryWorld ? std::string( "WORLD TOUR" )
               : p.m_biome < 0           ? std::string( "MIXED" )
                                         : std::string( biomeDef( static_cast<BiomeE>( p.m_biome ) ).m_name );
    case LabE::CURVES: return level( p.m_curvy );
    case LabE::SHARPNESS: return level( p.m_sharp );
    case LabE::HILLS: return level( p.m_hilly );
    case LabE::TRAFFIC: return level( p.m_traffic );
    case LabE::OBSTACLES: return level( p.m_obstacles );
    case LabE::LENGTH: return p.m_length <= 0.0 ? std::string( "ENDLESS" ) : format( "%.1f KM", p.m_length / 1000.0 );
    case LabE::SEED: return format( "%llu", static_cast<unsigned long long>( m_lab.m_seed ) );
    case LabE::CODE: return encodeTrack( m_lab );
    default: return "";
    }
}

void Game::changeLab( LabE item, int dir )
{
    TrackParams &p = m_lab.m_params;
    auto         level = [&]( int &v ) { v = v + dir > 9 ? -1 : v + dir < -1 ? 9 : v + dir; };
    switch( item )
    {
    case LabE::MODE:
        m_lab.m_race = !m_lab.m_race;
        if( m_lab.m_race && p.m_length <= 0.0 ) p.m_length = 10000.0;
        break;
    case LabE::SCENERY:
    {
        // The world tour, mixed, the eight classic biomes, and every place discovered on the road (in the route's order).
        std::vector<int> options = { SceneryWorld, SceneryMixed };
        for( BiomeE b : worldRoute() )
            if( static_cast<int>( b ) < 8 || m_records.m_seen.test( static_cast<size_t>( b ) ) ) options.push_back( static_cast<int>( b ) );
        for( int b = 0; b < biomeCount(); ++b )
            if( routeIndex( static_cast<BiomeE>( b ) ) < 0 && ( b < 8 || m_records.m_seen.test( static_cast<size_t>( b ) ) ) ) options.push_back( b );
        int idx = 0;
        for( size_t i = 0; i < options.size(); ++i )
            if( options[i] == p.m_biome ) idx = static_cast<int>( i );
        p.m_biome = options[static_cast<size_t>( ( idx + dir + static_cast<int>( options.size() ) ) % static_cast<int>( options.size() ) )];
        break;
    }
    case LabE::CURVES: level( p.m_curvy ); break;
    case LabE::SHARPNESS: level( p.m_sharp ); break;
    case LabE::HILLS: level( p.m_hilly ); break;
    case LabE::TRAFFIC: level( p.m_traffic ); break;
    case LabE::OBSTACLES: level( p.m_obstacles ); break;
    case LabE::LENGTH:
    {
        int n = static_cast<int>( std::lround( p.m_length / 2500.0 ) ) + dir;
        if( n > 15 ) n = m_lab.m_race ? 1 : 0;
        if( n < ( m_lab.m_race ? 1 : 0 ) ) n = 15;
        p.m_length = static_cast<double>( n ) * 2500.0;
        break;
    }
    case LabE::SEED: m_lab.m_seed = randomSeed(); break;
    default: break;
    }
}

void Game::menus( const MenuInput &m )
{
    auto move = [&]( int count )
    {
        if( m.m_up )
        {
            m_menuIndex = ( m_menuIndex + count - 1 ) % count;
            m_audio.play( SfxE::TICK );
        }
        if( m.m_down )
        {
            m_menuIndex = ( m_menuIndex + 1 ) % count;
            m_audio.play( SfxE::TICK );
        }
    };
    const int change = ( m.m_right ? 1 : 0 ) - ( m.m_left ? 1 : 0 );
    auto      carReady = [&]()
    {
        std::string why;
        if( !m_records.carUnlocked( m_carIndex ) ) why = "CAR  -  " + Records::carRequirement( m_carIndex );
        else if( !m_records.paintUnlocked( m_paintIndex ) ) why = "PAINT  -  " + Records::paintRequirement( m_paintIndex );
        else if( !m_records.rimsUnlocked( m_settings.m_rims ) ) why = "RIMS  -  " + Records::rimsRequirement( m_settings.m_rims );
        else if( !m_records.decalUnlocked( m_settings.m_decal ) ) why = "DECAL  -  " + Records::decalRequirement( m_settings.m_decal );
        if( why.empty() ) return true;
        popup( "LOCKED", why, Red, 0.8f, 2.4f );
        m_audio.play( SfxE::WARNING );
        return false;
    };

    switch( m_state )
    {
    case StateE::TITLE:
    {
        move( static_cast<int>( TitleE::COUNT ) );
        const auto item = static_cast<TitleE>( m_menuIndex );
        const bool canResume = !m_savedPlace.empty() && biomeId( m_savedPlace ) >= 0;
        const bool canContinue = !m_records.m_tourAt.empty() && biomeId( m_records.m_tourAt ) >= 0 && endlessParams().m_biome == SceneryWorld;
        // The ways to start an endless run that are there; left and right go round them.
        std::vector<int> ways;
        if( canResume ) ways.push_back( 0 );
        if( canContinue ) ways.push_back( 1 );
        ways.push_back( 2 );
        if( std::find( ways.begin(), ways.end(), m_endlessStart ) == ways.end() ) m_endlessStart = ways.front();
        if( item == TitleE::ENDLESS && change != 0 && ways.size() > 1 )
        {
            const auto at = std::find( ways.begin(), ways.end(), m_endlessStart ) - ways.begin();
            m_endlessStart = ways[static_cast<size_t>( ( at + static_cast<long>( ways.size() ) + change ) % static_cast<long>( ways.size() ) )];
            m_audio.play( SfxE::TICK );
        }
        if( !m.m_accept ) break;
        m_audio.play( SfxE::SELECT );
        switch( item )
        {
        case TitleE::DAILY:
            if( carReady() ) startDaily();
            break;
        case TitleE::TRAVEL:
            if( carReady() ) startTravel();
            break;
        case TitleE::RALLIES:
            m_state = StateE::RALLIES;
            m_menuIndex = 0;
            break;
        case TitleE::ENDLESS:
            if( carReady() && !( m_endlessStart == 0 && resumeRun() ) )
            {
                TrackSetup s;
                s.m_seed = m_options.m_seed ? ( ( m_options.m_seed & 0x1FFFFFFFull ) | 1ull ) : randomSeed();
                s.m_params = endlessParams();
                if( canContinue && m_endlessStart == 1 ) s.m_startBiome = biomeId( m_records.m_tourAt );
                startTrack( s, ModeE::ENDLESS );
            }
            break;
        case TitleE::CHAMPIONSHIP:
            if( carReady() ) startChampionship();
            break;
        case TitleE::TRACK_LAB:
            m_state = StateE::LAB;
            m_menuIndex = 0;
            break;
        case TitleE::GARAGE:
            m_state = StateE::GARAGE;
            m_menuIndex = 0;
            break;
        case TitleE::PASSPORT:
            m_state = StateE::PASSPORT;
            m_passportSheet = 0;
            break;
        case TitleE::RECORDS: m_state = StateE::RECORDS; break;
        case TitleE::SETTINGS:
            m_settingsReturn = StateE::TITLE;
            m_returnIndex = static_cast<int>( TitleE::SETTINGS );
            m_state = StateE::SETTINGS;
            m_menuIndex = 0;
            break;
        case TitleE::QUIT: m_quit = true; break;
        default: break;
        }
        break;
    }
    case StateE::SETTINGS:
    {
        const int count = static_cast<int>( SettingE::COUNT );
        move( count );
        const auto item = static_cast<SettingE>( m_menuIndex );
        // The two resets want a second press (within four seconds, on the same item).
        if( m_armed >= 0 && ( m_armed != static_cast<int>( item ) || m_time - m_armedAt > 4.0f ) ) m_armed = -1;
        if( ( item == SettingE::RESET || item == SettingE::ERASE ) && m.m_accept )
        {
            if( m_armed == static_cast<int>( item ) )
            {
                m_armed = -1;
                if( item == SettingE::RESET ) resetSettings();
                else eraseProgress();
                popup( item == SettingE::RESET ? "SETTINGS RESET" : "PROGRESS ERASED", "", item == SettingE::RESET ? Accent : Red, 0.8f, 2.5f );
                m_audio.play( SfxE::SELECT );
            }
            else
            {
                m_armed = static_cast<int>( item );
                m_armedAt = m_time;
                m_audio.play( SfxE::WARNING );
            }
        }
        else if( item != SettingE::BACK && item != SettingE::RESET && item != SettingE::ERASE && ( change != 0 || m.m_accept ) )
        {
            changeSetting( item, change != 0 ? change : 1 );
            settingsChanged();
            m_audio.play( SfxE::TICK );
        }
        if( m.m_back || ( m.m_accept && item == SettingE::BACK ) )
        {
            m_audio.play( SfxE::SELECT );
            m_state = m_settingsReturn;
            m_menuIndex = m_returnIndex;
        }
        break;
    }
    case StateE::LAB:
    {
        move( static_cast<int>( LabE::COUNT ) );
        const auto item = static_cast<LabE>( m_menuIndex );
        if( change != 0 && item < LabE::CODE )
        {
            changeLab( item, change );
            m_audio.play( SfxE::TICK );
        }
        if( m.m_back )
        {
            m_state = StateE::TITLE;
            m_menuIndex = static_cast<int>( TitleE::TRACK_LAB );
            break;
        }
        if( !m.m_accept ) break;
        m_audio.play( SfxE::SELECT );
        switch( item )
        {
        case LabE::CODE:
        {
            m_codeText = encodeTrack( m_lab );
            std::erase( m_codeText, '-' );
            m_codeCursor = 0;
            m_codeError.clear();
            m_state = StateE::CODE_ENTRY;
            break;
        }
        case LabE::START:
            if( carReady() ) startTrack( m_lab, ModeE::CUSTOM );
            break;
        case LabE::BACK:
            m_state = StateE::TITLE;
            m_menuIndex = static_cast<int>( TitleE::TRACK_LAB );
            break;
        default: changeLab( item, 1 ); break;
        }
        break;
    }
    case StateE::CODE_ENTRY:
    {
        static const char *alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
        bool               typed = false;
        for( int c = GetCharPressed(); c > 0; c = GetCharPressed() )
        {
            char ch = static_cast<char>( std::toupper( c ) );
            if( ch == 'O' ) ch = '0';
            if( ch == 'I' || ch == 'L' ) ch = '1';
            if( ch && std::strchr( alphabet, ch ) )
            {
                m_codeText[static_cast<size_t>( m_codeCursor )] = ch;
                m_codeCursor = std::min( 11, m_codeCursor + 1 );
                typed = true;
            }
        }
        const bool backspace = IsKeyPressed( KEY_BACKSPACE );
        if( backspace )
        {
            m_codeCursor = std::max( 0, m_codeCursor - 1 );
            typed = true;
        }
        if( !typed )
        {
            if( m.m_left ) m_codeCursor = std::max( 0, m_codeCursor - 1 );
            if( m.m_right ) m_codeCursor = std::min( 11, m_codeCursor + 1 );
            if( m.m_up || m.m_down )
            {
                const char *pos = std::strchr( alphabet, m_codeText[static_cast<size_t>( m_codeCursor )] );
                int         idx = pos ? static_cast<int>( pos - alphabet ) : 0;
                idx = ( idx + ( m.m_up ? 1 : 31 ) ) % 32;
                m_codeText[static_cast<size_t>( m_codeCursor )] = alphabet[idx];
            }
        }
        if( m.m_accept && !typed )
        {
            TrackSetup setup;
            if( decodeTrack( m_codeText, setup ) )
            {
                m_lab = setup;
                m_state = StateE::LAB;
                m_menuIndex = static_cast<int>( LabE::START );
                m_audio.play( SfxE::SELECT );
            }
            else
            {
                m_codeError = "THAT CODE IS NOT A TRACK";
                m_audio.play( SfxE::WARNING );
            }
        }
        if( m.m_back && !backspace )
        {
            m_state = StateE::LAB;
            m_menuIndex = static_cast<int>( LabE::CODE );
        }
        break;
    }
    case StateE::INITIALS:
    {
        bool typed = false;
        for( int c = GetCharPressed(); c > 0; c = GetCharPressed() )
        {
            const char ch = static_cast<char>( std::toupper( c ) );
            if( ch >= 'A' && ch <= 'Z' )
            {
                m_initials[static_cast<size_t>( m_initialCursor )] = ch;
                m_initialCursor = std::min( 2, m_initialCursor + 1 );
                typed = true;
            }
        }
        if( IsKeyPressed( KEY_BACKSPACE ) )
        {
            m_initialCursor = std::max( 0, m_initialCursor - 1 );
            typed = true;
        }
        if( !typed )
        {
            if( m.m_left ) m_initialCursor = std::max( 0, m_initialCursor - 1 );
            if( m.m_right ) m_initialCursor = std::min( 2, m_initialCursor + 1 );
            if( m.m_up || m.m_down )
            {
                char &ch = m_initials[static_cast<size_t>( m_initialCursor )];
                ch = static_cast<char>( 'A' + ( ch - 'A' + ( m.m_up ? 1 : 25 ) ) % 26 );
            }
        }
        if( m.m_accept && !typed && m_stateTime > 0.5f )
        {
            if( m_initialCursor < 2 && !IsKeyPressed( KEY_ENTER ) && !IsKeyPressed( KEY_KP_ENTER ) )
            {
                ++m_initialCursor;
            }
            else
            {
                m_records.insert( { m_initials, static_cast<int64_t>( m_score ), m_car.m_s - m_startS, m_code } );
                saveRecords();
                m_audio.play( SfxE::SELECT );
                m_state = StateE::GAMEOVER;
                m_stateTime = 0.0f;
                m_menuIndex = 0;
            }
        }
        break;
    }
    case StateE::RECORDS:
        if( m.m_back || m.m_accept )
        {
            m_state = StateE::TITLE;
            m_menuIndex = static_cast<int>( TitleE::RECORDS );
        }
        break;
    case StateE::RALLIES:
    {
        const int count = static_cast<int>( rallies().size() + journeys().size() ) + 1;
        move( count );
        if( m.m_back || ( m.m_accept && m_menuIndex == count - 1 ) )
        {
            m_audio.play( SfxE::SELECT );
            m_state = StateE::TITLE;
            m_menuIndex = static_cast<int>( TitleE::RALLIES );
        }
        else if( m.m_accept && carReady() )
        {
            m_audio.play( SfxE::SELECT );
            const int rallyCount = static_cast<int>( rallies().size() );
            m_menuIndex < rallyCount ? startRally( m_menuIndex ) : startJourney( m_menuIndex - rallyCount );
        }
        break;
    }
    case StateE::GARAGE:
    {
        move( static_cast<int>( GarageE::COUNT ) );
        const auto item = static_cast<GarageE>( m_menuIndex );
        if( change != 0 ) changeGarage( item, change );
        if( m.m_back || ( m.m_accept && item == GarageE::BACK ) )
        {
            m_audio.play( SfxE::SELECT );
            m_state = StateE::TITLE;
            m_menuIndex = static_cast<int>( TitleE::GARAGE );
        }
        break;
    }
    case StateE::PASSPORT:
    {
        const bool regions = m_passportPage == static_cast<int>( PassportE::REGIONS );
        if( regions && m_passportCountry >= 0 )
        {
            // A country opened: up and down turn its sheets, anything else goes back to the list.
            if( m.m_down || m.m_up )
            {
                m_passportSheet = std::max( 0, m_passportSheet + ( m.m_down ? 1 : -1 ) );
                m_audio.play( SfxE::TICK );
            }
            if( m.m_back || m.m_accept || change != 0 )
            {
                m_passportCountry = -1;
                m_passportSheet = 0;
                m_audio.play( SfxE::TICK );
            }
            break;
        }
        if( change != 0 )
        {
            m_passportPage = ( m_passportPage + static_cast<int>( PassportE::COUNT ) + change ) % static_cast<int>( PassportE::COUNT );
            m_passportSheet = 0;
            m_audio.play( SfxE::TICK );
        }
        if( regions && ( m.m_down || m.m_up ) )
        {
            const int count = static_cast<int>( regionRows().size() );
            m_passportPick = std::clamp( m_passportPick + ( m.m_down ? 1 : -1 ), 0, std::max( 0, count - 1 ) );
            m_audio.play( SfxE::TICK );
        }
        else if( m.m_down || m.m_up )
        {
            m_passportSheet = std::max( 0, m_passportSheet + ( m.m_down ? 1 : -1 ) );
            m_audio.play( SfxE::TICK );
        }
        if( regions && m.m_accept && !regionRows().empty() )
        {
            m_passportCountry = m_passportPick;
            m_passportSheet = 0;
            m_audio.play( SfxE::SELECT );
        }
        else if( m.m_back || m.m_accept )
        {
            unloadPostcards();
            m_state = StateE::TITLE;
            m_menuIndex = static_cast<int>( TitleE::PASSPORT );
        }
        break;
    }
    case StateE::PAUSED:
    {
        const std::vector<std::string> items = pauseItems();
        move( static_cast<int>( items.size() ) );
        if( m.m_back || m_input.m_pause )
        {
            m_state = m_pauseReturn;
            break;
        }
        if( m.m_accept )
        {
            m_audio.play( SfxE::SELECT );
            const std::string &item = items[static_cast<size_t>( std::clamp( m_menuIndex, 0, static_cast<int>( items.size() ) - 1 ) )];
            if( item == "RESUME" ) m_state = m_pauseReturn;
            else if( item == "RESTART" ) restart();
            else if( item == "PHOTO MODE" )
            {
                m_state = StateE::PHOTO;
                m_stateTime = 0.0f;
            }
            else if( item == "NEXT SONG" ) playMusic( hash64( m_trackSeed + 31 ), m_playingStation );
            else if( item == "SETTINGS" )
            {
                m_settingsReturn = StateE::PAUSED;
                m_returnIndex = 4;
                m_state = StateE::SETTINGS;
                m_menuIndex = 0;
            }
            else if( item == "SAVE AND QUIT" ) saveRun();
            else startDemo();
        }
        break;
    }
    case StateE::GAMEOVER:
    {
        const std::vector<std::string> items = gameOverItems();
        move( static_cast<int>( items.size() ) );
        if( m.m_accept && m_stateTime > 1.0f )
        {
            m_audio.play( SfxE::SELECT );
            const std::string &item = items[static_cast<size_t>( std::clamp( m_menuIndex, 0, static_cast<int>( items.size() ) - 1 ) )];
            if( item == "RETRY" || item == "RETRY SAME TRACK" )
                m_mode == ModeE::RALLY ? ( m_journey >= 0 ? startJourney( m_journey ) : startRally( m_rally ) ) : m_mode == ModeE::DAILY ? startDaily() : restart();
            else if( item == "NEW TRACK" )
            {
                TrackSetup s;
                s.m_seed = randomSeed();
                s.m_params = endlessParams();
                startTrack( s, ModeE::ENDLESS );
            }
            else if( item == "TRACK LAB" || item == "RALLIES" )
            {
                startDemo();
                m_state = item == "RALLIES" ? StateE::RALLIES : StateE::LAB;
                m_menuIndex = item == "RALLIES" ? ( m_journey >= 0 ? static_cast<int>( rallies().size() ) + m_journey : std::max( 0, m_rally ) ) : 0;
            }
            else if( item == "WATCH REPLAY" ) startReplay( StateE::GAMEOVER );
            else startDemo();
        }
        break;
    }
    case StateE::RESULTS:
    {
        if( m_stateTime < 1.0f ) break;
        const bool champ = m_mode == ModeE::CHAMPIONSHIP;
        const bool qualified = m_place <= QualifyPlace;
        const bool last = m_race.m_round + 1 >= RoundCount;
        if( m_resultPage == 0 && !champ )
        {
            move( 3 );
            if( m.m_accept )
            {
                m_audio.play( SfxE::SELECT );
                if( m_menuIndex == 0 ) restart();
                else
                {
                    startDemo();
                    if( m_menuIndex == 1 ) m_state = StateE::LAB;
                }
            }
        }
        else if( m_resultPage == 0 )
        {
            if( m.m_accept )
            {
                m_resultPage = 1;
                m_menuIndex = 0;
                m_stateTime = 0.0f;
                m_audio.play( SfxE::SELECT );
            }
        }
        else if( m_resultPage == 1 )
        {
            move( qualified && last ? 1 : 2 );
            if( !m.m_accept ) break;
            m_audio.play( SfxE::SELECT );
            if( m_menuIndex == 1 )
            {
                startDemo();
            }
            else if( !qualified )
            {
                // The failed race does not count: its points go, and it is driven again.
                for( const ResultLine &l : m_results )
                {
                    if( l.m_driver < 0 ) m_race.m_playerPoints -= l.m_points;
                    else m_race.m_drivers[l.m_driver].m_points -= l.m_points;
                }
                startRound( m_race.m_round );
            }
            else if( !last )
            {
                startRound( m_race.m_round + 1 );
            }
            else
            {
                const auto table = m_race.standings();
                int        place = 1;
                for( size_t i = 0; i < table.size(); ++i )
                    if( table[i].first < 0 ) place = static_cast<int>( i ) + 1;
                const bool goldBefore = m_records.paintUnlocked( 4 );
                if( place == 1 ) ++m_records.m_titles;
                m_records.m_bestPlace = m_records.m_bestPlace == 0 ? place : std::min( m_records.m_bestPlace, place );
                saveRecords();
                if( !goldBefore && m_records.paintUnlocked( 4 ) ) popup( "NEW PAINT", paintName( 4 ), Gold, 0.9f, 4.0f );
                m_place = place;
                m_resultPage = 2;
                m_stateTime = 0.0f;
            }
        }
        else if( m.m_accept )
        {
            startDemo();
        }
        break;
    }
    default: break;
    }
}

void Game::draw()
{
    const bool lastFrame = m_options.m_quitAfter > 0.0f && m_time >= m_options.m_quitAfter;
    BeginDrawing();
    ClearBackground( BLACK );

    SceneInput in;
    in.m_track = m_track.get();
    in.m_world = &m_world;
    in.m_models = &m_models;
    in.m_player = &m_car;
    in.m_car = m_carIndex;
    in.m_paint = m_paintIndex;
    in.m_traffic = &m_traffic;
    in.m_particles = &m_particles;
    in.m_env = sceneEnvironment( m_car.m_s + 30.0 );
    m_lastEnv = in.m_env;
    in.m_surface = surfaceAt( *m_track, m_car.m_s + 30.0 );
    in.m_time = m_time;
    in.m_dt = m_state == StateE::PAUSED ? 0.0f : m_dt;
    in.m_flash = m_flash;
    in.m_speedBlur = m_blur;
    in.m_aberration = m_aberration;
    in.m_windStreaks = smoothstepf( 0.62f, 0.95f, m_car.m_speed / m_car.m_spec.m_topSpeed ) + ( m_car.m_boosting ? 0.5f : 0.0f );
    in.m_drawPlayer = !( m_settings.m_camera >= 2 && !m_demo && m_state != StateE::PHOTO && m_state != StateE::REPLAY );
    if( m_lookingBack || m_state == StateE::REPLAY || m_state == StateE::PHOTO ) in.m_behind = 1.0e6f;
    in.m_shadows = m_settings.m_shadows;
    in.m_lensRain = m_settings.m_camera >= 2 && !m_demo ? in.m_env.m_rain : in.m_env.m_rain * 0.5f;
    in.m_slush = smoothstepf( 0.05f, 0.4f, in.m_env.m_snow );
    in.m_puddles.clear();
    if( in.m_env.m_rain > 0.35f && m_track )
    {
        // The puddles the rain leaves, where the car meets them too.
        const long first = static_cast<long>( ( m_car.m_s - 60.0 ) / SegmentLength ), last = static_cast<long>( ( m_car.m_s + 450.0 ) / SegmentLength );
        float      x = 0.0f;
        for( long j = std::max( 2L, first ); j <= last; ++j )
        {
            if( !rainPuddleAt( *m_track, j, x ) ) continue;
            const Pose p = roadPose( *m_track, ( static_cast<double>( j ) + 0.5 ) * SegmentLength, x, 0.0f );
            in.m_puddles.push_back( { nullptr, p.m_pos, std::atan2( p.m_forward.x, p.m_forward.z ) } );
        }
    }
    for( const Rival &r : m_race.m_rivals )
    {
        const Driver &d = m_race.m_drivers[r.m_driver];
        const float   wobble = r.m_hitTimer > 0.0f ? std::sin( r.m_hitTimer * 25.0f ) * 0.08f * r.m_hitTimer : 0.0f;
        in.m_cars.push_back( { r.m_s, r.m_x, r.m_yaw + wobble, d.m_car, d.m_paint, 1.0f } );
    }
    if( m_police.m_state != PoliceStateE::NONE && m_police.m_model >= 0 )
        in.m_police.push_back( { m_police.m_s, m_police.m_x, m_police.m_yaw, m_police.m_model,
                                 m_police.m_state == PoliceStateE::CHASE ? static_cast<int>( m_time * 6.0f ) & 1 : -1 } );
    if( !m_demo && ( m_state == StateE::RACE || m_state == StateE::COUNTDOWN || m_state == StateE::PAUSED ) )
    {
        double gs = 0.0;
        float  gx = 0.0f, gyaw = 0.0f;
        if( m_ghost.at( m_raceTime, gs, gx, gyaw ) ) in.m_cars.push_back( { gs, gx, gyaw, m_ghost.m_car, m_ghost.m_paint, 0.35f } );
    }
    CameraView view = m_cam;
    if( m_state == StateE::VIEWER ) viewerScene( in, view );
    if( m_state == StateE::CROSSING ) crossingScene( in, view );
    m_renderer.render( in, view, m_settings.m_renderScale );
    if( m_state == StateE::PHOTO && m_photoWanted ) takePhoto();
    if( m_albumWanted >= 0 && !m_demo && m_state == StateE::RACE ) takeAlbumPhoto();
    if( m_postcardWanted >= 0 && !m_demo && m_state == StateE::RACE && m_car.m_s >= m_postcardAt )
    {
        const long seg = static_cast<long>( m_car.m_s / SegmentLength );
        if( m_postcardWanted != static_cast<int>( m_track->biomeMix( m_car.m_s ).m_from ) ) m_postcardWanted = -1;
        else if( ( m_track->segment( seg ).m_flags | m_track->segment( seg + 8 ).m_flags ) & SEG_TUNNEL ) m_postcardAt = m_car.m_s + 40.0;
        else takePostcard( static_cast<BiomeE>( m_postcardWanted ) );
    }

    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    switch( m_state )
    {
    case StateE::TITLE: drawTitle(); break;
    case StateE::SETTINGS:
    {
        if( !m_demo ) drawHud();
        DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 110 } );
        std::vector<std::string> labels, values;
        for( int i = 0; i < static_cast<int>( SettingE::COUNT ); ++i )
        {
            labels.push_back( settingLabel( static_cast<SettingE>( i ) ) );
            values.push_back( settingValue( static_cast<SettingE>( i ) ) );
        }
        drawMenu( "SETTINGS", labels, values, H * 0.12f );
        break;
    }
    case StateE::COUNTDOWN:
    {
        drawHud();
        const int n = 3 - static_cast<int>( m_stateTime );
        if( n >= 1 && n <= 3 )
        {
            const float k = 1.0f - std::fmod( m_stateTime, 1.0f );
            drawTextFx( std::to_string( n ), W * 0.5f, H * 0.5f - 30.0f * u * k, ( 120.0f + 60.0f * k ) * u, Accent, AlignE::CENTER, 1.0f );
        }
        break;
    }
    case StateE::RACE: drawHud(); break;
    case StateE::PAUSED:
        drawHud();
        DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 120 } );
        drawMenu( "PAUSE", pauseItems(), {}, H * 0.24f );
        break;
    case StateE::GAMEOVER: drawResults(); break;
    case StateE::RESULTS: drawRaceResults(); break;
    case StateE::LAB: drawLab(); break;
    case StateE::CODE_ENTRY: drawCodeEntry(); break;
    case StateE::INITIALS: drawInitials(); break;
    case StateE::RECORDS: drawRecords(); break;
    case StateE::GARAGE: drawGarage(); break;
    case StateE::RALLIES: drawRallies(); break;
    case StateE::REPLAY:
    {
        const float pulse = 0.6f + 0.4f * std::sin( m_time * 3.0f );
        drawTextFx( "REPLAY", 28.0f * u, 22.0f * u, 26.0f * u, Color{ 255, 60, 60, static_cast<unsigned char>( 255 * pulse ) }, AlignE::LEFT, 0.4f );
        drawText( placeName( m_track->biomeMix( m_car.m_s ).m_from ), 28.0f * u, 58.0f * u, 12.0f * u, Accent );
        const bool mph = m_settings.m_units == UnitsE::MPH;
        drawText( format( "%d %s", static_cast<int>( m_car.m_speed * ( mph ? 2.23694f : 3.6f ) ), mph ? "MPH" : "KM/H" ), W - 28.0f * u, 22.0f * u, 20.0f * u, Ink,
                  AlignE::RIGHT );
        drawHint( m_input.m_gamepad ? "[A] BACK" : "[ENTER] BACK", W * 0.5f, H - 30.0f * u, 11.0f * u, Dim, AlignE::CENTER );
        break;
    }
    case StateE::PHOTO:
        drawHint( m_input.m_gamepad ? "[STICK] TURN     [LT] [RT] ZOOM     [A] TAKE PHOTO     [B] BACK"
                                    : "[ARROWS] TURN     [+] [-] ZOOM     [ENTER] TAKE PHOTO     [ESC] BACK",
                  W * 0.5f, H - 30.0f * u, 11.0f * u, Color{ 255, 255, 255, 190 }, AlignE::CENTER );
        drawPopups();
        break;
    case StateE::PASSPORT: drawPassport(); break;
    case StateE::VIEWER:
        drawViewer();
        drawPopups();
        break;
    case StateE::CROSSING: drawCrossing(); break;
    }
    const bool menuScreen = m_state == StateE::TITLE || m_state == StateE::LAB || m_state == StateE::CODE_ENTRY || m_state == StateE::RECORDS ||
                            m_state == StateE::GARAGE || m_state == StateE::PASSPORT || m_state == StateE::RALLIES;
    if( menuScreen ) drawPopups();

    if( m_nowTimer < 4.5f && !m_nowPlaying.empty() && m_state != StateE::PASSPORT && m_state != StateE::RALLIES && m_state != StateE::VIEWER )
    {
        const float a = std::min( 1.0f, m_nowTimer * 4.0f ) * std::min( 1.0f, ( 4.5f - m_nowTimer ) * 1.5f );
        // Menus keep the bottom line for their hints: there the radio shows in the top left corner.
        const bool  menu = menuScreen;
        const float x = menu ? 24.0f * u : W * 0.5f;
        const float y = menu ? 52.0f * u : H - 40.0f * u;
        const AlignE align = menu ? AlignE::LEFT : AlignE::CENTER;
        Color       lab = Accent;
        lab.a = static_cast<unsigned char>( 255.0f * a );
        Color txt = Ink;
        txt.a = lab.a;
        drawTextFx( "RADIO  " + radioName(), x, y - 22.0f * u, 11.0f * u, lab, align, 0.0f );
        drawTextFx( m_nowPlaying, x, y, 15.0f * u, txt, align, 0.2f * a );
    }

    if( m_showStats )
    {
        size_t triangles = 0;
        for( const Chunk &c : m_world.chunks() ) triangles += static_cast<size_t>( c.m_ground.triangleCount + c.m_props.triangleCount );
        const std::string stats = format( "%d FPS   FRAME %.1f MS (PEAK %.1f)   CHUNK BUILD %.1f MS (PEAK %.1f, SLOWEST %.1f)   %d CHUNKS  %.0fK TRIS",
                                          GetFPS(), m_dt * 1000.0f, m_frameMsPeak, m_world.m_buildMs, m_world.m_buildPeakMs,
                                          m_world.m_buildMaxMs, static_cast<int>( m_world.chunks().size() ), triangles / 1000.0 );
        drawTextFx( stats, 12.0f * u, 10.0f * u, 10.0f * u, Green, AlignE::LEFT, 0.0f );
    }

    auto saveScreen = []( const std::string &file )
    {
        // LoadImageFromScreen() scales by the desktop's DPI even when the framebuffer is not scaled.
        rlDrawRenderBatchActive();
        const int w = GetRenderWidth(), h = GetRenderHeight();
        Image     img = { rlReadScreenPixels( w, h ), w, h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        ExportImage( img, file.c_str() );
        UnloadImage( img );
    };
    if( m_options.m_shapeTour > 0 && m_state == StateE::VIEWER && m_viewerBuilt == m_viewerIndex && ++m_tourFrames >= 4 )
    {
        // Testing: one picture per model, then the next, the same view of each.
        saveScreen( m_options.m_screenshot + landmarkName( m_viewerIndex ) + ".png" );
        m_tourFrames = 0;
        m_viewerIndex = ( m_viewerIndex + 1 ) % std::max( 1, landmarkCount() );
        m_viewerYaw = 0.9f;
        if( ++m_tourShots >= m_options.m_shapeTour ) m_quit = true;
    }
    if( lastFrame )
    {
        if( !m_options.m_screenshot.empty() ) saveScreen( m_options.m_screenshot );
        m_quit = true;
    }
    EndDrawing();
}

void Game::drawTitle()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangleGradientV( 0, 0, static_cast<int>( W ), static_cast<int>( H * 0.45f ), Color{ 0, 0, 0, 150 }, Color{ 0, 0, 0, 0 } );

    // The emblem: a striped sun behind the name.
    const Vector2 c = { W * 0.5f, 130.0f * u };
    const float   r = 70.0f * u;
    for( int i = 0; i < 24; ++i )
    {
        const float y0 = -r + 2.0f * r * static_cast<float>( i ) / 24.0f;
        const float y1 = y0 + 2.0f * r / 24.0f;
        const float t = static_cast<float>( i ) / 23.0f;
        if( t > 0.5f && ( i % 3 ) == 0 ) continue;
        const float yc = ( y0 + y1 ) * 0.5f;
        const float hw = std::sqrt( std::max( 0.0f, r * r - yc * yc ) );
        const Color col = lerpColor( Color{ 255, 214, 80, 230 }, Color{ 255, 40, 140, 230 }, t );
        DrawRectangle( static_cast<int>( c.x - hw ), static_cast<int>( c.y + y0 ), static_cast<int>( hw * 2.0f ),
                       static_cast<int>( y1 - y0 + 1.0f ), col );
    }
    drawTextFx( "SUNSTREAK", W * 0.5f, 100.0f * u, 72.0f * u, Ink, AlignE::CENTER, 1.0f );
    drawText( "ENDLESS ARCADE RACING", W * 0.5f, 190.0f * u, 15.0f * u, Accent, AlignE::CENTER );
    if( m_settings.m_bestScore > 0 )
        drawText( format( "BEST  %lld   %.1f KM", static_cast<long long>( m_settings.m_bestScore ), m_settings.m_bestDistance / 1000.0 ),
                  W * 0.5f, 216.0f * u, 12.0f * u, Dim, AlignE::CENTER );

    int done = 0;
    for( double best : m_records.m_rallyBest ) done += best > 0.0 ? 1 : 0;
    const std::string daily = m_records.m_dailyDate == today() && m_records.m_dailyBest > 0 ? format( "TODAY %lld", static_cast<long long>( m_records.m_dailyBest ) )
                                                                                              : std::string( "NEW TODAY" );
    // The endless run resumes the saved run, continues the world tour where it stopped, or starts a new one (left and
    // right).
    std::string tour;
    const int   tourAt = m_records.m_tourAt.empty() ? -1 : biomeId( m_records.m_tourAt );
    const int   savedAt = m_savedPlace.empty() ? -1 : biomeId( m_savedPlace );
    const bool  continues = tourAt >= 0 && endlessParams().m_biome == SceneryWorld;
    auto        shorten = []( const std::string &lead, const std::string &name )
    { return lead.size() + name.size() <= 20 ? lead + name : name.substr( 0, 20 ); };
    if( m_endlessStart == 0 && savedAt >= 0 ) tour = shorten( "RESUME ", biomeDef( static_cast<BiomeE>( savedAt ) ).m_name );
    else if( m_endlessStart == 1 && continues ) tour = shorten( "FROM ", biomeDef( static_cast<BiomeE>( tourAt ) ).m_name );
    else if( savedAt >= 0 || continues ) tour = "NEW TOUR";
    const std::string travel = tourAt >= 0 ? shorten( "FROM ", biomeDef( static_cast<BiomeE>( tourAt ) ).m_name ) : std::string( "NO CLOCK" );
    drawMenu( "", { "ENDLESS RUN", "TRAVEL", "DAILY CHALLENGE", "RALLIES", "CHAMPIONSHIP", "TRACK LAB", "GARAGE", "PASSPORT", "RECORDS", "SETTINGS", "QUIT" },
              { tour, travel, daily, format( "%d / %d DONE", done, static_cast<int>( rallies().size() ) ), "", "", garageValue( GarageE::CAR ),
                format( "%d / %d COUNTRIES", m_records.countriesSeen(), countryCount() ), "", "", "" },
              H * 0.3f, 0.84f );
}

void Game::drawResults()
{
    drawHud();
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 130 } );
    const float w = 560.0f * u, h = 300.0f * u;
    const float x = W * 0.5f - w * 0.5f, y = H * 0.08f;
    panel( x, y, w, h );
    drawTextFx( m_finishedTrack ? "FINISHED" : "TIME UP", W * 0.5f, y + 22.0f * u, 40.0f * u, m_finishedTrack ? Gold : Red,
                AlignE::CENTER, 1.0f );
    const bool        mph = m_settings.m_units == UnitsE::MPH;
    const bool        rally = m_mode == ModeE::RALLY;
    const std::string rows[6][2] = {
        { "DISTANCE", format( "%.2f KM", m_finalDistance / 1000.0 ) },
        { rally ? "TIME" : "SCORE", rally ? clockText( m_raceTime ) : format( "%lld", static_cast<long long>( m_score ) ) },
        { "STAGE", format( "%d", m_stage + 1 ) },
        { "OVERTAKES", format( "%d", m_overtakes ) },
        { "NEAR MISSES", format( "%d", m_nearMisses ) },
        { "TOP SPEED", format( "%d %s", static_cast<int>( m_topSpeed * ( mph ? 2.23694f : 3.6f ) ), mph ? "MPH" : "KM/H" ) },
    };
    for( int i = 0; i < 6; ++i )
    {
        const float ry = y + 86.0f * u + 30.0f * u * static_cast<float>( i );
        drawText( rows[i][0], x + 50.0f * u, ry, 16.0f * u, Dim );
        drawText( rows[i][1], x + w - 50.0f * u, ry, 16.0f * u, i == 1 && m_newBest ? Accent : Ink, AlignE::RIGHT );
    }
    std::string note = m_newBest ? "NEW BEST" : "";
    if( m_newRecord ) note = m_mode == ModeE::DAILY ? "BEST OF THE DAY" : "NEW RALLY RECORD";
    if( m_newGhost ) note += note.empty() ? "NEW GHOST SAVED" : "  -  NEW GHOST SAVED";
    if( !note.empty() ) drawTextFx( note, W * 0.5f, y + h - 34.0f * u, 16.0f * u, Accent, AlignE::CENTER, 1.0f );
    if( m_mode != ModeE::RALLY ) drawText( "TRACK CODE  " + m_code, W * 0.5f, y + h + 14.0f * u, 14.0f * u, Dim, AlignE::CENTER );
    drawMenu( "", gameOverItems(), {}, y + h + 46.0f * u );
}

void Game::drawRaceResults()
{
    drawHud();
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 140 } );
    const float w = 660.0f * u;
    const float x = W * 0.5f - w * 0.5f, y = H * 0.06f;
    const bool  champ = m_mode == ModeE::CHAMPIONSHIP;
    auto        nameOf = [&]( int driver ) { return driver < 0 ? std::string( "YOU" ) : m_race.m_drivers[driver].m_name; };
    auto        carOf = [&]( int driver ) { return std::string( carSpec( driver < 0 ? m_carIndex : m_race.m_drivers[driver].m_car ).m_name ); };
    const float rowH = 24.0f * u;

    if( m_resultPage == 2 )
    {
        const float h = 220.0f * u;
        panel( x, y + 80.0f * u, w, h );
        drawTextFx( m_place == 1 ? "CHAMPION!" : format( "CHAMPIONSHIP  P%d", m_place ), W * 0.5f, y + 120.0f * u, 46.0f * u,
                    m_place == 1 ? Gold : Ink, AlignE::CENTER, 1.0f );
        drawText( format( "%d POINTS", m_race.m_playerPoints ), W * 0.5f, y + 196.0f * u, 18.0f * u, Accent, AlignE::CENTER );
        drawText( m_place == 1 ? "CHAMPION GOLD PAINT UNLOCKED" : "WIN IT TO UNLOCK CHAMPION GOLD", W * 0.5f, y + 236.0f * u, 13.0f * u, Dim,
                  AlignE::CENTER );
        drawMenu( "", { "TITLE" }, {}, y + 330.0f * u );
        return;
    }
    const int lines = RivalCount + 1;
    const float h = 90.0f * u + rowH * static_cast<float>( lines );
    panel( x, y, w, h );
    if( m_resultPage == 0 )
    {
        drawTextFx( champ ? format( "RACE %d RESULT", m_race.m_round + 1 ) : std::string( "RACE RESULT" ), W * 0.5f, y + 18.0f * u, 28.0f * u,
                    Accent, AlignE::CENTER, 0.8f );
        const float winner = m_results.empty() ? 0.0f : m_results.front().m_time;
        for( size_t i = 0; i < m_results.size(); ++i )
        {
            const ResultLine &l = m_results[i];
            const float       ry = y + 66.0f * u + rowH * static_cast<float>( i );
            const Color       col = l.m_driver < 0 ? Accent : ( i == 0 ? Gold : Ink );
            drawText( format( "%2d", static_cast<int>( i ) + 1 ), x + 30.0f * u, ry, 14.0f * u, col );
            drawText( nameOf( l.m_driver ), x + 80.0f * u, ry, 14.0f * u, col );
            drawText( carOf( l.m_driver ), x + 300.0f * u, ry, 12.0f * u, Dim );
            drawText( i == 0 ? clockText( l.m_time ) : format( "+%.1f", l.m_time - winner ), x + w - 130.0f * u, ry, 14.0f * u, col, AlignE::RIGHT );
            if( champ ) drawText( l.m_points ? format( "+%d", l.m_points ) : std::string( "" ), x + w - 30.0f * u, ry, 14.0f * u, col, AlignE::RIGHT );
        }
        if( champ ) drawText( "CONTINUE", W * 0.5f, y + h + 18.0f * u, 16.0f * u, Ink, AlignE::CENTER );
        else drawMenu( "", { "RETRY", "TRACK LAB", "TITLE" }, {}, y + h + 20.0f * u );
        return;
    }
    // Standings.
    drawTextFx( format( "STANDINGS AFTER RACE %d", m_race.m_round + 1 ), W * 0.5f, y + 18.0f * u, 26.0f * u, Accent, AlignE::CENTER, 0.8f );
    const auto table = m_race.standings();
    for( size_t i = 0; i < table.size(); ++i )
    {
        const float ry = y + 66.0f * u + rowH * static_cast<float>( i );
        const Color col = table[i].first < 0 ? Accent : ( i == 0 ? Gold : Ink );
        drawText( format( "%2d", static_cast<int>( i ) + 1 ), x + 30.0f * u, ry, 14.0f * u, col );
        drawText( nameOf( table[i].first ), x + 80.0f * u, ry, 14.0f * u, col );
        drawText( carOf( table[i].first ), x + 300.0f * u, ry, 12.0f * u, Dim );
        drawText( format( "%d", table[i].second ), x + w - 30.0f * u, ry, 14.0f * u, col, AlignE::RIGHT );
    }
    const bool qualified = m_place <= QualifyPlace;
    const bool last = m_race.m_round + 1 >= RoundCount;
    drawText( qualified ? ( last ? std::string( "THE LAST RACE IS RUN" ) : format( "QUALIFIED FOR RACE %d", m_race.m_round + 2 ) )
                        : format( "NOT QUALIFIED - FINISH IN THE TOP %d", QualifyPlace ),
              W * 0.5f, y + h - 18.0f * u, 13.0f * u, qualified ? Green : Red, AlignE::CENTER );
    if( qualified && last ) drawMenu( "", { "FINAL RESULT" }, {}, y + h + 16.0f * u );
    else if( qualified ) drawMenu( "", { "NEXT RACE", "QUIT CHAMPIONSHIP" }, {}, y + h + 16.0f * u );
    else drawMenu( "", { "RETRY RACE", "QUIT CHAMPIONSHIP" }, {}, y + h + 16.0f * u );
}

void Game::drawLab()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 90 } );
    static const char *labels[] = { "MODE",      "SCENERY", "CURVES", "SHARPNESS", "HILLS", "TRAFFIC",
                                    "OBSTACLES", "LENGTH",  "SEED",   "CODE",      "START", "BACK" };
    std::vector<std::string> items, values;
    for( int i = 0; i < static_cast<int>( LabE::COUNT ); ++i )
    {
        items.push_back( labels[i] );
        values.push_back( labValue( static_cast<LabE>( i ) ) );
    }
    drawMenu( "TRACK LAB", items, values, H * 0.08f );
    drawText( "AUTO LEAVES A VALUE TO THE SCENERY.  NEW SCENERIES APPEAR AS YOU DISCOVER THEM.", W * 0.5f, H - 26.0f * u, 11.0f * u, Dim,
              AlignE::CENTER );
}

void Game::drawCodeEntry()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 140 } );
    const float w = 720.0f * u, h = 260.0f * u;
    const float x = W * 0.5f - w * 0.5f, y = H * 0.25f;
    panel( x, y, w, h );
    drawTextFx( "ENTER A TRACK CODE", W * 0.5f, y + 22.0f * u, 26.0f * u, Accent, AlignE::CENTER, 0.8f );
    const float box = 40.0f * u, gap = 8.0f * u, groupGap = 30.0f * u;
    const float total = 12.0f * box + 9.0f * gap + 2.0f * groupGap;
    float       bx = W * 0.5f - total * 0.5f;
    for( int i = 0; i < 12; ++i )
    {
        if( i == 4 || i == 8 ) bx += groupGap - gap;
        const bool sel = i == m_codeCursor;
        DrawRectangleRounded( Rectangle{ bx, y + 90.0f * u, box, box * 1.3f }, 0.2f, 6,
                              sel ? Color{ 255, 150, 40, static_cast<unsigned char>( 120 + 80 * std::sin( m_time * 8.0f ) ) } : Color{ 255, 255, 255, 30 } );
        drawText( std::string( 1, m_codeText.size() > static_cast<size_t>( i ) ? m_codeText[static_cast<size_t>( i )] : '0' ), bx + box * 0.5f,
                  y + 98.0f * u, 30.0f * u, Ink, AlignE::CENTER );
        bx += box + gap;
    }
    if( !m_codeError.empty() ) drawText( m_codeError, W * 0.5f, y + 170.0f * u, 14.0f * u, Red, AlignE::CENTER );
    const char *hint = m_input.m_gamepad ? "[DPAD] LETTER AND PLACE     [A] ACCEPT     [B] BACK"
                                         : "TYPE THE CODE     [ARROWS] EDIT     [ENTER] ACCEPT     [ESC] BACK";
    drawHint( hint, W * 0.5f, y + h - 40.0f * u, 12.0f * u, Dim, AlignE::CENTER );
}

void Game::drawInitials()
{
    drawHud();
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 150 } );
    const float w = 560.0f * u, h = 300.0f * u;
    const float x = W * 0.5f - w * 0.5f, y = H * 0.2f;
    panel( x, y, w, h );
    drawTextFx( format( "NEW HIGH SCORE  #%d", m_rank + 1 ), W * 0.5f, y + 24.0f * u, 30.0f * u, Gold, AlignE::CENTER, 1.0f );
    drawText( format( "%lld", static_cast<long long>( m_score ) ), W * 0.5f, y + 74.0f * u, 22.0f * u, Ink, AlignE::CENTER );
    for( int i = 0; i < 3; ++i )
    {
        const float cx = W * 0.5f + ( static_cast<float>( i ) - 1.0f ) * 90.0f * u;
        const bool  sel = i == m_initialCursor;
        if( sel )
            DrawRectangleRounded( Rectangle{ cx - 34.0f * u, y + 120.0f * u, 68.0f * u, 90.0f * u }, 0.2f, 6,
                                  Color{ 255, 150, 40, static_cast<unsigned char>( 90 + 60 * std::sin( m_time * 8.0f ) ) } );
        drawTextFx( std::string( 1, m_initials[static_cast<size_t>( i )] ), cx, y + 132.0f * u, 64.0f * u, sel ? Ink : Dim, AlignE::CENTER, 0.6f );
    }
    drawHint( m_input.m_gamepad ? "[DPAD] LETTER     [A] NEXT / DONE" : "TYPE YOUR INITIALS     [ENTER] DONE", W * 0.5f, y + h - 36.0f * u,
              12.0f * u, Dim, AlignE::CENTER );
}

void Game::drawRecords()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 150 } );
    const float w = 760.0f * u, h = 600.0f * u;
    const float x = W * 0.5f - w * 0.5f, y = std::max( 10.0f, H * 0.5f - h * 0.5f );
    panel( x, y, w, h );
    drawTextFx( "RECORDS", W * 0.5f, y + 18.0f * u, 30.0f * u, Accent, AlignE::CENTER, 0.8f );
    drawText( "ENDLESS TOP TEN", x + 30.0f * u, y + 70.0f * u, 13.0f * u, Dim );
    for( int i = 0; i < 10; ++i )
    {
        const float ry = y + 96.0f * u + 25.0f * u * static_cast<float>( i );
        drawText( format( "%2d", i + 1 ), x + 30.0f * u, ry, 14.0f * u, i == 0 ? Gold : Ink );
        if( i < static_cast<int>( m_records.m_top.size() ) )
        {
            const ScoreEntry &e = m_records.m_top[static_cast<size_t>( i )];
            drawText( e.m_initials, x + 80.0f * u, ry, 14.0f * u, i == 0 ? Gold : Ink );
            drawText( format( "%lld", static_cast<long long>( e.m_score ) ), x + 300.0f * u, ry, 14.0f * u, Ink, AlignE::RIGHT );
            drawText( format( "%.1f KM", e.m_distance / 1000.0 ), x + 420.0f * u, ry, 13.0f * u, Dim, AlignE::RIGHT );
            drawText( e.m_code, x + w - 30.0f * u, ry, 12.0f * u, Dim, AlignE::RIGHT );
        }
        else
        {
            drawText( "---", x + 80.0f * u, ry, 14.0f * u, Dim );
        }
    }
    const float sy = y + 360.0f * u;
    drawText( format( "DRIVEN  %.1f KM", m_records.m_totalKm ), x + 30.0f * u, sy, 14.0f * u, Ink );
    drawText( format( "CHAMPIONSHIPS WON  %d", m_records.m_titles ), x + 30.0f * u, sy + 26.0f * u, 14.0f * u, Ink );
    drawText( m_records.m_bestPlace ? format( "BEST CHAMPIONSHIP  P%d", m_records.m_bestPlace ) : std::string( "BEST CHAMPIONSHIP  -" ),
              x + 30.0f * u, sy + 52.0f * u, 14.0f * u, Ink );
    drawText( format( "PLACES SEEN  %d / %d", m_records.seenCount(), biomeCount() ), x + 30.0f * u, sy + 78.0f * u,
              14.0f * u, Ink );
    drawText( format( "COUNTRIES  %d / %d", m_records.countriesSeen(), countryCount() ), x + 30.0f * u, sy + 104.0f * u, 14.0f * u, Ink );
    drawText( format( "ROUND THE WORLD  %d / %d", m_records.sectorsSeen(), LongitudeSectors ), x + 30.0f * u, sy + 130.0f * u, 14.0f * u,
              m_records.circumnavigated() ? Gold : Ink );
    drawText( format( "POSTCARDS  %d", static_cast<int>( m_records.m_postcards.count() ) ), x + 30.0f * u, sy + 156.0f * u, 14.0f * u, Ink );
    int rallied = 0;
    for( double best : m_records.m_rallyBest ) rallied += best > 0.0 ? 1 : 0;
    drawText( format( "RALLIES  %d / %d     DAILY CHALLENGES  %d", rallied, static_cast<int>( rallies().size() ), m_records.m_dailyDays ), x + 30.0f * u,
              sy + 182.0f * u, 14.0f * u, Ink );
    drawText( "UNLOCKS", x + 430.0f * u, sy, 13.0f * u, Dim );
    float uy = sy + 24.0f * u;
    for( int c = 1; c < CarCount; ++c, uy += 22.0f * u )
        drawText( std::string( carSpec( c ).m_name ) + ( m_records.carUnlocked( c ) ? "  OK" : "  " + Records::carRequirement( c ) ), x + 430.0f * u,
                  uy, 12.0f * u, m_records.carUnlocked( c ) ? Green : Dim );
    int paints = 0, rims = 0, decals = 0;
    for( int p = 0; p < PaintCount; ++p ) paints += m_records.paintUnlocked( p ) ? 1 : 0;
    for( int r = 0; r < RimCount; ++r ) rims += m_records.rimsUnlocked( r ) ? 1 : 0;
    for( int d = 0; d < DecalCount; ++d ) decals += m_records.decalUnlocked( d ) ? 1 : 0;
    drawText( format( "PAINTS  %d / %d", paints, PaintCount ), x + 430.0f * u, uy, 12.0f * u, Ink );
    drawText( format( "RIMS  %d / %d", rims, RimCount ), x + 430.0f * u, uy + 22.0f * u, 12.0f * u, Ink );
    drawText( format( "DECALS  %d / %d", decals, DecalCount ), x + 430.0f * u, uy + 44.0f * u, 12.0f * u, Ink );
    drawText( "CAREER POINTS", x + 430.0f * u, uy + 80.0f * u, 12.0f * u, Dim );
    drawText( Records::pointsText( m_records.m_points ), x + 430.0f * u, uy + 102.0f * u, 16.0f * u, Gold );
    drawText( "BACK", W * 0.5f, y + h - 34.0f * u, 14.0f * u, Accent, AlignE::CENTER );
}

std::string Game::garageValue( GarageE item ) const
{
    auto mark = []( std::string name, bool unlocked ) { return unlocked ? name : name + "  (LOCKED)"; };
    switch( item )
    {
    case GarageE::CAR: return mark( carSpec( m_carIndex ).m_name, m_records.carUnlocked( m_carIndex ) );
    case GarageE::PAINT: return mark( paintName( m_paintIndex ), m_records.paintUnlocked( m_paintIndex ) );
    case GarageE::RIMS: return mark( rimName( m_settings.m_rims ), m_records.rimsUnlocked( m_settings.m_rims ) );
    case GarageE::DECAL: return mark( decalName( m_settings.m_decal ), m_records.decalUnlocked( m_settings.m_decal ) );
    case GarageE::DECAL_COLOR: return m_settings.m_decal == 0 ? std::string( "-" ) : std::string( paintName( m_settings.m_decalColor ) );
    default: return "";
    }
}

void Game::changeGarage( GarageE item, int dir )
{
    auto step = [dir]( int v, int n ) { return ( v + n + dir ) % n; };
    switch( item )
    {
    case GarageE::CAR:
        m_carIndex = step( m_carIndex, CarCount );
        m_car.m_spec = carSpec( m_carIndex );
        break;
    case GarageE::PAINT: m_paintIndex = step( m_paintIndex, PaintCount ); break;
    case GarageE::RIMS: m_settings.m_rims = step( m_settings.m_rims, RimCount ); break;
    case GarageE::DECAL: m_settings.m_decal = step( m_settings.m_decal, DecalCount ); break;
    case GarageE::DECAL_COLOR: m_settings.m_decalColor = step( m_settings.m_decalColor, PaintCount ); break;
    default: return;
    }
    applyLivery();
    saveSettings( m_settings );
    m_audio.play( SfxE::TICK );
}

void Game::drawGarage()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangleGradientV( 0, static_cast<int>( H * 0.45f ), static_cast<int>( W ), static_cast<int>( H * 0.55f ), Color{ 0, 0, 0, 0 }, Color{ 0, 0, 0, 170 } );
    drawTextFx( "GARAGE", W * 0.5f, 26.0f * u, 34.0f * u, Accent, AlignE::CENTER, 0.8f );
    drawText( Records::pointsText( m_records.m_points ), W * 0.5f, 72.0f * u, 14.0f * u, Gold, AlignE::CENTER );
    std::vector<std::string> labels = { "CAR", "PAINT", "RIMS", "DECAL", "DECAL COLOUR", "BACK" }, values;
    for( int i = 0; i < static_cast<int>( GarageE::COUNT ); ++i ) values.push_back( garageValue( static_cast<GarageE>( i ) ) );
    const float my = H - ( 90.0f + 40.0f * 6.0f + 60.0f ) * u;
    drawMenu( "", labels, values, my );

    // Colour chips beside the paints.
    const float w = 720.0f * u, x = W * 0.5f - w * 0.5f, rowH = 40.0f * u;
    for( const auto [item, paint] : { std::pair{ GarageE::PAINT, m_paintIndex }, std::pair{ GarageE::DECAL_COLOR, m_settings.m_decalColor } } )
    {
        if( item == GarageE::DECAL_COLOR && m_settings.m_decal == 0 ) continue;
        const int         row = static_cast<int>( item );
        const std::string v = row == m_menuIndex ? "< " + values[static_cast<size_t>( row )] + " >" : values[static_cast<size_t>( row )];
        const float       cx = x + w - 40.0f * u - textWidth( v, 18.0f * u ) - 30.0f * u, cy = my + 76.0f * u + rowH * static_cast<float>( row );
        DrawRectangleRounded( Rectangle{ cx, cy - 1.0f * u, 20.0f * u, 18.0f * u }, 0.3f, 6, paintColor( paint ) );
        DrawRectangleRoundedLinesEx( Rectangle{ cx, cy - 1.0f * u, 20.0f * u, 18.0f * u }, 0.3f, 6, 1.0f, Color{ 255, 255, 255, 120 } );
    }

    // What the selected thing needs, and the car's figures.
    std::string need;
    const auto  item = static_cast<GarageE>( m_menuIndex );
    if( item == GarageE::CAR && !m_records.carUnlocked( m_carIndex ) ) need = Records::carRequirement( m_carIndex );
    if( item == GarageE::PAINT && !m_records.paintUnlocked( m_paintIndex ) ) need = Records::paintRequirement( m_paintIndex );
    if( item == GarageE::RIMS && !m_records.rimsUnlocked( m_settings.m_rims ) ) need = Records::rimsRequirement( m_settings.m_rims );
    if( item == GarageE::DECAL && !m_records.decalUnlocked( m_settings.m_decal ) ) need = Records::decalRequirement( m_settings.m_decal );
    if( !need.empty() ) drawTextFx( "UNLOCKS AT  " + need, W * 0.5f, my - 30.0f * u, 15.0f * u, Red, AlignE::CENTER, 0.4f );

    const CarSpec &spec = carSpec( m_carIndex );
    const float    px = 24.0f * u, py = 110.0f * u;
    panel( px, py, 210.0f * u, 176.0f * u );
    const float vals[3] = { ( spec.m_topSpeed - 60.0f ) / 32.0f, ( spec.m_accel - 0.8f ) / 0.45f, ( spec.m_grip - 0.8f ) / 0.4f };
    const char *names[3] = { "TOP SPEED", "ACCELERATION", "GRIP" };
    for( int i = 0; i < 3; ++i )
    {
        const float ry = py + 20.0f * u + 42.0f * u * static_cast<float>( i );
        drawText( names[i], px + 16.0f * u, ry, 12.0f * u, Dim );
        DrawRectangle( static_cast<int>( px + 16.0f * u ), static_cast<int>( ry + 20.0f * u ), static_cast<int>( 178.0f * u ), static_cast<int>( 8.0f * u ),
                       Color{ 255, 255, 255, 40 } );
        DrawRectangle( static_cast<int>( px + 16.0f * u ), static_cast<int>( ry + 20.0f * u ), static_cast<int>( 178.0f * u * clampf( vals[i], 0.08f, 1.0f ) ),
                       static_cast<int>( 8.0f * u ), Accent );
    }
    const char *engines[CarCount] = { "V8", "INLINE 4", "V12", "TURBO 4  RALLY" };
    drawText( std::string( spec.m_name ) + "  " + engines[std::clamp( m_carIndex, 0, CarCount - 1 )], px + 16.0f * u, py + 146.0f * u, 12.0f * u, Ink );
}

void Game::drawWorldMap( float x, float y, float w, float h )
{
    const float u = unit();
    const float latTop = 78.0f, latBottom = -58.0f;
    auto        at = [&]( const GeoPoint &g )
    { return Vector2{ x + ( g.m_lon + 180.0f ) / 360.0f * w, y + ( latTop - g.m_lat ) / ( latTop - latBottom ) * h }; };
    DrawRectangleRounded( Rectangle{ x, y, w, h }, 0.03f, 8, Color{ 12, 30, 60, 230 } );

    // The countries and provinces, those you have been to lit: painted at twice the size, again when more are seen.
    const int tw = std::min( 4096, static_cast<int>( w * 2.0f ) ), th = std::min( 4096, static_cast<int>( h * 2.0f ) );
    const int seenNow = static_cast<int>( m_records.m_seen.count() );
    if( !m_mapTex.id || m_mapSeen != seenNow || m_mapW != tw || m_mapH != th )
    {
        std::vector<GeoPoint> seenAt;
        for( int i = 0; i < biomeCount(); ++i )
        {
            GeoPoint g;
            if( m_records.m_seen.test( static_cast<size_t>( i ) ) && placeLocation( static_cast<BiomeE>( i ), g ) ) seenAt.push_back( g );
        }
        Image img = paintWorldMap( tw, th, latTop, latBottom, seenAt );
        if( m_mapTex.id ) UnloadTexture( m_mapTex );
        m_mapTex = LoadTextureFromImage( img );
        SetTextureFilter( m_mapTex, TEXTURE_FILTER_BILINEAR );
        UnloadImage( img );
        m_mapSeen = seenNow;
        m_mapW = tw;
        m_mapH = th;
    }
    DrawTexturePro( m_mapTex, Rectangle{ 0, 0, static_cast<float>( tw ), static_cast<float>( th ) }, Rectangle{ x, y, w, h }, Vector2{}, 0.0f, WHITE );
    // What the colours mean.
    {
        const float lx = x + 14.0f * u, ly = y + h - 22.0f * u, box = 10.0f * u;
        DrawRectangle( static_cast<int>( lx ), static_cast<int>( ly ), static_cast<int>( box ), static_cast<int>( box ), Color{ 52, 112, 118, 255 } );
        drawText( "COUNTRY BEEN TO", lx + box + 6.0f * u, ly - 1.0f * u, 10.0f * u, Dim );
        const float lx2 = lx + box + 18.0f * u + textWidth( "COUNTRY BEEN TO", 10.0f * u );
        DrawRectangle( static_cast<int>( lx2 ), static_cast<int>( ly ), static_cast<int>( box ), static_cast<int>( box ), Color{ 86, 176, 150, 255 } );
        drawText( "PROVINCE BEEN TO", lx2 + box + 6.0f * u, ly - 1.0f * u, 10.0f * u, Dim );
    }

    // The slices of the globe already driven through, then the grid.
    std::bitset<LongitudeSectors> sectors;
    for( int i = 0; i < biomeCount(); ++i )
    {
        GeoPoint g;
        if( m_records.m_seen.test( static_cast<size_t>( i ) ) && placeLocation( static_cast<BiomeE>( i ), g ) )
            sectors.set( static_cast<size_t>( longitudeSector( g.m_lon ) ) );
    }
    const float sw = w / static_cast<float>( LongitudeSectors );
    for( int k = 0; k < LongitudeSectors; ++k )
        if( sectors.test( static_cast<size_t>( k ) ) ) DrawRectangle( static_cast<int>( x + sw * k ), static_cast<int>( y ), static_cast<int>( sw ), static_cast<int>( h ), Color{ 90, 255, 150, 8 } );
    for( int lon = -150; lon <= 150; lon += 30 )
        DrawLineEx( at( GeoPoint{ latTop, static_cast<float>( lon ) } ), at( GeoPoint{ latBottom, static_cast<float>( lon ) } ), 1.0f, Color{ 255, 255, 255, 22 } );
    for( int lat = -30; lat <= 60; lat += 30 )
        DrawLineEx( at( GeoPoint{ static_cast<float>( lat ), -180.0f } ), at( GeoPoint{ static_cast<float>( lat ), 180.0f } ), lat == 0 ? 1.5f : 1.0f,
                    Color{ 255, 255, 255, static_cast<unsigned char>( lat == 0 ? 40 : 22 ) } );

    // The route, its places, and where the car is.
    const auto &route = worldRoute();
    GeoPoint    prev{};
    bool        havePrev = false;
    for( BiomeE b : route )
    {
        GeoPoint g;
        if( !placeLocation( b, g ) ) continue;
        if( havePrev && std::abs( g.m_lon - prev.m_lon ) < 180.0f ) DrawLineEx( at( prev ), at( g ), 1.2f * u, Color{ 255, 255, 255, 36 } );
        prev = g;
        havePrev = true;
    }
    // The way actually driven, from the travel journal.
    GeoPoint last{};
    bool     haveLast = false;
    for( const std::string &key : m_records.m_journey )
    {
        const int id = biomeId( key );
        GeoPoint  g;
        if( id < 0 || !placeLocation( static_cast<BiomeE>( id ), g ) )
        {
            haveLast = false;
            continue;
        }
        if( haveLast && std::abs( g.m_lon - last.m_lon ) < 180.0f ) DrawLineEx( at( last ), at( g ), 2.2f * u, Color{ 255, 150, 40, 170 } );
        last = g;
        haveLast = true;
    }
    for( BiomeE b : route )
    {
        GeoPoint g;
        if( !placeLocation( b, g ) ) continue;
        const Vector2 p = at( g );
        if( m_records.m_seen.test( static_cast<size_t>( b ) ) )
        {
            DrawCircleV( p, 5.5f * u, Color{ 255, 150, 40, 50 } );
            DrawCircleV( p, 2.8f * u, Accent );
        }
        else
        {
            DrawCircleV( p, 1.8f * u, Color{ 190, 196, 210, 90 } );
        }
    }
    GeoPoint here;
    if( placeLocation( m_track->biomeMix( m_car.m_s ).m_from, here ) )
    {
        const float pulse = 0.5f + 0.5f * std::sin( m_time * 4.0f );
        DrawCircleLinesV( at( here ), ( 8.0f + 5.0f * pulse ) * u, Color{ 255, 255, 255, static_cast<unsigned char>( 120 + 100 * pulse ) } );
    }

    // Round the world: a box per slice.
    for( int k = 0; k < LongitudeSectors; ++k )
    {
        const Rectangle r = { x + sw * k + 3.0f * u, y + h + 8.0f * u, sw - 6.0f * u, 8.0f * u };
        if( sectors.test( static_cast<size_t>( k ) ) ) DrawRectangleRec( r, Green );
        else DrawRectangleLinesEx( r, 1.0f, Color{ 190, 196, 210, 90 } );
    }
}

void Game::drawStamps( float x, float y, float w, float h )
{
    const float u = unit();
    // Countries in the order the tour meets them, and how many of their places were seen.
    std::vector<std::pair<const char *, int>> countries;
    for( BiomeE b : worldRoute() )
    {
        const char *c = biomeDef( b ).m_country;
        if( !c ) continue;
        auto it = std::find_if( countries.begin(), countries.end(), [c]( const auto &e ) { return std::strcmp( e.first, c ) == 0; } );
        if( it == countries.end() )
        {
            countries.push_back( { c, 0 } );
            it = countries.end() - 1;
        }
        if( m_records.m_seen.test( static_cast<size_t>( b ) ) ) ++it->second;
    }
    const int cols = 6, rows = 6, perSheet = cols * rows;
    const int sheets = std::max( 1, ( static_cast<int>( countries.size() ) + perSheet - 1 ) / perSheet );
    m_passportSheet = std::clamp( m_passportSheet, 0, sheets - 1 );
    const float cw = w / cols, ch = h / rows;
    static const Color inks[6] = { { 255, 96, 96, 255 }, { 120, 170, 255, 255 }, { 100, 225, 150, 255 },
                                   { 205, 140, 255, 255 }, { 255, 185, 95, 255 }, { 95, 225, 235, 255 } };
    for( int i = 0; i < perSheet; ++i )
    {
        const size_t idx = static_cast<size_t>( m_passportSheet * perSheet + i );
        if( idx >= countries.size() ) break;
        const auto [name, seen] = countries[idx];
        const float     cx = x + cw * static_cast<float>( i % cols ), cy = y + ch * static_cast<float>( i / cols );
        const Rectangle r = { cx + 6.0f * u, cy + 6.0f * u, cw - 12.0f * u, ch - 12.0f * u };
        const Color     ink = seen > 0 ? inks[hash64( std::strlen( name ) * 131 + static_cast<uint64_t>( name[0] ) * 7 + static_cast<uint64_t>( name[1] ) ) % 6]
                                       : Color{ 190, 196, 210, 70 };
        if( seen > 0 ) DrawRectangleRounded( r, 0.18f, 6, Color{ ink.r, ink.g, ink.b, 34 } );
        DrawRectangleRoundedLinesEx( r, 0.18f, 6, seen > 0 ? 2.0f * u : 1.0f, ink );
        // The name, broken into lines that fit.
        std::vector<std::string> lines( 1 );
        const float              size = 11.0f * u;
        for( const char *p = name; *p; )
        {
            const char *e = std::strchr( p, ' ' );
            const std::string word( p, e ? static_cast<size_t>( e - p ) : std::strlen( p ) );
            const std::string tryLine = lines.back().empty() ? word : lines.back() + " " + word;
            if( !lines.back().empty() && textWidth( tryLine, size ) > r.width - 12.0f * u ) lines.push_back( word );
            else lines.back() = tryLine;
            p = e ? e + 1 : p + word.size();
        }
        float ty = r.y + r.height * 0.5f - static_cast<float>( lines.size() ) * 8.0f * u - ( seen > 0 ? 6.0f * u : 0.0f );
        for( const std::string &line : lines )
        {
            drawText( line, r.x + r.width * 0.5f, ty, size, seen > 0 ? ink : Color{ 190, 196, 210, 110 }, AlignE::CENTER );
            ty += 16.0f * u;
        }
        if( seen > 0 ) drawText( seen == 1 ? std::string( "1 PLACE" ) : format( "%d PLACES", seen ), r.x + r.width * 0.5f, ty + 2.0f * u, 9.0f * u, Dim, AlignE::CENTER );
    }
    drawText( format( "SHEET %d / %d", m_passportSheet + 1, sheets ), x + w, y + h + 8.0f * u, 11.0f * u, Dim, AlignE::RIGHT );
}

void Game::drawPostcards( float x, float y, float w, float h )
{
    const float      u = unit();
    std::vector<int> cards;
    for( BiomeE b : worldRoute() )
        if( m_records.m_postcards.test( static_cast<size_t>( b ) ) ) cards.push_back( static_cast<int>( b ) );
    for( int i = 0; i < biomeCount(); ++i )
        if( m_records.m_postcards.test( static_cast<size_t>( i ) ) && routeIndex( static_cast<BiomeE>( i ) ) < 0 ) cards.push_back( i );
    if( cards.empty() )
    {
        drawText( "NO POSTCARDS YET", x + w * 0.5f, y + h * 0.42f, 20.0f * u, Ink, AlignE::CENTER );
        drawText( "EVERY NEW PLACE YOU DRIVE THROUGH SENDS ONE HOME", x + w * 0.5f, y + h * 0.42f + 34.0f * u, 13.0f * u, Dim, AlignE::CENTER );
        return;
    }
    const int cols = 3, rows = 2, perSheet = cols * rows;
    const int sheets = ( static_cast<int>( cards.size() ) + perSheet - 1 ) / perSheet;
    m_passportSheet = std::clamp( m_passportSheet, 0, sheets - 1 );
    const float cw = w / cols, ch = h / rows;
    for( int i = 0; i < perSheet; ++i )
    {
        const size_t idx = static_cast<size_t>( m_passportSheet * perSheet + i );
        if( idx >= cards.size() ) break;
        const int biome = cards[idx];
        if( !m_postcardTex.contains( biome ) )
        {
            const std::string file = configDirectory() + "/postcards/" + std::to_string( biome ) + ".png";
            m_postcardTex[biome] = FileExists( file.c_str() ) ? LoadTexture( file.c_str() ) : Texture2D{};
            if( m_postcardTex[biome].id ) SetTextureFilter( m_postcardTex[biome], TEXTURE_FILTER_BILINEAR );
        }
        const Texture2D &tex = m_postcardTex[biome];
        const float      cx = x + cw * static_cast<float>( i % cols ) + 10.0f * u, cy = y + ch * static_cast<float>( i / cols ) + 8.0f * u;
        const float      pw = cw - 20.0f * u, ph = pw * 9.0f / 16.0f;
        // A white card with the picture and the place written under it.
        const float tilt = ( static_cast<float>( hash64( static_cast<uint64_t>( biome ) ) % 5 ) - 2.0f ) * 0.6f;
        DrawRectanglePro( Rectangle{ cx + pw * 0.5f, cy + ( ph + 44.0f * u ) * 0.5f, pw + 12.0f * u, ph + 44.0f * u }, Vector2{ ( pw + 12.0f * u ) * 0.5f, ( ph + 44.0f * u ) * 0.5f },
                          tilt, Color{ 244, 240, 228, 255 } );
        if( tex.id ) DrawTexturePro( tex, Rectangle{ 0, 0, static_cast<float>( tex.width ), static_cast<float>( tex.height ) }, Rectangle{ cx, cy + 6.0f * u, pw, ph }, Vector2{}, 0.0f, WHITE );
        else DrawRectangle( static_cast<int>( cx ), static_cast<int>( cy + 6.0f * u ), static_cast<int>( pw ), static_cast<int>( ph ), Color{ 70, 80, 100, 255 } );
        const BiomeDef &d = biomeDef( static_cast<BiomeE>( biome ) );
        drawText( d.m_name, cx + 4.0f * u, cy + ph + 14.0f * u, 12.0f * u, Color{ 30, 34, 48, 255 }, AlignE::LEFT, 0.16f );
        if( d.m_country && std::strcmp( d.m_country, d.m_name ) != 0 )
            drawText( d.m_country, cx + pw - 4.0f * u, cy + ph + 16.0f * u, 9.0f * u, Color{ 110, 70, 50, 255 }, AlignE::RIGHT );
    }
    drawText( format( "%d %s   SHEET %d / %d", static_cast<int>( cards.size() ), cards.size() == 1 ? "POSTCARD" : "POSTCARDS", m_passportSheet + 1, sheets ), x + w, y + h + 8.0f * u, 11.0f * u, Dim,
              AlignE::RIGHT );
}

void Game::drawAlbum( float x, float y, float w, float h )
{
    const float u = unit();
    // Every landmark a place shows once per stage counts; the photos in the order they were taken.
    std::vector<std::string> all;
    for( int b = 0; b < biomeCount(); ++b )
        for( const PropRule &pr : biomeDef( static_cast<BiomeE>( b ) ).m_props )
            if( pr.m_kind == PropKindE::LANDMARK && pr.m_perStage > 0 && pr.m_variant >= 0 )
            {
                const std::string name = landmarkName( pr.m_variant );
                if( !name.empty() && std::find( all.begin(), all.end(), name ) == all.end() ) all.push_back( name );
            }
    const auto &photos = m_records.m_album;
    if( photos.empty() )
    {
        drawText( "NO PHOTOS YET", x + w * 0.5f, y + h * 0.42f, 20.0f * u, Ink, AlignE::CENTER );
        drawText( "EVERY LANDMARK YOU PASS GETS ITS PHOTO", x + w * 0.5f, y + h * 0.42f + 34.0f * u, 13.0f * u, Dim, AlignE::CENTER );
        return;
    }
    const int cols = 3, rows = 2, perSheet = cols * rows;
    const int sheets = ( static_cast<int>( photos.size() ) + perSheet - 1 ) / perSheet;
    m_passportSheet = std::clamp( m_passportSheet, 0, sheets - 1 );
    const float cw = w / cols, ch = h / rows;
    for( int i = 0; i < perSheet; ++i )
    {
        const size_t idx = static_cast<size_t>( m_passportSheet * perSheet + i );
        if( idx >= photos.size() ) break;
        const std::string &name = photos[idx];
        if( !m_albumTex.contains( name ) )
        {
            const std::string file = configDirectory() + "/album/" + name + ".png";
            m_albumTex[name] = FileExists( file.c_str() ) ? LoadTexture( file.c_str() ) : Texture2D{};
            if( m_albumTex[name].id ) SetTextureFilter( m_albumTex[name], TEXTURE_FILTER_BILINEAR );
        }
        const Texture2D &tex = m_albumTex[name];
        const float      cx = x + cw * static_cast<float>( i % cols ) + 10.0f * u, cy = y + ch * static_cast<float>( i / cols ) + 8.0f * u;
        const float      pw = cw - 20.0f * u, ph = pw * 9.0f / 16.0f;
        // A photo with a white border, its name written under it.
        DrawRectangle( static_cast<int>( cx - 6.0f * u ), static_cast<int>( cy ), static_cast<int>( pw + 12.0f * u ), static_cast<int>( ph + 40.0f * u ),
                       Color{ 248, 248, 244, 255 } );
        if( tex.id ) DrawTexturePro( tex, Rectangle{ 0, 0, static_cast<float>( tex.width ), static_cast<float>( tex.height ) }, Rectangle{ cx, cy + 6.0f * u, pw, ph }, Vector2{}, 0.0f, WHITE );
        else DrawRectangle( static_cast<int>( cx ), static_cast<int>( cy + 6.0f * u ), static_cast<int>( pw ), static_cast<int>( ph ), Color{ 70, 80, 100, 255 } );
        std::string shown = name;
        std::replace( shown.begin(), shown.end(), '_', ' ' );
        drawText( shown, cx + pw * 0.5f, cy + ph + 14.0f * u, 12.0f * u, Color{ 30, 34, 48, 255 }, AlignE::CENTER, 0.16f );
    }
    drawText( format( "%d OF %d LANDMARKS   SHEET %d / %d", static_cast<int>( photos.size() ), static_cast<int>( all.size() ), m_passportSheet + 1, sheets ),
              x + w, y + h + 8.0f * u, 11.0f * u, Dim, AlignE::RIGHT );
}

void Game::openViewer()
{
    if( m_state == StateE::VIEWER ) return;
    // From a drive it comes back to the pause menu.
    if( m_state == StateE::RACE || m_state == StateE::COUNTDOWN )
    {
        m_pauseReturn = m_state;
        m_menuIndex = 0;
        m_viewerReturn = StateE::PAUSED;
    }
    else
    {
        m_viewerReturn = m_state;
    }
    m_state = StateE::VIEWER;
    m_stateTime = 0.0f;
    m_viewerBuilt = -1;
}

void Game::closeViewer()
{
    if( m_viewerMesh.vaoId ) UnloadMesh( m_viewerMesh );
    if( m_viewerGround.vaoId ) UnloadMesh( m_viewerGround );
    m_viewerMesh = Mesh{};
    m_viewerGround = Mesh{};
    m_viewerBuilt = -1;
    m_state = m_viewerReturn;
    m_stateTime = 0.0f;
}

void Game::viewerStep( const MenuInput &mi, float dt )
{
    if( mi.m_back && m_stateTime > 0.2f )
    {
        closeViewer();
        return;
    }
    const int count = std::max( 1, landmarkCount() );
    const int step = ( mi.m_right ? 1 : 0 ) - ( mi.m_left ? 1 : 0 ) + ( IsKeyPressed( KEY_PAGE_DOWN ) ? 10 : 0 ) - ( IsKeyPressed( KEY_PAGE_UP ) ? 10 : 0 );
    if( step != 0 )
    {
        m_viewerIndex = ( ( m_viewerIndex + step ) % count + count ) % count;
        m_viewerZoom = 1.0f;
        m_audio.play( SfxE::TICK );
    }
    if( mi.m_up ) m_viewerZoom = std::max( 0.3f, m_viewerZoom * 0.85f );
    if( mi.m_down ) m_viewerZoom = std::min( 3.0f, m_viewerZoom / 0.85f );
    const float wheel = GetMouseWheelMove();
    if( wheel != 0.0f ) m_viewerZoom = clampf( m_viewerZoom * std::pow( 0.9f, wheel ), 0.3f, 3.0f );
    // The mouse turns it; left alone it turns slowly by itself.
    if( IsMouseButtonDown( MOUSE_BUTTON_LEFT ) )
    {
        const Vector2 d = GetMouseDelta();
        m_viewerYaw -= d.x * 0.008f;
        m_viewerPitch = clampf( m_viewerPitch + d.y * 0.006f, 0.02f, 1.35f );
    }
    else if( m_options.m_shapeTour == 0 )
    {
        m_viewerYaw += 0.22f * dt;
    }
}

void Game::buildViewerMesh()
{
    if( m_viewerMesh.vaoId ) UnloadMesh( m_viewerMesh );
    if( m_viewerGround.vaoId ) UnloadMesh( m_viewerGround );
    m_viewerBuilt = m_viewerIndex;
    const int  shape = m_viewerIndex - compiledLandmarks();
    const bool creature = shape >= 0 && shape < static_cast<int>( dataShapes().size() ) && dataShapes()[static_cast<size_t>( shape )].m_creature;
    // The places that show it; the first lends its colours, its sky and its ground.
    const PropRule *paint = nullptr;
    m_viewerPlace = -1;
    m_viewerPlaces.clear();
    for( int b = 0; b < biomeCount(); ++b )
    {
        const BiomeDef &d = biomeDef( static_cast<BiomeE>( b ) );
        bool            shows = false;
        for( const PropRule &pr : d.m_props )
            if( pr.m_kind == PropKindE::LANDMARK && pr.m_variant == m_viewerIndex )
            {
                shows = true;
                if( !paint ) paint = &pr;
            }
        for( const SwimRule &r : d.m_swim ) shows = shows || ( shape >= 0 && r.m_shape == shape );
        if( !shows ) continue;
        if( m_viewerPlace < 0 ) m_viewerPlace = b;
        if( d.m_name ) m_viewerPlaces.push_back( d.m_name );
    }
    const Color *defaults = m_models.keys( PropKindE::LANDMARK );
    Color        keys[3] = {};
    if( defaults )
    {
        keys[0] = paint && paint->m_a.a ? paint->m_a : defaults[0];
        keys[1] = paint && paint->m_b.a ? paint->m_b : defaults[1];
        keys[2] = paint && paint->m_c.a ? paint->m_c : defaults[2];
    }
    const MeshBuilder &src = m_models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( m_viewerIndex ) );
    const Vector3      e = src.extent();
    const float        lift = creature ? 2.0f + e.y : 0.0f;
    MeshBuilder        shown;
    shown.append( src, Vector3{ 0.0f, lift, 0.0f }, 0.0f, 1.0f, 1.0f, defaults ? keys : nullptr );
    m_viewerMesh = shown.upload();
    m_viewerSize = e;
    m_viewerCenterY = creature ? lift : e.y * 0.42f;
    // The ground: the place's land near, its far land out to the horizon.
    const BiomeDef &place = biomeDef( static_cast<BiomeE>( std::max( 0, m_viewerPlace ) ) );
    MeshBuilder     ground;
    const int       n = 48;
    auto            at = []( float r, float a ) { return Vector3{ std::cos( a ) * r, -0.05f, std::sin( a ) * r }; };
    for( int k = 0; k < n; ++k )
    {
        const float a0 = Tau * static_cast<float>( k ) / static_cast<float>( n ), a1 = Tau * static_cast<float>( k + 1 ) / static_cast<float>( n );
        ground.quad( at( 150.0f, a0 ), at( 1.0f, a0 ), at( 1.0f, a1 ), at( 150.0f, a1 ), place.m_surface.m_groundA );
        ground.quad( at( 3000.0f, a0 ), at( 150.0f, a0 ), at( 150.0f, a1 ), at( 3000.0f, a1 ), place.m_surface.m_groundFar );
    }
    m_viewerGround = ground.upload();
}

void Game::viewerScene( SceneInput &in, CameraView &cam )
{
    const int count = std::max( 1, landmarkCount() );
    m_viewerIndex = ( m_viewerIndex % count + count ) % count;
    if( m_viewerBuilt != m_viewerIndex ) buildViewerMesh();
    const BiomeDef &place = biomeDef( static_cast<BiomeE>( std::max( 0, m_viewerPlace ) ) );
    in.m_world = nullptr;
    in.m_player = nullptr;
    in.m_traffic = nullptr;
    in.m_particles = nullptr;
    in.m_cars.clear();
    in.m_police.clear();
    in.m_env = place.m_env;
    in.m_env.m_underwater = place.m_underwater ? 1.0f : 0.0f;
    in.m_surface = place.m_surface;
    in.m_speedBlur = 0.0f;
    in.m_aberration = 0.0f;
    in.m_windStreaks = 0.0f;
    in.m_lensRain = 0.0f;
    in.m_showcase = true;
    in.m_pieces = { { &m_viewerGround, Vec3d{}, 0.0f }, { &m_viewerMesh, Vec3d{}, 0.0f } };
    in.m_focus = Vec3d{ 0.0, m_viewerCenterY, 0.0 };
    // Round the model, far enough to see all of it.
    const float   radius = std::max( { m_viewerSize.x, m_viewerSize.y * 0.6f, m_viewerSize.z, 2.0f } );
    const float   dist = ( radius * 2.3f + 6.0f ) * m_viewerZoom, c = std::cos( m_viewerPitch );
    const Vector3 target = { 0.0f, m_viewerCenterY, 0.0f };
    const Vector3 pos = { std::sin( m_viewerYaw ) * c * dist, std::max( 1.0f, m_viewerCenterY + std::sin( m_viewerPitch ) * dist ), std::cos( m_viewerYaw ) * c * dist };
    cam.m_pos = Vec3d{ pos.x, pos.y, pos.z };
    cam.m_forward = Vector3Normalize( Vector3Subtract( target, pos ) );
    cam.m_up = Vector3{ 0.0f, 1.0f, 0.0f };
    cam.m_fovY = 50.0f;
}

void Game::drawViewer()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() ), u = unit();
    const int   shape = m_viewerIndex - compiledLandmarks();
    const bool  data = shape >= 0 && shape < static_cast<int>( dataShapes().size() );
    const bool  creature = data && dataShapes()[static_cast<size_t>( shape )].m_creature;
    std::string name = landmarkName( m_viewerIndex );
    std::replace( name.begin(), name.end(), '_', ' ' );
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( 128.0f * u ), Color{ 0, 0, 0, 120 } );
    DrawRectangle( 0, static_cast<int>( H - 52.0f * u ), static_cast<int>( W ), static_cast<int>( 52.0f * u ), Color{ 0, 0, 0, 120 } );
    drawTextFx( "LANDMARK VIEWER", 28.0f * u, 22.0f * u, 13.0f * u, Accent, AlignE::LEFT, 0.3f );
    drawTextFx( name, 28.0f * u, 44.0f * u, 28.0f * u, Ink, AlignE::LEFT, 0.6f );
    drawText( format( "%s %d / %d   %s   %.0f X %.0f X %.0f M", creature ? "CREATURE" : "LANDMARK", m_viewerIndex + 1, landmarkCount(),
                      data ? "DATA FILES" : "BUILT IN", m_viewerSize.x * 2.0f, m_viewerSize.y, m_viewerSize.z * 2.0f ),
              28.0f * u, 86.0f * u, 12.0f * u, Dim );
    std::string where;
    for( size_t i = 0; i < m_viewerPlaces.size() && i < 4; ++i ) where += ( i ? ",  " : "" ) + std::string( m_viewerPlaces[i] );
    if( m_viewerPlaces.size() > 4 ) where += format( "  AND %d MORE", static_cast<int>( m_viewerPlaces.size() ) - 4 );
    drawText( where.empty() ? std::string( "IN NO PLACE YET" ) : "IN  " + where, 28.0f * u, 106.0f * u, 12.0f * u, Dim );
    const int errors = dataErrors();
    if( errors > 0 ) drawText( format( "%d DATA ERRORS", errors ), W - 28.0f * u, 22.0f * u, 13.0f * u, Red, AlignE::RIGHT );
    drawHint( m_input.m_gamepad ? "[DPAD] MODEL AND ZOOM     [B] BACK"
                                : "[LEFT] [RIGHT] MODEL     [UP] [DOWN] ZOOM     [MOUSE] TURN     [F5] RELOAD DATA     [ESC] BACK",
              W * 0.5f, H - 30.0f * u, 11.0f * u, Color{ 255, 255, 255, 190 }, AlignE::CENTER );
}

void Game::buildCrossingScene()
{
    unloadCrossingScene();
    const bool ferry = m_crossing.m_type == CrossingE::FERRY;
    const int  vessel = landmarkByName( ferry ? "FERRY_SHIP" : "CAR_TRAIN" );
    if( vessel >= 0 ) m_crossVessel = m_models.prop( PropKindE::LANDMARK, static_cast<uint64_t>( vessel ) ).upload();
    const BiomeDef &from = biomeDef( m_track->stageBiome( static_cast<int>( m_crossing.m_board / StageLength ) ) );
    const BiomeDef &to = biomeDef( m_track->stageBiome( static_cast<int>( m_crossing.m_land / StageLength ) ) );
    MeshBuilder     m;
    if( ferry )
    {
        // The sea, a disc to the horizon.
        const int n = 48;
        for( int k = 0; k < n; ++k )
        {
            const float a0 = Tau * static_cast<float>( k ) / static_cast<float>( n ), a1 = Tau * static_cast<float>( k + 1 ) / static_cast<float>( n );
            m.tri( Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ std::cos( a1 ) * 6000.0f, 0.0f, std::sin( a1 ) * 6000.0f },
                   Vector3{ std::cos( a0 ) * 6000.0f, 0.0f, std::sin( a0 ) * 6000.0f }, from.m_surface.m_water, Mat{ from.m_surface.m_waterGlow, 0.85f } );
        }
        m_crossWater = m.upload();
        // The coasts: a line of green hills behind, another ahead.
        for( int side = 0; side < 2; ++side )
        {
            MeshBuilder     coast;
            const BiomeDef &land = side == 0 ? from : to;
            for( int k = -14; k <= 14; ++k )
            {
                const float h = 30.0f + 35.0f * ( 1.0f + std::sin( static_cast<float>( k ) * 2.39f + static_cast<float>( side ) * 4.1f ) );
                coast.blob( Vector3{ static_cast<float>( k ) * 190.0f, 0.0f, 0.0f }, Vector3{ 170.0f, h, 120.0f }, land.m_surface.m_groundFar,
                            static_cast<unsigned>( 300 + k * 7 + side * 101 ), 0.25f );
            }
            coast.box( Vector3{ 0.0f, 1.0f, -110.0f * ( side == 0 ? -1.0f : 1.0f ) }, Vector3{ 2800.0f, 1.5f, 20.0f }, Color{ 226, 214, 176, 255 } );
            ( side == 0 ? m_crossCoastA : m_crossCoastB ) = coast.upload();
        }
        // A gull: two wings and a body.
        MeshBuilder gull;
        gull.tri( Vector3{ 0.0f, 0.0f, 0.6f }, Vector3{ 2.0f, 0.5f, -0.2f }, Vector3{ 0.0f, 0.0f, -0.5f }, Color{ 236, 238, 240, 255 } );
        gull.tri( Vector3{ 0.0f, 0.0f, 0.6f }, Vector3{ 0.0f, 0.0f, -0.5f }, Vector3{ -2.0f, 0.5f, -0.2f }, Color{ 236, 238, 240, 255 } );
        gull.box( Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.16f, 0.16f, 0.7f }, Color{ 200, 202, 206, 255 } );
        m_crossGull = gull.upload();
    }
    else
    {
        // The tunnel: a vault of concrete, a walkway, lamps every 25 m on both walls.
        const int   n = 10;
        const float r = 9.5f;
        for( int z = -800; z < 800; z += 20 )
        {
            const float z0 = static_cast<float>( z ), z1 = z0 + 20.0f;
            for( int k = 0; k < n; ++k )
            {
                const float t0 = Tau * 0.5f * static_cast<float>( k ) / static_cast<float>( n ), t1 = Tau * 0.5f * static_cast<float>( k + 1 ) / static_cast<float>( n );
                const Color wall = k == 0 || k == n - 1 ? Color{ 120, 120, 116, 255 } : Color{ 150, 150, 144, 255 };
                m.quad( Vector3{ -std::cos( t0 ) * r, std::sin( t0 ) * r - 1.4f, z0 }, Vector3{ -std::cos( t0 ) * r, std::sin( t0 ) * r - 1.4f, z1 },
                        Vector3{ -std::cos( t1 ) * r, std::sin( t1 ) * r - 1.4f, z1 }, Vector3{ -std::cos( t1 ) * r, std::sin( t1 ) * r - 1.4f, z0 }, wall );
            }
            m.quad( Vector3{ -r, -1.4f, z0 }, Vector3{ r, -1.4f, z0 }, Vector3{ r, -1.4f, z1 }, Vector3{ -r, -1.4f, z1 }, Color{ 90, 86, 80, 255 } );
            for( float side : { -1.0f, 1.0f } )
            {
                if( z % 40 == 0 ) m.box( Vector3{ side * 7.3f, 4.2f, z0 }, Vector3{ 0.25f, 0.2f, 1.4f }, Color{ 255, 236, 190, 255 }, Mat{ 1.0f, 0.0f } );
                m.box( Vector3{ side * ( r - 1.6f ), -0.8f, z0 + 10.0f }, Vector3{ 0.6f, 0.6f, 10.0f }, Color{ 128, 128, 122, 255 } );
            }
        }
        m_crossWater = m.upload();
    }
}

void Game::unloadCrossingScene()
{
    for( Mesh *mesh : { &m_crossVessel, &m_crossWater, &m_crossCoastA, &m_crossCoastB, &m_crossGull } )
    {
        if( mesh->vaoId ) UnloadMesh( *mesh );
        *mesh = Mesh{};
    }
}

void Game::crossingScene( SceneInput &in, CameraView &cam )
{
    const float t = m_stateTime, k = clampf( t / CrossingTime, 0.0f, 1.0f );
    const bool  ferry = m_crossing.m_type == CrossingE::FERRY;
    in.m_world = nullptr;
    in.m_player = nullptr;
    in.m_traffic = nullptr;
    in.m_particles = nullptr;
    in.m_cars.clear();
    in.m_police.clear();
    in.m_speedBlur = 0.0f;
    in.m_aberration = 0.0f;
    in.m_windStreaks = 0.0f;
    in.m_lensRain = 0.0f;
    in.m_showcase = true;
    in.m_pieces.clear();
    Vector3 target{}, pos{};
    if( ferry )
    {
        // The ship under way between the coasts, gulls over its stern; the camera swings round from behind.
        in.m_env = blendEnvironment( m_crossEnvA, m_crossEnvB, smoothstepf( 0.35f, 0.95f, k ) );
        in.m_env.m_underwater = 0.0f;
        const float z = -200.0f + 30.0f * t;
        in.m_pieces.push_back( { &m_crossWater, Vec3d{ 0.0, -3.0, 0.0 }, 0.0f } );
        in.m_pieces.push_back( { &m_crossCoastA, Vec3d{ 0.0, -3.0, -1300.0 }, 0.0f } );
        in.m_pieces.push_back( { &m_crossCoastB, Vec3d{ 0.0, -3.0, 2400.0 - 90.0 * t }, 0.0f } );
        for( int g = 0; g < 5; ++g )
        {
            const float a = t * ( 0.7f + 0.12f * static_cast<float>( g ) ) + static_cast<float>( g ) * 1.3f, r = 10.0f + 4.0f * static_cast<float>( g );
            in.m_pieces.push_back( { &m_crossGull, Vec3d{ std::cos( a ) * r, 24.0 + 2.0 * std::sin( a * 1.7f ), z + 40.0f + std::sin( a ) * r }, -a } );
        }
        in.m_pieces.push_back( { &m_crossVessel, Vec3d{ 0.0, 0.0, z }, 0.0f } );
        target = Vector3{ 0.0f, 9.0f, z + 80.0f };
        const float yaw = lerpf( 2.9f, 1.25f, smoothstepf( 0.0f, 1.0f, k ) ), pitch = lerpf( 0.3f, 0.14f, k ), dist = 120.0f;
        pos = Vector3{ target.x + std::sin( yaw ) * std::cos( pitch ) * dist, target.y + std::sin( pitch ) * dist, target.z + std::cos( yaw ) * std::cos( pitch ) * dist };
    }
    else
    {
        // The train in its tunnel, the lamps rushing past.
        Environment e = m_crossEnvA;
        e.m_skyZenith = e.m_skyHorizon = e.m_fogColor = Color{ 10, 12, 16, 255 };
        e.m_fogDensity = 0.009f;
        e.m_sunIntensity = 0.18f;
        e.m_sunElevation = 70.0f;
        e.m_cloudAmount = e.m_stars = e.m_rain = e.m_snow = e.m_dust = e.m_aurora = e.m_rainbow = e.m_lightning = 0.0f;
        e.m_mountainHeight = 0.0f;
        e.m_planetSize = 0.0f;
        e.m_underwater = 0.0f;
        e.m_bloom = std::max( e.m_bloom, 0.8f );
        in.m_env = e;
        const float z = -400.0f + 48.0f * t;
        in.m_pieces.push_back( { &m_crossWater, Vec3d{}, 0.0f } );
        in.m_pieces.push_back( { &m_crossVessel, Vec3d{ 0.0, 0.0, z }, 0.0f } );
        target = Vector3{ 0.0f, 4.0f, z + 220.0f };
        pos = Vector3{ -0.8f, 7.0f, z + 30.0f };
    }
    in.m_focus = Vec3d{ target.x, target.y, target.z };
    cam.m_pos = Vec3d{ pos.x, pos.y, pos.z };
    cam.m_forward = Vector3Normalize( Vector3Subtract( target, pos ) );
    cam.m_up = Vector3{ 0.0f, 1.0f, 0.0f };
    cam.m_fovY = 50.0f;
}

void Game::drawCrossing()
{
    const float       W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() ), u = unit();
    const bool        ferry = m_crossing.m_type == CrossingE::FERRY;
    const BiomeDef   &dest = biomeDef( m_track->stageBiome( static_cast<int>( m_crossing.m_land / StageLength ) ) );
    const BiomeDef   &sea = biomeDef( m_crossing.m_sea );
    const std::string to = dest.m_name ? dest.m_name : "", over = sea.m_name ? sea.m_name : "";
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( 130.0f * u ), Color{ 0, 0, 0, 90 } );
    drawTextFx( std::string( ferry ? "FERRY TO " : "CAR TRAIN TO " ) + to, W * 0.5f, 34.0f * u, 30.0f * u, Ink, AlignE::CENTER, 0.6f );
    drawText( std::string( ferry ? "ACROSS THE " : "UNDER THE " ) + over, W * 0.5f, 78.0f * u, 14.0f * u, Accent, AlignE::CENTER );
    drawText( "THE CLOCK STANDS STILL", W * 0.5f, 102.0f * u, 11.0f * u, Dim, AlignE::CENTER );
    const float k = clampf( m_stateTime / CrossingTime, 0.0f, 1.0f ), bw = W * 0.4f, bx = W * 0.3f, by = H - 70.0f * u;
    DrawRectangle( static_cast<int>( bx ), static_cast<int>( by ), static_cast<int>( bw ), static_cast<int>( 4.0f * u ), Color{ 255, 255, 255, 50 } );
    DrawRectangle( static_cast<int>( bx ), static_cast<int>( by ), static_cast<int>( bw * k ), static_cast<int>( 4.0f * u ), Accent );
    drawHint( m_input.m_gamepad ? "[A] SKIP" : "[ENTER] SKIP", W * 0.5f, H - 30.0f * u, 11.0f * u, Color{ 255, 255, 255, 190 }, AlignE::CENTER );
}

std::vector<RegionRow> Game::regionRows() const
{
    // The countries driven state by state, in the order the tour meets them: their states, their places, those seen.
    std::vector<RegionRow> rows;
    for( BiomeE b : worldRoute() )
    {
        const BiomeDef &d = biomeDef( b );
        if( !d.m_country || !d.m_region ) continue;
        auto row = std::find_if( rows.begin(), rows.end(), [&]( const RegionRow &r ) { return std::strcmp( r.m_country, d.m_country ) == 0; } );
        if( row == rows.end() )
        {
            rows.push_back( RegionRow{ d.m_country, {}, {}, {} } );
            row = rows.end() - 1;
        }
        auto   reg = std::find_if( row->m_regions.begin(), row->m_regions.end(), [&]( const char *r ) { return std::strcmp( r, d.m_region ) == 0; } );
        size_t k = static_cast<size_t>( reg - row->m_regions.begin() );
        if( reg == row->m_regions.end() )
        {
            row->m_regions.push_back( d.m_region );
            row->m_places.push_back( 0 );
            row->m_seen.push_back( 0 );
            k = row->m_regions.size() - 1;
        }
        ++row->m_places[k];
        if( m_records.m_seen.test( static_cast<size_t>( b ) ) ) ++row->m_seen[k];
    }
    rows.erase( std::remove_if( rows.begin(), rows.end(), []( const RegionRow &r ) { return r.m_regions.size() < 2; } ), rows.end() );
    return rows;
}

void Game::drawCountry( const RegionRow &row, float x, float y, float w, float h )
{
    const float u = unit();
    const int   regions = static_cast<int>( row.m_regions.size() );
    int         seen = 0, places = 0, placesSeen = 0;
    for( int i = 0; i < regions; ++i )
    {
        seen += row.m_seen[static_cast<size_t>( i )] > 0 ? 1 : 0;
        places += row.m_places[static_cast<size_t>( i )];
        placesSeen += row.m_seen[static_cast<size_t>( i )];
    }
    drawTextFx( row.m_country, x + 8.0f * u, y + 2.0f * u, 24.0f * u, seen == regions ? Gold : Ink, AlignE::LEFT, 0.4f );
    drawText( format( "%d OF %d REGIONS   %d OF %d PLACES", seen, regions, placesSeen, places ), x + w - 8.0f * u, y + 10.0f * u, 12.0f * u, Dim,
              AlignE::RIGHT );
    // Its regions in three columns: a tick where you have been, the places seen of each.
    const float top = y + 44.0f * u, rowH = 28.0f * u, colW = w / 3.0f;
    const int   perColumn = std::max( 1, static_cast<int>( ( h - 44.0f * u ) / rowH ) ), perSheet = perColumn * 3;
    const int   sheets = std::max( 1, ( regions + perSheet - 1 ) / perSheet );
    m_passportSheet = std::clamp( m_passportSheet, 0, sheets - 1 );
    for( int i = 0; i < perSheet; ++i )
    {
        const int idx = m_passportSheet * perSheet + i;
        if( idx >= regions ) break;
        const bool  been = row.m_seen[static_cast<size_t>( idx )] > 0;
        const bool  all = row.m_seen[static_cast<size_t>( idx )] == row.m_places[static_cast<size_t>( idx )];
        const float rx = x + static_cast<float>( i / perColumn ) * colW + 8.0f * u, ry = top + static_cast<float>( i % perColumn ) * rowH;
        const float box = 14.0f * u;
        if( been )
        {
            DrawRectangleRounded( Rectangle{ rx, ry + 2.0f * u, box, box }, 0.25f, 4, all ? Gold : Accent );
            DrawLineEx( Vector2{ rx + box * 0.2f, ry + 2.0f * u + box * 0.55f }, Vector2{ rx + box * 0.42f, ry + 2.0f * u + box * 0.78f }, 2.2f * u, Color{ 20, 24, 36, 255 } );
            DrawLineEx( Vector2{ rx + box * 0.42f, ry + 2.0f * u + box * 0.78f }, Vector2{ rx + box * 0.82f, ry + 2.0f * u + box * 0.22f }, 2.2f * u, Color{ 20, 24, 36, 255 } );
        }
        else
        {
            DrawRectangleRoundedLinesEx( Rectangle{ rx, ry + 2.0f * u, box, box }, 0.25f, 4, 1.5f * u, Dim );
        }
        drawText( row.m_regions[static_cast<size_t>( idx )], rx + box + 8.0f * u, ry + 3.0f * u, 12.0f * u, been ? Ink : Dim );
        drawText( format( "%d/%d", row.m_seen[static_cast<size_t>( idx )], row.m_places[static_cast<size_t>( idx )] ), rx + colW - 18.0f * u, ry + 4.0f * u,
                  10.0f * u, all ? Gold : Dim, AlignE::RIGHT );
    }
    drawText( format( "SHEET %d / %d", m_passportSheet + 1, sheets ), x + w, y + h + 8.0f * u, 11.0f * u, Dim, AlignE::RIGHT );
}

void Game::drawRegions( float x, float y, float w, float h )
{
    const float                  u = unit();
    const std::vector<RegionRow> rows = regionRows();
    if( m_passportCountry >= 0 && m_passportCountry < static_cast<int>( rows.size() ) )
    {
        drawCountry( rows[static_cast<size_t>( m_passportCountry )], x, y, w, h );
        return;
    }
    m_passportCountry = -1;
    // Two columns of rows, as many sheets as they need; the sheet follows the country picked.
    const float rowH = 30.0f * u, colW = w * 0.5f;
    const int   perColumn = std::max( 1, static_cast<int>( h / rowH ) - 1 ), perSheet = perColumn * 2;
    const int   sheets = std::max( 1, ( static_cast<int>( rows.size() ) + perSheet - 1 ) / perSheet );
    m_passportPick = std::clamp( m_passportPick, 0, std::max( 0, static_cast<int>( rows.size() ) - 1 ) );
    m_passportSheet = std::clamp( m_passportPick / perSheet, 0, sheets - 1 );
    auto regionsSeen = []( const RegionRow &r ) { return static_cast<int>( std::count_if( r.m_seen.begin(), r.m_seen.end(), []( int n ) { return n > 0; } ) ); };
    int  complete = 0;
    for( const RegionRow &r : rows ) complete += regionsSeen( r ) == static_cast<int>( r.m_regions.size() ) ? 1 : 0;
    for( int i = 0; i < perSheet; ++i )
    {
        const size_t idx = static_cast<size_t>( m_passportSheet * perSheet + i );
        if( idx >= rows.size() ) break;
        const RegionRow &r = rows[idx];
        const int        seen = regionsSeen( r ), total = static_cast<int>( r.m_regions.size() );
        const float      rx = x + static_cast<float>( i / perColumn ) * colW + 8.0f * u, ry = y + static_cast<float>( i % perColumn ) * rowH;
        const bool       all = seen == total;
        if( static_cast<int>( idx ) == m_passportPick )
            DrawRectangleRounded( Rectangle{ rx - 6.0f * u, ry, colW - 12.0f * u, rowH - 4.0f * u }, 0.3f, 6, Color{ 255, 150, 40, 70 } );
        drawText( r.m_country, rx, ry + 6.0f * u, 13.0f * u, all ? Gold : Ink );
        const float bx = rx + colW * 0.42f, bw = colW * 0.36f;
        DrawRectangle( static_cast<int>( bx ), static_cast<int>( ry + 9.0f * u ), static_cast<int>( bw ), static_cast<int>( 8.0f * u ), Color{ 255, 255, 255, 30 } );
        DrawRectangle( static_cast<int>( bx ), static_cast<int>( ry + 9.0f * u ), static_cast<int>( bw * static_cast<float>( seen ) / static_cast<float>( total ) ),
                       static_cast<int>( 8.0f * u ), all ? Gold : Accent );
        drawText( format( "%d / %d", seen, total ), rx + colW - 24.0f * u, ry + 6.0f * u, 12.0f * u, all ? Gold : Dim, AlignE::RIGHT );
    }
    drawText( format( "%d OF %d COUNTRIES COMPLETE   JOURNAL %d PLACES   SHEET %d / %d", complete, static_cast<int>( rows.size() ),
                      static_cast<int>( m_records.m_journey.size() ), m_passportSheet + 1, sheets ),
              x + w, y + h + 8.0f * u, 11.0f * u, Dim, AlignE::RIGHT );
}

void Game::drawPassport()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 150 } );
    const float w = std::min( W - 40.0f * u, 1080.0f * u ), h = H - 40.0f * u;
    const float x = W * 0.5f - w * 0.5f, y = 20.0f * u;
    panel( x, y, w, h );
    drawTextFx( "PASSPORT", x + 28.0f * u, y + 18.0f * u, 28.0f * u, Accent, AlignE::LEFT, 0.8f );
    const char *tabs[5] = { "MAP", "STAMPS", "REGIONS", "POSTCARDS", "ALBUM" };
    float       tx = x + w - 28.0f * u;
    for( int i = 4; i >= 0; --i )
    {
        const float tw = textWidth( tabs[i], 15.0f * u );
        tx -= tw;
        if( i == m_passportPage )
            DrawRectangleRounded( Rectangle{ tx - 10.0f * u, y + 18.0f * u, tw + 20.0f * u, 30.0f * u }, 0.3f, 6, Color{ 255, 150, 40, 70 } );
        drawText( tabs[i], tx, y + 25.0f * u, 15.0f * u, i == m_passportPage ? Ink : Dim );
        tx -= 34.0f * u;
    }
    drawText( format( "PLACES %d / %d     COUNTRIES %d / %d     ROUND THE WORLD %d / %d     %s", m_records.seenCount(), biomeCount(),
                      m_records.countriesSeen(), countryCount(), m_records.sectorsSeen(), LongitudeSectors, Records::pointsText( m_records.m_points ).c_str() ),
              x + 28.0f * u, y + 62.0f * u, 12.0f * u, Dim );
    const float cx = x + 24.0f * u, cy = y + 90.0f * u, cw = w - 48.0f * u, ch = h - 150.0f * u;
    switch( static_cast<PassportE>( m_passportPage ) )
    {
    case PassportE::MAP: drawWorldMap( cx, cy, cw, ch - 20.0f * u ); break;
    case PassportE::STAMPS: drawStamps( cx, cy, cw, ch ); break;
    case PassportE::REGIONS: drawRegions( cx, cy, cw, ch ); break;
    case PassportE::ALBUM: drawAlbum( cx, cy, cw, ch ); break;
    default: drawPostcards( cx, cy, cw, ch ); break;
    }
    const bool  regions = m_passportPage == static_cast<int>( PassportE::REGIONS );
    const char *hint = m_input.m_gamepad ? "[DPAD] PAGE AND SHEET     [B] BACK" : "[ARROWS] PAGE AND SHEET     [ESC] BACK";
    if( regions && m_passportCountry >= 0 )
        hint = m_input.m_gamepad ? "[DPAD] SHEET     [A] [B] ALL COUNTRIES" : "[UP] [DOWN] SHEET     [ENTER] [ESC] ALL COUNTRIES";
    else if( regions )
        hint = m_input.m_gamepad ? "[DPAD] COUNTRY AND PAGE     [A] ITS REGIONS     [B] BACK"
                                 : "[UP] [DOWN] COUNTRY     [ENTER] ITS REGIONS     [LEFT] [RIGHT] PAGE     [ESC] BACK";
    drawHint( hint, W * 0.5f, y + h - 28.0f * u, 11.0f * u, Dim, AlignE::CENTER );
}

void Game::drawRallies()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H ), Color{ 0, 0, 0, 110 } );
    const auto              &list = rallies();
    std::vector<std::string> names, values;
    for( size_t i = 0; i < list.size(); ++i )
    {
        names.push_back( list[i].m_name );
        const double best = i < m_records.m_rallyBest.size() ? m_records.m_rallyBest[i] : 0.0;
        values.push_back( best > 0.0 ? "BEST " + clockText( static_cast<float>( best ) )
                                     : format( "%d PLACES  %d KM", static_cast<int>( list[i].m_stages.size() ),
                                               static_cast<int>( list[i].m_stages.size() * static_cast<size_t>( StageLength / 1000.0 ) ) ) );
    }
    // The journeys: from a place to another, the forks choosing the way.
    const auto &trips = journeys();
    for( size_t i = 0; i < trips.size(); ++i )
    {
        names.push_back( trips[i].m_name );
        const double best = i < m_records.m_journeyBest.size() ? m_records.m_journeyBest[i] : 0.0;
        const int    stages = routeStages( trips[i].m_from, trips[i].m_to );
        values.push_back( best > 0.0 ? "BEST " + clockText( static_cast<float>( best ) ) : format( "JOURNEY  %d STAGES", stages ) );
    }
    names.push_back( "BACK" );
    values.push_back( "" );
    drawMenu( "RALLIES AND JOURNEYS", names, values, H * 0.02f );
    const int trip = m_menuIndex - static_cast<int>( list.size() );
    if( trip >= 0 && trip < static_cast<int>( trips.size() ) )
    {
        const Journey &j = trips[static_cast<size_t>( trip )];
        drawText( j.m_sub, W * 0.5f, H - 64.0f * u, 13.0f * u, Accent, AlignE::CENTER );
        drawText( format( "FROM %s TO %s, %d STAGES ON THE MAIN WAY; THE FORKS MAY BE SHORTER OR LONGER", biomeDef( j.m_from ).m_name,
                          biomeDef( j.m_to ).m_name, routeStages( j.m_from, j.m_to ) ),
                  W * 0.5f, H - 42.0f * u, 10.0f * u, Dim, AlignE::CENTER );
    }
    if( m_menuIndex < static_cast<int>( list.size() ) )
    {
        const Rally &r = list[static_cast<size_t>( m_menuIndex )];
        std::string  route;
        for( size_t i = 0; i < r.m_stages.size(); ++i )
            route += ( i ? "  -  " : "" ) + std::string( biomeDef( r.m_stages[i] ).m_name );
        drawText( r.m_sub, W * 0.5f, H - 64.0f * u, 13.0f * u, Accent, AlignE::CENTER );
        // The places, in as many lines as they need.
        std::vector<std::string> lines( 1 );
        for( size_t i = 0; i < r.m_stages.size(); ++i )
        {
            const std::string part = ( lines.back().empty() ? "" : "  -  " ) + std::string( biomeDef( r.m_stages[i] ).m_name );
            if( textWidth( lines.back() + part, 11.0f * u ) > W - 80.0f * u ) lines.push_back( biomeDef( r.m_stages[i] ).m_name );
            else lines.back() += part;
        }
        for( size_t i = 0; i < lines.size(); ++i ) drawText( lines[i], W * 0.5f, H - 42.0f * u + 16.0f * u * static_cast<float>( i ), 10.0f * u, Dim, AlignE::CENTER );
    }
}

void Game::drawMinimap( float x, float y, float w, float h )
{
    const Track &track = *m_track;
    panel( x, y, w, h, 140 );
    const TrackFrame cf = track.frameAt( m_car.m_s );
    const Vector3    fwd = headingForward( cf.m_heading );
    const Vector3    right = headingRight( cf.m_heading );
    const Vec3d      c = cf.m_pos;
    const float      scale = h * 0.8f / 1300.0f;
    auto map = [&]( const Vec3d &p )
    {
        const float rx = static_cast<float>( p.m_x - c.m_x ), rz = static_cast<float>( p.m_z - c.m_z );
        const float f = rx * fwd.x + rz * fwd.z;
        const float r = rx * right.x + rz * right.z;
        return Vector2{ x + w * 0.5f + r * scale, y + h * 0.88f - f * scale };
    };
    BeginScissorMode( static_cast<int>( x ), static_cast<int>( y ), static_cast<int>( w ), static_cast<int>( h ) );
    Vector2     prev = map( track.pointAt( m_car.m_s - 80.0, 0.0f, 0.0f ) );
    const float u = h / 150.0f;
    for( double d = -55.0; d <= 1250.0; d += 25.0 )
    {
        const Vector2 p = map( track.pointAt( m_car.m_s + d, 0.0f, 0.0f ) );
        DrawLineEx( prev, p, 5.0f * u, Color{ 235, 238, 245, 200 } );
        prev = p;
    }
    const double finish = m_setup.m_params.m_length;
    if( !m_demo && finish > 0.0 && finish - m_car.m_s < 1250.0 )
    {
        const Vector2 p = map( track.pointAt( finish, 0.0f, 0.0f ) );
        DrawRectangle( static_cast<int>( p.x - 5.0f * u ), static_cast<int>( p.y - 5.0f * u ), static_cast<int>( 10.0f * u ),
                       static_cast<int>( 10.0f * u ), Ink );
    }
    else if( m_nextCheckpoint - m_car.m_s < 1250.0 && !m_demo && !m_setup.m_race )
    {
        DrawCircleV( map( track.pointAt( m_nextCheckpoint, 0.0f, 0.0f ) ), 5.0f * u, Green );
    }
    for( const TrafficCar &t : m_traffic.m_cars )
    {
        if( t.m_s - m_car.m_s > 1250.0 || t.m_swim ) continue;
        DrawCircleV( map( track.pointAt( t.m_s, t.m_x, 0.0f ) ), 2.6f * u, Color{ 255, 200, 80, 255 } );
    }
    for( const Rival &r : m_race.m_rivals )
    {
        if( r.m_s - m_car.m_s > 1250.0 || r.m_s - m_car.m_s < -80.0 ) continue;
        DrawCircleV( map( track.pointAt( r.m_s, r.m_x, 0.0f ) ), 3.4f * u, Red );
    }
    double gs = 0.0;
    float  gx = 0.0f, gyaw = 0.0f;
    if( !m_demo && m_ghost.at( m_raceTime, gs, gx, gyaw ) && gs - m_car.m_s < 1250.0 && gs - m_car.m_s > -80.0 )
        DrawCircleV( map( track.pointAt( gs, gx, 0.0f ) ), 3.4f * u, Cyan );
    const Vector2 me = map( track.pointAt( m_car.m_s, m_car.m_x, 0.0f ) );
    DrawTriangle( Vector2{ me.x, me.y - 7.0f * u }, Vector2{ me.x - 5.0f * u, me.y + 5.0f * u }, Vector2{ me.x + 5.0f * u, me.y + 5.0f * u }, Accent );
    EndScissorMode();
}

namespace
{

/** @brief What hangs from the mirror or stands on the dashboard. */
enum class OrnamentE
{
    DICE,       ///< fuzzy dice (North America)
    TREE,       ///< a tree-shaped air freshener (Britain, Europe)
    DOG,        ///< a nodding dog on the dashboard (Germany, Austria, Switzerland)
    CAT,        ///< a beckoning cat on the dashboard (Japan)
    MARIGOLD,   ///< a garland of marigolds (South Asia)
    JASMINE,    ///< a jasmine garland with a red tassel (Southeast Asia)
    KNOT,       ///< a red knot with a tassel (East Asia)
    POMPOMS,    ///< a fringe of pompoms along the top of the windscreen (Latin America)
    BEADS,      ///< a string of coloured beads (Africa)
    NAZAR,      ///< a blue eye charm (Turkey, Arabia)
    DASHCAM,    ///< a dashcam on the windscreen (the east)
    KOALA,      ///< a toy koala clinging to the mirror (Oceania)
    HIBISCUS,   ///< a hibiscus flower on the dashboard (the islands)
};

/**
 * @brief The ornament of a country's cars.
 * @param[in] country  its name (nullptr: none)
 * @return the ornament
 */
OrnamentE ornamentOf( const char *country )
{
    if( !country ) return OrnamentE::DICE;
    const std::string_view c = country;
    if( c == "JAPAN" ) return OrnamentE::CAT;
    if( c == "GERMANY" || c == "AUSTRIA" || c == "SWITZERLAND" || c == "LIECHTENSTEIN" ) return OrnamentE::DOG;
    if( c == "TURKEY" || c == "GREECE" || c == "CYPRUS" ) return OrnamentE::NAZAR;
    switch( countryInfo( country ).m_fleet )
    {
    case FleetE::NORTH_AMERICA: return OrnamentE::DICE;
    case FleetE::LATIN: return OrnamentE::POMPOMS;
    case FleetE::SOUTH_ASIA: return OrnamentE::MARIGOLD;
    case FleetE::SOUTHEAST_ASIA: return OrnamentE::JASMINE;
    case FleetE::EAST_ASIA: return OrnamentE::KNOT;
    case FleetE::OCEANIA: return OrnamentE::KOALA;
    case FleetE::AFRICA: return OrnamentE::BEADS;
    case FleetE::ARABIA: return OrnamentE::NAZAR;
    case FleetE::EASTERN: return OrnamentE::DASHCAM;
    case FleetE::ISLANDS: return OrnamentE::HIBISCUS;
    default: return OrnamentE::TREE;
    }
}

/**
 * @brief A filled quadrilateral (either winding).
 * @param[in] a,b,c,d  its corners in order round it
 * @param[in] col      its colour
 */
void fillQuad( Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color col )
{
    DrawTriangle( a, b, c, col );
    DrawTriangle( a, c, b, col );
    DrawTriangle( a, c, d, col );
    DrawTriangle( a, d, c, col );
}

} // namespace

void Game::drawCockpit()
{
    const float   W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float   side = m_seat;                         // 1: the driver on the left, the car's middle to the right
    const float   mid = W * 0.5f;                        // the wheel straight ahead of the driver
    const Color   trim{ 26, 26, 32, 255 }, trimLight{ 58, 58, 68, 255 }, leather{ 38, 34, 36, 255 };
    const BiomeDef &here = biomeDef( m_track->stageBiome( static_cast<int>( m_car.m_s / StageLength ) ) );
    const float   sn = m_car.m_speed / m_car.m_spec.m_topSpeed;
    auto          sx = [&]( float x ) { return mid + ( x - 0.5f ) * W * side; };   // a screen x mirrored for the side

    // The roof's edge and the pillars: the near one wide at the driver's side, the far one slim and further in.
    DrawRectangle( 0, 0, static_cast<int>( W ), static_cast<int>( H * 0.045f ), trim );
    fillQuad( { sx( -0.02f ), 0.0f }, { sx( 0.075f ), 0.0f }, { sx( 0.0f ), H * 0.86f }, { sx( -0.02f ), H * 0.86f }, trim );
    fillQuad( { sx( 0.92f ), 0.0f }, { sx( 1.02f ), 0.0f }, { sx( 1.02f ), H * 0.86f }, { sx( 0.975f ), H * 0.86f }, trim );
    DrawLineEx( { sx( 0.075f ), 0.0f }, { sx( 0.0f ), H * 0.86f }, 2.0f, trimLight );
    DrawLineEx( { sx( 0.92f ), 0.0f }, { sx( 0.975f ), H * 0.86f }, 2.0f, trimLight );

    // The mirror over the car's middle, a strip of sky and road in it.
    const float     mx = sx( 0.68f ), my = H * 0.075f, mw = W * 0.14f, mh = H * 0.055f;
    const Environment &env = m_lastEnv;
    DrawLineEx( { mx, H * 0.045f }, { mx, my - mh * 0.5f }, 5.0f, trim );
    DrawRectangleRounded( { mx - mw * 0.5f - 4.0f, my - mh * 0.5f - 4.0f, mw + 8.0f, mh + 8.0f }, 0.4f, 6, trim );
    DrawRectangleGradientV( static_cast<int>( mx - mw * 0.5f ), static_cast<int>( my - mh * 0.5f ), static_cast<int>( mw ), static_cast<int>( mh ),
                            env.m_skyHorizon, Color{ 70, 70, 78, 255 } );

    // The dashboard, the hood over the dials, the glovebox on the far side.
    const float dashY = H * 0.86f;
    DrawEllipse( static_cast<int>( mid ), static_cast<int>( dashY ), W * 0.13f, H * 0.1f, trim );
    DrawRectangle( 0, static_cast<int>( dashY ), static_cast<int>( W ), static_cast<int>( H - dashY ), trim );
    DrawLineEx( { 0.0f, dashY }, { W, dashY }, 3.0f, trimLight );
    DrawLineEx( { sx( 0.66f ), dashY + H * 0.05f }, { sx( 0.9f ), dashY + H * 0.05f }, 2.0f, trimLight );
    // The dials: revs and speed, their needles alive.
    for( int k = 0; k < 2; ++k )
    {
        const Vector2 c = { mid + ( k == 0 ? -1.0f : 1.0f ) * W * 0.055f, H * 0.865f };
        const float   r = H * 0.05f;
        DrawCircleV( c, r + 3.0f, trimLight );
        DrawCircleV( c, r, Color{ 12, 12, 16, 255 } );
        DrawRing( c, r * 0.82f, r * 0.9f, 135.0f, 405.0f, 24, Color{ 200, 200, 210, 255 } );
        if( k == 0 ) DrawRing( c, r * 0.82f, r * 0.9f, 340.0f, 405.0f, 8, Red );
        const float v = k == 0 ? clampf( m_car.m_rpm / 1.15f, 0.0f, 1.0f ) : clampf( sn, 0.0f, 1.0f );
        const float a = ( 135.0f + 270.0f * v ) * DEG2RAD;
        DrawLineEx( c, { c.x + std::cos( a ) * r * 0.8f, c.y + std::sin( a ) * r * 0.8f }, 3.0f, Accent );
        DrawCircleV( c, r * 0.12f, trimLight );
    }

    // The wheel, turning as the car steers.
    const Vector2 hub = { mid, H * 1.1f };
    const float   ro = H * 0.38f, ri = H * 0.335f;
    const float   turn = m_car.m_steer * 100.0f;
    DrawRing( hub, ri, ro, 180.0f + turn - 30.0f, 360.0f + turn + 30.0f, 48, leather );
    DrawRing( hub, ri + ( ro - ri ) * 0.35f, ri + ( ro - ri ) * 0.5f, 180.0f + turn - 30.0f, 360.0f + turn + 30.0f, 48, Color{ 52, 48, 50, 255 } );
    DrawRing( hub, ri, ro, 262.0f + turn, 278.0f + turn, 6, Accent );   // the stripe at the top
    for( float spoke : { 180.0f, 360.0f } )
    {
        const float a = ( spoke + turn ) * DEG2RAD;
        DrawLineEx( hub, { hub.x + std::cos( a ) * ri, hub.y + std::sin( a ) * ri }, H * 0.035f, Color{ 44, 42, 46, 255 } );
    }
    DrawCircleV( hub, H * 0.11f, Color{ 44, 42, 46, 255 } );

    // The country's ornament.
    const Vector2 anchor = { mx, my + mh * 0.5f + 4.0f };
    const float   len = H * 0.1f;
    const Vector2 end = { anchor.x + std::sin( m_swing ) * len, anchor.y + std::cos( m_swing ) * len };
    auto          string = [&]( Vector2 to ) { DrawLineEx( anchor, to, 2.0f, Color{ 40, 36, 30, 255 } ); };
    const float   t = m_time;
    switch( ornamentOf( here.m_country ) )
    {
    case OrnamentE::DICE:
        for( int k = 0; k < 2; ++k )
        {
            const Vector2 d = { end.x + ( k ? 1.0f : -1.0f ) * H * 0.018f, end.y + ( k ? H * 0.012f : 0.0f ) };
            string( d );
            DrawRectanglePro( { d.x, d.y, H * 0.03f, H * 0.03f }, { H * 0.015f, H * 0.015f }, m_swing * 40.0f + ( k ? 20.0f : -10.0f ), Color{ 236, 236, 230, 255 } );
            DrawCircleV( d, H * 0.004f, Color{ 30, 30, 30, 255 } );
        }
        break;
    case OrnamentE::TREE:
        string( end );
        DrawTriangle( { end.x, end.y }, { end.x - H * 0.025f, end.y + H * 0.05f }, { end.x + H * 0.025f, end.y + H * 0.05f }, Color{ 40, 150, 70, 255 } );
        DrawTriangle( { end.x, end.y }, { end.x + H * 0.025f, end.y + H * 0.05f }, { end.x - H * 0.025f, end.y + H * 0.05f }, Color{ 40, 150, 70, 255 } );
        DrawRectangle( static_cast<int>( end.x - H * 0.004f ), static_cast<int>( end.y + H * 0.05f ), static_cast<int>( H * 0.008f ), static_cast<int>( H * 0.012f ),
                       Color{ 40, 150, 70, 255 } );
        break;
    case OrnamentE::MARIGOLD:
    case OrnamentE::JASMINE:
    case OrnamentE::BEADS:
        // A garland: a loop of flowers or beads hanging from the mirror.
        for( int k = 0; k <= 12; ++k )
        {
            const float   f = static_cast<float>( k ) / 12.0f, s = std::sin( f * PI );
            const Vector2 p = { anchor.x + ( f - 0.5f ) * H * 0.07f + std::sin( m_swing ) * len * s, anchor.y + std::cos( m_swing ) * len * s };
            Color         col = k % 2 ? Color{ 250, 160, 20, 255 } : Color{ 250, 200, 40, 255 };
            if( ornamentOf( here.m_country ) == OrnamentE::JASMINE ) col = Color{ 245, 245, 235, 255 };
            if( ornamentOf( here.m_country ) == OrnamentE::BEADS )
            {
                static const Color beads[4] = { { 220, 40, 40, 255 }, { 250, 200, 30, 255 }, { 40, 140, 70, 255 }, { 40, 90, 200, 255 } };
                col = beads[k % 4];
            }
            DrawCircleV( p, H * ( ornamentOf( here.m_country ) == OrnamentE::BEADS ? 0.007f : 0.011f ), col );
        }
        if( ornamentOf( here.m_country ) == OrnamentE::JASMINE )
            DrawRectangleV( { end.x - H * 0.005f, end.y }, { H * 0.01f, H * 0.04f }, Color{ 210, 30, 40, 255 } );
        break;
    case OrnamentE::KNOT:
        string( end );
        DrawPoly( end, 4, H * 0.022f, 45.0f + m_swing * 30.0f, Color{ 210, 30, 30, 255 } );
        DrawRectangleV( { end.x - H * 0.006f, end.y + H * 0.02f }, { H * 0.012f, H * 0.045f }, Color{ 220, 40, 30, 255 } );
        break;
    case OrnamentE::NAZAR:
        string( end );
        DrawCircleV( end, H * 0.02f, Color{ 30, 70, 200, 255 } );
        DrawCircleV( end, H * 0.013f, Color{ 245, 245, 245, 255 } );
        DrawCircleV( end, H * 0.008f, Color{ 90, 170, 230, 255 } );
        DrawCircleV( end, H * 0.004f, Color{ 10, 10, 20, 255 } );
        break;
    case OrnamentE::KOALA:
    {
        const Vector2 k = { anchor.x + std::sin( m_swing ) * len * 0.4f, anchor.y + std::cos( m_swing ) * len * 0.4f };
        DrawCircleV( { k.x, k.y + H * 0.02f }, H * 0.022f, Color{ 150, 150, 156, 255 } );
        DrawCircleV( k, H * 0.017f, Color{ 160, 160, 166, 255 } );
        DrawCircleV( { k.x - H * 0.016f, k.y - H * 0.01f }, H * 0.009f, Color{ 190, 190, 196, 255 } );
        DrawCircleV( { k.x + H * 0.016f, k.y - H * 0.01f }, H * 0.009f, Color{ 190, 190, 196, 255 } );
        DrawCircleV( { k.x, k.y + H * 0.004f }, H * 0.005f, Color{ 30, 30, 34, 255 } );
        break;
    }
    case OrnamentE::POMPOMS:
        // Along the top of the windscreen, each swinging a little.
        for( int k = 0; k < 16; ++k )
        {
            const float   x = sx( 0.1f + 0.05f * static_cast<float>( k ) );
            const Vector2 p = { x + std::sin( m_swing + static_cast<float>( k ) ) * H * 0.006f, H * 0.06f };
            static const Color tufts[4] = { { 230, 40, 60, 255 }, { 250, 200, 30, 255 }, { 40, 160, 80, 255 }, { 240, 240, 240, 255 } };
            DrawLineEx( { x, H * 0.045f }, p, 2.0f, trim );
            DrawCircleV( p, H * 0.011f, tufts[k % 4] );
        }
        break;
    case OrnamentE::DOG:
    {
        // On the dashboard by the far pillar; its head nods with every bump.
        const Vector2 b = { sx( 0.64f ), dashY - H * 0.02f };
        const float   nod = std::sin( t * 9.0f ) * ( 0.15f + m_car.m_shake * 0.8f ) + m_swing * 0.4f;
        DrawEllipse( static_cast<int>( b.x ), static_cast<int>( b.y ), H * 0.045f, H * 0.022f, Color{ 120, 70, 30, 255 } );
        const Vector2 head = { b.x - side * H * 0.04f, b.y - H * 0.035f + nod * H * 0.015f };
        DrawCircleV( head, H * 0.022f, Color{ 130, 76, 34, 255 } );
        DrawEllipse( static_cast<int>( head.x - side * H * 0.016f ), static_cast<int>( head.y + H * 0.006f ), H * 0.012f, H * 0.008f, Color{ 150, 90, 40, 255 } );
        DrawEllipse( static_cast<int>( head.x + side * H * 0.012f ), static_cast<int>( head.y + H * 0.012f ), H * 0.006f, H * 0.016f, Color{ 90, 50, 20, 255 } );
        break;
    }
    case OrnamentE::CAT:
    {
        // A white beckoning cat, its paw going up and down.
        const Vector2 b = { sx( 0.64f ), dashY - H * 0.03f };
        DrawEllipse( static_cast<int>( b.x ), static_cast<int>( b.y ), H * 0.025f, H * 0.03f, Color{ 245, 245, 240, 255 } );
        DrawCircleV( { b.x, b.y - H * 0.04f }, H * 0.022f, Color{ 245, 245, 240, 255 } );
        DrawTriangle( { b.x - H * 0.02f, b.y - H * 0.055f }, { b.x - H * 0.012f, b.y - H * 0.07f }, { b.x - H * 0.005f, b.y - H * 0.058f }, Color{ 245, 245, 240, 255 } );
        DrawTriangle( { b.x + H * 0.005f, b.y - H * 0.058f }, { b.x + H * 0.012f, b.y - H * 0.07f }, { b.x + H * 0.02f, b.y - H * 0.055f }, Color{ 245, 245, 240, 255 } );
        DrawRectangleV( { b.x - H * 0.02f, b.y - H * 0.022f }, { H * 0.04f, H * 0.005f }, Color{ 210, 30, 40, 255 } );
        DrawCircleV( { b.x, b.y - H * 0.015f }, H * 0.005f, Color{ 240, 200, 40, 255 } );
        const float paw = 0.5f + 0.5f * std::sin( t * 4.0f );
        DrawCircleV( { b.x + H * 0.03f, b.y - H * 0.03f - paw * H * 0.02f }, H * 0.009f, Color{ 245, 245, 240, 255 } );
        break;
    }
    case OrnamentE::DASHCAM:
    {
        const Vector2 c = { mx - side * mw * 0.65f, my + H * 0.01f };
        DrawRectangleRounded( { c.x - H * 0.025f, c.y - H * 0.015f, H * 0.05f, H * 0.03f }, 0.3f, 4, Color{ 20, 20, 24, 255 } );
        DrawCircleV( { c.x, c.y }, H * 0.009f, Color{ 50, 60, 80, 255 } );
        if( static_cast<int>( t * 2.0f ) & 1 ) DrawCircleV( { c.x + H * 0.018f, c.y - H * 0.009f }, H * 0.003f, Red );
        break;
    }
    case OrnamentE::HIBISCUS:
    {
        const Vector2 b = { sx( 0.64f ), dashY - H * 0.012f };
        for( int k = 0; k < 5; ++k )
        {
            const float a = static_cast<float>( k ) * Tau / 5.0f + 0.3f;
            DrawCircleV( { b.x + std::cos( a ) * H * 0.016f, b.y + std::sin( a ) * H * 0.01f }, H * 0.013f, Color{ 230, 40, 70, 255 } );
        }
        DrawCircleV( b, H * 0.006f, Color{ 250, 220, 60, 255 } );
        break;
    }
    }
}

void Game::drawHud()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    if( m_cockpit && !m_demo && !m_lookingBack ) drawCockpit();
    const bool  mph = m_settings.m_units == UnitsE::MPH;
    auto        txt = []( std::string_view t, float x, float y, float s, Color c, AlignE a = AlignE::LEFT ) { drawTextFx( t, x, y, s, c, a, 0.0f ); };
    const bool  racing = m_setup.m_race && !m_demo;

    if( racing )
    {
        // Place, race clock, distance to go.
        const int place = m_finished ? m_place : m_race.position( m_car );
        txt( "POSITION", W * 0.5f, 12.0f * u, 13.0f * u, Dim, AlignE::CENTER );
        drawTextFx( format( "%d/%d", place, RivalCount + 1 ), W * 0.5f, 32.0f * u, 54.0f * u, place <= 3 ? Gold : Ink, AlignE::CENTER, 0.6f );
        txt( m_mode == ModeE::CHAMPIONSHIP ? format( "RACE %d/%d", m_race.m_round + 1, RoundCount ) : std::string( "RACE" ), 28.0f * u,
             14.0f * u, 13.0f * u, Dim );
        drawTextFx( clockText( m_raceTime ), 28.0f * u, 34.0f * u, 28.0f * u, Ink, AlignE::LEFT, 0.3f );
        txt( format( "%.1f KM TO GO", std::max( 0.0, m_race.m_finishS - m_car.m_s ) / 1000.0 ), 28.0f * u, 74.0f * u, 15.0f * u, Ink );
        txt( placeName( m_track->stageBiome( 0 ) ), 28.0f * u, 98.0f * u, 12.0f * u, Accent );
        if( m_mode == ModeE::CHAMPIONSHIP )
            txt( format( "POINTS %d", m_race.m_playerPoints ), W - 28.0f * u, 14.0f * u, 13.0f * u, Ink, AlignE::RIGHT );
    }
    else
    {
        // The clock.
        const bool  low = m_timeLeft < 10.0f && !m_timeUp && !m_finishedTrack;
        const float pulse = low ? 0.5f + 0.5f * std::sin( m_time * 12.0f ) : 0.0f;
        const Color tc = low ? lerpColor( Ink, Red, 0.5f + 0.5f * pulse ) : Ink;
        txt( m_mode == ModeE::TRAVEL ? "ON THE ROAD" : "TIME", W * 0.5f, 12.0f * u, 13.0f * u, Dim, AlignE::CENTER );
        if( m_mode == ModeE::RALLY || m_mode == ModeE::TRAVEL )
            drawTextFx( clockText( m_raceTime ), W * 0.5f, 32.0f * u, 46.0f * u, Ink, AlignE::CENTER, 0.6f );
        else
            drawTextFx( format( "%d", static_cast<int>( std::ceil( m_timeLeft ) ) ), W * 0.5f, 32.0f * u, ( 54.0f + 6.0f * pulse ) * u, tc,
                        AlignE::CENTER, 0.6f );

        // Score, distance, stage.
        // The rows spread with the text size.
        const float ts = textScale();
        auto        row = [&]( float y ) { return ( 14.0f + ( y - 14.0f ) * ts ) * u; };
        txt( "SCORE", 28.0f * u, row( 14.0f ), 13.0f * u, Dim );
        drawTextFx( format( "%lld", static_cast<long long>( m_score ) ), 28.0f * u, row( 34.0f ), 28.0f * u, Ink, AlignE::LEFT, 0.3f );
        txt( format( "%.2f KM", ( m_car.m_s - m_startS ) / 1000.0 ), 28.0f * u, row( 74.0f ), 15.0f * u, Ink );
        const BiomeE biome = m_track->stageBiome( static_cast<int>( m_car.m_s / StageLength ) );
        std::string stageLine = format( "STAGE %d  %s", static_cast<int>( m_car.m_s / StageLength ) + 1, placeName( biome ).c_str() );
        if( m_settings.m_timeOfDay == 1 && m_hour >= 0.0f )
            stageLine += format( "  %02d:%02d", static_cast<int>( m_hour ), static_cast<int>( std::fmod( m_hour, 1.0f ) * 60.0f ) );
        txt( stageLine, 28.0f * u, row( 98.0f ), 12.0f * u, Accent );

        // Records and combo.
        txt( format( "BEST %lld", static_cast<long long>( std::max<int64_t>( m_settings.m_bestScore, 0 ) ) ), W - 28.0f * u, 14.0f * u, 13.0f * u,
             Dim, AlignE::RIGHT );
        txt( format( "OVERTAKES %d", m_overtakes ), W - 28.0f * u, 38.0f * u, 13.0f * u, Ink, AlignE::RIGHT );
        if( m_combo > 1 ) drawTextFx( format( "COMBO X%d", m_combo ), W - 28.0f * u, 62.0f * u, 20.0f * u, Cyan, AlignE::RIGHT, 0.8f );
        if( m_police.m_state == PoliceStateE::CHASE )
        {
            // Close behind, its lights flash red and blue at the edges of the picture.
            const float close = 1.0f - smoothstepf( 20.0f, 160.0f, static_cast<float>( m_police.gap( m_car.m_s ) ) );
            if( close > 0.0f )
            {
                const bool          redLeft = ( static_cast<int>( m_time * 6.0f ) & 1 ) != 0;
                const unsigned char a = static_cast<unsigned char>( 110.0f * close );
                const Color         red = { 255, 40, 30, a }, blue = { 40, 90, 255, a }, clear = { 0, 0, 0, 0 };
                const float         w = W * 0.12f;
                DrawRectangleGradientH( 0, 0, static_cast<int>( w ), static_cast<int>( H ), redLeft ? red : blue, clear );
                DrawRectangleGradientH( static_cast<int>( W - w ), 0, static_cast<int>( w ), static_cast<int>( H ), clear, redLeft ? blue : red );
            }
            // The chase: how far the car is ahead, and how far it must get.
            const bool blink = ( static_cast<int>( m_time * 4.0f ) & 1 ) != 0;
            drawTextFx( format( "POLICE  %d / %d M", static_cast<int>( std::max( 0.0, m_police.gap( m_car.m_s ) ) ), static_cast<int>( PoliceEscape ) ), W * 0.5f,
                        H - 70.0f * u, 22.0f * u, blink ? Red : Color{ 90, 140, 255, 255 }, AlignE::CENTER, 0.6f );
            if( !m_lookingBack ) drawHint( m_input.m_gamepad ? "HOLD [R3] TO LOOK BACK" : "HOLD [B] TO LOOK BACK", W * 0.5f, H - 42.0f * u, 11.0f * u, Dim, AlignE::CENTER );
        }
    }

    // Speed, revs, gear, boost.
    const float speed = m_car.displaySpeed( mph );
    const float right = W - 34.0f * u;
    drawTextFx( format( "%d", static_cast<int>( speed ) ), right, H - 120.0f * u, 64.0f * u, Ink, AlignE::RIGHT, 0.4f );
    txt( mph ? "MPH" : "KM/H", right, H - 44.0f * u, 13.0f * u, Dim, AlignE::RIGHT );
    const int   segs = 22;
    const float barW = 300.0f * u;
    const float x0 = right - barW;
    for( int i = 0; i < segs; ++i )
    {
        const float t = static_cast<float>( i ) / static_cast<float>( segs - 1 );
        const float sw = barW / static_cast<float>( segs ) - 3.0f * u;
        const float sh = ( 8.0f + 14.0f * t ) * u;
        const bool  lit = t <= m_car.m_rpm;
        Color       col = t < 0.6f ? Color{ 90, 255, 150, 255 } : t < 0.85f ? Color{ 255, 220, 60, 255 } : Red;
        if( !lit ) col = Color{ 255, 255, 255, 35 };
        DrawRectangle( static_cast<int>( x0 + static_cast<float>( i ) * barW / static_cast<float>( segs ) ), static_cast<int>( H - 142.0f * u - sh ),
                       static_cast<int>( sw ), static_cast<int>( sh ), col );
    }
    const float gx = x0 - 6.0f * u;
    DrawRectangleRounded( Rectangle{ gx - 56.0f * u, H - 120.0f * u, 54.0f * u, 64.0f * u }, 0.2f, 6, Color{ 8, 10, 22, 170 } );
    drawTextFx( format( "%d", m_car.m_gear + 1 ), gx - 29.0f * u, H - 108.0f * u, 40.0f * u, m_settings.m_manualGears ? Accent : Ink,
                AlignE::CENTER, 0.3f );
    txt( "BOOST", x0, H - 30.0f * u, 11.0f * u, Cyan );
    const float bx = x0 + 70.0f * u, bw = right - bx;
    DrawRectangle( static_cast<int>( bx ), static_cast<int>( H - 30.0f * u ), static_cast<int>( bw ), static_cast<int>( 10.0f * u ),
                   Color{ 255, 255, 255, 35 } );
    const Color boostCol = m_car.m_boosting ? Color{ 160, 240, 255, 255 } : Cyan;
    DrawRectangle( static_cast<int>( bx ), static_cast<int>( H - 30.0f * u ), static_cast<int>( bw * m_car.m_boostTank ), static_cast<int>( 10.0f * u ),
                   boostCol );

    // A fork ahead: where each carriageway leads, the one the car is on lit.
    if( !m_demo )
    {
        const int  stage = static_cast<int>( m_car.m_s / StageLength );
        const Fork fork = m_track->fork( stage );
        if( fork.m_exists && fork.m_chosen == 0 && m_car.m_s > fork.m_at - 1900.0 )
        {
            // Two boards, the place each way leads to and its region and country; the side the car is on lit. Which
            // side counts is where the barrier begins.
            const bool   left = m_car.m_x < 0.0f;
            const double ahead = fork.m_at - m_car.m_s;
            const float  y = 150.0f * u, boardW = 300.0f * u, bh = 62.0f * u, gap = 14.0f * u;
            const float  fade = clampf( static_cast<float>( ( fork.m_at - 1900.0 - m_car.m_s ) / -150.0 ), 0.0f, 1.0f );
            const std::string head = ahead > 300.0 ? format( "THE ROAD FORKS IN %.1f KM", ahead / 1000.0 ) : std::string( "KEEP LEFT OR RIGHT NOW" );
            drawTextFx( head, W * 0.5f, y - 22.0f * u, 13.0f * u, ahead > 300.0 ? Ink : Gold, AlignE::CENTER, 0.3f );
            for( int k = 0; k < 2; ++k )
            {
                const bool      here = ( k == 0 ) == left;
                const BiomeE    to = k == 0 ? fork.m_left : fork.m_right;
                const BiomeDef &d = biomeDef( to );
                const float     boardX = k == 0 ? W * 0.5f - gap * 0.5f - boardW : W * 0.5f + gap * 0.5f;
                const Color     fill = here ? Color{ 255, 150, 40, static_cast<unsigned char>( 150.0f * fade ) } : Color{ 0, 0, 0, static_cast<unsigned char>( 110.0f * fade ) };
                DrawRectangleRounded( Rectangle{ boardX, y, boardW, bh }, 0.15f, 6, fill );
                if( here ) DrawRectangleRoundedLinesEx( Rectangle{ boardX, y, boardW, bh }, 0.15f, 6, 2.0f * u, Color{ 255, 220, 160, static_cast<unsigned char>( 230.0f * fade ) } );
                const std::string where = d.m_region && d.m_country ? std::string( d.m_region ) + " - " + d.m_country : d.m_country ? std::string( d.m_country ) : std::string();
                const float tx = k == 0 ? boardX + boardW - 16.0f * u : boardX + 16.0f * u;
                const AlignE al = k == 0 ? AlignE::RIGHT : AlignE::LEFT;
                drawTextFx( k == 0 ? "<" : ">", k == 0 ? boardX + 18.0f * u : boardX + boardW - 18.0f * u, y + 14.0f * u, 30.0f * u, here ? Ink : Dim, AlignE::CENTER, here ? 0.6f : 0.0f );
                drawTextFx( d.m_name ? d.m_name : "", tx, y + 10.0f * u, 19.0f * u, here ? Ink : Dim, al, here ? 0.5f : 0.0f );
                drawText( where, tx, y + 38.0f * u, 11.0f * u, here ? Ink : Dim, al );
            }
            drawText( left ? "YOU ARE ON THE LEFT" : "YOU ARE ON THE RIGHT", W * 0.5f, y + bh + 10.0f * u, 11.0f * u, Gold, AlignE::CENTER );
            if( m_mode == ModeE::RALLY && m_journey >= 0 && m_journey < static_cast<int>( journeys().size() ) )
            {
                // On a journey: how far the goal is either way, the shorter one in gold.
                const Journey &j = journeys()[static_cast<size_t>( m_journey )];
                const int      viaL = routeStages( fork.m_left, j.m_to ), viaR = routeStages( fork.m_right, j.m_to );
                const bool     shorterL = viaL > 0 && ( viaR <= 0 || viaL <= viaR );
                drawTextFx( format( "%d %s", viaL, viaL == 1 ? "STAGE" : "STAGES" ), W * 0.5f - gap * 0.5f - 16.0f * u, y + bh + 30.0f * u, 15.0f * u,
                            shorterL ? Gold : Ink, AlignE::RIGHT, shorterL ? 0.5f : 0.0f );
                drawTextFx( format( "%d %s", viaR, viaR == 1 ? "STAGE" : "STAGES" ), W * 0.5f + gap * 0.5f + 16.0f * u, y + bh + 30.0f * u, 15.0f * u,
                            shorterL ? Ink : Gold, AlignE::LEFT, shorterL ? 0.0f : 0.5f );
                drawTextFx( std::string( "TO " ) + biomeDef( j.m_to ).m_name, W * 0.5f, y + bh + 54.0f * u, 13.0f * u, Gold, AlignE::CENTER, 0.3f );
            }
        }
    }

    // The road ahead and the next checkpoint or the finish.
    const float mw = 210.0f * u, mh = 150.0f * u;
    drawMinimap( 24.0f * u, H - mh - 24.0f * u, mw, mh );
    if( !m_demo )
    {
        const double finish = m_setup.m_params.m_length;
        if( finish > 0.0 && ( racing || finish - m_car.m_s < m_nextCheckpoint - m_car.m_s + 1.0 ) )
            txt( format( "FINISH %.1f KM", std::max( 0.0, finish - m_car.m_s ) / 1000.0 ), 24.0f * u, H - mh - 46.0f * u, 12.0f * u, Ink );
        else if( !racing )
            txt( format( "CHECKPOINT %.1f KM", std::max( 0.0, m_nextCheckpoint - m_car.m_s ) / 1000.0 ), 24.0f * u, H - mh - 46.0f * u, 12.0f * u,
                 Green );
    }

    drawPopups();
}


void Game::drawPopups()
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = unit();
    // What matters now in the middle, small and above the horizon (in menus where the middle is free, lower); the news
    // in a ticker on the left, under the stage line.
    const bool racing = m_state == StateE::RACE || m_state == StateE::COUNTDOWN || m_state == StateE::PAUSED;
    float      py = racing ? 150.0f * u : H * 0.3f, ty = 132.0f * u;
    for( const Popup &p : m_popups )
    {
        const float in = std::min( 1.0f, p.m_age * 6.0f );
        const float out = std::min( 1.0f, ( p.m_life - p.m_age ) * 3.0f );
        const float k = in * out;
        Color       c = p.m_color;
        c.a = static_cast<unsigned char>( 230.0f * k );
        Color s = Ink;
        s.a = c.a;
        if( p.m_minor && racing )
        {
            const float size = 15.0f * u * std::min( 1.0f, p.m_size + 0.2f );
            const float x = 28.0f * u - ( 1.0f - in ) * 30.0f * u;
            DrawRectangle( static_cast<int>( x - 8.0f * u ), static_cast<int>( ty - 4.0f * u ),
                           static_cast<int>( std::max( textWidth( p.m_text, size ), textWidth( p.m_sub, size * 0.62f ) ) + 16.0f * u ),
                           static_cast<int>( size * ( p.m_sub.empty() ? 1.35f : 2.25f ) + 4.0f * u ), Color{ 0, 0, 0, static_cast<unsigned char>( 90.0f * k ) } );
            drawTextFx( p.m_text, x, ty, size, c, AlignE::LEFT, 0.4f * k );
            ty += size * 1.3f;
            if( !p.m_sub.empty() )
            {
                drawText( p.m_sub, x, ty, size * 0.62f, s, AlignE::LEFT );
                ty += size * 0.95f;
            }
            ty += 6.0f * u;
            continue;
        }
        const float size = ( racing ? 28.0f : 40.0f ) * u * p.m_size * ( 1.2f - 0.2f * in );
        drawTextFx( p.m_text, W * 0.5f, py, size, c, AlignE::CENTER, 0.9f * k );
        py += size * 1.3f;
        if( !p.m_sub.empty() )
        {
            drawText( p.m_sub, W * 0.5f, py, size * 0.48f, s, AlignE::CENTER );
            py += size * 0.85f;
        }
    }
}

void Game::drawMenu( const char *title, const std::vector<std::string> &items, const std::vector<std::string> &values, float y, float rowScale )
{
    const float W = static_cast<float>( GetScreenWidth() ), H = static_cast<float>( GetScreenHeight() );
    const float u = std::min( H / 720.0f, W / 1150.0f );
    const float rowH = 40.0f * u * rowScale;
    const float w = ( values.empty() ? 460.0f : 720.0f ) * u;
    const float x = W * 0.5f - w * 0.5f;
    // Long menus scroll: at most as many rows as fit, the selection kept in view.
    const int   fit = std::max( 4, static_cast<int>( ( H - y - 120.0f * u ) / rowH ) );
    const int   n = static_cast<int>( items.size() );
    const int   rows = std::min( n, fit );
    const int   first = std::clamp( m_menuIndex - rows / 2, 0, n - rows );
    const float h = 90.0f * u + rowH * static_cast<float>( rows );
    panel( x, y, w, h );
    drawTextFx( title, W * 0.5f, y + 22.0f * u, 30.0f * u, Accent, AlignE::CENTER, 0.8f );
    if( first > 0 ) drawText( "^", W * 0.5f, y + 56.0f * u, 10.0f * u, Dim, AlignE::CENTER );
    if( first + rows < n ) drawText( "v", W * 0.5f, y + h - 18.0f * u, 10.0f * u, Dim, AlignE::CENTER );
    for( int row = 0; row < rows; ++row )
    {
        const size_t i = static_cast<size_t>( first + row );
        const float  ry = y + 76.0f * u + rowH * static_cast<float>( row );
        const bool  sel = static_cast<int>( i ) == m_menuIndex;
        if( sel )
        {
            const float pulse = 0.6f + 0.4f * std::sin( m_time * 6.0f );
            DrawRectangleRounded( Rectangle{ x + 16.0f * u, ry - 8.0f * u, w - 32.0f * u, rowH - 4.0f * u }, 0.3f, 6,
                                  Color{ 255, 150, 40, static_cast<unsigned char>( 50 + 40 * pulse ) } );
        }
        const Color c = sel ? Ink : Dim;
        if( values.empty() || values[i].empty() )
        {
            drawText( items[i], W * 0.5f, ry, 20.0f * u, c, AlignE::CENTER );
        }
        else
        {
            drawText( items[i], x + 40.0f * u, ry, 18.0f * u, c, AlignE::LEFT );
            const std::string v = sel ? "< " + values[i] + " >" : values[i];
            drawText( v, x + w - 40.0f * u, ry, 18.0f * u, sel ? Accent : c, AlignE::RIGHT );
        }
    }
    const char *hint = m_input.m_gamepad ? "[A] SELECT     [B] BACK     [DPAD] CHANGE" : "[ENTER] SELECT     [ESC] BACK     [ARROWS] CHANGE";
    drawHint( hint, W * 0.5f, y + h + 18.0f * u, 13.0f * u, Color{ 220, 224, 235, 200 }, AlignE::CENTER );
}

} // namespace sunstreak
