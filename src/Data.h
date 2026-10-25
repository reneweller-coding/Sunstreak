/**
 * @file Data.h
 * @brief The game's data files (data/ in the source tree): embedded at build time, or read from a folder named by the
 *        environment variable SUNSTREAK_DATA (to try changes without building).
 * @author Rene Weller
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
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
    std::vector<std::string> m_homes;   ///< its "home" lines: the countries and regions whose places strew it (WorldData.cpp)
    std::string              m_litter;  ///< its "litter" line: CHANCE NEAREST FARTHEST [solid] FLEETS, for polluted places
};

/**
 * @brief The landmarks of the data files, read once, in the files' order (their variants follow the compiled ones).
 * @return them
 */
const std::vector<DataShape> &dataShapes();

/**
 * @brief How often the data files were read again: what is built from them is built again when it changes.
 * @return the generation (0 at the start)
 */
int dataGeneration();

/**
 * @brief Reads the data files again on their next use: from the folder SUNSTREAK_DATA names, else from a data folder in
 *        the working directory, beside the program or a few folders above either (a build's bin folder), else from the
 *        built-in copies. Nothing may be building from them meanwhile (the world's worker stands still).
 * @param[in] programDir  the program's folder
 * @return the folder they come from (empty: the built-in copies)
 */
std::string reloadData( const std::string &programDir );

/** @brief Something built from the data files: built on first use, and again after reloadData(). */
template <typename T>
class DataCache
{
public:
    /**
     * @brief The value, built if the data changed since.
     * @param[in] build  builds it
     * @return it
     */
    template <typename BuildT>
    const T &get( BuildT build )
    {
        const int g = dataGeneration();
        if( m_generation.load( std::memory_order_acquire ) != g )
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            if( m_generation.load( std::memory_order_relaxed ) != g )
            {
                m_value = build();
                m_generation.store( g, std::memory_order_release );
            }
        }
        return m_value;
    }

private:
    T                m_value{};              ///< the value
    std::atomic<int> m_generation{ -1 };     ///< the data generation it was built from
    std::mutex       m_mutex;                ///< one builder at a time
};

} // namespace sunstreak
