/**
 * @file Data.h
 * @brief The game's data files (data/ in the source tree): embedded at build time, or read from a folder named by the
 *        environment variable SUNSTREAK_DATA (to try changes without building).
 * @author reneweller
 */
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace sunstreak
{

/** @brief A file built into the program. */
struct EmbeddedFile
{
    const char          *m_name;   ///< path below data/, with forward slashes
    const unsigned char *m_data;   ///< its bytes
    size_t               m_size;   ///< how many
};

/**
 * @brief The files built into the program (EmbeddedData.cpp, generated).
 * @return them, sorted by name
 */
const std::vector<EmbeddedFile> &embeddedFiles();

/**
 * @brief A data file's text.
 * @param[in] name  path below data/
 * @return its contents (empty if there is none)
 */
std::string dataText( const std::string &name );

/**
 * @brief The data files below a folder, sorted.
 * @param[in] prefix  e.g. "places/"
 * @return their names
 */
std::vector<std::string> dataNames( const std::string &prefix );

/**
 * @brief Splits a data file into lines without comments ("#" to the end) and blank lines.
 * @param[in] text  the file
 * @return its lines, trimmed
 */
std::vector<std::string> dataLines( const std::string &text );

/**
 * @brief Splits a line at a separator, trimming each part.
 * @param[in] line  the line
 * @param[in] sep   the separator ('|' for tables, ' ' for words)
 * @return the parts (empty ones dropped for ' ')
 */
std::vector<std::string> splitLine( const std::string &line, char sep );

/** @brief A landmark's shape from data/landmarks/: its name and the lines that build it (ModelsWorld.cpp reads them). */
struct DataShape
{
    std::string              m_name;    ///< what the places call it ("landmark NAME ...")
    std::vector<std::string> m_lines;   ///< one primitive or modifier per line
    bool                     m_creature = false;   ///< from data/creatures: it swims (a traffic model too)
};

/**
 * @brief The landmarks of the data files, read once, in the files' order (their variants follow the compiled ones).
 * @return them
 */
const std::vector<DataShape> &dataShapes();

} // namespace sunstreak
