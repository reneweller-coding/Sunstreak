/**
 * @file Passenger.cpp
 * @brief The hitchhikers: where they wait, when they get in, when they get out, and their words.
 * @author reneweller
 */
#include "Passenger.h"

#include "Local.h"
#include "Rng.h"

#include <cmath>
#include <cstring>
#include <string_view>

namespace sunstreak
{

namespace
{

/** @brief A country's words for hello and thank you. */
struct Words
{
    const char *m_hello;    ///< hello
    const char *m_thanks;   ///< thank you
};

/**
 * @brief The words of a country: by name, else by the language its region speaks, else English.
 * @param[in] country  its name (nullptr: none)
 * @return them
 */
Words wordsOf( const char *country )
{
    if( !country ) return { "HELLO", "THANKS" };
    static const std::pair<const char *, Words> named[] = {
        { "GERMANY", { "HALLO", "DANKE" } }, { "AUSTRIA", { "SERVUS", "DANKE" } }, { "SWITZERLAND", { "GRUEZI", "MERCI VIELMAL" } },
        { "LIECHTENSTEIN", { "HOI", "DANKE" } }, { "FRANCE", { "BONJOUR", "MERCI" } }, { "BELGIUM", { "BONJOUR", "MERCI" } },
        { "LUXEMBOURG", { "MOIEN", "MERCI" } }, { "MONACO", { "BONJOUR", "MERCI" } }, { "NETHERLANDS", { "HALLO", "DANK JE" } },
        { "ITALY", { "CIAO", "GRAZIE" } }, { "SAN MARINO", { "CIAO", "GRAZIE" } }, { "VATICAN", { "SALVE", "GRAZIE" } },
        { "SPAIN", { "HOLA", "GRACIAS" } }, { "PORTUGAL", { "OLA", "OBRIGADO" } }, { "BRAZIL", { "OI", "OBRIGADO" } },
        { "ENGLAND", { "HELLO", "CHEERS" } }, { "SCOTLAND", { "HIYA", "CHEERS" } }, { "WALES", { "SHWMAE", "DIOLCH" } },
        { "IRELAND", { "DIA DUIT", "GO RAIBH MAITH AGAT" } }, { "AUSTRALIA", { "G'DAY", "CHEERS MATE" } }, { "NEW ZEALAND", { "KIA ORA", "CHEERS" } },
        { "JAPAN", { "KONNICHIWA", "ARIGATO" } }, { "CHINA", { "NI HAO", "XIE XIE" } }, { "TAIWAN", { "NI HAO", "XIE XIE" } },
        { "SOUTH KOREA", { "ANNYEONG", "GOMAWOYO" } }, { "INDIA", { "NAMASTE", "DHANYAVAAD" } }, { "NEPAL", { "NAMASTE", "DHANYABAD" } },
        { "THAILAND", { "SAWASDEE", "KHOP KHUN" } }, { "VIETNAM", { "XIN CHAO", "CAM ON" } }, { "INDONESIA", { "HALO", "TERIMA KASIH" } },
        { "MALAYSIA", { "HELO", "TERIMA KASIH" } }, { "PHILIPPINES", { "KUMUSTA", "SALAMAT" } }, { "CAMBODIA", { "SUOSDEI", "AKUN" } },
        { "LAOS", { "SABAIDEE", "KHOP JAI" } }, { "MYANMAR", { "MINGALABA", "CHE ZU BE" } }, { "MONGOLIA", { "SAIN UU", "BAYARLALAA" } },
        { "RUSSIA", { "PRIVET", "SPASIBO" } }, { "BELARUS", { "PRIVET", "DZYAKUY" } }, { "UKRAINE", { "PRYVIT", "DYAKUYU" } },
        { "POLAND", { "CZESC", "DZIEKUJE" } }, { "CZECHIA", { "AHOJ", "DEKUJI" } }, { "SLOVAKIA", { "AHOJ", "DAKUJEM" } },
        { "HUNGARY", { "SZIA", "KOSZONOM" } }, { "ROMANIA", { "SALUT", "MULTUMESC" } }, { "BULGARIA", { "ZDRASTI", "BLAGODARYA" } },
        { "CROATIA", { "BOK", "HVALA" } }, { "SERBIA", { "ZDRAVO", "HVALA" } }, { "BOSNIA", { "ZDRAVO", "HVALA" } }, { "SLOVENIA", { "ZIVJO", "HVALA" } },
        { "GREECE", { "YASSOU", "EFCHARISTO" } }, { "CYPRUS", { "YASSOU", "EFCHARISTO" } }, { "TURKEY", { "MERHABA", "TESEKKURLER" } },
        { "SWEDEN", { "HEJ", "TACK" } }, { "NORWAY", { "HEI", "TAKK" } }, { "DENMARK", { "HEJ", "TAK" } }, { "FINLAND", { "MOI", "KIITOS" } },
        { "ICELAND", { "HALLO", "TAKK" } }, { "ESTONIA", { "TERE", "AITAH" } }, { "LATVIA", { "SVEIKI", "PALDIES" } }, { "LITHUANIA", { "LABAS", "ACIU" } },
        { "ISRAEL", { "SHALOM", "TODA" } }, { "IRAN", { "SALAM", "MERSI" } }, { "KENYA", { "JAMBO", "ASANTE" } }, { "TANZANIA", { "JAMBO", "ASANTE" } },
        { "UGANDA", { "JAMBO", "WEBALE" } }, { "SOUTH AFRICA", { "SAWUBONA", "NGIYABONGA" } }, { "ETHIOPIA", { "SELAM", "AMESEGINALEHU" } },
        { "SENEGAL", { "BONJOUR", "MERCI" } }, { "IVORY COAST", { "BONJOUR", "MERCI" } }, { "MALI", { "BONJOUR", "MERCI" } },
        { "CAMEROON", { "BONJOUR", "MERCI" } }, { "MADAGASCAR", { "SALAMA", "MISAOTRA" } }, { "HAWAII", { "ALOHA", "MAHALO" } },
    };
    for( const auto &[name, words] : named )
        if( std::strcmp( name, country ) == 0 ) return words;
    switch( countryInfo( country ).m_fleet )
    {
    case FleetE::LATIN: return { "HOLA", "GRACIAS" };
    case FleetE::ARABIA: return { "MARHABA", "SHUKRAN" };
    case FleetE::EASTERN: return { "PRIVET", "SPASIBO" };
    default: return { "HELLO", "THANKS" };
    }
}

} // namespace

bool stageHitchhiker( const Track &track, int stage, double &s, float &side )
{
    const BiomeDef &place = biomeDef( track.stageBiome( stage ) );
    if( stage < 0 || place.m_underwater || place.m_surface.m_lineGlow > 0.0f ) return false;
    const uint64_t h = hash64( track.seed() * 2029 + static_cast<uint64_t>( stage ) * 13 + 3 );
    if( h % 3 != 0 ) return false;   // one stage in three
    side = place.m_country && countryInfo( place.m_country ).m_left ? -1.0f : 1.0f;
    const double base = static_cast<double>( stage ) * StageLength + HitchAt + static_cast<double>( ( h >> 8 ) % 10000 );
    for( int k = 0; k < 8; ++k )
    {
        s = base + 120.0 * k;
        bool clear = track.crossingAt( s ).m_type == CrossingE::NONE && !track.inForkZone( s, 150.0 );
        for( double d = -60.0; d <= 60.0 && clear; d += SegmentLength )
            clear = !( track.segment( static_cast<long>( ( s + d ) / SegmentLength ) ).m_flags & ( SEG_TUNNEL | SEG_BRIDGE | SEG_JUMP | SEG_ROADWORKS ) );
        if( clear ) return true;
    }
    return false;
}

const char *helloIn( const char *country )
{
    return wordsOf( country ).m_hello;
}

const char *thanksIn( const char *country )
{
    return wordsOf( country ).m_thanks;
}

void Passenger::reset()
{
    *this = Passenger{};
}

void Passenger::update( float dt, const Track &track, double s, float x, float speed, bool enabled, PassengerEvents &ev )
{
    ev = PassengerEvents{};
    switch( m_state )
    {
    case PassengerStateE::NONE:
    {
        if( !enabled ) return;
        // This stage's or the next one's, once the road there is laid and the spot is still ahead.
        const int stage = static_cast<int>( s / StageLength );
        for( int st = stage; st <= stage + 1; ++st )
        {
            double hs = 0.0;
            float  side = 1.0f;
            if( st <= m_lastStage || !stageHitchhiker( track, st, hs, side ) || hs < s + 150.0 || hs > s + 900.0 ) continue;
            const Segment  &seg = track.segment( static_cast<long>( hs / SegmentLength ) );
            const uint64_t h = hash64( track.seed() * 31 + static_cast<uint64_t>( st ) );
            m_lastStage = st;
            m_state = PassengerStateE::WAITING;
            m_s = hs;
            m_side = side;
            m_x = side * ( seg.m_halfWidth + RumbleWidth + 1.2f );
            m_look = static_cast<int>( h % 1000 );
            const BiomeDef &place = biomeDef( track.stageBiome( st ) );
            m_from = place.m_country ? place.m_country : "";
            break;
        }
        return;
    }
    case PassengerStateE::WAITING:
    {
        if( s > m_s + 60.0 || !enabled )
        {
            m_state = PassengerStateE::NONE;   // passed by
            return;
        }
        if( std::abs( s - m_s ) < 9.0 && std::abs( x - m_x ) < 5.0f && speed < HitchStop )
        {
            // In they get: to the next place, as soon as the road reaches it.
            const int next = static_cast<int>( m_s / StageLength ) + 1;
            m_destS = static_cast<double>( next ) * StageLength + 800.0;
            m_to = track.stageBiome( next );
            m_timeLeft = static_cast<float>( ( m_destS - s ) / HitchPace ) + HitchSlack;
            m_state = PassengerStateE::ABOARD;
            ev.m_pickedUp = true;
        }
        return;
    }
    case PassengerStateE::ABOARD:
    {
        if( s >= m_destS )
        {
            m_state = PassengerStateE::NONE;
            ev.m_delivered = true;
            return;
        }
        m_timeLeft -= dt;
        if( m_timeLeft <= 0.0f || !enabled )
        {
            m_timeLeft = 0.0f;
            m_state = PassengerStateE::NONE;
            ev.m_late = true;
        }
        return;
    }
    }
}

} // namespace sunstreak
