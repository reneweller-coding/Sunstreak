/**
 * @file Models.h
 * @brief Every 3D model of the game, made in code: props, road furniture, the player's cars and the traffic.
 *
 * Models are built with +x to the left, +y up and +z forward. Props stay on the CPU (MeshBuilder) and are baked
 * into the world's chunks; the vehicles, cones and shadows are uploaded once and drawn per object.
 *
 * @author reneweller
 */
#pragma once

#include "Biome.h"
#include "MeshBuilder.h"

#include <raylib.h>

#include <array>
#include <cstdint>
#include <vector>

namespace sunstreak
{

constexpr int CarCount = 3;     ///< cars the player can choose
constexpr int PaintCount = 14;  ///< paints per car
constexpr int RimCount = 7;     ///< rim finishes
constexpr int DecalCount = 7;   ///< decals (0: none)

/** @brief How the player's car looks: the garage's choices. */
struct Livery
{
    int m_car = 0;          ///< which car
    int m_paint = 0;        ///< body paint
    int m_rims = 0;         ///< rim finish
    int m_decal = 0;        ///< decal
    int m_decalColor = 1;   ///< the decal's colour (a paint)

    /**
     * @brief Compares two liveries.
     * @param[in] o  the other
     * @return true if they look the same
     */
    bool operator==( const Livery &o ) const = default;
};

/** @brief A car the player can drive. */
struct CarSpec
{
    const char *m_name;       ///< shown in the menu
    float       m_topSpeed;   ///< m/s without boost
    float       m_accel;      ///< acceleration factor
    float       m_grip;       ///< steering grip factor
    float       m_length;     ///< metres
    float       m_width;      ///< metres
    int         m_cylinders;  ///< engine cylinders (sound)
    float       m_idle;       ///< idle rpm
    float       m_redline;    ///< redline rpm
    float       m_bright;     ///< engine tone brightness (1 neutral)
};

/**
 * @brief The specification of a car.
 * @param[in] index  0..CarCount-1
 * @return its data
 */
const CarSpec &carSpec( int index );

/**
 * @brief The name of a paint.
 * @param[in] index  0..PaintCount-1
 * @return its name
 */
const char *paintName( int index );

/**
 * @brief The colour of a paint.
 * @param[in] index  0..PaintCount-1
 * @return its colour
 */
Color paintColor( int index );

/**
 * @brief The name of a rim finish.
 * @param[in] index  0..RimCount-1
 * @return its name
 */
const char *rimName( int index );

/**
 * @brief The name of a decal.
 * @param[in] index  0..DecalCount-1
 * @return its name
 */
const char *decalName( int index );

/** @brief Which way a prop turns. */
enum class FacingE : uint8_t
{
    FREE,    ///< any way
    ROAD,    ///< its front (+x) towards the road
    ALONG,   ///< along the road (its -z face towards the driver)
};

/** @brief A traffic vehicle's mesh and size. */
struct VehicleModel
{
    Mesh  m_mesh{};        ///< on the GPU
    float m_length = 4;    ///< metres
    float m_width = 1.8f;  ///< metres
    bool  m_truck = false; ///< slow and long
};

/** @brief All models. */
class Models
{
public:
    /** @brief Builds every model (needs a GL context for the uploads). */
    void build();

    /** @brief Frees the GPU meshes. */
    void unload();

    /**
     * @brief A prop's triangles.
     * @param[in] kind     which prop
     * @param[in] variant  any number; picks one of the kind's variants
     * @return its model
     */
    const MeshBuilder &prop( PropKindE kind, uint64_t variant ) const;

    /**
     * @brief How big a prop is for collisions.
     * @param[in] kind  which prop
     * @return its radius at scale 1 (0: the car passes through)
     */
    float propRadius( PropKindE kind ) const { return m_radius[static_cast<size_t>( kind )]; }

    /**
     * @brief The default colours of a prop with key colours.
     * @param[in] kind  which prop
     * @return three colours, or nullptr if the kind has no key colours
     */
    const Color *keys( PropKindE kind ) const
    {
        const auto &k = m_keys[static_cast<size_t>( kind )];
        return k[0].a ? k.data() : nullptr;
    }

    /**
     * @brief Which way a prop turns.
     * @param[in] kind  which prop
     * @return its facing
     */
    FacingE facing( PropKindE kind ) const { return m_facing[static_cast<size_t>( kind )]; }

    /**
     * @brief Builds the player's car as the garage dressed it (again only when the livery changed).
     * @param[in] livery  car, paint, rims and decal
     */
    void buildLivery( const Livery &livery );

    MeshBuilder m_post;           ///< delineator post
    MeshBuilder m_chevronRight;   ///< warning board, arrow to the driver's right
    MeshBuilder m_chevronLeft;    ///< warning board, arrow to the driver's left
    MeshBuilder m_lamp;           ///< street lamp, its arm along +z
    MeshBuilder m_jumpSign;       ///< crest warning

    std::array<std::array<Mesh, PaintCount>, CarCount> m_playerCars{};   ///< the player's cars in plain paint (rivals, ghosts)
    Mesh                                               m_livery{};       ///< the player's own car, dressed in the garage
    Livery                                             m_liveryOf{};     ///< what m_livery shows
    std::vector<VehicleModel>                          m_traffic;        ///< traffic, paints included
    Mesh                                               m_cone{};         ///< traffic cone
    std::array<Mesh, 2>                                m_arrowBoard{};   ///< roadworks board, arrow left / right
    Mesh                                               m_shadow{};       ///< unit blob shadow

private:
    /** @brief Builds the props. */
    void buildProps();

    /** @brief Builds the vehicles. */
    void buildVehicles();

    /** @brief Builds the world tour's props (ModelsWorld.cpp). */
    void buildWorldProps();

    std::array<std::vector<MeshBuilder>, static_cast<size_t>( PropKindE::COUNT )> m_props;    ///< variants per kind
    std::array<float, static_cast<size_t>( PropKindE::COUNT )>                    m_radius{};  ///< collision radii
    std::array<std::array<Color, 3>, static_cast<size_t>( PropKindE::COUNT )>     m_keys{};    ///< default key colours
    std::array<FacingE, static_cast<size_t>( PropKindE::COUNT )>                  m_facing{};  ///< which way they turn
};

} // namespace sunstreak
