/**
 * @file Achievements.h
 * @brief The achievements of the world tour: countries and continents driven through, police escaped, hitchhikers
 *        delivered, ambulances let through, postcards, kilometres. Earned ones are kept in the records.
 * @author Rene Weller
 */
#pragma once

#include "Records.h"

#include <cstddef>
#include <vector>

namespace sunstreak
{

/** @brief An achievement. */
struct Achievement
{
    const char *m_key;     ///< as the records keep it
    const char *m_title;   ///< its name
    const char *m_what;    ///< what earns it
};

/** @brief How far along an achievement is. */
struct AchievementProgress
{
    int m_have = 0;   ///< so far
    int m_need = 1;   ///< needed
};

/**
 * @brief All the achievements, in the order the passport lists them.
 * @return them
 */
const std::vector<Achievement> &achievements();

/**
 * @brief How far along one is.
 * @param[in] records  the records
 * @param[in] index    the achievement
 * @return its progress
 */
AchievementProgress achievementProgress( const Records &records, size_t index );

/**
 * @brief Marks the achievements now earned and not yet kept.
 * @param[in,out] records  the records (their earned list grows)
 * @return the newly earned ones (indices)
 */
std::vector<size_t> earnAchievements( Records &records );

/**
 * @brief How many police liveries there are to escape (the POLICE_CAR shapes).
 * @return the count
 */
int policeLiveries();

} // namespace sunstreak
