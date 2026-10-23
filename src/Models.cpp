/**
 * @file Models.cpp
 * @brief The procedural models.
 * @author reneweller
 */
#include "Models.h"

#include "MathUtil.h"
#include "Rng.h"

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

constexpr Mat Glow{ 1.0f, 0.0f };      ///< fully self-lit
constexpr Mat Paint{ 0.0f, 0.55f };    ///< car paint
constexpr Mat Glass{ 0.0f, 0.95f };    ///< windows
constexpr Mat Chrome{ 0.0f, 0.75f };   ///< rims, rails

const CarSpec CarSpecs[CarCount] = {
    { "ARROW GT", 80.5f, 1.0f, 1.0f, 4.5f, 1.88f },
    { "SPARROW", 72.0f, 1.16f, 1.12f, 3.9f, 1.72f },
    { "VORTEX", 88.0f, 0.92f, 0.93f, 4.8f, 2.0f },
};

const char *PaintNames[PaintCount] = { "RACING RED", "ICE WHITE", "SUN YELLOW", "DEEP BLUE" };
const Color PaintColors[PaintCount] = { rgb( 196, 18, 24 ), rgb( 232, 234, 240 ), rgb( 240, 186, 16 ),
                                        rgb( 22, 56, 168 ) };

/** @brief A cross-section of a car body. */
struct Station
{
    float m_z;    ///< position along the car
    float m_hw;   ///< half width
    float m_yb;   ///< bottom
    float m_yt;   ///< top (shoulder line)
};

/** @brief A cross-section of the greenhouse. */
struct CabinStation
{
    float m_z;     ///< position along the car
    float m_hwB;   ///< half width at the base
    float m_yB;    ///< base height
    float m_hwT;   ///< half width at the top
    float m_yT;    ///< top height
};

/** @brief What a car looks like. */
struct CarShape
{
    std::vector<Station>      m_body;        ///< rear to front
    std::vector<CabinStation> m_cabin;       ///< rear to front
    float                     m_wheelFront;  ///< front axle z
    float                     m_wheelRear;   ///< rear axle z
    float                     m_wheelR;      ///< wheel radius
    bool                      m_sideGlass;   ///< windows on the cabin's sides
    float                     m_wing;        ///< rear wing height above the deck (0: none)
};

/**
 * @brief Lofts a car body, its cabin, wheels and lights.
 * @param[out] mb     where the triangles go
 * @param[in]  shape  the outline
 * @param[in]  paint  body colour
 */
void buildCar( MeshBuilder &mb, const CarShape &shape, Color paint )
{
    const Color under = rgb( 26, 26, 30 );
    const Color trim = rgb( 40, 40, 46 );
    const Color glass = rgb( 18, 24, 36 );
    auto ring = []( const Station &s )
    {
        const float h = s.m_yt - s.m_yb;
        return std::array<Vector3, 8>{ Vector3{ -s.m_hw * 0.85f, s.m_yb, s.m_z },
                                        Vector3{ -s.m_hw, s.m_yb + 0.2f * h, s.m_z },
                                        Vector3{ -s.m_hw, s.m_yb + 0.72f * h, s.m_z },
                                        Vector3{ -s.m_hw * 0.88f, s.m_yt, s.m_z },
                                        Vector3{ s.m_hw * 0.88f, s.m_yt, s.m_z },
                                        Vector3{ s.m_hw, s.m_yb + 0.72f * h, s.m_z },
                                        Vector3{ s.m_hw, s.m_yb + 0.2f * h, s.m_z },
                                        Vector3{ s.m_hw * 0.85f, s.m_yb, s.m_z } };
    };
    const auto &body = shape.m_body;
    for( size_t i = 0; i + 1 < body.size(); ++i )
    {
        const auto r0 = ring( body[i] );
        const auto r1 = ring( body[i + 1] );
        for( int k = 0; k < 8; ++k )
        {
            const int   n = ( k + 1 ) % 8;
            const Color c = k == 7 ? under : ( k == 0 || k == 6 ) ? scaleColor( paint, 0.72f ) : paint;
            mb.quad( r0[k], r0[n], r1[n], r1[k], c, k == 7 ? Mat{} : Paint );
        }
    }
    // Nose and tail caps.
    const auto nose = ring( body.back() );
    const auto tail = ring( body.front() );
    const Vector3 noseC = { 0, ( body.back().m_yb + body.back().m_yt ) * 0.5f, body.back().m_z };
    const Vector3 tailC = { 0, ( body.front().m_yb + body.front().m_yt ) * 0.5f, body.front().m_z };
    for( int k = 0; k < 8; ++k )
    {
        mb.tri( noseC, nose[k], nose[( k + 1 ) % 8], paint, Paint );
        mb.tri( tailC, tail[( k + 1 ) % 8], tail[k], scaleColor( paint, 0.45f ), Paint );
    }
    // Cabin.
    const auto &cab = shape.m_cabin;
    for( size_t i = 0; i + 1 < cab.size(); ++i )
    {
        const CabinStation &a = cab[i];
        const CabinStation &b = cab[i + 1];
        const Vector3 a0 = { -a.m_hwB, a.m_yB, a.m_z }, a1 = { -a.m_hwT, a.m_yT, a.m_z }, a2 = { a.m_hwT, a.m_yT, a.m_z },
                      a3 = { a.m_hwB, a.m_yB, a.m_z };
        const Vector3 b0 = { -b.m_hwB, b.m_yB, b.m_z }, b1 = { -b.m_hwT, b.m_yT, b.m_z }, b2 = { b.m_hwT, b.m_yT, b.m_z },
                      b3 = { b.m_hwB, b.m_yB, b.m_z };
        const bool  windowTop = i == 0 || i + 2 == cab.size();
        const Color side = shape.m_sideGlass ? glass : paint;
        mb.quad( a0, b0, b1, a1, side, shape.m_sideGlass ? Glass : Paint );
        mb.quad( a2, b2, b3, a3, side, shape.m_sideGlass ? Glass : Paint );
        mb.quad( a1, b1, b2, a2, windowTop ? glass : paint, windowTop ? Glass : Paint );
    }
    // Wheels with hubs.
    const float hw = body[body.size() / 2].m_hw;
    for( float z : { shape.m_wheelFront, shape.m_wheelRear } )
    {
        for( float side : { -1.0f, 1.0f } )
        {
            const float r = shape.m_wheelR;
            mb.tube( Vector3{ side * ( hw - 0.29f ), r, z }, Vector3{ side * ( hw - 0.02f ), r, z }, r, 10,
                     rgb( 20, 20, 22 ), Mat{}, true );
            mb.tube( Vector3{ side * ( hw - 0.03f ), r, z }, Vector3{ side * ( hw + 0.005f ), r, z }, r * 0.58f, 8,
                     rgb( 170, 172, 178 ), Chrome, true );
        }
    }
    // Lights: red at the tail, white at the nose.
    const Station &t = body.front();
    const Station &f = body.back();
    for( float side : { -1.0f, 1.0f } )
    {
        const float th = t.m_yt - t.m_yb;
        const float zt = t.m_z - 0.01f;
        mb.quad( Vector3{ side * t.m_hw * 0.45f, t.m_yb + 0.52f * th, zt }, Vector3{ side * t.m_hw * 0.82f, t.m_yb + 0.52f * th, zt },
                 Vector3{ side * t.m_hw * 0.82f, t.m_yb + 0.74f * th, zt }, Vector3{ side * t.m_hw * 0.45f, t.m_yb + 0.74f * th, zt },
                 rgb( 255, 24, 24 ), Mat{ 0.5f, 0.3f } );
        const float fh = f.m_yt - f.m_yb;
        const float zf = f.m_z + 0.01f;
        mb.quad( Vector3{ side * f.m_hw * 0.32f, f.m_yb + 0.35f * fh, zf }, Vector3{ side * f.m_hw * 0.78f, f.m_yb + 0.35f * fh, zf },
                 Vector3{ side * f.m_hw * 0.78f, f.m_yb + 0.7f * fh, zf }, Vector3{ side * f.m_hw * 0.32f, f.m_yb + 0.7f * fh, zf },
                 rgb( 255, 246, 224 ), Mat{ 0.7f, 0.3f } );
    }
    if( shape.m_wing > 0.0f )
    {
        const float y = t.m_yt + shape.m_wing;
        const float z = t.m_z + 0.25f;
        mb.box( Vector3{ 0, y, z }, Vector3{ t.m_hw * 0.95f, 0.035f, 0.24f }, paint, Paint );
        for( float side : { -1.0f, 1.0f } )
            mb.box( Vector3{ side * t.m_hw * 0.6f, ( y + t.m_yt ) * 0.5f, z }, Vector3{ 0.04f, ( y - t.m_yt ) * 0.5f, 0.08f }, trim );
    }
}

