/**
 * @file MeshBuilder.cpp
 * @brief Triangle collection and upload.
 * @author reneweller
 */
#include "MeshBuilder.h"

#include "MathUtil.h"
#include "Rng.h"

#include <cstring>

namespace sunstreak
{

void MeshBuilder::tri( Vector3 a, Vector3 b, Vector3 c, Color col, Mat mat )
{
    Vector3     n = Vector3CrossProduct( Vector3Subtract( b, a ), Vector3Subtract( c, a ) );
    const float len = Vector3Length( n );
    if( len < 1e-9f ) return;
    n = Vector3Scale( n, 1.0f / len );
    for( const Vector3 &p : { a, b, c } )
    {
        m_pos.insert( m_pos.end(), { p.x, p.y, p.z } );
        m_nrm.insert( m_nrm.end(), { n.x, n.y, n.z } );
        m_col.insert( m_col.end(), { col.r, col.g, col.b, col.a } );
        m_uv.insert( m_uv.end(), { mat.m_emissive, mat.m_gloss } );
        m_uv2.insert( m_uv2.end(), { m_shade, 0.0f } );
    }
}

void MeshBuilder::tri3( Vector3 a, Vector3 b, Vector3 c, Color ca, Color cb, Color cc, Mat mat )
{
    Vector3     n = Vector3CrossProduct( Vector3Subtract( b, a ), Vector3Subtract( c, a ) );
    const float len = Vector3Length( n );
    if( len < 1e-9f ) return;
    n = Vector3Scale( n, 1.0f / len );
    const Vector3 p[3] = { a, b, c };
    const Color   k[3] = { ca, cb, cc };
    for( int i = 0; i < 3; ++i )
    {
        m_pos.insert( m_pos.end(), { p[i].x, p[i].y, p[i].z } );
        m_nrm.insert( m_nrm.end(), { n.x, n.y, n.z } );
        m_col.insert( m_col.end(), { k[i].r, k[i].g, k[i].b, k[i].a } );
        m_uv.insert( m_uv.end(), { mat.m_emissive, mat.m_gloss } );
        m_uv2.insert( m_uv2.end(), { m_shade, 0.0f } );
    }
}

void MeshBuilder::quadV( const Vector3 p[4], const Color col[4], const Mat mat[4] )
{
    static const int order[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } };
    for( const auto &t : order )
    {
        Vector3 n = Vector3CrossProduct( Vector3Subtract( p[t[1]], p[t[0]] ), Vector3Subtract( p[t[2]], p[t[0]] ) );
        const float len = Vector3Length( n );
        if( len < 1e-9f ) continue;
        n = Vector3Scale( n, 1.0f / len );
        for( int k : t )
        {
            m_pos.insert( m_pos.end(), { p[k].x, p[k].y, p[k].z } );
            m_nrm.insert( m_nrm.end(), { n.x, n.y, n.z } );
            m_col.insert( m_col.end(), { col[k].r, col[k].g, col[k].b, col[k].a } );
            m_uv.insert( m_uv.end(), { mat[k].m_emissive, mat[k].m_gloss } );
            m_uv2.insert( m_uv2.end(), { m_shade, 0.0f } );
        }
    }
}

void MeshBuilder::quad( Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color col, Mat mat )
{
    tri( a, b, c, col, mat );
    tri( a, c, d, col, mat );
}

void MeshBuilder::box( Vector3 center, Vector3 half, Color col, Mat mat )
{
    taperedBox( Vector3{ center.x, center.y - half.y, center.z }, Vector2{ half.x, half.z },
                Vector3{ center.x, center.y + half.y, center.z }, Vector2{ half.x, half.z }, col, mat, true );
}

