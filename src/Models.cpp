/**
 * @file Models.cpp
 * @brief The procedural models.
 * @author reneweller
 */
#include "Models.h"

#include "Data.h"

#include "MathUtil.h"
#include "Rng.h"
#include "Spectacle.h"
#include "Train.h"
#include "VectorFont.h"

#include <cstdlib>
#include <initializer_list>
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

constexpr Mat Glow{ 1.0f, 0.0f };      ///< fully self-lit
constexpr float CreatureScale = 1.35f;   ///< what swims round the undersea tunnels, larger than life
constexpr Mat Paint{ 0.0f, 0.55f };    ///< car paint
constexpr Mat Glass{ 0.0f, 0.95f };    ///< windows
constexpr Mat Chrome{ 0.0f, 0.75f };   ///< rims, rails

const CarSpec CarSpecs[CarCount] = {
    { "ARROW GT", 80.5f, 1.0f, 1.0f, 4.5f, 1.88f, 8, 900.0f, 7800.0f, 1.0f, 0.86f },
    { "SPARROW", 72.0f, 1.16f, 1.12f, 3.9f, 1.72f, 4, 1000.0f, 8600.0f, 1.35f, 0.92f },
    { "VORTEX", 88.0f, 0.92f, 0.93f, 4.8f, 2.0f, 12, 1100.0f, 9200.0f, 1.15f, 0.82f },
    { "KESTREL", 76.0f, 1.12f, 1.05f, 4.1f, 1.82f, 4, 950.0f, 8200.0f, 1.25f, 1.2f },
};

const char *PaintNames[PaintCount] = { "RACING RED",    "ICE WHITE",   "SUN YELLOW",    "DEEP BLUE",   "CHAMPION GOLD",
                                       "MIDNIGHT BLACK", "LIME GREEN",  "SUNSET ORANGE", "MIAMI PINK",  "GULF BLUE",
                                       "BRITISH GREEN", "SILVER ARROW", "PEARL PURPLE",  "GLOBETROTTER" };
const Color PaintColors[PaintCount] = { rgb( 196, 18, 24 ),  rgb( 232, 234, 240 ), rgb( 240, 186, 16 ), rgb( 22, 56, 168 ),
                                        rgb( 214, 168, 52 ), rgb( 22, 22, 26 ),    rgb( 120, 210, 40 ), rgb( 240, 110, 20 ),
                                        rgb( 236, 80, 160 ), rgb( 120, 180, 220 ), rgb( 20, 76, 46 ),   rgb( 178, 182, 190 ),
                                        rgb( 110, 60, 160 ), rgb( 20, 130, 150 ) };

const char *RimNames[RimCount] = { "SILVER", "BLACK", "WHITE", "GOLD", "BRONZE", "RED", "GUNMETAL" };
const Color RimColors[RimCount] = { rgb( 170, 172, 178 ), rgb( 26, 26, 28 ),  rgb( 232, 232, 234 ), rgb( 214, 170, 60 ),
                                    rgb( 150, 100, 52 ),  rgb( 190, 30, 30 ), rgb( 76, 80, 88 ) };

const char *DecalNames[DecalCount] = { "NONE", "TWIN STRIPES", "SIDE STRIPE", "ROUNDEL", "TWO TONE", "CHEQUERS", "GLOBE" };

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
    bool                      m_rally = false; ///< mud flaps, a light pod and a roof scoop
};

/** @brief What goes on a car besides its paint. */
struct Dress
{
    Color m_rims = rgb( 170, 172, 178 );   ///< rim colour
    int   m_decal = 0;                     ///< decal (DecalNames)
    Color m_decalColor{};                  ///< decal colour
};

/**
 * @brief A flat disc standing on a car's flank (roundels, the globe).
 * @param[out] mb    where the triangles go
 * @param[in]  c     its centre
 * @param[in]  side  -1 left, +1 right (the disc faces outward)
 * @param[in]  r     radius
 * @param[in]  col   colour
 */
void flankDisc( MeshBuilder &mb, Vector3 c, float side, float r, Color col )
{
    const int n = 14;
    for( int i = 0; i < n; ++i )
    {
        const float   a0 = Tau * static_cast<float>( i ) / n, a1 = Tau * static_cast<float>( i + 1 ) / n;
        const Vector3 p0 = { c.x, c.y + std::sin( a0 ) * r, c.z + std::cos( a0 ) * r };
        const Vector3 p1 = { c.x, c.y + std::sin( a1 ) * r, c.z + std::cos( a1 ) * r };
        if( side > 0 ) mb.tri( c, p0, p1, col, Mat{ 0.0f, 0.4f } );
        else mb.tri( c, p1, p0, col, Mat{ 0.0f, 0.4f } );
    }
}

/**
 * @brief Lofts a car body, its cabin, wheels and lights.
 * @param[out] mb     where the triangles go
 * @param[in]  shape  the outline
 * @param[in]  paint  body colour
 * @param[in]  dress  rims and decal
 */