/**
 * @brief The three player cars' outlines.
 * @param[in] index  which car
 * @return its shape
 */
CarShape playerShape( int index )
{
    CarShape s;
    switch( index )
    {
    case 0: // Wedge coupe.
        s.m_body = { { -2.2f, 0.88f, 0.30f, 0.86f }, { -1.7f, 0.92f, 0.24f, 0.88f }, { -0.6f, 0.94f, 0.22f, 0.86f },
                     { 0.6f, 0.94f, 0.21f, 0.78f },  { 1.5f, 0.90f, 0.21f, 0.64f },  { 2.05f, 0.80f, 0.24f, 0.50f },
                     { 2.25f, 0.66f, 0.30f, 0.42f } };
        s.m_cabin = { { -1.65f, 0.80f, 0.87f, 0.80f, 0.87f }, { -0.75f, 0.80f, 0.86f, 0.62f, 1.10f },
                      { -0.05f, 0.82f, 0.82f, 0.62f, 1.12f }, { 0.75f, 0.84f, 0.77f, 0.84f, 0.77f } };
        s.m_wheelFront = 1.38f;
        s.m_wheelRear = -1.35f;
        s.m_wheelR = 0.33f;
        s.m_sideGlass = true;
        s.m_wing = 0.16f;
        break;
    case 1: // Small rounded coupe.
        s.m_body = { { -1.9f, 0.82f, 0.28f, 0.84f }, { -1.4f, 0.86f, 0.22f, 0.86f }, { 0.0f, 0.86f, 0.22f, 0.80f },
                     { 1.1f, 0.84f, 0.22f, 0.72f },  { 1.7f, 0.76f, 0.26f, 0.60f },  { 1.95f, 0.60f, 0.32f, 0.50f } };
        s.m_cabin = { { -1.35f, 0.74f, 0.85f, 0.74f, 0.85f }, { -0.8f, 0.76f, 0.84f, 0.60f, 1.18f },
                      { 0.0f, 0.76f, 0.80f, 0.60f, 1.18f }, { 0.55f, 0.76f, 0.78f, 0.76f, 0.78f } };
        s.m_wheelFront = 1.25f;
        s.m_wheelRear = -1.2f;
        s.m_wheelR = 0.31f;
        s.m_sideGlass = true;
        s.m_wing = 0.0f;
        break;
    default: // Low concept car with a canopy and a big wing.
        s.m_body = { { -2.35f, 0.96f, 0.26f, 0.80f }, { -1.6f, 1.0f, 0.20f, 0.84f }, { -0.4f, 1.0f, 0.18f, 0.74f },
                     { 0.8f, 0.98f, 0.18f, 0.62f },   { 1.8f, 0.90f, 0.18f, 0.46f }, { 2.4f, 0.70f, 0.22f, 0.34f } };
        s.m_cabin = { { -1.5f, 0.62f, 0.80f, 0.62f, 0.80f }, { -0.7f, 0.64f, 0.76f, 0.50f, 1.0f },
                      { 0.0f, 0.64f, 0.70f, 0.50f, 1.02f }, { 0.9f, 0.62f, 0.62f, 0.62f, 0.62f } };
        s.m_wheelFront = 1.5f;
        s.m_wheelRear = -1.45f;
        s.m_wheelR = 0.34f;
        s.m_sideGlass = true;
        s.m_wing = 0.3f;
        break;
    }
    return s;
}