void MeshBuilder::taperedBox( Vector3 bottom, Vector2 halfBottom, Vector3 top, Vector2 halfTop, Color col, Mat mat,
                              bool caps )
{
    const Vector3 b[4] = { { bottom.x - halfBottom.x, bottom.y, bottom.z - halfBottom.y },
                           { bottom.x + halfBottom.x, bottom.y, bottom.z - halfBottom.y },
                           { bottom.x + halfBottom.x, bottom.y, bottom.z + halfBottom.y },
                           { bottom.x - halfBottom.x, bottom.y, bottom.z + halfBottom.y } };
    const Vector3 t[4] = { { top.x - halfTop.x, top.y, top.z - halfTop.y },
                           { top.x + halfTop.x, top.y, top.z - halfTop.y },
                           { top.x + halfTop.x, top.y, top.z + halfTop.y },
                           { top.x - halfTop.x, top.y, top.z + halfTop.y } };
    for( int i = 0; i < 4; ++i )
    {
        const int j = ( i + 1 ) % 4;
        quad( b[i], b[j], t[j], t[i], col, mat );
    }
    if( caps )
    {
        quad( t[0], t[1], t[2], t[3], col, mat );
        quad( b[3], b[2], b[1], b[0], col, mat );
    }
}

void MeshBuilder::cylinder( Vector3 base, float radiusBot, float radiusTop, float height, int sides, Color col, Mat mat,
                            float phase )
{
    const Vector3 apex = { base.x, base.y + height, base.z };
    for( int i = 0; i < sides; ++i )
    {
        const float   a0 = phase + Tau * static_cast<float>( i ) / static_cast<float>( sides );
        const float   a1 = phase + Tau * static_cast<float>( i + 1 ) / static_cast<float>( sides );
        const Vector3 b0 = { base.x + std::cos( a0 ) * radiusBot, base.y, base.z + std::sin( a0 ) * radiusBot };
        const Vector3 b1 = { base.x + std::cos( a1 ) * radiusBot, base.y, base.z + std::sin( a1 ) * radiusBot };
        if( radiusTop <= 0.0f )
        {
            tri( b0, b1, apex, col, mat );
        }
        else
        {
            const Vector3 t0 = { base.x + std::cos( a0 ) * radiusTop, apex.y, base.z + std::sin( a0 ) * radiusTop };
            const Vector3 t1 = { base.x + std::cos( a1 ) * radiusTop, apex.y, base.z + std::sin( a1 ) * radiusTop };
            quad( b0, b1, t1, t0, col, mat );
            tri( apex, t0, t1, col, mat );
        }
    }
}

void MeshBuilder::tube( Vector3 a, Vector3 b, float radius, int sides, Color col, Mat mat, bool caps )
{
    const Vector3 axis = Vector3Normalize( Vector3Subtract( b, a ) );
    const Vector3 ref = std::abs( axis.y ) < 0.9f ? Vector3{ 0, 1, 0 } : Vector3{ 1, 0, 0 };
    const Vector3 u = Vector3Normalize( Vector3CrossProduct( axis, ref ) );
    const Vector3 v = Vector3CrossProduct( axis, u );
    auto ring = [&]( Vector3 c, float ang )
    {
        return Vector3Add( c, Vector3Add( Vector3Scale( u, std::cos( ang ) * radius ), Vector3Scale( v, std::sin( ang ) * radius ) ) );
    };
    for( int i = 0; i < sides; ++i )
    {
        const float a0 = Tau * static_cast<float>( i ) / static_cast<float>( sides );
        const float a1 = Tau * static_cast<float>( i + 1 ) / static_cast<float>( sides );
        quad( ring( a, a0 ), ring( a, a1 ), ring( b, a1 ), ring( b, a0 ), col, mat );
        if( caps )
        {
            tri( a, ring( a, a1 ), ring( a, a0 ), col, mat );
            tri( b, ring( b, a0 ), ring( b, a1 ), col, mat );
        }
    }
}

