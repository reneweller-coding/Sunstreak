/**
 * @file VectorFont.cpp
 * @brief The stroke glyphs and their drawing.
 * @author Rene Weller
 */
#include "VectorFont.h"

#include "MathUtil.h"

#include <rlgl.h>

#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace sunstreak
{

namespace
{

constexpr float Slant = 0.22f;    ///< italic lean (x per y)
constexpr float Advance = 5.6f;   ///< glyph advance in grid units (glyphs are 4 wide, 6 high)

/** @brief A glyph: polylines on a 4 x 6 grid, y up. */
using GlyphT = std::vector<std::vector<Vector2>>;

/**
 * @brief Parses "x,y x,y | x,y ..." into polylines.
 * @param[in] spec  the outline
 * @return the glyph
 */
GlyphT parse( const char *spec )
{
    GlyphT            g( 1 );
    std::string       token;
    const std::string s = spec;
    size_t            i = 0;
    while ( i <= s.size() )
    {
        const char ch = i < s.size() ? s[i] : ' ';
        if ( ch == '|' )
        {
            g.emplace_back();
        }
        else if ( ch == ' ' )
        {
            if ( !token.empty() )
            {
                const auto comma = token.find( ',' );
                g.back().push_back(
                    Vector2{ std::stof( token.substr( 0, comma ) ), std::stof( token.substr( comma + 1 ) ) } );
                token.clear();
            }
        }
        else
        {
            token += ch;
        }
        ++i;
    }
    return g;
}

/**
 * @brief The glyph table, built once.
 * @return glyphs indexed by ASCII code
 */
const std::array<GlyphT, 128> &glyphs()
{
    static const std::array<GlyphT, 128> table = []
    {
        std::array<GlyphT, 128> t{};
        auto                    set = [&]( char c, const char *spec ) { t[static_cast<size_t>( c )] = parse( spec ); };
        set( '0', "0,1 0,5 1,6 3,6 4,5 4,1 3,0 1,0 0,1 | 1,1.5 3,4.5" );
        set( '1', "1,5 2,6 2,0 | 1,0 3,0" );
        set( '2', "0,5 1,6 3,6 4,5 4,4 0,0 4,0" );
        set( '3', "0,5 1,6 3,6 4,5 4,4 3,3 4,2 4,1 3,0 1,0 0,1 | 1.5,3 3,3" );
        set( '4', "3,0 3,6 0,2 4,2" );
        set( '5', "4,6 0,6 0,3.4 3,3.4 4,2.4 4,1 3,0 1,0 0,1" );
        set( '6', "4,5 3,6 1,6 0,5 0,1 1,0 3,0 4,1 4,2.4 3,3.4 0,3.4" );
        set( '7', "0,6 4,6 4,5 1.5,0" );
        set( '8', "1,3 0,4 0,5 1,6 3,6 4,5 4,4 3,3 1,3 0,2 0,1 1,0 3,0 4,1 4,2 3,3" );
        set( '9', "0,1 1,0 3,0 4,1 4,5 3,6 1,6 0,5 0,3.6 1,2.6 4,2.6" );
        set( 'A', "0,0 0,5 1,6 3,6 4,5 4,0 | 0,3 4,3" );
        set( 'B', "0,0 0,6 3,6 4,5 4,4 3,3 0,3 | 3,3 4,2 4,1 3,0 0,0" );
        set( 'C', "4,5 3,6 1,6 0,5 0,1 1,0 3,0 4,1" );
        set( 'D', "0,0 0,6 3,6 4,5 4,1 3,0 0,0" );
        set( 'E', "4,6 0,6 0,0 4,0 | 0,3 3,3" );
        set( 'F', "4,6 0,6 0,0 | 0,3 3,3" );
        set( 'G', "4,5 3,6 1,6 0,5 0,1 1,0 3,0 4,1 4,3 2.2,3" );
        set( 'H', "0,0 0,6 | 4,0 4,6 | 0,3 4,3" );
        set( 'I', "1,6 3,6 | 2,6 2,0 | 1,0 3,0" );
        set( 'J', "1,6 4,6 4,1 3,0 1,0 0,1" );
        set( 'K', "0,0 0,6 | 4,6 1,3 0,3 | 1,3 4,0" );
        set( 'L', "0,6 0,0 4,0" );
        set( 'M', "0,0 0,6 2,3.5 4,6 4,0" );
        set( 'N', "0,0 0,6 4,0 4,6" );
        set( 'O', "0,1 0,5 1,6 3,6 4,5 4,1 3,0 1,0 0,1" );
        set( 'P', "0,0 0,6 3,6 4,5 4,4 3,3 0,3" );
        set( 'Q', "0,1 0,5 1,6 3,6 4,5 4,1 3,0 1,0 0,1 | 2.6,1.6 4.2,-0.2" );
        set( 'R', "0,0 0,6 3,6 4,5 4,4 3,3 0,3 | 1.6,3 4,0" );
        set( 'S', "4,5 3,6 1,6 0,5 0,4 1,3 3,3 4,2 4,1 3,0 1,0 0,1" );
        set( 'T', "0,6 4,6 | 2,6 2,0" );
        set( 'U', "0,6 0,1 1,0 3,0 4,1 4,6" );
        set( 'V', "0,6 2,0 4,6" );
        set( 'W', "0,6 0.8,0 2,3 3.2,0 4,6" );
        set( 'X', "0,6 4,0 | 4,6 0,0" );
        set( 'Y', "0,6 2,3 4,6 | 2,3 2,0" );
        set( 'Z', "0,6 4,6 0,0 4,0" );
        set( '-', "0.8,3 3.2,3" );
        set( '+', "0.6,3 3.4,3 | 2,1.6 2,4.4" );
        set( '.', "1.9,0 2.1,0" );
        set( ',', "2,0.4 1.4,-0.8" );
        set( ':', "2,1.4 2,1.6 | 2,4.4 2,4.6" );
        set( '/', "0.5,0 3.5,6" );
        set( '!', "2,6 2,2 | 2,0.1 2,-0.1" );
        set( '?', "0,5 1,6 3,6 4,5 4,4 2,2.6 2,1.8 | 2,0.1 2,-0.1" );
        set( '%', "0,0 4,6 | 0.6,5.2 0.8,5.2 | 3.2,0.8 3.4,0.8" );
        set( '\'', "2,6 2,4.6" );
        set( '(', "3,6 1.6,4.4 1.6,1.6 3,0" );
        set( ')', "1,6 2.4,4.4 2.4,1.6 1,0" );
        set( '<', "4,5 0.5,3 4,1" );
        set( '>', "0,5 3.5,3 0,1" );
        set( '*', "2,1.5 2,4.5 | 0.7,2.2 3.3,3.8 | 0.7,3.8 3.3,2.2" );
        set( '=', "0.6,2 3.4,2 | 0.6,4 3.4,4" );
        set( '_', "0,-0.6 4,-0.6" );
        set( '#', "1.2,0 1.6,6 | 2.6,0 3,6 | 0,2 4,2 | 0,4 4,4" );
        return t;
    }();
    return table;
}

/**
 * @brief Draws the strokes of a text once.
 * @param[in] text   the text
 * @param[in] x      left edge
 * @param[in] y      top of the capitals
 * @param[in] size   cap height
 * @param[in] color  colour
 * @param[in] thick  stroke width in pixels
 * @param[in] round  round joints (off for translucent passes, where overlaps would show as beads)
 */
void strokes( std::string_view text, float x, float y, float size, Color color, float thick, bool round = true )
{
    const float u = size / 6.0f;
    const auto &table = glyphs();
    // The joints: small fans with as few segments as their radius needs (raylib's circles have 36, and they were most
    // of what the HUD cost), each polyline sent as one batch of triangles in raylib's own winding.
    const float r = thick * 0.5f;
    const int   segs = r < 1.5f ? 6 : r < 3.0f ? 8 : r < 6.0f ? 12 : 16;
    Vector2     ring[17];
    for ( int k = 0; k <= segs; ++k )
    {
        const float a = Tau * static_cast<float>( k ) / static_cast<float>( segs );
        ring[k] = Vector2{ std::cos( a ) * r, std::sin( a ) * r };
    }
    float pen = x;
    for ( char raw : text )
    {
        char c = raw;
        if ( c >= 'a' && c <= 'z' ) c = static_cast<char>( c - 'a' + 'A' );
        const auto idx = static_cast<unsigned char>( c );
        if ( idx < 128 )
        {
            for ( const auto &line : table[idx] )
            {
                const size_t n = line.size();
                rlCheckRenderBatchLimit( static_cast<int>( ( n > 1 ? ( n - 1 ) * 6 : 0 ) +
                                                           ( round ? n * static_cast<size_t>( segs ) * 3 : 0 ) ) );
                rlBegin( RL_TRIANGLES );
                rlColor4ub( color.r, color.g, color.b, color.a );
                Vector2 prev{};
                for ( size_t i = 0; i < n; ++i )
                {
                    const Vector2 p = { pen + ( line[i].x + line[i].y * Slant ) * u, y + ( 6.0f - line[i].y ) * u };
                    if ( round )
                        for ( int k = 0; k < segs; ++k )
                        {
                            rlVertex2f( p.x, p.y );
                            rlVertex2f( p.x + ring[k + 1].x, p.y + ring[k + 1].y );
                            rlVertex2f( p.x + ring[k].x, p.y + ring[k].y );
                        }
                    if ( i > 0 )
                    {
                        // The segment from the last point: a quad of two triangles, as DrawLineEx builds it.
                        const Vector2 d = { p.x - prev.x, p.y - prev.y };
                        const float   len = std::sqrt( d.x * d.x + d.y * d.y );
                        if ( len > 0.0f )
                        {
                            const float   s = thick / ( 2.0f * len );
                            const Vector2 h = { -s * d.y, s * d.x };
                            const Vector2 s0 = { prev.x - h.x, prev.y - h.y }, s1 = { prev.x + h.x, prev.y + h.y };
                            const Vector2 s2 = { p.x - h.x, p.y - h.y }, s3 = { p.x + h.x, p.y + h.y };
                            rlVertex2f( s2.x, s2.y );
                            rlVertex2f( s0.x, s0.y );
                            rlVertex2f( s1.x, s1.y );
                            rlVertex2f( s3.x, s3.y );
                            rlVertex2f( s2.x, s2.y );
                            rlVertex2f( s1.x, s1.y );
                        }
                    }
                    prev = p;
                }
                rlEnd();
            }
        }
        pen += Advance * u;
    }
}

}   // namespace

float textStrokes( std::string_view text, std::vector<std::pair<Vector2, Vector2>> &lines )
{
    const auto &table = glyphs();
    float       pen = 0.0f;
    for ( char raw : text )
    {
        char c = raw;
        if ( c >= 'a' && c <= 'z' ) c = static_cast<char>( c - 'a' + 'A' );
        const auto idx = static_cast<unsigned char>( c );
        if ( idx < 128 )
            for ( const auto &line : table[idx] )
                for ( size_t i = 0; i + 1 < line.size(); ++i )
                    lines.push_back( { Vector2{ ( pen + line[i].x ) / 6.0f, line[i].y / 6.0f },
                                       Vector2{ ( pen + line[i + 1].x ) / 6.0f, line[i + 1].y / 6.0f } } );
        pen += Advance;
    }
    return text.empty() ? 0.0f : ( pen - ( Advance - 4.0f ) ) / 6.0f;
}

namespace
{

float g_textScale = 1.0f;   ///< the UI's text scale

/**
 * @brief The width of a text at a size already scaled.
 * @param[in] text  the text
 * @param[in] size  cap height in pixels
 * @return the width in pixels
 */
float rawWidth( std::string_view text, float size )
{
    if ( text.empty() ) return 0.0f;
    const float u = size / 6.0f;
    return ( static_cast<float>( text.size() ) * Advance - ( Advance - 4.0f ) ) * u + 6.0f * Slant * u;
}

}   // namespace

void setTextScale( float scale )
{
    g_textScale = std::clamp( scale, 0.5f, 2.0f );
}

float textScale()
{
    return g_textScale;
}

float textWidth( std::string_view text, float size )
{
    return rawWidth( text, size * g_textScale );
}

void drawText( std::string_view text, float x, float y, float size, Color color, AlignE align, float weight )
{
    size *= g_textScale;
    const float w = rawWidth( text, size );
    if ( align == AlignE::CENTER )
        x -= w * 0.5f;
    else if ( align == AlignE::RIGHT )
        x -= w;
    strokes( text, x, y, size, color, std::max( 1.2f, size * weight ) );
}

void drawTextFx( std::string_view text, float x, float y, float size, Color color, AlignE align, float glow )
{
    size *= g_textScale;
    const float w = rawWidth( text, size );
    if ( align == AlignE::CENTER )
        x -= w * 0.5f;
    else if ( align == AlignE::RIGHT )
        x -= w;
    const float thick = std::max( 1.2f, size * 0.13f );
    if ( glow > 0.0f )
    {
        Color g = color;
        g.a = static_cast<unsigned char>( 22.0f * glow * static_cast<float>( color.a ) / 255.0f );
        strokes( text, x, y, size, g, thick * 2.6f, false );
        strokes( text, x, y, size, g, thick * 1.7f, false );
    }
    const Color shadow = { 0, 0, 0, static_cast<unsigned char>( 150.0f * static_cast<float>( color.a ) / 255.0f ) };
    strokes( text, x + size * 0.06f, y + size * 0.08f, size, shadow, thick );
    strokes( text, x, y, size, color, thick );
}

}   // namespace sunstreak
