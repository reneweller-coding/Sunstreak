/**
 * @file WorldMap.cpp
 * @brief Reading the outlines and painting the passport's map (scanline fills and stamped lines on a pixel buffer).
 * @author Rene Weller
 */
#include "WorldMap.h"

#include "Data.h"
#include "MathUtil.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace sunstreak
{

namespace
{

/**
 * @brief Reads an outline file.
 * @param[in] name  below data/
 * @return its areas
 */
std::vector<MapArea> readAreas( const std::string &name )
{
    std::vector<MapArea> out;
    for( const std::string &line : dataLines( dataText( name ) ) )
    {
        if( line.rfind( "area ", 0 ) == 0 )
        {
            out.push_back( MapArea{ line.substr( 5 ), {}, {} } );
            continue;
        }
        if( line.rfind( "ring ", 0 ) != 0 || out.empty() ) continue;
        std::vector<Vector2> ring;
        const char          *p = line.c_str() + 5;
        char                *end = nullptr;
        for( ;; )
        {
            const long x = std::strtol( p, &end, 10 );
            if( end == p ) break;
            p = end;
            const long y = std::strtol( p, &end, 10 );
            if( end == p ) break;
            p = end;
            ring.push_back( Vector2{ static_cast<float>( x ) * 0.01f, static_cast<float>( y ) * 0.01f } );
        }
        if( ring.size() >= 3 ) out.back().m_rings.push_back( std::move( ring ) );
    }
    for( MapArea &a : out )
    {
        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
        for( const auto &ring : a.m_rings )
            for( const Vector2 &v : ring )
            {
                x0 = std::min( x0, v.x ), x1 = std::max( x1, v.x );
                y0 = std::min( y0, v.y ), y1 = std::max( y1, v.y );
            }
        a.m_box = Rectangle{ x0, y0, x1 - x0, y1 - y0 };
    }
    return out;
}

/** @brief A picture being painted, colours blended in. */
struct Canvas
{
    int                m_w = 0;            ///< width
    int                m_h = 0;            ///< height
    std::vector<Color> m_px;               ///< the pixels

    /**
     * @brief Blends a colour into a pixel.
     * @param[in] x    column
     * @param[in] y    row
     * @param[in] c    the colour
     * @param[in] k    0..1 how much of it
     */
    void blend( int x, int y, Color c, float k )
    {
        if( x < 0 || y < 0 || x >= m_w || y >= m_h ) return;
        Color      &d = m_px[static_cast<size_t>( y ) * static_cast<size_t>( m_w ) + static_cast<size_t>( x )];
        const float a = static_cast<float>( c.a ) / 255.0f * k, da = static_cast<float>( d.a ) / 255.0f;
        const float out = a + da * ( 1.0f - a );
        if( out <= 0.0f ) return;
        auto mix = [&]( unsigned char s, unsigned char t ) { return static_cast<unsigned char>( ( s * a + t * da * ( 1.0f - a ) ) / out ); };
        d = Color{ mix( c.r, d.r ), mix( c.g, d.g ), mix( c.b, d.b ), static_cast<unsigned char>( out * 255.0f ) };
    }

    /**
     * @brief Fills an area's rings (even-odd), a scanline at a time.
     * @param[in] rings  the rings in pixels
     * @param[in] c      the colour
     */
    void fill( const std::vector<std::vector<Vector2>> &rings, Color c )
    {
        float y0 = 1e9f, y1 = -1e9f;
        for( const auto &r : rings )
            for( const Vector2 &v : r ) y0 = std::min( y0, v.y ), y1 = std::max( y1, v.y );
        std::vector<float> xs;
        for( int y = std::max( 0, static_cast<int>( y0 ) ); y <= std::min( m_h - 1, static_cast<int>( y1 ) ); ++y )
        {
            const float yc = static_cast<float>( y ) + 0.5f;
            xs.clear();
            for( const auto &r : rings )
                for( size_t i = 0; i < r.size(); ++i )
                {
                    const Vector2 &p = r[i], &q = r[( i + 1 ) % r.size()];
                    if( ( p.y <= yc ) != ( q.y <= yc ) ) xs.push_back( p.x + ( yc - p.y ) / ( q.y - p.y ) * ( q.x - p.x ) );
                }
            std::sort( xs.begin(), xs.end() );
            for( size_t i = 0; i + 1 < xs.size(); i += 2 )
                for( int x = std::max( 0, static_cast<int>( std::ceil( xs[i] - 0.5f ) ) ); x <= std::min( m_w - 1, static_cast<int>( std::floor( xs[i + 1] - 0.5f ) ) ); ++x )
                    blend( x, y, c, 1.0f );
        }
    }

    /**
     * @brief Draws a line of some width, its edges soft.
     * @param[in] a      from
     * @param[in] b      to
     * @param[in] width  in pixels
     * @param[in] c      the colour
     */
    void line( Vector2 a, Vector2 b, float width, Color c )
    {
        // Each pixel once, by its distance to the segment; the far end is left to the next segment of the ring.
        const float dx = b.x - a.x, dy = b.y - a.y, ll = dx * dx + dy * dy, r = width * 0.5f;
        const int   x0 = static_cast<int>( std::floor( std::min( a.x, b.x ) - r - 1.0f ) ), x1 = static_cast<int>( std::ceil( std::max( a.x, b.x ) + r + 1.0f ) );
        const int   y0 = static_cast<int>( std::floor( std::min( a.y, b.y ) - r - 1.0f ) ), y1 = static_cast<int>( std::ceil( std::max( a.y, b.y ) + r + 1.0f ) );
        for( int y = std::max( 0, y0 ); y <= std::min( m_h - 1, y1 ); ++y )
            for( int x = std::max( 0, x0 ); x <= std::min( m_w - 1, x1 ); ++x )
            {
                const float px = static_cast<float>( x ) + 0.5f, py = static_cast<float>( y ) + 0.5f;
                const float t = ll > 0.0f ? ( ( px - a.x ) * dx + ( py - a.y ) * dy ) / ll : 0.0f;
                if( t > 1.0f ) continue;
                const float tc = std::max( 0.0f, t ), qx = a.x + dx * tc - px, qy = a.y + dy * tc - py;
                const float k = clampf( r + 0.5f - std::sqrt( qx * qx + qy * qy ), 0.0f, 1.0f );
                if( k > 0.0f ) blend( x, y, c, k );
            }
    }
};

} // namespace

const std::vector<MapArea> &mapCountries()
{
    static DataCache<std::vector<MapArea>> cache;
    return cache.get( [] { return readAreas( "world/countries.txt" ); } );
}

const std::vector<MapArea> &mapProvinces()
{
    static DataCache<std::vector<MapArea>> cache;
    return cache.get( [] { return readAreas( "world/provinces.txt" ); } );
}

bool areaContains( const MapArea &a, float lon, float lat )
{
    if( lon < a.m_box.x || lat < a.m_box.y || lon > a.m_box.x + a.m_box.width || lat > a.m_box.y + a.m_box.height ) return false;
    bool in = false;
    for( const auto &r : a.m_rings )
        for( size_t i = 0, j = r.size() - 1; i < r.size(); j = i++ )
            if( ( r[i].y > lat ) != ( r[j].y > lat ) && lon < ( r[j].x - r[i].x ) * ( lat - r[i].y ) / ( r[j].y - r[i].y ) + r[i].x ) in = !in;
    return in;
}

std::vector<bool> areasWith( const std::vector<MapArea> &areas, const std::vector<GeoPoint> &points )
{
    std::vector<bool> hit( areas.size(), false );
    static const float offsets[][2] = { { 0, 0 }, { 0.3f, 0 }, { -0.3f, 0 }, { 0, 0.3f }, { 0, -0.3f }, { 0.25f, 0.25f }, { -0.25f, -0.25f },
                                        { 0.25f, -0.25f }, { -0.25f, 0.25f } };
    for( const GeoPoint &g : points )
        for( const auto &o : offsets )
        {
            bool found = false;
            for( size_t i = 0; i < areas.size() && !found; ++i )
                if( areaContains( areas[i], g.m_lon + o[0], g.m_lat + o[1] ) )
                {
                    hit[i] = true;
                    found = true;
                }
            if( found ) break;
        }
    return hit;
}

Image paintWorldMap( int w, int h, float latTop, float latBottom, const std::vector<GeoPoint> &seen )
{
    Canvas c;
    c.m_w = w;
    c.m_h = h;
    c.m_px.assign( static_cast<size_t>( w ) * static_cast<size_t>( h ), Color{ 0, 0, 0, 0 } );
    auto toPixels = [&]( const MapArea &a )
    {
        std::vector<std::vector<Vector2>> out;
        for( const auto &r : a.m_rings )
        {
            std::vector<Vector2> p;
            p.reserve( r.size() );
            for( const Vector2 &v : r )
                p.push_back( Vector2{ ( v.x + 180.0f ) / 360.0f * static_cast<float>( w ), ( latTop - v.y ) / ( latTop - latBottom ) * static_cast<float>( h ) } );
            out.push_back( std::move( p ) );
        }
        return out;
    };
    const auto             &countries = mapCountries();
    const auto             &provinces = mapProvinces();
    const std::vector<bool> countrySeen = areasWith( countries, seen ), provinceSeen = areasWith( provinces, seen );
    // The land, the countries you have been to, the provinces you have been to.
    std::vector<std::vector<std::vector<Vector2>>> countryPx;
    countryPx.reserve( countries.size() );
    for( size_t i = 0; i < countries.size(); ++i )
    {
        countryPx.push_back( toPixels( countries[i] ) );
        c.fill( countryPx.back(), countrySeen[i] ? Color{ 52, 112, 118, 255 } : Color{ 36, 54, 82, 255 } );
    }
    for( size_t i = 0; i < provinces.size(); ++i )
    {
        const auto px = toPixels( provinces[i] );
        if( provinceSeen[i] ) c.fill( px, Color{ 86, 176, 150, 255 } );
        for( const auto &r : px )
            for( size_t k = 0; k < r.size(); ++k ) c.line( r[k], r[( k + 1 ) % r.size()], 1.0f, Color{ 230, 240, 255, 60 } );
    }
    // The country borders and the coasts over everything.
    for( const auto &px : countryPx )
        for( const auto &r : px )
            for( size_t k = 0; k < r.size(); ++k ) c.line( r[k], r[( k + 1 ) % r.size()], 2.2f, Color{ 222, 236, 250, 200 } );
    Image img = GenImageColor( w, h, BLANK );
    std::copy( c.m_px.begin(), c.m_px.end(), static_cast<Color *>( img.data ) );
    return img;
}

} // namespace sunstreak
