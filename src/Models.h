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

#include "Local.h"
#include "Biome.h"
#include "MeshBuilder.h"

#include <raylib.h>

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sunstreak
{

class Models;

/**
 * @brief The vehicle of a service that drives in a place: the one whose home is the region, else the country's, else the
 *        shape named prefix itself (POLICE_CAR, AMBULANCE, FIRE_ENGINE).
 * @param[in] models  the models (Models::m_policeModel)
 * @param[in] place   the place
 * @param[in] prefix  the shapes' name prefix
 * @return its index in Models::m_traffic (-1: none)
 */
int serviceModel( const Models &models, const BiomeDef &place, std::string_view prefix );

/**
 * @brief Which flash of Models::m_policeFlash a light bar shows.
 * @param[in] lamps  VehicleModel::m_lamps
 * @param[in] phase  0 or 1, alternating
 * @return the index: red and blue alternate, one colour flashes left and right
 */
inline int flashMesh( uint8_t lamps, int phase )
{
    if( lamps == 2 ) return phase ? 1 : 2;
    if( lamps == 1 ) return phase ? 3 : 0;
    return phase ? 1 : 0;
}

constexpr int CarCount = 4;     ///< cars the player can choose
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
    float       m_dirt;       ///< grip on gravel and sand against asphalt
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
    Mesh      m_mesh{};                     ///< on the GPU
    float     m_length = 4;                 ///< metres
    float     m_width = 1.8f;               ///< metres
    bool      m_truck = false;              ///< slow and long
    uint32_t  m_fleets = 0;                 ///< the regions it belongs to (a bit per FleetE; 0: everywhere)
    uint8_t   m_where = 0;                  ///< 0 anywhere, 1 in cities, 2 in the country
    float     m_pace = 1.0f;                ///< its speed against an ordinary car
    PropKindE m_animal = PropKindE::COUNT;  ///< an animal crossing the road (COUNT: a vehicle)
    bool      m_police = false;             ///< a patrol car, an ambulance or a fire engine (never spawned as everyday traffic)
    Vector3   m_light{};                    ///< the middle of its light bar
    uint8_t   m_lamps = 0;                  ///< its lamps: 1 red, 2 blue, 3 both
};

/**
 * @brief Builds a place-name sign on two posts, its face towards -z (the driver, with the road's heading as yaw).
 * @param[out] out    where the triangles go
 * @param[in]  text   the name (upper case)
 * @param[in]  style  0 white, 1 yellow, 2 white with a red border, 3 blue, 4 green (CountryInfo::m_townSign)
 * @param[in]  cap    letter height (m)
 * @param[in]  lift   height of the board's lower edge (m)
 */
void buildPlaceSign( MeshBuilder &out, std::string_view text, int style, float cap = 0.42f, float lift = 1.4f, bool posts = true );

/**
 * @brief The gantry before a fork of the world tour: a green board over each carriageway with where it leads.
 * @param[out] out        where the triangles go (+x is the driver's left, the boards face -z)
 * @param[in]  left       where the left carriageway leads
 * @param[in]  right      where the right one leads
 * @param[in]  halfWidth  half the road's paved width
 */
void buildForkSign( MeshBuilder &out, std::string_view left, std::string_view right, float halfWidth );

/**
 * @brief Builds a speed limit sign on its post, its face towards -z: a red ring, or a white board (North America).
 * @param[out] out    where the triangles go
 * @param[in]  limit  the number
 * @param[in]  board  a white board with SPEED LIMIT over the number
 */
void buildSpeedSign( MeshBuilder &out, int limit, bool board );

/** @brief All models. */
struct DataShape;

/**
 * @brief Builds a landmark of data/landmarks from its lines (ModelsWorld.cpp; the language is described there).
 * @param[out] m      where the triangles go
 * @param[in]  shape  the shape
 * @return how many lines it got wrong
 */
int buildDataShape( MeshBuilder &m, const DataShape &shape );

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
    std::array<MeshBuilder, 3> m_chevronRight;   ///< warning board, arrow to the driver's right (CountryInfo::m_chevrons)
    std::array<MeshBuilder, 3> m_chevronLeft;    ///< warning board, arrow to the driver's left
    MeshBuilder                m_lamp;           ///< street lamp, its arm along +z
    std::array<MeshBuilder, 4> m_jumpSign;       ///< crest warning by CountryInfo::m_warn: red triangle, on yellow, yellow diamond, black on yellow
    std::vector<MeshBuilder>   m_flags;          ///< a flag on its pole per country (countryAt order), facing -z
    MeshBuilder                m_borderPost;     ///< customs booth and raised barrier, front (+x) to the road

    std::array<std::array<Mesh, PaintCount>, CarCount> m_playerCars{};   ///< the player's cars in plain paint (rivals, ghosts)
    Mesh                                               m_livery{};       ///< the player's own car, dressed in the garage
    Livery                                             m_liveryOf{};     ///< what m_livery shows
    std::vector<VehicleModel>                          m_traffic;        ///< traffic, paints included
    std::vector<int>                                   m_creatureModel;  ///< per data shape: its traffic model if it swims (-1: none)
    size_t                                             m_creatureFirst = 0;   ///< the first creature in m_traffic (they come last)
    std::vector<int>                                   m_policeModel;    ///< per data shape: its model if a patrol car, ambulance or fire engine (-1: none)
    std::array<Mesh, 4>                                m_policeFlash{};  ///< a flash of the light bar: red right, blue left, blue right, red left
    Mesh                                               m_cone{};         ///< traffic cone
    std::array<Mesh, 2>                                m_arrowBoard{};   ///< roadworks board, arrow left / right
    Mesh                                               m_shadow{};       ///< unit blob shadow
    std::array<Mesh, 7>                                m_plates{};       ///< number plates by style (PlateE), facing -z
    MeshBuilder                                        m_camera;         ///< a speed camera on its post, facing -z

private:
    /** @brief Builds the props. */
    void buildProps();

    /** @brief Builds the vehicles. */
    void buildVehicles();

    /** @brief Builds the world tour's props (ModelsWorld.cpp). */
    void buildWorldProps();

    /** @brief Appends the landmarks of the data files to the compiled ones. */
    void buildDataLandmarks();

public:
    /** @brief Builds the shapes of the data files and the flags again, after reloadData(). */
    void reloadDataShapes();

private:

    /** @brief Builds the traffic models of what swims (the creatures of the data files). */
    void buildCreatures();

    /** @brief Builds a flag per country of the data files. */
    void buildFlags();

    std::array<std::vector<MeshBuilder>, static_cast<size_t>( PropKindE::COUNT )> m_props;    ///< variants per kind
    std::array<float, static_cast<size_t>( PropKindE::COUNT )>                    m_radius{};  ///< collision radii
    std::array<std::array<Color, 3>, static_cast<size_t>( PropKindE::COUNT )>     m_keys{};    ///< default key colours
    std::array<FacingE, static_cast<size_t>( PropKindE::COUNT )>                  m_facing{};  ///< which way they turn
};

} // namespace sunstreak
