/**
 * @file Traffic.cpp
 * @brief Traffic driving, overtakes, near misses, collisions and flying cones.
 * @author Rene Weller
 */
#include "Traffic.h"
#include "Audio.h"
#include "Local.h"

#include <algorithm>

namespace sunstreak
{

namespace
{

/**
 * @brief Whether a lane is closed somewhere ahead.
 * @param[in] track     the road
 * @param[in] s         from here
 * @param[in] lane      the lane
 * @param[in] distance  how far to look
 * @return true if roadworks close it
 */
bool laneClosedAhead( const Track &track, double s, int lane, double distance )
{
    const long first = static_cast<long>( s / SegmentLength );
    const long last = static_cast<long>( ( s + distance ) / SegmentLength );
    for ( long i = first; i <= last; ++i )
        if ( track.segment( i ).m_blockedLanes & ( 1u << lane ) ) return true;
    return false;
}

}   // namespace

void Traffic::reset( uint64_t seed, double startS )
{
    m_cars.clear();
    m_obstacles.clear();
    m_rng = Rng( seed ^ 0x7AFF1Cull );
    m_spawnS = startS + 120.0;
    m_scanned = 0;
    m_sirenS = startS + static_cast<double>( m_rng.range( 1500.0f, 5000.0f ) );
    m_rngOncoming = Rng( seed ^ 0x0C0111E5ull );
    m_oncomingDue = 0.0;
    m_spawnThin = 1.0f;
}

int Traffic::oncomingLanes( const Track &track, const Segment &seg, int &first, int &last ) const
{
    first = 0;
    last = -1;
    const int lanes = seg.m_lanes;
    if ( !m_oncoming || lanes < 2 || lanes % 2 != 0 || ( seg.m_flags & ( SEG_FORK | SEG_MEDIAN ) ) ) return 0;
    const bool left = countryInfo( biomeDef( track.stageBiome( seg.m_stage ) ).m_country ).m_left;
    first = left ? lanes / 2 : 0;
    last = first + lanes / 2 - 1;
    return lanes / 2;
}

bool Traffic::twoWay( const Track &track, double from, double to ) const
{
    int side = -1;
    for ( long i = static_cast<long>( from / SegmentLength ); i <= static_cast<long>( to / SegmentLength ); ++i )
    {
        const Segment &seg = track.segment( i );
        int            first = 0, last = -1;
        if ( oncomingLanes( track, seg, first, last ) == 0 ) return false;
        // The border to a country that drives on the other side.
        if ( side >= 0 && side != ( first == 0 ? 0 : 1 ) ) return false;
        side = first == 0 ? 0 : 1;
    }
    for ( double s : { from, 0.5 * ( from + to ), to } )
    {
        const Crossing xc = track.crossingAt( s );
        if ( xc.m_type != CrossingE::NONE && to > xc.m_board - 150.0 && from < xc.m_land + 60.0 ) return false;
    }
    // Not out of the middle of roadworks (before them it waits: Traffic::update).
    for ( double s = to - 260.0; s <= to; s += SegmentLength )
        if ( track.segment( static_cast<long>( s / SegmentLength ) ).m_blockedLanes ) return false;
    return true;
}

bool Traffic::laneFree( int lane, double s, const TrafficCar *self ) const
{
    for ( const TrafficCar &c : m_cars )
    {
        if ( &c == self || c.m_lane != lane ) continue;
        if ( c.m_s > s - 18.0 && c.m_s < s + 28.0 ) return false;
    }
    return true;
}

int Traffic::flash( const PlayerCar &player )
{
    int made = 0;
    for ( TrafficCar &c : m_cars )
    {
        const double gap = c.m_s - player.m_s;
        if ( gap < 2.0 || gap > 110.0 || c.m_animal || c.m_swim || c.m_siren || c.m_gone || c.m_yield > 0.0f ) continue;
        if ( c.m_oncoming ) continue;   // what comes the other way answers with its own lights
        if ( std::abs( c.m_x - player.m_x ) > ( c.m_width + player.m_spec.m_width ) * 0.5f + 0.8f ) continue;
        if ( c.m_speed > player.m_speed + 2.0f ) continue;
        if ( c.m_chaos && m_rng.chance( 0.5f ) ) continue;
        c.m_yield = 3.5f;
        ++made;
    }
    return made;
}

int Traffic::honk( const PlayerCar &player, bool &answered, float &pan )
{
    int startled = 0;
    answered = false;
    pan = 0.0f;
    for ( TrafficCar &c : m_cars )
    {
        const double gap = c.m_s - player.m_s;
        if ( c.m_animal && gap > 0.0 && gap < 160.0 && !c.m_swim )
        {
            // No more standing in the road, and off it at a trot.
            c.m_wait = 0.0f;
            c.m_waited = true;
            if ( std::abs( c.m_vx ) < 6.0f ) c.m_vx *= 2.0f;
            ++startled;
        }
        else if ( c.m_chaos && !c.m_animal && !answered && std::abs( gap ) < 60.0 && m_rng.chance( 0.6f ) )
        {
            answered = true;
            pan = clampf( ( c.m_x - player.m_x ) / 6.0f, -1.0f, 1.0f );
        }
    }
    return startled;
}

void Traffic::scanRoadworks( const Track &track, double upTo )
{
    const long last = std::min( static_cast<long>( upTo / SegmentLength ), track.endIndex() - 2 );
    m_scanned = std::max( m_scanned, track.firstIndex() + 1 );
    for ( ; m_scanned <= last; ++m_scanned )
    {
        const Segment &seg = track.segment( m_scanned );
        if ( !( seg.m_flags & SEG_ROADWORKS ) || seg.m_blockedLanes == 0 ) continue;
        const int    lanes = seg.m_lanes;
        const bool   leftClosed = seg.m_blockedLanes & 1u;
        const float  w = 2.0f * seg.m_halfWidth / static_cast<float>( lanes );
        const float  boundary = leftClosed ? -seg.m_halfWidth + w : seg.m_halfWidth - w;
        const double s = static_cast<double>( m_scanned ) * SegmentLength;
        const bool   zoneStart = !( track.segment( m_scanned - 1 ).m_flags & SEG_ROADWORKS );
        if ( zoneStart )
        {
            // A taper of cones from the road's edge to the lane line, the arrow board in the closed lane.
            const float edge = leftClosed ? -seg.m_halfWidth + 0.4f : seg.m_halfWidth - 0.4f;
            for ( int k = 0; k < 5; ++k )
            {
                Obstacle o;
                o.m_s = s - 24.0 + 6.0 * k;
                o.m_x = lerpf( edge, boundary, static_cast<float>( k ) / 4.0f );
                m_obstacles.push_back( o );
            }
            Obstacle board;
            board.m_s = s - 30.0;
            board.m_x = leftClosed ? -seg.m_halfWidth + w * 0.5f : seg.m_halfWidth - w * 0.5f;
            board.m_kind = leftClosed ? 2 : 1;
            m_obstacles.push_back( board );
        }
        if ( m_scanned % 2 == 0 )
        {
            Obstacle o;
            o.m_s = s;
            o.m_x = boundary;
            m_obstacles.push_back( o );
        }
    }
}

void Traffic::update( float dt, const Track &track, PlayerCar &player, const Models &models, TrafficEvents &ev )
{
    // Spawn ahead, beyond the fog.
    const BiomeMix mix = track.biomeMix( player.m_s + 900.0 );
    const float    density = lerpf( biomeDef( mix.m_from ).m_traffic, biomeDef( mix.m_to ).m_traffic, mix.m_t ) *
                             std::min( 1.8f, 0.85f + 0.12f * static_cast<float>( player.m_s / 5000.0 ) ) *
                             tuning().m_traffic * m_density;
    if ( m_spawnS < player.m_s + 220.0 ) m_spawnS = player.m_s + 220.0;
    // The country decides the side: slower cars keep right, or left where they drive on the left.
    const int          playerStage = track.stageAt( player.m_s );
    const bool         keepLeft = countryInfo( biomeDef( track.stageBiome( playerStage ) ).m_country ).m_left;
    const BiomeDef    &spawnPlace = biomeDef( track.stageBiome( track.stageAt( player.m_s + 900.0 ) ) );
    const CountryInfo &spawnLand = countryInfo( spawnPlace.m_country );
    const uint32_t     fleetBit = 1u << static_cast<unsigned>( spawnLand.m_fleet );
    const uint8_t      styles = spawnPlace.m_country ? roadCustoms( spawnPlace.m_country ).m_styles : 0;
    m_clock += dt;

    // Animals of the place cross the road now and then, out in the country.
    if ( m_animalS < player.m_s + 500.0 ) m_animalS = player.m_s + 500.0 + m_rng.range( 300.0f, 1200.0f );
    if ( m_density > 0.0f && player.m_s + 800.0 > m_animalS )
    {
        std::vector<int> fauna;
        if ( spawnPlace.m_lanes <= 2 && spawnPlace.m_country )
            for ( const PropRule &r : spawnPlace.m_props )
                for ( size_t i = 0; i < models.m_traffic.size(); ++i )
                    if ( r.m_kind != PropKindE::LANDMARK && models.m_traffic[i].m_animal == r.m_kind )
                        fauna.push_back( static_cast<int>( i ) );
        const Segment &seg = track.segment( static_cast<long>( m_animalS / SegmentLength ) );
        if ( !fauna.empty() && !( seg.m_flags & ( SEG_TUNNEL | SEG_BRIDGE ) ) && m_animalS > StartLine + 300.0 )
        {
            const int model = fauna[static_cast<size_t>( m_rng.rangeInt( 0, static_cast<int>( fauna.size() ) - 1 ) )];
            const VehicleModel &vm = models.m_traffic[static_cast<size_t>( model )];
            const PropKindE     kind = vm.m_animal;
            const bool  herd = kind == PropKindE::SHEEP || kind == PropKindE::COW || kind == PropKindE::ZEBRA ||
                               kind == PropKindE::CAMEL || kind == PropKindE::BISON || kind == PropKindE::REINDEER ||
                               kind == PropKindE::YAK || kind == PropKindE::LLAMA;
            const float pace = kind == PropKindE::KANGAROO                               ? 7.0f
                               : kind == PropKindE::ZEBRA                                ? 3.0f
                               : kind == PropKindE::HORSE                                ? 2.5f
                               : kind == PropKindE::MOOSE || kind == PropKindE::REINDEER ? 2.0f
                               : kind == PropKindE::SHEEP                                ? 1.5f
                                                                                         : 1.1f;
            const float side = m_rng.chance( 0.5f ) ? 1.0f : -1.0f;
            const int   count = herd ? m_rng.rangeInt( 1, 3 ) : 1;
            for ( int k = 0; k < count; ++k )
            {
                TrafficCar a;
                a.m_animal = true;
                a.m_model = model;
                a.m_s = m_animalS + static_cast<double>( k ) * m_rng.range( 3.0f, 6.0f );
                a.m_x = side * ( seg.m_halfWidth + 4.0f + 2.5f * static_cast<float>( k ) );
                a.m_vx = -side * pace * m_rng.range( 0.85f, 1.15f );
                a.m_yaw = a.m_vx > 0.0f ? Tau * 0.25f : -Tau * 0.25f;
                a.m_length = vm.m_width;
                a.m_width = vm.m_length;
                a.m_lane = -1;
                a.m_passed = true;
                m_cars.push_back( a );
            }
            ev.m_animal = static_cast<int>( kind );
        }
        m_animalS += m_rng.range( 900.0f, 2400.0f );
    }
    // Under the sea: shoals, sharks, rays, whales and crabs round the glass tunnel.
    {
        const BiomeDef &sea = biomeDef( track.stageBiome( track.stageAt( player.m_s + 500.0 ) ) );
        if ( m_swimS < player.m_s + 200.0 ) m_swimS = player.m_s + 200.0 + m_rng.range( 0.0f, 80.0f );
        if ( !sea.m_swim.empty() && sea.m_crossing == CrossingE::NONE && player.m_s + 460.0 > m_swimS )
        {
            float total = 0.0f;
            for ( const SwimRule &r : sea.m_swim ) total += r.m_weight;
            float           pick = m_rng.range( 0.0f, total );
            const SwimRule *rule = &sea.m_swim.front();
            for ( const SwimRule &r : sea.m_swim )
            {
                if ( pick < r.m_weight )
                {
                    rule = &r;
                    break;
                }
                pick -= r.m_weight;
            }
            const int model = rule->m_shape >= 0 && rule->m_shape < static_cast<int>( models.m_creatureModel.size() )
                                  ? models.m_creatureModel[static_cast<size_t>( rule->m_shape )]
                                  : -1;
            if ( model >= 0 && rule->m_circle )
            {
                // Sharks round the tube: circling over it a little ahead, the car passing under them.
                const int      count = m_rng.rangeInt( rule->m_countMin, std::max( 2, rule->m_countMax ) );
                const Segment &seaSeg = track.segment( static_cast<long>( m_swimS / SegmentLength ) );
                const float    top = ( seaSeg.m_halfWidth + RumbleWidth + 2.6f ) * 0.82f + 0.2f;
                const float    radius = m_rng.range( 12.0f, 18.0f ),
                               speed = m_rng.range( 4.0f, 6.5f ) * ( m_rng.chance( 0.5f ) ? 1.0f : -1.0f );
                const float    lift = std::max( top + 2.5f, m_rng.range( rule->m_liftMin, rule->m_liftMax ) );
                for ( int k = 0; k < count; ++k )
                {
                    TrafficCar a;
                    a.m_swim = true;
                    a.m_animal = true;
                    a.m_passed = true;
                    a.m_lane = -1;
                    a.m_model = model;
                    a.m_orbit = radius + m_rng.range( -2.0f, 2.0f );
                    a.m_orbitS = m_swimS + 40.0;
                    a.m_orbitA =
                        Tau * static_cast<float>( k ) / static_cast<float>( count ) + m_rng.range( -0.3f, 0.3f );
                    a.m_orbitW = speed / a.m_orbit;
                    a.m_s = a.m_orbitS + static_cast<double>( a.m_orbit * std::cos( a.m_orbitA ) );
                    a.m_x = a.m_orbit * std::sin( a.m_orbitA );
                    a.m_lift = lift + m_rng.range( -1.0f, 1.5f );
                    a.m_length = models.m_traffic[static_cast<size_t>( model )].m_length;
                    a.m_width = models.m_traffic[static_cast<size_t>( model )].m_width;
                    m_cars.push_back( a );
                }
            }
            else if ( model >= 0 )
            {
                // A group beside the tunnel, close to the glass, or over it, along the road or across it.
                const int   count = m_rng.rangeInt( rule->m_countMin, rule->m_countMax );
                const float side = m_rng.chance( 0.5f ) ? 1.0f : -1.0f;
                const float lift = m_rng.range( rule->m_liftMin, rule->m_liftMax );
                const bool  across = lift > 9.0f && m_rng.chance( 0.5f );
                const float along = m_rng.range( -6.0f, 14.0f ), vx = across ? -side * m_rng.range( 1.5f, 4.0f ) : 0.0f;
                const Segment &seaSeg = track.segment( static_cast<long>( m_swimS / SegmentLength ) );
                const float    clear = seaSeg.m_halfWidth + RumbleWidth + 2.6f + 1.5f +
                                       0.5f * models.m_traffic[static_cast<size_t>( model )].m_width;
                const float    x0 =
                    across ? side * m_rng.range( 30.0f, 50.0f ) : side * m_rng.range( clear, clear + 14.0f );
                for ( int k = 0; k < count; ++k )
                {
                    TrafficCar a;
                    a.m_swim = true;
                    a.m_animal = true;
                    a.m_passed = true;
                    a.m_lane = -1;
                    a.m_model = model;
                    a.m_s = m_swimS + static_cast<double>( m_rng.range( -8.0f, 8.0f ) );
                    a.m_x = x0 + m_rng.range( -4.0f, 4.0f );
                    if ( !across ) a.m_x = side * std::max( clear, std::abs( a.m_x ) );
                    a.m_lift = std::max( 0.0f, lift + ( count > 1 ? m_rng.range( -2.0f, 2.0f ) : 0.0f ) );
                    a.m_speed = along;
                    a.m_cruise = along;
                    a.m_vx = vx;
                    a.m_yaw = std::atan2( vx, std::abs( along ) > 0.5f ? along : 0.5f ) +
                              ( along < -0.5f ? Tau * 0.5f : 0.0f );
                    a.m_length = models.m_traffic[static_cast<size_t>( model )].m_length;
                    a.m_width = models.m_traffic[static_cast<size_t>( model )].m_width;
                    m_cars.push_back( a );
                }
            }
            m_swimS += m_rng.range( 10.0f, 36.0f );
        }
    }
    // Everyday cars and lorries, and now and then what the region drives: tuk-tuks, yellow cabs, road trains.
    auto vehicle = [&]( Rng &rng, bool city, int &swarm ) -> int
    {
        std::vector<int> cars, trucks, local, scooters, tractors;
        int              roadTrain = -1;
        for ( size_t i = 0; i < models.m_traffic.size(); ++i )
        {
            const VehicleModel &vm = models.m_traffic[i];
            if ( vm.m_animal != PropKindE::COUNT || vm.m_police ) continue;
            if ( vm.m_fleets != 0 && vm.m_pace < 0.5f && !vm.m_truck && vm.m_where != 1 )
                tractors.push_back( static_cast<int>( i ) );
            if ( vm.m_fleets == 0 )
                ( vm.m_truck ? trucks : cars ).push_back( static_cast<int>( i ) );
            else if ( ( vm.m_fleets & fleetBit ) && spawnPlace.m_country &&
                      ( vm.m_where == 0 || ( vm.m_where == 1 ) == city ) )
            {
                local.push_back( static_cast<int>( i ) );
                if ( vm.m_width < 1.0f ) scooters.push_back( static_cast<int>( i ) );
                if ( vm.m_truck &&
                     ( roadTrain < 0 || vm.m_length > models.m_traffic[static_cast<size_t>( roadTrain )].m_length ) )
                    roadTrain = static_cast<int>( i );
            }
        }
        auto pick = [&]( const std::vector<int> &from )
        { return from[static_cast<size_t>( rng.rangeInt( 0, static_cast<int>( from.size() ) - 1 ) )]; };
        const bool truck = rng.chance( 0.14f );
        int        model = !local.empty() && rng.chance( 0.38f ) ? pick( local ) : truck ? pick( trucks ) : pick( cars );
        // The country's way of driving: tractors on its country roads, road trains in the outback, swarms of scooters.
        if ( ( styles & StyleTractors ) && !city && !tractors.empty() && rng.chance( 0.12f ) ) model = pick( tractors );
        if ( ( styles & StyleRoadTrains ) && !city && roadTrain >= 0 && rng.chance( 0.2f ) ) model = roadTrain;
        if ( ( styles & StyleScooters ) && !scooters.empty() && rng.chance( 0.3f ) )
        {
            model = pick( scooters );
            swarm = rng.rangeInt( 3, 7 );
        }
        return model;
    };
    while ( m_density > 0.0f && m_spawnS < player.m_s + 1050.0 )
    {
        m_spawnS += static_cast<double>( lerpf( 200.0f, 60.0f, std::min( 1.0f, density / 1.8f ) ) *
                                         m_rng.range( 0.55f, 1.45f ) * m_spawnThin );
        const Segment &seg = track.segment( static_cast<long>( m_spawnS / SegmentLength ) );
        if ( m_spawnS < StartLine + 150.0 ) continue;
        const Crossing xc = track.crossingAt( m_spawnS );
        if ( xc.m_type != CrossingE::NONE && m_spawnS > xc.m_board - 150.0 && m_spawnS < xc.m_land + 60.0 ) continue;
        TrafficCar c;
        c.m_s = m_spawnS;
        // Its own half of a road of two directions (and there fewer of them: they share half the lanes).
        int       oncFirst = 0, oncLast = -1;
        const int own = seg.m_lanes - oncomingLanes( track, seg, oncFirst, oncLast );
        const int ownFirst = oncFirst == 0 ? oncLast + 1 : 0;
        // Light: as many as before in each lane; denser: the player's own half fuller, and more to overtake.
        const float share = static_cast<float>( seg.m_lanes ) / static_cast<float>( own );
        m_spawnThin = 1.0f + ( share - 1.0f ) * ( m_oncoming >= 3 ? 0.3f : m_oncoming == 2 ? 0.6f : 1.0f );
        int lane = ownFirst + m_rng.rangeInt( 0, own - 1 );
        for ( int tries = 0; tries < own && laneClosedAhead( track, c.m_s - 40.0, lane, 80.0 ); ++tries )
            lane = ownFirst + ( lane - ownFirst + 1 ) % own;
        if ( laneClosedAhead( track, c.m_s - 40.0, lane, 80.0 ) || !laneFree( lane, c.m_s, nullptr ) ) continue;
        c.m_lane = lane;
        c.m_x = laneCenter( lane, seg.m_lanes, seg.m_halfWidth );
        c.m_plate = static_cast<uint8_t>( roadCustoms( spawnPlace.m_country ).m_plate );
        int swarm = 0;
        c.m_model = vehicle( m_rng, seg.m_lanes >= 3, swarm );
        const VehicleModel &vm = models.m_traffic[static_cast<size_t>( c.m_model )];
        c.m_length = vm.m_length;
        c.m_width = vm.m_width;
        const float laneBonus = static_cast<float>( keepLeft ? lane : seg.m_lanes - 1 - lane ) * 2.0f;
        c.m_cruise = ( vm.m_truck ? m_rng.range( 15.0f, 20.0f ) : m_rng.range( 16.0f, 26.0f ) + laneBonus ) * vm.m_pace;
        c.m_speed = c.m_cruise;
        c.m_chaos = ( styles & StyleChaos ) != 0;
        c.m_weave = m_rng.range( 0.0f, Tau );
        const float laneHalf = seg.m_halfWidth / static_cast<float>( seg.m_lanes ) - vm.m_width * 0.5f - 0.1f;
        for ( int k = 0; k < std::max( 1, swarm ); ++k )
        {
            TrafficCar b = c;
            b.m_swarm = swarm > 0;
            if ( b.m_swarm )
            {
                // Side by side and one behind the other, all at much the same pace.
                b.m_s = c.m_s + static_cast<double>( k ) * static_cast<double>( m_rng.range( 2.0f, 4.0f ) );
                b.m_xOff = m_rng.range( -1.0f, 1.0f ) * std::max( 0.0f, laneHalf );
                b.m_x += b.m_xOff;
                b.m_cruise = c.m_cruise * m_rng.range( 0.96f, 1.04f );
                b.m_speed = b.m_cruise;
                b.m_weave = m_rng.range( 0.0f, Tau );
            }
            m_cars.push_back( b );
        }
        if ( swarm > 0 ) m_spawnS += 4.0 * swarm;   // the swarm takes its room
    }
    // What comes the other way on a road of two directions: a stream of its own, which comes along whether the player
    // drives or stands. Only where its whole way to the player and past is such a road.
    if ( m_oncoming && m_density > 0.0f )
    {
        m_oncomingDue -= static_cast<double>( ( std::max( 0.0f, player.m_speed ) + 22.0f ) * dt );
        const double at = player.m_s + m_oncomingAhead;
        if ( m_oncomingDue <= 0.0 )
        {
            const bool test = m_oncomingAhead < 1000.0;
            m_oncomingAhead = 1050.0;
            m_oncomingDue = 150.0;   // not here: ask again soon
            const Segment &seg = track.segment( static_cast<long>( at / SegmentLength ) );
            int            first = 0, last = -1;
            if ( twoWay( track, player.m_s - 150.0, at + 80.0 ) && oncomingLanes( track, seg, first, last ) > 0 )
            {
                const float spacing = m_oncoming >= 3 ? 0.45f : m_oncoming == 2 ? 0.75f : 1.3f;
                m_oncomingDue = static_cast<double>( lerpf( 1000.0f, 380.0f, std::min( 1.0f, density / 1.8f ) ) *
                                                     m_rngOncoming.range( 0.4f, 1.6f ) * spacing );
                // One, or a few behind each other.
                const int lane = m_rngOncoming.rangeInt( first, last );
                const int count = test ? 3 : m_rngOncoming.chance( 0.3f ) ? m_rngOncoming.rangeInt( 2, 3 ) : 1;
                double     s = at;
                const bool free = laneFree( lane, at + 20.0, nullptr );
                for ( int k = 0; k < count && free; ++k )
                {
                    TrafficCar c;
                    int        swarm = 0;
                    c.m_oncoming = true;
                    c.m_model = vehicle( m_rngOncoming, seg.m_lanes >= 3, swarm );
                    const VehicleModel &vm = models.m_traffic[static_cast<size_t>( c.m_model )];
                    c.m_s = s;
                    s += static_cast<double>( vm.m_length + m_rngOncoming.range( 12.0f, 30.0f ) );
                    c.m_lane = lane;
                    c.m_x = laneCenter( lane, seg.m_lanes, seg.m_halfWidth );
                    c.m_length = vm.m_length;
                    c.m_width = vm.m_width;
                    c.m_cruise =
                        ( vm.m_truck ? m_rngOncoming.range( 15.0f, 20.0f ) : m_rngOncoming.range( 16.0f, 26.0f ) ) *
                        vm.m_pace;
                    c.m_speed = c.m_cruise;
                    c.m_yaw = Tau * 0.5f;
                    c.m_plate = static_cast<uint8_t>( roadCustoms( spawnPlace.m_country ).m_plate );
                    // Dense traffic: now and then the last of a few is in a hurry and overtakes the others, out in
                    // the player's half while the way is clear there (below).
                    if ( m_oncoming >= 3 && k == count - 1 && k > 0 && ( test || m_rngOncoming.chance( 0.45f ) ) )
                    {
                        c.m_overtake = 1;
                        c.m_cruise = std::max( c.m_cruise, m_cars.back().m_cruise + 9.0f );
                        c.m_speed = m_cars.back().m_speed;
                    }
                    m_cars.push_back( c );
                }
            }
        }
    }
    scanRoadworks( track, player.m_s + 900.0 );

    // Now and then an ambulance or a fire engine on a run: from behind when the player is slower than it, else ahead.
    if ( m_density > 0.0f && player.m_s > m_sirenS )
    {
        m_sirenS = player.m_s + static_cast<double>( m_rng.range( 3500.0f, 8000.0f ) );
        const bool fire = m_rng.chance( 0.4f );
        float      cruise = fire ? 30.0f : 36.0f;
        const bool behind = player.m_speed < cruise - 3.0f;
        TrafficCar c;
        c.m_s = behind ? player.m_s - 120.0 : player.m_s + m_sirenAhead;
        m_sirenAhead = 900.0;
        const Segment  &seg = track.segment( static_cast<long>( c.m_s / SegmentLength ) );
        const Crossing  xc = track.crossingAt( c.m_s );
        const BiomeDef &place = biomeDef( track.stageBiome( track.stageAt( c.m_s ) ) );
        const int       model = serviceModel( models, place, fire ? "FIRE_ENGINE" : "AMBULANCE" );
        const bool      busy =
            std::any_of( m_cars.begin(), m_cars.end(), []( const TrafficCar &o ) { return o.m_siren != 0; } );
        const bool aboard = xc.m_type != CrossingE::NONE && c.m_s > xc.m_board - 150.0 && c.m_s < xc.m_land + 60.0;
        if ( model >= 0 && !busy && !aboard && c.m_s > StartLine + 150.0 && !( seg.m_flags & SEG_FORK ) )
        {
            if ( behind ) cruise = std::max( cruise, player.m_speed + 6.0f );
            const VehicleModel &vm = models.m_traffic[static_cast<size_t>( model )];
            c.m_model = model;
            c.m_length = vm.m_length;
            c.m_width = vm.m_width;
            c.m_siren = static_cast<uint8_t>(
                place.m_country && countryInfo( place.m_country ).m_fleet == FleetE::EUROPE ? 2 : 1 );
            // The fast lane (of its half, on a road of two directions).
            int first = 0, last = -1;
            oncomingLanes( track, seg, first, last );
            c.m_lane = last < first ? ( keepLeft ? seg.m_lanes - 1 : 0 ) : first == 0 ? last + 1 : first - 1;
            c.m_x = laneCenter( c.m_lane, seg.m_lanes, seg.m_halfWidth );
            c.m_cruise = cruise;
            c.m_speed = cruise;
            c.m_passed = behind;
            c.m_plate = static_cast<uint8_t>( roadCustoms( place.m_country ).m_plate );
            m_cars.push_back( c );
        }
    }
    // Ahead of a siren the traffic pulls over.
    std::vector<const TrafficCar *> sirens;
    bool                            meeting = false;   // something comes the other way, about to pass the player
    for ( const TrafficCar &c : m_cars )
    {
        if ( c.m_siren ) sirens.push_back( &c );
        const double ahead = c.m_s - player.m_s;
        meeting = meeting || ( c.m_oncoming && ahead > -10.0 &&
                               ahead < 3.0 * static_cast<double>( c.m_speed + std::max( 0.0f, player.m_speed ) ) );
    }
    for ( TrafficCar &c : m_cars )
    {
        if ( c.m_siren || c.m_animal || c.m_swim || c.m_oncoming ) continue;
        for ( const TrafficCar *e : sirens )
            if ( c.m_s - e->m_s > -8.0 && c.m_s - e->m_s < 160.0 ) c.m_yield = 2.0f;
    }

    // Drive.
    for ( TrafficCar &c : m_cars )
    {
        if ( c.m_swim && c.m_orbit > 0.0f )
        {
            // Round the circle, heading along it.
            c.m_orbitA += c.m_orbitW * dt;
            c.m_s = c.m_orbitS + static_cast<double>( c.m_orbit * std::cos( c.m_orbitA ) );
            c.m_x = c.m_orbit * std::sin( c.m_orbitA );
            c.m_yaw = std::atan2( std::cos( c.m_orbitA ) * c.m_orbitW, -std::sin( c.m_orbitA ) * c.m_orbitW );
            continue;
        }
        if ( c.m_swim )
        {
            c.m_s += static_cast<double>( c.m_speed ) * dt;
            c.m_x += c.m_vx * dt;
            continue;
        }
        const Segment &seg = track.segment( static_cast<long>( c.m_s / SegmentLength ) );
        const int      lanes = seg.m_lanes;
        if ( c.m_animal )
        {
            // Across the road at its own pace; cattle and sheep stop in the middle for a while.
            const PropKindE kind = models.m_traffic[static_cast<size_t>( c.m_model )].m_animal;
            if ( !c.m_waited && std::abs( c.m_x ) < 1.6f &&
                 ( kind == PropKindE::COW || kind == PropKindE::SHEEP || kind == PropKindE::YAK ) )
            {
                c.m_waited = true;
                if ( m_rng.chance( 0.5f ) ) c.m_wait = m_rng.range( 2.0f, 5.0f );
            }
            if ( c.m_wait > 0.0f )
                c.m_wait -= dt;
            else
                c.m_x += c.m_vx * dt;
            c.m_speed = approach( c.m_speed, 0.0f, 3.0f, dt );
            c.m_s += static_cast<double>( c.m_speed ) * dt;
            c.m_lane = std::abs( c.m_x ) < seg.m_halfWidth + 0.8f ? laneOf( c.m_x, lanes, seg.m_halfWidth ) : -1;
            if ( c.m_hitTimer > 0.0f ) c.m_hitTimer -= dt;
            continue;
        }
        // The road gained or lost lanes (a fork widening or narrowing it): the lane is where the car is, not its number.
        if ( c.m_lanes != lanes )
        {
            if ( c.m_lanes != 0 ) c.m_lane = laneOf( c.m_x, lanes, seg.m_halfWidth );
            c.m_lanes = static_cast<uint8_t>( lanes );
        }
        if ( c.m_lane >= lanes ) c.m_lane = lanes - 1;
        // On a road of two directions each keeps to its half: the lanes lo..hi are this car's. Its fast lane is the
        // one at the middle of the road, its slow side its edge. `dir` is its way along the road.
        int       oncFirst = 0, oncLast = -1;
        const int onc = oncomingLanes( track, seg, oncFirst, oncLast );
        if ( c.m_oncoming && onc == 0 )
        {
            c.m_gone = true;   // no road for it any more (the setting was turned off)
            continue;
        }
        int lo = 0, hi = lanes - 1;
        if ( c.m_oncoming )
        {
            lo = oncFirst;
            hi = oncLast;
        }
        else if ( onc > 0 )
        {
            ( oncFirst == 0 ? lo : hi ) = oncFirst == 0 ? oncLast + 1 : oncFirst - 1;
        }
        const double dir = c.m_oncoming ? -1.0 : 1.0;
        const bool   innerLow = onc > 0 ? ( oncFirst == 0 ) != c.m_oncoming : !keepLeft;
        const int    fastLane = innerLow ? lo : hi, slowLane = innerLow ? hi : lo;
        const float  slowSide = innerLow ? 1.0f : -1.0f;
        const double roomAt = c.m_oncoming ? c.m_s - 10.0 : c.m_s;   // laneFree looks further ahead than behind
        auto         closedAhead = [&]( int lane, double distance )
        { return laneClosedAhead( track, c.m_oncoming ? c.m_s - distance : c.m_s, lane, distance ); };
        const int laneBefore = c.m_lane;
        float     want = c.m_cruise;
        // Out of its half (the road became one of two directions, or roadworks sent it across): back as soon as it can.
        if ( ( c.m_lane < lo || c.m_lane > hi ) && c.m_overtake != 2 )
        {
            const int home = std::clamp( c.m_lane, lo, hi );
            if ( !closedAhead( home, 140.0 ) ) c.m_lane = home;
        }
        // Slow in time for the tight bends ahead (the hairpins of a pass).
        for ( double d = 0.0; d < 200.0; d += SegmentLength )
        {
            const float k =
                std::abs( track.segment( static_cast<long>( ( c.m_s + dir * d ) / SegmentLength ) ).m_curve );
            if ( k > 0.004f ) want = std::min( want, std::sqrt( 4.0f / k + 3.2f * static_cast<float>( d ) ) );
        }
        // The siren keeps to the fast lane, the others pull over to the slow side's edge.
        if ( c.m_siren && !closedAhead( fastLane, 150.0 ) ) c.m_lane = fastLane;
        if ( c.m_yield > 0.0f )
        {
            c.m_yield -= dt;
            want = c.m_cruise * 0.6f;
            if ( !closedAhead( slowLane, 120.0 ) ) c.m_lane = slowLane;
        }
        // A fork's two carriageways (from where the road widens for them): no changing to the other one. Nor to the
        // other half of a road of two directions.
        const bool median =
            ( seg.m_flags & SEG_MEDIAN ) || ( ( seg.m_flags & SEG_FORK ) && lanes >= 4 && lanes % 2 == 0 );
        auto sameSide = [&]( int l ) { return !median || ( l * 2 < lanes ) == ( c.m_lane * 2 < lanes ); };
        auto ownLane = [&]( int l ) { return l >= lo && l <= hi && sameSide( l ); };
        // A look in the mirror first: no moving over in front of the player coming up fast in that lane, nor beside
        // the player (and what comes the other way not into the lane the player drives in).
        const float laneW = 2.0f * seg.m_halfWidth / static_cast<float>( lanes );
        auto        mirror = [&]( int l )
        {
            if ( std::abs( player.m_x - laneCenter( l, lanes, seg.m_halfWidth ) ) >
                 0.5f * ( laneW + player.m_spec.m_width ) )
                return true;
            const double ds = ( player.m_s - c.m_s ) * dir;   // the player ahead of it (> 0) or behind
            const double body = 0.5 * static_cast<double>( c.m_length + player.m_spec.m_length );
            if ( c.m_oncoming ) return ds < -body - 6.0 || ds > 220.0;
            if ( ds >= 0.0 ) return ds > body + 4.0;
            return -ds > body + 6.0 + 1.8 * static_cast<double>( std::max( 0.0f, player.m_speed - c.m_speed ) );
        };
        auto roomIn = [&]( int l ) { return laneFree( l, roomAt, &c ) && mirror( l ); };
        // Where the lines are a suggestion: now and then another lane, for no reason.
        if ( c.m_chaos && !c.m_swarm && c.m_yield <= 0.0f && m_rng.chance( dt * 0.12f ) )
        {
            const int l = c.m_lane + ( m_rng.chance( 0.5f ) ? 1 : -1 );
            if ( l >= 0 && l < lanes && ownLane( l ) && roomIn( l ) && !closedAhead( l, 100.0 ) ) c.m_lane = l;
        }
        // Closed lane ahead: move over, across the middle of a road of two directions if it must (what comes the other
        // way waits at the roadworks' end, and never crosses itself).
        if ( closedAhead( c.m_lane, 120.0 ) )
        {
            for ( int d : { -1, 1 } )
            {
                const int l = c.m_lane + d;
                if ( l >= 0 && l < lanes && ( c.m_oncoming ? ownLane( l ) : sameSide( l ) ) &&
                     !closedAhead( l, 140.0 ) && roomIn( l ) )
                {
                    c.m_lane = l;
                    break;
                }
            }
            if ( closedAhead( c.m_lane, 50.0 ) ) want = 6.0f;
        }
        // A slower car ahead: overtake or follow. The siren only slows behind whatever is still in its way.
        if ( c.m_siren )
        {
            auto inWay = [&]( double s, float x, float width, float speed )
            {
                const double gap = s - c.m_s;
                if ( gap > 0.0 && gap < 25.0 + c.m_speed * 0.8 &&
                     std::abs( x - c.m_x ) < ( width + c.m_width ) * 0.5f + 0.3f )
                    want = std::min( want, speed );
            };
            for ( const TrafficCar &o : m_cars )
                if ( &o != &c && !o.m_swim && !o.m_oncoming ) inWay( o.m_s, o.m_x, o.m_width, o.m_speed );
            inWay( player.m_s, player.m_x, player.m_spec.m_width, player.m_speed );
        }
        float braking = 1.6f;
        if ( c.m_oncoming && c.m_overtake > 0 )
        {
            // In a hurry behind slower traffic: out into the player's half to overtake it while nothing comes there
            // for a good while and the line down the middle is broken; back in when past, at once when something
            // comes (or, if it cannot yet, it drops back behind).
            const int    outLane = innerLow ? lo - 1 : hi + 1;
            const float  laneW2 = seg.m_halfWidth / static_cast<float>( lanes );   // half a lane
            const double body = 0.5 * static_cast<double>( c.m_length ) + 2.0;
            const TrafficCar *leader = nullptr;
            double            lead = 1e9;   // how far ahead of it (its way) the one it overtakes is
            for ( const TrafficCar &o : m_cars )
            {
                if ( &o == &c || !o.m_oncoming || o.m_animal || o.m_lane != fastLane ) continue;
                const double g = c.m_s - o.m_s;
                if ( g > -( body + 0.5 * o.m_length + 40.0 ) && g < 60.0 && std::abs( g ) < std::abs( lead ) )
                {
                    lead = g;
                    leader = &o;
                }
            }
            // How soon it would meet what drives the other way in the lane it overtakes in (the player too).
            float meet = 1e9f;
            auto  coming = [&]( double s, float x, float speed )
            {
                if ( std::abs( x - laneCenter( outLane, lanes, seg.m_halfWidth ) ) > laneW2 + 1.0f ) return;
                const double g = c.m_s - s;
                if ( g > -20.0 ) meet = std::min( meet, static_cast<float>( g ) / std::max( 5.0f, c.m_speed + speed ) );
            };
            if ( outLane >= 0 && outLane < lanes )
            {
                for ( const TrafficCar &o : m_cars )
                    if ( !o.m_oncoming && !o.m_animal && !o.m_swim && !o.m_gone ) coming( o.m_s, o.m_x, o.m_speed );
                coming( player.m_s, player.m_x, std::max( 0.0f, player.m_speed ) );
            }
            const float  toPlayer = c.m_s > player.m_s ? static_cast<float>( c.m_s - player.m_s ) /
                                                            std::max( 5.0f, c.m_speed + std::max( 0.0f, player.m_speed ) )
                                                      : 0.0f;
            const double ahead = std::min( 260.0, static_cast<double>( c.m_speed ) * 9.0 );
            bool         lined = false;   // a solid line on its way
            for ( double d = 0.0; d <= ahead && !lined; d += 10.0 )
                lined = track.noPassing( static_cast<long>( ( c.m_s - d ) / SegmentLength ) );
            if ( c.m_overtake == 1 )
            {
                // Out when it is held up, sees the way clear for a good while, and the player sees it coming.
                const bool held = leader && lead > 0.0 && lead < body + 0.5 * leader->m_length + 30.0 &&
                                  leader->m_speed < c.m_cruise - 4.0f;
                if ( held && outLane >= 0 && outLane < lanes && !lined && !closedAhead( outLane, 220.0 ) &&
                     meet > 7.0f && toPlayer > 5.0f && toPlayer < 12.0f )
                {
                    c.m_lane = outLane;
                    c.m_overtake = 2;
                }
            }
            else
            {
                const bool past = !leader || lead < -( body + 0.5 * leader->m_length + 5.0 );
                const bool danger = meet < 3.2f || outLane < 0 || outLane >= lanes;
                bool       room = true;   // its own lane free beside it
                for ( const TrafficCar &o : m_cars )
                    if ( &o != &c && o.m_oncoming && o.m_lane == fastLane &&
                         std::abs( o.m_s - c.m_s ) < body + 0.5 * o.m_length + 3.0 )
                        room = false;
                if ( ( past || danger ) && room )
                {
                    c.m_lane = fastLane;
                    c.m_overtake = 0;
                    c.m_dodge = 1.0f;   // back in smartly
                }
                else if ( danger && leader )
                {
                    want = std::min( want, leader->m_speed - 8.0f );   // drop back behind it
                    braking = 3.0f;
                }
            }
        }
        for ( const TrafficCar &o : m_cars )
        {
            if ( &o == &c || c.m_siren || ( c.m_swarm && o.m_swarm ) ) continue;
            if ( o.m_oncoming && o.m_overtake == 2 && !c.m_oncoming && o.m_lane == c.m_lane )
            {
                // An overtaker coming at it in its lane: it slows to let it back in.
                const double gap = o.m_s - c.m_s;
                if ( gap > 0.0 && gap < 30.0 + 2.5 * static_cast<double>( c.m_speed + o.m_speed ) )
                    want = std::min( want, c.m_cruise * 0.5f );
                continue;
            }
            if ( !o.m_animal && o.m_oncoming != c.m_oncoming ) continue;   // the other direction's are not in its way
            const bool leaving = c.m_signalWait > 0.0f && o.m_lane == c.m_laneFrom && o.m_lane != c.m_lane;
            if ( o.m_lane != c.m_lane && !leaving ) continue;
            const double gap = ( o.m_s - c.m_s ) * dir;
            if ( gap > 0.0 && gap < 20.0 + c.m_speed * 1.2 && o.m_speed < want && !o.m_siren && c.m_yield <= 0.0f )
            {
                if ( leaving )
                {
                    // Still behind it until it has moved over.
                    want = std::min( want, o.m_speed - ( gap < 12.0 ? 3.0f : 0.0f ) );
                    continue;
                }
                // Overtaking on the side of its fast lane: the left, or the right where they keep left.
                const int l = c.m_lane + ( m_rng.chance( 0.8f ) == innerLow ? -1 : 1 );
                if ( l >= 0 && l < lanes && ownLane( l ) && roomIn( l ) && !closedAhead( l, 100.0 ) )
                    c.m_lane = l;
                else
                    want = std::min( want, o.m_speed - ( gap < 12.0 ? 3.0f : 0.0f ) );
            }
        }
        const float reach = ( c.m_width + player.m_spec.m_width ) * 0.5f;
        const float apart = std::abs( player.m_x - c.m_x );
        if ( c.m_oncoming )
        {
            // The player near its half, coming at it: it keeps to its edge to leave room. In its lane: it flashes its
            // lights and sounds its horn; and it brakes for a car that stands in its way.
            const double gap = c.m_s - player.m_s;
            const float  closing = c.m_speed + std::max( 0.0f, player.m_speed );
            if ( gap > 0.0 && gap < std::max( 70.0, 3.0 * static_cast<double>( closing ) ) && apart < reach + 1.3f )
                c.m_dodge = 0.8f;
            if ( gap > 0.0 && gap < std::max( 50.0, 2.2 * static_cast<double>( closing ) ) && apart < reach + 0.2f )
            {
                if ( c.m_flash <= 0.0f ) c.m_flash = 1.2f;
                if ( c.m_honk <= 0.0f && gap < 160.0 )
                {
                    c.m_honk = 4.0f;
                    if ( ev.m_horn < 0.9f )
                    {
                        ev.m_horn = 0.9f;
                        ev.m_hornPan = clampf( ( c.m_x - player.m_x ) / 4.0f, -1.0f, 1.0f );
                        ev.m_hornPitch = doppler( closing );
                    }
                }
            }
            if ( gap > 0.0 && gap < 12.0 + static_cast<double>( c.m_speed ) * 1.5 && apart < reach + 0.3f )
            {
                want = 0.0f;
                braking = 3.0f;
            }
            // Roadworks ahead of it: one lane for both directions, and it waits well before their end until the
            // others are through.
            for ( double d = 0.0; d < 90.0 && want > 0.0f; d += SegmentLength )
                if ( track.segment( static_cast<long>( ( c.m_s - d ) / SegmentLength ) ).m_blockedLanes ) want = 0.0f;
        }
        else if ( onc > 0 && meeting && !c.m_siren )
        {
            // The player squeezing past between it and what comes the other way: it keeps to its edge too.
            const double ds = player.m_s - c.m_s;
            if ( ds > -30.0 && ds < 12.0 && ( c.m_x - player.m_x ) * slowSide > 0.8f && apart < reach + 1.6f )
                c.m_dodge = 0.8f;
        }
        if ( c.m_flash > 0.0f ) c.m_flash -= dt;
        c.m_braking = want < c.m_speed - 1.5f;
        c.m_speed = approach( c.m_speed, std::max( 0.0f, want ), want < c.m_speed ? braking : 0.5f, dt );
        // Before it changes lanes a car indicates for a moment and only then moves over (where the lines are a
        // suggestion nobody does, and whoever makes way for a siren or a flash of the headlights goes at once).
        if ( c.m_lane != laneBefore && !c.m_chaos && !c.m_siren && c.m_yield <= 0.0f )
        {
            c.m_signal = c.m_lane > laneBefore ? 1 : -1;
            if ( c.m_signalWait <= 0.0f ) c.m_laneFrom = laneBefore;
            c.m_signalWait = 0.55f;
        }
        if ( c.m_yield > 0.0f )
        {
            c.m_signal = slowSide > 0.0f ? 1 : -1;
            c.m_signalWait = 0.0f;
        }
        if ( c.m_signalWait > 0.0f ) c.m_signalWait -= dt;
        const int laneNow = c.m_signalWait > 0.0f ? std::clamp( c.m_laneFrom, 0, lanes - 1 ) : c.m_lane;
        float     target = laneCenter( laneNow, lanes, seg.m_halfWidth ) + c.m_xOff;
        if ( c.m_chaos ) target += std::sin( m_clock * 0.3f + c.m_weave ) * ( c.m_swarm ? 0.4f : 0.75f );
        target = clampf( target, -seg.m_halfWidth + c.m_width * 0.5f, seg.m_halfWidth - c.m_width * 0.5f );
        const float edge = seg.m_halfWidth - c.m_width * 0.5f + 0.5f;   // as far out as the verge allows
        if ( c.m_yield > 0.0f )
            target = slowSide * edge;
        else if ( c.m_siren && lanes == 1 )
            target = -slowSide * seg.m_halfWidth * 0.4f;
        else if ( c.m_siren && lo == hi )
            target = slowSide * seg.m_halfWidth * 0.22f;   // its half has one lane: along the middle of the road
        else if ( c.m_dodge > 0.0f )
            target = slowSide > 0.0f ? std::min( target + 1.1f, edge ) : std::max( target - 1.1f, -edge );
        if ( c.m_dodge > 0.0f ) c.m_dodge -= dt;
        if ( seg.m_flags & SEG_FORK )
        {
            // On the carriageway the player did not take: follow it as it turns away, then leave.
            const Fork   fork = track.fork( seg.m_stage );
            const double v = c.m_s - fork.m_at - ForkMedian;
            if ( fork.m_chosen != 0 && v > 0.0 && c.m_x * static_cast<float>( fork.m_chosen ) < 0.0f )
            {
                const double w = std::min( v, ForkPeel ) / ForkPeel;
                target = static_cast<float>( -fork.m_chosen ) * static_cast<float>( 3.0 + 130.0 * w * w );
                c.m_x = target;
                c.m_gone = v > ForkPeel;
            }
        }
        // Making room it moves over smartly.
        const float quick = c.m_dodge > 0.0f ? 2.0f : 1.0f;
        c.m_vx = clampf( ( target - c.m_x ) * 1.3f * quick, -2.6f * quick, 2.6f * quick );
        c.m_x += c.m_vx * dt;
        if ( c.m_signal != 0 && c.m_signalWait <= 0.0f && c.m_yield <= 0.0f && std::abs( target - c.m_x ) < 0.3f )
            c.m_signal = 0;   // arrived in its lane
        c.m_yaw = std::atan2( c.m_vx, static_cast<float>( dir ) * std::max( c.m_speed, 5.0f ) );
        c.m_s += dir * static_cast<double>( c.m_speed ) * dt;
        if ( c.m_hitTimer > 0.0f ) c.m_hitTimer -= dt;
    }

    // Contact with the player, overtakes, near misses.
    const float pl = player.m_spec.m_length, pw = player.m_spec.m_width;
    for ( TrafficCar &c : m_cars )
    {
        if ( c.m_swim ) continue;
        const double ds = player.m_s - c.m_s;
        const float  dx = player.m_x - c.m_x;
        const float  halfL = ( pl + c.m_length ) * 0.5f;
        const float  halfW = ( pw + c.m_width ) * 0.5f * 0.92f;
        const bool   lowEnough = !player.m_airborne || player.m_y - track.surfaceHeight( player.m_s, player.m_x ) < 1.2;
        if ( std::abs( ds ) < halfL && std::abs( dx ) < halfW && lowEnough )
        {
            const float rel = player.m_speed - c.m_speed;
            const float side = dx >= 0.0f ? 1.0f : -1.0f;
            if ( c.m_oncoming )
            {
                // Head-on, or brushing past each other: either way they end up side by side and go on.
                const float closing = player.m_speed + c.m_speed, deep = halfW - std::abs( dx );
                if ( closing > 14.0f && deep > 0.3f && player.m_crashTimer <= 0.0f )
                {
                    player.crash( 0.2f, side * deep );
                    c.m_speed = 0.0f;
                    ev.m_hit = 1.0f;
                    ++ev.m_headOn;
                }
                else
                {
                    player.m_x += side * deep;
                    player.m_vx = side * 3.0f;
                    player.m_speed *= 0.95f;
                    c.m_speed *= 0.8f;
                    ev.m_hit = std::max( ev.m_hit, 0.4f );
                }
                c.m_x -= side * 0.25f;
            }
            else if ( std::abs( ds ) < halfL * 0.6f )
            {
                // Side by side: shove apart.
                player.m_x += side * ( halfW - std::abs( dx ) ) * 0.9f;
                player.m_vx = side * 3.0f;
                player.m_speed *= 0.97f;
                c.m_x -= side * 0.25f;
                ev.m_hit = std::max( ev.m_hit, 0.35f );
            }
            else if ( ds < 0.0 )
            {
                // Rear-ended it.
                if ( rel > 18.0f )
                    player.crash( 0.4f, side * 1.4f );
                else
                    player.m_speed = std::max( 0.0f, c.m_speed - 2.0f );
                player.m_s = c.m_s - halfL;
                c.m_speed += std::max( 0.0f, rel ) * 0.4f;
                ev.m_hit = std::max( ev.m_hit, clampf( rel / 30.0f, 0.3f, 1.0f ) );
            }
            else
            {
                // It ran into the player.
                c.m_speed = std::max( 0.0f, player.m_speed - 1.0f );
                c.m_s = player.m_s - halfL;
                player.m_speed += 1.5f;
                ev.m_hit = std::max( ev.m_hit, 0.3f );
            }
            c.m_hitTimer = 1.5f;
            player.m_shake = std::max( player.m_shake, ev.m_hit );
            if ( !c.m_animal && !c.m_siren && c.m_honk <= 0.0f )
            {
                // Hit, it honks.
                c.m_honk = 3.0f;
                ev.m_horn = 1.0f;
                ev.m_hornPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
                ev.m_hornPitch = doppler( ds > 0.0 ? c.along() - player.m_speed : player.m_speed - c.along() );
            }
        }
        if ( c.m_honk > 0.0f ) c.m_honk -= dt;
        if ( c.m_chaos && !c.m_animal && !c.m_siren && c.m_honk <= 0.0f && std::abs( ds ) < 40.0 &&
             std::abs( dx ) < 5.0f && m_rng.chance( dt * 0.35f ) )
        {
            // Where everyone honks: at the player close by, for being there.
            c.m_honk = m_rng.range( 4.0f, 9.0f );
            const float strength = 0.35f + 0.5f * ( 1.0f - static_cast<float>( std::abs( ds ) ) / 40.0f );
            if ( strength > ev.m_horn )
            {
                ev.m_horn = strength;
                ev.m_hornPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
                ev.m_hornPitch = doppler( ds > 0.0 ? c.m_speed - player.m_speed : player.m_speed - c.m_speed );
            }
        }
        if ( c.m_animal ) continue;
        if ( c.m_oncoming )
        {
            // Gone by: no overtake. Missed close and fast it is a close call; and the air thumps as it passes.
            if ( !c.m_passed && ds > halfL )
            {
                c.m_passed = true;
                ++ev.m_met;
                const float closing = player.m_speed + c.m_speed;
                if ( std::abs( dx ) < halfW + 1.2f && player.m_speed > 30.0f && c.m_hitTimer <= 0.0f &&
                     player.m_crashTimer <= 0.0f )
                    ++ev.m_closeCalls;
                if ( models.m_traffic[static_cast<size_t>( c.m_model )].m_truck && std::abs( dx ) < 5.5f &&
                     closing > 22.0f && !player.m_airborne )
                {
                    // A lorry or a bus: its blast shoves the car a little away from it.
                    const float k = clampf( ( closing - 22.0f ) / 30.0f + 0.3f, 0.3f, 1.0f ) * ( 1.0f - std::abs( dx ) / 6.5f );
                    ev.m_gust = std::max( ev.m_gust, k );
                    player.m_vx += ( dx >= 0.0f ? 1.0f : -1.0f ) * 1.5f * k;
                    player.m_shake = std::max( player.m_shake, 0.35f * k );
                }
                const float strength = 0.8f * clampf( closing / 50.0f, 0.2f, 1.0f ) * ( 1.0f - std::abs( dx ) / 10.0f );
                if ( std::abs( dx ) < 9.0f && closing > 8.0f && strength > ev.m_whoosh )
                {
                    ev.m_whoosh = strength;
                    ev.m_whooshPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
                    ev.m_whooshPitch = clampf( 0.7f + closing / 80.0f, 0.7f, 1.6f );
                }
            }
            continue;
        }
        if ( !c.m_passed && ds > halfL )
        {
            c.m_passed = true;
            ++ev.m_overtakes;
            if ( std::abs( dx ) < halfW + 1.4f && player.m_speed > 30.0f && c.m_hitTimer <= 0.0f ) ++ev.m_nearMisses;
            const float rel = player.m_speed - c.m_speed;
            if ( std::abs( dx ) < 9.0f && rel > 8.0f )
            {
                const float strength = clampf( rel / 50.0f, 0.2f, 1.0f ) * ( 1.0f - std::abs( dx ) / 10.0f );
                if ( strength > ev.m_whoosh )
                {
                    ev.m_whoosh = strength;
                    ev.m_whooshPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
                    ev.m_whooshPitch = clampf( 0.7f + rel / 80.0f, 0.7f, 1.6f );
                }
            }
        }
        else if ( c.m_passed && ds < -halfL )
        {
            c.m_passed = false;
            if ( c.m_siren && std::abs( dx ) > halfW + 0.6f && player.m_speed > 3.0f ) ++ev.m_gaveWay;
        }
    }
    std::erase_if(
        m_cars,
        [&]( const TrafficCar &c )
        {
            const bool across =
                c.m_animal && c.m_x * c.m_vx > 0.0f &&
                std::abs( c.m_x ) > track.segment( static_cast<long>( c.m_s / SegmentLength ) ).m_halfWidth + 9.0f;
            const Crossing xc = c.m_swim ? Crossing() : track.crossingAt( c.m_s );
            const bool aboard = xc.m_type != CrossingE::NONE && c.m_s > xc.m_board - 25.0 && c.m_s < xc.m_land - 20.0;
            return across || aboard || c.m_gone || ( c.m_swim && std::abs( c.m_x ) > 160.0f ) ||
                   c.m_s < player.m_s - ( c.m_siren ? 400.0 : 140.0 ) || c.m_s > player.m_s + 1700.0;
        } );

    // Cones and boards.
    const TrackFrame pf = track.frameAt( player.m_s );
    for ( Obstacle &o : m_obstacles )
    {
        if ( o.m_knocked )
        {
            o.m_life += dt;
            o.m_vel.y -= 9.81f * dt;
            o.m_pos.m_x += static_cast<double>( o.m_vel.x * dt );
            o.m_pos.m_y += static_cast<double>( o.m_vel.y * dt );
            o.m_pos.m_z += static_cast<double>( o.m_vel.z * dt );
            o.m_angle += o.m_spinRate * dt;
            if ( o.m_pos.m_y < o.m_groundY && o.m_vel.y < 0.0f )
            {
                o.m_pos.m_y = o.m_groundY;
                o.m_vel = Vector3{ o.m_vel.x * 0.6f, -o.m_vel.y * 0.35f, o.m_vel.z * 0.6f };
                o.m_spinRate *= 0.6f;
            }
            continue;
        }
        const double ds = player.m_s - o.m_s;
        const float  dx = player.m_x - o.m_x;
        const float  reachL = pl * 0.5f + ( o.m_kind ? 0.3f : 0.25f );
        const float  reachW = pw * 0.5f + ( o.m_kind ? 1.0f : 0.25f );
        if ( std::abs( ds ) < reachL && std::abs( dx ) < reachW && !player.m_airborne )
        {
            o.m_knocked = true;
            o.m_pos = track.pointAt( o.m_s, o.m_x, 0.0f );
            o.m_groundY = static_cast<float>( o.m_pos.m_y );
            const float side = dx >= 0.0f ? -1.0f : 1.0f;
            o.m_vel = Vector3Add( Vector3Scale( pf.m_forward, player.m_speed * m_rng.range( 0.7f, 1.0f ) ),
                                  Vector3Add( Vector3{ 0, m_rng.range( 3.0f, 7.0f ), 0 },
                                              Vector3Scale( pf.m_right, side * m_rng.range( 1.0f, 5.0f ) ) ) );
            o.m_axis =
                Vector3Normalize( Vector3{ m_rng.range( -1, 1 ), m_rng.range( -0.3f, 0.3f ), m_rng.range( -1, 1 ) } );
            o.m_spinRate = m_rng.range( 8.0f, 16.0f );
            player.m_speed *= o.m_kind ? 0.82f : 0.97f;
            player.m_shake = std::max( player.m_shake, o.m_kind ? 0.5f : 0.18f );
            ++ev.m_cones;
        }
    }
    std::erase_if( m_obstacles, [&]( const Obstacle &o )
                   { return o.m_life > 3.0f || ( !o.m_knocked && o.m_s < player.m_s - 60.0 ); } );
}

}   // namespace sunstreak
