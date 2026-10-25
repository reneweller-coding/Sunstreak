/**
 * @file WorldMap.h
 * @brief The passport's map: country and province outlines (data/world, from Natural Earth), painted with the ones
 *        where you have seen a place.
 * @author Rene Weller
 */
#pragma once

#include "Biome.h"

#include <raylib.h>

#include <string>
#include <vector>

namespace sunstreak
{

/** @brief A country or a province: its outline in degrees. */
struct MapArea
{
    std::string                       m_name;    ///< its name
    std::vector<std::vector<Vector2>> m_rings;   ///< its rings (longitude, latitude), islands and holes alike
    Rectangle                         m_box{};   ///< the rings' bounds in degrees
};

/**
 * @brief The countries of data/world/countries.txt (read again after a reload).
 * @return them
 */
const std::vector<MapArea> &mapCountries();

/**
 * @brief The states and provinces of data/world/provinces.txt.
 * @return them
 */
const std::vector<MapArea> &mapProvinces();

/**
 * @brief Whether a point lies in an area (even-odd over all its rings).
 * @param[in] a    the area
 * @param[in] lon  longitude
 * @param[in] lat  latitude
 * @return true inside
 */
bool areaContains( const MapArea &a, float lon, float lat );

/**
 * @brief Which areas hold any of some points (a point just off a coast counts for the nearest shore within a third of a
 *        degree).
 * @param[in] areas   the areas
 * @param[in] points  the points
 * @return a flag per area
 */
std::vector<bool> areasWith( const std::vector<MapArea> &areas, const std::vector<GeoPoint> &points );

/**
 * @brief Paints the world for the passport: the land, the countries and provinces where a place has been seen, the
 *        province borders thin and the country borders stronger, the sea left clear.
 * @param[in] w          width in pixels
 * @param[in] h          height in pixels
 * @param[in] latTop     the latitude at the top edge (longitude runs -180..180 across)
 * @param[in] latBottom  the latitude at the bottom edge
 * @param[in] seen       where the places seen lie
 * @return the picture (the caller unloads it)
 */
Image paintWorldMap( int w, int h, float latTop, float latBottom, const std::vector<GeoPoint> &seen );

}   // namespace sunstreak
