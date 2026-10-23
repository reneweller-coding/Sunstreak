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
constexpr int PaintCount = 4;   ///< paints per car

/** @brief A car the player can drive. */
struct CarSpec
{
    const char *m_name;       ///< shown in the menu
    float       m_topSpeed;   ///< m/s without boost
    float       m_accel;      ///< acceleration factor
    float       m_grip;       ///< steering grip factor
    float       m_length;     ///< metres
    float       m_width;      ///< metres
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

    MeshBuilder m_post;           ///< delineator post
    MeshBuilder m_chevronRight;   ///< warning board, arrow to the driver's right
    MeshBuilder m_chevronLeft;    ///< warning board, arrow to the driver's left
    MeshBuilder m_lamp;           ///< street lamp, its arm along +z
    MeshBuilder m_jumpSign;       ///< crest warning

    std::array<std::array<Mesh, PaintCount>, CarCount> m_playerCars{};   ///< the player's cars
    std::vector<VehicleModel>                          m_traffic;        ///< traffic, paints included
    Mesh                                               m_cone{};         ///< traffic cone
    std::array<Mesh, 2>                                m_arrowBoard{};   ///< roadworks board, arrow left / right
    Mesh                                               m_shadow{};       ///< unit blob shadow

private:
    /** @brief Builds the props. */
    void buildProps();

    /** @brief Builds the vehicles. */
    void buildVehicles();

    std::array<std::vector<MeshBuilder>, static_cast<size_t>( PropKindE::COUNT )> m_props;    ///< variants per kind
    std::array<float, static_cast<size_t>( PropKindE::COUNT )>                    m_radius{};  ///< collision radii
};

} // namespace sunstreak