void buildCar( MeshBuilder &mb, const CarShape &shape, Color paint, const Dress &dress = {} )
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
            // Two tone: the lower flanks in the decal's colour.
            const Color coat = dress.m_decal == 4 && ( k == 0 || k == 1 || k == 5 || k == 6 ) ? dress.m_decalColor : paint;
            const Color c = k == 7 ? under : ( k == 0 || k == 6 ) ? scaleColor( coat, 0.72f ) : coat;
            mb.quad( r0[k], r0[n], r1[n], r1[k], c, k == 7 ? Mat{} : Paint );
        }
        const Station &s0 = body[i], &s1 = body[i + 1];
        if( dress.m_decal == 1 )
        { // Twin stripes over the top.
            for( float x0 : { -0.34f, 0.14f } )
            {
                const float x1 = x0 + 0.2f;
                mb.quad( Vector3{ x0 * s0.m_hw, s0.m_yt + 0.012f, s0.m_z }, Vector3{ x1 * s0.m_hw, s0.m_yt + 0.012f, s0.m_z },
                         Vector3{ x1 * s1.m_hw, s1.m_yt + 0.012f, s1.m_z }, Vector3{ x0 * s1.m_hw, s1.m_yt + 0.012f, s1.m_z }, dress.m_decalColor, Paint );
            }
        }
        if( dress.m_decal == 2 )
        { // A band along both flanks.
            for( float side : { -1.0f, 1.0f } )
            {
                const float h0 = s0.m_yt - s0.m_yb, h1 = s1.m_yt - s1.m_yb;
                const Vector3 a = { side * ( s0.m_hw + 0.012f ), s0.m_yb + 0.42f * h0, s0.m_z }, b = { side * ( s1.m_hw + 0.012f ), s1.m_yb + 0.42f * h1, s1.m_z };
                const Vector3 c = { side * ( s1.m_hw + 0.012f ), s1.m_yb + 0.6f * h1, s1.m_z }, d = { side * ( s0.m_hw + 0.012f ), s0.m_yb + 0.6f * h0, s0.m_z };
                if( side > 0 ) mb.quad( a, b, c, d, dress.m_decalColor, Paint );
                else mb.quad( b, a, d, c, dress.m_decalColor, Paint );
            }
        }
        if( dress.m_decal == 5 && i + 3 >= body.size() )
        { // Chequers across the bonnet.
            for( int row = 0; row < 2; ++row )
                for( int col = 0; col < 8; ++col )
                {
                    const float t0 = row * 0.5f, t1 = t0 + 0.5f;
                    const float u0 = -0.8f + 0.2f * col, u1 = u0 + 0.2f;
                    auto p = [&]( float t, float u )
                    {
                        const float hw = lerpf( s0.m_hw, s1.m_hw, t );
                        return Vector3{ u * hw, lerpf( s0.m_yt, s1.m_yt, t ) + 0.012f, lerpf( s0.m_z, s1.m_z, t ) };
                    };
                    mb.quad( p( t0, u0 ), p( t0, u1 ), p( t1, u1 ), p( t1, u0 ), ( row + col ) % 2 ? dress.m_decalColor : rgb( 240, 240, 240 ), Paint );
                }
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
        if( dress.m_decal == 1 && !windowTop )
            for( float x0 : { -0.34f, 0.14f } )
            {
                const float x1 = x0 + 0.2f;
                mb.quad( Vector3{ x0 * a.m_hwT * 1.2f, a.m_yT + 0.012f, a.m_z }, Vector3{ x1 * a.m_hwT * 1.2f, a.m_yT + 0.012f, a.m_z },
                         Vector3{ x1 * b.m_hwT * 1.2f, b.m_yT + 0.012f, b.m_z }, Vector3{ x0 * b.m_hwT * 1.2f, b.m_yT + 0.012f, b.m_z }, dress.m_decalColor, Paint );
            }
    }
    // Roundels and globes on the doors.
    if( dress.m_decal == 3 || dress.m_decal == 6 )
    {
        const Station &mid = body[body.size() / 2];
        const float    y = mid.m_yb + 0.5f * ( mid.m_yt - mid.m_yb ), r = 0.22f * ( mid.m_yt - mid.m_yb ) / 0.6f;
        for( float side : { -1.0f, 1.0f } )
        {
            const float x = side * ( mid.m_hw + 0.012f );
            if( dress.m_decal == 3 )
            {
                flankDisc( mb, Vector3{ x, y, mid.m_z }, side, r, dress.m_decalColor );
                flankDisc( mb, Vector3{ x + side * 0.003f, y, mid.m_z }, side, r * 0.72f, rgb( 244, 244, 240 ) );
            }
            else
            { // The globe: an ocean, two continents and a meridian.
                flankDisc( mb, Vector3{ x, y, mid.m_z }, side, r, rgb( 30, 90, 190 ) );
                flankDisc( mb, Vector3{ x + side * 0.003f, y + r * 0.25f, mid.m_z - r * 0.3f }, side, r * 0.35f, rgb( 70, 170, 70 ) );
                flankDisc( mb, Vector3{ x + side * 0.003f, y - r * 0.3f, mid.m_z + r * 0.35f }, side, r * 0.3f, rgb( 70, 170, 70 ) );
                const Vector3 a = { x + side * 0.006f, y - r, mid.m_z - 0.012f }, b = { x + side * 0.006f, y + r, mid.m_z + 0.012f };
                const Vector3 c = { x + side * 0.006f, y + r, mid.m_z - 0.012f }, d = { x + side * 0.006f, y - r, mid.m_z + 0.012f };
                if( side > 0 ) mb.quad( a, d, b, c, rgb( 240, 240, 240 ) );
                else mb.quad( d, a, c, b, rgb( 240, 240, 240 ) );
            }
        }
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
                     dress.m_rims, Chrome, true );
            // Five spokes in a darker shade.
            const float x = side * ( hw + 0.007f );
            for( int k = 0; k < 5; ++k )
            {
                const float   a = Tau * static_cast<float>( k ) / 5.0f;
                const Vector3 d = { 0, std::sin( a ), std::cos( a ) }, n = { 0, -d.z * 0.05f, d.y * 0.05f };
                const Vector3 p0 = { x, r + d.y * r * 0.12f + n.y, z + d.z * r * 0.12f + n.z }, p1 = { x, r + d.y * r * 0.12f - n.y, z + d.z * r * 0.12f - n.z };
                const Vector3 p2 = { x, r + d.y * r * 0.54f - n.y, z + d.z * r * 0.54f - n.z }, p3 = { x, r + d.y * r * 0.54f + n.y, z + d.z * r * 0.54f + n.z };
                if( side > 0 ) mb.quad( p0, p1, p2, p3, scaleColor( dress.m_rims, 0.55f ), Chrome );
                else mb.quad( p1, p0, p3, p2, scaleColor( dress.m_rims, 0.55f ), Chrome );
            }
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
    if( shape.m_rally )
    { // Mud flaps behind the wheels, a pod of lamps on the bonnet, a scoop on the roof.
        for( float side : { -1.0f, 1.0f } )
            for( float z : { shape.m_wheelFront - shape.m_wheelR - 0.12f, shape.m_wheelRear - shape.m_wheelR - 0.12f } )
                mb.box( Vector3{ side * ( hw - 0.15f ), 0.22f, z }, Vector3{ 0.16f, 0.2f, 0.015f }, rgb( 24, 24, 26 ) );
        const Station &bonnet = body[body.size() - 2];
        mb.box( Vector3{ 0, bonnet.m_yt + 0.08f, bonnet.m_z + 0.15f }, Vector3{ 0.55f, 0.07f, 0.06f }, rgb( 24, 24, 26 ) );
        for( int k = 0; k < 4; ++k )
            mb.quad( Vector3{ -0.48f + 0.25f * k, bonnet.m_yt + 0.03f, bonnet.m_z + 0.215f }, Vector3{ -0.32f + 0.25f * k, bonnet.m_yt + 0.03f, bonnet.m_z + 0.215f },
                     Vector3{ -0.32f + 0.25f * k, bonnet.m_yt + 0.13f, bonnet.m_z + 0.215f }, Vector3{ -0.48f + 0.25f * k, bonnet.m_yt + 0.13f, bonnet.m_z + 0.215f },
                     rgb( 255, 246, 224 ), Mat{ 0.8f, 0.3f } );
        const CabinStation &roof = shape.m_cabin[1];
        mb.taperedBox( Vector3{ 0, roof.m_yT, roof.m_z + 0.2f }, Vector2{ 0.2f, 0.25f }, Vector3{ 0, roof.m_yT + 0.1f, roof.m_z + 0.15f }, Vector2{ 0.16f, 0.18f },
                       rgb( 24, 24, 26 ) );
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
    case 3: // Rally hatchback: high off the ground, a wing, flaps and lamps.
        s.m_body = { { -2.0f, 0.86f, 0.42f, 0.98f }, { -1.75f, 0.9f, 0.36f, 1.02f }, { 0.4f, 0.9f, 0.34f, 0.96f },
                     { 1.4f, 0.88f, 0.36f, 0.86f },  { 1.95f, 0.8f, 0.42f, 0.74f },  { 2.08f, 0.7f, 0.46f, 0.66f } };
        s.m_cabin = { { -1.85f, 0.8f, 1.0f, 0.8f, 1.0f }, { -1.5f, 0.82f, 0.98f, 0.66f, 1.5f },
                      { 0.2f, 0.82f, 0.94f, 0.66f, 1.52f }, { 0.85f, 0.84f, 0.88f, 0.84f, 0.88f } };
        s.m_wheelFront = 1.35f;
        s.m_wheelRear = -1.3f;
        s.m_wheelR = 0.36f;
        s.m_sideGlass = true;
        s.m_wing = 0.22f;
        s.m_rally = true;
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

Color paintColor( int index )
{
    return PaintColors[std::clamp( index, 0, PaintCount - 1 )];
}

const char *rimName( int index )
{
    return RimNames[std::clamp( index, 0, RimCount - 1 )];
}

const char *decalName( int index )
{
    return DecalNames[std::clamp( index, 0, DecalCount - 1 )];
}

void Models::buildLivery( const Livery &livery )
{
    if( m_livery.vaoId && livery == m_liveryOf ) return;
    if( m_livery.vaoId ) UnloadMesh( m_livery );
    Dress dress;
    dress.m_rims = RimColors[std::clamp( livery.m_rims, 0, RimCount - 1 )];
    dress.m_decal = std::clamp( livery.m_decal, 0, DecalCount - 1 );
    dress.m_decalColor = PaintColors[std::clamp( livery.m_decalColor, 0, PaintCount - 1 )];
    MeshBuilder mb;
    buildCar( mb, playerShape( std::clamp( livery.m_car, 0, CarCount - 1 ) ), PaintColors[std::clamp( livery.m_paint, 0, PaintCount - 1 )], dress );
    m_livery = mb.upload();
    m_liveryOf = livery;
}

void Models::buildPlayerPlate( PlateE style, const std::string &code, const std::string &text )
{
    const std::string key = std::to_string( static_cast<int>( style ) ) + "|" + code + "|" + text;
    if( key == m_playerPlateOf && ( m_playerPlate.vaoId || text.empty() ) ) return;
    if( m_playerPlate.vaoId ) UnloadMesh( m_playerPlate );
    m_playerPlate = Mesh{};
    m_playerPlateOf = key;
    if( text.empty() ) return;
    // The plate as the traffic's (+x is the viewer's left from behind), its letters in the game's own strokes.
    const bool  small = style == PlateE::US || style == PlateE::JP;
    const float hw = small ? 0.17f : 0.26f, hh = small ? 0.08f : 0.06f;
    const Color face = style == PlateE::UK || style == PlateE::AMBER ? rgb( 250, 206, 30 ) : style == PlateE::DARK ? rgb( 20, 20, 22 ) : rgb( 240, 240, 236 );
    const Color ink = style == PlateE::DARK ? rgb( 240, 240, 236 ) : style == PlateE::JP ? rgb( 20, 110, 60 ) : rgb( 30, 30, 34 );
    MeshBuilder m;
    m.box( Vector3{ 0, 0, 0 }, Vector3{ hw, hh, 0.01f }, face, Mat{ 0.1f, 0.0f } );
    if( style == PlateE::US ) m.box( Vector3{ 0, hh - 0.015f, -0.012f }, Vector3{ hw, 0.015f, 0.002f }, rgb( 180, 30, 40 ) );
    auto write = [&]( const std::string &what, float left, float right, float height, float midY, Color col )
    {
        std::vector<std::pair<Vector2, Vector2>> lines;
        const float width = textStrokes( what, lines );
        if( width <= 0.0f ) return;
        const float scale = std::min( height, ( left - right ) / width ), x0 = ( left + right ) * 0.5f + width * scale * 0.5f;
        const float y0 = midY - scale * 0.5f, t = std::max( 0.004f, scale * 0.13f );
        for( const auto &[a, b] : lines )
        {
            const Vector3 p = { x0 - a.x * scale, y0 + a.y * scale, -0.0125f }, q = { x0 - b.x * scale, y0 + b.y * scale, -0.0125f };
            const float   dx = q.x - p.x, dy = q.y - p.y, len = std::sqrt( dx * dx + dy * dy );
            if( len <= 0.0f ) continue;
            const Vector3 n = { -dy / len * t * 0.5f, dx / len * t * 0.5f, 0.0f };
            const Vector3 c[4] = { { p.x - n.x, p.y - n.y, p.z }, { q.x - n.x, q.y - n.y, q.z }, { q.x + n.x, q.y + n.y, q.z }, { p.x + n.x, p.y + n.y, p.z } };
            m.quad( c[0], c[1], c[2], c[3], col );
            m.quad( c[3], c[2], c[1], c[0], col );
        }
    };
    float right = -hw + 0.02f;
    float left = hw - 0.02f;
    if( style == PlateE::EU )
    {
        m.box( Vector3{ hw - 0.025f, 0, -0.012f }, Vector3{ 0.025f, hh, 0.002f }, rgb( 0, 51, 153 ) );
        if( !code.empty() ) write( code, hw - 0.006f, hw - 0.044f, 0.022f, -hh * 0.45f, rgb( 240, 240, 236 ) );
        left = hw - 0.06f;
    }
    write( text, left, right, hh * 1.2f, 0.0f, ink );
    m_playerPlate = m.upload();
}

const MeshBuilder &Models::prop( PropKindE kind, uint64_t variant ) const
{
    static const MeshBuilder none;
    const auto              &v = m_props[static_cast<size_t>( kind )];
    return v.empty() ? none : v[static_cast<size_t>( variant % v.size() )];
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

    // A slanted pointed prism (crystals, ice): a tube with a tip.
    auto prism = []( MeshBuilder &m, Vector3 base, Vector3 dir, float len, float radius, int sides, float tip, Color col, Mat mat )
    {
        dir = Vector3Normalize( dir );
        const Vector3 top = Vector3Add( base, Vector3Scale( dir, len ) );
        m.tube( base, top, radius, sides, col, mat, false );
        const Vector3 ref = std::abs( dir.y ) < 0.9f ? Vector3{ 0, 1, 0 } : Vector3{ 1, 0, 0 };
        const Vector3 u = Vector3Normalize( Vector3CrossProduct( dir, ref ) );
        const Vector3 v = Vector3CrossProduct( dir, u );
        const Vector3 apex = Vector3Add( top, Vector3Scale( dir, tip ) );
        for( int i = 0; i < sides; ++i )
        {
            const float a0 = Tau * static_cast<float>( i ) / static_cast<float>( sides );
            const float a1 = Tau * static_cast<float>( i + 1 ) / static_cast<float>( sides );
            auto ring = [&]( float a )
            { return Vector3Add( top, Vector3Add( Vector3Scale( u, std::cos( a ) * radius ), Vector3Scale( v, std::sin( a ) * radius ) ) ); };
            m.tri( ring( a0 ), ring( a1 ), apex, scaleColor( col, 1.1f ), mat );
        }
    };
    // A pitched roof over a box of half size hx, hz whose walls end at height y.
    auto roof = []( MeshBuilder &m, float hx, float hz, float y, float ridge, Color col )
    {
        const Vector3 e0 = { -hx - 0.4f, y, -hz - 0.3f }, e1 = { hx + 0.4f, y, -hz - 0.3f }, e2 = { hx + 0.4f, y, hz + 0.3f },
                      e3 = { -hx - 0.4f, y, hz + 0.3f };
        const Vector3 r0 = { 0, y + ridge, -hz - 0.3f }, r1 = { 0, y + ridge, hz + 0.3f };
        m.quad( e0, r0, r1, e3, col );
        m.quad( e1, e2, r1, r0, col );
    };
    auto gables = []( MeshBuilder &m, float hx, float hz, float y, float ridge, Color col )
    {
        m.tri( Vector3{ -hx, y, -hz }, Vector3{ hx, y, -hz }, Vector3{ 0, y + ridge - 0.2f, -hz }, col );
        m.tri( Vector3{ -hx, y, hz }, Vector3{ hx, y, hz }, Vector3{ 0, y + ridge - 0.2f, hz }, col );
    };

    for( int v = 0; v < 3; ++v )
    { // Autumn trees: orange, red, yellow.
        MeshBuilder &m = add( PropKindE::AUTUMN_TREE, 0.7f );
        const Color  leaf[3] = { rgb( 224, 120, 30 ), rgb( 200, 52, 30 ), rgb( 236, 190, 50 ) };
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.1f * v, 2.4f, 0 }, 0.28f, 6, rgb( 86, 62, 44 ) );
        m.blob( Vector3{ 0.1f * v, 3.9f, 0 }, Vector3{ 2.4f, 2.0f, 2.3f }, leaf[v], 111 + v, 0.45f );
        m.blob( Vector3{ -0.8f, 4.6f, 0.4f }, Vector3{ 1.5f, 1.3f, 1.4f }, scaleColor( leaf[v], 1.1f ), 121 + v, 0.45f );
    }
    for( int v = 0; v < 2; ++v )
    { // Birches.
        MeshBuilder &m = add( PropKindE::BIRCH, 0.4f );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.15f * v, 5.2f, 0 }, 0.17f, 6, rgb( 228, 226, 216 ) );
        for( int i = 0; i < 4; ++i )
            m.box( Vector3{ 0.03f * v * i, 0.8f + 1.1f * i, 0.17f }, Vector3{ 0.08f, 0.04f, 0.02f }, rgb( 30, 30, 30 ) );
        m.blob( Vector3{ 0.15f * v, 5.6f, 0 }, Vector3{ 1.4f, 2.4f, 1.3f }, v ? rgb( 210, 190, 60 ) : rgb( 150, 182, 64 ), 131 + v, 0.4f );
    }
    for( int v = 0; v < 3; ++v )
    { // Cherry trees in blossom.
        MeshBuilder &m = add( PropKindE::CHERRY, 0.6f );
        const Color  bark = rgb( 80, 52, 42 );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.3f, 2.0f, 0 }, 0.28f, 6, bark );
        m.tube( Vector3{ 0.3f, 2.0f, 0 }, Vector3{ -1.2f, 3.2f, 0.3f }, 0.14f, 5, bark );
        m.tube( Vector3{ 0.3f, 2.0f, 0 }, Vector3{ 1.5f, 3.0f, -0.4f }, 0.14f, 5, bark );
        const Color pink[3] = { rgb( 250, 182, 210 ), rgb( 240, 160, 200 ), rgb( 255, 204, 226 ) };
        m.blob( Vector3{ -1.2f, 3.6f, 0.3f }, Vector3{ 1.8f, 1.3f, 1.7f }, pink[v], 141 + v, 0.5f );
        m.blob( Vector3{ 1.4f, 3.5f, -0.3f }, Vector3{ 1.7f, 1.2f, 1.6f }, pink[( v + 1 ) % 3], 151 + v, 0.5f );
        m.blob( Vector3{ 0.2f, 4.3f, 0 }, Vector3{ 1.6f, 1.2f, 1.6f }, pink[( v + 2 ) % 3], 161 + v, 0.5f );
    }
    { // Torii: a red shrine gate.
        MeshBuilder &m = add( PropKindE::TORII, 0.4f );
        const Color  red = rgb( 214, 44, 30 );
        for( float x : { -2.4f, 2.4f } ) m.cylinder( Vector3{ x, 0, 0 }, 0.3f, 0.26f, 5.2f, 8, red );
        m.box( Vector3{ 0, 5.45f, 0 }, Vector3{ 3.5f, 0.24f, 0.38f }, rgb( 30, 28, 30 ) );
        m.box( Vector3{ 0, 5.1f, 0 }, Vector3{ 3.2f, 0.16f, 0.3f }, red );
        m.box( Vector3{ 0, 4.3f, 0 }, Vector3{ 2.9f, 0.16f, 0.2f }, red );
        m.box( Vector3{ 0, 4.7f, 0 }, Vector3{ 0.16f, 0.3f, 0.18f }, red );
    }
    { // Stone lantern with a warm light.
        MeshBuilder &m = add( PropKindE::LANTERN, 0.3f );
        const Color  stone = rgb( 150, 150, 144 );
        m.box( Vector3{ 0, 0.15f, 0 }, Vector3{ 0.4f, 0.15f, 0.4f }, stone );
        m.cylinder( Vector3{ 0, 0.3f, 0 }, 0.15f, 0.15f, 0.8f, 6, stone );
        m.box( Vector3{ 0, 1.3f, 0 }, Vector3{ 0.32f, 0.22f, 0.32f }, rgb( 255, 214, 140 ), Mat{ 0.9f, 0.0f } );
        m.taperedBox( Vector3{ 0, 1.52f, 0 }, Vector2{ 0.55f, 0.55f }, Vector3{ 0, 1.85f, 0 }, Vector2{ 0.1f, 0.1f }, stone );
    }
    { // Pagoda: four tiers.
        MeshBuilder &m = add( PropKindE::PAGODA, 0.0f );
        float y = 0.0f;
        for( int i = 0; i < 4; ++i )
        {
            const float w = 4.0f - 0.7f * static_cast<float>( i );
            m.box( Vector3{ 0, y + 1.4f, 0 }, Vector3{ w, 1.4f, w }, rgb( 160, 52, 40 ) );
            m.taperedBox( Vector3{ 0, y + 2.8f, 0 }, Vector2{ w + 1.6f, w + 1.6f }, Vector3{ 0, y + 3.6f, 0 }, Vector2{ w * 0.6f, w * 0.6f },
                          rgb( 56, 66, 64 ) );
            y += 3.6f;
        }
        m.cylinder( Vector3{ 0, y, 0 }, 0.25f, 0.05f, 4.0f, 6, rgb( 190, 160, 60 ) );
    }
    for( int v = 0; v < 3; ++v )
    { // Rainforest giants with buttress roots.
        MeshBuilder &m = add( PropKindE::JUNGLE_TREE, 1.0f );
        const float  h = 12.0f + 3.0f * v;
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.2f * v, h, 0 }, 0.45f, 7, rgb( 132, 112, 82 ) );
        for( int i = 0; i < 3; ++i )
        {
            const float a = Tau * static_cast<float>( i ) / 3.0f + 0.4f * v;
            m.tri( Vector3{ 0, 2.2f, 0 }, Vector3{ std::cos( a ) * 1.6f, 0, std::sin( a ) * 1.6f }, Vector3{ 0, 0, 0 }, rgb( 120, 100, 72 ) );
        }
        m.blob( Vector3{ 0.2f * v, h, 0 }, Vector3{ 5.5f, 2.2f, 5.2f }, rgb( 40 + v * 8, 100, 36 ), 171 + v, 0.5f );
        m.blob( Vector3{ 1.5f, h + 1.2f, 1.0f }, Vector3{ 3.0f, 1.6f, 3.0f }, rgb( 56, 120, 44 ), 181 + v, 0.5f );
    }
    for( int v = 0; v < 2; ++v )
    { // Ferns.
        MeshBuilder &m = add( PropKindE::FERN, 0.0f );
        for( int i = 0; i < 8; ++i )
        {
            const float   a = Tau * ( static_cast<float>( i ) + 0.5f * v ) / 8.0f;
            const Vector3 d = { std::cos( a ), 0, std::sin( a ) };
            const Vector3 s = { -d.z * 0.16f, 0, d.x * 0.16f };
            const Vector3 mid = Vector3Add( Vector3Scale( d, 0.8f ), Vector3{ 0, 0.9f, 0 } );
            const Vector3 tip = Vector3Add( Vector3Scale( d, 1.7f ), Vector3{ 0, 0.3f, 0 } );
            m.quad( Vector3{ 0, 0.1f, 0 }, Vector3Add( mid, s ), tip, Vector3Subtract( mid, s ), i % 2 ? rgb( 56, 128, 46 ) : rgb( 76, 146, 56 ) );
        }
    }
    for( int v = 0; v < 2; ++v )
    { // Acacias: flat tops.
        MeshBuilder &m = add( PropKindE::ACACIA, 0.5f );
        const Color  bark = rgb( 70, 56, 44 );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0.3f, 3.4f, 0 }, 0.25f, 6, bark );
        m.tube( Vector3{ 0.3f, 3.0f, 0 }, Vector3{ -1.8f, 4.3f, 0.5f }, 0.13f, 5, bark );
        m.tube( Vector3{ 0.3f, 3.2f, 0 }, Vector3{ 2.0f, 4.4f, -0.4f }, 0.13f, 5, bark );
        m.blob( Vector3{ 0.2f, 4.6f, 0 }, Vector3{ 4.6f, 0.75f, 4.0f }, rgb( 100 + 10 * v, 118, 48 ), 191 + v, 0.35f );
        m.blob( Vector3{ -1.6f, 4.9f, 0.6f }, Vector3{ 2.4f, 0.6f, 2.2f }, rgb( 110, 126, 54 ), 201 + v, 0.35f );
    }
    { // Termite mound.
        MeshBuilder &m = add( PropKindE::TERMITE_MOUND, 0.7f );
        const Color  clay = rgb( 172, 112, 62 );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.95f, 0.25f, 2.8f, 7, clay );
        m.cylinder( Vector3{ 0.6f, 0, 0.3f }, 0.45f, 0.1f, 1.6f, 6, scaleColor( clay, 0.9f ) );
        m.cylinder( Vector3{ -0.5f, 0, -0.4f }, 0.4f, 0.08f, 1.3f, 6, scaleColor( clay, 1.08f ) );
    }
    for( int v = 0; v < 2; ++v )
    { // Dry grass.
        MeshBuilder &m = add( PropKindE::GRASS_TUFT, 0.0f );
        Rng          r( 1100 + v );
        for( int i = 0; i < 8; ++i )
        {
            const float   a = r.range( 0.0f, Tau );
            const Vector3 base = { std::cos( a ) * r.range( 0.0f, 0.4f ), 0, std::sin( a ) * r.range( 0.0f, 0.4f ) };
            const Vector3 tip = { base.x + r.range( -0.5f, 0.5f ), r.range( 0.5f, 1.0f ), base.z + r.range( -0.5f, 0.5f ) };
            m.tri( Vector3Subtract( base, Vector3{ 0.06f, 0, 0.04f } ), Vector3Add( base, Vector3{ 0.06f, 0, 0.04f } ), tip,
                   i % 2 ? rgb( 206, 184, 104 ) : rgb( 180, 160, 90 ) );
        }
    }
    for( int v = 0; v < 2; ++v )
    { // Lava rocks: black with glowing cracks.
        MeshBuilder &m = add( PropKindE::LAVA_ROCK, 0.9f );
        m.blob( Vector3{ 0, 0.4f, 0 }, Vector3{ 1.2f, 0.9f, 1.0f }, rgb( 40, 36, 38 ), 211 + v, 0.7f );
        Rng r( 1200 + v );
        for( int i = 0; i < 5; ++i )
        {
            const float a = r.range( 0.0f, Tau );
            const Vector3 p = { std::cos( a ) * 1.05f, r.range( 0.1f, 0.9f ), std::sin( a ) * 0.9f };
            m.box( p, Vector3{ 0.08f + 0.18f * std::abs( std::sin( a ) ), r.range( 0.15f, 0.4f ), 0.08f + 0.18f * std::abs( std::cos( a ) ) },
                   rgb( 255, 110, 20 ), Mat{ 1.0f, 0.0f } );
        }
    }
    for( int v = 0; v < 2; ++v )
    { // Ice spikes.
        MeshBuilder &m = add( PropKindE::ICE_SPIKE, 0.5f );
        Rng          r( 1300 + v );
        for( int i = 0; i < 4 + v; ++i )
            prism( m, Vector3{ r.range( -0.8f, 0.8f ), -0.2f, r.range( -0.8f, 0.8f ) },
                   Vector3{ r.range( -0.35f, 0.35f ), 1.0f, r.range( -0.35f, 0.35f ) }, r.range( 1.5f, 4.0f ), r.range( 0.2f, 0.45f ), 6,
                   r.range( 0.6f, 1.2f ), rgb( 170, 220, 255 ), Mat{ 0.15f, 0.9f } );
    }
    for( int v = 0; v < 2; ++v )
    { // Cypresses.
        MeshBuilder &m = add( PropKindE::CYPRESS, 0.4f );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.2f, 0.18f, 1.2f, 5, rgb( 90, 70, 50 ) );
        m.blob( Vector3{ 0, 4.4f + v, 0 }, Vector3{ 1.0f, 3.8f + v, 1.0f }, rgb( 40, 80 + 6 * v, 42 ), 221 + v, 0.25f );
        m.cylinder( Vector3{ 0, 7.4f + 2.0f * v, 0 }, 0.6f, 0.0f, 2.0f, 6, rgb( 46, 88, 46 ) );
    }
    { // Dutch windmill with four sails.
        MeshBuilder &m = add( PropKindE::DUTCH_MILL, 0.0f );
        m.cylinder( Vector3{ 0, 0, 0 }, 3.2f, 2.0f, 9.0f, 8, rgb( 90, 72, 54 ) );
        m.cylinder( Vector3{ 0, 9.0f, 0 }, 2.4f, 0.0f, 2.6f, 8, rgb( 50, 50, 54 ) );
        const Vector3 hub = { 0, 9.6f, -2.4f };
        for( int i = 0; i < 4; ++i )
        {
            const float   a = Tau * static_cast<float>( i ) / 4.0f + 0.5f;
            const Vector3 d = { std::cos( a ), std::sin( a ), 0 };
            const Vector3 s = { -d.y * 0.75f, d.x * 0.75f, 0 };
            const Vector3 p0 = Vector3Add( hub, Vector3Scale( d, 1.2f ) ), p1 = Vector3Add( hub, Vector3Scale( d, 9.0f ) );
            m.quad( p0, Vector3Add( p0, s ), Vector3Add( p1, s ), p1, rgb( 232, 222, 200 ) );
            m.tube( hub, p1, 0.08f, 4, rgb( 70, 60, 50 ) );
        }
    }
    { // Ruined tower.
        MeshBuilder &m = add( PropKindE::RUIN, 0.0f );
        const Color  stone = rgb( 122, 120, 112 );
        m.cylinder( Vector3{ 0, 0, 0 }, 3.2f, 2.9f, 10.0f, 8, stone );
        for( int i = 0; i < 8; i += 2 )
        {
            const float a = Tau * static_cast<float>( i ) / 8.0f;
            m.box( Vector3{ std::cos( a ) * 2.8f, 10.6f + 0.4f * ( i % 3 ), std::sin( a ) * 2.8f }, Vector3{ 0.6f, 0.6f + 0.3f * ( i % 3 ), 0.6f }, stone );
        }
        m.quad( Vector3{ -0.4f, 5.0f, -3.05f }, Vector3{ 0.4f, 5.0f, -3.05f }, Vector3{ 0.4f, 6.4f, -3.05f }, Vector3{ -0.4f, 6.4f, -3.05f },
                rgb( 20, 20, 22 ) );
        m.blob( Vector3{ 4.0f, 0.3f, 1.0f }, Vector3{ 1.0f, 0.6f, 0.8f }, stone, 231, 0.5f );
    }
    { // Dry stone wall, 5 m along the road.
        MeshBuilder &m = add( PropKindE::STONE_WALL, 0.0f );
        const Color  stone = rgb( 132, 128, 120 );
        m.box( Vector3{ 0, 0.42f, 0 }, Vector3{ 0.35f, 0.42f, 2.5f }, stone );
        for( int i = 0; i < 3; ++i )
            m.box( Vector3{ 0, 0.9f, -1.7f + 1.7f * i }, Vector3{ 0.3f, 0.08f + 0.04f * i, 0.8f }, scaleColor( stone, 0.9f + 0.08f * i ) );
    }
    for( int v = 0; v < 2; ++v )
    { // Red cabins.
        MeshBuilder &m = add( PropKindE::CABIN, 4.0f );
        const Color  wall = v ? rgb( 176, 44, 34 ) : rgb( 150, 40, 30 );
        m.box( Vector3{ 0, 2.0f, 0 }, Vector3{ 3.0f, 2.0f, 4.0f }, wall );
        for( float x : { -3.02f, 3.02f } )
            for( float z : { -4.02f, 4.02f } ) m.box( Vector3{ x, 2.0f, z }, Vector3{ 0.08f, 2.0f, 0.08f }, rgb( 240, 240, 240 ) );
        roof( m, 3.0f, 4.0f, 4.0f, 2.2f, rgb( 50, 50, 56 ) );
        gables( m, 3.0f, 4.0f, 4.0f, 2.2f, wall );
        windows( m, Vector3{ -3.0f, -0.6f, -4.0f }, Vector3{ 1, 0, 0 }, Vector3{ 0, 0, -1 }, 6.0f, 4.0f, 500 + v, 0.6f );
    }
    for( int v = 0; v < 2; ++v )
    { // Bamboo clusters.
        MeshBuilder &m = add( PropKindE::BAMBOO, 0.0f );
        Rng          r( 1400 + v );
        for( int i = 0; i < 9; ++i )
        {
            const Vector3 base = { r.range( -1.0f, 1.0f ), 0, r.range( -1.0f, 1.0f ) };
            const float   h = r.range( 6.0f, 9.5f );
            const Vector3 top = { base.x + r.range( -0.6f, 0.6f ), h, base.z + r.range( -0.6f, 0.6f ) };
            m.tube( base, top, 0.08f, 5, i % 2 ? rgb( 120, 172, 70 ) : rgb( 104, 156, 60 ) );
            for( int k = 0; k < 3; ++k )
            {
                const Vector3 p = Vector3Lerp( base, top, 0.75f + 0.08f * k );
                m.tri( p, Vector3Add( p, Vector3{ r.range( -1.2f, 1.2f ), -0.3f, r.range( -1.2f, 1.2f ) } ),
                       Vector3Add( p, Vector3{ r.range( -0.4f, 0.4f ), 0.2f, r.range( -0.4f, 0.4f ) } ), rgb( 90, 150, 60 ) );
            }
        }
    }
    { // Stilt hut with a thatched roof.
        MeshBuilder &m = add( PropKindE::HUT, 3.0f );
        const Color  wood = rgb( 140, 100, 62 );
        for( float x : { -2.0f, 2.0f } )
            for( float z : { -2.0f, 2.0f } ) m.box( Vector3{ x, 0.8f, z }, Vector3{ 0.12f, 0.8f, 0.12f }, scaleColor( wood, 0.8f ) );
        m.box( Vector3{ 0, 1.7f, 0 }, Vector3{ 2.4f, 0.1f, 2.4f }, wood );
        m.box( Vector3{ 0, 2.8f, 0 }, Vector3{ 2.0f, 1.0f, 2.0f }, wood );
        m.taperedBox( Vector3{ 0, 3.8f, 0 }, Vector2{ 3.0f, 3.0f }, Vector3{ 0, 6.0f, 0 }, Vector2{ 0.2f, 0.2f }, rgb( 196, 166, 96 ) );
    }
    for( int v = 0; v < 2; ++v )
    { // Buttes: slim rock towers.
        MeshBuilder &m = add( PropKindE::BUTTE, 0.0f );
        const float  k = 1.0f + 0.3f * v;
        m.taperedBox( Vector3{ 0, -2, 0 }, Vector2{ 9 * k, 6 }, Vector3{ 0, 10, 0 }, Vector2{ 6 * k, 4.5f }, rgb( 160, 76, 50 ) );
        m.taperedBox( Vector3{ 0, 10, 0 }, Vector2{ 5.5f * k, 4.2f }, Vector3{ 0.5f, 34, 0 }, Vector2{ 5 * k, 3.8f }, rgb( 186, 92, 58 ) );
        m.taperedBox( Vector3{ 0.5f, 34, 0 }, Vector2{ 5 * k, 3.8f }, Vector3{ 0.5f, 37, 0 }, Vector2{ 4.6f * k, 3.5f }, rgb( 140, 66, 44 ) );
    }
    for( int v = 0; v < 3; ++v )
    { // Mailboxes.
        MeshBuilder &m = add( PropKindE::MAILBOX, 0.0f );
        const Color  c[3] = { rgb( 40, 70, 160 ), rgb( 190, 40, 40 ), rgb( 220, 220, 220 ) };
        m.box( Vector3{ 0, 0.5f, 0 }, Vector3{ 0.05f, 0.5f, 0.05f }, rgb( 110, 90, 70 ) );
        m.box( Vector3{ 0, 1.1f, 0 }, Vector3{ 0.16f, 0.13f, 0.27f }, c[v], Mat{ 0.0f, 0.4f } );
        m.box( Vector3{ 0.18f, 1.25f, 0.1f }, Vector3{ 0.01f, 0.12f, 0.06f }, rgb( 230, 40, 30 ) );
    }
    for( int v = 0; v < 3; ++v )
    { // Container stacks.
        MeshBuilder &m = add( PropKindE::CONTAINERS, 6.0f );
        const Color  c[6] = { rgb( 220, 110, 30 ), rgb( 40, 90, 160 ), rgb( 50, 130, 70 ), rgb( 180, 40, 40 ), rgb( 150, 150, 156 ), rgb( 230, 190, 50 ) };
        Rng          r( 1500 + v );
        for( int col = 0; col < 2; ++col )
            for( int lvl = 0; lvl < 2 + v % 2; ++lvl )
            {
                if( lvl > 0 && r.chance( 0.25f ) ) continue;
                const Color k = c[r.rangeInt( 0, 5 )];
                const Vector3 p = { static_cast<float>( col ) * 2.6f - 1.3f, 1.3f + 2.6f * lvl, 0 };
                m.box( p, Vector3{ 1.25f, 1.28f, 6.0f }, k );
                m.box( Vector3{ p.x, p.y, -6.01f }, Vector3{ 1.1f, 1.15f, 0.01f }, scaleColor( k, 0.75f ) );
            }
    }
    for( int v = 0; v < 2; ++v )
    { // Quay cranes, the boom over the water.
        MeshBuilder &m = add( PropKindE::CRANE, 0.0f );
        const Color  c = v ? rgb( 60, 110, 180 ) : rgb( 220, 70, 40 );
        for( float x : { -5.0f, 5.0f } )
            for( float z : { -4.0f, 4.0f } ) m.box( Vector3{ x, 12.0f, z }, Vector3{ 0.4f, 12.0f, 0.4f }, c );
        m.box( Vector3{ 0, 24.0f, 0 }, Vector3{ 5.4f, 0.6f, 4.4f }, c );
        m.box( Vector3{ 10.0f, 26.0f, 0 }, Vector3{ 18.0f, 1.0f, 1.2f }, c );
        m.box( Vector3{ -2.0f, 27.5f, 0 }, Vector3{ 2.0f, 1.5f, 1.8f }, rgb( 230, 230, 230 ) );
        m.box( Vector3{ 27.5f, 26.0f, 0 }, Vector3{ 0.4f, 0.4f, 0.4f }, rgb( 255, 40, 40 ), Mat{ 1.0f, 0.0f } );
    }
    { // Factory chimney with a warning light.
        MeshBuilder &m = add( PropKindE::CHIMNEY, 0.0f );
        m.cylinder( Vector3{ 0, 0, 0 }, 1.7f, 1.1f, 40.0f, 10, rgb( 150, 62, 50 ) );
        m.cylinder( Vector3{ 0, 34.0f, 0 }, 1.26f, 1.2f, 2.0f, 10, rgb( 240, 240, 240 ) );
        m.cylinder( Vector3{ 0, 38.0f, 0 }, 1.16f, 1.12f, 2.0f, 10, rgb( 240, 240, 240 ) );
        m.box( Vector3{ 0, 40.3f, 0 }, Vector3{ 0.4f, 0.3f, 0.4f }, rgb( 255, 30, 30 ), Mat{ 1.0f, 0.0f } );
    }
    for( int v = 0; v < 3; ++v )
    { // Towers with neon strips and a sign.
        MeshBuilder &m = add( PropKindE::NEON_TOWER, 8.0f );
        const float  h = 30.0f + 15.0f * v;
        const Color  neon[3] = { rgb( 255, 40, 190 ), rgb( 40, 220, 255 ), rgb( 180, 80, 255 ) };
        block( m, 7.5f, 7.0f, h, rgb( 30, 30, 46 ), 600 + v, 0.25f );
        for( float x : { -7.55f, 7.55f } ) m.box( Vector3{ x, h * 0.5f, -7.05f }, Vector3{ 0.12f, h * 0.5f, 0.12f }, neon[v], Mat{ 1.0f, 0.0f } );
        m.box( Vector3{ 0, h * 0.62f, -7.3f }, Vector3{ 4.5f, 1.4f, 0.2f }, scaleColor( neon[( v + 1 ) % 3], 0.5f ), Mat{ 0.8f, 0.0f } );
        for( int i = 0; i < 3; ++i )
            m.box( Vector3{ -3.0f + 2.6f * i, h * 0.62f, -7.52f }, Vector3{ 0.9f, 0.9f, 0.04f }, neon[( v + 2 ) % 3], Mat{ 1.0f, 0.0f } );
        m.box( Vector3{ 0, h + 0.15f, 0 }, Vector3{ 7.6f, 0.15f, 7.1f }, neon[v], Mat{ 0.8f, 0.0f } );
    }
    { // Crater rim.
        MeshBuilder &m = add( PropKindE::CRATER, 0.0f );
        const Color  dust = rgb( 112, 112, 110 );
        const int    n = 16;
        for( int i = 0; i < n; ++i )
        {
            const float a0 = Tau * static_cast<float>( i ) / n, a1 = Tau * static_cast<float>( i + 1 ) / n;
            auto p = [&]( float a, float r, float y ) { return Vector3{ std::cos( a ) * r, y, std::sin( a ) * r }; };
            m.quad( p( a0, 5.5f, -0.05f ), p( a1, 5.5f, -0.05f ), p( a1, 4.2f, 0.9f ), p( a0, 4.2f, 0.9f ), dust );
            m.quad( p( a0, 4.2f, 0.9f ), p( a1, 4.2f, 0.9f ), p( a1, 3.0f, -0.4f ), p( a0, 3.0f, -0.4f ), scaleColor( dust, 0.8f ) );
            m.tri( p( a0, 3.0f, -0.4f ), p( a1, 3.0f, -0.4f ), Vector3{ 0, -0.5f, 0 }, scaleColor( dust, 0.7f ) );
        }
    }
    { // Habitat dome.
        MeshBuilder &m = add( PropKindE::DOME, 6.0f );
        const Color  white = rgb( 222, 226, 232 );
        float        prevR = 6.0f, prevY = 0.0f;
        for( int i = 1; i <= 5; ++i )
        {
            const float a = static_cast<float>( i ) / 5.0f * 1.5708f;
            const float r = 6.0f * std::cos( a ), y = 6.0f * std::sin( a );
            m.cylinder( Vector3{ 0, prevY, 0 }, prevR, std::max( r, 0.01f ), y - prevY, 12, white, Mat{ 0.0f, 0.6f } );
            prevR = r;
            prevY = y;
        }
        m.cylinder( Vector3{ 0, 1.2f, 0 }, 6.05f, 6.0f, 0.4f, 12, rgb( 120, 200, 255 ), Mat{ 0.8f, 0.0f } );
        m.box( Vector3{ 0, 1.2f, -6.5f }, Vector3{ 1.2f, 1.2f, 1.2f }, white );
    }
    { // Dish antenna.
        MeshBuilder &m = add( PropKindE::ANTENNA, 0.0f );
        m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 4.5f, 0 }, 0.2f, 6, rgb( 160, 160, 166 ) );
        m.tube( Vector3{ 0, 4.6f, 0 }, Vector3{ 0, 5.3f, -0.7f }, 2.3f, 14, rgb( 230, 230, 236 ), Mat{ 0.0f, 0.5f }, true );
        m.tube( Vector3{ 0, 5.0f, -0.4f }, Vector3{ 0, 7.0f, -2.4f }, 0.06f, 4, rgb( 120, 120, 126 ) );
        m.box( Vector3{ 0, 7.05f, -2.45f }, Vector3{ 0.15f, 0.15f, 0.15f }, rgb( 255, 60, 60 ), Mat{ 1.0f, 0.0f } );
    }
    for( int v = 0; v < 3; ++v )
    { // Glowing crystal clusters.
        MeshBuilder &m = add( PropKindE::CRYSTAL, 0.8f );
        const Color  c[3] = { rgb( 90, 230, 255 ), rgb( 230, 90, 255 ), rgb( 140, 120, 255 ) };
        Rng          r( 1600 + v );
        for( int i = 0; i < 5 + v; ++i )
            prism( m, Vector3{ r.range( -1.0f, 1.0f ), -0.3f, r.range( -1.0f, 1.0f ) },
                   Vector3{ r.range( -0.5f, 0.5f ), 1.0f, r.range( -0.5f, 0.5f ) }, r.range( 1.5f, 5.5f ), r.range( 0.3f, 0.8f ), 6,
                   r.range( 0.6f, 1.4f ), c[( v + i ) % 3], Mat{ 0.55f, 0.8f } );
    }
    for( int v = 0; v < 3; ++v )
    { // Giant mushrooms: spotted red, glowing teal, violet.
        MeshBuilder &m = add( PropKindE::MUSHROOM, 0.6f );
        const float  h = 4.0f + 1.0f * v;
        m.cylinder( Vector3{ 0, 0, 0 }, 0.6f, 0.4f, h, 8, rgb( 230, 222, 200 ) );
        const Color cap[3] = { rgb( 210, 40, 40 ), rgb( 40, 200, 190 ), rgb( 150, 70, 200 ) };
        const Mat   capMat = v ? Mat{ 0.35f, 0.2f } : Mat{ 0.0f, 0.3f };
        m.blob( Vector3{ 0, h + 0.4f, 0 }, Vector3{ 3.0f, 1.1f, 3.0f }, cap[v], 241 + v, 0.15f, capMat );
        Rng r( 1700 + v );
        for( int i = 0; i < 9; ++i )
        {
            const float a = r.range( 0.0f, Tau ), d = r.range( 0.4f, 2.5f );
            m.box( Vector3{ std::cos( a ) * d, h + 0.4f + 1.15f * std::sqrt( std::max( 0.0f, 1.0f - d * d / 9.0f ) ), std::sin( a ) * d },
                   Vector3{ 0.22f, 0.06f, 0.22f }, v ? rgb( 220, 255, 240 ) : rgb( 250, 250, 240 ), v ? Mat{ 1.0f, 0.0f } : Mat{ 0.2f, 0.0f } );
        }
    }
    for( int v = 0; v < 2; ++v )
    { // Small glowing mushrooms.
        MeshBuilder &m = add( PropKindE::GLOW_SHROOM, 0.0f );
        Rng          r( 1800 + v );
        const Color  c = v ? rgb( 120, 255, 200 ) : rgb( 120, 200, 255 );
        for( int i = 0; i < 6; ++i )
        {
            const Vector3 p = { r.range( -0.6f, 0.6f ), 0, r.range( -0.6f, 0.6f ) };
            const float   h = r.range( 0.2f, 0.6f );
            m.cylinder( p, 0.05f, 0.04f, h, 4, rgb( 200, 220, 210 ) );
            m.blob( Vector3{ p.x, p.y + h, p.z }, Vector3{ 0.22f, 0.1f, 0.22f }, c, 251 + i, 0.2f, Mat{ 1.0f, 0.0f } );
        }
    }
    for( int v = 0; v < 3; ++v )
    { // Icebergs, partly under water.
        MeshBuilder &m = add( PropKindE::ICEBERG, 0.0f );
        m.blob( Vector3{ 0, 1.0f, 0 }, Vector3{ 7.0f, 4.0f + 1.5f * v, 6.0f }, rgb( 226, 238, 252 ), 261 + v, 0.6f, Mat{ 0.05f, 0.5f } );
        m.blob( Vector3{ 3.0f, 2.5f, 1.5f }, Vector3{ 3.0f, 3.0f + v, 2.6f }, rgb( 200, 226, 248 ), 271 + v, 0.6f, Mat{ 0.05f, 0.5f } );
    }
    for( int v = 0; v < 2; ++v )
    { // Swamp cypresses with hanging moss.
        MeshBuilder &m = add( PropKindE::MOSS_TREE, 0.6f );
        const Color  bark = rgb( 76, 70, 62 );
        m.cylinder( Vector3{ 0, 0, 0 }, 1.0f, 0.35f, 2.2f, 7, bark );
        m.tube( Vector3{ 0, 2.0f, 0 }, Vector3{ 0.3f * v, 9.0f, 0 }, 0.33f, 6, bark );
        Rng r( 1900 + v );
        for( int i = 0; i < 4; ++i )
        {
            const float   a = r.range( 0.0f, Tau );
            const float   y = r.range( 5.0f, 8.5f );
            const Vector3 end = { std::cos( a ) * r.range( 2.0f, 3.5f ), y + r.range( 0.3f, 1.2f ), std::sin( a ) * r.range( 2.0f, 3.5f ) };
            m.tube( Vector3{ 0, y, 0 }, end, 0.12f, 4, bark );
            m.blob( end, Vector3{ 1.6f, 0.7f, 1.6f }, rgb( 62, 82, 50 ), 281 + i + v * 7, 0.4f );
            for( int k = 0; k < 3; ++k )
            {
                const Vector3 top = Vector3Lerp( Vector3{ 0, y, 0 }, end, 0.4f + 0.25f * k );
                m.quad( Vector3Add( top, Vector3{ -0.2f, 0, 0 } ), Vector3Add( top, Vector3{ 0.2f, 0, 0 } ),
                        Vector3Add( top, Vector3{ 0.05f, -r.range( 1.8f, 3.0f ), 0 } ), Vector3Add( top, Vector3{ -0.05f, -r.range( 1.8f, 3.0f ), 0 } ),
                        rgb( 132, 140, 112 ) );
            }
        }
    }
    { // Fireflies.
        MeshBuilder &m = add( PropKindE::FIREFLIES, 0.0f );
        Rng          r( 2000 );
        for( int i = 0; i < 16; ++i )
            m.box( Vector3{ r.range( -2.5f, 2.5f ), r.range( 0.4f, 2.6f ), r.range( -2.5f, 2.5f ) }, Vector3{ 0.04f, 0.04f, 0.04f },
                   rgb( 220, 255, 120 ), Mat{ 1.0f, 0.0f } );
    }
    for( int v = 0; v < 2; ++v )
    { // Barns.
        MeshBuilder &m = add( PropKindE::BARN, 7.0f );
        const Color  red = v ? rgb( 160, 40, 34 ) : rgb( 176, 52, 40 );
        m.box( Vector3{ 0, 3.5f, 0 }, Vector3{ 6.0f, 3.5f, 8.0f }, red );
        const Vector3 a0 = { -6.4f, 7.0f, -8.3f }, a1 = { -3.6f, 9.8f, -8.3f }, a2 = { 0, 11.0f, -8.3f };
        const Vector3 b0 = { -6.4f, 7.0f, 8.3f }, b1 = { -3.6f, 9.8f, 8.3f }, b2 = { 0, 11.0f, 8.3f };
        const Color   roofCol = rgb( 70, 70, 76 );
        m.quad( a0, a1, b1, b0, roofCol );
        m.quad( a1, a2, b2, b1, roofCol );
        m.quad( Vector3{ 6.4f, 7.0f, -8.3f }, Vector3{ 6.4f, 7.0f, 8.3f }, Vector3{ 3.6f, 9.8f, 8.3f }, Vector3{ 3.6f, 9.8f, -8.3f }, roofCol );
        m.quad( Vector3{ 3.6f, 9.8f, -8.3f }, Vector3{ 3.6f, 9.8f, 8.3f }, b2, a2, roofCol );
        for( float z : { -8.0f, 8.0f } )
        {
            m.quad( Vector3{ -6, 7, z }, Vector3{ 6, 7, z }, Vector3{ 3.4f, 9.6f, z }, Vector3{ -3.4f, 9.6f, z }, red );
            m.tri( Vector3{ -3.4f, 9.6f, z }, Vector3{ 3.4f, 9.6f, z }, Vector3{ 0, 10.8f, z }, red );
        }
        m.box( Vector3{ 0, 2.2f, -8.05f }, Vector3{ 2.2f, 2.2f, 0.05f }, rgb( 236, 236, 230 ) );
        m.box( Vector3{ 0, 2.2f, -8.1f }, Vector3{ 2.0f, 2.0f, 0.03f }, scaleColor( red, 0.8f ) );
    }
    { // Grain silo.
        MeshBuilder &m = add( PropKindE::SILO, 2.6f );
        m.cylinder( Vector3{ 0, 0, 0 }, 2.5f, 2.5f, 14.0f, 12, rgb( 194, 198, 204 ), Mat{ 0.0f, 0.55f } );
        m.cylinder( Vector3{ 0, 14.0f, 0 }, 2.55f, 0.3f, 2.2f, 12, rgb( 170, 174, 180 ), Mat{ 0.0f, 0.55f } );
        for( int i = 1; i < 5; ++i ) m.cylinder( Vector3{ 0, 2.8f * i, 0 }, 2.53f, 2.53f, 0.12f, 12, rgb( 150, 154, 160 ) );
    }
    { // Lighthouse with a lit lantern room.
        MeshBuilder &m = add( PropKindE::LIGHTHOUSE, 2.0f );
        for( int i = 0; i < 6; ++i )
        {
            const float r0 = 2.2f - 0.13f * i, r1 = 2.2f - 0.13f * ( i + 1 );
            m.cylinder( Vector3{ 0, 3.0f * i, 0 }, r0, r1, 3.0f, 10, i % 2 ? rgb( 200, 40, 36 ) : rgb( 240, 240, 236 ) );
        }
        m.cylinder( Vector3{ 0, 18.0f, 0 }, 2.0f, 2.0f, 0.3f, 10, rgb( 50, 50, 54 ) );
        m.cylinder( Vector3{ 0, 18.3f, 0 }, 1.0f, 1.0f, 1.8f, 10, rgb( 255, 240, 180 ), Mat{ 1.0f, 0.0f } );
        m.cylinder( Vector3{ 0, 20.1f, 0 }, 1.3f, 0.0f, 1.4f, 10, rgb( 200, 40, 36 ) );
    }
    for( int v = 0; v < 2; ++v )
    { // Jack-o'-lanterns, the face towards -z.
        MeshBuilder &m = add( PropKindE::PUMPKIN, 0.0f );
        m.blob( Vector3{ 0, 0.48f, 0 }, Vector3{ 0.72f, 0.5f, 0.68f }, rgb( 240, 120, 20 ), 291 + v, 0.15f, Mat{ 0.1f, 0.2f } );
        m.cylinder( Vector3{ 0, 0.95f, 0 }, 0.07f, 0.05f, 0.22f, 5, rgb( 70, 100, 40 ) );
        const Color glow = rgb( 255, 210, 80 );
        const float z = -0.69f;
        m.tri( Vector3{ -0.36f, 0.62f, z }, Vector3{ -0.12f, 0.62f, z }, Vector3{ -0.24f, 0.8f, z }, glow, Mat{ 1.0f, 0.0f } );
        m.tri( Vector3{ 0.12f, 0.62f, z }, Vector3{ 0.36f, 0.62f, z }, Vector3{ 0.24f, 0.8f, z }, glow, Mat{ 1.0f, 0.0f } );
        m.quad( Vector3{ -0.36f, 0.32f, z }, Vector3{ 0.36f, 0.32f, z }, Vector3{ 0.3f, 0.46f, z }, Vector3{ -0.3f, 0.46f, z }, glow, Mat{ 1.0f, 0.0f } );
    }
    for( int v = 0; v < 3; ++v )
    { // Gravestones.
        MeshBuilder &m = add( PropKindE::GRAVESTONE, 0.3f );
        const Color  stone = rgb( 120 + 10 * v, 120 + 10 * v, 118 + 10 * v );
        if( v == 2 )
        {
            m.box( Vector3{ 0, 0.7f, 0 }, Vector3{ 0.09f, 0.7f, 0.09f }, stone );
            m.box( Vector3{ 0, 1.0f, 0 }, Vector3{ 0.4f, 0.09f, 0.09f }, stone );
        }
        else
        {
            m.box( Vector3{ 0, 0.5f, 0 }, Vector3{ 0.38f, 0.5f, 0.1f }, stone );
            m.tube( Vector3{ 0, 1.0f, -0.1f }, Vector3{ 0, 1.0f, 0.1f }, 0.38f, 8, stone, Mat{}, true );
        }
    }
    for( int v = 0; v < 2; ++v )
    { // Pines with festive lights.
        MeshBuilder &m = add( PropKindE::LIGHT_TREE, 0.6f );
        m.cylinder( Vector3{ 0, 0, 0 }, 0.24f, 0.2f, 1.4f, 6, rgb( 80, 60, 46 ) );
        const float base[3] = { 1.0f, 2.5f, 3.9f }, rad[3] = { 2.0f, 1.55f, 1.0f }, hgt[3] = { 3.0f, 2.7f, 2.3f };
        for( int i = 0; i < 3; ++i )
        {
            m.cylinder( Vector3{ 0, base[i], 0 }, rad[i], 0.0f, hgt[i], 7, rgb( 34, 80, 56 ), {}, 0.4f * i );
            m.cylinder( Vector3{ 0, base[i] + hgt[i] * 0.45f, 0 }, rad[i] * 0.6f, 0.0f, hgt[i] * 0.55f + 0.05f, 7, rgb( 236, 242, 250 ), {}, 0.4f * i );
        }
        Rng         r( 2100 + v );
        const Color bulbs[4] = { rgb( 255, 60, 60 ), rgb( 255, 210, 60 ), rgb( 80, 160, 255 ), rgb( 90, 255, 120 ) };
        for( int i = 0; i < 22; ++i )
        {
            const float t = r.range( 0.05f, 0.95f );
            const float y = 1.0f + t * 5.3f;
            const float rr = 2.05f * ( 1.0f - t ) + 0.1f;
            const float a = r.range( 0.0f, Tau );
            m.box( Vector3{ std::cos( a ) * rr, y, std::sin( a ) * rr }, Vector3{ 0.07f, 0.07f, 0.07f }, bulbs[i % 4], Mat{ 1.0f, 0.0f } );
        }
        m.box( Vector3{ 0, 6.35f, 0 }, Vector3{ 0.18f, 0.18f, 0.18f }, rgb( 255, 230, 100 ), Mat{ 1.0f, 0.0f } );
    }
    for( int v = 0; v < 2; ++v )
    { // Houses with snowy roofs and warm windows.
        MeshBuilder &m = add( PropKindE::SNOW_HOUSE, 5.0f );
        const Color  wall = v ? rgb( 150, 90, 70 ) : rgb( 210, 196, 170 );
        m.box( Vector3{ 0, 2.2f, 0 }, Vector3{ 4.0f, 2.2f, 5.0f }, wall );
        roof( m, 4.0f, 5.0f, 4.4f, 2.6f, rgb( 240, 244, 252 ) );
        gables( m, 4.0f, 5.0f, 4.4f, 2.6f, wall );
        m.box( Vector3{ 2.0f, 6.0f, 1.5f }, Vector3{ 0.4f, 1.0f, 0.4f }, rgb( 120, 70, 60 ) );
        windows( m, Vector3{ -4.0f, -0.4f, -5.0f }, Vector3{ 1, 0, 0 }, Vector3{ 0, 0, -1 }, 8.0f, 4.4f, 700 + v, 0.85f );
        windows( m, Vector3{ 4.0f, -0.4f, -5.0f }, Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, 10.0f, 4.4f, 710 + v, 0.85f );
        windows( m, Vector3{ -4.0f, -0.4f, 5.0f }, Vector3{ 0, 0, -1 }, Vector3{ -1, 0, 0 }, 10.0f, 4.4f, 720 + v, 0.85f );
    }

    // The wind moves plants: more at the top, palms and bamboo most.
    const std::pair<PropKindE, float> sway[] = { { PropKindE::PINE, 0.5f },        { PropKindE::SNOW_PINE, 0.4f },
                                                 { PropKindE::OAK, 0.6f },         { PropKindE::BUSH, 0.3f },
                                                 { PropKindE::PALM, 1.0f },        { PropKindE::REEDS, 0.6f },
                                                 { PropKindE::AUTUMN_TREE, 0.6f }, { PropKindE::BIRCH, 0.8f },
                                                 { PropKindE::CHERRY, 0.5f },      { PropKindE::JUNGLE_TREE, 0.4f },
                                                 { PropKindE::FERN, 0.4f },        { PropKindE::ACACIA, 0.35f },
                                                 { PropKindE::GRASS_TUFT, 0.5f },  { PropKindE::CYPRESS, 0.45f },
                                                 { PropKindE::BAMBOO, 0.9f },      { PropKindE::MOSS_TREE, 0.35f },
                                                 { PropKindE::LIGHT_TREE, 0.25f }, { PropKindE::DEAD_TREE, 0.2f },
                                                 { PropKindE::FIREFLIES, 0.8f } };
    for( const auto &[kind, amount] : sway )
        for( MeshBuilder &variant : m_props[static_cast<size_t>( kind )] ) variant.applySway( amount );
    buildWorldProps();

    // Road furniture.
    m_post.box( Vector3{ 0, 0.55f, 0 }, Vector3{ 0.06f, 0.55f, 0.06f }, rgb( 236, 236, 236 ) );
    m_post.box( Vector3{ 0, 0.95f, 0 }, Vector3{ 0.07f, 0.08f, 0.07f }, rgb( 255, 150, 40 ), Mat{ 0.7f, 0.0f } );
    // Chevron boards in the country's colours: red and white, black and white, yellow and black.
    const Color chevronBoard[3] = { rgb( 200, 26, 26 ), rgb( 24, 24, 26 ), rgb( 245, 196, 20 ) };
    const Color chevronArrow[3] = { rgb( 250, 250, 250 ), rgb( 250, 250, 250 ), rgb( 20, 20, 20 ) };
    for( int style = 0; style < 3; ++style )
        for( int dir = 0; dir < 2; ++dir )
        {
            MeshBuilder &m = dir ? m_chevronLeft[static_cast<size_t>( style )] : m_chevronRight[static_cast<size_t>( style )];
            m.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 1.1f, 0 }, 0.07f, 5, rgb( 120, 120, 128 ) );
            m.box( Vector3{ 0, 1.6f, 0 }, Vector3{ 0.95f, 0.55f, 0.05f }, chevronBoard[style], Mat{ 0.12f, 0.0f } );
            // The arrow points to the driver's right (-x) or left (+x), drawn on the face towards the driver (-z).
            const float s = dir ? 1.0f : -1.0f;
            for( float off : { -0.35f, 0.35f } )
            {
                boardLine( m, Vector2{ -s * 0.2f + off, 1.95f }, Vector2{ s * 0.2f + off, 1.6f }, -0.07f, 0.17f, chevronArrow[style], Mat{ 0.45f, 0.0f } );
                boardLine( m, Vector2{ s * 0.2f + off, 1.6f }, Vector2{ -s * 0.2f + off, 1.25f }, -0.07f, 0.17f, chevronArrow[style], Mat{ 0.45f, 0.0f } );
            }
        }
    m_lamp.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 8.6f, 0 }, 0.14f, 6, rgb( 110, 112, 122 ) );
    m_lamp.tube( Vector3{ 0, 8.4f, 0 }, Vector3{ 0, 8.7f, 2.6f }, 0.09f, 5, rgb( 110, 112, 122 ) );
    m_lamp.box( Vector3{ 0, 8.65f, 2.9f }, Vector3{ 0.28f, 0.1f, 0.62f }, rgb( 70, 72, 80 ) );
    m_lamp.quad( Vector3{ -0.24f, 8.54f, 2.35f }, Vector3{ 0.24f, 8.54f, 2.35f }, Vector3{ 0.24f, 8.54f, 3.45f },
                 Vector3{ -0.24f, 8.54f, 3.45f }, rgb( 255, 226, 170 ), Glow );
    for( size_t style : { size_t{ 0 }, size_t{ 1 }, size_t{ 3 } } )
    { // A crest triangle: red on white, red on yellow, black on yellow.
        MeshBuilder &crest = m_jumpSign[style];
        const Color  border = style == 3 ? rgb( 20, 20, 20 ) : rgb( 210, 30, 30 ), face = style == 0 ? rgb( 250, 250, 250 ) : rgb( 245, 196, 20 );
        crest.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 1.4f, 0 }, 0.07f, 5, rgb( 120, 120, 128 ) );
        crest.tri( Vector3{ -0.75f, 1.4f, -0.04f }, Vector3{ 0.75f, 1.4f, -0.04f }, Vector3{ 0, 2.7f, -0.04f }, border, Mat{ 0.15f, 0.0f } );
        crest.tri( Vector3{ -0.5f, 1.55f, -0.06f }, Vector3{ 0.5f, 1.55f, -0.06f }, Vector3{ 0, 2.42f, -0.06f }, face, Mat{ 0.3f, 0.0f } );
        crest.tri( Vector3{ -0.32f, 1.68f, -0.07f }, Vector3{ 0.32f, 1.68f, -0.07f }, Vector3{ 0, 1.98f, -0.07f }, rgb( 20, 20, 20 ) );
    }
    MeshBuilder &bump = m_jumpSign[2];
    bump.tube( Vector3{ 0, 0, 0 }, Vector3{ 0, 1.5f, 0 }, 0.07f, 5, rgb( 120, 120, 128 ) );
    bump.quad( Vector3{ 0.75f, 2.25f, -0.04f }, Vector3{ 0, 1.5f, -0.04f }, Vector3{ -0.75f, 2.25f, -0.04f }, Vector3{ 0, 3.0f, -0.04f }, rgb( 20, 20, 20 ), Mat{ 0.15f, 0.0f } );
    bump.quad( Vector3{ 0.66f, 2.25f, -0.05f }, Vector3{ 0, 1.59f, -0.05f }, Vector3{ -0.66f, 2.25f, -0.05f }, Vector3{ 0, 2.91f, -0.05f }, rgb( 245, 196, 20 ), Mat{ 0.3f, 0.0f } );
    bump.tri( Vector3{ 0.35f, 2.05f, -0.06f }, Vector3{ -0.35f, 2.05f, -0.06f }, Vector3{ 0, 2.4f, -0.06f }, rgb( 20, 20, 20 ) );
}