/**
 * @brief A thick line on a board's face (signs).
 * @param[out] mb     where the triangles go
 * @param[in]  a      start (x, y)
 * @param[in]  b      end (x, y)
 * @param[in]  z      the face's depth
 * @param[in]  width  line width
 * @param[in]  col    its colour
 * @param[in]  mat    its material
 */
void boardLine( MeshBuilder &mb, Vector2 a, Vector2 b, float z, float width, Color col, Mat mat )
{
    const Vector2 d = Vector2Normalize( Vector2Subtract( b, a ) );
    const Vector2 n = { -d.y * width * 0.5f, d.x * width * 0.5f };
    mb.quad( Vector3{ a.x + n.x, a.y + n.y, z }, Vector3{ b.x + n.x, b.y + n.y, z }, Vector3{ b.x - n.x, b.y - n.y, z },
             Vector3{ a.x - n.x, a.y - n.y, z }, col, mat );
}

/**
 * @brief Windows on one face of a block.
 * @param[out] mb       where the triangles go
 * @param[in]  origin   the face's lower left corner
 * @param[in]  across   unit vector along the face
 * @param[in]  out      the face's outward normal
 * @param[in]  width    face width
 * @param[in]  height   face height
 * @param[in]  seed     which windows are lit
 * @param[in]  litRate  0..1 lit share
 */
void windows( MeshBuilder &mb, Vector3 origin, Vector3 across, Vector3 out, float width, float height, uint64_t seed,
              float litRate )
{
    const int cols = std::max( 1, static_cast<int>( width / 3.0f ) );
    const int rows = std::max( 1, static_cast<int>( ( height - 2.0f ) / 3.4f ) );
    const float stepX = width / static_cast<float>( cols );
    const Vector3 lift = Vector3Scale( out, 0.06f );
    for( int r = 0; r < rows; ++r )
    {
        for( int c = 0; c < cols; ++c )
        {
            const bool  lit = hashFloat( seed, static_cast<uint64_t>( r * 97 + c ) ) < litRate;
            const float x0 = stepX * ( static_cast<float>( c ) + 0.25f );
            const float x1 = stepX * ( static_cast<float>( c ) + 0.75f );
            const float y0 = 2.0f + static_cast<float>( r ) * 3.4f;
            const float y1 = y0 + 1.7f;
            auto p = [&]( float x, float y )
            { return Vector3Add( Vector3Add( origin, lift ), Vector3Add( Vector3Scale( across, x ), Vector3{ 0, y, 0 } ) ); };
            const Color col = lit ? ( hashFloat( seed + 5, static_cast<uint64_t>( r * 31 + c ) ) < 0.3f ? rgb( 170, 220, 255 )
                                                                                                        : rgb( 255, 214, 150 ) )
                                  : rgb( 30, 38, 54 );
            mb.quad( p( x0, y0 ), p( x1, y0 ), p( x1, y1 ), p( x0, y1 ), col, lit ? Mat{ 0.85f, 0.0f } : Glass );
        }
    }
}

/**
 * @brief A block with windows on all four sides.
 * @param[out] mb      where the triangles go
 * @param[in]  hx      half width (x)
 * @param[in]  hz      half depth (z)
 * @param[in]  h       height
 * @param[in]  wall    wall colour
 * @param[in]  seed    window seed
 * @param[in]  litRate lit share
 */
void block( MeshBuilder &mb, float hx, float hz, float h, Color wall, uint64_t seed, float litRate )
{
    mb.box( Vector3{ 0, h * 0.5f, 0 }, Vector3{ hx, h * 0.5f, hz }, wall );
    windows( mb, Vector3{ -hx, 0, -hz }, Vector3{ 1, 0, 0 }, Vector3{ 0, 0, -1 }, hx * 2, h, seed, litRate );
    windows( mb, Vector3{ hx, 0, hz }, Vector3{ -1, 0, 0 }, Vector3{ 0, 0, 1 }, hx * 2, h, seed + 1, litRate );
    windows( mb, Vector3{ hx, 0, -hz }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, hz * 2, h, seed + 2, litRate );
    windows( mb, Vector3{ -hx, 0, hz }, Vector3{ 0, 0, -1 }, Vector3{ -1, 0, 0 }, hz * 2, h, seed + 3, litRate );
}

} // namespace

const CarSpec &carSpec( int index )
{
    return CarSpecs[std::clamp( index, 0, CarCount - 1 )];
}

const char *paintName( int index )
{
    return PaintNames[std::clamp( index, 0, PaintCount - 1 )];
}

const MeshBuilder &Models::prop( PropKindE kind, uint64_t variant ) const
{
    const auto &v = m_props[static_cast<size_t>( kind )];
    return v[static_cast<size_t>( variant % v.size() )];
}

