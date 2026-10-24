/**
 * @file MeshBuilder.h
 * @brief Collects flat-shaded, vertex-coloured triangles on the CPU and uploads them as one raylib mesh.
 *
 * Every triangle gets its own three vertices and its face normal: the faceted low-poly look. The texture coordinates
 * carry material values instead of a texture: u = emissive (0..1, glows and pierces the fog), v = gloss (0..1).
 * The second set carries u = shade (0 in a tunnel's dark, 1 in the open) and v = sway (how far the wind moves the
 * vertex: tree tops, reeds).
 * The world shader lights both faces, so the winding of a triangle does not matter.
 *
 * @author reneweller
 */
#pragma once

#include <raylib.h>

#include <vector>

namespace sunstreak
{

/**
 * @brief Key colour one: append() replaces it by the first of its keys, scaled by k (a shade of the same paint).
 * @param[in] k  0.01..1
 * @return the marker
 */
inline Color keyA( float k = 1.0f )
{
    const auto v = static_cast<unsigned char>( 2.0f + 248.0f * k );
    return Color{ v, 1, v, 255 };
}

/**
 * @brief Key colour two (see keyA()).
 * @param[in] k  0.01..1
 * @return the marker
 */
inline Color keyB( float k = 1.0f )
{
    const auto v = static_cast<unsigned char>( 2.0f + 248.0f * k );
    return Color{ 1, v, v, 255 };
}

/**
 * @brief Key colour three (see keyA()), used for windows.
 * @param[in] k  0.01..1
 * @return the marker
 */
inline Color keyC( float k = 1.0f )
{
    const auto v = static_cast<unsigned char>( 2.0f + 248.0f * k );
    return Color{ v, v, 1, 255 };
}

/** @brief Material values of a triangle. */
struct Mat
{
    float m_emissive = 0.0f;   ///< 0..1 self-lit
    float m_gloss = 0.0f;      ///< 0..1 shiny
};

/** @brief What a season does to the models it reaches (Season.h decides it per place). */
struct SeasonLook
{
    float m_autumn = 0.0f;   ///< 0..1 the leaves turned
    float m_winter = 0.0f;   ///< 0..1 snow lying
    float m_bare = 0.0f;     ///< 0..1 the leaves fallen
    float m_spring = 0.0f;   ///< 0..1 fresh green and blossom
};

/** @brief A growing list of triangles. */
class MeshBuilder
{
public:
    /**
     * @brief Adds a triangle.
     * @param[in] a    first corner
     * @param[in] b    second corner
     * @param[in] c    third corner
     * @param[in] col  its colour
     * @param[in] mat  its material
     */
    void tri( Vector3 a, Vector3 b, Vector3 c, Color col, Mat mat = {} );

    /**
     * @brief Adds a triangle with a colour per corner (soft shadows, gradients).
     * @param[in] a    first corner
     * @param[in] b    second corner
     * @param[in] c    third corner
     * @param[in] ca   colour at a
     * @param[in] cb   colour at b
     * @param[in] cc   colour at c
     * @param[in] mat  its material
     */
    void tri3( Vector3 a, Vector3 b, Vector3 c, Color ca, Color cb, Color cc, Mat mat = {} );

    /**
     * @brief Adds a quad with a colour and a material per corner (light pools on the road).
     * @param[in] p    the four corners
     * @param[in] col  their colours
     * @param[in] mat  their materials
     */
    void quadV( const Vector3 p[4], const Color col[4], const Mat mat[4] );

    /**
     * @brief Adds a quad as two triangles (a-b-c and a-c-d).
     * @param[in] a    first corner
     * @param[in] b    second corner
     * @param[in] c    third corner
     * @param[in] d    fourth corner
     * @param[in] col  its colour
     * @param[in] mat  its material
     */
    void quad( Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color col, Mat mat = {} );

    /**
     * @brief Adds an axis-aligned box.
     * @param[in] center  its centre
     * @param[in] half    half its size per axis
     * @param[in] col     its colour
     * @param[in] mat     its material
     */
    void box( Vector3 center, Vector3 half, Color col, Mat mat = {} );

    /**
     * @brief Adds a box whose top is narrower or shifted (a frustum of four sides).
     * @param[in] bottom     centre of the bottom face
     * @param[in] halfBottom half the bottom's size (x, z)
     * @param[in] top        centre of the top face
     * @param[in] halfTop    half the top's size (x, z)
     * @param[in] col        its colour
     * @param[in] mat        its material
     * @param[in] caps       add bottom and top faces
     */
    void taperedBox( Vector3 bottom, Vector2 halfBottom, Vector3 top, Vector2 halfTop, Color col, Mat mat = {},
                     bool caps = true );