namespace
{

/**
 * @brief A colour from "RRGGBB".
 * @param[in] hex  the digits
 * @return the colour
 */
Color hexColor( const std::string &hex )
{
    const unsigned v = static_cast<unsigned>( std::strtoul( hex.c_str(), nullptr, 16 ) );
    return rgb( static_cast<int>( v >> 16 ), static_cast<int>( ( v >> 8 ) & 255 ), static_cast<int>( v & 255 ) );
}

/**
 * @brief Builds a flag from its description (Countries.cpp) on a pole; the cloth faces -z and +z.
 * @param[out] m     where the triangles go
 * @param[in]  spec  the flag
 */
void buildFlag( MeshBuilder &m, const char *spec )
{
    const float W = 1.5f, H = 1.0f, X0 = 0.08f, Y0 = 5.4f;   // the cloth, its hoist at the pole
    std::vector<std::string> t;
    for( const char *p = spec; *p; )
    {
        while( *p == ' ' ) ++p;
        const char *e = p;
        while( *e && *e != ' ' ) ++e;
        if( e > p ) t.emplace_back( p, static_cast<size_t>( e - p ) );
        p = e;
    }
    int layer = 0;
    // A shape in cloth coordinates (u across from the hoist, v up), on both faces; models face +x to the left, so the
    // hoist (next to the pole) is at +x and the fly towards -x.
    auto vtx = [&]( float u, float v, float side ) { return Vector3{ X0 - u * W, Y0 + v * H, side * ( 0.012f + 0.002f * static_cast<float>( layer ) ) }; };
    auto poly = [&]( const std::vector<Vector2> &pts, Color c )
    {
        for( size_t i = 1; i + 1 < pts.size(); ++i )
        {
            m.tri( vtx( pts[0].x, pts[0].y, -1.0f ), vtx( pts[i].x, pts[i].y, -1.0f ), vtx( pts[i + 1].x, pts[i + 1].y, -1.0f ), c, Mat{ 0.15f, 0.0f } );
            m.tri( vtx( pts[0].x, pts[0].y, 1.0f ), vtx( pts[i + 1].x, pts[i + 1].y, 1.0f ), vtx( pts[i].x, pts[i].y, 1.0f ), c, Mat{ 0.15f, 0.0f } );
        }
        ++layer;
    };
    auto rect = [&]( float u0, float v0, float u1, float v1, Color c ) { poly( { { u0, v0 }, { u1, v0 }, { u1, v1 }, { u0, v1 } }, c ); };
    auto disc = [&]( float u, float v, float r, Color c, float hole = 0.0f )
    {
        std::vector<Vector2> pts;
        const int            n = 18;
        if( hole <= 0.0f )
        {
            for( int i = 0; i < n; ++i )
            {
                const float a = Tau * static_cast<float>( i ) / n;
                pts.push_back( Vector2{ u + std::cos( a ) * r * H / W, v + std::sin( a ) * r } );
            }
            poly( pts, c );
            return;
        }
        for( int i = 0; i < n; ++i )
        {
            const float a0 = Tau * static_cast<float>( i ) / n, a1 = Tau * static_cast<float>( i + 1 ) / n;
            poly( { { u + std::cos( a0 ) * r * H / W, v + std::sin( a0 ) * r }, { u + std::cos( a1 ) * r * H / W, v + std::sin( a1 ) * r },
                    { u + std::cos( a1 ) * hole * H / W, v + std::sin( a1 ) * hole }, { u + std::cos( a0 ) * hole * H / W, v + std::sin( a0 ) * hole } },
                  c );
            --layer;
        }
        ++layer;
    };
    auto band = [&]( Vector2 a, Vector2 b, float thick, Color c )
    {
        // A straight band of the given thickness (in heights) from a to b, in metres so it keeps its width.
        const Vector2 d = Vector2Normalize( Vector2{ ( b.x - a.x ) * W, ( b.y - a.y ) * H } );
        const Vector2 n = { -d.y * thick * 0.5f / W, d.x * thick * 0.5f / H };
        poly( { { a.x + n.x, a.y + n.y }, { b.x + n.x, b.y + n.y }, { b.x - n.x, b.y - n.y }, { a.x - n.x, a.y - n.y } }, c );
    };
    auto unionJack = [&]( float u0, float v0, float u1, float v1 )
    {
        rect( u0, v0, u1, v1, rgb( 1, 33, 105 ) );
        const float k = ( v1 - v0 );
        band( { u0, v0 }, { u1, v1 }, 0.2f * k, rgb( 255, 255, 255 ) );
        band( { u0, v1 }, { u1, v0 }, 0.2f * k, rgb( 255, 255, 255 ) );
        band( { u0, v0 }, { u1, v1 }, 0.07f * k, rgb( 200, 16, 46 ) );
        band( { u0, v1 }, { u1, v0 }, 0.07f * k, rgb( 200, 16, 46 ) );
        rect( u0, ( v0 + v1 ) * 0.5f - 0.17f * k, u1, ( v0 + v1 ) * 0.5f + 0.17f * k, rgb( 255, 255, 255 ) );
        rect( ( u0 + u1 ) * 0.5f - 0.17f * k * H / W, v0, ( u0 + u1 ) * 0.5f + 0.17f * k * H / W, v1, rgb( 255, 255, 255 ) );
        rect( u0, ( v0 + v1 ) * 0.5f - 0.1f * k, u1, ( v0 + v1 ) * 0.5f + 0.1f * k, rgb( 200, 16, 46 ) );
        rect( ( u0 + u1 ) * 0.5f - 0.1f * k * H / W, v0, ( u0 + u1 ) * 0.5f + 0.1f * k * H / W, v1, rgb( 200, 16, 46 ) );
    };
    float cantonW = 0.45f, cantonH = 0.5f;
    size_t i = 0;
    auto num = [&]() { return i < t.size() ? std::stof( t[i++] ) : 0.0f; };
    auto col = [&]() { return i < t.size() ? hexColor( t[i++] ) : rgb( 255, 255, 255 ); };
    // Stripes.
    if( !t.empty() )
    {
        const bool horizontal = t[0] == "H";
        i = 1;
        std::vector<Color> stripes;
        while( i < t.size() && t[i][0] != '+' ) stripes.push_back( col() );
        const float n = static_cast<float>( std::max<size_t>( 1, stripes.size() ) );
        for( size_t k = 0; k < stripes.size(); ++k )
        {
            const float a = static_cast<float>( k ) / n, b = static_cast<float>( k + 1 ) / n;
            if( horizontal ) rect( 0.0f, 1.0f - b, 1.0f, 1.0f - a, stripes[k] );
            else rect( a, 0.0f, b, 1.0f, stripes[k] );
        }
        layer = 1;
    }
    // Decorations.
    while( i < t.size() )
    {
        const std::string op = t[i++];
        if( op == "+D" ) { const Color c = col(); disc( 0.5f, 0.5f, num(), c ); }
        else if( op == "+DH" ) { const Color c = col(); disc( 0.42f, 0.5f, num(), c ); }
        else if( op == "+O" ) { const Color c = col(); const float r = num(); disc( 0.5f, 0.5f, r, c, r * 0.72f ); }
        else if( op == "+T" ) { const Color c = col(); const float d = num(); poly( { { 0.0f, 0.0f }, { d, 0.5f }, { 0.0f, 1.0f } }, c ); }
        else if( op == "+K" ) { const Color c = col(); cantonW = num(); cantonH = num(); rect( 0.0f, 1.0f - cantonH, cantonW, 1.0f, c ); }
        else if( op == "+KD" ) { const Color c = col(); disc( cantonW * 0.5f, 1.0f - cantonH * 0.5f, num(), c ); }
        else if( op == "+KX" )
        {
            const Color c = col();
            const float th = num();
            rect( 0.0f, 1.0f - cantonH * 0.5f - th * 0.5f, cantonW, 1.0f - cantonH * 0.5f + th * 0.5f, c );
            rect( cantonW * 0.5f - th * 0.5f * H / W, 1.0f - cantonH, cantonW * 0.5f + th * 0.5f * H / W, 1.0f, c );
        }
        else if( op == "+N" || op == "+X" )
        {
            const Color c = col();
            const float th = num(), u = op == "+N" ? 0.36f : 0.5f;
            rect( 0.0f, 0.5f - th * 0.5f, 1.0f, 0.5f + th * 0.5f, c );
            rect( u - th * 0.5f * H / W, 0.0f, u + th * 0.5f * H / W, 1.0f, c );
        }
        else if( op == "+S" )
        {
            const Color c = col();
            const float th = num();
            band( { 0.0f, 0.0f }, { 1.0f, 1.0f }, th, c );
            band( { 0.0f, 1.0f }, { 1.0f, 0.0f }, th, c );
        }
        else if( op == "+HB" ) { const Color c = col(); rect( 0.0f, 0.0f, num(), 1.0f, c ); }
        else if( op == "+VB" ) { const Color c = col(); const float w = num(); rect( 0.5f - w * 0.5f, 0.0f, 0.5f + w * 0.5f, 1.0f, c ); }
        else if( op == "+DB" ) { const Color c = col(); band( { 0.0f, 0.0f }, { 1.0f, 1.0f }, num(), c ); }
        else if( op == "+DIA" ) { const Color c = col(); poly( { { 0.08f, 0.5f }, { 0.5f, 0.08f }, { 0.92f, 0.5f }, { 0.5f, 0.92f } }, c ); }
        else if( op == "+Q" )
        {
            const Color a = col(), b = col(), c = col(), d = col();
            rect( 0.0f, 0.5f, 0.5f, 1.0f, a );
            --layer;
            rect( 0.5f, 0.5f, 1.0f, 1.0f, b );
            --layer;
            rect( 0.0f, 0.0f, 0.5f, 0.5f, c );
            --layer;
            rect( 0.5f, 0.0f, 1.0f, 0.5f, d );
        }
        else if( op == "+R" ) { const Color c = col(); const float w = num(), h = num(); rect( 0.5f - w * 0.5f, 0.5f - h * 0.5f, 0.5f + w * 0.5f, 0.5f + h * 0.5f, c ); }
        else if( op == "+UJ" ) { cantonW = 0.5f; cantonH = 0.5f; unionJack( 0.0f, 0.5f, 0.5f, 1.0f ); }
    }
    // The pole and its finial.
    m.cylinder( Vector3{ X0 + 0.05f, 0, 0 }, 0.06f, 0.045f, Y0 + H + 0.3f, 6, rgb( 225, 228, 232 ), Mat{ 0.0f, 0.5f } );
    m.blob( Vector3{ X0 + 0.05f, Y0 + H + 0.35f, 0 }, Vector3{ 0.08f, 0.08f, 0.08f }, rgb( 214, 170, 60 ), 31, 0.0f );
}

/**
 * @brief Four tyres on two axles.
 * @param[out] m      where the triangles go
 * @param[in]  half   half the track width
 * @param[in]  r      tyre radius
 * @param[in]  axles  the axles' z
 * @param[in]  hub    rim colour
 */
void tyres( MeshBuilder &m, float half, float r, std::initializer_list<float> axles, Color hub = rgb( 170, 172, 178 ) )
{
    for( float z : axles )
        for( float side : { -1.0f, 1.0f } )
        {
            m.tube( Vector3{ side * ( half - 0.26f ), r, z }, Vector3{ side * half, r, z }, r, 10, rgb( 20, 20, 22 ), Mat{}, true );
            m.tube( Vector3{ side * ( half - 0.02f ), r, z }, Vector3{ side * ( half + 0.01f ), r, z }, r * 0.55f, 8, hub, Mat{ 0.0f, 0.6f }, true );
        }
}

/**
 * @brief Head and tail lights on a box-shaped vehicle.
 * @param[out] m      where the triangles go
 * @param[in]  half   half its width
 * @param[in]  y      their height
 * @param[in]  front  the front face's z
 * @param[in]  back   the back face's z
 */
void lights( MeshBuilder &m, float half, float y, float front, float back )
{
    for( float side : { -1.0f, 1.0f } )
    {
        const float x0 = side * ( half - 0.1f ), x1 = side * ( half - 0.45f );
        m.quad( Vector3{ x1, y, front + 0.01f }, Vector3{ x0, y, front + 0.01f }, Vector3{ x0, y + 0.22f, front + 0.01f }, Vector3{ x1, y + 0.22f, front + 0.01f },
                rgb( 255, 246, 224 ), Mat{ 0.7f, 0.3f } );
        m.quad( Vector3{ x0, y, back - 0.01f }, Vector3{ x1, y, back - 0.01f }, Vector3{ x1, y + 0.22f, back - 0.01f }, Vector3{ x0, y + 0.22f, back - 0.01f },
                rgb( 255, 24, 24 ), Mat{ 0.5f, 0.3f } );
    }
}

/**
 * @brief A row of windows along both flanks of a box body.
 * @param[out] m     where the triangles go
 * @param[in]  half  half the width
 * @param[in]  y0    sill
 * @param[in]  y1    top
 * @param[in]  z0    rear end
 * @param[in]  z1    front end
 * @param[in]  n     how many per side
 */
void sideWindows( MeshBuilder &m, float half, float y0, float y1, float z0, float z1, int n )
{
    const float step = ( z1 - z0 ) / static_cast<float>( n );
    for( int i = 0; i < n; ++i )
    {
        const float a = z0 + step * ( static_cast<float>( i ) + 0.1f ), b = z0 + step * ( static_cast<float>( i ) + 0.9f );
        m.quad( Vector3{ half + 0.01f, y0, a }, Vector3{ half + 0.01f, y0, b }, Vector3{ half + 0.01f, y1, b }, Vector3{ half + 0.01f, y1, a }, rgb( 22, 30, 44 ), Glass );
        m.quad( Vector3{ -half - 0.01f, y0, b }, Vector3{ -half - 0.01f, y0, a }, Vector3{ -half - 0.01f, y1, a }, Vector3{ -half - 0.01f, y1, b }, rgb( 22, 30, 44 ), Glass );
    }
}

/**
 * @brief The fleet bit of a region.
 * @param[in] f  the fleet
 * @return its bit
 */
constexpr uint32_t bit( FleetE f )
{
    return 1u << static_cast<unsigned>( f );
}

} // namespace