void MeshBuilder::blob( Vector3 center, Vector3 radius, Color col, unsigned seed, float lumpy, Mat mat )
{
    const float t = 1.6180339887f;
    Vector3     v[12] = { { -1, t, 0 }, { 1, t, 0 }, { -1, -t, 0 }, { 1, -t, 0 }, { 0, -1, t }, { 0, 1, t },
                          { 0, -1, -t }, { 0, 1, -t }, { t, 0, -1 }, { t, 0, 1 }, { -t, 0, -1 }, { -t, 0, 1 } };
    static const int f[20][3] = { { 0, 11, 5 }, { 0, 5, 1 },  { 0, 1, 7 },   { 0, 7, 10 }, { 0, 10, 11 },
                                  { 1, 5, 9 },  { 5, 11, 4 }, { 11, 10, 2 }, { 10, 7, 6 }, { 7, 1, 8 },
                                  { 3, 9, 4 },  { 3, 4, 2 },  { 3, 2, 6 },   { 3, 6, 8 },  { 3, 8, 9 },
                                  { 4, 9, 5 },  { 2, 4, 11 }, { 6, 2, 10 },  { 8, 6, 7 },  { 9, 8, 1 } };
    for( int i = 0; i < 12; ++i )
    {
        const Vector3 n = Vector3Normalize( v[i] );
        const float   k = 1.0f + lumpy * ( hashFloat( seed, static_cast<uint64_t>( i ) ) - 0.5f );
        v[i] = Vector3{ center.x + n.x * radius.x * k, center.y + n.y * radius.y * k, center.z + n.z * radius.z * k };
    }
    for( const auto &face : f )
    {
        // Small shade variations per face keep the facets readable.
        const float shade = 0.92f + 0.16f * hashFloat( seed + 77, static_cast<uint64_t>( &face - f ) );
        // A key colour keeps its marker channel at 1 and only changes its level.
        Color c = scaleColor( col, shade );
        if( col.r == 1 || col.g == 1 || col.b == 1 )
        {
            c.r = col.r == 1 ? 1 : std::max<unsigned char>( 2, c.r );
            c.g = col.g == 1 ? 1 : std::max<unsigned char>( 2, c.g );
            c.b = col.b == 1 ? 1 : std::max<unsigned char>( 2, c.b );
        }
        tri( v[face[0]], v[face[1]], v[face[2]], c, mat );
    }
}

void MeshBuilder::append( const MeshBuilder &src, Vector3 pos, float yaw, float scale, float bright, const Color *keys,
                          const SeasonLook *season, float pick )
{
    static const Color autumn[4] = { { 214, 120, 40, 255 }, { 190, 62, 40, 255 }, { 226, 182, 60, 255 }, { 150, 100, 52, 255 } };
    const Color        turned = autumn[static_cast<int>( pick * 4.0f ) & 3];
    const Color        blossom = pick < 0.1f ? Color{ 246, 240, 236, 255 } : Color{ 246, 186, 208, 255 };
    const Vector3 fwd = headingForward( yaw );
    const Vector3 left = Vector3Negate( headingRight( yaw ) );
    const size_t  n = src.m_pos.size() / 3;
    m_pos.reserve( m_pos.size() + n * 3 );
    for( size_t i = 0; i < n; ++i )
    {
        const float x = src.m_pos[i * 3], y = src.m_pos[i * 3 + 1], z = src.m_pos[i * 3 + 2];
        m_pos.insert( m_pos.end(), { pos.x + ( left.x * x + fwd.x * z ) * scale, pos.y + y * scale,
                                     pos.z + ( left.z * x + fwd.z * z ) * scale } );
        const float nx = src.m_nrm[i * 3], ny = src.m_nrm[i * 3 + 1], nz = src.m_nrm[i * 3 + 2];
        m_nrm.insert( m_nrm.end(), { left.x * nx + fwd.x * nz, ny, left.z * nx + fwd.z * nz } );
        const unsigned char *s = &src.m_col[i * 4];
        float                rgb[3] = { static_cast<float>( s[0] ), static_cast<float>( s[1] ), static_cast<float>( s[2] ) };
        if( keys )
        {
            // Key colours: one channel at 1, the other two equal; their level is the shade.
            int which = -1;
            if( s[1] == 1 && s[0] == s[2] && s[0] > 1 ) which = 0;
            else if( s[0] == 1 && s[1] == s[2] && s[1] > 1 ) which = 1;
            else if( s[2] == 1 && s[0] == s[1] && s[0] > 1 ) which = 2;
            if( which >= 0 )
            {
                const float k = static_cast<float>( which == 1 ? s[1] : s[0] ) / 250.0f;
                rgb[0] = keys[which].r * k;
                rgb[1] = keys[which].g * k;
                rgb[2] = keys[which].b * k;
            }
        }
        if( season )
        {
            // Leaves (green, but not the dark green of conifers) turn, fall or blossom; snow lies on whatever faces up.
            auto       toward = [&]( Color c, float k )
            {
                rgb[0] += ( c.r - rgb[0] ) * k;
                rgb[1] += ( c.g - rgb[1] ) * k;
                rgb[2] += ( c.b - rgb[2] ) * k;
            };
            const bool leafy = rgb[1] > rgb[0] * 1.08f && rgb[1] > rgb[2] * 1.05f && !( rgb[1] < 112.0f && rgb[0] < 72.0f );
            if( leafy )
            {
                if( season->m_autumn > 0.0f ) toward( turned, 0.85f * season->m_autumn );
                if( season->m_spring > 0.0f ) toward( pick < 0.3f ? blossom : Color{ 120, 200, 90, 255 }, ( pick < 0.3f ? 0.85f : 0.35f ) * season->m_spring );
                if( season->m_bare > 0.0f ) toward( Color{ 112, 100, 88, 255 }, 0.65f * season->m_bare );
            }
            if( season->m_winter > 0.0f ) toward( Color{ 236, 240, 248, 255 }, 0.9f * season->m_winter * smoothstepf( 0.45f, 0.85f, ny ) );
        }
        for( float v : rgb ) m_col.push_back( static_cast<unsigned char>( clampf( v * bright, 0.0f, 255.0f ) ) );
        m_col.push_back( s[3] );
        m_uv.insert( m_uv.end(), { src.m_uv[i * 2], src.m_uv[i * 2 + 1] } );
        m_uv2.insert( m_uv2.end(), { src.m_uv2[i * 2] * m_shade, src.m_uv2[i * 2 + 1] * scale } );
    }
}

