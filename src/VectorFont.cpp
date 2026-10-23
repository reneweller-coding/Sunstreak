/**
 * @file VectorFont.cpp
 * @brief The stroke glyphs and their drawing.
 * @author reneweller
 */
#include "VectorFont.h"

#include "MathUtil.h"

#include <array>
#include <string>
#include <vector>

namespace sunstreak
{

namespace
{

constexpr float Slant = 0.22f;     ///< italic lean (x per y)
constexpr float Advance = 5.6f;    ///< glyph advance in grid units (glyphs are 4 wide, 6 high)

/** @brief A glyph: polylines on a 4 x 6 grid, y up. */
using Glyph = std::vector<std::vector<Vector2>>;

/**
 * @brief Parses "x,y x,y | x,y ..." into polylines.
 * @param[in] spec  the outline
 * @return the glyph
 */
Glyph parse( const char *spec )
{
    Glyph                g( 1 );
    std::string          token;
    const std::string    s = spec;
    size_t               i = 0;
    while( i <= s.size() )
    {
        const char ch = i < s.size() ? s[i] : ' ';
        if( ch == '|' )
        {
            g.emplace_back();
        }
        else if( ch == ' ' )
        {
            if( !token.empty() )
            {
                const auto comma = token.find( ',' );
                g.back().push_back( Vector2{ std::stof( token.substr( 0, comma ) ), std::stof( token.substr( comma + 1 ) ) } );
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
const std::array<Glyph, 128> &glyphs()
{
    static const std::array<Glyph, 128> table = []
    {
        std::array<Glyph, 128> t{};
        auto set = [&]( char c, const char *spec ) { t[static_cast<size_t>( c )] = parse( spec ); };
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
    float       pen = x;
    for( char raw : text )
    {
        char c = raw;
        if( c >= 'a' && c <= 'z' ) c = static_cast<char>( c - 'a' + 'A' );
        const auto idx = static_cast<unsigned char>( c );
        if( idx < 128 )
        {
            for( const auto &line : table[idx] )
            {
                for( size_t i = 0; i < line.size(); ++i )
                {
                    const Vector2 p = { pen + ( line[i].x + line[i].y * Slant ) * u, y + ( 6.0f - line[i].y ) * u };
                    if( round ) DrawCircleV( p, thick * 0.5f, color );
                    if( i + 1 < line.size() )
                    {
                        const Vector2 q = { pen + ( line[i + 1].x + line[i + 1].y * Slant ) * u, y + ( 6.0f - line[i + 1].y ) * u };
                        DrawLineEx( p, q, thick, color );
                    }
                }
            }
        }
        pen += Advance * u;
    }
}

} // namespace

float textWidth( std::string_view text, float size )
{
    if( text.empty() ) return 0.0f;
    const float u = size / 6.0f;
    return ( static_cast<float>( text.size() ) * Advance - ( Advance - 4.0f ) ) * u + 6.0f * Slant * u;
}

void drawText( std::string_view text, float x, float y, float size, Color color, AlignE align, float weight )
{
    const float w = textWidth( text, size );
    if( align == AlignE::CENTER ) x -= w * 0.5f;
    else if( align == AlignE::RIGHT ) x -= w;
    strokes( text, x, y, size, color, std::max( 1.2f, size * weight ) );
}

void drawTextFx( std::string_view text, float x, float y, float size, Color color, AlignE align, float glow )
{
    const float w = textWidth( text, size );
    if( align == AlignE::CENTER ) x -= w * 0.5f;
    else if( align == AlignE::RIGHT ) x -= w;
    const float thick = std::max( 1.2f, size * 0.13f );
    if( glow > 0.0f )
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

} // namespace sunstreak