void buildPlaceSign( MeshBuilder &out, std::string_view text, int style, float cap, float lift, bool posts )
{
    static const Color boards[5] = { rgb( 244, 244, 240 ), rgb( 255, 204, 0 ), rgb( 244, 244, 240 ), rgb( 0, 72, 160 ), rgb( 0, 112, 66 ) };
    static const Color inks[5] = { rgb( 20, 20, 22 ), rgb( 20, 20, 22 ), rgb( 20, 20, 22 ), rgb( 250, 250, 250 ), rgb( 250, 250, 250 ) };
    static const Color frames[5] = { rgb( 20, 20, 22 ), rgb( 20, 20, 22 ), rgb( 214, 30, 36 ), rgb( 250, 250, 250 ), rgb( 250, 250, 250 ) };
    const int                                 s = std::clamp( style, 0, 4 );
    std::vector<std::pair<Vector2, Vector2>> lines;
    const float                               w = textStrokes( text, lines ) * cap;
    const float                               hw = w * 0.5f + cap * 0.9f, h = cap * 2.2f, y = lift + h * 0.5f;
    if( posts )
        for( float x : { -hw * 0.7f, hw * 0.7f } ) out.box( Vector3{ x, lift * 0.5f + 0.1f, 0.04f }, Vector3{ 0.05f, lift * 0.5f + 0.1f, 0.05f }, rgb( 140, 142, 148 ) );
    out.box( Vector3{ 0, y, 0 }, Vector3{ hw, h * 0.5f, 0.03f }, frames[s], Mat{ 0.2f, 0.0f } );
    const float inset = s == 2 ? cap * 0.22f : cap * 0.1f;
    out.quad( Vector3{ hw - inset, y - h * 0.5f + inset, -0.035f }, Vector3{ -hw + inset, y - h * 0.5f + inset, -0.035f },
              Vector3{ -hw + inset, y + h * 0.5f - inset, -0.035f }, Vector3{ hw - inset, y + h * 0.5f - inset, -0.035f }, boards[s], Mat{ 0.22f, 0.0f } );
    // The letters, read left to right by the driver (whose left is +x).
    const float   thick = cap * 0.15f;
    const Vector3 origin = { w * 0.5f, y - cap * 0.5f, -0.04f };
    for( const auto &[a, b] : lines )
    {
        const Vector3 p = { origin.x - a.x * cap, origin.y + a.y * cap, origin.z }, q = { origin.x - b.x * cap, origin.y + b.y * cap, origin.z };
        Vector3       d = Vector3Subtract( q, p );
        const float   len = Vector3Length( d );
        d = len > 1e-4f ? Vector3Scale( d, 1.0f / len ) : Vector3{ 1, 0, 0 };
        const Vector3 n = { -d.y * thick * 0.5f, d.x * thick * 0.5f, 0 };
        const Vector3 p0 = Vector3Subtract( Vector3Subtract( p, n ), Vector3Scale( d, thick * 0.4f ) ), p1 = Vector3Add( Vector3Subtract( q, n ), Vector3Scale( d, thick * 0.4f ) );
        const Vector3 p2 = Vector3Add( Vector3Add( q, n ), Vector3Scale( d, thick * 0.4f ) ), p3 = Vector3Subtract( Vector3Add( p, n ), Vector3Scale( d, thick * 0.4f ) );
        out.quad( p0, p1, p2, p3, inks[s], Mat{ 0.25f, 0.0f } );
        out.quad( p3, p2, p1, p0, inks[s], Mat{ 0.25f, 0.0f } );
    }
}