    /**
     * @brief Adds a vertical cylinder or cone (radiusTop 0) with a number of sides.
     * @param[in] base       centre of the bottom
     * @param[in] radiusBot  radius at the bottom
     * @param[in] radiusTop  radius at the top
     * @param[in] height     height
     * @param[in] sides      number of sides
     * @param[in] col        its colour
     * @param[in] mat        its material
     * @param[in] phase      rotation of the first corner (rad)
     */
    void cylinder( Vector3 base, float radiusBot, float radiusTop, float height, int sides, Color col, Mat mat = {},
                   float phase = 0.0f );

    /**
     * @brief Adds a cylinder between two arbitrary points (a trunk, a pole, a wheel).
     * @param[in] a       centre of one end
     * @param[in] b       centre of the other
     * @param[in] radius  its radius
     * @param[in] sides   number of sides
     * @param[in] col     its colour
     * @param[in] mat     its material
     * @param[in] caps    close the ends
     */
    void tube( Vector3 a, Vector3 b, float radius, int sides, Color col, Mat mat = {}, bool caps = false );

    /**
     * @brief Adds a lumpy ball: an icosahedron with its corners pushed in and out.
     * @param[in] center   its centre
     * @param[in] radius   its size (x, y, z)
     * @param[in] col      its colour
     * @param[in] seed     the shape's seed
     * @param[in] lumpy    0..1 how far corners move
     * @param[in] mat      its material
     */
    void blob( Vector3 center, Vector3 radius, Color col, unsigned seed, float lumpy, Mat mat = {} );

    /**
     * @brief Appends another builder's triangles, rotated about y, scaled and moved.
     * @param[in] src     the triangles
     * @param[in] pos     where src's origin goes
     * @param[in] yaw     rotation about y (rad, the model's +z towards headingForward(yaw))
     * @param[in] scale   uniform scale
     * @param[in] bright  colour multiplier (variation)
     * @param[in] keys    three colours that replace the key colours (keyA/B/C), or nullptr
     * @param[in] season  the season to colour it for (leaves, snow on whatever faces up, blossom), or nullptr
     * @param[in] pick    0..1 which autumn colour or blossom this one takes
     */
    void append( const MeshBuilder &src, Vector3 pos, float yaw, float scale, float bright = 1.0f,
                 const Color *keys = nullptr, const SeasonLook *season = nullptr, float pick = 0.0f );

    /**
     * @brief Appends another builder's triangles through a full basis.
     * @param[in] src      the triangles
     * @param[in] left     where src's +x goes
     * @param[in] up       where its +y goes
     * @param[in] forward  where its +z goes
     * @param[in] pos      where its origin goes
     */
    void appendBasis( const MeshBuilder &src, Vector3 left, Vector3 up, Vector3 forward, Vector3 pos );

    /**
     * @brief Uploads the triangles to the GPU.
     * @return the mesh (vertexCount 0 if empty); the caller unloads it
     */
    Mesh upload() const;

    /**
     * @brief Number of triangles so far.
     * @return the count
     */
    size_t triangleCount() const { return m_pos.size() / 9; }

    /** @brief Forgets all triangles. */
    void clear();

    /**
     * @brief How far the triangles reach from the origin along each axis.
     * @return the largest |x|, |y|, |z|
     */
    Vector3 extent() const;

    /**
     * @brief Sets the shade of the triangles that follow (and of appended ones).
     * @param[in] shade  1 open sky, 0 deep in a tunnel
     */
    void setShade( float shade ) { m_shade = shade; }

    /**
     * @brief Lets the wind move the vertices, more the higher they are (call on a finished prop).
     * @param[in] amount  sway at the top
     */
    void applySway( float amount );

private:
    std::vector<float>         m_pos;   ///< xyz per vertex
    std::vector<float>         m_nrm;   ///< normal per vertex
    std::vector<unsigned char> m_col;   ///< rgba per vertex
    std::vector<float>         m_uv;    ///< emissive, gloss per vertex
    std::vector<float>         m_uv2;   ///< shade, sway per vertex
    float                      m_shade = 1.0f;   ///< shade for new vertices
};

} // namespace sunstreak
