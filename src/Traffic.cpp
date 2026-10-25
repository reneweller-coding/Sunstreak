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
    for( long i = first; i <= last; ++i )
        if( track.segment( i ).m_blockedLanes & ( 1u << lane ) ) return true;
    return false;
}

} // namespace

void Traffic::reset( uint64_t seed, double startS )
{
    m_cars.clear();
    m_obstacles.clear();
    m_rng = Rng( seed ^ 0x7AFF1Cull );
    m_spawnS = startS + 120.0;
    m_scanned = 0;
    m_sirenS = startS + static_cast<double>( m_rng.range( 1500.0f, 5000.0f ) );
}

bool Traffic::laneFree( int lane, double s, const TrafficCar *self ) const
{
    for( const TrafficCar &c : m_cars )
    {
        if( &c == self || c.m_lane != lane ) continue;
        if( c.m_s > s - 18.0 && c.m_s < s + 28.0 ) return false;
    }
    return true;
}

void Traffic::scanRoadworks( const Track &track, double upTo )
{
    const long last = std::min( static_cast<long>( upTo / SegmentLength ), track.endIndex() - 2 );
    m_scanned = std::max( m_scanned, track.firstIndex() + 1 );
    for( ; m_scanned <= last; ++m_scanned )
    {
        const Segment &seg = track.segment( m_scanned );
        if( !( seg.m_flags & SEG_ROADWORKS ) || seg.m_blockedLanes == 0 ) continue;
        const int   lanes = seg.m_lanes;
        const bool  leftClosed = seg.m_blockedLanes & 1u;
        const float w = 2.0f * seg.m_halfWidth / static_cast<float>( lanes );
        const float boundary = leftClosed ? -seg.m_halfWidth + w : seg.m_halfWidth - w;
        const double s = static_cast<double>( m_scanned ) * SegmentLength;
        const bool  zoneStart = !( track.segment( m_scanned - 1 ).m_flags & SEG_ROADWORKS );
        if( zoneStart )
        {
            // A taper of cones from the road's edge to the lane line, the arrow board in the closed lane.
            const float edge = leftClosed ? -seg.m_halfWidth + 0.4f : seg.m_halfWidth - 0.4f;
            for( int k = 0; k < 5; ++k )
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
        if( m_scanned % 2 == 0 )
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
    const BiomeMix  mix = track.biomeMix( player.m_s + 900.0 );
    const float     density = lerpf( biomeDef( mix.m_from ).m_traffic, biomeDef( mix.m_to ).m_traffic, mix.m_t ) *
                          std::min( 1.8f, 0.85f + 0.12f * static_cast<float>( player.m_s / 5000.0 ) ) * tuning().m_traffic * m_density;
    if( m_spawnS < player.m_s + 220.0 ) m_spawnS = player.m_s + 220.0;
    // The country decides the side: slower cars keep right, or left where they drive on the left.
    const int          playerStage = static_cast<int>( player.m_s / StageLength );
    const bool         keepLeft = countryInfo( biomeDef( track.stageBiome( playerStage ) ).m_country ).m_left;
    const BiomeDef    &spawnPlace = biomeDef( track.stageBiome( static_cast<int>( ( player.m_s + 900.0 ) / StageLength ) ) );
    const CountryInfo &spawnLand = countryInfo( spawnPlace.m_country );
    const uint32_t     fleetBit = 1u << static_cast<unsigned>( spawnLand.m_fleet );
    const uint8_t      styles = spawnPlace.m_country ? roadCustoms( spawnPlace.m_country ).m_styles : 0;
    m_clock += dt;

    // Animals of the place cross the road now and then, out in the country.
    if( m_animalS < player.m_s + 500.0 ) m_animalS = player.m_s + 500.0 + m_rng.range( 300.0f, 1200.0f );
    if( m_density > 0.0f && player.m_s + 800.0 > m_animalS )
    {
        std::vector<int> fauna;
        if( spawnPlace.m_lanes <= 2 && spawnPlace.m_country )
            for( const PropRule &r : spawnPlace.m_props )
                for( size_t i = 0; i < models.m_traffic.size(); ++i )
                    if( r.m_kind != PropKindE::LANDMARK && models.m_traffic[i].m_animal == r.m_kind ) fauna.push_back( static_cast<int>( i ) );
        const Segment &seg = track.segment( static_cast<long>( m_animalS / SegmentLength ) );
        if( !fauna.empty() && !( seg.m_flags & ( SEG_TUNNEL | SEG_BRIDGE ) ) && m_animalS > StartLine + 300.0 )
        {
            const int          model = fauna[static_cast<size_t>( m_rng.rangeInt( 0, static_cast<int>( fauna.size() ) - 1 ) )];
            const VehicleModel &vm = models.m_traffic[static_cast<size_t>( model )];
            const PropKindE    kind = vm.m_animal;
            const bool herd = kind == PropKindE::SHEEP || kind == PropKindE::COW || kind == PropKindE::ZEBRA || kind == PropKindE::CAMEL ||
                              kind == PropKindE::BISON || kind == PropKindE::REINDEER || kind == PropKindE::YAK || kind == PropKindE::LLAMA;
            const float pace = kind == PropKindE::KANGAROO ? 7.0f : kind == PropKindE::ZEBRA ? 3.0f : kind == PropKindE::HORSE ? 2.5f
                             : kind == PropKindE::MOOSE || kind == PropKindE::REINDEER ? 2.0f : kind == PropKindE::SHEEP ? 1.5f : 1.1f;
            const float side = m_rng.chance( 0.5f ) ? 1.0f : -1.0f;
            const int   count = herd ? m_rng.rangeInt( 1, 3 ) : 1;
            for( int k = 0; k < count; ++k )
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
        const BiomeDef &sea = biomeDef( track.stageBiome( static_cast<int>( ( player.m_s + 500.0 ) / StageLength ) ) );
        if( m_swimS < player.m_s + 200.0 ) m_swimS = player.m_s + 200.0 + m_rng.range( 0.0f, 80.0f );
        if( !sea.m_swim.empty() && sea.m_crossing == CrossingE::NONE && player.m_s + 460.0 > m_swimS )
        {
            float total = 0.0f;
            for( const SwimRule &r : sea.m_swim ) total += r.m_weight;
            float           pick = m_rng.range( 0.0f, total );
            const SwimRule *rule = &sea.m_swim.front();
            for( const SwimRule &r : sea.m_swim )
            {
                if( pick < r.m_weight )
                {
                    rule = &r;
                    break;
                }
                pick -= r.m_weight;
            }
            const int model = rule->m_shape >= 0 && rule->m_shape < static_cast<int>( models.m_creatureModel.size() ) ? models.m_creatureModel[static_cast<size_t>( rule->m_shape )] : -1;
            if( model >= 0 && rule->m_circle )
            {
                // Sharks round the tube: circling over it a little ahead, the car passing under them.
                const int     count = m_rng.rangeInt( rule->m_countMin, std::max( 2, rule->m_countMax ) );
                const Segment &seaSeg = track.segment( static_cast<long>( m_swimS / SegmentLength ) );
                const float   top = ( seaSeg.m_halfWidth + RumbleWidth + 2.6f ) * 0.82f + 0.2f;
                const float   radius = m_rng.range( 12.0f, 18.0f ), speed = m_rng.range( 4.0f, 6.5f ) * ( m_rng.chance( 0.5f ) ? 1.0f : -1.0f );
                const float   lift = std::max( top + 2.5f, m_rng.range( rule->m_liftMin, rule->m_liftMax ) );
                for( int k = 0; k < count; ++k )
                {
                    TrafficCar a;
                    a.m_swim = true;
                    a.m_animal = true;
                    a.m_passed = true;
                    a.m_lane = -1;
                    a.m_model = model;
                    a.m_orbit = radius + m_rng.range( -2.0f, 2.0f );
                    a.m_orbitS = m_swimS + 40.0;
                    a.m_orbitA = Tau * static_cast<float>( k ) / static_cast<float>( count ) + m_rng.range( -0.3f, 0.3f );
                    a.m_orbitW = speed / a.m_orbit;
                    a.m_s = a.m_orbitS + static_cast<double>( a.m_orbit * std::cos( a.m_orbitA ) );
                    a.m_x = a.m_orbit * std::sin( a.m_orbitA );
                    a.m_lift = lift + m_rng.range( -1.0f, 1.5f );
                    a.m_length = models.m_traffic[static_cast<size_t>( model )].m_length;
                    a.m_width = models.m_traffic[static_cast<size_t>( model )].m_width;
                    m_cars.push_back( a );
                }
            }
            else if( model >= 0 )
            {
                // A group beside the tunnel, close to the glass, or over it, along the road or across it.
                const int      count = m_rng.rangeInt( rule->m_countMin, rule->m_countMax );
                const float    side = m_rng.chance( 0.5f ) ? 1.0f : -1.0f;
                const float    lift = m_rng.range( rule->m_liftMin, rule->m_liftMax );
                const bool     across = lift > 9.0f && m_rng.chance( 0.5f );
                const float    along = m_rng.range( -6.0f, 14.0f ), vx = across ? -side * m_rng.range( 1.5f, 4.0f ) : 0.0f;
                const Segment &seaSeg = track.segment( static_cast<long>( m_swimS / SegmentLength ) );
                const float    clear = seaSeg.m_halfWidth + RumbleWidth + 2.6f + 1.5f + 0.5f * models.m_traffic[static_cast<size_t>( model )].m_width;
                const float    x0 = across ? side * m_rng.range( 30.0f, 50.0f ) : side * m_rng.range( clear, clear + 14.0f );
                for( int k = 0; k < count; ++k )
                {
                    TrafficCar a;
                    a.m_swim = true;
                    a.m_animal = true;
                    a.m_passed = true;
                    a.m_lane = -1;
                    a.m_model = model;
                    a.m_s = m_swimS + static_cast<double>( m_rng.range( -8.0f, 8.0f ) );
                    a.m_x = x0 + m_rng.range( -4.0f, 4.0f );
                    if( !across ) a.m_x = side * std::max( clear, std::abs( a.m_x ) );
                    a.m_lift = std::max( 0.0f, lift + ( count > 1 ? m_rng.range( -2.0f, 2.0f ) : 0.0f ) );
                    a.m_speed = along;
                    a.m_cruise = along;
                    a.m_vx = vx;
                    a.m_yaw = std::atan2( vx, std::abs( along ) > 0.5f ? along : 0.5f ) + ( along < -0.5f ? Tau * 0.5f : 0.0f );
                    a.m_length = models.m_traffic[static_cast<size_t>( model )].m_length;
                    a.m_width = models.m_traffic[static_cast<size_t>( model )].m_width;
                    m_cars.push_back( a );
                }
            }
            m_swimS += m_rng.range( 10.0f, 36.0f );
        }
    }
    while( m_density > 0.0f && m_spawnS < player.m_s + 1050.0 )
    {
        m_spawnS += static_cast<double>( lerpf( 200.0f, 60.0f, std::min( 1.0f, density / 1.8f ) ) * m_rng.range( 0.55f, 1.45f ) );
        const Segment &seg = track.segment( static_cast<long>( m_spawnS / SegmentLength ) );
        if( m_spawnS < StartLine + 150.0 ) continue;
        const Crossing xc = track.crossingAt( m_spawnS );
        if( xc.m_type != CrossingE::NONE && m_spawnS > xc.m_board - 150.0 && m_spawnS < xc.m_land + 60.0 ) continue;
        TrafficCar c;
        c.m_s = m_spawnS;
        int lane = m_rng.rangeInt( 0, seg.m_lanes - 1 );
        for( int tries = 0; tries < seg.m_lanes && laneClosedAhead( track, c.m_s - 40.0, lane, 80.0 ); ++tries )
            lane = ( lane + 1 ) % seg.m_lanes;
        if( laneClosedAhead( track, c.m_s - 40.0, lane, 80.0 ) || !laneFree( lane, c.m_s, nullptr ) ) continue;
        c.m_lane = lane;
        c.m_x = laneCenter( lane, seg.m_lanes, seg.m_halfWidth );
        c.m_plate = static_cast<uint8_t>( roadCustoms( spawnPlace.m_country ).m_plate );
        // Everyday cars and lorries, and now and then what the region drives: tuk-tuks, yellow cabs, road trains.
        const bool       city = seg.m_lanes >= 3;
        std::vector<int> cars, trucks, local, scooters, tractors;
        int              roadTrain = -1;
        for( size_t i = 0; i < models.m_traffic.size(); ++i )
        {
            const VehicleModel &vm = models.m_traffic[i];
            if( vm.m_animal != PropKindE::COUNT || vm.m_police ) continue;
            if( vm.m_fleets != 0 && vm.m_pace < 0.5f && !vm.m_truck && vm.m_where != 1 ) tractors.push_back( static_cast<int>( i ) );
            if( vm.m_fleets == 0 ) ( vm.m_truck ? trucks : cars ).push_back( static_cast<int>( i ) );
            else if( ( vm.m_fleets & fleetBit ) && spawnPlace.m_country && ( vm.m_where == 0 || ( vm.m_where == 1 ) == city ) )
            {
                local.push_back( static_cast<int>( i ) );
                if( vm.m_width < 1.0f ) scooters.push_back( static_cast<int>( i ) );
                if( vm.m_truck && ( roadTrain < 0 || vm.m_length > models.m_traffic[static_cast<size_t>( roadTrain )].m_length ) ) roadTrain = static_cast<int>( i );
            }
        }
        auto pick = [&]( const std::vector<int> &from ) { return from[static_cast<size_t>( m_rng.rangeInt( 0, static_cast<int>( from.size() ) - 1 ) )]; };
        const bool truck = m_rng.chance( 0.14f );
        c.m_model = !local.empty() && m_rng.chance( 0.38f ) ? pick( local ) : truck ? pick( trucks ) : pick( cars );
        // The country's way of driving: tractors on its country roads, road trains in the outback, swarms of scooters.
        int swarm = 0;
        if( ( styles & StyleTractors ) && !city && !tractors.empty() && m_rng.chance( 0.12f ) ) c.m_model = pick( tractors );
        if( ( styles & StyleRoadTrains ) && !city && roadTrain >= 0 && m_rng.chance( 0.2f ) ) c.m_model = roadTrain;
        if( ( styles & StyleScooters ) && !scooters.empty() && m_rng.chance( 0.3f ) )
        {
            c.m_model = pick( scooters );
            swarm = m_rng.rangeInt( 3, 7 );
        }
        const VehicleModel &vm = models.m_traffic[static_cast<size_t>( c.m_model )];
        c.m_length = vm.m_length;
        c.m_width = vm.m_width;
        const float laneBonus = static_cast<float>( keepLeft ? lane : seg.m_lanes - 1 - lane ) * 2.0f;
        c.m_cruise = ( vm.m_truck ? m_rng.range( 15.0f, 20.0f ) : m_rng.range( 16.0f, 26.0f ) + laneBonus ) * vm.m_pace;
        c.m_speed = c.m_cruise;
        c.m_chaos = ( styles & StyleChaos ) != 0;
        c.m_weave = m_rng.range( 0.0f, Tau );
        const float laneHalf = seg.m_halfWidth / static_cast<float>( seg.m_lanes ) - vm.m_width * 0.5f - 0.1f;
        for( int k = 0; k < std::max( 1, swarm ); ++k )
        {
            TrafficCar b = c;
            b.m_swarm = swarm > 0;
            if( b.m_swarm )
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
        if( swarm > 0 ) m_spawnS += 4.0 * swarm;   // the swarm takes its room
    }
    scanRoadworks( track, player.m_s + 900.0 );

    // Now and then an ambulance or a fire engine on a run: from behind when the player is slower than it, else ahead.
    if( m_density > 0.0f && player.m_s > m_sirenS )
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
        const BiomeDef &place = biomeDef( track.stageBiome( static_cast<int>( c.m_s / StageLength ) ) );
        const int       model = serviceModel( models, place, fire ? "FIRE_ENGINE" : "AMBULANCE" );
        const bool      busy = std::any_of( m_cars.begin(), m_cars.end(), []( const TrafficCar &o ) { return o.m_siren != 0; } );
        const bool      aboard = xc.m_type != CrossingE::NONE && c.m_s > xc.m_board - 150.0 && c.m_s < xc.m_land + 60.0;
        if( model >= 0 && !busy && !aboard && c.m_s > StartLine + 150.0 && !( seg.m_flags & SEG_FORK ) )
        {
            if( behind ) cruise = std::max( cruise, player.m_speed + 6.0f );
            const VehicleModel &vm = models.m_traffic[static_cast<size_t>( model )];
            c.m_model = model;
            c.m_length = vm.m_length;
            c.m_width = vm.m_width;
            c.m_siren = static_cast<uint8_t>( place.m_country && countryInfo( place.m_country ).m_fleet == FleetE::EUROPE ? 2 : 1 );
            c.m_lane = keepLeft ? seg.m_lanes - 1 : 0;
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
    for( const TrafficCar &c : m_cars )
        if( c.m_siren ) sirens.push_back( &c );
    for( TrafficCar &c : m_cars )
    {
        if( c.m_siren || c.m_animal || c.m_swim ) continue;
        for( const TrafficCar *e : sirens )
            if( c.m_s - e->m_s > -8.0 && c.m_s - e->m_s < 160.0 ) c.m_yield = 2.0f;
    }

    // Drive.
    for( TrafficCar &c : m_cars )
    {
        if( c.m_swim && c.m_orbit > 0.0f )
        {
            // Round the circle, heading along it.
            c.m_orbitA += c.m_orbitW * dt;
            c.m_s = c.m_orbitS + static_cast<double>( c.m_orbit * std::cos( c.m_orbitA ) );
            c.m_x = c.m_orbit * std::sin( c.m_orbitA );
            c.m_yaw = std::atan2( std::cos( c.m_orbitA ) * c.m_orbitW, -std::sin( c.m_orbitA ) * c.m_orbitW );
            continue;
        }
        if( c.m_swim )
        {
            c.m_s += static_cast<double>( c.m_speed ) * dt;
            c.m_x += c.m_vx * dt;
            continue;
        }
        const Segment &seg = track.segment( static_cast<long>( c.m_s / SegmentLength ) );
        const int      lanes = seg.m_lanes;
        if( c.m_animal )
        {
            // Across the road at its own pace; cattle and sheep stop in the middle for a while.
            const PropKindE kind = models.m_traffic[static_cast<size_t>( c.m_model )].m_animal;
            if( !c.m_waited && std::abs( c.m_x ) < 1.6f && ( kind == PropKindE::COW || kind == PropKindE::SHEEP || kind == PropKindE::YAK ) )
            {
                c.m_waited = true;
                if( m_rng.chance( 0.5f ) ) c.m_wait = m_rng.range( 2.0f, 5.0f );
            }
            if( c.m_wait > 0.0f ) c.m_wait -= dt;
            else c.m_x += c.m_vx * dt;
            c.m_speed = approach( c.m_speed, 0.0f, 3.0f, dt );
            c.m_s += static_cast<double>( c.m_speed ) * dt;
            c.m_lane = std::abs( c.m_x ) < seg.m_halfWidth + 0.8f ? laneOf( c.m_x, lanes, seg.m_halfWidth ) : -1;
            if( c.m_hitTimer > 0.0f ) c.m_hitTimer -= dt;
            continue;
        }
        // The road gained or lost lanes (a fork widening or narrowing it): the lane is where the car is, not its number.
        if( c.m_lanes != lanes )
        {
            if( c.m_lanes != 0 ) c.m_lane = laneOf( c.m_x, lanes, seg.m_halfWidth );
            c.m_lanes = static_cast<uint8_t>( lanes );
        }
        if( c.m_lane >= lanes ) c.m_lane = lanes - 1;
        float want = c.m_cruise;
        // Slow in time for the tight bends ahead (the hairpins of a pass).
        for( double d = 0.0; d < 200.0; d += SegmentLength )
        {
            const float k = std::abs( track.segment( static_cast<long>( ( c.m_s + d ) / SegmentLength ) ).m_curve );
            if( k > 0.004f ) want = std::min( want, std::sqrt( 4.0f / k + 3.2f * static_cast<float>( d ) ) );
        }
        // The siren keeps to the fast lane, the others pull over to the slow side's edge.
        const int   fastLane = keepLeft ? lanes - 1 : 0, slowLane = lanes - 1 - fastLane;
        const float slowSide = keepLeft ? -1.0f : 1.0f;
        if( c.m_siren && !laneClosedAhead( track, c.m_s, fastLane, 150.0 ) ) c.m_lane = fastLane;
        if( c.m_yield > 0.0f )
        {
            c.m_yield -= dt;
            want = c.m_cruise * 0.6f;
            if( !laneClosedAhead( track, c.m_s, slowLane, 120.0 ) ) c.m_lane = slowLane;
        }
        // A fork's two carriageways (from where the road widens for them): no changing to the other one.
        const bool median = ( seg.m_flags & SEG_MEDIAN ) || ( ( seg.m_flags & SEG_FORK ) && lanes >= 4 && lanes % 2 == 0 );
        auto       sameSide = [&]( int l ) { return !median || ( l * 2 < lanes ) == ( c.m_lane * 2 < lanes ); };
        // Where the lines are a suggestion: now and then another lane, for no reason.
        if( c.m_chaos && !c.m_swarm && c.m_yield <= 0.0f && m_rng.chance( dt * 0.12f ) )
        {
            const int l = c.m_lane + ( m_rng.chance( 0.5f ) ? 1 : -1 );
            if( l >= 0 && l < lanes && sameSide( l ) && laneFree( l, c.m_s, &c ) && !laneClosedAhead( track, c.m_s, l, 100.0 ) ) c.m_lane = l;
        }
        // Closed lane ahead: move over.
        if( laneClosedAhead( track, c.m_s, c.m_lane, 120.0 ) )
        {
            for( int d : { -1, 1 } )
            {
                const int l = c.m_lane + d;
                if( l >= 0 && l < lanes && sameSide( l ) && !laneClosedAhead( track, c.m_s, l, 140.0 ) && laneFree( l, c.m_s, &c ) )
                {
                    c.m_lane = l;
                    break;
                }
            }
            if( laneClosedAhead( track, c.m_s, c.m_lane, 50.0 ) ) want = 6.0f;
        }
        // A slower car ahead: overtake or follow. The siren only slows behind whatever is still in its way.
        if( c.m_siren )
        {
            auto inWay = [&]( double s, float x, float width, float speed )
            {
                const double gap = s - c.m_s;
                if( gap > 0.0 && gap < 25.0 + c.m_speed * 0.8 && std::abs( x - c.m_x ) < ( width + c.m_width ) * 0.5f + 0.3f ) want = std::min( want, speed );
            };
            for( const TrafficCar &o : m_cars )
                if( &o != &c && !o.m_swim ) inWay( o.m_s, o.m_x, o.m_width, o.m_speed );
            inWay( player.m_s, player.m_x, player.m_spec.m_width, player.m_speed );
        }
        for( const TrafficCar &o : m_cars )
        {
            if( &o == &c || o.m_lane != c.m_lane || c.m_siren || ( c.m_swarm && o.m_swarm ) ) continue;
            const double gap = o.m_s - c.m_s;
            if( gap > 0.0 && gap < 20.0 + c.m_speed * 1.2 && o.m_speed < want && !o.m_siren && c.m_yield <= 0.0f )
            {
                // Overtaking on the outer side: the left, or the right where they keep left.
                const int l = c.m_lane + ( m_rng.chance( 0.8f ) == keepLeft ? 1 : -1 );
                if( l >= 0 && l < lanes && sameSide( l ) && laneFree( l, c.m_s, &c ) && !laneClosedAhead( track, c.m_s, l, 100.0 ) )
                    c.m_lane = l;
                else
                    want = std::min( want, o.m_speed - ( gap < 12.0 ? 3.0f : 0.0f ) );
            }
        }
        c.m_speed = approach( c.m_speed, std::max( 0.0f, want ), want < c.m_speed ? 1.6f : 0.5f, dt );
        float target = laneCenter( c.m_lane, lanes, seg.m_halfWidth ) + c.m_xOff;
        if( c.m_chaos ) target += std::sin( m_clock * 0.3f + c.m_weave ) * ( c.m_swarm ? 0.4f : 0.75f );
        target = clampf( target, -seg.m_halfWidth + c.m_width * 0.5f, seg.m_halfWidth - c.m_width * 0.5f );
        if( c.m_yield > 0.0f ) target = slowSide * ( seg.m_halfWidth - c.m_width * 0.5f + 0.5f );
        else if( c.m_siren && lanes == 1 ) target = -slowSide * seg.m_halfWidth * 0.4f;
        if( seg.m_flags & SEG_FORK )
        {
            // On the carriageway the player did not take: follow it as it turns away, then leave.
            const Fork   fork = track.fork( seg.m_stage );
            const double v = c.m_s - fork.m_at - ForkMedian;
            if( fork.m_chosen != 0 && v > 0.0 && c.m_x * static_cast<float>( fork.m_chosen ) < 0.0f )
            {
                const double w = std::min( v, ForkPeel ) / ForkPeel;
                target = static_cast<float>( -fork.m_chosen ) * static_cast<float>( 3.0 + 130.0 * w * w );
                c.m_x = target;
                c.m_gone = v > ForkPeel;
            }
        }
        c.m_vx = clampf( ( target - c.m_x ) * 1.3f, -2.6f, 2.6f );
        c.m_x += c.m_vx * dt;
        c.m_yaw = std::atan2( c.m_vx, std::max( c.m_speed, 5.0f ) );
        c.m_s += static_cast<double>( c.m_speed ) * dt;
        if( c.m_hitTimer > 0.0f ) c.m_hitTimer -= dt;
    }

    // Contact with the player, overtakes, near misses.
    const float pl = player.m_spec.m_length, pw = player.m_spec.m_width;
    for( TrafficCar &c : m_cars )
    {
        if( c.m_swim ) continue;
        const double ds = player.m_s - c.m_s;
        const float  dx = player.m_x - c.m_x;
        const float  halfL = ( pl + c.m_length ) * 0.5f;
        const float  halfW = ( pw + c.m_width ) * 0.5f * 0.92f;
        const bool   lowEnough = !player.m_airborne || player.m_y - track.surfaceHeight( player.m_s, player.m_x ) < 1.2;
        if( std::abs( ds ) < halfL && std::abs( dx ) < halfW && lowEnough )
        {
            const float rel = player.m_speed - c.m_speed;
            const float side = dx >= 0.0f ? 1.0f : -1.0f;
            if( std::abs( ds ) < halfL * 0.6f )
            {
                // Side by side: shove apart.
                player.m_x += side * ( halfW - std::abs( dx ) ) * 0.9f;
                player.m_vx = side * 3.0f;
                player.m_speed *= 0.97f;
                c.m_x -= side * 0.25f;
                ev.m_hit = std::max( ev.m_hit, 0.35f );
            }
            else if( ds < 0.0 )
            {
                // Rear-ended it.
                if( rel > 18.0f )
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
            if( !c.m_animal && !c.m_siren && c.m_honk <= 0.0f )
            {
                // Hit, it honks.
                c.m_honk = 3.0f;
                ev.m_horn = 1.0f;
                ev.m_hornPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
                ev.m_hornPitch = doppler( ds > 0.0 ? c.m_speed - player.m_speed : player.m_speed - c.m_speed );
            }
        }
        if( c.m_honk > 0.0f ) c.m_honk -= dt;
        if( c.m_chaos && !c.m_animal && !c.m_siren && c.m_honk <= 0.0f && std::abs( ds ) < 40.0 && std::abs( dx ) < 5.0f && m_rng.chance( dt * 0.35f ) )
        {
            // Where everyone honks: at the player close by, for being there.
            c.m_honk = m_rng.range( 4.0f, 9.0f );
            const float strength = 0.35f + 0.5f * ( 1.0f - static_cast<float>( std::abs( ds ) ) / 40.0f );
            if( strength > ev.m_horn )
            {
                ev.m_horn = strength;
                ev.m_hornPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
                ev.m_hornPitch = doppler( ds > 0.0 ? c.m_speed - player.m_speed : player.m_speed - c.m_speed );
            }
        }
        if( c.m_animal ) continue;
        if( !c.m_passed && ds > halfL )
        {
            c.m_passed = true;
            ++ev.m_overtakes;
            if( std::abs( dx ) < halfW + 1.4f && player.m_speed > 30.0f && c.m_hitTimer <= 0.0f ) ++ev.m_nearMisses;
            const float rel = player.m_speed - c.m_speed;
            if( std::abs( dx ) < 9.0f && rel > 8.0f )
            {
                const float strength = clampf( rel / 50.0f, 0.2f, 1.0f ) * ( 1.0f - std::abs( dx ) / 10.0f );
                if( strength > ev.m_whoosh )
                {
                    ev.m_whoosh = strength;
                    ev.m_whooshPan = clampf( -dx / 4.0f, -1.0f, 1.0f );
                    ev.m_whooshPitch = clampf( 0.7f + rel / 80.0f, 0.7f, 1.6f );
                }
            }
        }
        else if( c.m_passed && ds < -halfL )
        {
            c.m_passed = false;
            if( c.m_siren && std::abs( dx ) > halfW + 0.6f && player.m_speed > 3.0f ) ++ev.m_gaveWay;
        }
    }
    std::erase_if( m_cars,
                   [&]( const TrafficCar &c )
                   {
                       const bool across = c.m_animal && c.m_x * c.m_vx > 0.0f && std::abs( c.m_x ) > track.segment( static_cast<long>( c.m_s / SegmentLength ) ).m_halfWidth + 9.0f;
                       const Crossing xc = c.m_swim ? Crossing() : track.crossingAt( c.m_s );
                       const bool     aboard = xc.m_type != CrossingE::NONE && c.m_s > xc.m_board - 25.0 && c.m_s < xc.m_land - 20.0;
                       return across || aboard || c.m_gone || ( c.m_swim && std::abs( c.m_x ) > 160.0f ) || c.m_s < player.m_s - ( c.m_siren ? 400.0 : 140.0 ) ||
                              c.m_s > player.m_s + 1700.0;
                   } );

    // Cones and boards.
    const TrackFrame pf = track.frameAt( player.m_s );
    for( Obstacle &o : m_obstacles )
    {
        if( o.m_knocked )
        {
            o.m_life += dt;
            o.m_vel.y -= 9.81f * dt;
            o.m_pos.m_x += static_cast<double>( o.m_vel.x * dt );
            o.m_pos.m_y += static_cast<double>( o.m_vel.y * dt );
            o.m_pos.m_z += static_cast<double>( o.m_vel.z * dt );
            o.m_angle += o.m_spinRate * dt;
            if( o.m_pos.m_y < o.m_groundY && o.m_vel.y < 0.0f )
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
        if( std::abs( ds ) < reachL && std::abs( dx ) < reachW && !player.m_airborne )
        {
            o.m_knocked = true;
            o.m_pos = track.pointAt( o.m_s, o.m_x, 0.0f );
            o.m_groundY = static_cast<float>( o.m_pos.m_y );
            const float side = dx >= 0.0f ? -1.0f : 1.0f;
            o.m_vel = Vector3Add( Vector3Scale( pf.m_forward, player.m_speed * m_rng.range( 0.7f, 1.0f ) ),
                                  Vector3Add( Vector3{ 0, m_rng.range( 3.0f, 7.0f ), 0 },
                                              Vector3Scale( pf.m_right, side * m_rng.range( 1.0f, 5.0f ) ) ) );
            o.m_axis = Vector3Normalize( Vector3{ m_rng.range( -1, 1 ), m_rng.range( -0.3f, 0.3f ), m_rng.range( -1, 1 ) } );
            o.m_spinRate = m_rng.range( 8.0f, 16.0f );
            player.m_speed *= o.m_kind ? 0.82f : 0.97f;
            player.m_shake = std::max( player.m_shake, o.m_kind ? 0.5f : 0.18f );
            ++ev.m_cones;
        }
    }
    std::erase_if( m_obstacles, [&]( const Obstacle &o ) { return o.m_life > 3.0f || ( !o.m_knocked && o.m_s < player.m_s - 60.0 ); } );
}

} // namespace sunstreak
