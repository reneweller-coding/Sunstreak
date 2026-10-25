/**
 * @file Phrases.h
 * @brief What people say on the road, per country (data/phrases.txt): hitchhikers' hellos and thanks, the local heroes'
 *        challenges, taunts and grumbles, and their first names.
 * @author reneweller
 */
#pragma once

#include <string>
#include <vector>

namespace sunstreak
{

/** @brief A country's phrases. */
struct Phrases
{
    std::string              m_hello;       ///< hello
    std::string              m_thanks;      ///< thank you
    std::string              m_challenge;   ///< a challenge to race
    std::string              m_taunt;       ///< for the loser
    std::string              m_grumble;     ///< when beaten
    std::vector<std::string> m_names;       ///< first names of its drivers
};

/**
 * @brief A country's phrases: its own line, else its fleet's, else the default.
 * @param[in] country  its name (nullptr: none)
 * @return them
 */
const Phrases &phrasesOf( const char *country );

/**
 * @brief How many lines of data/phrases.txt were wrong.
 * @return the count
 */
int phraseErrors();

} // namespace sunstreak