void Models::buildProps()
{
    auto add = [&]( PropKindE kind, float radius ) -> MeshBuilder &
    {
        m_radius[static_cast<size_t>( kind )] = radius;
        auto &v = m_props[static_cast<size_t>( kind )];
        v.emplace_back();
        return v.back();
    };

    for( int v = 0; v < 3; ++v )
    { // Pines, a little different each.
        MeshBuilder &m = add( PropKindE::PINE, 0.6f );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.26f, 0.2f, 1.6f, 6, rgb( 92, 62, 40 ) );
        const Color g[3] = { rgb( 30 + v * 6, 86 + v * 6, 42 ), rgb( 36 + v * 6, 98 + v * 4, 48 ), rgb( 44, 110, 54 + v * 4 ) };
        const float k = 1.0f + 0.12f * static_cast<float>( v );
        m.cylinder( Vector3{ 0, 1.0f, 0 }, 2.2f, 0.0f, 3.3f * k, 7, g[0], {}, 0.3f * v );
        m.cylinder( Vector3{ 0, 2.6f * k, 0 }, 1.7f, 0.0f, 2.9f * k, 7, g[1], {}, 0.9f );
        m.cylinder( Vector3{ 0, 4.1f * k, 0 }, 1.1f, 0.0f, 2.5f * k, 7, g[2], {}, 1.7f );
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::SNOW_PINE, 0.6f );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.26f, 0.2f, 1.6f, 6, rgb( 80, 60, 46 ) );
        const float k = 1.0f + 0.15f * static_cast<float>( v );
        const float base[3] = { 1.0f, 2.6f * k, 4.1f * k };
        const float rad[3] = { 2.2f, 1.7f, 1.1f };
        const float hgt[3] = { 3.3f * k, 2.9f * k, 2.5f * k };
        for( int i = 0; i < 3; ++i )
        {
            m.cylinder( Vector3{ 0, base[i], 0 }, rad[i], 0.0f, hgt[i], 7, rgb( 34, 76, 56 ), {}, 0.5f * i );
            m.cylinder( Vector3{ 0, base[i] + hgt[i] * 0.42f, 0 }, rad[i] * 0.62f, 0.0f, hgt[i] * 0.58f + 0.05f, 7,
                        rgb( 236, 242, 250 ), {}, 0.5f * i );
        }
    }
    for( int v = 0; v < 3; ++v )
    {
        MeshBuilder &m = add( PropKindE::OAK, 0.7f );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.1f * v, 2.4f, 0 }, 0.3f, 6, rgb( 96, 70, 48 ) );
        const Color leaf = v == 2 ? rgb( 120, 150, 50 ) : rgb( 64 + v * 10, 128, 48 );
        m.blob( Vector3{ 0.1f * v, 3.8f, 0 }, Vector3{ 2.5f, 2.1f, 2.4f }, leaf, 11 + v, 0.4f );
        m.blob( Vector3{ 0.9f, 4.5f, 0.5f }, Vector3{ 1.5f, 1.3f, 1.5f }, scaleColor( leaf, 1.12f ), 21 + v, 0.4f );
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::BUSH, 0.0f );
        m.blob( Vector3{ 0, 0.5f, 0 }, Vector3{ 1.1f, 0.75f, 1.0f }, v ? rgb( 74, 112, 46 ) : rgb( 58, 100, 44 ), 31 + v, 0.5f );
    }
    for( int v = 0; v < 3; ++v )
    {
        MeshBuilder &m = add( PropKindE::ROCK, 0.8f );
        m.blob( Vector3{ 0, 0.25f, 0 }, Vector3{ 0.95f, 0.65f, 0.8f }, rgb( 132 + v * 8, 124 + v * 4, 116 ), 41 + v, 0.7f );
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::BOULDER, 2.0f );
        m.blob( Vector3{ 0, 0.9f, 0 }, Vector3{ 2.3f, 1.7f, 2.0f }, rgb( 128, 118, 108 ), 51 + v, 0.55f );
        m.blob( Vector3{ 1.4f, 0.6f, 1.0f }, Vector3{ 1.2f, 0.9f, 1.1f }, rgb( 140, 130, 118 ), 61 + v, 0.6f );
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::CACTUS, 0.5f );
        const Color g = rgb( 68, 116 + v * 10, 60 );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.36f, 0.32f, 4.0f + v * 0.8f, 8, g );
        m.cylinder( Vector3{ 0, 4.0f + v * 0.8f, 0 }, 0.32f, 0.0f, 0.35f, 8, g );
        m.tube( Vector3{ 0.3f, 1.8f, 0 }, Vector3{ 1.05f, 1.8f, 0 }, 0.22f, 6, g );
        m.cylinder( Vector3{ 1.05f, 1.6f, 0 }, 0.24f, 0.2f, 1.6f, 6, g );
        m.tube( Vector3{ -0.3f, 2.5f, 0 }, Vector3{ -0.95f, 2.5f, 0 }, 0.2f, 6, g );
        m.cylinder( Vector3{ -0.95f, 2.3f, 0 }, 0.22f, 0.18f, 1.3f, 6, g );
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::MESA, 0.0f );
        const float k = 1.0f + 0.25f * v;
        m.taperedBox( Vector3{ 0, -2, 0 }, Vector2{ 16 * k, 11 }, Vector3{ 0, 12, 0 }, Vector2{ 12 * k, 9 }, rgb( 168, 86, 58 ) );
        m.taperedBox( Vector3{ 0.5f, 12, 0 }, Vector2{ 11.5f * k, 8.5f }, Vector3{ 1, 21, 0 }, Vector2{ 10 * k, 7.5f },
                      rgb( 196, 112, 72 ) );
        m.taperedBox( Vector3{ 1, 21, 0 }, Vector2{ 10 * k, 7.5f }, Vector3{ 1, 24, 0 }, Vector2{ 9.4f * k, 7 }, rgb( 150, 76, 52 ) );
    }
    for( int v = 0; v < 3; ++v )
    {
        MeshBuilder &m = add( PropKindE::PALM, 0.4f );
        const float lean = 0.5f + 0.3f * v;
        Vector3     prev = { 0, 0, 0 };
        for( int i = 1; i <= 6; ++i )
        {
            const float   u = static_cast<float>( i ) / 6.0f;
            const Vector3 p = { lean * u * u * 1.6f, 7.0f * u, 0 };
            m.tube( prev, p, 0.24f - 0.06f * u, 6, i % 2 ? rgb( 128, 96, 62 ) : rgb( 112, 84, 56 ) );
            prev = p;
        }
        for( int i = 0; i < 7; ++i )
        {
            const float   a = Tau * ( static_cast<float>( i ) + 0.3f * v ) / 7.0f;
            const Vector3 dir = { std::cos( a ), 0, std::sin( a ) };
            const Vector3 side = { -dir.z * 0.45f, 0, dir.x * 0.45f };
            const Vector3 mid = Vector3Add( prev, Vector3Add( Vector3Scale( dir, 1.7f ), Vector3{ 0, 0.45f, 0 } ) );
            const Vector3 tip = Vector3Add( prev, Vector3Add( Vector3Scale( dir, 3.4f ), Vector3{ 0, -1.3f, 0 } ) );
            const Color   leaf = i % 2 ? rgb( 58, 132, 56 ) : rgb( 74, 150, 62 );
            m.quad( prev, Vector3Add( mid, side ), tip, Vector3Subtract( mid, side ), leaf );
        }
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::DEAD_TREE, 0.4f );
        const Color bark = rgb( 66, 60, 56 );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.2f, 4.6f, 0.1f }, 0.24f, 6, bark );
        Rng r( 900 + v );
        for( int i = 0; i < 5; ++i )
        {
            const float   y = r.range( 1.8f, 4.2f );
            const float   a = r.range( 0.0f, Tau );
            const Vector3 from = { 0.1f, y, 0.05f };
            const Vector3 to = { std::cos( a ) * r.range( 1.2f, 2.4f ), y + r.range( 0.6f, 1.6f ), std::sin( a ) * 1.6f };
            m.tube( from, to, 0.09f, 4, bark );
        }
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::REEDS, 0.0f );
        Rng r( 1000 + v );
        for( int i = 0; i < 9; ++i )
        {
            const float   a = r.range( 0.0f, Tau );
            const Vector3 base = { std::cos( a ) * r.range( 0.0f, 0.7f ), 0, std::sin( a ) * r.range( 0.0f, 0.7f ) };
            const Vector3 tip = { base.x + r.range( -0.4f, 0.4f ), r.range( 1.1f, 1.9f ), base.z + r.range( -0.4f, 0.4f ) };
            const Vector3 w = { 0.07f, 0, 0.05f };
            m.tri( Vector3Subtract( base, w ), Vector3Add( base, w ), tip, i % 3 ? rgb( 128, 136, 72 ) : rgb( 150, 140, 90 ) );
        }
    }
    for( int v = 0; v < 4; ++v )
    { // City blocks: four heights and colours.
        MeshBuilder &m = add( PropKindE::BUILDING, 7.0f );
        const float  h = 12.0f + 7.0f * v;
        const Color  wall[4] = { rgb( 60, 62, 76 ), rgb( 76, 70, 70 ), rgb( 50, 56, 70 ), rgb( 70, 74, 86 ) };
        block( m, 7.0f, 6.5f, h, wall[v], 200 + v, 0.42f );
        m.box( Vector3{ 0, h + 0.4f, 0 }, Vector3{ 5.0f, 0.4f, 4.5f }, scaleColor( wall[v], 0.8f ) );
    }
    for( int v = 0; v < 3; ++v )
    {
        MeshBuilder &m = add( PropKindE::TOWER, 9.0f );
        const float  h = 45.0f + 20.0f * v;
        block( m, 9.0f, 9.0f, h, rgb( 44 + v * 8, 50, 66 ), 300 + v, 0.33f );
        m.tube( Vector3{ 0, h, 0 }, Vector3{ 0, h + 10.0f, 0 }, 0.25f, 4, rgb( 90, 90, 100 ) );
        m.box( Vector3{ 0, h + 10.2f, 0 }, Vector3{ 0.4f, 0.4f, 0.4f }, rgb( 255, 30, 30 ), Glow );
    }
    for( int v = 0; v < 3; ++v )
    {
        MeshBuilder &m = add( PropKindE::BILLBOARD, 0.5f );
        const Color face[3] = { rgb( 255, 40, 180 ), rgb( 40, 210, 255 ), rgb( 255, 150, 30 ) };
        for( float x : { -3.0f, 3.0f } ) m.tube( Vector3{ x, 0, 0 }, Vector3{ x, 6.2f, 0 }, 0.18f, 5, rgb( 70, 70, 80 ) );
        m.box( Vector3{ 0, 7.6f, 0 }, Vector3{ 4.2f, 1.7f, 0.2f }, rgb( 30, 30, 36 ) );
        m.quad( Vector3{ -3.9f, 6.2f, -0.22f }, Vector3{ 3.9f, 6.2f, -0.22f }, Vector3{ 3.9f, 9.0f, -0.22f },
                Vector3{ -3.9f, 9.0f, -0.22f }, scaleColor( face[v], 0.45f ), Mat{ 0.6f, 0.0f } );
        for( int i = 0; i < 3; ++i )
            boardLine( m, Vector2{ -3.2f, 6.8f + 0.8f * i }, Vector2{ 1.4f + 0.6f * i, 6.8f + 0.8f * i }, -0.24f, 0.35f,
                       face[v], Glow );
    }
    {
        MeshBuilder &m = add( PropKindE::POLE, 0.3f );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.16f, 0.13f, 9.0f, 6, rgb( 96, 74, 56 ) );
        m.box( Vector3{ 0, 8.4f, 0 }, Vector3{ 1.3f, 0.08f, 0.09f }, rgb( 96, 74, 56 ) );
        for( float x : { -1.1f, -0.4f, 0.4f, 1.1f } )
            m.cylinder( Vector3{ x, 8.48f, 0 }, 0.06f, 0.04f, 0.22f, 5, rgb( 120, 160, 140 ) );
    }
    {
        MeshBuilder &m = add( PropKindE::FENCE, 0.0f );
        const Color wood = rgb( 214, 204, 184 );
        m.box( Vector3{ 0, 0.55f, -2.5f }, Vector3{ 0.07f, 0.55f, 0.07f }, wood );
        m.box( Vector3{ 0, 0.5f, 0 }, Vector3{ 0.04f, 0.06f, 2.5f }, wood );
        m.box( Vector3{ 0, 0.92f, 0 }, Vector3{ 0.04f, 0.06f, 2.5f }, wood );
    }
    {
        MeshBuilder &m = add( PropKindE::HAYBALE, 0.0f );
        m.tube( Vector3{ -0.65f, 0.72f, 0 }, Vector3{ 0.65f, 0.72f, 0 }, 0.72f, 9, rgb( 214, 184, 96 ), Mat{}, true );
    }
    for( int v = 0; v < 3; ++v )
    {
        MeshBuilder &m = add( PropKindE::HOUSE, 5.0f );
        const Color wall[3] = { rgb( 236, 230, 214 ), rgb( 226, 210, 180 ), rgb( 200, 214, 226 ) };
        const Color roof[3] = { rgb( 156, 58, 46 ), rgb( 80, 80, 92 ), rgb( 120, 70, 50 ) };
        m.box( Vector3{ 0, 2.2f, 0 }, Vector3{ 4.0f, 2.2f, 5.0f }, wall[v] );
        const Vector3 e0 = { -4.4f, 4.4f, -5.3f }, e1 = { 4.4f, 4.4f, -5.3f }, e2 = { 4.4f, 4.4f, 5.3f }, e3 = { -4.4f, 4.4f, 5.3f };
        const Vector3 r0 = { 0, 7.0f, -5.3f }, r1 = { 0, 7.0f, 5.3f };
        m.quad( e0, r0, r1, e3, roof[v] );
        m.quad( e1, e2, r1, r0, roof[v] );
        m.tri( Vector3{ -4.0f, 4.4f, -5.0f }, Vector3{ 4.0f, 4.4f, -5.0f }, Vector3{ 0, 6.8f, -5.0f }, wall[v] );
        m.tri( Vector3{ -4.0f, 4.4f, 5.0f }, Vector3{ 4.0f, 4.4f, 5.0f }, Vector3{ 0, 6.8f, 5.0f }, wall[v] );
        windows( m, Vector3{ -4.0f, -0.4f, -5.0f }, Vector3{ 1, 0, 0 }, Vector3{ 0, 0, -1 }, 8.0f, 4.4f, 400 + v, 0.5f );
        windows( m, Vector3{ 4.0f, -0.4f, -5.0f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 10.0f, 4.4f, 410 + v, 0.5f );
        windows( m, Vector3{ -4.0f, -0.4f, 5.0f }, Vector3{ 0, 0, -1 }, Vector3{ -1, 0, 0 }, 10.0f, 4.4f, 420 + v, 0.5f );
    }
    {
        MeshBuilder &m = add( PropKindE::WINDMILL, 0.0f );
        const Color white = rgb( 236, 238, 242 );
        m.cylinder( Vector3{ 0, 0, 0 }, 1.3f, 0.6f, 42.0f, 8, white );
        m.box( Vector3{ 0, 42.6f, 0.5f }, Vector3{ 0.9f, 0.9f, 2.2f }, white );
        for( int i = 0; i < 3; ++i )
        {
            const float   a = Tau * static_cast<float>( i ) / 3.0f + 0.4f;
            const Vector3 hub = { 0, 42.6f, -1.8f };
            const Vector3 dir = { std::cos( a ), std::sin( a ), 0 };
            const Vector3 side = { -dir.y * 0.7f, dir.x * 0.7f, 0 };
            const Vector3 tip = Vector3Add( hub, Vector3Scale( dir, 19.0f ) );
            m.tri( Vector3Add( hub, side ), Vector3Subtract( hub, side ), tip, white );
        }
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::NEON_PILLAR, 0.6f );
        const Color glow = v ? rgb( 255, 50, 200 ) : rgb( 40, 220, 255 );
        m.box( Vector3{ 0, 5.0f, 0 }, Vector3{ 0.45f, 5.0f, 0.45f }, rgb( 26, 16, 44 ) );
        for( float x : { -0.47f, 0.47f } )
            for( float z : { -0.47f, 0.47f } ) m.box( Vector3{ x, 5.0f, z }, Vector3{ 0.06f, 5.0f, 0.06f }, glow, Glow );
        m.box( Vector3{ 0, 10.2f, 0 }, Vector3{ 0.6f, 0.2f, 0.6f }, glow, Glow );
    }
    {
        MeshBuilder &m = add( PropKindE::PYRAMID, 0.0f );
        const Vector3 c[4] = { { -15, 0, -15 }, { 15, 0, -15 }, { 15, 0, 15 }, { -15, 0, 15 } };
        const Vector3 top = { 0, 22, 0 };
        for( int i = 0; i < 4; ++i )
        {
            m.tri( c[i], c[( i + 1 ) % 4], top, rgb( 30, 10, 54 ) );
            m.tube( c[i], top, 0.25f, 4, rgb( 255, 60, 200 ), Glow );
            m.tube( c[i], c[( i + 1 ) % 4], 0.25f, 4, rgb( 40, 220, 255 ), Glow );
        }
    }
    for( int v = 0; v < 2; ++v )
    {
        MeshBuilder &m = add( PropKindE::SNOWBANK, 0.0f );
        m.blob( Vector3{ 0, 0.15f, 0 }, Vector3{ 1.6f, 0.65f, 2.6f }, rgb( 240, 244, 250 ), 70 + v, 0.35f );
    }

    // Road furniture.
    m_post.box( Vector3{ 0, 0.55f, 0 }, Vector3{ 0.06f, 0.55f, 0.06f }, rgb( 236, 236, 236 ) );
    m_post.box( Vector3{ 0, 0.95f, 0 }, Vector3{ 0.07f, 0.08f, 0.07f }, rgb( 255, 150, 40 ), Mat{ 0.7f, 0.0f } );
    for( int dir = 0; dir < 2; ++dir )
    {
        MeshBuilder &m = dir ? m_chevronLeft : m_chevronRight;
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 1.1f, 0 }, 0.07f, 5, rgb( 120, 120, 128 ) );
        m.box( Vector3{ 0, 1.6f, 0 }, Vector3{ 0.95f, 0.55f, 0.05f }, rgb( 200, 26, 26 ), Mat{ 0.12f, 0.0f } );
        // The arrow points to the driver's right (-x) or left (+x), drawn on the face towards the driver (-z).
        const float s = dir ? 1.0f : -1.0f;
        for( float off : { -0.35f, 0.35f } )
        {
            boardLine( m, Vector2{ -s * 0.2f + off, 1.95f }, Vector2{ s * 0.2f + off, 1.6f }, -0.07f, 0.17f,
                       rgb( 250, 250, 250 ), Mat{ 0.45f, 0.0f } );
            boardLine( m, Vector2{ s * 0.2f + off, 1.6f }, Vector2{ -s * 0.2f + off, 1.25f }, -0.07f, 0.17f,
                       rgb( 250, 250, 250 ), Mat{ 0.45f, 0.0f } );
        }
    }
    m_lamp.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 8.6f, 0 }, 0.14f, 6, rgb( 110, 112, 122 ) );
    m_lamp.tube( Vector3{ 0, 8.4f, 0 }, Vector3{ 0, 8.7f, 2.6f }, 0.09f, 5, rgb( 110, 112, 122 ) );
    m_lamp.box( Vector3{ 0, 8.65f, 2.9f }, Vector3{ 0.28f, 0.1f, 0.62f }, rgb( 70, 72, 80 ) );
    m_lamp.quad( Vector3{ -0.24f, 8.54f, 2.35f }, Vector3{ 0.24f, 8.54f, 2.35f }, Vector3{ 0.24f, 8.54f, 3.45f },
                 Vector3{ -0.24f, 8.54f, 3.45f }, rgb( 255, 226, 170 ), Glow );
    m_jumpSign.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 1.4f, 0 }, 0.07f, 5, rgb( 120, 120, 128 ) );
    m_jumpSign.tri( Vector3{ -0.75f, 1.4f, -0.04f }, Vector3{ 0.75f, 1.4f, -0.04f }, Vector3{ 0, 2.7f, -0.04f },
                    rgb( 210, 30, 30 ), Mat{ 0.15f, 0.0f } );
    m_jumpSign.tri( Vector3{ -0.5f, 1.55f, -0.06f }, Vector3{ 0.5f, 1.55f, -0.06f }, Vector3{ 0, 2.42f, -0.06f },
                    rgb( 250, 250, 250 ), Mat{ 0.3f, 0.0f } );
    m_jumpSign.tri( Vector3{ -0.32f, 1.68f, -0.07f }, Vector3{ 0.32f, 1.68f, -0.07f }, Vector3{ 0, 1.98f, -0.07f },
                    rgb( 20, 20, 20 ) );
}