void MeshBuilder::appendBasis( const MeshBuilder &src, Vector3 left, Vector3 up, Vector3 forward, Vector3 pos )
{
    const size_t n = src.m_pos.size() / 3;
    for( size_t i = 0; i < n; ++i )
    {
        const float x = src.m_pos[i * 3], y = src.m_pos[i * 3 + 1], z = src.m_pos[i * 3 + 2];
        m_pos.insert( m_pos.end(), { pos.x + left.x * x + up.x * y + forward.x * z,
                                     pos.y + left.y * x + up.y * y + forward.y * z,
                                     pos.z + left.z * x + up.z * y + forward.z * z } );
        const float nx = src.m_nrm[i * 3], ny = src.m_nrm[i * 3 + 1], nz = src.m_nrm[i * 3 + 2];
        m_nrm.insert( m_nrm.end(), { left.x * nx + up.x * ny + forward.x * nz, left.y * nx + up.y * ny + forward.y * nz,
                                     left.z * nx + up.z * ny + forward.z * nz } );
        m_col.insert( m_col.end(), src.m_col.begin() + static_cast<long>( i * 4 ), src.m_col.begin() + static_cast<long>( i * 4 + 4 ) );
        m_uv.insert( m_uv.end(), { src.m_uv[i * 2], src.m_uv[i * 2 + 1] } );
        m_uv2.insert( m_uv2.end(), { src.m_uv2[i * 2] * m_shade, src.m_uv2[i * 2 + 1] } );
    }
}

Mesh MeshBuilder::upload() const
{
    Mesh mesh = { 0 };
    const int n = static_cast<int>( m_pos.size() / 3 );
    if( n == 0 ) return mesh;
    mesh.vertexCount = n;
    mesh.triangleCount = n / 3;
    mesh.vertices = static_cast<float *>( MemAlloc( static_cast<unsigned>( m_pos.size() * sizeof( float ) ) ) );
    mesh.normals = static_cast<float *>( MemAlloc( static_cast<unsigned>( m_nrm.size() * sizeof( float ) ) ) );
    mesh.texcoords = static_cast<float *>( MemAlloc( static_cast<unsigned>( m_uv.size() * sizeof( float ) ) ) );
    mesh.colors = static_cast<unsigned char *>( MemAlloc( static_cast<unsigned>( m_col.size() ) ) );
    mesh.texcoords2 = static_cast<float *>( MemAlloc( static_cast<unsigned>( m_uv2.size() * sizeof( float ) ) ) );
    std::memcpy( mesh.texcoords2, m_uv2.data(), m_uv2.size() * sizeof( float ) );
    std::memcpy( mesh.vertices, m_pos.data(), m_pos.size() * sizeof( float ) );
    std::memcpy( mesh.normals, m_nrm.data(), m_nrm.size() * sizeof( float ) );
    std::memcpy( mesh.texcoords, m_uv.data(), m_uv.size() * sizeof( float ) );
    std::memcpy( mesh.colors, m_col.data(), m_col.size() );
    UploadMesh( &mesh, false );
    return mesh;
}