void buildSpeedSign( MeshBuilder &out, int limit, bool board )
{
    // Strokes of text centred at (0, y), facing -z, read by the driver (whose left is +x).
    auto write = [&]( std::string_view text, float y, float cap, Color ink )
    {
        std::vector<std::pair<Vector2, Vector2>> lines;
        const float                               w = textStrokes( text, lines ) * cap, thick = cap * 0.17f;
        const Vector3                             origin = { w * 0.5f, y - cap * 0.5f, -0.045f };
        for( const auto &[a, b] : lines )
        {
            const Vector3 p = { origin.x - a.x * cap, origin.y + a.y * cap, origin.z }, q = { origin.x - b.x * cap, origin.y + b.y * cap, origin.z };
            Vector3       d = Vector3Subtract( q, p );
            const float   len = Vector3Length( d );
            d = len > 1e-4f ? Vector3Scale( d, 1.0f / len ) : Vector3{ 1, 0, 0 };
            const Vector3 n = { -d.y * thick * 0.5f, d.x * thick * 0.5f, 0 };
            const Vector3 p0 = Vector3Subtract( Vector3Subtract( p, n ), Vector3Scale( d, thick * 0.4f ) ), p1 = Vector3Add( Vector3Subtract( q, n ), Vector3Scale( d, thick * 0.4f ) );
            const Vector3 p2 = Vector3Add( Vector3Add( q, n ), Vector3Scale( d, thick * 0.4f ) ), p3 = Vector3Subtract( Vector3Add( p, n ), Vector3Scale( d, thick * 0.4f ) );
            out.quad( p0, p1, p2, p3, ink, Mat{ 0.2f, 0.0f } );
            out.quad( p3, p2, p1, p0, ink, Mat{ 0.2f, 0.0f } );
        }
    };
    const Color white = rgb( 246, 246, 242 ), black = rgb( 20, 20, 22 ), red = rgb( 214, 30, 36 ), grey = rgb( 140, 142, 148 );
    const std::string number = std::to_string( limit );
    if( board )
    {
        out.box( Vector3{ 0, 1.1f, 0.03f }, Vector3{ 0.05f, 1.1f, 0.05f }, grey );
        out.box( Vector3{ 0, 2.6f, 0 }, Vector3{ 0.36f, 0.46f, 0.025f }, black, Mat{ 0.2f, 0.0f } );
        out.box( Vector3{ 0, 2.6f, -0.005f }, Vector3{ 0.33f, 0.43f, 0.025f }, white, Mat{ 0.22f, 0.0f } );
        write( "SPEED", 2.9f, 0.09f, black );
        write( "LIMIT", 2.75f, 0.09f, black );
        write( number, 2.42f, 0.26f, black );
        return;
    }
    out.box( Vector3{ 0, 1.1f, 0.03f }, Vector3{ 0.05f, 1.1f, 0.05f }, grey );
    const int   n = 20;
    const float y = 2.45f;
    for( int k = 0; k < n; ++k )
    {
        const float a0 = Tau * static_cast<float>( k ) / static_cast<float>( n ), a1 = Tau * static_cast<float>( k + 1 ) / static_cast<float>( n );
        auto        at = [&]( float r, float a, float z ) { return Vector3{ std::cos( a ) * r, y + std::sin( a ) * r, z }; };
        out.quad( at( 0.42f, a0, -0.03f ), at( 0.42f, a1, -0.03f ), at( 0.33f, a1, -0.03f ), at( 0.33f, a0, -0.03f ), red, Mat{ 0.2f, 0.0f } );
        out.tri( Vector3{ 0, y, -0.03f }, at( 0.33f, a0, -0.03f ), at( 0.33f, a1, -0.03f ), white, Mat{ 0.22f, 0.0f } );
        out.quad( at( 0.42f, a1, 0.0f ), at( 0.42f, a0, 0.0f ), Vector3{ 0, y, 0.0f }, Vector3{ 0, y, 0.0f }, grey );
    }
    write( number, y, number.size() > 2 ? 0.2f : 0.26f, black );
}