void Models::buildVehicles()
{
    for( int car = 0; car < CarCount; ++car )
    {
        const CarShape shape = playerShape( car );
        for( int p = 0; p < PaintCount; ++p )
        {
            MeshBuilder mb;
            buildCar( mb, shape, PaintColors[p] );
            m_playerCars[car][p] = mb.upload();
        }
    }

    // Traffic: hatchback, saloon, van and lorry in everyday paints.
    const Color paints[8] = { rgb( 150, 150, 156 ), rgb( 40, 44, 52 ),   rgb( 120, 30, 34 ), rgb( 34, 70, 120 ),
                              rgb( 220, 220, 224 ), rgb( 60, 100, 70 ), rgb( 180, 140, 60 ), rgb( 96, 60, 110 ) };
    CarShape hatch;
    hatch.m_body = { { -1.95f, 0.84f, 0.30f, 0.95f }, { -1.7f, 0.86f, 0.25f, 1.0f }, { 1.2f, 0.86f, 0.25f, 0.92f },
                     { 1.85f, 0.80f, 0.30f, 0.78f },  { 1.95f, 0.74f, 0.36f, 0.70f } };
    hatch.m_cabin = { { -1.85f, 0.78f, 1.0f, 0.78f, 1.0f }, { -1.55f, 0.80f, 0.98f, 0.70f, 1.45f },
                      { 0.35f, 0.80f, 0.94f, 0.70f, 1.48f }, { 0.95f, 0.80f, 0.92f, 0.80f, 0.92f } };
    hatch.m_wheelFront = 1.25f;
    hatch.m_wheelRear = -1.25f;
    hatch.m_wheelR = 0.31f;
    hatch.m_sideGlass = true;
    hatch.m_wing = 0.0f;
    CarShape saloon;
    saloon.m_body = { { -2.3f, 0.86f, 0.32f, 0.92f }, { -1.9f, 0.89f, 0.27f, 0.96f }, { 1.4f, 0.89f, 0.27f, 0.92f },
                      { 2.15f, 0.84f, 0.32f, 0.80f }, { 2.3f, 0.76f, 0.38f, 0.70f } };
    saloon.m_cabin = { { -1.6f, 0.80f, 0.96f, 0.80f, 0.96f }, { -0.95f, 0.82f, 0.95f, 0.70f, 1.40f },
                       { 0.35f, 0.82f, 0.93f, 0.70f, 1.42f }, { 1.05f, 0.80f, 0.94f, 0.80f, 0.94f } };
    saloon.m_wheelFront = 1.45f;
    saloon.m_wheelRear = -1.4f;
    saloon.m_wheelR = 0.32f;
    saloon.m_sideGlass = true;
    saloon.m_wing = 0.0f;
    CarShape van;
    van.m_body = { { -2.45f, 0.95f, 0.35f, 1.05f }, { 2.0f, 0.95f, 0.35f, 1.05f }, { 2.45f, 0.90f, 0.40f, 0.95f } };
    van.m_cabin = { { -2.44f, 0.94f, 1.05f, 0.94f, 1.05f }, { -2.4f, 0.94f, 1.05f, 0.90f, 2.05f },
                    { 1.2f, 0.94f, 1.05f, 0.90f, 2.05f }, { 1.95f, 0.94f, 1.05f, 0.94f, 1.05f } };
    van.m_wheelFront = 1.6f;
    van.m_wheelRear = -1.6f;
    van.m_wheelR = 0.36f;
    van.m_sideGlass = false;
    van.m_wing = 0.0f;
    for( int p = 0; p < 8; ++p )
    {
        for( int kind = 0; kind < 3; ++kind )
        {
            MeshBuilder mb;
            const CarShape &shape = kind == 0 ? hatch : kind == 1 ? saloon : van;
            buildCar( mb, shape, paints[( p + kind * 3 ) % 8] );
            VehicleModel vm;
            vm.m_mesh = mb.upload();
            vm.m_length = kind == 0 ? 3.9f : kind == 1 ? 4.6f : 4.9f;
            vm.m_width = kind == 2 ? 1.9f : 1.76f;
            m_traffic.push_back( vm );
        }
    }
    const Color trailers[4] = { rgb( 230, 230, 230 ), rgb( 190, 40, 40 ), rgb( 40, 90, 160 ), rgb( 220, 170, 40 ) };
    for( int p = 0; p < 4; ++p )
    {
        MeshBuilder mb;
        const Color cab = paints[( p * 3 + 1 ) % 8];
        mb.box( Vector3{ 0, 1.75f, 3.85f }, Vector3{ 1.2f, 1.25f, 1.05f }, cab, Paint );
        mb.quad( Vector3{ -1.05f, 2.0f, 4.91f }, Vector3{ 1.05f, 2.0f, 4.91f }, Vector3{ 1.05f, 2.85f, 4.91f },
                 Vector3{ -1.05f, 2.85f, 4.91f }, rgb( 18, 24, 36 ), Glass );
        mb.box( Vector3{ 0, 0.62f, 3.9f }, Vector3{ 1.22f, 0.12f, 1.1f }, rgb( 30, 30, 34 ) );
        mb.box( Vector3{ 0, 2.25f, -1.25f }, Vector3{ 1.25f, 1.65f, 3.75f }, trailers[p] );
        mb.box( Vector3{ 0, 0.55f, -1.25f }, Vector3{ 1.0f, 0.12f, 3.6f }, rgb( 30, 30, 34 ) );
        for( float z : { 3.6f, -2.8f, -4.0f } )
            for( float side : { -1.0f, 1.0f } )
                mb.tube( Vector3{ side * 0.85f, 0.48f, z }, Vector3{ side * 1.18f, 0.48f, z }, 0.48f, 10, rgb( 20, 20, 22 ),
                         Mat{}, true );
        for( float side : { -1.0f, 1.0f } )
        {
            mb.quad( Vector3{ side * 0.7f, 0.8f, -5.01f }, Vector3{ side * 1.15f, 0.8f, -5.01f }, Vector3{ side * 1.15f, 1.05f, -5.01f },
                     Vector3{ side * 0.7f, 1.05f, -5.01f }, rgb( 255, 24, 24 ), Glow );
            mb.quad( Vector3{ side * 0.6f, 0.9f, 4.91f }, Vector3{ side * 1.05f, 0.9f, 4.91f }, Vector3{ side * 1.05f, 1.15f, 4.91f },
                     Vector3{ side * 0.6f, 1.15f, 4.91f }, rgb( 255, 246, 224 ), Glow );
        }
        VehicleModel vm;
        vm.m_mesh = mb.upload();
        vm.m_length = 10.0f;
        vm.m_width = 2.5f;
        vm.m_truck = true;
        m_traffic.push_back( vm );
    }

    { // Cone.
        MeshBuilder mb;
        mb.box( Vector3{ 0, 0.02f, 0 }, Vector3{ 0.24f, 0.02f, 0.24f }, rgb( 20, 20, 20 ) );
        mb.cylinder( Vector3{ 0, 0.04f, 0 }, 0.2f, 0.03f, 0.72f, 8, rgb( 255, 104, 16 ), Mat{ 0.15f, 0.0f } );
        mb.cylinder( Vector3{ 0, 0.34f, 0 }, 0.135f, 0.115f, 0.12f, 8, rgb( 250, 250, 250 ), Mat{ 0.5f, 0.0f } );
        m_cone = mb.upload();
    }
    for( int dir = 0; dir < 2; ++dir )
    {
        MeshBuilder mb;
        for( float x : { -0.9f, 0.9f } ) mb.tube( Vector3{ x, 0, 0 }, Vector3{ x, 0.6f, 0 }, 0.05f, 4, rgb( 200, 200, 200 ) );
        mb.box( Vector3{ 0, 1.15f, 0 }, Vector3{ 1.05f, 0.58f, 0.06f }, rgb( 24, 24, 28 ) );
        const float s = dir ? 1.0f : -1.0f;
        for( float off : { -0.45f, 0.1f } )
        {
            boardLine( mb, Vector2{ -s * ( off + 0.0f ), 1.5f }, Vector2{ -s * ( off + 0.35f ), 1.15f }, -0.08f, 0.16f,
                       rgb( 255, 196, 40 ), Glow );
            boardLine( mb, Vector2{ -s * ( off + 0.35f ), 1.15f }, Vector2{ -s * ( off + 0.0f ), 0.8f }, -0.08f, 0.16f,
                       rgb( 255, 196, 40 ), Glow );
        }
        m_arrowBoard[dir] = mb.upload();
    }
    { // Soft shadow: dark centre, transparent rim.
        MeshBuilder mb;
        const Color inner = { 0, 0, 0, 150 };
        const Color outer = { 0, 0, 0, 0 };
        for( int i = 0; i < 12; ++i )
        {
            const float   a0 = Tau * static_cast<float>( i ) / 12.0f, a1 = Tau * static_cast<float>( i + 1 ) / 12.0f;
            const Vector3 i0 = { std::cos( a0 ) * 0.38f, 0, std::sin( a0 ) * 0.42f };
            const Vector3 i1 = { std::cos( a1 ) * 0.38f, 0, std::sin( a1 ) * 0.42f };
            const Vector3 o0 = { std::cos( a0 ) * 0.62f, 0, std::sin( a0 ) * 0.58f };
            const Vector3 o1 = { std::cos( a1 ) * 0.62f, 0, std::sin( a1 ) * 0.58f };
            mb.tri3( Vector3{ 0, 0, 0 }, i0, i1, inner, inner, inner );
            mb.tri3( i0, o0, o1, inner, outer, outer );
            mb.tri3( i0, o1, i1, inner, outer, inner );
        }
        m_shadow = mb.upload();
    }
}

void Models::build()
{
    buildProps();
    buildVehicles();
}

void Models::unload()
{
    for( auto &car : m_playerCars )
        for( auto &m : car )
            if( m.vaoId ) UnloadMesh( m );
    for( auto &v : m_traffic ) UnloadMesh( v.m_mesh );
    m_traffic.clear();
    UnloadMesh( m_cone );
    for( auto &m : m_arrowBoard ) UnloadMesh( m );
    UnloadMesh( m_shadow );
}

} // namespace sunstreak