void MeshBuilder::clear()
{
    m_pos.clear();
    m_nrm.clear();
    m_col.clear();
    m_uv.clear();
    m_uv2.clear();
}

Vector3 MeshBuilder::extent() const
{
    Vector3 e = { 0, 0, 0 };
    for( size_t i = 0; i + 2 < m_pos.size(); i += 3 )
    {
        e.x = std::max( e.x, std::abs( m_pos[i] ) );
        e.y = std::max( e.y, std::abs( m_pos[i + 1] ) );
        e.z = std::max( e.z, std::abs( m_pos[i + 2] ) );
    }
    return e;
}

bool MeshBuilder::centreOf( bool ( *match )( Color ), Vector3 &at ) const
{
    Vector3 sum = { 0, 0, 0 };
    float   n = 0.0f;
    for( size_t v = 0; v * 3 + 2 < m_pos.size() && v * 4 + 3 < m_col.size(); ++v )
    {
        const Color c = { m_col[v * 4], m_col[v * 4 + 1], m_col[v * 4 + 2], m_col[v * 4 + 3] };
        if( !match( c ) ) continue;
        sum = Vector3{ sum.x + m_pos[v * 3], sum.y + m_pos[v * 3 + 1], sum.z + m_pos[v * 3 + 2] };
        n += 1.0f;
    }
    if( n <= 0.0f ) return false;
    at = Vector3{ sum.x / n, sum.y / n, sum.z / n };
    return true;
}

MeshBuilder MeshBuilder::simplified( float minEdge ) const
{
    MeshBuilder out;
    const float min2 = minEdge * minEdge;
    for( size_t t = 0; t < triangleCount(); ++t )
    {
        const float *p = &m_pos[t * 9];
        auto         d2 = [&]( int a, int b )
        {
            const float dx = p[a * 3] - p[b * 3], dy = p[a * 3 + 1] - p[b * 3 + 1], dz = p[a * 3 + 2] - p[b * 3 + 2];
            return dx * dx + dy * dy + dz * dz;
        };
        if( std::max( { d2( 0, 1 ), d2( 1, 2 ), d2( 2, 0 ) } ) < min2 && m_uv[t * 6] < 0.3f ) continue;
        out.m_pos.insert( out.m_pos.end(), p, p + 9 );
        out.m_nrm.insert( out.m_nrm.end(), m_nrm.begin() + static_cast<long>( t * 9 ), m_nrm.begin() + static_cast<long>( t * 9 + 9 ) );
        out.m_col.insert( out.m_col.end(), m_col.begin() + static_cast<long>( t * 12 ), m_col.begin() + static_cast<long>( t * 12 + 12 ) );
        out.m_uv.insert( out.m_uv.end(), m_uv.begin() + static_cast<long>( t * 6 ), m_uv.begin() + static_cast<long>( t * 6 + 6 ) );
        out.m_uv2.insert( out.m_uv2.end(), m_uv2.begin() + static_cast<long>( t * 6 ), m_uv2.begin() + static_cast<long>( t * 6 + 6 ) );
    }
    return out;
}

void MeshBuilder::applySway( float amount )
{
    float top = 0.1f;
    for( size_t i = 1; i < m_pos.size(); i += 3 ) top = std::max( top, m_pos[i] );
    for( size_t v = 0; v < m_uv2.size() / 2; ++v )
    {
        const float t = clampf( m_pos[v * 3 + 1] / top, 0.0f, 1.0f );
        m_uv2[v * 2 + 1] = amount * t * t;
    }
}

} // namespace sunstreak