void buildForkSign( MeshBuilder &out, std::string_view left, std::string_view right, float halfWidth )
{
    const Color steel = rgb( 140, 142, 148 );
    const float span = halfWidth + 2.2f, top = 7.4f;
    for( float x : { -span, span } ) out.box( Vector3{ x, top * 0.5f, 0.3f }, Vector3{ 0.2f, top * 0.5f, 0.2f }, steel );
    out.box( Vector3{ 0, top - 0.25f, 0.3f }, Vector3{ span, 0.18f, 0.12f }, steel );
    out.box( Vector3{ 0, top - 1.6f, 0.3f }, Vector3{ span, 0.12f, 0.1f }, steel );
    for( int k = 0; k < 2; ++k )
    {
        // Each board over its carriageway (the driver's left is +x), the arrow on the side it turns to.
        const std::string text = k == 0 ? "< " + std::string( left ) : std::string( right ) + " >";
        std::vector<std::pair<Vector2, Vector2>> lines;
        const float room = halfWidth - 0.6f;
        const float cap = std::min( 0.6f, room / ( textStrokes( text, lines ) + 1.8f ) );
        MeshBuilder board;
        buildPlaceSign( board, text, 4, cap, top - 0.4f - cap * 2.2f, false );
        out.append( board, Vector3{ ( k == 0 ? 1.0f : -1.0f ) * halfWidth * 0.5f, 0, 0 }, 0.0f, 1.0f );
    }
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

    // ---- Regional vehicles ------------------------------------------------------------------------------------------
    auto addVehicle = [&]( MeshBuilder &mb, float length, float width, uint32_t fleets, uint8_t where, float pace, bool truck )
    {
        VehicleModel vm;
        vm.m_mesh = mb.upload();
        vm.m_length = length;
        vm.m_width = width;
        vm.m_fleets = fleets;
        vm.m_where = where;
        vm.m_pace = pace;
        vm.m_truck = truck;
        m_traffic.push_back( vm );
    };
    const Color tyreHub = rgb( 170, 172, 178 );
    { // Yellow cab.
        MeshBuilder mb;
        buildCar( mb, saloon, rgb( 250, 196, 18 ) );
        mb.box( Vector3{ 0, 1.52f, -0.3f }, Vector3{ 0.32f, 0.1f, 0.12f }, rgb( 250, 250, 240 ), Mat{ 0.6f, 0.0f } );
        for( float side : { -1.0f, 1.0f } ) mb.box( Vector3{ side * 0.895f, 0.62f, 0.0f }, Vector3{ 0.01f, 0.06f, 1.2f }, rgb( 20, 20, 20 ) );
        addVehicle( mb, 4.6f, 1.78f, bit( FleetE::NORTH_AMERICA ), 1, 1.0f, false );
    }
    for( int v = 0; v < 2; ++v )
    { // School bus, or a painted chicken bus.
        MeshBuilder mb;
        const Color body = v ? rgb( 40, 120, 200 ) : rgb( 246, 178, 0 );
        mb.box( Vector3{ 0, 1.75f, -0.8f }, Vector3{ 1.2f, 1.25f, 4.6f }, body, Mat{ 0.0f, 0.4f } );
        mb.box( Vector3{ 0, 1.0f, 4.45f }, Vector3{ 1.1f, 0.5f, 0.65f }, body, Mat{ 0.0f, 0.4f } );
        mb.quad( Vector3{ 1.0f, 2.0f, 3.81f }, Vector3{ -1.0f, 2.0f, 3.81f }, Vector3{ -1.0f, 2.75f, 3.81f }, Vector3{ 1.0f, 2.75f, 3.81f }, rgb( 22, 30, 44 ), Glass );
        sideWindows( mb, 1.2f, 1.9f, 2.6f, -5.0f, 3.4f, 9 );
        if( v )
            for( int k = 0; k < 4; ++k )
                for( float side : { -1.0f, 1.0f } )
                    mb.box( Vector3{ side * 1.205f, 1.0f + 0.18f * k, -0.8f }, Vector3{ 0.005f, 0.07f, 4.5f },
                            k % 2 ? rgb( 250, 210, 30 ) : k == 0 ? rgb( 220, 40, 40 ) : rgb( 60, 180, 80 ) );
        else
            for( float side : { -1.0f, 1.0f } ) mb.box( Vector3{ side * 1.205f, 1.45f, -0.8f }, Vector3{ 0.005f, 0.05f, 4.5f }, rgb( 20, 20, 20 ) );
        tyres( mb, 1.18f, 0.5f, { 3.6f, -3.4f }, tyreHub );
        lights( mb, 1.1f, 0.85f, 5.1f, -5.4f );
        addVehicle( mb, 10.6f, 2.4f, v ? bit( FleetE::LATIN ) : bit( FleetE::NORTH_AMERICA ), v ? 2 : 0, 0.8f, true );
    }
    const Color pickups[4] = { rgb( 180, 30, 30 ), rgb( 236, 236, 232 ), rgb( 150, 154, 160 ), rgb( 36, 40, 48 ) };
    for( int v = 0; v < 4; ++v )
    { // Pickup (a ute down under).
        MeshBuilder mb;
        const Color c = pickups[v];
        mb.box( Vector3{ 0, 0.85f, 1.55f }, Vector3{ 0.95f, 0.4f, 0.95f }, c, Mat{ 0.0f, 0.5f } );
        mb.box( Vector3{ 0, 1.35f, 0.25f }, Vector3{ 0.93f, 0.6f, 0.75f }, c, Mat{ 0.0f, 0.5f } );
        mb.quad( Vector3{ 0.85f, 1.25f, 1.01f }, Vector3{ -0.85f, 1.25f, 1.01f }, Vector3{ -0.8f, 1.85f, 0.95f }, Vector3{ 0.8f, 1.85f, 0.95f }, rgb( 22, 30, 44 ), Glass );
        sideWindows( mb, 0.93f, 1.3f, 1.8f, -0.4f, 0.9f, 1 );
        mb.box( Vector3{ 0, 0.6f, -1.3f }, Vector3{ 0.95f, 0.15f, 1.0f }, scaleColor( c, 0.6f ) );
        for( float side : { -1.0f, 1.0f } ) mb.box( Vector3{ side * 0.9f, 1.0f, -1.3f }, Vector3{ 0.05f, 0.3f, 1.0f }, c, Mat{ 0.0f, 0.5f } );
        mb.box( Vector3{ 0, 1.0f, -2.28f }, Vector3{ 0.95f, 0.3f, 0.04f }, c, Mat{ 0.0f, 0.5f } );
        tyres( mb, 0.98f, 0.4f, { 1.6f, -1.4f }, tyreHub );
        lights( mb, 0.95f, 0.95f, 2.5f, -2.32f );
        addVehicle( mb, 5.0f, 1.95f, bit( FleetE::NORTH_AMERICA ) | bit( FleetE::LATIN ) | bit( FleetE::AFRICA ) | bit( FleetE::ARABIA ) | bit( FleetE::OCEANIA ) |
                                         bit( FleetE::ISLANDS ),
                    2, 1.0f, false );
    }
    for( int v = 0; v < 2; ++v )
    { // Long-nose big rig, or a road train of three trailers.
        MeshBuilder mb;
        const Color cab = v ? rgb( 200, 60, 30 ) : rgb( 30, 70, 150 );
        mb.box( Vector3{ 0, 1.3f, 5.6f }, Vector3{ 1.15f, 0.65f, 1.1f }, cab, Mat{ 0.0f, 0.5f } );
        mb.box( Vector3{ 0, 2.0f, 3.9f }, Vector3{ 1.25f, 1.3f, 0.75f }, cab, Mat{ 0.0f, 0.5f } );
        mb.quad( Vector3{ 1.1f, 2.4f, 4.66f }, Vector3{ -1.1f, 2.4f, 4.66f }, Vector3{ -1.1f, 3.0f, 4.66f }, Vector3{ 1.1f, 3.0f, 4.66f }, rgb( 22, 30, 44 ), Glass );
        mb.box( Vector3{ 0, 1.05f, 6.72f }, Vector3{ 1.0f, 0.5f, 0.05f }, rgb( 200, 200, 204 ), Mat{ 0.0f, 0.7f } );
        for( float side : { -1.0f, 1.0f } ) mb.cylinder( Vector3{ side * 1.1f, 2.6f, 3.1f }, 0.1f, 0.1f, 1.6f, 6, rgb( 200, 200, 204 ), Mat{ 0.0f, 0.7f } );
        const int   loads = v ? 3 : 1;
        float       z = 2.9f;
        for( int k = 0; k < loads; ++k )
        {
            const float len = v ? 9.5f : 13.0f;
            const Color box = v ? ( k % 2 ? rgb( 210, 206, 196 ) : rgb( 190, 180, 160 ) ) : rgb( 214, 218, 224 );
            mb.box( Vector3{ 0, 2.35f, z - len * 0.5f }, Vector3{ 1.25f, 1.55f, len * 0.5f }, box, Mat{ 0.0f, 0.4f } );
            tyres( mb, 1.2f, 0.5f, { z - len + 1.2f, z - len + 2.4f }, tyreHub );
            z -= len + 0.8f;
        }
        tyres( mb, 1.2f, 0.5f, { 5.5f, 2.2f }, tyreHub );
        lights( mb, 1.1f, 0.9f, 6.75f, z + 0.79f );
        const float length = 6.7f - ( z + 0.8f );
        addVehicle( mb, length, 2.5f, v ? bit( FleetE::OCEANIA ) : bit( FleetE::NORTH_AMERICA ), 2, v ? 0.85f : 0.9f, true );
    }
    { // Black cab.
        MeshBuilder mb;
        CarShape cab = saloon;
        for( CabinStation &c : cab.m_cabin ) c.m_yT += 0.15f;
        buildCar( mb, cab, rgb( 18, 18, 22 ) );
        mb.box( Vector3{ 0, 1.68f, -0.2f }, Vector3{ 0.25f, 0.08f, 0.1f }, rgb( 255, 190, 40 ), Mat{ 0.8f, 0.0f } );
        addVehicle( mb, 4.6f, 1.78f, bit( FleetE::BRITISH ), 1, 1.0f, false );
    }
    { // Red double-decker.
        MeshBuilder mb;
        const Color red = rgb( 200, 20, 28 );
        mb.box( Vector3{ 0, 2.35f, 0 }, Vector3{ 1.25f, 2.0f, 5.4f }, red, Mat{ 0.0f, 0.4f } );
        sideWindows( mb, 1.25f, 1.5f, 2.3f, -5.0f, 4.8f, 7 );
        sideWindows( mb, 1.25f, 3.0f, 3.9f, -5.0f, 5.0f, 7 );
        for( float y : { 1.5f, 3.0f } )
            mb.quad( Vector3{ 1.1f, y, 5.41f }, Vector3{ -1.1f, y, 5.41f }, Vector3{ -1.1f, y + 0.8f, 5.41f }, Vector3{ 1.1f, y + 0.8f, 5.41f }, rgb( 22, 30, 44 ), Glass );
        mb.box( Vector3{ 0, 4.36f, 4.0f }, Vector3{ 0.9f, 0.08f, 0.3f }, rgb( 20, 20, 20 ) );
        tyres( mb, 1.2f, 0.52f, { 3.8f, -3.2f }, tyreHub );
        lights( mb, 1.15f, 0.7f, 5.4f, -5.4f );
        addVehicle( mb, 10.9f, 2.5f, bit( FleetE::BRITISH ), 0, 0.8f, true );
    }
    const Color beetles[3] = { rgb( 60, 160, 70 ), rgb( 236, 236, 228 ), rgb( 70, 130, 200 ) };
    for( int v = 0; v < 3; ++v )
    { // Beetle.
        MeshBuilder mb;
        CarShape    bug;
        bug.m_body = { { -1.95f, 0.78f, 0.32f, 0.86f }, { -1.6f, 0.8f, 0.28f, 1.0f }, { 0.9f, 0.8f, 0.28f, 0.98f }, { 1.6f, 0.76f, 0.32f, 0.8f },
                       { 1.95f, 0.6f, 0.4f, 0.66f } };
        bug.m_cabin = { { -1.6f, 0.7f, 0.98f, 0.7f, 0.98f }, { -1.1f, 0.72f, 1.0f, 0.55f, 1.42f }, { 0.2f, 0.72f, 0.98f, 0.55f, 1.42f }, { 0.75f, 0.72f, 0.96f, 0.72f, 0.96f } };
        bug.m_wheelFront = 1.3f;
        bug.m_wheelRear = -1.2f;
        bug.m_wheelR = 0.33f;
        bug.m_sideGlass = true;
        bug.m_wing = 0.0f;
        buildCar( mb, bug, beetles[v] );
        addVehicle( mb, 4.0f, 1.6f, bit( FleetE::LATIN ) | ( v == 2 ? bit( FleetE::EUROPE ) : 0u ), 0, 0.85f, false );
    }
    const Color minibuses[3] = { rgb( 250, 210, 30 ), rgb( 40, 160, 120 ), rgb( 230, 70, 140 ) };
    for( int v = 0; v < 3; ++v )
    { // Minibus: matatu, marshrutka, island bus.
        MeshBuilder mb;
        buildCar( mb, van, minibuses[v] );
        for( float side : { -1.0f, 1.0f } )
            for( int k = 0; k < 3; ++k )
                mb.box( Vector3{ side * 0.955f, 0.75f + 0.12f * k, 0.0f }, Vector3{ 0.005f, 0.04f, 2.0f },
                        k == 1 ? rgb( 250, 250, 250 ) : scaleColor( minibuses[( v + 1 ) % 3], 0.9f ) );
        mb.box( Vector3{ 0, 2.12f, -0.4f }, Vector3{ 0.8f, 0.06f, 1.2f }, rgb( 60, 60, 64 ) );
        addVehicle( mb, 4.9f, 1.9f, v == 0 ? bit( FleetE::EASTERN ) | bit( FleetE::AFRICA ) : v == 1 ? bit( FleetE::AFRICA ) | bit( FleetE::SOUTHEAST_ASIA ) : bit( FleetE::ISLANDS ) | bit( FleetE::LATIN ),
                    0, 0.95f, false );
    }
    const Color rickshaws[3] = { rgb( 250, 200, 20 ), rgb( 40, 140, 70 ), rgb( 30, 100, 200 ) };
    for( int v = 0; v < 3; ++v )
    { // Auto rickshaw, tuk-tuk.
        MeshBuilder mb;
        const Color c = rickshaws[v];
        mb.box( Vector3{ 0, 0.55f, 0.0f }, Vector3{ 0.65f, 0.25f, 1.25f }, rgb( 30, 30, 34 ) );
        mb.box( Vector3{ 0, 1.15f, -0.3f }, Vector3{ 0.68f, 0.35f, 0.8f }, c, Mat{ 0.0f, 0.4f } );
        mb.taperedBox( Vector3{ 0, 0.8f, 0.85f }, Vector2{ 0.45f, 0.3f }, Vector3{ 0, 1.5f, 0.75f }, Vector2{ 0.35f, 0.2f }, c, Mat{ 0.0f, 0.4f } );
        mb.box( Vector3{ 0, 1.85f, -0.15f }, Vector3{ 0.72f, 0.06f, 1.0f }, v == 0 ? rgb( 40, 140, 70 ) : rgb( 30, 30, 34 ) );
        for( float side : { -1.0f, 1.0f } ) mb.box( Vector3{ side * 0.62f, 1.55f, 0.75f }, Vector3{ 0.03f, 0.3f, 0.03f }, rgb( 30, 30, 34 ) );
        mb.quad( Vector3{ 0.55f, 1.35f, 0.86f }, Vector3{ -0.55f, 1.35f, 0.86f }, Vector3{ -0.55f, 1.78f, 0.86f }, Vector3{ 0.55f, 1.78f, 0.86f }, rgb( 22, 30, 44 ), Glass );
        mb.tube( Vector3{ -0.1f, 0.28f, 1.1f }, Vector3{ 0.1f, 0.28f, 1.1f }, 0.28f, 8, rgb( 20, 20, 22 ), Mat{}, true );
        for( float side : { -1.0f, 1.0f } )
            mb.tube( Vector3{ side * 0.48f, 0.28f, -0.75f }, Vector3{ side * 0.68f, 0.28f, -0.75f }, 0.28f, 8, rgb( 20, 20, 22 ), Mat{}, true );
        lights( mb, 0.6f, 0.75f, 1.26f, -1.26f );
        addVehicle( mb, 2.6f, 1.4f, bit( FleetE::SOUTH_ASIA ) | bit( FleetE::SOUTHEAST_ASIA ), 0, 0.55f, false );
    }
    { // Decorated truck.
        MeshBuilder mb;
        const Color colours[5] = { rgb( 230, 40, 40 ), rgb( 250, 200, 20 ), rgb( 40, 160, 80 ), rgb( 40, 100, 220 ), rgb( 240, 120, 30 ) };
        mb.box( Vector3{ 0, 1.6f, 3.0f }, Vector3{ 1.15f, 1.0f, 0.9f }, colours[0], Mat{ 0.0f, 0.5f } );
        mb.taperedBox( Vector3{ 0, 2.6f, 3.3f }, Vector2{ 1.15f, 0.6f }, Vector3{ 0, 3.6f, 3.6f }, Vector2{ 1.0f, 0.3f }, colours[1], Mat{ 0.0f, 0.5f } );
        mb.quad( Vector3{ 1.0f, 1.9f, 3.91f }, Vector3{ -1.0f, 1.9f, 3.91f }, Vector3{ -1.0f, 2.4f, 3.91f }, Vector3{ 1.0f, 2.4f, 3.91f }, rgb( 22, 30, 44 ), Glass );
        mb.box( Vector3{ 0, 1.9f, -1.0f }, Vector3{ 1.25f, 1.2f, 3.0f }, colours[3] );
        for( int k = 0; k < 10; ++k )
            for( float side : { -1.0f, 1.0f } )
                mb.box( Vector3{ side * 1.26f, 1.1f + 0.16f * static_cast<float>( k ), -1.0f }, Vector3{ 0.005f, 0.06f, 2.9f }, colours[( k + 1 ) % 5], Mat{ 0.2f, 0.4f } );
        tyres( mb, 1.2f, 0.5f, { 2.9f, -2.6f }, tyreHub );
        lights( mb, 1.1f, 0.85f, 3.95f, -4.05f );
        addVehicle( mb, 8.0f, 2.5f, bit( FleetE::SOUTH_ASIA ), 0, 0.75f, true );
    }
    const Color scooters[3] = { rgb( 220, 40, 40 ), rgb( 240, 240, 236 ), rgb( 50, 120, 200 ) };
    for( int v = 0; v < 3; ++v )
    { // Scooter with its rider.
        MeshBuilder mb;
        for( float z : { 0.62f, -0.6f } ) mb.tube( Vector3{ -0.06f, 0.28f, z }, Vector3{ 0.06f, 0.28f, z }, 0.28f, 8, rgb( 20, 20, 22 ), Mat{}, true );
        mb.box( Vector3{ 0, 0.55f, -0.2f }, Vector3{ 0.2f, 0.2f, 0.55f }, scooters[v], Mat{ 0.0f, 0.5f } );
        mb.box( Vector3{ 0, 0.85f, 0.55f }, Vector3{ 0.18f, 0.35f, 0.08f }, scooters[v], Mat{ 0.0f, 0.5f } );
        mb.tube( Vector3{ -0.3f, 1.2f, 0.55f }, Vector3{ 0.3f, 1.2f, 0.55f }, 0.025f, 4, rgb( 40, 40, 44 ) );
        const Color shirt = v == 0 ? rgb( 240, 240, 236 ) : v == 1 ? rgb( 60, 90, 140 ) : rgb( 200, 80, 40 );
        mb.box( Vector3{ 0, 1.15f, -0.25f }, Vector3{ 0.2f, 0.32f, 0.12f }, shirt );
        mb.blob( Vector3{ 0, 1.65f, -0.2f }, Vector3{ 0.14f, 0.15f, 0.15f }, v == 1 ? rgb( 230, 230, 230 ) : rgb( 30, 30, 34 ), 41 + v, 0.0f );
        for( float side : { -1.0f, 1.0f } )
        {
            mb.tube( Vector3{ side * 0.12f, 0.85f, -0.25f }, Vector3{ side * 0.15f, 0.6f, 0.15f }, 0.06f, 4, rgb( 50, 60, 80 ) );
            mb.tube( Vector3{ side * 0.18f, 1.35f, -0.2f }, Vector3{ side * 0.28f, 1.2f, 0.5f }, 0.05f, 4, shirt );
        }
        mb.quad( Vector3{ 0.08f, 0.85f, 0.64f }, Vector3{ -0.08f, 0.85f, 0.64f }, Vector3{ -0.08f, 0.95f, 0.64f }, Vector3{ 0.08f, 0.95f, 0.64f }, rgb( 255, 246, 224 ), Mat{ 0.7f, 0.3f } );
        mb.quad( Vector3{ -0.08f, 0.6f, -0.76f }, Vector3{ 0.08f, 0.6f, -0.76f }, Vector3{ 0.08f, 0.68f, -0.76f }, Vector3{ -0.08f, 0.68f, -0.76f }, rgb( 255, 24, 24 ), Mat{ 0.5f, 0.3f } );
        addVehicle( mb, 1.9f, 0.8f, bit( FleetE::SOUTHEAST_ASIA ) | bit( FleetE::EAST_ASIA ) | bit( FleetE::SOUTH_ASIA ), 0, 0.7f, false );
    }
    { // Jeepney.
        MeshBuilder mb;
        const Color chrome = rgb( 210, 212, 218 );
        mb.box( Vector3{ 0, 1.0f, 2.3f }, Vector3{ 0.85f, 0.4f, 0.8f }, chrome, Mat{ 0.0f, 0.7f } );
        mb.box( Vector3{ 0, 1.55f, -0.9f }, Vector3{ 1.0f, 0.85f, 2.6f }, rgb( 236, 60, 50 ), Mat{ 0.0f, 0.5f } );
        mb.box( Vector3{ 0, 2.45f, -0.9f }, Vector3{ 1.02f, 0.06f, 2.7f }, rgb( 250, 210, 30 ) );
        sideWindows( mb, 1.0f, 1.6f, 2.15f, -3.3f, 1.4f, 6 );
        for( float side : { -1.0f, 1.0f } ) mb.box( Vector3{ side * 1.005f, 1.1f, -0.9f }, Vector3{ 0.005f, 0.1f, 2.5f }, rgb( 40, 100, 220 ) );
        mb.blob( Vector3{ 0, 1.55f, 3.0f }, Vector3{ 0.3f, 0.3f, 0.12f }, chrome, 51, 0.0f, Mat{ 0.0f, 0.8f } );
        tyres( mb, 0.98f, 0.38f, { 2.3f, -2.4f }, chrome );
        lights( mb, 0.85f, 1.0f, 3.1f, -3.52f );
        addVehicle( mb, 6.6f, 2.0f, bit( FleetE::SOUTHEAST_ASIA ), 0, 0.85f, false );
    }
    { // Kei truck.
        MeshBuilder mb;
        mb.box( Vector3{ 0, 1.05f, 1.2f }, Vector3{ 0.72f, 0.65f, 0.55f }, rgb( 240, 240, 236 ), Mat{ 0.0f, 0.5f } );
        mb.quad( Vector3{ 0.65f, 1.15f, 1.76f }, Vector3{ -0.65f, 1.15f, 1.76f }, Vector3{ -0.65f, 1.6f, 1.76f }, Vector3{ 0.65f, 1.6f, 1.76f }, rgb( 22, 30, 44 ), Glass );
        mb.box( Vector3{ 0, 0.6f, -0.55f }, Vector3{ 0.72f, 0.1f, 1.2f }, rgb( 200, 200, 196 ) );
        for( float side : { -1.0f, 1.0f } ) mb.box( Vector3{ side * 0.7f, 0.82f, -0.55f }, Vector3{ 0.02f, 0.14f, 1.2f }, rgb( 200, 200, 196 ) );
        tyres( mb, 0.72f, 0.28f, { 1.2f, -1.2f }, tyreHub );
        lights( mb, 0.7f, 0.75f, 1.76f, -1.76f );
        addVehicle( mb, 3.4f, 1.48f, bit( FleetE::EAST_ASIA ), 0, 0.85f, false );
    }
    { // White land cruiser.
        MeshBuilder mb;
        buildCar( mb, van, rgb( 242, 242, 238 ) );
        mb.box( Vector3{ 0, 2.1f, -0.2f }, Vector3{ 0.85f, 0.05f, 1.3f }, rgb( 60, 60, 64 ) );
        mb.box( Vector3{ 0, 1.0f, -2.5f }, Vector3{ 0.3f, 0.3f, 0.06f }, rgb( 40, 40, 44 ) );
        addVehicle( mb, 4.9f, 1.9f, bit( FleetE::ARABIA ) | bit( FleetE::AFRICA ), 0, 1.05f, false );
    }
    const Color ladas[3] = { rgb( 200, 190, 160 ), rgb( 150, 40, 40 ), rgb( 70, 110, 90 ) };
    for( int v = 0; v < 3; ++v )
    { // Boxy saloon of the east.
        MeshBuilder mb;
        CarShape    box = saloon;
        for( Station &s : box.m_body ) s.m_hw *= 0.94f;
        box.m_body.front().m_yt = box.m_body.back().m_yt = 0.86f;
        buildCar( mb, box, ladas[v] );
        addVehicle( mb, 4.2f, 1.62f, bit( FleetE::EASTERN ), 0, 0.85f, false );
    }
    for( int v = 0; v < 2; ++v )
    { // Tractor.
        MeshBuilder mb;
        const Color body = v ? rgb( 200, 30, 30 ) : rgb( 40, 120, 50 );
        mb.box( Vector3{ 0, 1.15f, 0.9f }, Vector3{ 0.45f, 0.45f, 0.9f }, body, Mat{ 0.0f, 0.4f } );
        mb.box( Vector3{ 0, 1.95f, -0.45f }, Vector3{ 0.7f, 0.85f, 0.6f }, rgb( 22, 30, 44 ), Glass );
        mb.box( Vector3{ 0, 2.85f, -0.45f }, Vector3{ 0.78f, 0.06f, 0.7f }, body );
        for( float side : { -1.0f, 1.0f } )
        {
            mb.tube( Vector3{ side * 0.55f, 0.8f, -0.6f }, Vector3{ side * 1.0f, 0.8f, -0.6f }, 0.8f, 12, rgb( 20, 20, 22 ), Mat{}, true );
            mb.tube( Vector3{ side * 0.5f, 0.45f, 1.45f }, Vector3{ side * 0.78f, 0.45f, 1.45f }, 0.45f, 10, rgb( 20, 20, 22 ), Mat{}, true );
        }
        mb.cylinder( Vector3{ 0.25f, 1.6f, 1.4f }, 0.06f, 0.06f, 1.0f, 6, rgb( 40, 40, 44 ) );
        lights( mb, 0.45f, 1.2f, 1.81f, -1.06f );
        addVehicle( mb, 3.6f, 2.0f, bit( FleetE::EUROPE ) | bit( FleetE::EASTERN ), 2, 0.38f, false );
    }
    for( int v = 0; v < 2; ++v )
    { // Camper van in two tones.
        MeshBuilder mb;
        buildCar( mb, van, v ? rgb( 230, 120, 40 ) : rgb( 70, 150, 190 ) );
        for( float side : { -1.0f, 1.0f } ) mb.box( Vector3{ side * 0.955f, 1.6f, 0.0f }, Vector3{ 0.005f, 0.5f, 2.3f }, rgb( 244, 242, 234 ) );
        mb.box( Vector3{ 0, 2.12f, -0.6f }, Vector3{ 0.8f, 0.1f, 1.4f }, rgb( 244, 242, 234 ) );
        addVehicle( mb, 4.9f, 1.9f, bit( FleetE::EUROPE ) | bit( FleetE::OCEANIA ), 2, 0.9f, false );
    }

    // ---- Animals that cross the road ---------------------------------------------------------------------------
    const std::pair<PropKindE, Vector2> beasts[] = { { PropKindE::COW, { 2.2f, 0.8f } },      { PropKindE::SHEEP, { 1.3f, 0.6f } },
                                                     { PropKindE::HORSE, { 2.2f, 0.7f } },    { PropKindE::CAMEL, { 2.6f, 0.9f } },
                                                     { PropKindE::ELEPHANT, { 4.5f, 2.0f } }, { PropKindE::GIRAFFE, { 3.0f, 1.2f } },
                                                     { PropKindE::ZEBRA, { 2.2f, 0.7f } },    { PropKindE::KANGAROO, { 1.4f, 0.6f } },
                                                     { PropKindE::LLAMA, { 1.6f, 0.6f } },    { PropKindE::BISON, { 3.0f, 1.2f } },
                                                     { PropKindE::MOOSE, { 3.0f, 1.0f } },    { PropKindE::REINDEER, { 1.8f, 0.7f } },
                                                     { PropKindE::YAK, { 2.4f, 1.0f } } };
    for( const auto &[kind, size] : beasts )
    {
        const MeshBuilder &src = prop( kind, 0 );
        if( src.triangleCount() == 0 ) continue;
        MeshBuilder mb;
        mb.append( src, Vector3{ 0, 0, 0 }, 0.0f, 1.0f );
        VehicleModel vm;
        vm.m_mesh = mb.upload();
        vm.m_length = size.x;
        vm.m_width = size.y;
        vm.m_animal = kind;
        vm.m_fleets = ~0u;
        m_traffic.push_back( vm );
    }

    buildCreatures();

    // ---- Number plates and the speed camera ------------------------------------------------------------------
    {
        const Color white = rgb( 240, 240, 236 ), ink = rgb( 30, 30, 34 ), blue = rgb( 0, 51, 153 ), yellow = rgb( 250, 206, 30 );
        for( int style = 0; style < static_cast<int>( m_plates.size() ); ++style )
        {
            MeshBuilder m;
            const auto  s = static_cast<PlateE>( style );
            const float hw = s == PlateE::US || s == PlateE::JP ? 0.17f : 0.26f, hh = s == PlateE::US || s == PlateE::JP ? 0.08f : 0.06f;
            const Color face = s == PlateE::UK || s == PlateE::AMBER ? yellow : s == PlateE::DARK ? rgb( 20, 20, 22 ) : white;
            const Color letters = s == PlateE::DARK ? white : s == PlateE::JP ? rgb( 20, 110, 60 ) : ink;
            m.box( Vector3{ 0, 0, 0 }, Vector3{ hw, hh, 0.01f }, face, Mat{ 0.1f, 0.0f } );
            if( s == PlateE::EU ) m.box( Vector3{ hw - 0.025f, 0, -0.012f }, Vector3{ 0.025f, hh, 0.002f }, blue );
            if( s == PlateE::US ) m.box( Vector3{ 0, hh - 0.015f, -0.012f }, Vector3{ hw, 0.015f, 0.002f }, rgb( 180, 30, 40 ) );
            for( int k = 0; k < 6; ++k )
                m.box( Vector3{ hw * 0.62f - static_cast<float>( k ) * hw * 0.25f, -0.005f, -0.013f }, Vector3{ hw * 0.07f, hh * 0.5f, 0.002f }, letters );
            m_plates[static_cast<size_t>( style )] = m.upload();
        }
        // The camera: a grey box on a post, its lens and flash towards the traffic.
        MeshBuilder &m = m_camera;
        m.box( Vector3{ 0, 1.3f, 0 }, Vector3{ 0.07f, 1.3f, 0.07f }, rgb( 120, 122, 128 ) );
        m.box( Vector3{ 0, 2.9f, 0 }, Vector3{ 0.28f, 0.3f, 0.3f }, rgb( 150, 152, 156 ) );
        m.box( Vector3{ 0.08f, 2.95f, -0.31f }, Vector3{ 0.08f, 0.08f, 0.02f }, rgb( 20, 22, 26 ), Mat{ 0.0f, 0.8f } );
        m.box( Vector3{ -0.12f, 2.95f, -0.31f }, Vector3{ 0.06f, 0.1f, 0.02f }, rgb( 230, 230, 236 ), Mat{ 0.3f, 0.0f } );
    }

    // ---- Flags and the border post -----------------------------------------------------------------------------
    buildFlags();
    {
        MeshBuilder &m = m_borderPost;
        m.box( Vector3{ 0, 1.3f, 0 }, Vector3{ 1.2f, 1.3f, 1.4f }, rgb( 236, 236, 230 ) );
        m.box( Vector3{ 0, 2.75f, 0 }, Vector3{ 1.5f, 0.15f, 1.7f }, rgb( 40, 70, 140 ) );
        m.quad( Vector3{ 1.21f, 1.2f, -0.9f }, Vector3{ 1.21f, 1.2f, 0.9f }, Vector3{ 1.21f, 2.1f, 0.9f }, Vector3{ 1.21f, 2.1f, -0.9f }, rgb( 30, 40, 60 ), Glass );
        // The canopy over the lane, and the barrier, raised.
        for( float z : { -2.4f, 2.4f } ) m.box( Vector3{ 1.6f, 2.6f, z }, Vector3{ 0.12f, 2.6f, 0.12f }, rgb( 200, 202, 206 ) );
        m.box( Vector3{ 5.0f, 5.3f, 0 }, Vector3{ 3.6f, 0.2f, 2.8f }, rgb( 236, 236, 230 ) );
        m.box( Vector3{ 5.0f, 5.05f, 0 }, Vector3{ 3.6f, 0.06f, 2.8f }, rgb( 255, 240, 200 ), Mat{ 0.8f, 0.0f } );
        m.box( Vector3{ 1.5f, 0.55f, -3.2f }, Vector3{ 0.18f, 0.55f, 0.18f }, rgb( 60, 60, 64 ) );
        for( int k = 0; k < 6; ++k )
        {
            const float a = 1.25f, t0 = 0.6f * k, t1 = t0 + 0.6f;
            const Vector3 p0 = { 1.5f + std::cos( a ) * t0, 1.0f + std::sin( a ) * t0, -3.2f }, p1 = { 1.5f + std::cos( a ) * t1, 1.0f + std::sin( a ) * t1, -3.2f };
            m.tube( p0, p1, 0.06f, 5, k % 2 ? rgb( 250, 250, 250 ) : rgb( 210, 30, 30 ) );
        }
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

void Models::buildFlags()
{
    m_flags.assign( static_cast<size_t>( countryTableSize() ), {} );
    for( int c = 0; c < countryTableSize(); ++c ) buildFlag( m_flags[static_cast<size_t>( c )], countryAt( c ).m_flag );
}

void Models::buildCreatures()
{
    // ---- What swims round the undersea tunnels: the creatures of data/creatures ----------------------------------
    m_creatureFirst = m_traffic.size();
    m_creatureModel.assign( dataShapes().size(), -1 );
    for( size_t i = 0; i < dataShapes().size(); ++i )
    {
        if( !dataShapes()[i].m_creature ) continue;
        MeshBuilder shape, mb;
        buildDataShape( shape, dataShapes()[i] );
        mb.append( shape, Vector3{ 0.0f, 0.0f, 0.0f }, 0.0f, CreatureScale );   // a little larger than life: seen through glass
        const Vector3 e = mb.extent();
        VehicleModel  vm;
        vm.m_mesh = mb.upload();
        vm.m_length = std::max( 0.5f, e.z * 2.0f );
        vm.m_width = std::max( 0.5f, e.x * 2.0f );
        vm.m_animal = PropKindE::LANDMARK;   // not a vehicle, not a land animal: a swimmer
        m_creatureModel[i] = static_cast<int>( m_traffic.size() );
        m_traffic.push_back( vm );
    }

    // ---- The police, the ambulances and the fire engines: the POLICE_CAR, AMBULANCE and FIRE_ENGINE shapes, -------
    //      their light bars found by colour
    m_policeModel.assign( dataShapes().size(), -1 );
    for( size_t i = 0; i < dataShapes().size(); ++i )
    {
        const std::string &name = dataShapes()[i].m_name;
        if( name.rfind( "POLICE_CAR", 0 ) != 0 && name.rfind( "AMBULANCE", 0 ) != 0 && name.rfind( "FIRE_ENGINE", 0 ) != 0 ) continue;
        MeshBuilder shape;
        buildDataShape( shape, dataShapes()[i] );
        const Vector3 e = shape.extent();
        VehicleModel  vm;
        vm.m_length = std::max( 1.0f, e.z * 2.0f );
        vm.m_width = std::max( 1.0f, e.x * 2.0f );
        vm.m_police = true;
        Vector3 light{}, unused{};
        auto    red = []( Color c ) { return c.r > 200 && c.g < 80 && c.b < 80; };
        auto    blue = []( Color c ) { return c.r < 80 && c.g < 120 && c.b > 200; };
        auto    lamp = []( Color c ) { return ( c.r > 200 && c.g < 80 && c.b < 80 ) || ( c.r < 80 && c.g < 120 && c.b > 200 ); };
        vm.m_lamps = static_cast<uint8_t>( ( shape.centreOf( red, unused ) ? 1 : 0 ) | ( shape.centreOf( blue, unused ) ? 2 : 0 ) );
        vm.m_light = shape.centreOf( lamp, light ) ? Vector3{ 0.0f, light.y, light.z } : Vector3{ 0.0f, e.y, -0.2f };
        vm.m_mesh = shape.upload();
        m_policeModel[i] = static_cast<int>( m_traffic.size() );
        m_traffic.push_back( vm );
    }
    // ---- The fast trains: TRAIN_NAME and TRAIN_NAME_COACH for each of data/trains.txt ---------------------------------
    for( auto &t : m_trains )
        for( Mesh &m : t )
            if( m.vaoId ) UnloadMesh( m );
    m_trains.assign( trainLines().size(), {} );
    for( size_t t = 0; t < trainLines().size(); ++t )
        for( int part = 0; part < 2; ++part )
        {
            const std::string name = "TRAIN_" + trainLines()[t].m_name + ( part ? "_COACH" : "" );
            for( const DataShape &d : dataShapes() )
                if( d.m_name == name )
                {
                    MeshBuilder shape;
                    buildDataShape( shape, d );
                    m_trains[t][static_cast<size_t>( part )] = shape.upload();
                    break;
                }
        }
    // ---- The spectacles: the SKY_ shapes -------------------------------------------------------------------------------
    for( Mesh &m : m_sky )
        if( m.vaoId ) UnloadMesh( m );
    m_sky.assign( SKY_MODELS, Mesh{} );
    for( int k = 0; k < SKY_MODELS; ++k )
        for( const DataShape &d : dataShapes() )
            if( d.m_name == skyModelName( k ) )
            {
                MeshBuilder shape;
                buildDataShape( shape, d );
                m_sky[static_cast<size_t>( k )] = shape.upload();
                break;
            }
    // ---- The hitchhikers: the HITCHHIKER shapes ------------------------------------------------------------------------
    for( Mesh &m : m_hitchhikers )
        if( m.vaoId ) UnloadMesh( m );
    m_hitchhikers.clear();
    for( const DataShape &d : dataShapes() )
    {
        if( d.m_name.rfind( "HITCHHIKER", 0 ) != 0 ) continue;
        MeshBuilder shape;
        buildDataShape( shape, d );
        m_hitchhikers.push_back( shape.upload() );
    }
    {
        // A puddle the rain leaves: a dark, ragged mirror on the road.
        if( m_puddle.vaoId ) UnloadMesh( m_puddle );
        MeshBuilder pud;
        const Color dark = { 26, 30, 40, 255 };
        for( int q = 0; q < 12; ++q )
        {
            const float a0 = Tau * static_cast<float>( q ) / 12.0f, a1 = Tau * static_cast<float>( q + 1 ) / 12.0f;
            const float r0 = 1.0f + 0.18f * std::sin( a0 * 3.0f ), r1 = 1.0f + 0.18f * std::sin( a1 * 3.0f );
            pud.tri( Vector3{ 0, 0.035f, 0 }, Vector3{ std::cos( a0 ) * 1.3f * r0, 0.035f, std::sin( a0 ) * 2.4f * r0 },
                     Vector3{ std::cos( a1 ) * 1.3f * r1, 0.035f, std::sin( a1 ) * 2.4f * r1 }, dark, Mat{ 0.0f, 1.0f } );
        }
        m_puddle = pud.upload();
    }
    for( int k = 0; k < 4; ++k )
    {
        if( m_policeFlash[static_cast<size_t>( k )].vaoId ) UnloadMesh( m_policeFlash[static_cast<size_t>( k )] );
        MeshBuilder flash;
        const float side = k == 0 || k == 2 ? 0.25f : -0.25f;
        const bool  isRed = k == 0 || k == 3;
        flash.box( Vector3{ side, 0.05f, 0.0f }, Vector3{ 0.26f, 0.09f, 0.16f }, isRed ? rgb( 255, 40, 30 ) : rgb( 40, 90, 255 ), Mat{ 1.0f, 0.4f } );
        flash.blob( Vector3{ side, 0.08f, 0.0f }, Vector3{ 0.34f, 0.17f, 0.3f }, isRed ? rgb( 255, 60, 40 ) : rgb( 60, 110, 255 ), 1u, 0.0f, Mat{ 0.8f, 0.0f } );
        m_policeFlash[static_cast<size_t>( k )] = flash.upload();
    }
}

int serviceModel( const Models &models, const BiomeDef &place, std::string_view prefix )
{
    // By the shapes' home lines, not the place's props: the fire engine belongs to the country even where no town is.
    const auto       &shapes = dataShapes();
    const std::string country = place.m_country ? place.m_country : "";
    const std::string region = place.m_region && !country.empty() ? country + ":" + place.m_region : std::string();
    int               found = -1, generic = -1;
    for( size_t i = 0; i < shapes.size() && i < models.m_policeModel.size(); ++i )
    {
        const int model = models.m_policeModel[i];
        if( model < 0 || shapes[i].m_name.rfind( prefix, 0 ) != 0 ) continue;
        if( shapes[i].m_name == prefix ) generic = model;
        for( const std::string &line : shapes[i].m_homes )
        {
            std::string_view list = std::string_view( line ).substr( 0, line.find( '|' ) );
            while( !list.empty() )
            {
                const size_t     comma = list.find( ',' );
                std::string_view home = list.substr( 0, comma );
                list = comma == std::string_view::npos ? std::string_view() : list.substr( comma + 1 );
                while( !home.empty() && home.front() == ' ' ) home.remove_prefix( 1 );
                while( !home.empty() && home.back() == ' ' ) home.remove_suffix( 1 );
                if( !region.empty() && home == region ) return model;   // the region's before the country's (Hong Kong's before China's)
                if( found < 0 && !country.empty() && home == country ) found = model;
            }
        }
    }
    return found >= 0 ? found : generic;
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
    if( m_livery.vaoId ) UnloadMesh( m_livery );
    m_livery = Mesh{};
    for( auto &v : m_traffic ) UnloadMesh( v.m_mesh );
    m_traffic.clear();
    UnloadMesh( m_cone );
    for( auto &m : m_arrowBoard ) UnloadMesh( m );
    UnloadMesh( m_shadow );
    for( auto &m : m_policeFlash )
        if( m.vaoId ) UnloadMesh( m );
    m_policeFlash = {};
    if( m_puddle.vaoId ) UnloadMesh( m_puddle );
    m_puddle = Mesh{};
    if( m_playerPlate.vaoId ) UnloadMesh( m_playerPlate );
    m_playerPlate = Mesh{};
    m_playerPlateOf.clear();
    for( Mesh &m : m_hitchhikers )
        if( m.vaoId ) UnloadMesh( m );
    m_hitchhikers.clear();
    for( auto &t : m_trains )
        for( Mesh &m : t )
            if( m.vaoId ) UnloadMesh( m );
    m_trains.clear();
    for( Mesh &m : m_sky )
        if( m.vaoId ) UnloadMesh( m );
    m_sky.clear();
}

} // namespace sunstreak
