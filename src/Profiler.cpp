/**
 * @file Profiler.cpp
 * @brief GPU timestamps through the GL 3.3 query functions, fetched from GLFW (raylib leaves them out of its API).
 * @author reneweller
 */
#include "Profiler.h"

#include <rlgl.h>

#include <cstdint>

#if defined( _WIN32 )
#define SUNSTREAK_GLAPI __stdcall
#else
#define SUNSTREAK_GLAPI
#endif

extern "C" void ( *glfwGetProcAddress( const char *procname ) )( void );

namespace sunstreak
{

namespace
{

constexpr unsigned GlTimestamp = 0x8E28;        ///< GL_TIMESTAMP
constexpr unsigned GlQueryResult = 0x8866;      ///< GL_QUERY_RESULT
constexpr unsigned GlResultAvailable = 0x8867;  ///< GL_QUERY_RESULT_AVAILABLE

using GenQueries = void( SUNSTREAK_GLAPI * )( int, unsigned * );
using QueryCounter = void( SUNSTREAK_GLAPI * )( unsigned, unsigned );
using GetQueryObjectiv = void( SUNSTREAK_GLAPI * )( unsigned, unsigned, int * );
using GetQueryObjectui64v = void( SUNSTREAK_GLAPI * )( unsigned, unsigned, uint64_t * );

GenQueries          g_genQueries = nullptr;
QueryCounter        g_queryCounter = nullptr;
GetQueryObjectiv    g_getQueryiv = nullptr;
GetQueryObjectui64v g_getQueryui64v = nullptr;

} // namespace

void GpuTimer::init()
{
    g_genQueries = reinterpret_cast<GenQueries>( glfwGetProcAddress( "glGenQueries" ) );
    g_queryCounter = reinterpret_cast<QueryCounter>( glfwGetProcAddress( "glQueryCounter" ) );
    g_getQueryiv = reinterpret_cast<GetQueryObjectiv>( glfwGetProcAddress( "glGetQueryObjectiv" ) );
    g_getQueryui64v = reinterpret_cast<GetQueryObjectui64v>( glfwGetProcAddress( "glGetQueryObjectui64v" ) );
    m_ok = g_genQueries && g_queryCounter && g_getQueryiv && g_getQueryui64v;
    if( m_ok ) g_genQueries( Ring * MARKS, &m_query[0][0] );
}

void GpuTimer::mark( PassE at )
{
    if( !m_ok ) return;
    rlDrawRenderBatchActive();
    const int slot = m_frame % Ring;
    g_queryCounter( m_query[slot][at], GlTimestamp );
    m_set[slot][at] = true;
}

void GpuTimer::endFrame()
{
    if( !m_ok ) return;
    // The oldest frame in the ring: complete, and its results are (almost always) in.
    const int old = ( m_frame + 1 ) % Ring;
    bool      complete = true;
    for( int k = 0; k < MARKS; ++k ) complete = complete && m_set[old][k];
    int available = 0;
    if( complete ) g_getQueryiv( m_query[old][HUD], GlResultAvailable, &available );
    if( complete && available )
    {
        uint64_t t[MARKS]{};
        for( int k = 0; k < MARKS; ++k ) g_getQueryui64v( m_query[old][k], GlQueryResult, &t[k] );
        for( int k = 1; k < MARKS; ++k )
        {
            const float ms = t[k] >= t[k - 1] ? static_cast<float>( static_cast<double>( t[k] - t[k - 1] ) * 1.0e-6 ) : 0.0f;
            m_ms[k] += ( ms - m_ms[k] ) * 0.05f;
            m_sum[k] += ms;
        }
        ++m_count;
    }
    for( int k = 0; k < MARKS; ++k ) m_set[old][k] = false;
    ++m_frame;
}

void GpuTimer::resetTotals()
{
    for( double &s : m_sum ) s = 0.0;
    m_count = 0;
}

} // namespace sunstreak
