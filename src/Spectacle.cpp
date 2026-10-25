/**
 * @file Spectacle.cpp
 * @brief The spectacles: where they happen and how they move.
 * @author Rene Weller
 */
#include "Spectacle.h"

#include "Data.h"
#include "Rng.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace sunstreak
{

namespace
{

int g_skyErrors = 0;   ///< wrong lines in the last read

const char *const KindNames[] = { "PLANE", "JETS", "BALLOONS", "SHIP", "ZEPPELIN" };   ///< SkyE's names in the file
const char *const ModelNames[] = {
    "SKY_AIRLINER",  "SKY_JET",       "SKY_BALLOON_1", "SKY_BALLOON_2",
    "SKY_BALLOON_3", "SKY_BALLOON_4", "SKY_SHIP",      "SKY_ZEPPELIN"
};   ///< SkyModelE's shapes

/**
 * @brief Whether a place answers to a "where" (a country, COUNTRY:REGION or place:ID).
 * @param[in] where  the name
 * @param[in] biome  the place
 * @return true if it does
 */
bool placeIs( const std::string &where, BiomeE biome )
{
    const BiomeDef &d = biomeDef( biome );
    if ( where.rfind( "place:", 0 ) == 0 ) return biomeKey( biome ) == where.substr( 6 );
    if ( !d.m_country ) return false;
    const size_t colon = where.find( ':' );
    if ( colon == std::string::npos ) return where == d.m_country;
    return d.m_region && where.substr( 0, colon ) == d.m_country && where.substr( colon + 1 ) == d.m_region;
}

}   // namespace

const char *skyModelName( int model )
{
    return model >= 0 && model < SKY_MODELS ? ModelNames[model] : "";
}

const std::vector<SkyRule> &skyRules()
{
    static DataCache<std::vector<SkyRule>> cache;
    return cache.get(
        []
        {
            g_skyErrors = 0;
            std::vector<SkyRule> out;
            for ( const std::string &line : dataLines( dataText( "spectacles.txt" ) ) )
            {
                const std::vector<std::string> f = splitLine( line, '|' );
                const auto kind = f.empty() ? std::end( KindNames )
                                            : std::find_if( std::begin( KindNames ), std::end( KindNames ),
                                                            [&]( const char *k ) { return f[0] == k; } );
                if ( f.size() != 3 || kind == std::end( KindNames ) )
                {
                    ++g_skyErrors;
                    continue;
                }
                SkyRule r;
                r.m_kind = static_cast<SkyE>( kind - std::begin( KindNames ) );
                r.m_share = std::clamp( static_cast<float>( std::atof( f[2].c_str() ) ), 0.0f, 1.0f );
                for ( const std::string &w : splitLine( f[1], ',' ) )
                {
                    if ( w.empty() ) continue;
                    bool known = false;
                    for ( int b = 0; b < biomeCount() && !known; ++b ) known = placeIs( w, static_cast<BiomeE>( b ) );
                    if ( !known ) ++g_skyErrors;
                    r.m_where.push_back( w );
                }
                out.push_back( std::move( r ) );
            }
            return out;
        } );
}

int skyErrors()
{
    skyRules();
    return g_skyErrors;
}

bool stageSpectacle( const Track &track, int stage, SkyE &kind, double &at )
{
    if ( stage < 0 ) return false;
    const BiomeE    biome = track.stageBiome( stage );
    const BiomeDef &place = biomeDef( biome );
    if ( place.m_underwater || place.m_crossing != CrossingE::NONE ) return false;
    for ( const SkyRule &r : skyRules() )
    {
        if ( std::none_of( r.m_where.begin(), r.m_where.end(),
                           [&]( const std::string &w ) { return placeIs( w, biome ); } ) )
            continue;
        const uint64_t h =
            hash64( track.seed() * 8191 + static_cast<uint64_t>( stage ) * 29 + static_cast<uint64_t>( r.m_kind ) );
        if ( static_cast<float>( h % 1000 ) >= r.m_share * 1000.0f ) continue;
        // A ship needs water beside the road.
        if ( r.m_kind == SkyE::SHIP && place.m_terrain.m_waterLeft < 0.5f && place.m_terrain.m_waterRight < 0.5f )
            continue;
        kind = r.m_kind;
        at = track.stageStart( stage ) + 1500.0 +
             static_cast<double>( ( h >> 12 ) %
                                  static_cast<uint64_t>( std::max( 1000.0, track.stageLength( stage ) - 7000.0 ) ) );
        return true;
    }
    return false;
}

bool stageRunway( const Track &track, int stage, double &s, float &side )
{
    SkyE   kind = SkyE::COUNT;
    double at = 0.0;
    if ( !stageSpectacle( track, stage, kind, at ) || kind != SkyE::PLANE ) return false;
    s = at + RunwayAhead;
    side = ( hash64( track.seed() * 77 + static_cast<uint64_t>( stage ) ) & 1 ) ? 1.0f : -1.0f;
    return true;
}

void Spectacle::reset()
{
    *this = Spectacle{};
}

void Spectacle::update( float dt, const Track &track, const PlayerCar &player, bool enabled, SkyEvents &ev )
{
    ev = SkyEvents{};
    const int stage = track.stageAt( player.m_s );
    if ( stage > m_lastStage )
    {
        m_lastStage = stage;
        SkyE   kind = SkyE::COUNT;
        double at = 0.0;
        if ( stageSpectacle( track, stage, kind, at ) && at > player.m_s )
        {
            m_next = kind;
            m_at = at;
        }
    }
    if ( m_kind == SkyE::COUNT && m_at >= 0.0 && player.m_s >= m_at && enabled )
    {
        // It begins: what and where depend on the kind.
        m_kind = m_next;
        m_at = -1.0;
        m_things.clear();
        const uint64_t  h = hash64( track.seed() * 77 + static_cast<uint64_t>( stage ) );
        const BiomeDef &place = biomeDef( track.stageBiome( stage ) );
        m_dir = ( h & 1 ) ? 1.0f : -1.0f;
        const float speed = std::max( player.m_speed, 15.0f );
        switch ( m_kind )
        {
        case SkyE::PLANE:
        {
            // It crosses the road over the end of the runway just as the car gets there, and lands beyond it.
            double cross = player.m_s + RunwayAhead;
            float  side = m_dir;
            stageRunway( track, stage, cross, side );
            const float eta = static_cast<float>( std::max( 60.0, cross - player.m_s ) ) / speed;
            m_things.push_back( { SKY_AIRLINER, cross, -m_dir * std::clamp( 85.0f * eta, 250.0f, 1100.0f ), 70.0f,
                                  m_dir * PI * 0.5f } );
            ev.m_roar = true;
            ev.m_pan = -m_dir;
            break;
        }
        case SkyE::JETS:
            for ( float side : { -1.0f, 1.0f } )
                m_things.push_back(
                    { SKY_JET, player.m_s - 500.0 + ( side > 0.0f ? -40.0 : 0.0 ), side * 14.0f, 55.0f, 0.0f } );
            ev.m_roar = true;
            break;
        case SkyE::BALLOONS:
            for ( int k = 0; k < 12; ++k )
            {
                const uint64_t b = hash64( h + static_cast<uint64_t>( k ) * 31 );
                const float    side = ( b & 1 ) ? 1.0f : -1.0f;
                m_things.push_back( { SKY_BALLOON_1 + static_cast<int>( ( b >> 4 ) % 4 ),
                                      player.m_s + 200.0 + static_cast<double>( ( b >> 8 ) % 1500 ),
                                      side * ( 35.0f + static_cast<float>( ( b >> 20 ) % 220 ) ),
                                      18.0f + static_cast<float>( ( b >> 32 ) % 120 ), 0.0f } );
            }
            break;
        case SkyE::SHIP:
        {
            const float side = place.m_terrain.m_waterLeft >= place.m_terrain.m_waterRight ? -1.0f : 1.0f;
            m_things.push_back( { SKY_SHIP, player.m_s + 1300.0, side * 170.0f, place.m_terrain.m_seaLevel + 3.0f,
                                  m_dir > 0.0f ? 0.0f : PI } );
            break;
        }
        case SkyE::ZEPPELIN:
            m_things.push_back( { SKY_ZEPPELIN, player.m_s + 1400.0, m_dir * 260.0f, 220.0f, 0.0f } );
            break;
        default: break;
        }
    }
    if ( m_kind == SkyE::COUNT ) return;

    // On their way; gone when out of sight.
    bool gone = true;
    for ( SkyThing &t : m_things )
    {
        switch ( m_kind )
        {
        case SkyE::PLANE:
            // Down over the road, then onto the runway, slowing as it rolls.
            t.m_lateral += m_dir * ( t.m_height > 0.5f ? 85.0f : 45.0f ) * dt;
            t.m_height = t.m_lateral * m_dir > 0.0f ? std::max( 0.0f, t.m_height - 9.0f * dt )
                                                    : std::max( 22.0f, t.m_height - 3.2f * dt );
            gone = gone && std::abs( t.m_lateral ) > 800.0f && t.m_lateral * m_dir > 0.0f;
            break;
        case SkyE::JETS:
            t.m_s += static_cast<double>( std::max( player.m_speed, 20.0f ) + 75.0f ) * dt;
            gone = gone && t.m_s > player.m_s + 2200.0;
            break;
        case SkyE::BALLOONS:
            t.m_height += 0.25f * dt;
            t.m_lateral += 0.6f * dt;
            gone = gone && t.m_s < player.m_s - 400.0;
            break;
        case SkyE::SHIP:
            t.m_s += static_cast<double>( m_dir * 6.0f * dt );
            gone = gone && t.m_s < player.m_s - 900.0;
            break;
        case SkyE::ZEPPELIN:
            t.m_s += 10.0 * dt;
            gone = gone && t.m_s < player.m_s - 1200.0;
            break;
        default: break;
        }
    }
    if ( gone || m_things.empty() )
    {
        m_kind = SkyE::COUNT;
        m_things.clear();
    }
}

}   // namespace sunstreak
