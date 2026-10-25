/**
 * @file ModelsWorld.cpp
 * @brief The world tour's props: trees of every climate, landforms, regional houses and landmarks, boats, animals.
 *
 * Props with key colours (keyA/B/C) take their paint from the biome's rule, so one cottage serves Ireland and
 * Mexico; keyC marks windows that stay dark by day and glow where a night biome passes a warm colour. Fronts face +x
 * (FacingE::ROAD turns them to the road), boards face -z (FacingE::ALONG shows them to the driver).
 *
 * @author Rene Weller
 */
#include "Models.h"

#include "Data.h"

#include "MathUtil.h"
#include "Rng.h"
#include "VectorFont.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>

namespace sunstreak
{

namespace
{

/**
 * @brief Shorthand for an opaque colour.
 * @param[in] r  red
 * @param[in] g  green
 * @param[in] b  blue
 * @return the colour
 */
constexpr Color rgb( int r, int g, int b )
{
    return Color{ static_cast<unsigned char>( r ), static_cast<unsigned char>( g ), static_cast<unsigned char>( b ),
                  255 };
}

constexpr Mat Glow{ 1.0f, 0.0f };     ///< fully self-lit
constexpr Mat Lit{ 0.85f, 0.0f };     ///< a lit window (its colour decides how bright)
constexpr Mat Glass{ 0.0f, 0.95f };   ///< glass
constexpr Mat Shiny{ 0.0f, 0.6f };    ///< metal, glazed tiles
constexpr Mat Water{ 0.0f, 0.9f };    ///< wet

/**
 * @brief Windows on one face: every one keyed (keyC), the unlit ones darker.
 * @param[out] m        where the triangles go
 * @param[in]  origin   the face's lower left corner
 * @param[in]  across   unit vector along the face
 * @param[in]  out      the face's outward normal
 * @param[in]  width    face width
 * @param[in]  height   face height
 * @param[in]  seed     which windows are lit
 * @param[in]  lit      0..1 lit share
 * @param[in]  floor    storey height
 * @param[in]  first    sill of the lowest row
 */
void panes( MeshBuilder &m, Vector3 origin, Vector3 across, Vector3 out, float width, float height, uint64_t seed,
            float lit, float floor = 3.0f, float first = 1.0f )
{
    const int     cols = std::max( 1, static_cast<int>( width / 2.6f ) );
    const int     rows = std::max( 1, static_cast<int>( ( height - first - 0.4f ) / floor ) + 1 );
    const float   step = width / static_cast<float>( cols );
    const Vector3 lift = Vector3Add( origin, Vector3Scale( out, 0.05f ) );
    for ( int r = 0; r < rows; ++r )
    {
        const float y0 = first + static_cast<float>( r ) * floor, y1 = y0 + std::min( 1.4f, floor * 0.5f );
        if ( y1 > height - 0.3f ) break;
        for ( int c = 0; c < cols; ++c )
        {
            const bool  on = hashFloat( seed, static_cast<uint64_t>( r * 131 + c ) ) < lit;
            const float x0 = step * ( static_cast<float>( c ) + 0.28f ),
                        x1 = step * ( static_cast<float>( c ) + 0.72f );
            auto        p = [&]( float x, float y )
            { return Vector3Add( lift, Vector3Add( Vector3Scale( across, x ), Vector3{ 0, y, 0 } ) ); };
            m.quad( p( x0, y0 ), p( x1, y0 ), p( x1, y1 ), p( x0, y1 ), keyC( on ? 1.0f : 0.25f ), on ? Lit : Glass );
        }
    }
}

/**
 * @brief Windows on all four walls of a box standing on the ground.
 * @param[out] m       where the triangles go
 * @param[in]  c       the box's centre at ground level
 * @param[in]  hx      half its size along x
 * @param[in]  hz      half its size along z
 * @param[in]  height  its height
 * @param[in]  seed    which windows are lit
 * @param[in]  lit     lit share
 * @param[in]  floor   storey height
 * @param[in]  first   sill of the lowest row
 */
void boxPanes( MeshBuilder &m, Vector3 c, float hx, float hz, float height, uint64_t seed, float lit,
               float floor = 3.0f, float first = 1.0f )
{
    panes( m, Vector3{ c.x + hx, c.y, c.z - hz }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, hz * 2.0f, height, seed, lit,
           floor, first );
    panes( m, Vector3{ c.x - hx, c.y, c.z + hz }, Vector3{ 0, 0, -1 }, Vector3{ -1, 0, 0 }, hz * 2.0f, height, seed + 1,
           lit, floor, first );
    panes( m, Vector3{ c.x - hx, c.y, c.z - hz }, Vector3{ 1, 0, 0 }, Vector3{ 0, 0, -1 }, hx * 2.0f, height, seed + 2,
           lit, floor, first );
    panes( m, Vector3{ c.x + hx, c.y, c.z + hz }, Vector3{ -1, 0, 0 }, Vector3{ 0, 0, 1 }, hx * 2.0f, height, seed + 3,
           lit, floor, first );
}

/**
 * @brief A gable roof with its ridge along z, and the gable walls under it.
 * @param[out] m     where the triangles go
 * @param[in]  c     centre of the roof's base (the top of the walls)
 * @param[in]  hx    half the walls' width
 * @param[in]  hz    half the walls' length
 * @param[in]  rise  ridge height above the base
 * @param[in]  over  overhang
 * @param[in]  roof  roof colour
 * @param[in]  wall  gable colour
 */
void gable( MeshBuilder &m, Vector3 c, float hx, float hz, float rise, float over, Color roof, Color wall )
{
    const float   x = hx + over, z = hz + over, y = c.y - over * rise / std::max( hx, 0.1f );
    const Vector3 e0 = { c.x - x, y, c.z - z }, e1 = { c.x + x, y, c.z - z }, e2 = { c.x + x, y, c.z + z },
                  e3 = { c.x - x, y, c.z + z };
    const Vector3 r0 = { c.x, c.y + rise, c.z - z }, r1 = { c.x, c.y + rise, c.z + z };
    m.quad( e0, r0, r1, e3, roof );
    m.quad( e1, e2, r1, r0, roof );
    m.tri( Vector3{ c.x - hx, c.y, c.z - hz }, Vector3{ c.x + hx, c.y, c.z - hz }, Vector3{ c.x, c.y + rise, c.z - hz },
           wall );
    m.tri( Vector3{ c.x - hx, c.y, c.z + hz }, Vector3{ c.x + hx, c.y, c.z + hz }, Vector3{ c.x, c.y + rise, c.z + hz },
           wall );
}

/**
 * @brief A hip roof (a frustum with a short ridge).
 * @param[out] m     where the triangles go
 * @param[in]  c     centre of its base
 * @param[in]  hx    half width
 * @param[in]  hz    half length
 * @param[in]  rise  height
 * @param[in]  over  overhang
 * @param[in]  roof  colour
 */
void hip( MeshBuilder &m, Vector3 c, float hx, float hz, float rise, float over, Color roof )
{
    m.taperedBox( c, Vector2{ hx + over, hz + over }, Vector3{ c.x, c.y + rise, c.z },
                  Vector2{ 0.05f, std::max( 0.05f, hz - hx ) }, roof );
}

/**
 * @brief A palm's crown of fronds.
 * @param[out] m      where the triangles go
 * @param[in]  top    where the fronds start
 * @param[in]  n      how many
 * @param[in]  len    their length
 * @param[in]  droop  how far the tips hang (m)
 * @param[in]  col    colour
 * @param[in]  phase  rotation
 */
void fronds( MeshBuilder &m, Vector3 top, int n, float len, float droop, Color col, float phase )
{
    for ( int i = 0; i < n; ++i )
    {
        const float   a = Tau * static_cast<float>( i ) / static_cast<float>( n ) + phase;
        const Vector3 d = { std::cos( a ), 0, std::sin( a ) }, s = { -d.z * 0.45f, 0, d.x * 0.45f };
        const Vector3 mid = { top.x + d.x * len * 0.5f, top.y + len * 0.18f, top.z + d.z * len * 0.5f };
        const Vector3 tip = { top.x + d.x * len, top.y - droop, top.z + d.z * len };
        m.tri( top, Vector3Add( mid, s ), mid, col );
        m.tri( top, mid, Vector3Subtract( mid, s ), scaleColor( col, 0.85f ) );
        m.tri( Vector3Add( mid, s ), tip, mid, col );
        m.tri( mid, tip, Vector3Subtract( mid, s ), scaleColor( col, 0.85f ) );
    }
}

/**
 * @brief Crenellations along the top of a wall.
 * @param[out] m    where the triangles go
 * @param[in]  a    one end of the wall's top centre line
 * @param[in]  b    the other end
 * @param[in]  w    wall half thickness
 * @param[in]  col  colour
 */
void merlons( MeshBuilder &m, Vector3 a, Vector3 b, float w, Color col )
{
    const float len = Vector3Distance( a, b );
    const int   n = std::max( 2, static_cast<int>( len / 1.4f ) );
    for ( int i = 0; i <= n; i += 2 )
    {
        const Vector3 p = Vector3Lerp( a, b, static_cast<float>( i ) / static_cast<float>( n ) );
        m.box( Vector3{ p.x, p.y + 0.45f, p.z }, Vector3{ w * 0.9f + 0.05f, 0.45f, w * 0.9f + 0.05f }, col );
    }
}

/** @brief The shape of a four-legged animal (+z forward). */
struct Beast
{
    float    m_length = 2.0f;                  ///< body length
    float    m_height = 0.5f;                  ///< body half height
    float    m_width = 0.4f;                   ///< body half width
    float    m_legs = 0.8f;                    ///< leg length below the body
    float    m_legR = 0.09f;                   ///< leg radius
    float    m_neck = 0.5f;                    ///< neck length
    float    m_neckUp = 0.6f;                  ///< neck angle above horizontal (rad)
    float    m_head = 0.5f;                    ///< head length
    float    m_headR = 0.18f;                  ///< head half height
    float    m_tail = 0.6f;                    ///< tail length
    float    m_lumpy = 0.12f;                  ///< body roughness (wool)
    Color    m_body = rgb( 120, 90, 60 );      ///< body
    Color    m_legsCol = rgb( 90, 70, 50 );    ///< legs
    Color    m_headCol = rgb( 110, 84, 56 );   ///< head and neck
    unsigned m_seed = 1;                       ///< shape seed
};

/**
 * @brief Builds a four-legged animal.
 * @param[out] m  where the triangles go
 * @param[in]  b  its shape
 * @return the body's centre (for humps, spots, antlers)
 */
Vector3 beast( MeshBuilder &m, const Beast &b )
{
    const float   y = b.m_legs + b.m_height * 0.85f;
    const Vector3 body = { 0, y, 0 };
    m.blob( body, Vector3{ b.m_width, b.m_height, b.m_length * 0.5f }, b.m_body, b.m_seed, b.m_lumpy );
    for ( float sx : { -1.0f, 1.0f } )
        for ( float sz : { -1.0f, 1.0f } )
        {
            const Vector3 top = { sx * b.m_width * 0.55f, y - b.m_height * 0.4f, sz * b.m_length * 0.34f };
            m.tube( top, Vector3{ top.x, 0.0f, top.z + sz * 0.04f }, b.m_legR, 5, b.m_legsCol );
        }
    const Vector3 n0 = { 0, y + b.m_height * 0.35f, b.m_length * 0.42f };
    const Vector3 n1 = { 0, n0.y + std::sin( b.m_neckUp ) * b.m_neck, n0.z + std::cos( b.m_neckUp ) * b.m_neck };
    m.tube( n0, n1, b.m_headR * 0.85f, 6, b.m_headCol );
    m.blob( Vector3{ 0, n1.y, n1.z + b.m_head * 0.35f }, Vector3{ b.m_headR * 0.8f, b.m_headR, b.m_head * 0.5f },
            b.m_headCol, b.m_seed + 7, 0.1f );
    if ( b.m_tail > 0.0f )
        m.tube( Vector3{ 0, y + b.m_height * 0.3f, -b.m_length * 0.48f },
                Vector3{ 0, y + b.m_height * 0.3f - b.m_tail, -b.m_length * 0.56f }, 0.04f, 4, b.m_legsCol );
    return body;
}

/**
 * @brief A colour of the shape language: RRGGBB, or A, B, C for the key colours, each with an optional ":shade".
 * @param[in]  word  the word
 * @param[out] ok    false if it is no colour
 * @return the colour
 */
Color shapeColor( const std::string &word, bool &ok )
{
    const size_t      colon = word.find( ':' );
    const std::string base = word.substr( 0, colon );
    const float       shade = colon == std::string::npos ? 1.0f : std::strtof( word.c_str() + colon + 1, nullptr );
    ok = true;
    if ( base == "A" ) return keyA( std::clamp( shade, 0.01f, 1.0f ) );
    if ( base == "B" ) return keyB( std::clamp( shade, 0.01f, 1.0f ) );
    if ( base == "C" ) return keyC( std::clamp( shade, 0.01f, 1.0f ) );
    char               *end = nullptr;
    const unsigned long v = std::strtoul( base.c_str(), &end, 16 );
    ok = base.size() == 6 && end && *end == 0;
    const Color c =
        rgb( static_cast<int>( v >> 16 ) & 255, static_cast<int>( v >> 8 ) & 255, static_cast<int>( v ) & 255 );
    return shade == 1.0f ? c : scaleColor( c, shade );
}

/**
 * @brief A material of the shape language.
 * @param[in]  word  glow, lit, glass, shiny or water
 * @param[out] mat   the material
 * @return false if it is none of them
 */
bool shapeMat( const std::string &word, Mat &mat )
{
    const std::pair<const char *, Mat> mats[] = {
        { "glow", Glow }, { "lit", Lit }, { "glass", Glass }, { "shiny", Shiny }, { "water", Water }
    };
    for ( const auto &[name, m] : mats )
        if ( word == name )
        {
            mat = m;
            return true;
        }
    return false;
}

/**
 * @brief A dome: a half sphere of stacked rings.
 * @param[out] m      where the triangles go
 * @param[in]  base   centre of its rim
 * @param[in]  r      radius
 * @param[in]  h      height
 * @param[in]  col    colour
 * @param[in]  mat    material
 */
void dome( MeshBuilder &m, Vector3 base, float r, float h, Color col, Mat mat )
{
    const int rings = 5;
    for ( int i = 0; i < rings; ++i )
    {
        const float a0 = 0.25f * Tau * static_cast<float>( i ) / rings,
                    a1 = 0.25f * Tau * static_cast<float>( i + 1 ) / rings;
        const float y0 = std::sin( a0 ) * h, y1 = std::sin( a1 ) * h;
        m.cylinder( Vector3{ base.x, base.y + y0, base.z }, std::cos( a0 ) * r, std::cos( a1 ) * r, y1 - y0, 16,
                    i % 2 ? scaleColor( col, 0.97f ) : col, mat );
    }
}

}   // namespace

int buildDataShape( MeshBuilder &m, const DataShape &shape )
{
    int      errors = 0;
    Vector3  origin = { 0, 0, 0 };
    int      repeat = 1, ring = 0, ringAxis = 0;   // ringAxis 1: about y (ring), 2: about x (vring)
    Vector3  step = { 0, 0, 0 };
    float    ringRadius = 0.0f, ringPhase = 0.0f;
    bool     mirrorZ = false, mirrorX = false;
    uint64_t seed = 0;
    for ( char ch : shape.m_name ) seed = hash64( seed * 131 + static_cast<unsigned char>( ch ) );
    for ( const std::string &line : shape.m_lines )
    {
        std::vector<std::string> w = splitLine( line, ' ' );
        const std::string        op = w.front();
        // Numbers first, then a colour or two, then a material.
        Mat mat{};
        if ( w.size() > 1 && shapeMat( w.back(), mat ) ) w.pop_back();
        auto f = [&]( size_t i ) { return i < w.size() ? std::strtof( w[i].c_str(), nullptr ) : 0.0f; };
        auto need = [&]( size_t n )
        {
            if ( w.size() >= n ) return true;
            ++errors;
            return false;
        };
        auto col = [&]( size_t i )
        {
            bool        ok = false;
            const Color c = i < w.size() ? shapeColor( w[i], ok ) : rgb( 255, 0, 255 );
            if ( !ok ) ++errors;
            return c;
        };
        if ( op == "at" && need( 4 ) )
        {
            origin = { f( 1 ), f( 2 ), f( 3 ) };
            continue;
        }
        if ( op == "repeat" && need( 5 ) )
        {
            repeat = std::max( 1, static_cast<int>( f( 1 ) ) );
            step = { f( 2 ), f( 3 ), f( 4 ) };
            continue;
        }
        if ( ( op == "ring" || op == "vring" ) && need( 3 ) )
        {
            ring = std::max( 1, static_cast<int>( f( 1 ) ) );
            ringAxis = op == "ring" ? 1 : 2;
            ringRadius = f( 2 );
            ringPhase = f( 3 ) * Tau / 360.0f;
            continue;
        }
        if ( op == "mirror" || op == "mirrorx" )
        {
            ( op == "mirror" ? mirrorZ : mirrorX ) = true;
            continue;
        }
        if ( op == "at" || op == "repeat" || op == "ring" || op == "vring" ) continue;

        // The copies this line makes: repeated, turned round a ring (its points pushed out by the radius, then turned
        // about the origin), mirrored.
        struct Copy
        {
            Vector3 m_off;      ///< repeat offset
            float   m_angle;    ///< ring angle
            int     m_mirror;   ///< 1 z, 2 x, 3 both
        };
        std::vector<Copy> copies;
        const int         n = ring > 0 ? ring : repeat;
        for ( int k = 0; k < n; ++k )
        {
            const float   a = ringPhase + Tau * static_cast<float>( k ) / static_cast<float>( std::max( 1, ring ) );
            const Vector3 off = ring > 0 ? Vector3{ 0, 0, 0 } : Vector3Scale( step, static_cast<float>( k ) );
            copies.push_back( { off, a, 0 } );
            if ( mirrorZ ) copies.push_back( { off, a, 1 } );
            if ( mirrorX ) copies.push_back( { off, a, 2 } );
            if ( mirrorZ && mirrorX ) copies.push_back( { off, a, 3 } );
        }
        const int   axis = ring > 0 ? ringAxis : 0;
        const float radius = ringRadius;
        repeat = 1;
        ring = 0;
        mirrorZ = mirrorX = false;
        for ( const Copy &copy : copies )
        {
            const Vector3 off = copy.m_off;
            const int     mirror = copy.m_mirror;
            auto          p = [&]( size_t i )
            {
                Vector3     v = { f( i ), f( i + 1 ), f( i + 2 ) };
                const float cs = std::cos( copy.m_angle ), sn = std::sin( copy.m_angle );
                if ( axis == 1 )
                {
                    v.x += radius;
                    v = Vector3{ v.x * cs - v.z * sn, v.y, v.x * sn + v.z * cs };
                }
                else if ( axis == 2 )
                {
                    v.y += radius;
                    v = Vector3{ v.x, v.y * cs - v.z * sn, v.y * sn + v.z * cs };
                }
                v = Vector3Add( Vector3Add( v, origin ), off );
                if ( mirror & 1 ) v.z = -v.z;
                if ( mirror & 2 ) v.x = -v.x;
                return v;
            };
            ++seed;
            if ( op == "box" && need( 8 ) )
                m.box( p( 1 ), Vector3{ f( 4 ), f( 5 ), f( 6 ) }, col( 7 ), mat );
            else if ( op == "tbox" && need( 12 ) )
                m.taperedBox( p( 1 ), Vector2{ f( 4 ), f( 5 ) }, p( 6 ), Vector2{ f( 9 ), f( 10 ) }, col( 11 ), mat );
            else if ( op == "cyl" && need( 9 ) )
                m.cylinder( p( 1 ), f( 4 ), f( 5 ), f( 6 ), std::max( 3, static_cast<int>( f( 7 ) ) ), col( 8 ), mat );
            else if ( op == "spire" && need( 8 ) )
                m.cylinder( p( 1 ), f( 4 ), 0.0f, f( 5 ), std::max( 3, static_cast<int>( f( 6 ) ) ), col( 7 ), mat );
            else if ( op == "tube" && need( 10 ) )
                m.tube( p( 1 ), p( 4 ), f( 7 ), std::max( 3, static_cast<int>( f( 8 ) ) ), col( 9 ), mat, true );
            else if ( op == "blob" && need( 8 ) )
                m.blob( p( 1 ), Vector3{ f( 4 ), f( 5 ), f( 6 ) }, col( 7 ), static_cast<unsigned>( seed ),
                        w.size() > 8 ? f( 8 ) : 0.0f, mat );
            else if ( op == "dome" && need( 6 ) )
                dome( m, p( 1 ), f( 4 ), w.size() > 6 ? f( 5 ) : f( 4 ), col( w.size() > 6 ? 6 : 5 ), mat );
            else if ( op == "quad" && need( 14 ) )
                m.quad( p( 1 ), p( 4 ), p( 7 ), p( 10 ), col( 13 ), mat );
            else if ( op == "tri" && need( 11 ) )
                m.tri( p( 1 ), p( 4 ), p( 7 ), col( 10 ), mat );
            else if ( op == "gable" && need( 10 ) )
                gable( m, p( 1 ), f( 4 ), f( 5 ), f( 6 ), f( 7 ), col( 8 ), col( 9 ) );
            else if ( op == "hip" && need( 9 ) )
                hip( m, p( 1 ), f( 4 ), f( 5 ), f( 6 ), f( 7 ), col( 8 ) );
            else if ( op == "fronds" && need( 8 ) )
                fronds( m, p( 1 ), static_cast<int>( f( 4 ) ), f( 5 ), f( 6 ), col( 7 ),
                        0.3f * static_cast<float>( seed % 7 ) );
            else if ( op == "merlons" && need( 9 ) )
                merlons( m, p( 1 ), p( 4 ), f( 7 ), col( 8 ) );
            else if ( op == "windows" && need( 8 ) )
            {   // windows x y z ax az width height [lit] [storey]: a facade's windows, facing away from the axis's left
                Vector3 across = Vector3Normalize( Vector3{ f( 4 ), 0, f( 5 ) } );
                Vector3 out = { across.z, 0, -across.x };
                if ( mirror & 1 ) across.z = -across.z, out.z = -out.z;
                if ( mirror & 2 ) across.x = -across.x, out.x = -out.x;
                panes( m, p( 1 ), across, out, f( 6 ), f( 7 ), seed, w.size() > 8 ? f( 8 ) : 0.4f,
                       w.size() > 9 ? f( 9 ) : 3.0f, 1.0f );
            }
            else if ( op == "boxwindows" && need( 7 ) )
                boxPanes( m, p( 1 ), f( 4 ), f( 5 ), f( 6 ), seed, w.size() > 7 ? f( 7 ) : 0.4f,
                          w.size() > 8 ? f( 8 ) : 3.0f );
            else if ( op == "arch" && need( 9 ) )
            {   // arch x y z span rise radius segments COLOUR: legs along z, its face to the road
                const Vector3 c = p( 1 );
                const float   span = f( 4 ), rise = f( 5 ), r = f( 6 );
                const int     segs = std::max( 2, static_cast<int>( f( 7 ) ) );
                const Color   k = col( 8 );
                for ( int i = 0; i < segs; ++i )
                {
                    auto at = [&]( int j )
                    {
                        const float t = static_cast<float>( j ) / static_cast<float>( segs ), u = 2.0f * t - 1.0f;
                        return Vector3{ c.x, c.y + rise * ( 1.0f - u * u ), c.z + span * 0.5f * u };
                    };
                    m.tube( at( i ), at( i + 1 ),
                            r * ( 1.0f - 0.35f * ( 1.0f - std::abs( 2.0f * ( i + 0.5f ) / segs - 1.0f ) ) ), 6, k, mat,
                            true );
                }
            }
            else if ( op == "text" && need( 7 ) )
            {   // text x y z cap COLOUR WORDS: painted letters on a face towards the road (+x), centred on x y z
                std::string words;
                for ( size_t i = 6; i < w.size(); ++i ) words += ( i > 6 ? " " : "" ) + w[i];
                std::vector<std::pair<Vector2, Vector2>> strokes;
                const float   cap = f( 4 ), width = textStrokes( words, strokes ) * cap, thick = cap * 0.16f;
                const Vector3 c = p( 1 );
                const Color   k = col( 5 );
                for ( const auto &[a, b] : strokes )
                {
                    // Read from the road, the letters run along -z.
                    const Vector3 s0 = { c.x, c.y + ( a.y - 0.5f ) * cap, c.z + width * 0.5f - a.x * cap };
                    const Vector3 s1 = { c.x, c.y + ( b.y - 0.5f ) * cap, c.z + width * 0.5f - b.x * cap };
                    Vector3       d = Vector3Subtract( s1, s0 );
                    const float   len = Vector3Length( d );
                    d = len > 1e-4f ? Vector3Scale( d, 1.0f / len ) : Vector3{ 0, 0, -1 };
                    const Vector3 side = { 0.0f, -d.z * thick * 0.5f, d.y * thick * 0.5f },
                                  e0 = Vector3Scale( d, thick * 0.4f );
                    const Vector3 q0 = Vector3Subtract( Vector3Subtract( s0, side ), e0 ),
                                  q1 = Vector3Add( Vector3Subtract( s1, side ), e0 );
                    const Vector3 q2 = Vector3Add( Vector3Add( s1, side ), e0 ),
                                  q3 = Vector3Subtract( Vector3Add( s0, side ), e0 );
                    m.quad( q0, q1, q2, q3, k, mat.m_emissive > 0.0f ? mat : Mat{ 0.25f, 0.0f } );
                    m.quad( q3, q2, q1, q0, k, mat.m_emissive > 0.0f ? mat : Mat{ 0.25f, 0.0f } );
                }
            }
            else if ( ( op == "hoop" || op == "halo" ) && need( 8 ) )
            {   // hoop x y z r thickness segments COLOUR: a circle facing the road (a wheel); halo: lying flat
                const Vector3 c = p( 1 );
                const float   r = f( 4 ), th = f( 5 );
                const int     segs = std::max( 6, static_cast<int>( f( 6 ) ) );
                const Color   k = col( 7 );
                auto          at = [&]( int j )
                {
                    const float t = Tau * static_cast<float>( j ) / static_cast<float>( segs );
                    return op == "hoop" ? Vector3{ c.x, c.y + r * std::sin( t ), c.z + r * std::cos( t ) }
                                        : Vector3{ c.x + r * std::cos( t ), c.y, c.z + r * std::sin( t ) };
                };
                for ( int i = 0; i < segs; ++i ) m.tube( at( i ), at( i + 1 ), th, 5, k, mat, false );
            }
            else if ( op != "box" && op != "tbox" && op != "cyl" && op != "spire" && op != "tube" && op != "blob" &&
                      op != "dome" && op != "quad" && op != "tri" && op != "gable" && op != "hip" && op != "fronds" &&
                      op != "merlons" && op != "windows" && op != "boxwindows" && op != "arch" && op != "hoop" &&
                      op != "halo" && op != "text" )
                ++errors;
        }
    }
    return errors;
}

namespace
{

}   // namespace

void Models::buildDataLandmarks()
{
    const size_t k = static_cast<size_t>( PropKindE::LANDMARK );
    m_radius[k] = 0.0f;
    m_facing[k] = FacingE::ROAD;
    for ( const DataShape &shape : dataShapes() )
    {
        m_props[k].emplace_back();
        const int errors = buildDataShape( m_props[k].back(), shape );
        if ( errors ) TraceLog( LOG_WARNING, "SUNSTREAK: landmark %s: %d broken lines", shape.m_name.c_str(), errors );
    }
}

void Models::reloadDataShapes()
{
    m_props[static_cast<size_t>( PropKindE::LANDMARK )].resize( static_cast<size_t>( compiledLandmarks() ) );
    buildDataLandmarks();
    buildFarProps();
    for ( size_t i = m_creatureFirst; i < m_traffic.size(); ++i ) UnloadMesh( m_traffic[i].m_mesh );
    m_traffic.resize( m_creatureFirst );
    buildCreatures();
    buildFlags();
}

void Models::buildWorldProps()
{
    auto add = [&]( PropKindE kind, float radius, FacingE facing = FacingE::FREE ) -> MeshBuilder &
    {
        const size_t k = static_cast<size_t>( kind );
        m_radius[k] = radius;
        m_facing[k] = facing;
        m_props[k].emplace_back();
        return m_props[k].back();
    };
    auto keyed = [&]( PropKindE kind, float radius, FacingE facing, Color a, Color b = rgb( 128, 128, 128 ),
                      Color c = rgb( 34, 40, 54 ) ) -> MeshBuilder &
    {
        m_keys[static_cast<size_t>( kind )] = { a, b, c };
        return add( kind, radius, facing );
    };
    const Color bark = rgb( 96, 72, 52 );
    const Color darkWood = rgb( 70, 50, 36 );
    const Color white = rgb( 236, 234, 226 );
    const Color steel = rgb( 140, 144, 150 );
    const Color slate = rgb( 70, 72, 80 );

    // ---- Trees and plants -------------------------------------------------------------------------------------------
    for ( int v = 0; v < 3; ++v )
    {   // Olive: a twisted low trunk, silver-green crowns.
        MeshBuilder &m = add( PropKindE::OLIVE, 0.6f );
        const float  lean = 0.3f * static_cast<float>( v ) - 0.3f;
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ lean, 1.2f, 0.2f }, 0.32f, 6, rgb( 110, 98, 84 ) );
        m.tube( Vector3{ lean, 1.2f, 0.2f }, Vector3{ lean - 0.6f, 2.2f, 0 }, 0.2f, 5, rgb( 110, 98, 84 ) );
        m.tube( Vector3{ lean, 1.2f, 0.2f }, Vector3{ lean + 0.7f, 2.1f, 0.3f }, 0.2f, 5, rgb( 110, 98, 84 ) );
        const Color leaf = rgb( 118 + v * 6, 138, 100 );
        m.blob( Vector3{ lean - 0.6f, 2.8f, 0 }, Vector3{ 1.6f, 1.0f, 1.5f }, leaf, 601 + v, 0.55f );
        m.blob( Vector3{ lean + 0.8f, 2.7f, 0.3f }, Vector3{ 1.5f, 0.9f, 1.4f }, scaleColor( leaf, 1.1f ), 611 + v,
                0.55f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Lombardy poplar.
        MeshBuilder &m = add( PropKindE::POPLAR, 0.4f );
        const float  h = 15.0f + 3.0f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.25f, 0.15f, 3.0f, 5, bark );
        m.blob( Vector3{ 0, h * 0.55f, 0 }, Vector3{ 1.4f, h * 0.45f, 1.3f }, rgb( 70, 112 + 8 * v, 52 ), 621 + v,
                0.35f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Stone pine: a bare trunk and a flat umbrella.
        MeshBuilder &m = add( PropKindE::UMBRELLA_PINE, 0.6f );
        const float  h = 9.0f + 2.0f * static_cast<float>( v );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.8f - 1.6f * v, h, 0.3f }, 0.32f, 6, rgb( 120, 88, 64 ) );
        m.blob( Vector3{ 0.8f - 1.6f * v, h + 0.6f, 0.3f }, Vector3{ 5.5f, 1.5f, 5.0f }, rgb( 48, 88, 50 ), 631 + v,
                0.35f );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Spruce: slim, dark, many tiers.
        MeshBuilder &m = add( PropKindE::SPRUCE, 0.5f );
        const float  k = 1.0f + 0.15f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.2f, 0.15f, 1.2f, 5, darkWood );
        for ( int i = 0; i < 4; ++i )
        {
            const float f = static_cast<float>( i );
            m.cylinder( Vector3{ 0, ( 0.8f + 2.4f * f ) * k, 0 }, ( 1.7f - 0.33f * f ) * k, 0.0f, 3.6f * k, 7,
                        rgb( 26 + 3 * i, 62 + 4 * i, 44 ), {}, 0.4f * f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Baobab: the thick bottle trunk, short branches, few leaves.
        MeshBuilder &m = add( PropKindE::BAOBAB, 1.8f );
        const float  h = 9.0f + 2.0f * static_cast<float>( v );
        const Color  trunk = rgb( 150, 128, 108 );
        m.cylinder( Vector3{ 0, 0, 0 }, 1.9f, 1.4f, h, 9, trunk );
        for ( int i = 0; i < 6; ++i )
        {
            const float   a = Tau * static_cast<float>( i ) / 6.0f + 0.5f * v;
            const Vector3 tip = { std::cos( a ) * 3.2f, h + 2.0f + 0.6f * std::sin( a * 3.0f ), std::sin( a ) * 3.2f };
            m.tube( Vector3{ 0, h - 0.3f, 0 }, tip, 0.3f, 5, trunk );
            if ( v == 0 ) m.blob( tip, Vector3{ 1.2f, 0.6f, 1.2f }, rgb( 92, 122, 60 ), 641 + i, 0.5f );
        }
    }
    for ( int v = 0; v < 3; ++v )
    {   // Eucalyptus: a pale leaning trunk, a thin blue-green crown.
        MeshBuilder &m = add( PropKindE::EUCALYPTUS, 0.5f );
        const float  h = 13.0f + 2.0f * static_cast<float>( v );
        const Color  trunk = rgb( 222, 214, 200 );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.6f * v - 0.6f, h * 0.6f, 0 }, 0.3f, 6, trunk );
        m.tube( Vector3{ 0.6f * v - 0.6f, h * 0.6f, 0 }, Vector3{ 0.6f * v - 1.6f, h, 0.6f }, 0.2f, 5, trunk );
        m.tube( Vector3{ 0.6f * v - 0.6f, h * 0.6f, 0 }, Vector3{ 0.6f * v + 0.8f, h * 0.95f, -0.4f }, 0.18f, 5,
                trunk );
        const Color leaf = rgb( 104, 138, 116 );
        m.blob( Vector3{ 0.6f * v - 1.6f, h + 0.5f, 0.6f }, Vector3{ 2.2f, 1.4f, 2.0f }, leaf, 651 + v, 0.6f );
        m.blob( Vector3{ 0.6f * v + 0.8f, h + 0.2f, -0.4f }, Vector3{ 1.8f, 1.2f, 1.8f }, scaleColor( leaf, 1.08f ),
                661 + v, 0.6f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Joshua tree: forked arms with spiky tufts.
        MeshBuilder &m = add( PropKindE::JOSHUA_TREE, 0.4f );
        const Color  trunk = rgb( 120, 104, 84 ), tuft = rgb( 96, 116, 64 );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 2.6f, 0 }, 0.25f, 6, trunk );
        for ( int i = 0; i < 3 + v; ++i )
        {
            const float   a = Tau * static_cast<float>( i ) / static_cast<float>( 3 + v ) + 0.7f * v;
            const Vector3 mid = { std::cos( a ) * 1.0f, 3.6f, std::sin( a ) * 1.0f };
            const Vector3 tip = { std::cos( a ) * 1.5f, 4.6f + 0.4f * i, std::sin( a ) * 1.5f };
            m.tube( Vector3{ 0, 2.6f, 0 }, mid, 0.18f, 5, trunk );
            m.tube( mid, tip, 0.15f, 5, trunk );
            m.blob( tip, Vector3{ 0.5f, 0.55f, 0.5f }, tuft, 671 + i, 0.9f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Agave: a rosette of stiff blue leaves.
        MeshBuilder &m = add( PropKindE::AGAVE, 0.0f );
        const Color  leaf = rgb( 96, 150, 150 - v * 10 );
        for ( int i = 0; i < 12; ++i )
        {
            const float   a = Tau * static_cast<float>( i ) / 12.0f + 0.3f * ( i % 2 );
            const float   r = i % 2 ? 0.9f : 1.2f;
            const Vector3 tip = { std::cos( a ) * r, i % 2 ? 1.2f : 0.7f, std::sin( a ) * r };
            const Vector3 s = { -std::sin( a ) * 0.18f, 0, std::cos( a ) * 0.18f };
            m.tri( Vector3Add( Vector3{ 0, 0.15f, 0 }, s ), tip, Vector3Subtract( Vector3{ 0, 0.15f, 0 }, s ),
                   i % 2 ? leaf : scaleColor( leaf, 0.85f ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Prickly pear: pads on pads, a few flowers.
        MeshBuilder  &m = add( PropKindE::PRICKLY_PEAR, 0.5f );
        const Color   pad = rgb( 92, 132, 70 );
        const Vector3 pads[] = {
            { 0, 0.6f, 0 }, { 0.5f, 1.4f, 0.1f }, { -0.5f, 1.3f, -0.1f }, { 0.2f, 2.1f, 0 }, { -0.8f, 2.0f, 0.2f }
        };
        for ( int i = 0; i < 4 + v; ++i )
            m.blob( pads[i], Vector3{ 0.45f, 0.55f, 0.12f }, scaleColor( pad, 0.9f + 0.05f * i ), 681 + i, 0.2f );
        m.blob( Vector3{ 0.2f, 2.65f, 0 }, Vector3{ 0.1f, 0.1f, 0.1f }, rgb( 240, 200, 60 ), 691, 0.2f );
        m.blob( Vector3{ 0.7f, 1.95f, 0.1f }, Vector3{ 0.1f, 0.1f, 0.1f }, rgb( 220, 70, 90 ), 692, 0.2f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Banana: a soft trunk and huge drooping leaves.
        MeshBuilder &m = add( PropKindE::BANANA, 0.3f );
        const float  h = 3.0f + 0.6f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.22f, 0.16f, h, 6, rgb( 110, 130, 70 ) );
        fronds( m, Vector3{ 0, h, 0 }, 7, 2.6f, 1.0f, rgb( 90, 160, 56 ), 0.5f * v );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Lebanon cedar: flat tiers.
        MeshBuilder &m = add( PropKindE::CEDAR, 0.8f );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 12.0f, 0 }, 0.5f, 6, bark );
        for ( int i = 0; i < 4; ++i )
        {
            const float f = static_cast<float>( i ), w = 5.5f - 1.1f * f;
            m.blob( Vector3{ 0.4f * ( v ? f : -f ) * 0.3f, 3.5f + 2.6f * f, 0 }, Vector3{ w, 0.7f, w * 0.85f },
                    rgb( 40 + 4 * i, 78 + 4 * i, 52 ), 701 + i + 10 * v, 0.4f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Redwood: a huge red trunk, the crown high above.
        MeshBuilder &m = add( PropKindE::REDWOOD, 2.6f );
        const float  h = 34.0f + 6.0f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, 0, 0 }, 2.4f, 1.2f, h, 9, rgb( 138, 70, 50 ) );
        for ( int i = 0; i < 4; ++i )
        {
            const float f = static_cast<float>( i );
            m.cylinder( Vector3{ 0, h * 0.5f + 5.0f * f, 0 }, 4.0f - 0.7f * f, 0.0f, 9.0f, 7,
                        rgb( 34 + 4 * i, 72 + 3 * i, 46 ), {}, 0.5f * f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Quindio wax palm: a very tall pale stem.
        MeshBuilder &m = add( PropKindE::WAX_PALM, 0.4f );
        const float  h = 30.0f + 8.0f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.35f, 0.25f, h, 6, rgb( 196, 196, 186 ) );
        fronds( m, Vector3{ 0, h, 0 }, 9, 3.6f, 1.6f, rgb( 70, 120, 64 ), 0.4f * v );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Dragon's blood tree: a fan of branches holding a dense umbrella.
        MeshBuilder &m = add( PropKindE::DRAGON_TREE, 0.6f );
        const Color  trunk = rgb( 160, 150, 132 );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.6f, 0.45f, 2.6f, 7, trunk );
        for ( int i = 0; i < 8; ++i )
        {
            const float a = Tau * static_cast<float>( i ) / 8.0f + 0.3f * v;
            m.tube( Vector3{ 0, 2.5f, 0 }, Vector3{ std::cos( a ) * 2.6f, 4.6f, std::sin( a ) * 2.6f }, 0.16f, 4,
                    trunk );
        }
        m.blob( Vector3{ 0, 5.2f, 0 }, Vector3{ 4.2f, 1.1f, 4.0f }, rgb( 54, 92, 52 ), 711 + v, 0.25f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Tree fern.
        MeshBuilder &m = add( PropKindE::TREE_FERN, 0.3f );
        const float  h = 4.0f + 1.5f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.3f, 0.22f, h, 6, rgb( 70, 54, 40 ) );
        fronds( m, Vector3{ 0, h, 0 }, 10, 2.8f, 0.8f, rgb( 74, 132, 56 ), 0.3f * v );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Lupins: a clump of flower spikes.
        MeshBuilder &m = keyed( PropKindE::LUPIN, 0.0f, FacingE::FREE, rgb( 150, 90, 200 ) );
        for ( int i = 0; i < 6; ++i )
        {
            const float a = Tau * static_cast<float>( i ) / 6.0f + v, r = 0.25f + 0.15f * ( i % 2 );
            const float h = 0.9f + 0.25f * ( ( i + v ) % 3 );
            m.cylinder( Vector3{ std::cos( a ) * r, 0, std::sin( a ) * r }, 0.03f, 0.03f, h * 0.5f, 4,
                        rgb( 70, 120, 60 ) );
            m.cylinder( Vector3{ std::cos( a ) * r, h * 0.5f, std::sin( a ) * r }, 0.12f, 0.02f, h * 0.6f, 5,
                        keyA( 0.8f + 0.2f * ( i % 2 ) ) );
        }
        m.blob( Vector3{ 0, 0.25f, 0 }, Vector3{ 0.5f, 0.25f, 0.5f }, rgb( 76, 124, 64 ), 721 + v, 0.5f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // A flowering bush (hydrangea, bougainvillea, rhododendron).
        MeshBuilder &m = keyed( PropKindE::FLOWER_BUSH, 0.0f, FacingE::FREE, rgb( 230, 100, 170 ) );
        m.blob( Vector3{ 0, 0.8f, 0 }, Vector3{ 1.2f, 0.85f, 1.1f }, rgb( 60, 104, 50 ), 731 + v, 0.5f );
        for ( int i = 0; i < 7; ++i )
        {
            const float a = Tau * static_cast<float>( i ) / 7.0f + v;
            m.blob( Vector3{ std::cos( a ) * 0.9f, 0.9f + 0.5f * ( i % 3 ) * 0.5f, std::sin( a ) * 0.8f },
                    Vector3{ 0.35f, 0.3f, 0.35f }, keyA( 0.85f + 0.15f * ( i % 2 ) ), 741 + i, 0.3f );
        }
    }
    for ( int v = 0; v < 3; ++v )
    {   // A low bush in the biome's colour (heather, sagebrush, fynbos, spinifex).
        MeshBuilder &m = keyed( PropKindE::SHRUB, 0.0f, FacingE::FREE, rgb( 84, 112, 60 ) );
        m.blob( Vector3{ 0, 0.4f, 0 }, Vector3{ 0.9f + 0.2f * v, 0.5f, 0.8f + 0.1f * v }, keyA( 0.9f + 0.05f * v ),
                751 + v, 0.6f );
        m.blob( Vector3{ 0.5f, 0.3f, 0.3f }, Vector3{ 0.5f, 0.35f, 0.5f }, keyA( 0.8f ), 761 + v, 0.6f );
    }
    for ( int v = 0; v < 3; ++v )
    {   // A round tree in the biome's colour (maples, jacarandas, flame trees).
        MeshBuilder &m = keyed( PropKindE::BROADLEAF, 0.6f, FacingE::FREE, rgb( 70, 128, 50 ) );
        const float  h = 3.0f + 0.6f * static_cast<float>( v );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.2f * v, h, 0 }, 0.28f, 6, bark );
        m.blob( Vector3{ 0.2f * v, h + 1.6f, 0 }, Vector3{ 2.6f, 2.0f, 2.4f }, keyA( 0.95f ), 771 + v, 0.45f );
        m.blob( Vector3{ 1.0f, h + 2.4f, 0.6f }, Vector3{ 1.6f, 1.3f, 1.5f }, keyA( 1.0f ), 781 + v, 0.45f );
        m.blob( Vector3{ -0.9f, h + 1.2f, -0.5f }, Vector3{ 1.4f, 1.1f, 1.3f }, keyA( 0.85f ), 791 + v, 0.45f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Vineyard: four rows on posts, parallel to the road.
        MeshBuilder &m = add( PropKindE::VINE_ROW, 0.0f, FacingE::ALONG );
        const Color  vine = v ? rgb( 86, 120, 50 ) : rgb( 72, 110, 46 );
        for ( int r = 0; r < 4; ++r )
        {
            const float x = -3.3f + 2.2f * static_cast<float>( r );
            m.box( Vector3{ x, 0.95f, 0 }, Vector3{ 0.32f, 0.5f, 2.9f }, vine );
            m.box( Vector3{ x, 1.5f, 0 }, Vector3{ 0.22f, 0.12f, 2.8f }, scaleColor( vine, 1.12f ) );
            for ( float z : { -2.9f, 2.9f } )
                m.box( Vector3{ x, 0.8f, z }, Vector3{ 0.05f, 0.8f, 0.05f }, rgb( 120, 100, 80 ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Tea: rows of rounded clipped bushes.
        MeshBuilder &m = add( PropKindE::TEA_ROW, 0.0f, FacingE::ALONG );
        const Color  tea = rgb( 64 + 8 * v, 124, 52 );
        for ( int r = 0; r < 3; ++r )
        {
            const float x = -2.0f + 2.0f * static_cast<float>( r );
            for ( int i = 0; i < 3; ++i )
                m.blob( Vector3{ x, 0.5f, -2.0f + 2.0f * i }, Vector3{ 0.75f, 0.5f, 1.2f },
                        scaleColor( tea, 0.95f + 0.05f * i ), 801 + r * 3 + i, 0.2f );
        }
    }
    for ( int v = 0; v < 3; ++v )
    {   // Date palm: a dense crown with fruit.
        MeshBuilder  &m = add( PropKindE::DATE_PALM, 0.4f );
        const float   h = 8.0f + 2.0f * static_cast<float>( v );
        const Vector3 top = { 0.4f * v - 0.4f, h, 0 };
        m.tube( Vector3{ 0, 0, 0 }, top, 0.32f, 6, rgb( 130, 106, 80 ) );
        fronds( m, top, 14, 3.4f, 1.4f, rgb( 82, 120, 70 ), 0.2f * v );
        fronds( m, Vector3Add( top, Vector3{ 0, 0.4f, 0 } ), 7, 2.4f, -0.8f, rgb( 96, 136, 76 ), 0.5f );
        for ( int i = 0; i < 3; ++i )
        {
            const float a = Tau * i / 3.0f;
            m.blob( Vector3{ top.x + std::cos( a ) * 0.5f, h - 0.5f, std::sin( a ) * 0.5f },
                    Vector3{ 0.25f, 0.4f, 0.25f }, rgb( 200, 120, 40 ), 811 + i, 0.4f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Mangrove: stilt roots under a dark crown.
        MeshBuilder &m = add( PropKindE::MANGROVE, 0.6f );
        const Color  root = rgb( 110, 92, 76 );
        for ( int i = 0; i < 6; ++i )
        {
            const float a = Tau * static_cast<float>( i ) / 6.0f + 0.4f * v;
            m.tube( Vector3{ 0, 1.6f, 0 }, Vector3{ std::cos( a ) * 1.6f, 0.9f, std::sin( a ) * 1.6f }, 0.08f, 4,
                    root );
            m.tube( Vector3{ std::cos( a ) * 1.6f, 0.9f, std::sin( a ) * 1.6f },
                    Vector3{ std::cos( a ) * 2.0f, -0.6f, std::sin( a ) * 2.0f }, 0.07f, 4, root );
        }
        m.tube( Vector3{ 0, 1.6f, 0 }, Vector3{ 0, 4.0f, 0 }, 0.2f, 5, root );
        m.blob( Vector3{ 0, 4.8f, 0 }, Vector3{ 2.6f, 1.6f, 2.4f }, rgb( 46, 92, 50 ), 821 + v, 0.5f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Supertree: a steel funnel grown with plants, a lit crown.
        MeshBuilder &m = add( PropKindE::SUPERTREE, 2.0f );
        const float  h = 25.0f + 8.0f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, 0, 0 }, 1.2f, 2.0f, h * 0.6f, 10, rgb( 120, 70, 110 ) );
        m.cylinder( Vector3{ 0, h * 0.6f, 0 }, 2.0f, 7.5f, h * 0.4f, 12, rgb( 96, 120, 70 ) );
        m.cylinder( Vector3{ 0, h, 0 }, 7.5f, 7.0f, 0.8f, 16, rgb( 220, 120, 220 ), Mat{ 0.45f, 0.0f } );
        for ( int i = 0; i < 8; ++i )
        {
            const float a = Tau * i / 8.0f;
            m.tube( Vector3{ std::cos( a ) * 1.6f, h * 0.35f, std::sin( a ) * 1.6f },
                    Vector3{ std::cos( a ) * 7.2f, h, std::sin( a ) * 7.2f }, 0.18f, 4, rgb( 150, 80, 130 ) );
        }
    }

    // ---- Landforms --------------------------------------------------------------------------------------------------
    for ( int v = 0; v < 4; ++v )
    {   // Karst tower: steep limestone with green on its shoulders.
        MeshBuilder &m = keyed( PropKindE::KARST, 0.0f, FacingE::FREE, rgb( 168, 164, 150 ), rgb( 60, 110, 52 ) );
        const float  h = 34.0f + 10.0f * static_cast<float>( v % 3 ), w = 11.0f + 3.0f * static_cast<float>( v % 2 );
        m.blob( Vector3{ 0, h * 0.35f, 0 }, Vector3{ w, h * 0.45f, w * 0.85f }, keyA( 0.95f ), 901 + v, 0.35f );
        m.blob( Vector3{ 1.0f, h * 0.75f, 0.5f }, Vector3{ w * 0.75f, h * 0.32f, w * 0.65f }, keyA( 1.0f ), 911 + v,
                0.35f );
        m.blob( Vector3{ 0.5f, h * 0.95f, 0.3f }, Vector3{ w * 0.6f, h * 0.12f, w * 0.55f }, keyB( 0.95f ), 921 + v,
                0.5f );
        m.blob( Vector3{ -w * 0.6f, h * 0.5f, 0 }, Vector3{ w * 0.35f, h * 0.15f, w * 0.4f }, keyB( 0.85f ), 931 + v,
                0.5f );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Sea stack.
        MeshBuilder &m = keyed( PropKindE::SEA_STACK, 0.0f, FacingE::FREE, rgb( 206, 170, 120 ) );
        const float  h = 26.0f + 8.0f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, -6.0f, 0 }, 9.0f, 5.0f, h + 6.0f, 7, keyA( 0.9f ), {}, 0.4f * v );
        m.blob( Vector3{ 0.5f, h, 0 }, Vector3{ 5.2f, 2.0f, 5.0f }, keyA( 1.0f ), 941 + v, 0.4f );
        m.cylinder( Vector3{ 0, -6.0f, 0 }, 10.5f, 9.0f, 7.0f, 8, keyA( 0.75f ) );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Fairy chimneys: cones of tuff with dark caps.
        MeshBuilder &m = add( PropKindE::FAIRY_CHIMNEY, 2.0f );
        const Color  tuff = rgb( 222, 200, 170 ), cap = rgb( 120, 96, 78 );
        for ( int i = 0; i < 2 + v; ++i )
        {
            const float x = -4.0f + 4.0f * i, h = 9.0f + 3.0f * ( ( i + v ) % 3 ), r = 2.6f + 0.4f * ( i % 2 );
            m.cylinder( Vector3{ x, 0, 0.8f * i }, r, 0.7f, h, 8, tuff );
            m.blob( Vector3{ x, h + 0.4f, 0.8f * i }, Vector3{ 1.2f, 0.8f, 1.2f }, cap, 951 + i + v * 5, 0.3f );
        }
    }
    for ( int v = 0; v < 3; ++v )
    {   // Sandstone pillar with pines on top.
        MeshBuilder &m = add( PropKindE::PILLAR, 0.0f );
        const float  h = 50.0f + 15.0f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, -4.0f, 0 }, 7.0f, 4.5f, h + 4.0f, 7, rgb( 168, 156, 136 ), {}, 0.6f * v );
        m.blob( Vector3{ 0, h, 0 }, Vector3{ 4.8f, 1.6f, 4.6f }, rgb( 60, 100, 56 ), 961 + v, 0.5f );
        for ( int i = 0; i < 3; ++i )
            m.cylinder( Vector3{ -2.0f + 2.0f * i, h + 0.5f, 0.5f * i }, 0.9f, 0.0f, 3.5f, 6, rgb( 40, 84, 50 ) );
        m.blob( Vector3{ 4.0f, h * 0.55f, 0 }, Vector3{ 1.4f, 1.0f, 1.6f }, rgb( 66, 108, 60 ), 971 + v, 0.5f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Natural arch.
        MeshBuilder &m = keyed( PropKindE::ARCH, 0.0f, FacingE::FREE, rgb( 200, 112, 70 ) );
        const float  span = 22.0f + 8.0f * v, h = 18.0f + 4.0f * v;
        m.taperedBox( Vector3{ -span * 0.5f, 0, 0 }, Vector2{ 5.0f, 4.0f }, Vector3{ -span * 0.45f, h, 0 },
                      Vector2{ 3.6f, 3.0f }, keyA( 0.9f ) );
        m.taperedBox( Vector3{ span * 0.5f, 0, 0 }, Vector2{ 4.6f, 4.0f }, Vector3{ span * 0.45f, h, 0 },
                      Vector2{ 3.4f, 3.0f }, keyA( 0.85f ) );
        for ( int i = 0; i < 6; ++i )
        {
            const float t = static_cast<float>( i ) / 5.0f, x = lerpf( -span * 0.45f, span * 0.45f, t );
            const float y = h + 3.0f * std::sin( t * 3.14159f );
            m.box( Vector3{ x, y + 1.5f, 0 }, Vector3{ span * 0.11f, 2.6f, 2.6f }, keyA( 0.95f ) );
        }
    }
    for ( int v = 0; v < 3; ++v )
    {   // Sand dune: a long ridge.
        MeshBuilder &m = keyed( PropKindE::DUNE, 0.0f, FacingE::FREE, rgb( 222, 156, 92 ) );
        const float  l = 40.0f + 12.0f * v, h = 10.0f + 4.0f * v;
        m.blob( Vector3{ 0, -h * 0.25f, 0 }, Vector3{ l * 0.45f, h, l * 0.3f }, keyA( 1.0f ), 981 + v, 0.12f );
        m.blob( Vector3{ l * 0.25f, -h * 0.4f, l * 0.15f }, Vector3{ l * 0.3f, h * 0.7f, l * 0.25f }, keyA( 0.9f ),
                991 + v, 0.12f );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Kopje: rounded granite boulders piled up.
        MeshBuilder &m = add( PropKindE::KOPJE, 3.0f );
        const Color  granite = rgb( 168, 146, 124 );
        for ( int i = 0; i < 6; ++i )
        {
            const float a = Tau * i / 6.0f + v;
            const float r = 2.0f + 0.8f * ( ( i + v ) % 3 );
            m.blob( Vector3{ std::cos( a ) * 3.0f, r * 0.7f + ( i == 5 ? 2.5f : 0.0f ), std::sin( a ) * 2.5f },
                    Vector3{ r, r * 0.8f, r * 0.9f }, scaleColor( granite, 0.9f + 0.04f * i ), 1001 + i + v * 7,
                    0.15f );
        }
    }
    for ( int v = 0; v < 3; ++v )
    {   // White desert chalk mushroom.
        MeshBuilder &m = add( PropKindE::CHALK_ROCK, 1.5f );
        const float  h = 3.0f + 1.5f * v;
        m.cylinder( Vector3{ 0, 0, 0 }, 1.6f, 0.8f, h, 7, rgb( 230, 224, 210 ) );
        m.blob( Vector3{ 0.3f, h + 0.6f, 0 }, Vector3{ 2.6f + 0.5f * v, 1.2f, 2.2f }, rgb( 244, 240, 230 ), 1011 + v,
                0.35f );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Standing stone.
        MeshBuilder &m = add( PropKindE::MENHIR, 0.6f );
        const float  h = 2.6f + 1.2f * v;
        m.taperedBox( Vector3{ 0, -0.2f, 0 }, Vector2{ 0.7f, 0.45f }, Vector3{ 0.1f * v, h, 0 },
                      Vector2{ 0.35f, 0.25f }, rgb( 150 + 6 * v, 148, 140 ) );
        m.blob( Vector3{ 0.1f * v, h, 0 }, Vector3{ 0.38f, 0.25f, 0.28f }, rgb( 156, 152, 144 ), 1021 + v, 0.3f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Moai.
        MeshBuilder &m = add( PropKindE::MOAI, 1.0f, FacingE::ROAD );
        const Color  tuff = rgb( 98, 88, 80 );
        m.taperedBox( Vector3{ 0, 0, 0 }, Vector2{ 1.0f, 1.3f }, Vector3{ 0, 3.0f, 0 }, Vector2{ 0.9f, 1.1f }, tuff );
        m.taperedBox( Vector3{ 0, 3.0f, 0 }, Vector2{ 0.85f, 0.95f }, Vector3{ 0.1f, 6.2f, 0 }, Vector2{ 0.75f, 0.85f },
                      tuff );
        m.box( Vector3{ 0.85f, 5.0f, 0 }, Vector3{ 0.3f, 0.25f, 0.9f }, scaleColor( tuff, 0.9f ) );
        m.taperedBox( Vector3{ 0.8f, 3.9f, 0 }, Vector2{ 0.35f, 0.3f }, Vector3{ 1.05f, 5.0f, 0 },
                      Vector2{ 0.1f, 0.2f }, scaleColor( tuff, 1.08f ) );
        m.box( Vector3{ 0.82f, 3.5f, 0 }, Vector3{ 0.2f, 0.15f, 0.5f }, scaleColor( tuff, 0.8f ) );
        if ( v == 1 ) m.cylinder( Vector3{ 0.1f, 6.2f, 0 }, 0.85f, 0.8f, 1.1f, 8, rgb( 150, 72, 52 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // A great lone rock (Uluru).
        MeshBuilder &m = keyed( PropKindE::MONOLITH, 0.0f, FacingE::FREE, rgb( 192, 84, 42 ) );
        m.blob( Vector3{ 0, 4.0f, 0 }, Vector3{ 60.0f + 15.0f * v, 26.0f, 30.0f }, keyA( 1.0f ), 1031 + v, 0.15f );
        m.blob( Vector3{ 20.0f, 0.0f, 10.0f }, Vector3{ 30.0f, 18.0f, 18.0f }, keyA( 0.85f ), 1041 + v, 0.2f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Tepui: sheer cliffs, a green top, a waterfall.
        MeshBuilder &m = keyed( PropKindE::TEPUI, 0.0f, FacingE::FREE, rgb( 120, 96, 86 ), rgb( 76, 108, 60 ) );
        const float  w = 70.0f + 20.0f * v, h = 70.0f;
        m.taperedBox( Vector3{ 0, -5.0f, 0 }, Vector2{ w, w * 0.6f }, Vector3{ 0, h, 0 },
                      Vector2{ w * 0.92f, w * 0.55f }, keyA( 0.9f ) );
        m.taperedBox( Vector3{ 0, -5.0f, 0 }, Vector2{ w * 1.25f, w * 0.8f }, Vector3{ 0, h * 0.35f, 0 },
                      Vector2{ w * 1.0f, w * 0.62f }, keyB( 0.7f ) );
        m.blob( Vector3{ 0, h + 1.0f, 0 }, Vector3{ w * 0.9f, 3.0f, w * 0.52f }, keyB( 1.0f ), 1051 + v, 0.3f );
        m.quad( Vector3{ w * 0.93f, h, -3 }, Vector3{ w * 0.93f, h, 3 }, Vector3{ w * 1.05f, h * 0.35f, 4 },
                Vector3{ w * 1.05f, h * 0.35f, -4 }, rgb( 236, 240, 244 ), Mat{ 0.25f, 0.3f } );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Volcano: a wide cone, a snow cap, a crater.
        MeshBuilder &m =
            keyed( PropKindE::VOLCANO_CONE, 0.0f, FacingE::FREE, rgb( 104, 96, 92 ), rgb( 240, 244, 250 ) );
        const float r = 140.0f + 30.0f * v, h = 90.0f + 15.0f * v;
        m.cylinder( Vector3{ 0, -8.0f, 0 }, r, r * 0.28f, h * 0.62f, 16, keyA( 0.95f ) );
        m.cylinder( Vector3{ 0, h * 0.62f - 8.0f, 0 }, r * 0.28f, r * 0.08f, h * 0.38f, 16, keyB( 1.0f ) );
        m.cylinder( Vector3{ 0, h - 8.05f, 0 }, r * 0.08f, r * 0.05f, 0.2f, 12, keyA( 0.5f ) );
        for ( int i = 0; i < 8; ++i )
        {   // Snow running down the gullies.
            const float   a = Tau * i / 8.0f + 0.2f * v;
            const Vector3 top = { std::cos( a ) * r * 0.27f, h * 0.62f - 8.0f, std::sin( a ) * r * 0.27f };
            const Vector3 low = { std::cos( a ) * r * 0.42f, h * 0.48f - 8.0f, std::sin( a ) * r * 0.42f };
            const Vector3 s = { -std::sin( a ) * r * 0.03f, 0, std::cos( a ) * r * 0.03f };
            m.tri( Vector3Add( top, s ), Vector3Subtract( top, s ),
                   Vector3Add( low, Vector3{ std::cos( a ) * 1.0f, 0.6f, std::sin( a ) * 1.0f } ), keyB( 0.95f ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Granite dome (Half Dome) and sugarloaf.
        MeshBuilder &m = keyed( PropKindE::GRANITE_DOME, 0.0f, FacingE::FREE, rgb( 186, 184, 178 ) );
        if ( v == 0 )
        {
            for ( int i = 0; i < 6; ++i )
            {
                const float f = static_cast<float>( i ) / 6.0f, f1 = static_cast<float>( i + 1 ) / 6.0f;
                m.cylinder( Vector3{ 0, 70.0f * ( 1.0f - std::cos( f * 1.5708f ) ), 0 },
                            45.0f * std::cos( f * 1.4f ) + 5.0f, 45.0f * std::cos( f1 * 1.4f ) + 5.0f,
                            70.0f * ( std::cos( f * 1.5708f ) - std::cos( f1 * 1.5708f ) ), 12,
                            keyA( 0.9f + 0.02f * i ) );
            }
            m.box( Vector3{ 30.0f, 30.0f, 0 }, Vector3{ 12.0f, 32.0f, 40.0f }, keyA( 0.8f ) );
        }
        else
        {
            m.blob( Vector3{ 0, 30.0f, 0 }, Vector3{ 28.0f, 48.0f, 24.0f }, keyA( 0.95f ), 1061, 0.1f );
            m.blob( Vector3{ -10.0f, 4.0f, 0 }, Vector3{ 30.0f, 12.0f, 26.0f }, rgb( 66, 104, 58 ), 1062, 0.4f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Round hill (the Chocolate Hills, a hillock in the field).
        MeshBuilder &m = keyed( PropKindE::HILLOCK, 0.0f, FacingE::FREE, rgb( 150, 116, 64 ) );
        const float  r = 16.0f + 4.0f * v, h = 22.0f + 6.0f * v;
        m.cylinder( Vector3{ 0, -2.0f, 0 }, r, r * 0.45f, h * 0.7f, 12, keyA( 0.95f ) );
        m.blob( Vector3{ 0, h * 0.68f - 2.0f, 0 }, Vector3{ r * 0.47f, h * 0.3f, r * 0.47f }, keyA( 1.0f ), 1071 + v,
                0.1f );
        m.cylinder( Vector3{ 0, -2.0f, 0 }, r * 1.15f, r, 2.5f, 12, keyA( 0.85f ) );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Badlands: eroded ridges in bands.
        MeshBuilder &m = keyed( PropKindE::BADLAND, 0.0f, FacingE::FREE, rgb( 206, 178, 146 ), rgb( 168, 112, 90 ) );
        for ( int p = 0; p < 3; ++p )
        {
            const float x = -18.0f + 18.0f * p + 3.0f * v, h = 14.0f + 6.0f * ( ( p + v ) % 3 ), w = 11.0f;
            for ( int b = 0; b < 5; ++b )
            {
                const float y0 = h * b / 5.0f, y1 = h * ( b + 1 ) / 5.0f;
                const float w0 = w * ( 1.0f - 0.85f * b / 5.0f ), w1 = w * ( 1.0f - 0.85f * ( b + 1 ) / 5.0f );
                m.taperedBox( Vector3{ x, y0 - 1.0f, 0 }, Vector2{ w0, w0 * 0.7f }, Vector3{ x, y1 - 1.0f, 0 },
                              Vector2{ w1, w1 * 0.7f }, b % 2 ? keyB( 1.0f ) : keyA( 1.0f ) );
            }
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Geyser: a sinter mound and a column of steam.
        MeshBuilder &m = add( PropKindE::GEYSER, 0.0f );
        m.blob( Vector3{ 0, 0, 0 }, Vector3{ 5.0f, 1.2f, 4.5f }, rgb( 222, 212, 186 ), 1081 + v, 0.3f );
        const float h = 18.0f + 10.0f * v;
        for ( int i = 0; i < 5; ++i )
        {
            const float f = static_cast<float>( i ) / 4.0f;
            m.blob( Vector3{ 0.8f * f, h * f + 2.0f, 0 }, Vector3{ 1.2f + 3.0f * f, h * 0.16f, 1.2f + 3.0f * f },
                    rgb( 244, 246, 248 ), 1091 + i, 0.35f, Mat{ 0.18f, 0.0f } );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Terraced hillside: steps of crop held by walls.
        MeshBuilder &m = keyed( PropKindE::TERRACES, 0.0f, FacingE::ALONG, rgb( 112, 162, 64 ), rgb( 126, 98, 70 ) );
        for ( int s = 0; s < 6; ++s )
        {
            const float f = static_cast<float>( s ), w = 26.0f - 3.6f * f, d = 16.0f - 2.2f * f;
            m.box( Vector3{ 2.0f * f, 1.5f + 3.0f * f - 1.5f, 0 }, Vector3{ w, 1.5f, d }, keyB( 0.9f + 0.02f * s ) );
            m.box( Vector3{ 2.0f * f, 3.0f * f + 0.05f, 0 }, Vector3{ w - 0.4f, 0.08f, d - 0.4f },
                   keyA( 0.9f + 0.03f * ( s % 3 ) ), s % 2 && v ? Water : Mat{} );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // A rock spire with snow (Fitz Roy, the Matterhorn).
        MeshBuilder &m = keyed( PropKindE::PEAK, 0.0f, FacingE::FREE, rgb( 120, 118, 120 ), rgb( 244, 246, 250 ) );
        const float  h = 130.0f + 30.0f * v, r = 60.0f;
        m.cylinder( Vector3{ 0, -10.0f, 0 }, r, r * 0.35f, h * 0.55f, 6, keyA( 0.9f ), {}, 0.3f * v );
        m.cylinder( Vector3{ 0, h * 0.55f - 10.0f, 0 }, r * 0.35f, 2.0f, h * 0.45f, 5, keyA( 1.0f ), {},
                    0.5f + 0.3f * v );
        m.cylinder( Vector3{ 0, h * 0.42f - 10.0f, 0 }, r * 0.52f, r * 0.3f, h * 0.16f, 6, keyB( 1.0f ), {}, 0.3f * v );
        m.cylinder( Vector3{ 0, h * 0.82f - 10.0f, 0 }, r * 0.12f, 1.0f, h * 0.18f, 5, keyB( 0.95f ), {},
                    0.5f + 0.3f * v );
        m.cylinder( Vector3{ r * 0.7f, -10.0f, 0 }, r * 0.5f, 3.0f, h * 0.55f, 5, keyA( 0.85f ), {}, 0.2f );
    }

    // ---- Houses -----------------------------------------------------------------------------------------------------
    for ( int v = 0; v < 3; ++v )
    {   // Cottage: whitewash or stone, a pitched roof, a chimney.
        MeshBuilder &m = keyed( PropKindE::COTTAGE, 4.0f, FacingE::ROAD, rgb( 236, 232, 220 ), rgb( 150, 70, 52 ) );
        const float  hz = 4.5f + 1.0f * v, hx = 3.2f, h = 3.0f + 0.4f * v;
        m.box( Vector3{ 0, h * 0.5f, 0 }, Vector3{ hx, h * 0.5f, hz }, keyA( 1.0f ) );
        gable( m, Vector3{ 0, h, 0 }, hx, hz, 2.6f + 0.6f * ( v == 1 ), 0.4f, keyB( 1.0f ), keyA( 0.95f ) );
        m.box( Vector3{ 0, h + 2.0f, hz - 0.8f }, Vector3{ 0.35f, 1.4f, 0.4f }, keyA( 0.8f ) );
        m.box( Vector3{ hx + 0.02f, 1.0f, 0 }, Vector3{ 0.05f, 1.0f, 0.5f }, rgb( 90 + 40 * v, 50, 40 + 30 * v ) );
        panes( m, Vector3{ hx, 0, -hz }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, hz * 2.0f, h, 1101 + v, 0.6f );
        panes( m, Vector3{ -hx, 0, hz }, Vector3{ 0, 0, -1 }, Vector3{ -1, 0, 0 }, hz * 2.0f, h, 1111 + v, 0.6f );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Cube house with a flat roof: one or two storeys, one with a dome.
        MeshBuilder &m = keyed( PropKindE::FLAT_HOUSE, 4.0f, FacingE::ROAD, rgb( 240, 238, 230 ), rgb( 40, 90, 170 ) );
        const float  h = v == 1 ? 6.4f : 3.4f, hx = 3.4f + 0.4f * v, hz = 3.8f + 0.6f * v;
        m.box( Vector3{ 0, h * 0.5f, 0 }, Vector3{ hx, h * 0.5f, hz }, keyA( 1.0f ) );
        m.box( Vector3{ 0, h + 0.25f, 0 }, Vector3{ hx + 0.1f, 0.25f, hz + 0.1f }, keyA( 0.93f ) );
        m.box( Vector3{ 0, h + 0.05f, 0 }, Vector3{ hx - 0.3f, 0.3f, hz - 0.3f }, keyA( 0.85f ) );
        m.box( Vector3{ hx + 0.03f, 1.05f, -0.8f }, Vector3{ 0.05f, 1.05f, 0.55f }, keyB( 1.0f ) );
        m.box( Vector3{ hx + 0.03f, 2.1f, -0.8f }, Vector3{ 0.06f, 0.06f, 0.75f }, keyB( 0.8f ) );
        panes( m, Vector3{ hx, 0, 0.0f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, hz, h, 1121 + v, 0.5f, 3.0f, 1.1f );
        panes( m, Vector3{ -hx, 0, hz }, Vector3{ 0, 0, -1 }, Vector3{ -1, 0, 0 }, hz * 2.0f, h, 1131 + v, 0.5f, 3.0f,
               1.1f );
        if ( v == 2 )
        {
            m.cylinder( Vector3{ -1.0f, h, 0 }, 1.8f, 1.8f, 0.6f, 10, keyA( 1.0f ) );
            m.blob( Vector3{ -1.0f, h + 0.6f, 0 }, Vector3{ 1.8f, 1.5f, 1.8f }, keyB( 1.0f ), 1141, 0.0f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Alpine chalet: stone ground floor, timber above, a wide low roof, a balcony.
        MeshBuilder &m = keyed( PropKindE::CHALET, 5.0f, FacingE::ROAD, rgb( 150, 98, 58 ), rgb( 92, 70, 54 ) );
        const float  hx = 5.0f + v, hz = 5.5f + v;
        m.box( Vector3{ 0, 1.4f, 0 }, Vector3{ hx, 1.4f, hz }, rgb( 214, 210, 200 ) );
        m.box( Vector3{ 0, 4.3f, 0 }, Vector3{ hx, 1.5f, hz }, keyA( 1.0f ) );
        // The gable faces the road: the ridge runs along x.
        const float   rise = 2.8f;
        const Vector3 c = { 0, 5.8f, 0 };
        m.quad( Vector3{ hx + 1.6f, c.y - 0.6f, -hz - 1.6f }, Vector3{ -hx - 1.6f, c.y - 0.6f, -hz - 1.6f },
                Vector3{ -hx - 1.6f, c.y + rise, 0 }, Vector3{ hx + 1.6f, c.y + rise, 0 }, keyB( 1.0f ) );
        m.quad( Vector3{ hx + 1.6f, c.y - 0.6f, hz + 1.6f }, Vector3{ hx + 1.6f, c.y + rise, 0 },
                Vector3{ -hx - 1.6f, c.y + rise, 0 }, Vector3{ -hx - 1.6f, c.y - 0.6f, hz + 1.6f }, keyB( 1.0f ) );
        for ( float s : { -1.0f, 1.0f } )
            m.tri( Vector3{ s * hx, c.y, -hz }, Vector3{ s * hx, c.y, hz }, Vector3{ s * hx, c.y + rise, 0 },
                   keyA( 0.9f ) );
        m.box( Vector3{ hx + 0.8f, 3.4f, 0 }, Vector3{ 0.8f, 0.08f, hz }, keyA( 0.75f ) );
        m.box( Vector3{ hx + 1.55f, 3.9f, 0 }, Vector3{ 0.05f, 0.5f, hz }, keyA( 0.65f ) );
        for ( int i = 0; i < 5; ++i )
            m.blob( Vector3{ hx + 1.6f, 4.5f, -hz + 1.0f + i * ( hz * 2.0f - 2.0f ) / 4.0f },
                    Vector3{ 0.35f, 0.2f, 0.3f }, rgb( 220, 50, 50 ), 1151 + i, 0.4f );
        panes( m, Vector3{ hx, 0, -hz }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, hz * 2.0f, 5.8f, 1161 + v, 0.6f, 2.8f,
               1.0f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Half-timbered house.
        MeshBuilder &m = keyed( PropKindE::HALF_TIMBER, 4.5f, FacingE::ROAD, rgb( 238, 232, 214 ), rgb( 160, 66, 48 ) );
        const float  hx = 3.6f, hz = 5.0f + 0.8f * v, h = 6.0f;
        const Color  beam = rgb( 76, 54, 40 );
        m.box( Vector3{ 0, h * 0.5f, 0 }, Vector3{ hx, h * 0.5f, hz }, keyA( 1.0f ) );
        gable( m, Vector3{ 0, h, 0 }, hx, hz, 4.2f, 0.4f, keyB( 1.0f ), keyA( 0.96f ) );
        for ( float s : { -1.0f, 1.0f } )
        {
            const float x = s * ( hx + 0.03f );
            for ( float y : { 0.2f, 3.0f, 5.9f } ) m.box( Vector3{ x, y, 0 }, Vector3{ 0.04f, 0.12f, hz }, beam );
            for ( int i = 0; i <= 4; ++i )
                m.box( Vector3{ x, h * 0.5f, -hz + i * hz * 0.5f }, Vector3{ 0.04f, h * 0.5f, 0.12f }, beam );
            for ( int i = 0; i < 4; i += 2 )
            {
                const float z0 = -hz + i * hz * 0.5f, z1 = z0 + hz * 0.5f;
                m.quad( Vector3{ x, 3.0f, z0 }, Vector3{ x, 3.0f, z0 + 0.25f }, Vector3{ x, 5.9f, z1 },
                        Vector3{ x, 5.9f, z1 - 0.25f }, beam );
            }
        }
        panes( m, Vector3{ hx, 0, -hz }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, hz * 2.0f, h, 1171 + v, 0.6f, 2.9f,
               0.9f );
    }
    for ( int v = 0; v < 3; ++v )
    {   // A short terrace: two or three tall narrow houses.
        MeshBuilder &m = keyed( PropKindE::ROW_HOUSES, 6.0f, FacingE::ROAD, rgb( 214, 196, 168 ), rgb( 80, 80, 90 ) );
        const int    n = 2 + ( v == 2 );
        for ( int i = 0; i < n; ++i )
        {
            const float z = ( static_cast<float>( i ) - 0.5f * ( n - 1 ) ) * 4.6f, h = 8.0f + 1.6f * ( ( i + v ) % 3 );
            const Color wall = keyA( i % 2 ? 0.86f : 1.0f );
            m.box( Vector3{ 0, h * 0.5f, z }, Vector3{ 4.0f, h * 0.5f, 2.25f }, wall );
            if ( v == 1 )
            {   // Stepped gable (Flanders, Amsterdam).
                for ( int s = 0; s < 3; ++s )
                    m.box( Vector3{ 2.8f, h + 0.6f + 1.2f * s, z }, Vector3{ 0.25f, 0.6f, 2.0f - 0.6f * s }, wall );
                hip( m, Vector3{ -0.6f, h, z }, 3.2f, 2.2f, 2.6f, 0.1f, keyB( 1.0f ) );
            }
            else
            {
                hip( m, Vector3{ 0, h, z }, 4.0f, 2.25f, 2.0f, 0.15f, keyB( 1.0f ) );
            }
            m.box( Vector3{ 4.03f, h - 0.3f, z }, Vector3{ 0.05f, 0.2f, 2.25f }, rgb( 236, 236, 230 ) );
            m.box( Vector3{ 4.03f, 1.1f, z + 1.0f }, Vector3{ 0.05f, 1.1f, 0.5f }, rgb( 40 + 60 * ( i % 2 ), 50, 60 ) );
            panes( m, Vector3{ 4.0f, 0, z - 2.25f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 4.5f, h, 1181 + i + v * 5,
                   0.55f, 2.9f, 1.2f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Church: a nave and a tower with a spire or a pyramid roof.
        MeshBuilder &m = keyed( PropKindE::CHURCH, 6.0f, FacingE::ROAD, rgb( 238, 234, 224 ), rgb( 74, 78, 88 ) );
        m.box( Vector3{ 0, 4.0f, 2.0f }, Vector3{ 4.5f, 4.0f, 8.0f }, keyA( 1.0f ) );
        gable( m, Vector3{ 0, 8.0f, 2.0f }, 4.5f, 8.0f, 4.0f, 0.3f, keyB( 1.0f ), keyA( 0.97f ) );
        m.box( Vector3{ 0, 8.0f, -8.0f }, Vector3{ 2.4f, 8.0f, 2.4f }, keyA( 0.97f ) );
        if ( v == 0 )
            m.cylinder( Vector3{ 0, 16.0f, -8.0f }, 2.6f, 0.05f, 10.0f, 8, keyB( 1.0f ), {}, 0.39f );
        else
            m.taperedBox( Vector3{ 0, 16.0f, -8.0f }, Vector2{ 2.7f, 2.7f }, Vector3{ 0, 19.5f, -8.0f },
                          Vector2{ 0.05f, 0.05f }, keyB( 1.0f ) );
        m.box( Vector3{ 0, v ? 20.2f : 26.6f, -8.0f }, Vector3{ 0.06f, 0.7f, 0.06f }, rgb( 60, 60, 60 ) );
        m.box( Vector3{ 0, v ? 20.4f : 26.8f, -8.0f }, Vector3{ 0.06f, 0.06f, 0.4f }, rgb( 60, 60, 60 ) );
        m.box( Vector3{ 2.45f, 12.5f, -8.0f }, Vector3{ 0.05f, 1.2f, 0.7f }, rgb( 40, 40, 46 ) );
        for ( int i = 0; i < 4; ++i )
            m.box( Vector3{ 4.53f, 4.5f, -2.0f + 3.0f * i }, Vector3{ 0.05f, 1.6f, 0.5f }, keyC( 0.4f ), Glass );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Orthodox church: white walls, onion domes.
        MeshBuilder &m =
            keyed( PropKindE::ONION_CHURCH, 6.0f, FacingE::ROAD, rgb( 240, 238, 230 ), rgb( 214, 170, 60 ) );
        m.box( Vector3{ 0, 4.5f, 0 }, Vector3{ 5.0f, 4.5f, 6.0f }, keyA( 1.0f ) );
        hip( m, Vector3{ 0, 9.0f, 0 }, 5.0f, 6.0f, 1.2f, 0.3f, rgb( 90, 110, 100 ) );
        auto onion = [&]( Vector3 base, float r, float drum )
        {
            m.cylinder( base, r * 0.7f, r * 0.7f, drum, 10, keyA( 0.95f ) );
            m.blob( Vector3{ base.x, base.y + drum + r * 0.8f, base.z }, Vector3{ r, r, r }, keyB( 1.0f ), 1191, 0.0f,
                    Shiny );
            m.cylinder( Vector3{ base.x, base.y + drum + r * 1.5f, base.z }, r * 0.45f, 0.02f, r * 1.4f, 8,
                        keyB( 0.95f ), Shiny );
            m.box( Vector3{ base.x, base.y + drum + r * 3.2f, base.z }, Vector3{ 0.05f, 0.6f, 0.05f },
                   rgb( 214, 180, 80 ) );
            m.box( Vector3{ base.x, base.y + drum + r * 3.3f, base.z }, Vector3{ 0.05f, 0.05f, 0.35f },
                   rgb( 214, 180, 80 ) );
        };
        onion( Vector3{ 0, 9.6f, 0 }, 2.2f, 3.0f );
        if ( v == 0 )
            for ( float x : { -3.0f, 3.0f } )
                for ( float z : { -3.6f, 3.6f } ) onion( Vector3{ x, 9.4f, z }, 1.1f, 1.6f );
        else
        {
            m.box( Vector3{ 0, 6.0f, -9.0f }, Vector3{ 2.0f, 6.0f, 2.0f }, keyA( 0.97f ) );
            onion( Vector3{ 0, 12.0f, -9.0f }, 1.2f, 1.5f );
        }
        for ( int i = 0; i < 3; ++i )
            m.box( Vector3{ 5.03f, 5.0f, -3.0f + 3.0f * i }, Vector3{ 0.05f, 1.5f, 0.5f }, keyC( 0.4f ), Glass );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Mosque: a hall, a dome, one or two minarets.
        MeshBuilder &m = keyed( PropKindE::MOSQUE, 7.0f, FacingE::ROAD, rgb( 230, 214, 184 ), rgb( 60, 150, 170 ) );
        m.box( Vector3{ 0, 4.0f, 0 }, Vector3{ 8.0f, 4.0f, 8.0f }, keyA( 1.0f ) );
        m.cylinder( Vector3{ 0, 8.0f, 0 }, 5.0f, 5.0f, 1.6f, 14, keyA( 0.95f ) );
        m.blob( Vector3{ 0, 9.6f, 0 }, Vector3{ 5.0f, 4.6f, 5.0f }, keyB( 1.0f ), 1201, 0.0f, Shiny );
        m.cylinder( Vector3{ 0, 14.0f, 0 }, 0.12f, 0.05f, 1.6f, 5, rgb( 214, 180, 80 ) );
        for ( int i = 0; i < 1 + v; ++i )
        {
            const Vector3 b = { -6.0f, 0, i ? 9.5f : -9.5f };
            m.cylinder( b, 1.1f, 0.9f, 22.0f, 10, keyA( 0.97f ) );
            m.cylinder( Vector3{ b.x, 17.0f, b.z }, 1.6f, 1.6f, 0.6f, 10, keyA( 0.9f ) );
            m.cylinder( Vector3{ b.x, 22.0f, b.z }, 1.0f, 0.02f, 3.5f, 10, keyB( 1.0f ), Shiny );
        }
        for ( int i = 0; i < 4; ++i )
        {
            m.box( Vector3{ 8.03f, 3.5f, -5.0f + 3.3f * i }, Vector3{ 0.05f, 1.8f, 0.7f }, keyC( 0.4f ), Glass );
            m.blob( Vector3{ 8.03f, 5.3f, -5.0f + 3.3f * i }, Vector3{ 0.06f, 0.7f, 0.7f }, keyC( 0.4f ), 1211 + i,
                    0.0f );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Stupa: terraces, a white dome, a golden spire.
        MeshBuilder &m = keyed( PropKindE::STUPA, 4.0f, FacingE::FREE, rgb( 240, 238, 232 ), rgb( 214, 168, 60 ) );
        const float  k = v ? 0.7f : 1.0f;
        for ( int i = 0; i < 3; ++i )
            m.box( Vector3{ 0, 0.6f + 1.2f * i, 0 }, Vector3{ 6.0f - 1.2f * i, 0.6f, 6.0f - 1.2f * i }, keyA( 0.9f ) );
        if ( v == 0 )
            m.blob( Vector3{ 0, 3.6f, 0 }, Vector3{ 3.6f, 3.4f, 3.6f }, keyA( 1.0f ), 1221, 0.0f );
        else
            m.cylinder( Vector3{ 0, 3.6f, 0 }, 3.4f, 1.4f, 4.0f, 14, keyA( 1.0f ) );
        m.box( Vector3{ 0, 7.4f, 0 }, Vector3{ 1.0f * k + 0.2f, 0.6f, 1.0f * k + 0.2f }, keyB( 0.9f ) );
        m.cylinder( Vector3{ 0, 8.0f, 0 }, 1.0f * k, 0.08f, 6.5f + 3.0f * v, 10, keyB( 1.0f ), Shiny );
        for ( int i = 0; i < 5; ++i )
            m.cylinder( Vector3{ 0, 8.4f + 1.0f * i, 0 }, 0.95f * k - 0.15f * i + 0.06f, 0.95f * k - 0.15f * i + 0.06f,
                        0.15f, 10, keyB( 0.8f ), Shiny );
    }
    {   // Thai wat: layered red and green roofs with golden edges.
        MeshBuilder &m = add( PropKindE::WAT, 7.0f, FacingE::ROAD );
        const Color  red = rgb( 170, 40, 36 ), green = rgb( 40, 110, 70 ), gold = rgb( 226, 180, 70 );
        m.box( Vector3{ 0, 0.6f, 0 }, Vector3{ 7.0f, 0.6f, 11.0f }, rgb( 230, 226, 214 ) );
        m.box( Vector3{ 0, 3.6f, 0 }, Vector3{ 5.0f, 2.4f, 9.0f }, rgb( 240, 236, 226 ) );
        for ( int i = 0; i < 3; ++i )
        {
            const float y = 6.0f + 1.4f * i, w = 6.0f - 0.9f * i, l = 10.0f - 1.0f * i;
            gable( m, Vector3{ 0, y, 0 }, w, l, 4.5f - 0.8f * i, 0.0f, i % 2 ? green : red, gold );
            for ( float z : { -l, l } )
                m.tri( Vector3{ 0, y + 4.5f - 0.8f * i, z }, Vector3{ 0, y + 6.2f - 0.8f * i, z * 1.06f },
                       Vector3{ 0, y + 4.2f - 0.8f * i, z * 1.02f }, gold );
        }
        for ( int i = 0; i < 4; ++i )
            m.box( Vector3{ 5.03f, 3.6f, -6.0f + 4.0f * i }, Vector3{ 0.05f, 1.6f, 0.7f }, rgb( 150, 30, 30 ) );
        m.cylinder( Vector3{ 0, 0, -16.0f }, 3.0f, 0.1f, 18.0f, 12, gold, Shiny );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Khmer and Hindu temple towers: a stepped base, beehive towers.
        MeshBuilder &m = keyed( PropKindE::TEMPLE_TOWER, 8.0f, FacingE::ROAD, rgb( 150, 140, 118 ) );
        for ( int i = 0; i < 3; ++i )
            m.box( Vector3{ 0, 1.0f + 2.0f * i, 0 }, Vector3{ 16.0f - 3.5f * i, 1.0f, 16.0f - 3.5f * i },
                   keyA( 0.85f + 0.05f * i ) );
        auto tower = [&]( Vector3 base, float s )
        {
            for ( int t = 0; t < 7; ++t )
            {
                const float f = static_cast<float>( t ) / 7.0f;
                m.cylinder( Vector3{ base.x, base.y + 2.2f * s * t, base.z }, 3.2f * s * ( 1.0f - 0.8f * f ),
                            2.8f * s * ( 1.0f - 0.8f * f ) - 0.2f * s, 2.2f * s, 8, keyA( 0.9f + 0.015f * t ), {},
                            0.39f );
            }
        };
        tower( Vector3{ 0, 6.0f, 0 }, 1.4f );
        if ( v == 0 )
            for ( float x : { -7.0f, 7.0f } )
                for ( float z : { -7.0f, 7.0f } ) tower( Vector3{ x, 6.0f, z }, 0.8f );
    }
    {   // Maya step pyramid with a temple on top.
        MeshBuilder &m = keyed( PropKindE::STEP_PYRAMID, 0.0f, FacingE::ROAD, rgb( 172, 166, 150 ) );
        for ( int i = 0; i < 7; ++i )
            m.box( Vector3{ 0, 2.0f + 4.0f * i, 0 }, Vector3{ 20.0f - 2.4f * i, 2.0f, 20.0f - 2.4f * i },
                   keyA( 0.85f + 0.02f * i ) );
        m.taperedBox( Vector3{ 20.0f, 0, 0 }, Vector2{ 3.0f, 3.5f }, Vector3{ 4.0f, 28.0f, 0 }, Vector2{ 1.0f, 3.0f },
                      keyA( 0.8f ) );
        m.box( Vector3{ 0, 30.5f, 0 }, Vector3{ 4.0f, 2.5f, 3.5f }, keyA( 1.0f ) );
        m.box( Vector3{ 0, 34.0f, 0 }, Vector3{ 3.2f, 1.0f, 2.8f }, keyA( 0.95f ) );
        m.box( Vector3{ 4.03f, 30.0f, 0 }, Vector3{ 0.05f, 1.5f, 0.8f }, rgb( 30, 26, 24 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Egyptian pyramid.
        MeshBuilder &m = add( PropKindE::STONE_PYRAMID, 0.0f );
        const float  s = v ? 0.7f : 1.0f;
        m.taperedBox( Vector3{ 0, -1.0f, 0 }, Vector2{ 60.0f * s, 60.0f * s }, Vector3{ 0, 72.0f * s, 0 },
                      Vector2{ 0.4f, 0.4f }, rgb( 214, 184, 124 ) );
        if ( v == 0 )
            m.taperedBox( Vector3{ 0, 62.0f, 0 }, Vector2{ 8.6f, 8.6f }, Vector3{ 0, 72.0f, 0 }, Vector2{ 0.4f, 0.4f },
                          rgb( 232, 214, 170 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Kasbah: earthen walls, corner towers that taper, crenellations.
        MeshBuilder &m = keyed( PropKindE::KASBAH, 9.0f, FacingE::ROAD, rgb( 186, 112, 72 ) );
        const float  hs = 9.0f + 2.0f * v;
        m.box( Vector3{ 0, 3.5f, 0 }, Vector3{ hs, 3.5f, hs }, keyA( 0.95f ) );
        for ( float x : { -1.0f, 1.0f } )
            for ( float z : { -1.0f, 1.0f } )
            {
                const Vector3 c = { x * hs, 0, z * hs };
                m.taperedBox( c, Vector2{ 2.6f, 2.6f }, Vector3{ c.x, 12.0f + 2.0f * v, c.z }, Vector2{ 2.0f, 2.0f },
                              keyA( 1.0f ) );
                merlons( m, Vector3{ c.x - 1.8f, 12.0f + 2.0f * v, c.z }, Vector3{ c.x + 1.8f, 12.0f + 2.0f * v, c.z },
                         2.0f, keyA( 0.9f ) );
                for ( int i = 0; i < 3; ++i )
                    m.box( Vector3{ c.x + x * 2.1f, 4.0f + 2.6f * i, c.z }, Vector3{ 0.05f, 0.35f, 0.25f },
                           rgb( 50, 30, 24 ) );
            }
        merlons( m, Vector3{ hs, 7.0f, -hs }, Vector3{ hs, 7.0f, hs }, 0.5f, keyA( 0.9f ) );
        m.box( Vector3{ 0, 7.5f, 0 }, Vector3{ hs * 0.5f, 3.0f, hs * 0.5f }, keyA( 0.9f ) );
        m.box( Vector3{ hs + 0.03f, 1.8f, 0 }, Vector3{ 0.05f, 1.8f, 1.2f }, rgb( 80, 50, 30 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Adobe: rounded mud-brick blocks with roof beams, or a straw roof.
        MeshBuilder &m = keyed( PropKindE::ADOBE, 4.0f, FacingE::ROAD, rgb( 196, 146, 100 ), rgb( 170, 140, 90 ) );
        m.box( Vector3{ 0, 1.6f, 0 }, Vector3{ 3.6f, 1.6f, 4.6f }, keyA( 1.0f ) );
        if ( v == 0 )
        {
            m.box( Vector3{ -1.0f, 4.2f, -1.2f }, Vector3{ 2.4f, 1.0f, 2.6f }, keyA( 0.95f ) );
            for ( int i = 0; i < 4; ++i )
                m.tube( Vector3{ 3.4f, 3.0f, -3.6f + 2.4f * i }, Vector3{ 4.1f, 3.0f, -3.6f + 2.4f * i }, 0.09f, 4,
                        darkWood );
            m.tube( Vector3{ 3.8f, 0, 2.0f }, Vector3{ 3.8f, 3.8f, 2.4f }, 0.05f, 4, darkWood );
        }
        else
        {
            gable( m, Vector3{ 0, 3.2f, 0 }, 3.6f, 4.6f, 2.0f, 0.5f, keyB( 1.0f ), keyA( 0.95f ) );
        }
        m.box( Vector3{ 3.63f, 1.0f, -0.5f }, Vector3{ 0.05f, 1.0f, 0.5f }, rgb( 70, 90, 120 ) );
        panes( m, Vector3{ 3.6f, 0, 0.5f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 4.0f, 3.2f, 1231 + v, 0.5f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Yurt.
        MeshBuilder &m = keyed( PropKindE::YURT, 3.0f, FacingE::FREE, rgb( 236, 232, 222 ), rgb( 180, 50, 50 ) );
        const float  r = 3.0f + 0.5f * v;
        m.cylinder( Vector3{ 0, 0, 0 }, r, r, 1.8f, 14, keyA( 1.0f ) );
        m.cylinder( Vector3{ 0, 1.8f, 0 }, r + 0.15f, 0.6f, 1.3f, 14, keyA( 0.94f ) );
        m.cylinder( Vector3{ 0, 1.2f, 0 }, r + 0.03f, r + 0.03f, 0.25f, 14, keyB( 1.0f ) );
        m.cylinder( Vector3{ 0, 3.1f, 0 }, 0.6f, 0.4f, 0.25f, 10, rgb( 120, 90, 60 ) );
        m.box( Vector3{ r - 0.05f, 0.8f, 0 }, Vector3{ 0.1f, 0.8f, 0.5f }, rgb( 200, 110, 40 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Rondavel: a round hut under a thatched cone.
        MeshBuilder &m = keyed( PropKindE::RONDAVEL, 3.0f, FacingE::ROAD, rgb( 230, 220, 196 ), rgb( 170, 140, 82 ) );
        const float  r = 2.8f + 0.4f * v;
        m.cylinder( Vector3{ 0, 0, 0 }, r, r, 2.4f, 12, keyA( 1.0f ) );
        m.cylinder( Vector3{ 0, 2.2f, 0 }, r + 0.7f, 0.1f, 3.0f + 0.5f * v, 12, keyB( 1.0f ) );
        m.box( Vector3{ r - 0.02f, 0.9f, 0 }, Vector3{ 0.1f, 0.9f, 0.45f }, darkWood );
        m.cylinder( Vector3{ 0, 0, 0 }, r + 0.05f, r + 0.05f, 0.4f, 12, keyA( 0.6f ) );
    }
    {   // Hanok: white walls, dark timber, a curved tiled roof.
        MeshBuilder &m = add( PropKindE::HANOK, 5.0f, FacingE::ROAD );
        const Color  tile = rgb( 70, 74, 80 ), wood = rgb( 110, 76, 50 );
        m.box( Vector3{ 0, 0.4f, 0 }, Vector3{ 4.6f, 0.4f, 8.6f }, rgb( 170, 164, 150 ) );
        m.box( Vector3{ 0, 2.0f, 0 }, Vector3{ 3.6f, 1.6f, 7.6f }, rgb( 236, 230, 214 ) );
        for ( int i = 0; i <= 5; ++i )
            m.box( Vector3{ 3.63f, 2.0f, -7.6f + 3.04f * i }, Vector3{ 0.08f, 1.6f, 0.12f }, wood );
        hip( m, Vector3{ 0, 3.6f, 0 }, 3.6f, 7.6f, 2.6f, 1.6f, tile );
        for ( float x : { -1.0f, 1.0f } )
            for ( float z : { -1.0f, 1.0f } )
                m.tri( Vector3{ x * 5.2f, 3.4f, z * 9.2f }, Vector3{ x * 5.9f, 4.4f, z * 9.9f },
                       Vector3{ x * 4.6f, 3.6f, z * 9.4f }, tile );
        m.box( Vector3{ 0, 6.25f, 0 }, Vector3{ 0.3f, 0.2f, 6.5f }, scaleColor( tile, 0.8f ) );
    }
    {   // Gassho-zukuri: a steep thatched roof like praying hands.
        MeshBuilder &m = add( PropKindE::GASSHO, 5.0f, FacingE::ROAD );
        const Color  thatch = rgb( 150, 122, 84 ), wood = rgb( 84, 62, 46 );
        m.box( Vector3{ 0, 1.5f, 0 }, Vector3{ 5.0f, 1.5f, 8.0f }, wood );
        gable( m, Vector3{ 0, 3.0f, 0 }, 5.0f, 8.0f, 9.5f, 0.8f, thatch, rgb( 200, 190, 170 ) );
        for ( float z : { -8.03f, 8.03f } )
            for ( int r = 0; r < 3; ++r )
                m.box( Vector3{ 0, 4.5f + 2.6f * r, z }, Vector3{ 2.6f - 0.8f * r, 0.5f, 0.04f },
                       keyC( r == 1 ? 1.0f : 0.3f ), r == 1 ? Lit : Glass );
        m_keys[static_cast<size_t>( PropKindE::GASSHO )] = { rgb( 128, 128, 128 ), rgb( 128, 128, 128 ),
                                                             rgb( 34, 40, 54 ) };
    }
    {   // Huizhou house: white walls, black tiles, stepped horse-head gables.
        MeshBuilder &m = add( PropKindE::HUIZHOU_HOUSE, 5.0f, FacingE::ROAD );
        const Color  wall = rgb( 238, 236, 230 ), tile = rgb( 48, 50, 56 );
        m.box( Vector3{ 0, 3.6f, 0 }, Vector3{ 4.6f, 3.6f, 6.0f }, wall );
        gable( m, Vector3{ 0, 7.2f, 0 }, 4.6f, 5.6f, 1.6f, 0.2f, tile, wall );
        for ( float z : { -6.0f, 6.0f } )
            for ( int s = 0; s < 3; ++s )
            {
                const float x = 4.6f - 1.6f * s;
                m.box( Vector3{ 0, 7.6f + 0.9f * s, z }, Vector3{ x, 0.45f, 0.25f }, wall );
                m.box( Vector3{ 0, 8.1f + 0.9f * s, z }, Vector3{ x + 0.2f, 0.08f, 0.35f }, tile );
            }
        m.box( Vector3{ 4.63f, 1.2f, 0 }, Vector3{ 0.05f, 1.2f, 0.8f }, rgb( 60, 50, 44 ) );
        for ( int i = 0; i < 3; ++i )
            m.box( Vector3{ 4.63f, 5.0f, -3.5f + 3.5f * i }, Vector3{ 0.05f, 0.5f, 0.5f }, rgb( 40, 40, 44 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Castle: a curtain wall, round towers with cone roofs, a keep.
        MeshBuilder &m = keyed( PropKindE::CASTLE, 0.0f, FacingE::ROAD, rgb( 170, 164, 150 ), rgb( 70, 74, 90 ) );
        const float  s = 14.0f + 2.0f * v;
        for ( int i = 0; i < 4; ++i )
        {
            const Vector3 a = { i < 2 ? -s : s, 0, i % 3 == 0 ? -s : s };
            const Vector3 b = { i == 0 || i == 3 ? -s : s, 0, i < 2 ? s : -s };
            m.box( Vector3Add( Vector3Scale( Vector3Add( a, b ), 0.5f ), Vector3{ 0, 4.5f, 0 } ),
                   Vector3{ std::abs( b.x - a.x ) * 0.5f + 0.9f, 4.5f, std::abs( b.z - a.z ) * 0.5f + 0.9f },
                   keyA( 0.95f ) );
            merlons( m, Vector3{ a.x, 9.0f, a.z }, Vector3{ b.x, 9.0f, b.z }, 0.9f, keyA( 0.9f ) );
            m.cylinder( Vector3{ a.x, 0, a.z }, 3.0f, 2.8f, 13.0f, 10, keyA( 1.0f ) );
            m.cylinder( Vector3{ a.x, 13.0f, a.z }, 3.4f, 0.05f, 6.0f, 10, keyB( 1.0f ) );
        }
        m.box( Vector3{ 0, 11.0f, 0 }, Vector3{ 5.5f, 11.0f, 5.5f }, keyA( 0.9f ) );
        if ( v == 0 )
            m.taperedBox( Vector3{ 0, 22.0f, 0 }, Vector2{ 6.0f, 6.0f }, Vector3{ 0, 28.0f, 0 },
                          Vector2{ 0.05f, 0.05f }, keyB( 1.0f ) );
        else
            merlons( m, Vector3{ 5.5f, 22.0f, -5.5f }, Vector3{ 5.5f, 22.0f, 5.5f }, 0.6f, keyA( 0.9f ) );
        m.box( Vector3{ s + 0.95f, 2.4f, 0 }, Vector3{ 0.05f, 2.4f, 1.8f }, rgb( 60, 44, 32 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Ruined temple: a platform and columns, some with their beams.
        MeshBuilder &m = add( PropKindE::COLUMNS, 3.0f, FacingE::ROAD );
        const Color  marble = rgb( 226, 218, 200 );
        m.box( Vector3{ 0, 0.6f, 0 }, Vector3{ 6.0f, 0.6f, 12.0f }, scaleColor( marble, 0.9f ) );
        for ( int i = 0; i < 6; ++i )
            for ( float x : { -4.5f, 4.5f } )
            {
                if ( ( i + ( x > 0 ) + v ) % 4 == 3 ) continue;
                const float h = ( i + v ) % 3 == 2 ? 4.0f : 8.0f;
                m.cylinder( Vector3{ x, 1.2f, -10.0f + 4.0f * i }, 0.75f, 0.65f, h, 10, marble );
            }
        m.box( Vector3{ -4.5f, 9.6f, -6.0f + 2.0f * v }, Vector3{ 0.9f, 0.4f, 4.4f }, marble );
    }
    {   // Aqueduct: two tiers of arches along the road.
        MeshBuilder &m = keyed( PropKindE::AQUEDUCT, 0.0f, FacingE::ALONG, rgb( 192, 172, 140 ) );
        for ( int tier = 0; tier < 2; ++tier )
        {
            const float y0 = tier * 12.0f, h = tier ? 9.0f : 12.0f, span = tier ? 5.0f : 10.0f;
            const int   n = static_cast<int>( 40.0f / span );
            for ( int i = 0; i <= n; ++i )
            {
                const float z = -20.0f + span * i;
                m.box( Vector3{ 0, y0 + h * 0.5f, z }, Vector3{ 1.6f, h * 0.5f, tier ? 0.7f : 1.4f }, keyA( 0.95f ) );
            }
            for ( int i = 0; i < n; ++i )
            {
                const float z0 = -20.0f + span * i, z1 = z0 + span, r = ( span - ( tier ? 1.4f : 2.8f ) ) * 0.5f,
                            zc = ( z0 + z1 ) * 0.5f;
                for ( int k = 0; k < 4; ++k )
                {   // The spandrel down to the arch, in four steps.
                    const float a0 = 3.14159f * k / 4.0f, a1 = 3.14159f * ( k + 1 ) / 4.0f;
                    const float za = zc - std::cos( a0 ) * r, zb = zc - std::cos( a1 ) * r;
                    const float ya = y0 + h - r + std::sin( a0 ) * r, yb = y0 + h - r + std::sin( a1 ) * r;
                    for ( float x : { -1.6f, 1.6f } )
                        m.quad( Vector3{ x, ya, za }, Vector3{ x, yb, zb }, Vector3{ x, y0 + h + 1.0f, zb },
                                Vector3{ x, y0 + h + 1.0f, za }, keyA( 0.9f ) );
                    m.quad( Vector3{ -1.6f, ya, za }, Vector3{ 1.6f, ya, za }, Vector3{ 1.6f, yb, zb },
                            Vector3{ -1.6f, yb, zb }, keyA( 0.75f ) );
                }
            }
            m.box( Vector3{ 0, y0 + h + 0.5f, 0 }, Vector3{ 1.7f, 0.5f, 20.5f }, keyA( 1.0f ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // White round windmill: four lattice sails (La Mancha) or eight cloth ones (the Cyclades).
        MeshBuilder &m = add( PropKindE::ROUND_MILL, 3.0f );
        m.cylinder( Vector3{ 0, 0, 0 }, 2.8f, 2.4f, 7.0f, 12, rgb( 244, 242, 236 ) );
        m.cylinder( Vector3{ 0, 7.0f, 0 }, 2.6f, 0.4f, 2.4f, 12, v ? rgb( 120, 90, 60 ) : rgb( 40, 40, 44 ) );
        m.box( Vector3{ 2.5f, 1.0f, 0 }, Vector3{ 0.05f, 1.0f, 0.5f }, rgb( 40, 80, 140 ) );
        const Vector3 hub = { 0, 7.6f, -2.8f };
        const int     n = v ? 8 : 4;
        for ( int i = 0; i < n; ++i )
        {
            const float   a = Tau * i / n + 0.3f;
            const Vector3 d = { std::cos( a ), std::sin( a ), 0 }, s = { -d.y, d.x, 0 };
            const Vector3 tip = Vector3Add( hub, Vector3Scale( d, 7.0f ) );
            m.tube( hub, tip, 0.08f, 4, rgb( 90, 70, 50 ) );
            if ( v )
                m.tri( Vector3Add( hub, Vector3Scale( d, 1.0f ) ), tip,
                       Vector3Add( Vector3Add( hub, Vector3Scale( d, 5.5f ) ), Vector3Scale( s, 1.6f ) ),
                       rgb( 244, 244, 240 ) );
            else
                m.quad( Vector3Add( hub, Vector3Scale( d, 1.5f ) ), Vector3Add( hub, Vector3Scale( d, 7.0f ) ),
                        Vector3Add( Vector3Add( hub, Vector3Scale( d, 7.0f ) ), Vector3Scale( s, 1.2f ) ),
                        Vector3Add( Vector3Add( hub, Vector3Scale( d, 1.5f ) ), Vector3Scale( s, 1.2f ) ),
                        rgb( 150, 130, 100 ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Watchtower: a tower on a wall (the Great Wall), or a tall slim stone tower (Svaneti).
        MeshBuilder &m = keyed( PropKindE::WATCHTOWER, 3.0f, FacingE::ALONG, rgb( 150, 144, 132 ) );
        if ( v == 0 )
        {
            m.box( Vector3{ 0, 3.0f, 0 }, Vector3{ 2.2f, 3.0f, 22.0f }, keyA( 0.9f ) );
            merlons( m, Vector3{ 2.0f, 6.0f, -22.0f }, Vector3{ 2.0f, 6.0f, 22.0f }, 0.3f, keyA( 0.85f ) );
            merlons( m, Vector3{ -2.0f, 6.0f, -22.0f }, Vector3{ -2.0f, 6.0f, 22.0f }, 0.3f, keyA( 0.85f ) );
            m.box( Vector3{ 0, 5.5f, 0 }, Vector3{ 4.5f, 5.5f, 4.5f }, keyA( 1.0f ) );
            merlons( m, Vector3{ 4.3f, 11.0f, -4.3f }, Vector3{ 4.3f, 11.0f, 4.3f }, 0.4f, keyA( 0.95f ) );
            merlons( m, Vector3{ -4.3f, 11.0f, -4.3f }, Vector3{ -4.3f, 11.0f, 4.3f }, 0.4f, keyA( 0.95f ) );
            for ( int i = 0; i < 3; ++i )
                m.box( Vector3{ 4.53f, 7.5f, -2.5f + 2.5f * i }, Vector3{ 0.05f, 0.9f, 0.5f }, rgb( 40, 36, 32 ) );
        }
        else
        {
            m.box( Vector3{ 0, 12.0f, 0 }, Vector3{ 2.0f, 12.0f, 2.0f }, keyA( 1.0f ) );
            m.box( Vector3{ 0, 22.5f, 0 }, Vector3{ 2.3f, 1.5f, 2.3f }, keyA( 0.95f ) );
            m.taperedBox( Vector3{ 0, 24.0f, 0 }, Vector2{ 2.5f, 2.5f }, Vector3{ 0, 25.6f, 0 },
                          Vector2{ 0.05f, 0.05f }, slate );
            for ( int i = 0; i < 4; ++i )
                m.box( Vector3{ 2.03f, 6.0f + 4.5f * i, 0 }, Vector3{ 0.05f, 0.5f, 0.3f }, rgb( 40, 36, 32 ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Fortress: battered walls, round bastions; or a dzong with its red band and golden roof.
        MeshBuilder &m = keyed( PropKindE::FORT, 0.0f, FacingE::ROAD, rgb( 206, 168, 112 ), rgb( 180, 130, 80 ) );
        if ( v == 0 )
        {
            m.taperedBox( Vector3{ 0, 0, 0 }, Vector2{ 22.0f, 16.0f }, Vector3{ 0, 14.0f, 0 }, Vector2{ 20.0f, 14.5f },
                          keyA( 0.95f ) );
            for ( float x : { -1.0f, 1.0f } )
                for ( float z : { -1.0f, 1.0f } )
                    m.cylinder( Vector3{ x * 21.0f, 0, z * 15.0f }, 4.5f, 3.8f, 17.0f, 12, keyA( 1.0f ) );
            m.box( Vector3{ 0, 15.0f, 0 }, Vector3{ 20.0f, 1.0f, 14.5f }, keyB( 1.0f ) );
            m.box( Vector3{ -4.0f, 20.0f, 0 }, Vector3{ 9.0f, 5.0f, 8.0f }, keyA( 0.95f ) );
            for ( int i = 0; i < 3; ++i )
                m.cylinder( Vector3{ -10.0f + 6.0f * i, 25.0f, 6.0f }, 1.8f, 1.8f, 1.2f, 8, keyA( 0.9f ) );
            for ( int i = 0; i < 3; ++i )
                m.blob( Vector3{ -10.0f + 6.0f * i, 26.6f, 6.0f }, Vector3{ 1.9f, 1.4f, 1.9f }, keyA( 0.9f ), 1241 + i,
                        0.0f );
        }
        else
        {
            m.taperedBox( Vector3{ 0, 0, 0 }, Vector2{ 24.0f, 18.0f }, Vector3{ 0, 16.0f, 0 }, Vector2{ 22.5f, 16.5f },
                          keyA( 1.0f ) );
            m.box( Vector3{ 0, 15.0f, 0 }, Vector3{ 22.6f, 1.2f, 16.6f }, keyB( 1.0f ) );
            m.box( Vector3{ 0, 22.0f, 0 }, Vector3{ 8.0f, 6.0f, 7.0f }, keyA( 1.0f ) );
            m.box( Vector3{ 0, 27.0f, 0 }, Vector3{ 8.1f, 1.0f, 7.1f }, keyB( 1.0f ) );
            hip( m, Vector3{ 0, 28.0f, 0 }, 8.0f, 7.0f, 3.5f, 1.2f, rgb( 214, 170, 70 ) );
            hip( m, Vector3{ 0, 16.0f, 0 }, 22.5f, 16.5f, 2.0f, 1.0f, rgb( 120, 100, 80 ) );
            for ( int i = 0; i < 6; ++i )
                m.box( Vector3{ 23.3f, 9.0f, -12.0f + 4.8f * i }, Vector3{ 0.05f, 0.9f, 0.6f }, rgb( 50, 40, 36 ) );
        }
    }
    {   // Cape Dutch: white walls, thatch, the ornate central gable.
        MeshBuilder &m = add( PropKindE::CAPE_DUTCH, 5.0f, FacingE::ROAD );
        const Color  wall = rgb( 246, 244, 238 ), thatch = rgb( 92, 76, 58 );
        m.box( Vector3{ 0, 2.4f, 0 }, Vector3{ 4.0f, 2.4f, 11.0f }, wall );
        gable( m, Vector3{ 0, 4.8f, 0 }, 4.0f, 11.0f, 3.8f, 0.4f, thatch, wall );
        m.box( Vector3{ 4.2f, 5.8f, 0 }, Vector3{ 0.3f, 1.2f, 2.2f }, wall );
        m.blob( Vector3{ 4.2f, 7.4f, 0 }, Vector3{ 0.3f, 1.0f, 1.6f }, wall, 1251, 0.0f );
        m.box( Vector3{ 4.23f, 1.2f, 0 }, Vector3{ 0.05f, 1.2f, 0.7f }, rgb( 40, 70, 50 ) );
        for ( float z : { -8.0f, -4.0f, 4.0f, 8.0f } )
            m.box( Vector3{ 4.03f, 2.6f, z }, Vector3{ 0.05f, 1.0f, 0.6f }, rgb( 40, 70, 50 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Grain elevator.
        MeshBuilder &m =
            keyed( PropKindE::GRAIN_ELEVATOR, 5.0f, FacingE::ROAD, rgb( 170, 50, 40 ), rgb( 230, 230, 226 ) );
        const float h = 24.0f + 4.0f * v;
        m.box( Vector3{ 0, h * 0.5f, 0 }, Vector3{ 4.0f, h * 0.5f, 5.0f }, keyA( 1.0f ) );
        gable( m, Vector3{ 0, h, 0 }, 4.0f, 5.0f, 1.6f, 0.2f, keyB( 0.8f ), keyA( 0.95f ) );
        m.box( Vector3{ 0, h + 3.0f, 0 }, Vector3{ 2.0f, 1.5f, 2.0f }, keyA( 0.95f ) );
        gable( m, Vector3{ 0, h + 4.5f, 0 }, 2.0f, 2.0f, 1.0f, 0.2f, keyB( 0.8f ), keyA( 0.95f ) );
        m.box( Vector3{ 0, 2.5f, 8.0f }, Vector3{ 3.0f, 2.5f, 3.0f }, keyA( 0.9f ) );
        m.box( Vector3{ 4.05f, h * 0.7f, 0 }, Vector3{ 0.05f, h * 0.12f, 4.0f }, keyB( 1.0f ) );
    }
    {   // Water tower on four legs.
        MeshBuilder &m = keyed( PropKindE::WATER_TOWER, 2.5f, FacingE::FREE, rgb( 220, 222, 226 ) );
        for ( float x : { -1.0f, 1.0f } )
            for ( float z : { -1.0f, 1.0f } )
                m.tube( Vector3{ x * 2.6f, 0, z * 2.6f }, Vector3{ x * 1.8f, 16.0f, z * 1.8f }, 0.18f, 5, steel );
        m.tube( Vector3{ -2.3f, 8.0f, -2.3f }, Vector3{ 2.3f, 8.0f, 2.3f }, 0.08f, 4, steel );
        m.tube( Vector3{ 2.3f, 8.0f, -2.3f }, Vector3{ -2.3f, 8.0f, 2.3f }, 0.08f, 4, steel );
        m.cylinder( Vector3{ 0, 16.0f, 0 }, 3.6f, 3.6f, 4.0f, 14, keyA( 1.0f ), Shiny );
        m.cylinder( Vector3{ 0, 20.0f, 0 }, 3.7f, 0.2f, 1.6f, 14, keyA( 0.85f ), Shiny );
    }
    {   // Pumpjack.
        MeshBuilder &m = add( PropKindE::PUMPJACK, 2.0f, FacingE::FREE );
        const Color  body = rgb( 70, 74, 80 ), head = rgb( 60, 60, 66 );
        m.box( Vector3{ 0, 0.3f, 0 }, Vector3{ 1.4f, 0.3f, 4.5f }, rgb( 150, 146, 136 ) );
        m.tube( Vector3{ -1.0f, 0.6f, -0.6f }, Vector3{ 0, 4.6f, 0.4f }, 0.12f, 5, body );
        m.tube( Vector3{ 1.0f, 0.6f, -0.6f }, Vector3{ 0, 4.6f, 0.4f }, 0.12f, 5, body );
        m.tube( Vector3{ 0, 0.6f, 1.8f }, Vector3{ 0, 4.6f, 0.4f }, 0.12f, 5, body );
        m.box( Vector3{ 0, 5.0f, 0.6f }, Vector3{ 0.25f, 0.3f, 3.6f }, body );
        m.box( Vector3{ 0, 4.4f, 4.4f }, Vector3{ 0.3f, 1.2f, 0.5f }, head );
        m.box( Vector3{ 0, 2.4f, -3.0f }, Vector3{ 0.9f, 1.0f, 0.4f }, rgb( 180, 60, 40 ) );
        m.tube( Vector3{ 0, 3.2f, 4.7f }, Vector3{ 0, 0.6f, 4.7f }, 0.04f, 4, steel );
    }
    {   // Farm wind pump.
        MeshBuilder &m = add( PropKindE::WIND_PUMP, 1.0f, FacingE::FREE );
        for ( float x : { -1.0f, 1.0f } )
            for ( float z : { -1.0f, 1.0f } )
                m.tube( Vector3{ x * 1.5f, 0, z * 1.5f }, Vector3{ x * 0.3f, 10.0f, z * 0.3f }, 0.07f, 4, steel );
        for ( int i = 1; i < 4; ++i )
        {
            const float y = 2.5f * i, w = 1.5f - 1.2f * y / 10.0f;
            m.tube( Vector3{ -w, y, -w }, Vector3{ w, y, w }, 0.04f, 3, steel );
            m.tube( Vector3{ w, y, -w }, Vector3{ -w, y, w }, 0.04f, 3, steel );
        }
        const Vector3 hub = { 0, 10.6f, -0.6f };
        for ( int i = 0; i < 16; ++i )
        {
            const float   a = Tau * i / 16.0f;
            const Vector3 d = { std::cos( a ), std::sin( a ), 0 }, s = { -d.y * 0.25f, d.x * 0.25f, 0 };
            m.quad( Vector3Add( hub, Vector3Scale( d, 0.6f ) ), Vector3Add( hub, Vector3Scale( d, 2.4f ) ),
                    Vector3Add( Vector3Add( hub, Vector3Scale( d, 2.4f ) ), s ),
                    Vector3Add( Vector3Add( hub, Vector3Scale( d, 0.6f ) ), s ), rgb( 196, 198, 200 ) );
        }
        m.tube( Vector3{ 0, 10.6f, -0.6f }, Vector3{ 0, 10.6f, 3.0f }, 0.06f, 4, steel );
        m.quad( Vector3{ 0, 10.0f, 2.0f }, Vector3{ 0, 10.0f, 3.6f }, Vector3{ 0, 11.6f, 3.6f },
                Vector3{ 0, 11.2f, 2.0f }, rgb( 196, 198, 200 ) );
    }
    {   // Diner: chrome, big windows, a red and turquoise sign.
        MeshBuilder &m =
            keyed( PropKindE::DINER, 5.0f, FacingE::ROAD, rgb( 210, 214, 220 ), rgb( 214, 40, 50 ), rgb( 60, 64, 76 ) );
        m.box( Vector3{ 0, 1.8f, 0 }, Vector3{ 4.0f, 1.8f, 9.0f }, keyA( 1.0f ), Shiny );
        m.box( Vector3{ 0, 3.75f, 0 }, Vector3{ 4.2f, 0.25f, 9.2f }, keyB( 1.0f ) );
        m.box( Vector3{ 0, 0.5f, 0 }, Vector3{ 4.03f, 0.18f, 9.03f }, keyB( 1.0f ) );
        panes( m, Vector3{ 4.0f, 0, -8.5f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 17.0f, 3.6f, 1261, 0.9f, 3.0f,
               1.2f );
        m.tube( Vector3{ 6.0f, 0, -9.0f }, Vector3{ 6.0f, 8.0f, -9.0f }, 0.2f, 6, steel );
        m.box( Vector3{ 6.0f, 9.0f, -9.0f }, Vector3{ 0.3f, 1.4f, 3.4f }, rgb( 230, 40, 50 ), Mat{ 0.5f, 0.0f } );
        m.box( Vector3{ 6.0f, 9.0f, -9.0f }, Vector3{ 0.35f, 0.5f, 2.8f }, rgb( 60, 220, 220 ), Mat{ 0.6f, 0.0f } );
    }
    {   // Motel sign: an arrow and a star on a tall pole.
        MeshBuilder &m = add( PropKindE::MOTEL_SIGN, 0.4f, FacingE::ALONG );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 9.0f, 0 }, 0.2f, 6, steel );
        m.box( Vector3{ 0, 10.0f, 0 }, Vector3{ 3.0f, 1.6f, 0.25f }, rgb( 30, 120, 150 ), Mat{ 0.3f, 0.0f } );
        m.box( Vector3{ 0, 10.0f, -0.26f }, Vector3{ 2.6f, 0.5f, 0.02f }, rgb( 255, 230, 120 ), Mat{ 0.8f, 0.0f } );
        m.tri( Vector3{ -3.0f, 8.0f, -0.27f }, Vector3{ -1.0f, 8.0f, -0.27f }, Vector3{ -2.0f, 7.0f, -0.27f },
               rgb( 255, 80, 60 ), Mat{ 0.8f, 0.0f } );
        m.box( Vector3{ -2.0f, 8.4f, 0 }, Vector3{ 0.3f, 0.4f, 0.2f }, rgb( 255, 80, 60 ), Mat{ 0.8f, 0.0f } );
        for ( int i = 0; i < 5; ++i )
        {
            const float a = Tau * i / 5.0f + 1.57f;
            m.tri( Vector3{ 0, 13.0f, -0.2f }, Vector3{ std::cos( a ) * 1.2f, 13.0f + std::sin( a ) * 1.2f, -0.2f },
                   Vector3{ std::cos( a + 0.6f ) * 0.4f, 13.0f + std::sin( a + 0.6f ) * 0.4f, -0.2f },
                   rgb( 255, 220, 80 ), Mat{ 0.8f, 0.0f } );
        }
    }
    {   // Filling station: a canopy, pumps, a shop.
        MeshBuilder &m = keyed( PropKindE::GAS_STATION, 4.0f, FacingE::ROAD, rgb( 220, 40, 40 ), rgb( 240, 240, 240 ),
                                rgb( 60, 66, 80 ) );
        for ( float z : { -5.0f, 5.0f } )
            m.box( Vector3{ 0, 2.4f, z }, Vector3{ 0.25f, 2.4f, 0.25f }, rgb( 230, 230, 230 ) );
        m.box( Vector3{ 0, 5.1f, 0 }, Vector3{ 4.5f, 0.4f, 8.0f }, keyA( 1.0f ) );
        m.box( Vector3{ 0, 4.65f, 0 }, Vector3{ 4.3f, 0.05f, 7.8f }, rgb( 250, 250, 240 ), Mat{ 0.5f, 0.0f } );
        for ( float z : { -2.5f, 2.5f } ) m.box( Vector3{ 0, 0.9f, z }, Vector3{ 0.4f, 0.9f, 0.6f }, keyB( 1.0f ) );
        m.box( Vector3{ -10.0f, 2.0f, 0 }, Vector3{ 3.0f, 2.0f, 6.0f }, keyB( 0.95f ) );
        panes( m, Vector3{ -7.0f, 0, -5.5f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 11.0f, 3.6f, 1271, 0.9f, 3.0f,
               0.8f );
        m.tube( Vector3{ 5.0f, 0, 9.0f }, Vector3{ 5.0f, 6.0f, 9.0f }, 0.15f, 5, steel );
        m.box( Vector3{ 5.0f, 7.0f, 9.0f }, Vector3{ 0.2f, 1.2f, 1.2f }, keyA( 1.0f ), Mat{ 0.3f, 0.0f } );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Casino hotel: a tower with glowing bands, a marquee.
        MeshBuilder &m = add( PropKindE::CASINO, 12.0f, FacingE::ROAD );
        const Color  facade = v ? rgb( 210, 170, 90 ) : rgb( 200, 210, 230 );
        const Color  glow = v ? rgb( 255, 200, 90 ) : rgb( 120, 200, 255 );
        const float  h = 70.0f + 20.0f * v;
        m.box( Vector3{ 0, h * 0.5f, 0 }, Vector3{ 8.0f, h * 0.5f, 18.0f }, facade, Glass );
        for ( int i = 1; i < 8; ++i )
            m.box( Vector3{ 8.05f, h * i / 8.0f, 0 }, Vector3{ 0.05f, 0.3f, 18.0f }, glow, Mat{ 0.8f, 0.0f } );
        m.box( Vector3{ 6.0f, 4.0f, 0 }, Vector3{ 6.0f, 4.0f, 22.0f }, rgb( 60, 30, 60 ) );
        m.box( Vector3{ 12.05f, 6.0f, 0 }, Vector3{ 0.05f, 1.6f, 20.0f }, v ? rgb( 255, 60, 120 ) : rgb( 255, 220, 80 ),
               Mat{ 1.0f, 0.0f } );
        m.box( Vector3{ 12.05f, 3.0f, 0 }, Vector3{ 0.05f, 0.5f, 20.0f }, rgb( 80, 255, 200 ), Mat{ 1.0f, 0.0f } );
        m.box( Vector3{ 0, h + 3.0f, 0 }, Vector3{ 6.0f, 3.0f, 10.0f }, glow, Mat{ 0.7f, 0.0f } );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Glass high-rise.
        MeshBuilder &m = keyed( PropKindE::GLASS_TOWER, 10.0f, FacingE::ROAD, rgb( 90, 126, 156 ), rgb( 200, 204, 210 ),
                                rgb( 40, 50, 66 ) );
        const float  h = 50.0f + 25.0f * v, w = 10.0f + 2.0f * ( v % 2 );
        m.box( Vector3{ 0, h * 0.5f, 0 }, Vector3{ w, h * 0.5f, w }, keyA( 1.0f ), Glass );
        if ( v == 2 ) m.box( Vector3{ 0, h + 10.0f, 0 }, Vector3{ w * 0.6f, 10.0f, w * 0.6f }, keyA( 0.95f ), Glass );
        for ( int i = 1; i < static_cast<int>( h / 4.0f ); ++i )
            m.box( Vector3{ 0, 4.0f * i, 0 }, Vector3{ w + 0.05f, 0.18f, w + 0.05f }, keyB( 1.0f ) );
        boxPanes( m, Vector3{ 0, 0, 0 }, w + 0.02f, w + 0.02f, h, 1281 + v, 0.35f, 4.0f, 1.5f );
        m.tube( Vector3{ 0, h + ( v == 2 ? 20.0f : 0.0f ), 0 }, Vector3{ 0, h + ( v == 2 ? 32.0f : 8.0f ), 0 }, 0.3f, 5,
                steel );
    }
    {   // Supertall: stepped setbacks to a needle.
        MeshBuilder &m = add( PropKindE::SUPERTALL, 0.0f );
        const Color  glass = rgb( 170, 190, 210 );
        float        y = 0.0f;
        for ( int i = 0; i < 7; ++i )
        {
            const float w = 18.0f - 2.3f * i, h = 42.0f - 2.0f * i;
            m.cylinder( Vector3{ 0, y, 0 }, w, w * 0.94f, h, 6, glass, Glass, 0.26f * i );
            y += h;
        }
        m.cylinder( Vector3{ 0, y, 0 }, 2.2f, 0.2f, 80.0f, 6, rgb( 210, 216, 224 ), Shiny );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Television tower: a shaft with spheres (Shanghai) or a pod (Berlin, Toronto).
        MeshBuilder &m = add( PropKindE::TV_TOWER, 0.0f );
        const Color  concrete = rgb( 214, 212, 206 );
        m.cylinder( Vector3{ 0, 0, 0 }, 3.0f, 2.2f, 160.0f, 10, concrete );
        if ( v == 0 )
        {
            for ( float x : { -1.0f, 1.0f } )
                m.tube( Vector3{ x * 9.0f, 0, 0 }, Vector3{ x * 1.5f, 60.0f, 0 }, 1.4f, 8, concrete );
            m.blob( Vector3{ 0, 60.0f, 0 }, Vector3{ 13.0f, 13.0f, 13.0f }, rgb( 200, 70, 120 ), 1291, 0.0f, Shiny );
            m.blob( Vector3{ 0, 120.0f, 0 }, Vector3{ 9.0f, 9.0f, 9.0f }, rgb( 200, 70, 120 ), 1292, 0.0f, Shiny );
        }
        else
        {
            m.blob( Vector3{ 0, 130.0f, 0 }, Vector3{ 12.0f, 7.0f, 12.0f }, rgb( 190, 196, 204 ), 1293, 0.0f, Shiny );
            m.cylinder( Vector3{ 0, 124.0f, 0 }, 13.0f, 13.0f, 2.0f, 16, rgb( 60, 70, 90 ), Glass );
        }
        m.cylinder( Vector3{ 0, 160.0f, 0 }, 1.2f, 0.2f, 40.0f, 6, rgb( 220, 70, 60 ) );
    }
    {   // Iron lattice tower.
        MeshBuilder &m = keyed( PropKindE::LATTICE_TOWER, 0.0f, FacingE::FREE, rgb( 112, 92, 70 ), rgb( 112, 92, 70 ) );
        const float  hs[] = { 0.0f, 20.0f, 45.0f, 80.0f, 115.0f };
        const float  ws[] = { 24.0f, 14.0f, 8.0f, 4.0f, 1.2f };
        for ( int i = 0; i < 4; ++i )
        {
            const Color c = i % 2 ? keyB( 1.0f ) : keyA( 1.0f );
            for ( float x : { -1.0f, 1.0f } )
                for ( float z : { -1.0f, 1.0f } )
                    m.tube( Vector3{ x * ws[i], hs[i], z * ws[i] }, Vector3{ x * ws[i + 1], hs[i + 1], z * ws[i + 1] },
                            1.4f - 0.3f * i, 5, c );
            for ( int s = 0; s < 4; ++s )
            {   // Bracing on the four faces.
                const float   b0 = s % 2 ? -1.0f : 1.0f;
                const Vector3 p0 = s < 2 ? Vector3{ -ws[i], hs[i], b0 * ws[i] } : Vector3{ b0 * ws[i], hs[i], -ws[i] };
                const Vector3 p1 = s < 2 ? Vector3{ ws[i + 1], hs[i + 1], b0 * ws[i + 1] }
                                         : Vector3{ b0 * ws[i + 1], hs[i + 1], ws[i + 1] };
                m.tube( p0, p1, 0.35f, 4, c );
            }
            if ( i < 3 ) m.box( Vector3{ 0, hs[i + 1], 0 }, Vector3{ ws[i + 1] + 1.0f, 1.0f, ws[i + 1] + 1.0f }, c );
        }
        m.cylinder( Vector3{ 0, 115.0f, 0 }, 1.2f, 0.15f, 14.0f, 6, keyA( 1.0f ) );
        // The arch between the legs.
        for ( int k = 0; k < 6; ++k )
        {
            const float a0 = 3.14159f * k / 6.0f, a1 = 3.14159f * ( k + 1 ) / 6.0f;
            for ( float z : { -20.0f, 20.0f } )
                m.tube( Vector3{ -std::cos( a0 ) * 16.0f, 4.0f + std::sin( a0 ) * 10.0f, z },
                        Vector3{ -std::cos( a1 ) * 16.0f, 4.0f + std::sin( a1 ) * 10.0f, z }, 0.5f, 4, keyA( 0.9f ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Panel block of flats.
        MeshBuilder &m = keyed( PropKindE::PANEL_BLOCK, 12.0f, FacingE::ROAD, rgb( 206, 200, 186 ),
                                rgb( 150, 150, 150 ), rgb( 40, 46, 58 ) );
        const float  h = v ? 30.0f : 15.0f, l = v ? 16.0f : 30.0f;
        m.box( Vector3{ 0, h * 0.5f, 0 }, Vector3{ 6.0f, h * 0.5f, l }, keyA( 1.0f ) );
        for ( int i = 1; i < static_cast<int>( h / 3.0f ); ++i )
            m.box( Vector3{ 6.02f, 3.0f * i, 0 }, Vector3{ 0.04f, 0.08f, l }, keyA( 0.85f ) );
        boxPanes( m, Vector3{ 0, 0, 0 }, 6.0f, l, h, 1301 + v, 0.35f, 3.0f, 1.2f );
        for ( int i = 0; i < static_cast<int>( l / 4.0f ); i += 2 )
            for ( int f = 1; f < static_cast<int>( h / 3.0f ); ++f )
                m.box( Vector3{ 6.5f, 3.0f * f - 0.4f, -l + 4.0f * i + 2.0f }, Vector3{ 0.5f, 0.5f, 1.4f },
                       keyA( 0.9f ) );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Favela: painted brick boxes stacked up the slope.
        MeshBuilder &m = add( PropKindE::FAVELA, 8.0f, FacingE::ROAD );
        const Color  paints[] = { rgb( 230, 120, 80 ),  rgb( 90, 170, 200 ),  rgb( 240, 210, 90 ), rgb( 180, 110, 170 ),
                                  rgb( 120, 190, 120 ), rgb( 236, 236, 228 ), rgb( 196, 120, 70 ), rgb( 220, 90, 90 ) };
        Rng          r( 1311 + v );
        for ( int i = 0; i < 14; ++i )
        {
            const float x = r.range( -8.0f, 8.0f ), z = r.range( -10.0f, 10.0f ),
                        base = std::max( 0.0f, -x * 0.6f + 2.0f );
            const float hx = r.range( 1.8f, 2.8f ), hz = r.range( 1.8f, 3.0f ), h = r.range( 2.8f, 6.0f );
            const Color c = paints[r.rangeInt( 0, 7 )];
            m.box( Vector3{ x, base + h * 0.5f - 1.0f, z }, Vector3{ hx, h * 0.5f + 1.0f, hz }, c );
            m.box( Vector3{ x, base + h, z }, Vector3{ hx + 0.1f, 0.08f, hz + 0.1f }, rgb( 160, 150, 140 ) );
            panes( m, Vector3{ x + hx, base - 1.0f, z - hz }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, hz * 2.0f,
                   h + 1.0f, 1321 + i + v * 20, 0.3f, 2.8f, 1.8f );
        }
        m_keys[static_cast<size_t>( PropKindE::FAVELA )] = { rgb( 128, 128, 128 ), rgb( 128, 128, 128 ),
                                                             rgb( 40, 46, 58 ) };
    }
    for ( int v = 0; v < 2; ++v )
    {   // Water villa on stilts with a thatched roof and a jetty.
        MeshBuilder &m = add( PropKindE::WATER_VILLA, 0.0f, FacingE::ROAD );
        const Color  wood = rgb( 150, 110, 76 ), thatch = rgb( 170, 140, 92 );
        for ( float x : { -3.0f, 3.0f } )
            for ( float z : { -3.5f, 3.5f } ) m.tube( Vector3{ x, -3.0f, z }, Vector3{ x, 1.6f, z }, 0.15f, 4, wood );
        m.box( Vector3{ 0, 1.7f, 0 }, Vector3{ 3.8f, 0.15f, 4.4f }, wood );
        m.box( Vector3{ 0, 3.0f, 0 }, Vector3{ 3.0f, 1.2f, 3.6f }, rgb( 236, 226, 206 ) );
        hip( m, Vector3{ 0, 4.2f, 0 }, 3.0f, 3.6f, 2.6f, 1.0f, thatch );
        m.box( Vector3{ 9.0f, 1.6f, 0 }, Vector3{ 5.5f, 0.1f, 0.8f }, wood );
        for ( int i = 0; i < 4; ++i )
            m.tube( Vector3{ 5.0f + 2.8f * i, -3.0f, 0.7f }, Vector3{ 5.0f + 2.8f * i, 1.5f, 0.7f }, 0.1f, 4, wood );
        panes( m, Vector3{ 3.0f, 1.8f, -3.6f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 7.2f, 2.4f, 1361 + v, 0.6f,
               3.0f, 0.4f );
        m_keys[static_cast<size_t>( PropKindE::WATER_VILLA )] = { rgb( 128, 128, 128 ), rgb( 128, 128, 128 ),
                                                                  rgb( 40, 46, 58 ) };
    }
    {   // Balinese split gate.
        MeshBuilder &m = add( PropKindE::TEMPLE_GATE, 2.0f, FacingE::ALONG );
        const Color  brick = rgb( 150, 80, 60 ), stone = rgb( 110, 108, 100 );
        for ( float x : { -1.0f, 1.0f } )
        {
            for ( int s = 0; s < 8; ++s )
            {
                const float y = 1.0f * s, w = 2.4f - 0.24f * s;
                m.box( Vector3{ x * ( 1.6f + w * 0.5f ), y + 0.5f, 0 }, Vector3{ w * 0.5f, 0.5f, w * 0.6f },
                       s % 2 ? stone : brick );
            }
            m.cylinder( Vector3{ x * 2.0f, 8.0f, 0 }, 0.4f, 0.05f, 1.5f, 6, stone );
        }
    }
    {   // Prayer flags strung between poles.
        MeshBuilder &m = add( PropKindE::PRAYER_FLAGS, 0.0f, FacingE::FREE );
        const Color  flags[] = { rgb( 40, 90, 200 ), rgb( 240, 240, 240 ), rgb( 210, 40, 40 ), rgb( 40, 160, 70 ),
                                 rgb( 240, 200, 40 ) };
        m.tube( Vector3{ 0, 0, -8.0f }, Vector3{ 0, 7.0f, -8.0f }, 0.08f, 4, rgb( 120, 100, 80 ) );
        m.tube( Vector3{ 0, 0, 8.0f }, Vector3{ 0, 5.0f, 8.0f }, 0.08f, 4, rgb( 120, 100, 80 ) );
        for ( int line = 0; line < 2; ++line )
            for ( int i = 0; i < 20; ++i )
            {
                const float t = ( i + 0.5f ) / 20.0f, z = -8.0f + 16.0f * t;
                const float y = lerpf( 7.0f, 5.0f, t ) - 1.2f * std::sin( t * 3.14159f ) - line * 1.4f;
                m.quad( Vector3{ 0, y, z - 0.3f }, Vector3{ 0, y, z + 0.3f }, Vector3{ 0.05f, y - 0.7f, z + 0.3f },
                        Vector3{ 0.05f, y - 0.7f, z - 0.3f }, flags[i % 5] );
            }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Wayside cross under a little roof, or a small white shrine.
        MeshBuilder &m = add( PropKindE::WAYSIDE_CROSS, 0.3f, FacingE::ROAD );
        if ( v == 0 )
        {
            m.box( Vector3{ 0, 1.6f, 0 }, Vector3{ 0.1f, 1.6f, 0.1f }, darkWood );
            m.box( Vector3{ 0, 2.4f, 0 }, Vector3{ 0.08f, 0.08f, 0.7f }, darkWood );
            gable( m, Vector3{ 0, 3.2f, 0 }, 0.25f, 0.6f, 0.4f, 0.1f, darkWood, darkWood );
        }
        else
        {
            m.box( Vector3{ 0, 1.2f, 0 }, Vector3{ 0.7f, 1.2f, 0.9f }, white );
            gable( m, Vector3{ 0, 2.4f, 0 }, 0.7f, 0.9f, 0.7f, 0.15f, rgb( 150, 70, 52 ), white );
            m.box( Vector3{ 0.72f, 1.4f, 0 }, Vector3{ 0.03f, 0.5f, 0.35f }, rgb( 60, 70, 110 ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Beach umbrella with two loungers.
        MeshBuilder &m = keyed( PropKindE::PARASOL, 0.0f, FacingE::FREE, rgb( 240, 120, 60 ) );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 2.6f, 0 }, 0.04f, 4, white );
        m.cylinder( Vector3{ 0, 2.1f, 0 }, 1.6f, 0.05f, 0.6f, 8, keyA( 1.0f ), {}, 0.2f * v );
        for ( float z : { -1.0f, 1.0f } ) m.box( Vector3{ 0.8f, 0.3f, z }, Vector3{ 0.9f, 0.08f, 0.35f }, white );
    }
    {   // Hedgerow.
        MeshBuilder &m = add( PropKindE::HEDGE, 0.0f, FacingE::ALONG );
        m.box( Vector3{ 0, 0.8f, 0 }, Vector3{ 0.6f, 0.8f, 2.8f }, rgb( 60, 98, 48 ) );
        for ( int i = 0; i < 4; ++i )
            m.blob( Vector3{ 0, 1.6f, -2.1f + 1.4f * i }, Vector3{ 0.7f, 0.45f, 0.9f }, rgb( 66 + 4 * i, 108, 52 ),
                    1371 + i, 0.5f );
    }
    {   // White ranch fence.
        MeshBuilder &m = add( PropKindE::RANCH_FENCE, 0.0f, FacingE::ALONG );
        for ( int i = 0; i < 3; ++i )
            m.box( Vector3{ 0, 0.7f, -2.6f + 2.6f * i }, Vector3{ 0.08f, 0.7f, 0.08f }, white );
        for ( float y : { 0.5f, 0.9f, 1.3f } ) m.box( Vector3{ 0.1f, y, 0 }, Vector3{ 0.03f, 0.08f, 2.7f }, white );
    }
    {   // Lattice power pylon.
        MeshBuilder &m = add( PropKindE::POWER_PYLON, 1.5f, FacingE::ALONG );
        for ( float x : { -1.0f, 1.0f } )
            for ( float z : { -1.0f, 1.0f } )
                m.tube( Vector3{ x * 3.0f, 0, z * 3.0f }, Vector3{ x * 0.8f, 30.0f, z * 0.8f }, 0.15f, 4, steel );
        for ( int i = 1; i < 6; ++i )
        {
            const float y = 5.0f * i, w = 3.0f - 2.2f * y / 30.0f;
            m.tube( Vector3{ -w, y, -w }, Vector3{ w, y, -w }, 0.06f, 3, steel );
            m.tube( Vector3{ -w, y, w }, Vector3{ w, y, w }, 0.06f, 3, steel );
            m.tube( Vector3{ -w, y - 5.0f, -w }, Vector3{ w, y, -w }, 0.05f, 3, steel );
        }
        for ( float y : { 22.0f, 28.0f } )
        {
            m.box( Vector3{ 0, y, 0 }, Vector3{ 9.0f - ( y > 25.0f ? 3.0f : 0.0f ), 0.25f, 0.4f }, steel );
            for ( float x : { -1.0f, 1.0f } )
                m.tube( Vector3{ x * ( 8.5f - ( y > 25.0f ? 3.0f : 0.0f ) ), y, 0 },
                        Vector3{ x * ( 8.5f - ( y > 25.0f ? 3.0f : 0.0f ) ), y - 2.0f, 0 }, 0.12f, 4,
                        rgb( 200, 210, 220 ) );
        }
    }
    for ( int v = 0; v < 3; ++v )
    {   // Hot air balloon, high above.
        MeshBuilder &m = keyed( PropKindE::BALLOON, 0.0f, FacingE::FREE, rgb( 230, 80, 60 ) );
        const float  y = 40.0f + 25.0f * v;
        m.blob( Vector3{ 0, y + 9.0f, 0 }, Vector3{ 7.0f, 8.5f, 7.0f }, keyA( 1.0f ), 1381 + v, 0.0f );
        m.cylinder( Vector3{ 0, y + 1.6f, 0 }, 2.0f, 5.5f, 3.0f, 12, keyA( 0.8f ) );
        for ( int i = 0; i < 6; ++i )
        {
            const float a = Tau * i / 6.0f;
            m.quad( Vector3{ std::cos( a ) * 7.05f, y + 9.0f, std::sin( a ) * 7.05f },
                    Vector3{ std::cos( a + 0.15f ) * 7.05f, y + 9.0f, std::sin( a + 0.15f ) * 7.05f },
                    Vector3{ std::cos( a + 0.15f ) * 5.6f, y + 3.5f, std::sin( a + 0.15f ) * 5.6f },
                    Vector3{ std::cos( a ) * 5.6f, y + 3.5f, std::sin( a ) * 5.6f }, rgb( 250, 240, 200 ) );
        }
        m.box( Vector3{ 0, y - 0.6f, 0 }, Vector3{ 0.7f, 0.6f, 0.7f }, rgb( 150, 110, 70 ) );
        m.box( Vector3{ 0, y + 0.6f, 0 }, Vector3{ 0.25f, 0.4f, 0.25f }, rgb( 255, 170, 80 ), Mat{ 0.8f, 0.0f } );
    }

    // ---- Boats (on the water) ---------------------------------------------------------------------------------------
    for ( int v = 0; v < 3; ++v )
    {   // Sailing boat: a sloop, a lateen-rigged felucca, or a junk (by the sail's colour).
        MeshBuilder &m = keyed( PropKindE::SAILBOAT, 0.0f, FacingE::FREE, rgb( 244, 242, 236 ), rgb( 236, 236, 236 ) );
        const float  l = 4.0f + 1.5f * v;
        m.taperedBox( Vector3{ 0, -0.3f, 0 }, Vector2{ 0.5f, l * 0.8f }, Vector3{ 0, 0.8f, 0 }, Vector2{ 1.2f, l },
                      keyB( 1.0f ) );
        m.box( Vector3{ 0, 0.85f, 0 }, Vector3{ 1.1f, 0.05f, l * 0.95f }, rgb( 170, 130, 90 ) );
        const float h = 7.0f + 2.0f * v;
        m.tube( Vector3{ 0, 0.8f, l * 0.15f }, Vector3{ 0, h, l * 0.15f }, 0.08f, 4, rgb( 120, 100, 80 ) );
        if ( v == 1 )
            m.tri( Vector3{ 0, 1.6f, -l * 0.9f }, Vector3{ 0, h + 3.0f, l * 0.9f }, Vector3{ 0, 1.2f, l * 0.6f },
                   keyA( 1.0f ) );
        else
        {
            m.tri( Vector3{ 0, 1.4f, l * 0.15f }, Vector3{ 0, h, l * 0.15f }, Vector3{ 0, 1.4f, -l * 0.7f },
                   keyA( 1.0f ) );
            m.tri( Vector3{ 0, 1.2f, l * 0.25f }, Vector3{ 0, h * 0.85f, l * 0.18f }, Vector3{ 0, 1.0f, l * 0.95f },
                   keyA( 0.92f ) );
            if ( v == 2 )
                for ( int i = 1; i < 4; ++i )
                    m.box( Vector3{ 0, 1.4f + ( h - 1.4f ) * i / 4.0f,
                                    l * 0.15f - l * 0.85f * ( 1.0f - i / 4.0f ) * 0.5f },
                           Vector3{ 0.03f, 0.04f, l * 0.85f * ( 1.0f - i / 4.0f ) * 0.5f }, rgb( 80, 60, 40 ) );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Small boat: a pirogue, a longtail, a lobster boat.
        MeshBuilder &m = keyed( PropKindE::BOAT, 0.0f, FacingE::FREE, rgb( 40, 110, 170 ), rgb( 240, 240, 236 ) );
        const float  l = 4.5f + 2.0f * v;
        m.taperedBox( Vector3{ 0, -0.2f, 0 }, Vector2{ 0.5f, l * 0.75f }, Vector3{ 0, 0.7f, 0 }, Vector2{ 1.1f, l },
                      keyA( 1.0f ) );
        m.box( Vector3{ 0, 0.72f, 0 }, Vector3{ 1.12f, 0.05f, l }, keyB( 1.0f ) );
        m.tri( Vector3{ 0, 0.7f, l }, Vector3{ 0, 1.6f, l + 0.6f }, Vector3{ 0, 0.7f, l - 0.8f }, keyA( 0.9f ) );
        if ( v == 1 )
        {
            m.box( Vector3{ 0, 1.6f, -l * 0.2f }, Vector3{ 0.9f, 0.9f, 1.2f }, keyB( 1.0f ) );
            m.box( Vector3{ 0, 1.9f, -l * 0.2f + 1.21f }, Vector3{ 0.7f, 0.35f, 0.02f }, keyC( 0.4f ), Glass );
        }
    }
    {   // Kerala houseboat.
        MeshBuilder &m = add( PropKindE::HOUSEBOAT, 0.0f, FacingE::FREE );
        const Color  wood = rgb( 90, 60, 40 ), straw = rgb( 190, 160, 100 );
        m.taperedBox( Vector3{ 0, -0.3f, 0 }, Vector2{ 1.2f, 9.0f }, Vector3{ 0, 0.8f, 0 }, Vector2{ 2.0f, 11.0f },
                      wood );
        for ( int i = 0; i < 8; ++i )
        {
            const float a0 = 3.14159f * i / 8.0f, a1 = 3.14159f * ( i + 1 ) / 8.0f;
            m.quad( Vector3{ -std::cos( a0 ) * 2.0f, 0.8f + std::sin( a0 ) * 2.4f, -7.0f },
                    Vector3{ -std::cos( a1 ) * 2.0f, 0.8f + std::sin( a1 ) * 2.4f, -7.0f },
                    Vector3{ -std::cos( a1 ) * 2.0f, 0.8f + std::sin( a1 ) * 2.4f, 7.0f },
                    Vector3{ -std::cos( a0 ) * 2.0f, 0.8f + std::sin( a0 ) * 2.4f, 7.0f }, straw );
        }
        m.tri( Vector3{ 0, 0.8f, 11.0f }, Vector3{ 0, 2.4f, 11.6f }, Vector3{ 0, 0.8f, 9.0f }, wood );
    }

    // ---- Animals ----------------------------------------------------------------------------------------------------
    for ( int v = 0; v < 3; ++v )
    {   // Cow: black and white, brown, or a shaggy highlander.
        MeshBuilder &m = add( PropKindE::COW, 0.0f );
        Beast        b;
        b.m_length = 2.2f, b.m_height = 0.55f, b.m_width = 0.42f, b.m_legs = 0.75f, b.m_legR = 0.08f, b.m_neck = 0.4f,
        b.m_neckUp = 0.1f;
        b.m_head = 0.55f, b.m_headR = 0.2f, b.m_tail = 0.7f, b.m_seed = 1401 + v;
        b.m_body = v == 0 ? rgb( 240, 238, 232 ) : v == 1 ? rgb( 140, 90, 54 ) : rgb( 170, 100, 50 );
        b.m_headCol = v == 0 ? rgb( 40, 38, 36 ) : b.m_body;
        b.m_legsCol = v == 0 ? rgb( 230, 226, 220 ) : scaleColor( b.m_body, 0.8f );
        if ( v == 2 ) b.m_lumpy = 0.35f;
        const Vector3 c = beast( m, b );
        if ( v == 0 )
            for ( int i = 0; i < 4; ++i )
                m.blob( Vector3{ ( i % 2 ? 0.3f : -0.3f ), c.y + 0.15f, -0.6f + 0.45f * i },
                        Vector3{ 0.18f, 0.35f, 0.3f }, rgb( 36, 34, 32 ), 1411 + i, 0.3f );
        for ( float x : { -1.0f, 1.0f } )
            m.tube( Vector3{ x * 0.12f, c.y + 0.75f, 1.6f },
                    Vector3{ x * ( v == 2 ? 0.55f : 0.3f ), c.y + 0.95f, 1.55f }, 0.04f, 3, rgb( 230, 220, 200 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Sheep.
        MeshBuilder &m = add( PropKindE::SHEEP, 0.0f );
        Beast        b;
        b.m_length = 1.2f, b.m_height = 0.42f, b.m_width = 0.36f, b.m_legs = 0.42f, b.m_legR = 0.05f, b.m_neck = 0.25f,
        b.m_neckUp = 0.5f;
        b.m_head = 0.32f, b.m_headR = 0.13f, b.m_tail = 0.0f, b.m_lumpy = 0.45f, b.m_seed = 1421 + v;
        b.m_body = v ? rgb( 226, 222, 210 ) : rgb( 240, 238, 230 ), b.m_headCol = rgb( 40, 38, 36 ),
        b.m_legsCol = rgb( 50, 46, 42 );
        beast( m, b );
    }
    for ( int v = 0; v < 3; ++v )
    {   // Horse: bay, white (the Camargue), black.
        MeshBuilder &m = add( PropKindE::HORSE, 0.0f );
        Beast        b;
        b.m_length = 2.0f, b.m_height = 0.45f, b.m_width = 0.32f, b.m_legs = 1.0f, b.m_legR = 0.07f, b.m_neck = 0.9f,
        b.m_neckUp = 0.95f;
        b.m_head = 0.6f, b.m_headR = 0.16f, b.m_tail = 0.8f, b.m_seed = 1431 + v;
        b.m_body = v == 0 ? rgb( 120, 70, 40 ) : v == 1 ? rgb( 236, 234, 228 ) : rgb( 40, 34, 32 );
        b.m_headCol = b.m_body, b.m_legsCol = v == 1 ? rgb( 210, 206, 200 ) : rgb( 40, 30, 26 );
        const Vector3 c = beast( m, b );
        m.box( Vector3{ 0, c.y + 0.9f, 1.25f }, Vector3{ 0.05f, 0.3f, 0.35f },
               v == 1 ? rgb( 230, 230, 226 ) : rgb( 30, 26, 24 ) );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Camel: one hump (dromedary) or two (Bactrian).
        MeshBuilder &m = add( PropKindE::CAMEL, 0.0f );
        Beast        b;
        b.m_length = 2.0f, b.m_height = 0.45f, b.m_width = 0.36f, b.m_legs = 1.3f, b.m_legR = 0.07f, b.m_neck = 1.0f,
        b.m_neckUp = 0.7f;
        b.m_head = 0.6f, b.m_headR = 0.15f, b.m_tail = 0.5f, b.m_seed = 1441 + v;
        b.m_body = v ? rgb( 150, 110, 70 ) : rgb( 200, 160, 110 ), b.m_headCol = b.m_body,
        b.m_legsCol = scaleColor( b.m_body, 0.9f );
        if ( v ) b.m_lumpy = 0.3f;
        const Vector3 c = beast( m, b );
        for ( int i = 0; i < 1 + v; ++i )
            m.blob( Vector3{ 0, c.y + 0.5f, v ? -0.35f + 0.7f * i : 0.0f }, Vector3{ 0.3f, 0.45f, 0.4f }, b.m_body,
                    1451 + i, 0.2f );
    }
    {   // Elephant.
        MeshBuilder &m = add( PropKindE::ELEPHANT, 1.5f );
        Beast        b;
        b.m_length = 3.4f, b.m_height = 1.1f, b.m_width = 0.95f, b.m_legs = 1.3f, b.m_legR = 0.3f, b.m_neck = 0.3f,
        b.m_neckUp = 0.2f;
        b.m_head = 0.9f, b.m_headR = 0.6f, b.m_tail = 0.9f, b.m_seed = 1461;
        b.m_body = rgb( 130, 126, 122 ), b.m_headCol = b.m_body, b.m_legsCol = rgb( 120, 116, 112 );
        const Vector3 c = beast( m, b );
        const Vector3 head = { 0, c.y + 0.6f, 2.2f };
        m.tube( Vector3{ 0, head.y - 0.2f, head.z + 0.5f }, Vector3{ 0, head.y - 1.2f, head.z + 0.9f }, 0.2f, 6,
                b.m_body );
        m.tube( Vector3{ 0, head.y - 1.2f, head.z + 0.9f }, Vector3{ 0, 0.4f, head.z + 0.8f }, 0.14f, 6, b.m_body );
        for ( float x : { -1.0f, 1.0f } )
        {
            m.quad( Vector3{ x * 0.55f, head.y + 0.6f, head.z - 0.2f },
                    Vector3{ x * 0.55f, head.y + 0.6f, head.z + 0.2f },
                    Vector3{ x * 1.4f, head.y - 0.8f, head.z - 0.1f },
                    Vector3{ x * 1.2f, head.y + 0.2f, head.z - 0.8f }, scaleColor( b.m_body, 0.9f ) );
            m.tube( Vector3{ x * 0.3f, head.y - 0.4f, head.z + 0.6f },
                    Vector3{ x * 0.35f, head.y - 0.6f, head.z + 1.4f }, 0.06f, 4, rgb( 240, 236, 220 ) );
        }
    }
    {   // Giraffe.
        MeshBuilder &m = add( PropKindE::GIRAFFE, 0.0f );
        Beast        b;
        b.m_length = 1.8f, b.m_height = 0.45f, b.m_width = 0.32f, b.m_legs = 1.8f, b.m_legR = 0.07f, b.m_neck = 2.2f,
        b.m_neckUp = 1.15f;
        b.m_head = 0.55f, b.m_headR = 0.14f, b.m_tail = 0.8f, b.m_seed = 1471;
        b.m_body = rgb( 214, 170, 100 ), b.m_headCol = b.m_body, b.m_legsCol = rgb( 220, 190, 140 );
        const Vector3 c = beast( m, b );
        for ( int i = 0; i < 8; ++i )
        {
            const float sx = i % 2 ? 0.28f : -0.28f;
            m.blob( Vector3{ sx, c.y + 0.1f * ( i % 3 ) - 0.1f, -0.7f + 0.2f * i }, Vector3{ 0.06f, 0.15f, 0.12f },
                    rgb( 130, 76, 40 ), 1481 + i, 0.3f );
        }
        for ( int i = 0; i < 5; ++i )
            m.blob( Vector3{ 0.15f * ( i % 2 ? 1.0f : -1.0f ), c.y + 0.6f + 0.38f * i, 0.95f + 0.16f * i },
                    Vector3{ 0.05f, 0.1f, 0.08f }, rgb( 130, 76, 40 ), 1491 + i, 0.3f );
    }
    {   // Zebra.
        MeshBuilder &m = add( PropKindE::ZEBRA, 0.0f );
        Beast        b;
        b.m_length = 1.9f, b.m_height = 0.45f, b.m_width = 0.32f, b.m_legs = 0.9f, b.m_legR = 0.07f, b.m_neck = 0.75f,
        b.m_neckUp = 0.85f;
        b.m_head = 0.55f, b.m_headR = 0.15f, b.m_tail = 0.6f, b.m_seed = 1501;
        b.m_body = rgb( 240, 238, 232 ), b.m_headCol = b.m_body, b.m_legsCol = rgb( 220, 218, 212 );
        const Vector3 c = beast( m, b );
        for ( int i = 0; i < 7; ++i )
        {
            const float z = -0.75f + 0.25f * i, k = std::sqrt( std::max( 0.0f, 1.0f - ( z / 0.95f ) * ( z / 0.95f ) ) );
            m.box( Vector3{ 0, c.y, z }, Vector3{ 0.33f * k + 0.01f, 0.46f * k + 0.01f, 0.05f }, rgb( 30, 28, 28 ) );
        }
    }
    {   // Kangaroo, upright on its big hind legs.
        MeshBuilder &m = add( PropKindE::KANGAROO, 0.0f );
        const Color  fur = rgb( 176, 120, 80 );
        m.blob( Vector3{ 0, 1.05f, 0 }, Vector3{ 0.32f, 0.55f, 0.36f }, fur, 1511, 0.1f );
        m.blob( Vector3{ 0, 0.55f, -0.15f }, Vector3{ 0.3f, 0.32f, 0.45f }, fur, 1512, 0.1f );
        for ( float x : { -1.0f, 1.0f } )
        {
            m.box( Vector3{ x * 0.18f, 0.05f, 0.15f }, Vector3{ 0.07f, 0.05f, 0.3f }, scaleColor( fur, 0.8f ) );
            m.tube( Vector3{ x * 0.15f, 1.15f, 0.25f }, Vector3{ x * 0.15f, 0.9f, 0.4f }, 0.04f, 4, fur );
            m.tri( Vector3{ x * 0.08f, 1.85f, 0.15f }, Vector3{ x * 0.12f, 2.15f, 0.1f },
                   Vector3{ x * 0.14f, 1.85f, 0.05f }, fur );
        }
        m.blob( Vector3{ 0, 1.75f, 0.2f }, Vector3{ 0.14f, 0.14f, 0.24f }, fur, 1513, 0.1f );
        m.tube( Vector3{ 0, 0.5f, -0.5f }, Vector3{ 0, 0.05f, -1.3f }, 0.08f, 5, fur );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Llama.
        MeshBuilder &m = add( PropKindE::LLAMA, 0.0f );
        Beast        b;
        b.m_length = 1.5f, b.m_height = 0.4f, b.m_width = 0.3f, b.m_legs = 0.85f, b.m_legR = 0.06f, b.m_neck = 0.9f,
        b.m_neckUp = 1.35f;
        b.m_head = 0.38f, b.m_headR = 0.12f, b.m_tail = 0.2f, b.m_lumpy = 0.35f, b.m_seed = 1521 + v;
        b.m_body = v ? rgb( 236, 232, 222 ) : rgb( 150, 100, 64 ), b.m_headCol = b.m_body,
        b.m_legsCol = scaleColor( b.m_body, 0.85f );
        const Vector3 c = beast( m, b );
        for ( float x : { -1.0f, 1.0f } )
            m.tri( Vector3{ x * 0.05f, c.y + 1.45f, 0.82f }, Vector3{ x * 0.08f, c.y + 1.7f, 0.78f },
                   Vector3{ x * 0.1f, c.y + 1.45f, 0.75f }, b.m_body );
        m.box( Vector3{ 0, c.y + 0.42f, 0 }, Vector3{ 0.32f, 0.04f, 0.3f }, rgb( 200, 40, 60 ) );
    }
    {   // Bison.
        MeshBuilder &m = add( PropKindE::BISON, 0.0f );
        Beast        b;
        b.m_length = 2.6f, b.m_height = 0.7f, b.m_width = 0.55f, b.m_legs = 0.75f, b.m_legR = 0.11f, b.m_neck = 0.3f,
        b.m_neckUp = -0.3f;
        b.m_head = 0.6f, b.m_headR = 0.3f, b.m_tail = 0.5f, b.m_lumpy = 0.3f, b.m_seed = 1531;
        b.m_body = rgb( 96, 66, 44 ), b.m_headCol = rgb( 60, 40, 28 ), b.m_legsCol = rgb( 60, 42, 30 );
        const Vector3 c = beast( m, b );
        m.blob( Vector3{ 0, c.y + 0.5f, 0.6f }, Vector3{ 0.55f, 0.6f, 0.6f }, rgb( 80, 54, 36 ), 1532, 0.4f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Moose with palmate antlers, reindeer with branched ones.
        const PropKindE kind = v ? PropKindE::REINDEER : PropKindE::MOOSE;
        MeshBuilder    &m = add( kind, 0.0f );
        Beast           b;
        const float     k = v ? 0.7f : 1.0f;
        b.m_length = 2.4f * k, b.m_height = 0.6f * k, b.m_width = 0.42f * k, b.m_legs = 1.3f * k, b.m_legR = 0.08f * k,
        b.m_neck = 0.5f * k;
        b.m_neckUp = 0.6f, b.m_head = 0.7f * k, b.m_headR = 0.2f * k, b.m_tail = 0.2f, b.m_seed = 1541 + v;
        b.m_body = v ? rgb( 130, 116, 100 ) : rgb( 70, 50, 36 ), b.m_headCol = b.m_body,
        b.m_legsCol = v ? rgb( 210, 204, 196 ) : rgb( 90, 74, 60 );
        const Vector3 c = beast( m, b );
        const Vector3 h = { 0, c.y + b.m_height * 0.35f + std::sin( 0.6f ) * b.m_neck + b.m_headR,
                            b.m_length * 0.42f + std::cos( 0.6f ) * b.m_neck };
        for ( float x : { -1.0f, 1.0f } )
        {
            if ( v == 0 )
                m.quad( Vector3{ x * 0.15f, h.y + 0.1f, h.z }, Vector3{ x * 0.9f, h.y + 0.5f, h.z - 0.3f },
                        Vector3{ x * 1.0f, h.y + 0.6f, h.z + 0.3f }, Vector3{ x * 0.2f, h.y + 0.15f, h.z + 0.2f },
                        rgb( 200, 186, 160 ) );
            else
            {
                m.tube( Vector3{ x * 0.1f, h.y, h.z }, Vector3{ x * 0.35f, h.y + 0.8f, h.z - 0.3f }, 0.03f, 3,
                        rgb( 200, 186, 160 ) );
                m.tube( Vector3{ x * 0.3f, h.y + 0.5f, h.z - 0.2f }, Vector3{ x * 0.45f, h.y + 0.7f, h.z + 0.2f },
                        0.025f, 3, rgb( 200, 186, 160 ) );
            }
        }
    }
    {   // Yak.
        MeshBuilder &m = add( PropKindE::YAK, 0.0f );
        Beast        b;
        b.m_length = 2.2f, b.m_height = 0.6f, b.m_width = 0.5f, b.m_legs = 0.6f, b.m_legR = 0.1f, b.m_neck = 0.3f,
        b.m_neckUp = 0.0f;
        b.m_head = 0.5f, b.m_headR = 0.24f, b.m_tail = 0.6f, b.m_lumpy = 0.4f, b.m_seed = 1551;
        b.m_body = rgb( 40, 36, 34 ), b.m_headCol = rgb( 50, 44, 40 ), b.m_legsCol = rgb( 40, 36, 34 );
        const Vector3 c = beast( m, b );
        m.taperedBox( Vector3{ 0, 0.25f, 0 }, Vector2{ 0.55f, 1.0f }, Vector3{ 0, c.y, 0 }, Vector2{ 0.45f, 0.95f },
                      rgb( 34, 30, 28 ), {}, false );
        for ( float x : { -1.0f, 1.0f } )
            m.tube( Vector3{ x * 0.15f, c.y + 0.4f, 1.4f }, Vector3{ x * 0.5f, c.y + 0.7f, 1.3f }, 0.05f, 4,
                    rgb( 230, 220, 200 ) );
    }
    {   // Panda, sitting.
        MeshBuilder &m = add( PropKindE::PANDA, 0.0f );
        const Color  w = rgb( 240, 238, 232 ), k = rgb( 30, 28, 28 );
        m.blob( Vector3{ 0, 0.6f, 0 }, Vector3{ 0.55f, 0.65f, 0.5f }, w, 1561, 0.15f );
        m.box( Vector3{ 0, 0.95f, 0 }, Vector3{ 0.56f, 0.18f, 0.5f }, k );
        for ( float x : { -1.0f, 1.0f } )
        {
            m.blob( Vector3{ x * 0.3f, 0.2f, 0.4f }, Vector3{ 0.18f, 0.18f, 0.3f }, k, 1562, 0.2f );
            m.tube( Vector3{ x * 0.4f, 1.0f, 0.2f }, Vector3{ x * 0.3f, 0.7f, 0.5f }, 0.12f, 5, k );
            m.blob( Vector3{ x * 0.22f, 1.75f, 0.0f }, Vector3{ 0.1f, 0.1f, 0.06f }, k, 1563, 0.0f );
            m.blob( Vector3{ x * 0.12f, 1.48f, 0.3f }, Vector3{ 0.08f, 0.06f, 0.04f }, k, 1564, 0.0f );
        }
        m.blob( Vector3{ 0, 1.48f, 0.08f }, Vector3{ 0.32f, 0.28f, 0.28f }, w, 1565, 0.1f );
        m.cylinder( Vector3{ 0.3f, 0.9f, 0.5f }, 0.04f, 0.04f, 0.9f, 4, rgb( 110, 160, 70 ) );
    }
    {   // Flamingo.
        MeshBuilder &m = add( PropKindE::FLAMINGO, 0.0f );
        const Color  pink = rgb( 240, 130, 140 );
        m.blob( Vector3{ 0, 1.0f, 0 }, Vector3{ 0.18f, 0.16f, 0.3f }, pink, 1571, 0.1f );
        m.tube( Vector3{ 0.04f, 0.9f, 0 }, Vector3{ 0.04f, 0, 0.02f }, 0.015f, 3, pink );
        m.tube( Vector3{ -0.04f, 0.9f, 0 }, Vector3{ -0.06f, 0.5f, -0.1f }, 0.015f, 3, pink );
        m.tube( Vector3{ 0, 1.1f, 0.25f }, Vector3{ 0, 1.45f, 0.35f }, 0.03f, 4, pink );
        m.tube( Vector3{ 0, 1.45f, 0.35f }, Vector3{ 0, 1.6f, 0.25f }, 0.03f, 4, pink );
        m.blob( Vector3{ 0, 1.62f, 0.3f }, Vector3{ 0.05f, 0.05f, 0.07f }, pink, 1572, 0.0f );
        m.tube( Vector3{ 0, 1.6f, 0.36f }, Vector3{ 0, 1.52f, 0.44f }, 0.02f, 3, rgb( 30, 30, 30 ) );
    }

    // ---- Landmarks: one variant per monument (rules pick theirs with .v(n)) -------------------------------------------
    const Color sandstone = rgb( 200, 178, 130 ), marble = rgb( 230, 224, 210 ), copper = rgb( 96, 156, 130 ),
                gold = rgb( 222, 178, 70 );
    {   // 0 Big Ben and a wing of the parliament.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        m.box( Vector3{ 0, 27.0f, 0 }, Vector3{ 4.0f, 27.0f, 4.0f }, sandstone );
        m.box( Vector3{ 0, 47.0f, 0 }, Vector3{ 4.6f, 4.0f, 4.6f }, rgb( 186, 160, 110 ) );
        for ( int i = 0; i < 4; ++i )
        {
            const float   a = Tau * i / 4.0f;
            const Vector3 c = { std::cos( a ) * 4.65f, 47.0f, std::sin( a ) * 4.65f };
            m.box( c,
                   Vector3{ std::abs( std::cos( a ) ) > 0.5f ? 0.05f : 2.6f, 2.6f,
                            std::abs( std::cos( a ) ) > 0.5f ? 2.6f : 0.05f },
                   rgb( 240, 236, 220 ), Mat{ 0.3f, 0.0f } );
        }
        m.box( Vector3{ 4.72f, 47.5f, 0 }, Vector3{ 0.04f, 1.8f, 0.12f }, rgb( 30, 30, 30 ) );
        m.box( Vector3{ 4.72f, 47.0f, 0.6f }, Vector3{ 0.04f, 0.12f, 1.0f }, rgb( 30, 30, 30 ) );
        m.taperedBox( Vector3{ 0, 51.0f, 0 }, Vector2{ 4.2f, 4.2f }, Vector3{ 0, 58.0f, 0 }, Vector2{ 2.5f, 2.5f },
                      rgb( 70, 84, 80 ) );
        m.taperedBox( Vector3{ 0, 58.0f, 0 }, Vector2{ 2.5f, 2.5f }, Vector3{ 0, 70.0f, 0 }, Vector2{ 0.1f, 0.1f },
                      rgb( 70, 84, 80 ) );
        m.box( Vector3{ 0, 11.0f, 34.0f }, Vector3{ 9.0f, 11.0f, 30.0f }, sandstone );
        for ( int i = 0; i < 12; ++i )
            m.cylinder( Vector3{ 9.0f, 22.0f, 6.0f + 5.0f * i }, 0.6f, 0.05f, 4.0f, 6, rgb( 186, 160, 110 ) );
        panes( m, Vector3{ 9.0f, 0, 4.0f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 60.0f, 22.0f, 2001, 0.4f, 4.0f,
               2.0f );
    }
    {   // 1 Tower Bridge.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  stone = rgb( 206, 196, 176 ), blue = rgb( 70, 120, 170 );
        for ( float z : { -30.0f, 30.0f } )
        {
            m.box( Vector3{ 0, 22.0f, z }, Vector3{ 7.0f, 22.0f, 7.0f }, stone );
            for ( float x : { -6.0f, 6.0f } )
                for ( float dz : { -6.0f, 6.0f } )
                {
                    m.cylinder( Vector3{ x, 44.0f, z + dz }, 1.4f, 1.4f, 4.0f, 8, stone );
                    m.cylinder( Vector3{ x, 48.0f, z + dz }, 1.6f, 0.05f, 5.0f, 8, rgb( 80, 90, 100 ) );
                }
            m.taperedBox( Vector3{ 0, 44.0f, z }, Vector2{ 5.0f, 5.0f }, Vector3{ 0, 52.0f, z }, Vector2{ 0.1f, 0.1f },
                          rgb( 80, 90, 100 ) );
            for ( float x : { -1.0f, 1.0f } )
                m.tube( Vector3{ x * 6.0f, 30.0f, z + ( z > 0 ? 6.0f : -6.0f ) },
                        Vector3{ x * 6.0f, 9.0f, z + ( z > 0 ? 50.0f : -50.0f ) }, 0.6f, 6, blue );
        }
        m.box( Vector3{ 0, 8.5f, 0 }, Vector3{ 6.0f, 0.8f, 80.0f }, blue );
        for ( float x : { -5.0f, 5.0f } ) m.box( Vector3{ x, 37.0f, 0 }, Vector3{ 1.2f, 1.6f, 23.0f }, blue );
    }
    {   // 2 Colosseum: an elliptical ring in three arcaded tiers, one side fallen.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  trav = rgb( 214, 198, 168 ), shadow = rgb( 70, 60, 50 );
        const int    n = 40;
        for ( int i = 0; i < n; ++i )
        {
            const float a0 = Tau * i / n, a1 = Tau * ( i + 1 ) / n;
            const float h = ( i > 26 && i < 36 ) ? 30.0f : 46.0f;
            for ( float r : { 1.0f, 0.82f } )
            {
                const Vector3 p0 = { std::cos( a0 ) * 47.0f * r, 0, std::sin( a0 ) * 39.0f * r };
                const Vector3 p1 = { std::cos( a1 ) * 47.0f * r, 0, std::sin( a1 ) * 39.0f * r };
                m.quad( p0, p1, Vector3{ p1.x, h, p1.z }, Vector3{ p0.x, h, p0.z },
                        r < 1.0f ? scaleColor( trav, 0.8f ) : trav );
            }
            const Vector3 o0 = { std::cos( a0 ) * 47.2f, 0, std::sin( a0 ) * 39.2f },
                          o1 = { std::cos( a1 ) * 47.2f, 0, std::sin( a1 ) * 39.2f };
            for ( int t = 0; t < 3; ++t )
            {
                const float y0 = 3.0f + 11.0f * t, y1 = y0 + 7.0f;
                if ( y1 > h ) break;
                const Vector3 q0 = Vector3Lerp( o0, o1, 0.25f ), q1 = Vector3Lerp( o0, o1, 0.75f );
                m.quad( Vector3{ q0.x, y0, q0.z }, Vector3{ q1.x, y0, q1.z }, Vector3{ q1.x, y1, q1.z },
                        Vector3{ q0.x, y1, q0.z }, shadow );
            }
        }
    }
    {   // 3 The Leaning Tower of Pisa and its cathedral.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const float  lean = std::tan( 0.07f );
        for ( int i = 0; i < 8; ++i )
        {
            const float y = 7.0f * i;
            m.cylinder( Vector3{ y * lean, y, 0 }, 7.8f - ( i == 7 ? 2.0f : 0.0f ), 7.8f - ( i == 7 ? 2.0f : 0.0f ),
                        7.0f, 16, marble );
            m.cylinder( Vector3{ ( y + 6.6f ) * lean, y + 6.6f, 0 }, 8.3f, 8.3f, 0.4f, 16, rgb( 214, 206, 190 ) );
        }
        m.box( Vector3{ 10.0f, 8.0f, 45.0f }, Vector3{ 9.0f, 8.0f, 28.0f }, marble );
        gable( m, Vector3{ 10.0f, 16.0f, 45.0f }, 9.0f, 28.0f, 5.0f, 0.3f, rgb( 160, 90, 70 ), marble );
        m.cylinder( Vector3{ 10.0f, 16.0f, 40.0f }, 6.0f, 6.0f, 4.0f, 12, marble );
        m.blob( Vector3{ 10.0f, 20.0f, 40.0f }, Vector3{ 6.0f, 5.0f, 6.0f }, rgb( 200, 190, 170 ), 2011, 0.0f );
    }
    {   // 4 A basilica's dome over a columned front (St Peter's, Yamoussoukro).
        MeshBuilder &m = keyed( PropKindE::LANDMARK, 0.0f, FacingE::ROAD, rgb( 226, 214, 186 ), rgb( 120, 136, 150 ) );
        m.box( Vector3{ 0, 14.0f, 0 }, Vector3{ 30.0f, 14.0f, 40.0f }, keyA( 1.0f ) );
        m.cylinder( Vector3{ -8.0f, 28.0f, 0 }, 18.0f, 18.0f, 14.0f, 20, keyA( 0.95f ) );
        m.blob( Vector3{ -8.0f, 42.0f, 0 }, Vector3{ 18.0f, 20.0f, 18.0f }, keyB( 1.0f ), 2021, 0.0f );
        m.cylinder( Vector3{ -8.0f, 60.0f, 0 }, 3.0f, 2.5f, 8.0f, 10, keyA( 1.0f ) );
        m.cylinder( Vector3{ -8.0f, 68.0f, 0 }, 2.5f, 0.1f, 5.0f, 10, keyB( 0.9f ) );
        for ( int i = 0; i < 10; ++i )
            m.cylinder( Vector3{ 31.5f, 0, -27.0f + 6.0f * i }, 1.3f, 1.2f, 26.0f, 10, keyA( 1.0f ) );
        m.box( Vector3{ 32.0f, 27.0f, 0 }, Vector3{ 2.5f, 2.0f, 31.0f }, keyA( 0.95f ) );
    }
    {   // 5 Sagrada Familia: a crowd of perforated spires.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  s = rgb( 190, 170, 140 );
        m.box( Vector3{ 0, 18.0f, 0 }, Vector3{ 22.0f, 18.0f, 40.0f }, s );
        const float spires[][3] = { { 18, -30, 98 },  { 18, -18, 105 },  { 18, 18, 105 },  { 18, 30, 98 },
                                    { -18, -30, 95 }, { -18, -18, 100 }, { -18, 18, 100 }, { -18, 30, 95 },
                                    { 0, 0, 160 },    { 0, -12, 130 },   { 0, 12, 130 } };
        for ( const auto &sp : spires )
        {
            m.cylinder( Vector3{ sp[0], 20.0f, sp[1] }, 3.5f, 0.6f, sp[2] - 20.0f, 8, s );
            m.blob( Vector3{ sp[0], sp[2], sp[1] }, Vector3{ 1.4f, 2.0f, 1.4f }, rgb( 220, 120, 60 ), 2031, 0.3f );
            for ( int i = 0; i < 6; ++i )
                m.cylinder( Vector3{ sp[0], 26.0f + 10.0f * i, sp[1] }, 3.4f - 0.3f * i, 3.4f - 0.3f * i, 0.6f, 8,
                            scaleColor( s, 0.75f ) );
        }
    }
    {   // 6 Neuschwanstein on its crag.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  wall = rgb( 238, 234, 226 ), roof = rgb( 80, 96, 120 );
        m.blob( Vector3{ 0, 4.0f, 0 }, Vector3{ 40.0f, 20.0f, 34.0f }, rgb( 120, 116, 104 ), 2041, 0.4f );
        m.box( Vector3{ 0, 34.0f, 0 }, Vector3{ 9.0f, 14.0f, 26.0f }, wall );
        gable( m, Vector3{ 0, 48.0f, 0 }, 9.0f, 26.0f, 8.0f, 0.3f, roof, wall );
        const float towers[][4] = { { 10, -24, 62, 3.5f },
                                    { 10, 22, 52, 3.0f },
                                    { -10, -20, 56, 3.2f },
                                    { -8, 26, 46, 2.6f },
                                    { 12, 0, 46, 2.4f } };
        for ( const auto &t : towers )
        {
            m.cylinder( Vector3{ t[0], 20.0f, t[1] }, t[3], t[3], t[2] - 20.0f, 12, wall );
            m.cylinder( Vector3{ t[0], t[2], t[1] }, t[3] + 0.4f, 0.1f, t[3] * 2.6f, 12, roof );
            panes( m, Vector3{ t[0] + t[3], 20.0f, t[1] - 1.0f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 2.0f,
                   t[2] - 22.0f, 2042, 0.2f, 4.0f, 3.0f );
        }
        panes( m, Vector3{ 9.0f, 20.0f, -26.0f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 52.0f, 28.0f, 2043, 0.3f,
               4.0f, 2.0f );
    }
    {   // 7 Mont-Saint-Michel: the rock, the town, the abbey and its spire.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        m.blob( Vector3{ 0, 8.0f, 0 }, Vector3{ 60.0f, 40.0f, 55.0f }, rgb( 150, 140, 124 ), 2051, 0.3f );
        Rng r( 2052 );
        for ( int i = 0; i < 24; ++i )
        {
            const float a = r.range( 0.0f, Tau ), rr = r.range( 25.0f, 50.0f ), y = 48.0f - rr * 0.8f;
            m.box( Vector3{ std::cos( a ) * rr, y, std::sin( a ) * rr }, Vector3{ 3.0f, 4.0f, 3.5f },
                   rgb( 170, 160, 144 ) );
            hip( m, Vector3{ std::cos( a ) * rr, y + 4.0f, std::sin( a ) * rr }, 3.0f, 3.5f, 2.5f, 0.3f, slate );
        }
        m.box( Vector3{ 0, 54.0f, 0 }, Vector3{ 10.0f, 10.0f, 22.0f }, rgb( 168, 158, 140 ) );
        gable( m, Vector3{ 0, 64.0f, 0 }, 10.0f, 22.0f, 7.0f, 0.3f, slate, rgb( 168, 158, 140 ) );
        m.box( Vector3{ 0, 74.0f, 0 }, Vector3{ 3.5f, 6.0f, 3.5f }, rgb( 168, 158, 140 ) );
        m.cylinder( Vector3{ 0, 80.0f, 0 }, 3.0f, 0.1f, 20.0f, 8, slate );
        m.blob( Vector3{ 0, 101.0f, 0 }, Vector3{ 0.5f, 1.2f, 0.5f }, gold, 2053, 0.0f, Glow );
    }
    {   // 8 Brandenburg Gate.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        for ( int i = 0; i < 6; ++i )
            m.cylinder( Vector3{ 0, 1.5f, -16.0f + 6.4f * i }, 0.9f, 0.8f, 14.0f, 10, sandstone );
        m.box( Vector3{ 0, 0.75f, 0 }, Vector3{ 4.5f, 0.75f, 19.0f }, sandstone );
        m.box( Vector3{ 0, 17.5f, 0 }, Vector3{ 4.5f, 2.0f, 19.0f }, sandstone );
        m.box( Vector3{ 0, 21.0f, 0 }, Vector3{ 3.0f, 1.5f, 8.0f }, sandstone );
        m.box( Vector3{ 0, 23.5f, 0 }, Vector3{ 1.2f, 1.0f, 2.6f }, copper );
        for ( int i = 0; i < 4; ++i )
            m.blob( Vector3{ 0.5f, 23.4f, -1.8f + 1.2f * i }, Vector3{ 0.5f, 1.0f, 0.4f }, copper, 2061 + i, 0.3f );
        m.blob( Vector3{ 0, 26.0f, 0 }, Vector3{ 0.5f, 1.5f, 0.6f }, copper, 2065, 0.2f );
    }
    {   // 9 The Acropolis: the rock and the Parthenon.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        m.blob( Vector3{ 0, 0, 0 }, Vector3{ 90.0f, 32.0f, 60.0f }, rgb( 168, 156, 134 ), 2071, 0.25f );
        m.taperedBox( Vector3{ 0, 26.0f, 0 }, Vector2{ 60.0f, 40.0f }, Vector3{ 0, 30.0f, 0 }, Vector2{ 56.0f, 38.0f },
                      rgb( 176, 164, 140 ) );
        m.box( Vector3{ 0, 31.0f, 0 }, Vector3{ 16.0f, 1.0f, 34.0f }, marble );
        for ( int i = 0; i < 8; ++i )
            for ( float x : { -14.5f, 14.5f } )
                if ( !( x > 0 && i == 5 ) )
                    m.cylinder( Vector3{ x, 32.0f, -31.0f + 8.9f * i }, 1.0f, 0.85f, 10.5f, 10, marble );
        for ( int i = 0; i < 8; ++i )
            m.cylinder( Vector3{ -14.5f + 4.15f * i, 32.0f, -31.0f }, 1.0f, 0.85f, 10.5f, 10, marble );
        m.box( Vector3{ 0, 43.5f, -18.0f }, Vector3{ 16.0f, 1.0f, 16.0f }, marble );
        m.tri( Vector3{ -16.0f, 44.5f, -32.0f }, Vector3{ 16.0f, 44.5f, -32.0f }, Vector3{ 0, 49.0f, -32.0f }, marble );
    }
    {   // 10 St Basil's: onion domes in every colour.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  brick = rgb( 168, 64, 48 );
        m.box( Vector3{ 0, 8.0f, 0 }, Vector3{ 16.0f, 8.0f, 16.0f }, brick );
        m.cylinder( Vector3{ 0, 16.0f, 0 }, 5.0f, 4.0f, 22.0f, 8, brick );
        m.cylinder( Vector3{ 0, 38.0f, 0 }, 4.2f, 0.1f, 12.0f, 8, rgb( 60, 140, 90 ) );
        m.blob( Vector3{ 0, 51.0f, 0 }, Vector3{ 1.4f, 1.6f, 1.4f }, gold, 2081, 0.0f, Shiny );
        const Color domes[8] = { rgb( 230, 60, 60 ),  rgb( 60, 150, 90 ),  rgb( 60, 110, 200 ), rgb( 240, 200, 60 ),
                                 rgb( 230, 120, 40 ), rgb( 80, 180, 170 ), rgb( 200, 60, 140 ), rgb( 120, 180, 60 ) };
        for ( int i = 0; i < 8; ++i )
        {
            const float   a = Tau * i / 8.0f, r = i % 2 ? 11.0f : 12.5f, h = i % 2 ? 22.0f : 28.0f;
            const Vector3 b = { std::cos( a ) * r, 16.0f, std::sin( a ) * r };
            m.cylinder( b, 3.0f, 3.0f, h - 16.0f, 8, i % 2 ? rgb( 236, 220, 190 ) : brick );
            m.blob( Vector3{ b.x, h + 2.5f, b.z }, Vector3{ 3.6f, 3.4f, 3.6f }, domes[i], 2082 + i, 0.15f, Shiny );
            m.cylinder( Vector3{ b.x, h + 5.2f, b.z }, 1.4f, 0.05f, 3.0f, 8, domes[i] );
        }
    }
    {   // 11 A statue with open arms on its pedestal (Christ the Redeemer, Cristo Rei).
        MeshBuilder &m = keyed( PropKindE::LANDMARK, 0.0f, FacingE::ROAD, rgb( 236, 232, 222 ) );
        m.box( Vector3{ 0, 4.0f, 0 }, Vector3{ 3.5f, 4.0f, 3.5f }, keyA( 0.85f ) );
        m.taperedBox( Vector3{ 0, 8.0f, 0 }, Vector2{ 2.6f, 2.8f }, Vector3{ 0, 26.0f, 0 }, Vector2{ 1.6f, 2.2f },
                      keyA( 1.0f ) );
        m.box( Vector3{ 0, 24.5f, 0 }, Vector3{ 1.4f, 1.3f, 14.0f }, keyA( 0.97f ) );
        m.blob( Vector3{ 0.2f, 28.0f, 0 }, Vector3{ 1.3f, 1.6f, 1.3f }, keyA( 1.0f ), 2091, 0.0f );
    }
    {   // 12 The Statue of Liberty.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        m.taperedBox( Vector3{ 0, 0, 0 }, Vector2{ 12.0f, 12.0f }, Vector3{ 0, 27.0f, 0 }, Vector2{ 8.0f, 8.0f },
                      rgb( 170, 160, 140 ) );
        m.taperedBox( Vector3{ 0, 27.0f, 0 }, Vector2{ 4.5f, 5.0f }, Vector3{ 0, 52.0f, 0 }, Vector2{ 2.4f, 3.0f },
                      copper );
        m.blob( Vector3{ 0.4f, 55.0f, 0 }, Vector3{ 2.0f, 2.6f, 2.0f }, copper, 2101, 0.1f );
        for ( int i = 0; i < 7; ++i )
        {
            const float a = -1.2f + 0.4f * i;
            m.tube( Vector3{ 0.4f, 57.0f, 0 },
                    Vector3{ 0.4f + std::cos( a ) * 0.5f, 57.0f + std::cos( a * 0.5f ) * 3.0f, std::sin( a ) * 3.0f },
                    0.15f, 3, copper );
        }
        m.tube( Vector3{ 0, 49.0f, -2.5f }, Vector3{ 0.5f, 64.0f, -4.0f }, 0.9f, 6, copper );
        m.cylinder( Vector3{ 0.5f, 64.0f, -4.0f }, 1.0f, 1.4f, 2.0f, 8, copper );
        m.blob( Vector3{ 0.5f, 67.0f, -4.0f }, Vector3{ 0.9f, 1.6f, 0.9f }, rgb( 255, 190, 80 ), 2102, 0.3f, Glow );
        m.box( Vector3{ 2.3f, 44.0f, 2.0f }, Vector3{ 0.4f, 2.5f, 1.6f }, copper );
    }
    {   // 13 Sydney Opera House: shells on a podium.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  shell = rgb( 244, 242, 234 );
        m.box( Vector3{ 0, 4.0f, 0 }, Vector3{ 30.0f, 4.0f, 60.0f }, rgb( 196, 170, 140 ) );
        auto sail = [&]( float z, float h, float x, float dir )
        {
            const Vector3 base0 = { x - 10.0f, 8.0f, z }, base1 = { x + 10.0f, 8.0f, z },
                          tip = { x, 8.0f + h, z + dir * h * 0.5f };
            const Vector3 back = { x, 8.0f, z + dir * h * 0.9f };
            m.tri( base0, base1, tip, shell, Shiny );
            m.tri( base0, tip, back, scaleColor( shell, 0.93f ), Shiny );
            m.tri( base1, back, tip, scaleColor( shell, 0.9f ), Shiny );
        };
        for ( float x : { -12.0f, 12.0f } )
        {
            sail( -40.0f, 40.0f, x, 1.0f );
            sail( -20.0f, 34.0f, x, 1.0f );
            sail( 0.0f, 26.0f, x, 1.0f );
            sail( 30.0f, 18.0f, x, -1.0f );
        }
    }
    {   // 14 Petronas Towers.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  steel2 = rgb( 196, 202, 210 );
        for ( float z : { -22.0f, 22.0f } )
        {
            float y = 0.0f;
            for ( int i = 0; i < 9; ++i )
            {
                const float r = 14.0f - 1.1f * i, h = 40.0f - 2.5f * i;
                m.cylinder( Vector3{ 0, y, z }, r, r * 0.97f, h, 16, steel2, Shiny );
                y += h;
            }
            m.cylinder( Vector3{ 0, y, z }, 3.0f, 0.2f, 70.0f, 8, steel2, Shiny );
        }
        m.box( Vector3{ 0, 170.0f, 0 }, Vector3{ 2.5f, 3.0f, 12.0f }, steel2, Shiny );
        m.tube( Vector3{ 0, 140.0f, -12.0f }, Vector3{ 0, 168.0f, 0 }, 0.6f, 5, steel2 );
        m.tube( Vector3{ 0, 140.0f, 12.0f }, Vector3{ 0, 168.0f, 0 }, 0.6f, 5, steel2 );
    }
    {   // 15 Space Needle.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  w = rgb( 236, 236, 236 );
        for ( int i = 0; i < 3; ++i )
        {
            const float   a = Tau * i / 3.0f;
            const Vector3 foot = { std::cos( a ) * 14.0f, 0, std::sin( a ) * 14.0f },
                          waist = { std::cos( a ) * 3.0f, 110.0f, std::sin( a ) * 3.0f };
            const Vector3 top = { std::cos( a ) * 6.0f, 150.0f, std::sin( a ) * 6.0f };
            m.tube( foot, waist, 1.2f, 6, w );
            m.tube( waist, top, 1.0f, 6, w );
        }
        m.cylinder( Vector3{ 0, 150.0f, 0 }, 7.0f, 22.0f, 6.0f, 20, w );
        m.cylinder( Vector3{ 0, 156.0f, 0 }, 22.0f, 14.0f, 4.0f, 20, rgb( 230, 140, 60 ) );
        m.cylinder( Vector3{ 0, 160.0f, 0 }, 14.0f, 4.0f, 4.0f, 20, w );
        m.cylinder( Vector3{ 0, 164.0f, 0 }, 1.0f, 0.1f, 20.0f, 6, w );
    }
    {   // 16 The Capitol.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  w = rgb( 240, 238, 232 );
        m.box( Vector3{ 0, 12.0f, 0 }, Vector3{ 20.0f, 12.0f, 110.0f }, w );
        m.cylinder( Vector3{ 0, 24.0f, 0 }, 22.0f, 22.0f, 6.0f, 24, w );
        for ( int i = 0; i < 24; ++i )
        {
            const float a = Tau * i / 24.0f;
            m.cylinder( Vector3{ std::cos( a ) * 20.0f, 30.0f, std::sin( a ) * 20.0f }, 0.8f, 0.8f, 12.0f, 6, w );
        }
        m.cylinder( Vector3{ 0, 30.0f, 0 }, 18.0f, 18.0f, 12.0f, 24, scaleColor( w, 0.9f ) );
        m.blob( Vector3{ 0, 42.0f, 0 }, Vector3{ 19.0f, 18.0f, 19.0f }, w, 2111, 0.0f );
        m.cylinder( Vector3{ 0, 60.0f, 0 }, 3.0f, 3.0f, 8.0f, 12, w );
        m.blob( Vector3{ 0, 70.0f, 0 }, Vector3{ 0.8f, 2.0f, 0.8f }, rgb( 70, 70, 70 ), 2112, 0.0f );
        for ( int i = 0; i < 8; ++i ) m.cylinder( Vector3{ 21.0f, 0, -14.0f + 4.0f * i }, 0.9f, 0.8f, 22.0f, 8, w );
    }
    {   // 17 Obelisk (Washington, Luxor).
        MeshBuilder &m = keyed( PropKindE::LANDMARK, 0.0f, FacingE::ROAD, rgb( 236, 232, 222 ) );
        m.taperedBox( Vector3{ 0, 0, 0 }, Vector2{ 8.4f, 8.4f }, Vector3{ 0, 152.0f, 0 }, Vector2{ 5.2f, 5.2f },
                      keyA( 1.0f ), {}, false );
        m.taperedBox( Vector3{ 0, 152.0f, 0 }, Vector2{ 5.2f, 5.2f }, Vector3{ 0, 169.0f, 0 }, Vector2{ 0.1f, 0.1f },
                      keyA( 0.95f ) );
    }
    {   // 18 HOLLYWOOD on its hill.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        m.blob( Vector3{ -30.0f, 0, 0 }, Vector3{ 70.0f, 40.0f, 80.0f }, rgb( 150, 140, 96 ), 2121, 0.3f );
        static const char *glyphs[6][5] = {
            { "101", "101", "111", "101", "101" }, { "111", "101", "101", "101", "111" },
            { "100", "100", "100", "100", "111" }, { "101", "101", "010", "010", "010" },
            { "101", "101", "101", "111", "101" }, { "110", "101", "101", "101", "110" }
        };
        const int word[9] = { 0, 1, 2, 2, 3, 4, 1, 1, 5 };
        for ( int l = 0; l < 9; ++l )
            for ( int row = 0; row < 5; ++row )
                for ( int col = 0; col < 3; ++col )
                    if ( glyphs[word[l]][row][col] == '1' )
                        m.box( Vector3{ 6.0f, 22.0f + ( 4 - row ) * 3.0f, -40.0f + l * 10.0f + col * 3.0f },
                               Vector3{ 0.3f, 1.5f, 1.5f }, rgb( 246, 246, 244 ) );
    }
    {   // 19 Kuwait Towers.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  w = rgb( 230, 232, 236 ), blue = rgb( 40, 100, 150 );
        m.cylinder( Vector3{ 0, 0, 0 }, 2.0f, 1.0f, 187.0f, 10, w );
        m.blob( Vector3{ 0, 82.0f, 0 }, Vector3{ 16.0f, 16.0f, 16.0f }, blue, 2131, 0.0f, Shiny );
        m.blob( Vector3{ 0, 122.0f, 0 }, Vector3{ 10.0f, 10.0f, 10.0f }, blue, 2132, 0.0f, Shiny );
        m.cylinder( Vector3{ 0, 0, 30.0f }, 1.6f, 0.8f, 147.0f, 10, w );
        m.blob( Vector3{ 0, 70.0f, 30.0f }, Vector3{ 14.0f, 14.0f, 14.0f }, blue, 2133, 0.0f, Shiny );
        m.cylinder( Vector3{ 0, 0, -22.0f }, 1.2f, 0.4f, 113.0f, 10, w );
    }
    {   // 20 A victory column with a golden figure (the Angel of Independence, the Siegessaeule).
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        m.box( Vector3{ 0, 4.0f, 0 }, Vector3{ 8.0f, 4.0f, 8.0f }, marble );
        m.cylinder( Vector3{ 0, 8.0f, 0 }, 2.6f, 2.2f, 36.0f, 12, marble );
        m.cylinder( Vector3{ 0, 44.0f, 0 }, 3.2f, 3.2f, 1.2f, 12, marble );
        m.taperedBox( Vector3{ 0, 45.0f, 0 }, Vector2{ 0.7f, 0.9f }, Vector3{ 0, 50.0f, 0 }, Vector2{ 0.4f, 0.5f },
                      gold, Shiny );
        for ( float s : { -1.0f, 1.0f } )
            m.tri( Vector3{ 0, 48.0f, 0 }, Vector3{ -0.5f, 52.0f, s * 3.0f }, Vector3{ -0.3f, 46.0f, s * 1.5f }, gold,
                   Shiny );
        m.blob( Vector3{ 0, 50.8f, 0 }, Vector3{ 0.4f, 0.5f, 0.4f }, gold, 2141, 0.0f, Shiny );
    }
    {   // 21 The Colossi of Memnon: two seated kings.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        for ( float z : { -10.0f, 10.0f } )
        {
            m.box( Vector3{ 0, 2.0f, z }, Vector3{ 4.0f, 2.0f, 3.5f }, sandstone );
            m.box( Vector3{ -0.5f, 7.5f, z }, Vector3{ 3.0f, 3.5f, 3.0f }, scaleColor( sandstone, 0.9f ) );
            m.box( Vector3{ 2.0f, 6.0f, z }, Vector3{ 2.0f, 1.0f, 2.4f }, scaleColor( sandstone, 0.85f ) );
            m.taperedBox( Vector3{ -1.0f, 11.0f, z }, Vector2{ 2.0f, 2.6f }, Vector3{ -1.0f, 17.0f, z },
                          Vector2{ 1.5f, 1.8f }, sandstone );
            m.blob( Vector3{ -0.8f, 18.5f, z }, Vector3{ 1.4f, 1.8f, 1.6f }, scaleColor( sandstone, 0.9f ), 2151,
                    0.3f );
        }
    }
    {   // 22 The Sphinx.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  lime = rgb( 210, 180, 130 );
        m.blob( Vector3{ -10.0f, 6.0f, 0 }, Vector3{ 28.0f, 7.0f, 8.0f }, lime, 2161, 0.25f );
        for ( float z : { -4.0f, 4.0f } ) m.box( Vector3{ 18.0f, 1.5f, z }, Vector3{ 9.0f, 1.5f, 2.0f }, lime );
        m.box( Vector3{ 12.0f, 14.0f, 0 }, Vector3{ 4.0f, 7.0f, 5.0f }, lime );
        m.taperedBox( Vector3{ 11.0f, 14.0f, 0 }, Vector2{ 3.0f, 7.0f }, Vector3{ 11.0f, 22.0f, 0 },
                      Vector2{ 3.0f, 4.0f }, scaleColor( lime, 0.9f ) );
        m.box( Vector3{ 14.5f, 18.0f, 0 }, Vector3{ 1.5f, 3.5f, 3.0f }, scaleColor( lime, 1.05f ) );
    }
    {   // 23 Borobudur: square terraces, round terraces of bell stupas, the great stupa.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  stone = rgb( 120, 116, 104 );
        for ( int i = 0; i < 6; ++i )
            m.box( Vector3{ 0, 2.0f + 4.0f * i, 0 }, Vector3{ 60.0f - 7.0f * i, 2.0f, 60.0f - 7.0f * i },
                   scaleColor( stone, 0.9f + 0.02f * i ) );
        for ( int ring = 0; ring < 3; ++ring )
        {
            const float r = 14.0f - 4.0f * ring, y = 24.0f + 2.5f * ring;
            m.cylinder( Vector3{ 0, y, 0 }, r + 2.5f, r + 2.5f, 2.5f, 24, stone );
            const int n = 16 - 4 * ring;
            for ( int i = 0; i < n; ++i )
            {
                const float a = Tau * i / n;
                m.blob( Vector3{ std::cos( a ) * r, y + 3.5f, std::sin( a ) * r }, Vector3{ 1.5f, 1.8f, 1.5f }, stone,
                        2171 + i, 0.0f );
                m.cylinder( Vector3{ std::cos( a ) * r, y + 5.0f, std::sin( a ) * r }, 0.4f, 0.05f, 1.2f, 6, stone );
            }
        }
        m.blob( Vector3{ 0, 36.0f, 0 }, Vector3{ 5.0f, 6.0f, 5.0f }, stone, 2191, 0.0f );
        m.cylinder( Vector3{ 0, 41.0f, 0 }, 1.2f, 0.1f, 6.0f, 8, stone );
    }
    {   // 24 La Mano: fingers rising from the sand.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  c = rgb( 190, 186, 180 );
        const float  fingers[5][3] = {
            { 0, -4.5f, 6.0f }, { 0.3f, -2.0f, 7.2f }, { 0.2f, 0.4f, 7.6f }, { 0, 2.8f, 7.0f }, { -1.0f, 5.2f, 3.6f }
        };
        for ( const auto &f : fingers )
        {
            m.tube( Vector3{ f[0], -0.5f, f[1] }, Vector3{ f[0] + 0.3f, f[2], f[1] * 1.05f }, 0.9f, 8, c );
            m.blob( Vector3{ f[0] + 0.3f, f[2], f[1] * 1.05f }, Vector3{ 0.9f, 0.6f, 0.9f }, c, 2201, 0.0f );
        }
    }
    {   // 25 Harbin's ice castle, lit from inside.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  lights[4] = { rgb( 120, 200, 255 ), rgb( 255, 120, 220 ), rgb( 140, 255, 180 ),
                                   rgb( 255, 220, 120 ) };
        for ( int i = 0; i < 9; ++i )
        {
            const float z = -40.0f + 10.0f * i, h = 14.0f + 10.0f * std::sin( i * 0.9f ) + ( i == 4 ? 26.0f : 0.0f );
            m.box( Vector3{ 0, h * 0.5f, z }, Vector3{ 4.0f, h * 0.5f, 4.0f }, lights[i % 4], Mat{ 0.55f, 0.6f } );
            m.taperedBox( Vector3{ 0, h, z }, Vector2{ 4.4f, 4.4f }, Vector3{ 0, h + 7.0f, z }, Vector2{ 0.1f, 0.1f },
                          lights[( i + 1 ) % 4], Mat{ 0.6f, 0.6f } );
        }
        m.box( Vector3{ 0, 4.0f, 0 }, Vector3{ 3.0f, 4.0f, 44.0f }, rgb( 200, 230, 255 ), Mat{ 0.4f, 0.7f } );
    }
    {   // 26 Darvaza: the burning crater.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const int    n = 20;
        for ( int i = 0; i < n; ++i )
        {
            const float a = Tau * i / n;
            m.blob( Vector3{ std::cos( a ) * 34.0f, 0.5f, std::sin( a ) * 34.0f }, Vector3{ 8.0f, 3.0f, 8.0f },
                    rgb( 150, 120, 90 ), 2211 + i, 0.4f );
            m.tri( Vector3{ std::cos( a ) * 26.0f, -6.0f, std::sin( a ) * 26.0f },
                   Vector3{ std::cos( a + 0.15f ) * 26.0f, -6.0f, std::sin( a + 0.15f ) * 26.0f },
                   Vector3{ std::cos( a + 0.07f ) * 25.0f, -1.0f + 2.0f * ( i % 3 ), std::sin( a + 0.07f ) * 25.0f },
                   rgb( 255, 150, 40 ), Glow );
        }
        m.cylinder( Vector3{ 0, -8.0f, 0 }, 30.0f, 30.0f, 0.5f, 20, rgb( 255, 110, 30 ), Glow );
        m.cylinder( Vector3{ 0, -7.4f, 0 }, 31.0f, 34.0f, 7.0f, 20, rgb( 90, 60, 40 ), Mat{ 0.3f, 0.0f } );
    }
    {   // 27 A triumphal arch (the Arc de Triomphe, Palmyra).
        MeshBuilder &m = keyed( PropKindE::LANDMARK, 0.0f, FacingE::ROAD, rgb( 222, 210, 184 ) );
        for ( float z : { -15.0f, 15.0f } )
            m.box( Vector3{ 0, 15.0f, z }, Vector3{ 11.0f, 15.0f, 7.5f }, keyA( 1.0f ) );
        m.box( Vector3{ 0, 38.0f, 0 }, Vector3{ 11.0f, 8.0f, 22.5f }, keyA( 0.97f ) );
        for ( int k = 0; k < 6; ++k )
        {
            const float a0 = 3.14159f * k / 6.0f, a1 = 3.14159f * ( k + 1 ) / 6.0f;
            for ( float x : { -11.0f, 11.0f } )
                m.quad( Vector3{ x, 22.0f + std::sin( a0 ) * 8.0f, -std::cos( a0 ) * 7.5f },
                        Vector3{ x, 22.0f + std::sin( a1 ) * 8.0f, -std::cos( a1 ) * 7.5f },
                        Vector3{ x, 30.0f, -std::cos( a1 ) * 7.5f }, Vector3{ x, 30.0f, -std::cos( a0 ) * 7.5f },
                        keyA( 0.92f ) );
        }
        m.box( Vector3{ 0, 46.5f, 0 }, Vector3{ 11.5f, 0.6f, 23.0f }, keyA( 0.9f ) );
    }
    {   // 28 Great Zimbabwe: the curved wall and the conical tower.
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  granite = rgb( 150, 136, 116 );
        for ( int i = 0; i < 30; ++i )
        {
            if ( i == 7 || i == 8 ) continue;
            const float a = Tau * i / 30.0f;
            m.box( Vector3{ std::cos( a ) * 40.0f, 5.0f, std::sin( a ) * 30.0f },
                   Vector3{ 4.5f, 5.0f + ( i % 5 == 0 ? 1.0f : 0.0f ), 4.5f },
                   scaleColor( granite, 0.9f + 0.02f * ( i % 5 ) ) );
        }
        m.cylinder( Vector3{ 15.0f, 0, 5.0f }, 3.0f, 1.6f, 10.0f, 12, granite );
    }
    {   // 29 An island church in a lake (Bled).
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        m.blob( Vector3{ 0, 0, 0 }, Vector3{ 36.0f, 8.0f, 28.0f }, rgb( 120, 116, 104 ), 2231, 0.3f );
        for ( int i = 0; i < 8; ++i )
            m.blob( Vector3{ -20.0f + 6.0f * i, 10.0f, -14.0f + 3.0f * ( i % 3 ) }, Vector3{ 4.0f, 5.0f, 4.0f },
                    rgb( 60, 110, 56 ), 2232 + i, 0.4f );
        m.box( Vector3{ 4.0f, 12.0f, 4.0f }, Vector3{ 5.0f, 5.0f, 9.0f }, white );
        gable( m, Vector3{ 4.0f, 17.0f, 4.0f }, 5.0f, 9.0f, 4.0f, 0.3f, rgb( 170, 70, 52 ), white );
        m.box( Vector3{ 4.0f, 18.0f, -7.0f }, Vector3{ 2.5f, 11.0f, 2.5f }, white );
        m.cylinder( Vector3{ 4.0f, 29.0f, -7.0f }, 2.6f, 0.1f, 9.0f, 8, rgb( 170, 70, 52 ) );
    }
    {   // 30 A torii standing in the sea (Itsukushima).
        MeshBuilder &m = add( PropKindE::LANDMARK, 0.0f, FacingE::ROAD );
        const Color  red = rgb( 214, 60, 40 );
        for ( float z : { -8.0f, 8.0f } )
        {
            m.cylinder( Vector3{ 0, -4.0f, z }, 0.9f, 0.8f, 20.0f, 10, red );
            for ( float x : { -3.0f, 3.0f } ) m.tube( Vector3{ x, -4.0f, z }, Vector3{ 0, 8.0f, z }, 0.4f, 6, red );
        }
        m.box( Vector3{ 0, 13.5f, 0 }, Vector3{ 1.0f, 0.6f, 10.5f }, red );
        m.box( Vector3{ 0, 16.3f, 0 }, Vector3{ 1.4f, 0.6f, 12.5f }, rgb( 40, 36, 34 ) );
        m.box( Vector3{ 0, 15.5f, 0 }, Vector3{ 1.0f, 0.4f, 11.5f }, red );
    }

    // ---- Waterfalls, basalt, stones, ships, marsh houses, pipelines, signs, tortoises, trulli ---------------------------
    for ( int v = 0; v < 2; ++v )
    {   // Waterfall: wide and low (Niagara, Iguazu) or a tall thread (Kaieteur, Trollstigen).
        MeshBuilder &m = keyed( PropKindE::WATERFALL, 0.0f, FacingE::ROAD, rgb( 96, 92, 88 ), rgb( 236, 244, 248 ) );
        const float  w = v ? 14.0f : 60.0f, h = v ? 90.0f : 30.0f;
        m.box( Vector3{ -10.0f, h * 0.5f, 0 }, Vector3{ 10.0f, h * 0.5f, w + 30.0f }, keyA( 0.9f ) );
        m.blob( Vector3{ -14.0f, h, 0 }, Vector3{ 14.0f, 6.0f, w + 30.0f }, rgb( 70, 110, 56 ), 2241 + v, 0.4f );
        for ( int i = 0; i < ( v ? 1 : 4 ); ++i )
        {
            const float z = v ? 0.0f : -w + w * 0.5f * i + w * 0.25f, hw = v ? w * 0.5f : w * 0.22f;
            m.quad( Vector3{ 0.2f, h, z - hw }, Vector3{ 0.2f, h, z + hw }, Vector3{ 4.0f, 0, z + hw * 1.2f },
                    Vector3{ 4.0f, 0, z - hw * 1.2f }, keyB( 1.0f ), Mat{ 0.25f, 0.4f } );
            m.blob( Vector3{ 6.0f, 2.0f, z }, Vector3{ 6.0f, 5.0f, hw * 1.4f }, keyB( 0.95f ), 2245 + i, 0.4f,
                    Mat{ 0.15f, 0.0f } );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Basalt columns: a causeway of steps, or a cliff of organ pipes.
        MeshBuilder &m = add( PropKindE::BASALT_COLUMNS, 3.0f );
        Rng          r( 2251 + v );
        for ( int i = 0; i < 40; ++i )
        {
            const float x = r.range( -6.0f, 6.0f ), z = r.range( -6.0f, 6.0f );
            const float h = v ? r.range( 6.0f, 14.0f ) : r.range( 0.3f, 2.6f ) + std::max( 0.0f, -x * 0.25f );
            m.cylinder( Vector3{ x, -0.5f, z }, 0.55f, 0.55f, h + 0.5f, 6, rgb( 52 + i % 3 * 6, 52, 56 ), {},
                        0.5f * i );
        }
    }
    {   // A stone circle of trilithons.
        MeshBuilder &m = add( PropKindE::STONE_CIRCLE, 0.0f );
        const Color  stone = rgb( 150, 148, 138 );
        for ( int i = 0; i < 15; ++i )
        {
            if ( i == 4 || i == 11 ) continue;
            const float   a0 = Tau * i / 15.0f, a1 = a0 + Tau * 0.6f / 15.0f;
            const Vector3 p0 = { std::cos( a0 ) * 15.0f, 0, std::sin( a0 ) * 15.0f },
                          p1 = { std::cos( a1 ) * 15.0f, 0, std::sin( a1 ) * 15.0f };
            m.box( Vector3{ p0.x, 2.2f, p0.z }, Vector3{ 1.0f, 2.2f, 1.0f }, stone );
            m.box( Vector3{ p1.x, 2.2f, p1.z }, Vector3{ 1.0f, 2.2f, 1.0f }, stone );
            if ( i % 3 != 2 )
                m.box( Vector3{ ( p0.x + p1.x ) * 0.5f, 4.8f, ( p0.z + p1.z ) * 0.5f }, Vector3{ 1.6f, 0.4f, 1.6f },
                       scaleColor( stone, 0.95f ) );
        }
        for ( int i = 0; i < 5; ++i )
        {
            const float a = Tau * i / 5.0f;
            m.box( Vector3{ std::cos( a ) * 6.0f, 3.2f, std::sin( a ) * 6.0f }, Vector3{ 1.2f, 3.2f, 1.0f }, stone );
        }
    }
    for ( int v = 0; v < 2; ++v )
    {   // Cargo ship with containers.
        MeshBuilder &m = keyed( PropKindE::SHIP, 0.0f, FacingE::FREE, rgb( 40, 60, 110 ) );
        const float  l = 60.0f + 30.0f * v;
        m.taperedBox( Vector3{ 0, -4.0f, 0 }, Vector2{ 6.0f, l * 0.95f }, Vector3{ 0, 5.0f, 0 }, Vector2{ 8.0f, l },
                      keyA( 1.0f ) );
        m.box( Vector3{ 0, 4.0f, 0 }, Vector3{ 8.05f, 1.0f, l }, rgb( 170, 50, 40 ) );
        const Color boxes[] = { rgb( 200, 60, 50 ), rgb( 40, 120, 180 ),  rgb( 230, 170, 40 ),
                                rgb( 60, 150, 90 ), rgb( 210, 210, 210 ), rgb( 120, 70, 140 ) };
        Rng         r( 2261 + v );
        for ( int i = 0; i < static_cast<int>( l / 7.0f ); ++i )
            for ( int s = 0; s < 3 + v; ++s )
                m.box( Vector3{ 0, 6.5f + 2.6f * s, -l + 14.0f + 6.5f * i }, Vector3{ 7.0f, 1.3f, 3.0f },
                       boxes[r.rangeInt( 0, 5 )] );
        m.box( Vector3{ 0, 12.0f, -l + 7.0f }, Vector3{ 7.5f, 7.0f, 4.0f }, rgb( 240, 240, 236 ) );
        panes( m, Vector3{ 7.5f, 6.0f, -l + 3.0f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 8.0f, 13.0f, 2265 + v, 0.5f,
               3.0f, 1.0f );
    }
    {   // Mudhif: an arched reed hall.
        MeshBuilder &m = add( PropKindE::REED_HOUSE, 4.0f, FacingE::ROAD );
        const Color  reed = rgb( 196, 170, 110 );
        for ( int i = 0; i < 9; ++i )
        {
            const float z = -10.0f + 2.5f * i;
            for ( int k = 0; k < 6; ++k )
            {
                const float a0 = 3.14159f * k / 6.0f, a1 = 3.14159f * ( k + 1 ) / 6.0f;
                m.tube( Vector3{ -std::cos( a0 ) * 4.0f, std::sin( a0 ) * 6.0f, z },
                        Vector3{ -std::cos( a1 ) * 4.0f, std::sin( a1 ) * 6.0f, z }, 0.35f, 5,
                        scaleColor( reed, 0.9f ) );
            }
            if ( i < 8 )
                for ( int k = 0; k < 6; ++k )
                {
                    const float a0 = 3.14159f * k / 6.0f, a1 = 3.14159f * ( k + 1 ) / 6.0f;
                    m.quad( Vector3{ -std::cos( a0 ) * 3.9f, std::sin( a0 ) * 5.9f, z },
                            Vector3{ -std::cos( a0 ) * 3.9f, std::sin( a0 ) * 5.9f, z + 2.5f },
                            Vector3{ -std::cos( a1 ) * 3.9f, std::sin( a1 ) * 5.9f, z + 2.5f },
                            Vector3{ -std::cos( a1 ) * 3.9f, std::sin( a1 ) * 5.9f, z }, reed );
                }
        }
    }
    {   // Raised pipeline on its supports.
        MeshBuilder &m = add( PropKindE::PIPELINE, 0.0f, FacingE::ALONG );
        m.tube( Vector3{ 0, 2.6f, -2.6f }, Vector3{ 0, 2.6f, 2.6f }, 0.6f, 10, rgb( 200, 204, 208 ), Shiny, true );
        for ( float x : { -1.0f, 1.0f } )
            m.box( Vector3{ x, 1.0f, 0 }, Vector3{ 0.12f, 1.0f, 0.12f }, rgb( 110, 110, 110 ) );
        m.box( Vector3{ 0, 1.9f, 0 }, Vector3{ 1.2f, 0.1f, 0.15f }, rgb( 110, 110, 110 ) );
    }
    for ( int sv = 0; sv < 20; ++sv )
    {   // Animal warning sign: kangaroo, camel, moose, cow, deer; on a yellow diamond, a white or a yellow triangle bordered
        // red, a yellow triangle bordered black.
        const int    v = sv % 5, shape = sv / 5;
        MeshBuilder &m = add( PropKindE::WARNING_SIGN, 0.3f, FacingE::ALONG );
        m.box( Vector3{ 0, 1.2f, 0 }, Vector3{ 0.05f, 1.2f, 0.05f }, rgb( 150, 150, 156 ) );
        const Vector3 c = { 0, 3.1f, -0.06f };
        const Color   y = rgb( 250, 200, 30 ), k = rgb( 20, 20, 20 );
        if ( shape == 0 )
        {
            m.quad( Vector3{ -0.9f, c.y, c.z }, Vector3{ 0, c.y - 0.9f, c.z }, Vector3{ 0.9f, c.y, c.z },
                    Vector3{ 0, c.y + 0.9f, c.z }, k, Mat{ 0.2f, 0.0f } );
            m.quad( Vector3{ -0.8f, c.y, c.z - 0.01f }, Vector3{ 0, c.y - 0.8f, c.z - 0.01f },
                    Vector3{ 0.8f, c.y, c.z - 0.01f }, Vector3{ 0, c.y + 0.8f, c.z - 0.01f }, y, Mat{ 0.25f, 0.0f } );
        }
        else
        {
            m.tri( Vector3{ 1.0f, c.y - 0.7f, c.z }, Vector3{ -1.0f, c.y - 0.7f, c.z }, Vector3{ 0, c.y + 1.0f, c.z },
                   shape == 3 ? k : rgb( 210, 30, 30 ), Mat{ 0.2f, 0.0f } );
            m.tri( Vector3{ 0.74f, c.y - 0.55f, c.z - 0.01f }, Vector3{ -0.74f, c.y - 0.55f, c.z - 0.01f },
                   Vector3{ 0, c.y + 0.72f, c.z - 0.01f }, shape >= 2 ? y : rgb( 248, 248, 246 ), Mat{ 0.25f, 0.0f } );
        }
        const float zz = c.z - 0.02f, sc = shape == 0 ? 1.0f : 0.62f, dy = shape == 0 ? 0.0f : -0.12f;
        auto        px = [&]( float x0, float y0, float x1, float y1 )
        {
            m.quad( Vector3{ x0 * sc, c.y + y0 * sc + dy, zz }, Vector3{ x1 * sc, c.y + y0 * sc + dy, zz },
                    Vector3{ x1 * sc, c.y + y1 * sc + dy, zz }, Vector3{ x0 * sc, c.y + y1 * sc + dy, zz }, k );
        };
        switch ( v )
        {
        case 0:
            px( -0.25f, -0.2f, 0.15f, 0.2f ), px( 0.05f, 0.2f, 0.25f, 0.4f ), px( -0.45f, -0.35f, -0.15f, -0.25f ),
                px( 0.1f, -0.45f, 0.3f, -0.2f );
            break;
        case 1:
            px( -0.35f, -0.05f, 0.25f, 0.15f ), px( -0.15f, 0.15f, 0.05f, 0.3f ), px( 0.25f, 0.05f, 0.35f, 0.4f ),
                px( -0.3f, -0.4f, -0.22f, -0.05f ), px( 0.12f, -0.4f, 0.2f, -0.05f );
            break;
        case 2:
            px( -0.3f, -0.1f, 0.2f, 0.15f ), px( 0.2f, 0.05f, 0.4f, 0.2f ), px( 0.15f, 0.2f, 0.45f, 0.3f ),
                px( -0.25f, -0.45f, -0.17f, -0.1f ), px( 0.1f, -0.45f, 0.18f, -0.1f );
            break;
        case 3:
            px( -0.35f, -0.1f, 0.25f, 0.15f ), px( 0.25f, 0.0f, 0.4f, 0.15f ), px( -0.3f, -0.35f, -0.22f, -0.1f ),
                px( 0.15f, -0.35f, 0.23f, -0.1f );
            break;
        default:
            px( -0.3f, -0.05f, 0.15f, 0.15f ), px( 0.15f, 0.1f, 0.3f, 0.3f ), px( 0.2f, 0.3f, 0.4f, 0.4f ),
                px( -0.25f, -0.4f, -0.17f, -0.05f ), px( 0.05f, -0.4f, 0.13f, -0.05f );
            break;
        }
    }
    {   // Giant tortoise.
        MeshBuilder &m = add( PropKindE::TORTOISE, 0.0f );
        const Color  shell = rgb( 70, 64, 56 ), skin = rgb( 110, 104, 92 );
        m.blob( Vector3{ 0, 0.55f, 0 }, Vector3{ 0.75f, 0.5f, 0.9f }, shell, 2271, 0.15f );
        for ( float x : { -1.0f, 1.0f } )
            for ( float z : { -1.0f, 1.0f } )
                m.tube( Vector3{ x * 0.55f, 0.35f, z * 0.55f }, Vector3{ x * 0.65f, 0.0f, z * 0.6f }, 0.14f, 5, skin );
        m.tube( Vector3{ 0, 0.45f, 0.8f }, Vector3{ 0, 0.7f, 1.3f }, 0.12f, 5, skin );
        m.blob( Vector3{ 0, 0.72f, 1.4f }, Vector3{ 0.13f, 0.12f, 0.18f }, skin, 2272, 0.1f );
    }
    for ( int v = 0; v < 2; ++v )
    {   // Trullo: whitewashed walls under stone cones with white tips.
        MeshBuilder &m = add( PropKindE::TRULLO, 3.0f, FacingE::ROAD );
        const Color  w = rgb( 242, 240, 234 ), stone = rgb( 120, 116, 110 );
        for ( int i = 0; i < 2 + v; ++i )
        {
            const float z = -3.5f + 3.5f * i, r = 2.0f + 0.3f * ( ( i + v ) % 2 );
            m.cylinder( Vector3{ 0, 0, z }, r, r, 2.4f, 10, w );
            m.cylinder( Vector3{ 0, 2.4f, z }, r + 0.1f, 0.3f, 3.0f + 0.4f * i, 10, stone );
            m.blob( Vector3{ 0, 5.6f + 0.4f * i, z }, Vector3{ 0.25f, 0.35f, 0.25f }, w, 2281 + i, 0.0f );
        }
        m.box( Vector3{ 2.25f, 0.9f, -3.5f }, Vector3{ 0.05f, 0.9f, 0.45f }, rgb( 60, 90, 120 ) );
    }
    m_keys[static_cast<size_t>( PropKindE::LANDMARK )] = { rgb( 226, 214, 186 ), rgb( 120, 136, 150 ),
                                                           rgb( 34, 40, 54 ) };

    // ---- The landmarks of data/landmarks, numbered after the compiled ones ------------------------------------------
    if ( static_cast<int>( m_props[static_cast<size_t>( PropKindE::LANDMARK )].size() ) != compiledLandmarks() )
        TraceLog( LOG_WARNING, "SUNSTREAK: %d compiled landmarks, the loader expects %d",
                  static_cast<int>( m_props[static_cast<size_t>( PropKindE::LANDMARK )].size() ), compiledLandmarks() );
    buildDataLandmarks();

    // The wind in the new plants.
    const std::pair<PropKindE, float> sway[] = {
        { PropKindE::OLIVE, 0.3f },     { PropKindE::POPLAR, 0.7f },       { PropKindE::UMBRELLA_PINE, 0.35f },
        { PropKindE::SPRUCE, 0.45f },   { PropKindE::EUCALYPTUS, 0.6f },   { PropKindE::BANANA, 0.8f },
        { PropKindE::CEDAR, 0.3f },     { PropKindE::REDWOOD, 0.2f },      { PropKindE::WAX_PALM, 1.0f },
        { PropKindE::TREE_FERN, 0.5f }, { PropKindE::LUPIN, 0.5f },        { PropKindE::FLOWER_BUSH, 0.3f },
        { PropKindE::SHRUB, 0.3f },     { PropKindE::BROADLEAF, 0.55f },   { PropKindE::DATE_PALM, 0.9f },
        { PropKindE::MANGROVE, 0.25f }, { PropKindE::PRAYER_FLAGS, 0.6f }, { PropKindE::BALLOON, 0.3f }
    };
    for ( const auto &[kind, amount] : sway )
        for ( MeshBuilder &variant : m_props[static_cast<size_t>( kind )] ) variant.applySway( amount );
}

}   // namespace sunstreak
