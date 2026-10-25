# Sunstreak

An arcade racer in the spirit of *Lotus III: The Ultimate Challenge*: a low camera, a road that bends, climbs and
throws you over crests, traffic to slice through and a clock that only checkpoints refill. The endless mode drives a
track generated from a seed, one biome per 15 km stage, and it grows curvier, denser and stricter as you go. By
default it is a world tour: 781 places in all 202 countries and territories it visits, every member of the United
Nations among them, the big countries state by state and province by province, round the globe, through the seasons
and their festivals. At the borders the road forks and you choose where to go next; between continents and to the
islands you drive along the sea floor in a glass tunnel, or onto a ferry or the car train under the Channel.

Beside the endless run there is a championship against eleven rivals, a Track Lab that turns any setting into a
shareable track code, a ghost of your best drive on every code and a top ten.

The manual is [docs/manual/Sunstreak-Manual.pdf](docs/manual/Sunstreak-Manual.pdf) (and `.html`), with pictures
the game takes itself: `tools/manual/make_shots.ps1` retakes them into `docs/manual/shots` and
`tools/manual/make_manual.py` writes both files from `tools/manual/chapters.txt`, the place data and the sources.

C++20 on [raylib 5.5](https://github.com/raysan5/raylib) (fetched by CMake). Everything you see and hear is made in
code: the low-poly models, the sky, the stroke font, the engines (four, eight and twelve cylinders), and the
soundtrack.

## Playing

| | Keyboard | Gamepad / Steam Deck |
|---|---|---|
| Accelerate | Up / W | RT or A |
| Brake | Down / S | LT or X |
| Steer | Left Right / A D | left stick, d-pad |
| Boost | Space / Shift | B |
| Gear down / up (manual) | Q / E | LB / RB |
| Camera (chase, far, bumper, cockpit) | C | Y |
| Look back (hold) | B | right stick pressed in |
| Radio station / next track | M / N | View |
| Pause | Esc / P | Start |
| Window / fullscreen | F11 or Alt+Enter | |
| Performance overlay | F3 | |
| Read the data files again | F5 | |
| Landmark viewer | F6 | |

The clock starts at 55 s. Every checkpoint (2.5 km) adds time, a little less each stage. Near misses (passing
traffic close and fast) fill the boost tank and raise the combo; overtakes and speed add to the score. The run ends
when the clock reaches zero; the results show the track code to drive the same road again.

Gamepads rumble on hits, crashes, landings, boosts and gear changes and buzz on kerbs and gravel (Settings >
VIBRATION; on the Deck through Steam Input).

## Modes

- **Endless run**: the clock, the checkpoints and the stages, on a fresh random track each time. On the world tour it
  continues where the last endless run stopped (left and right on the title: NEW TOUR). SAVE AND QUIT in the pause menu
  keeps the run as it stands, the place on the road, the clock and the score; RESUME on the title drives on from there.
- **Achievements** (PASSPORT > AWARDS, in sheets of 32): 69 of them, from the first stamp to every country of Africa,
  Europe, the Americas, Asia, the steppe or the islands, every place of the tour, the random tour to every country,
  getting away from the police of every livery, hitchhikers from 50 countries, the festivals, 10000 overtakes, a
  marathon run, the daily challenge on 30 days, 40075 km. They are announced when earned and kept with the records.
- **Travel**: the world tour without a clock, on and on from where the last tour stopped, for the places, the passport and
  the postcards. Speed cameras and the police still fine you, in points.
- **Championship**: five races on generated tracks against eleven rivals, from the back of the grid; points
  10-8-6-5-4-3-2-1 per race, the standings after each. Rivals draft in your slipstream and you in theirs.
- **Track Lab**: mode (time attack or a race against the eleven), scenery (the world tour, all mixed, or one place:
  the eight classics and every place you have driven through), curves, sharpness, hills, traffic and obstacles (0..9
  or AUTO), length (2.5..37.5 km, or endless in time attack) and the seed. Every combination is a code of fifteen
  characters such as `K3ZQ8-M2TVX-41P0R` with a check symbol against typos; CODE types one in, the results show the
  code of every run. The twelve-character codes of earlier versions still work.
- **Daily challenge**: today's track, the same all day: a world tour from a seed of the date with a twist (rush
  hour, roadworks everywhere, all bends); the best score of the day is kept.
- **Rallies**: ten long-distance routes driven in one go against the clock, place after place: Paris - Dakar, the
  Panamericana from Alaska to Patagonia, Route 66, the Silk Road, Cape to Cairo, the Alpine passes, the Outback, a
  Himalayan raid, the Nordkapp and the Mekong Express. The clock counts up; each finished rally keeps its best time.
- **Journeys** (in the rallies' menu): from one place of the world tour to another against the clock, London - Paris,
  Venice - Rome, Osaka - Tokyo and more, up to Lisbon - Tokyo, half the world. The forks choose the way: before each
  the screen shows how many stages the goal is either way, the shorter in gold; the other may be a detour or a
  shortcut. Each journey keeps its best time and its ghost.
- **Replay and photo mode**: after a run, WATCH REPLAY plays the last minute again from roadside cameras, a helicopter
  and the chase camera. PHOTO MODE (pause menu) stops the game and lets the camera roam round the car; a photo is saved
  in the settings folder's `photos` and becomes the place's postcard.
- **Passport**: the world tour on a map with the countries and their provinces (the ones you have been to lit,
  outlines from Natural Earth; every place a dot, the ones you have seen lit, the way you drove as a line, the
  twelve slices of the globe you have driven through as a bar: all twelve is a drive round the world), a stamp for every
  country, the regions of the big countries with how many of their states you have seen (pick one with up and down and
  ENTER shows its regions, ticked where you have been, with the places seen of each), the postcards (every new place
  you drive through sends one home, taken in sight of its landmark) and an album with a photo of every landmark you
  have passed.
- **Records**: the top ten (with your initials), total kilometres, titles, places and countries seen. The best drive
  on each code is kept as a ghost car that races along the next time.

Unlocks: the SPARROW (light, revvy four-cylinder) after 30 km, the VORTEX (V12, fastest) after 100 km or a title, the
KESTREL (a turbocharged rally hatchback that keeps its grip on gravel and sand) after 40 countries. The GARAGE dresses
the car: fourteen paints, seven rim finishes and seven decals (twin stripes, a side stripe, a roundel, two tones,
chequers, a globe) in any paint's colour. Career points from every run, every race and every new place and country pay
for them; SUN YELLOW, DEEP BLUE and CHAMPION GOLD keep their old conditions, GLOBETROTTER needs a drive round the world,
the globe 50 countries. The car carries the number plate of its HOME COUNTRY (at first the computer's region, NONE for
no plate) in that country's style, the blue EU band with its letters included, and says what you type into it under
NUMBER PLATE (up to eight letters, digits, spaces and dashes).

Gravel, dirt and sand tracks (the Gibb River Road, the Yungas, the Sani Pass, the Road of Bones, the Sahara) have no
markings, crunch under the tyres, throw up dust and hold less than asphalt.

Assists and accessibility (Settings): STEER ASSIST (light or strong) steers where your hands are idle, keeps the road
and goes round the traffic; BRAKE ASSIST brakes for bends and obstacles taken too fast; COLOUR BLIND swaps the HUD's
red and green for vermilion and sky blue; TEXT SIZE enlarges every text to 115 or 130 %.

Settings > 3D RESOLUTION renders the 3D picture at 50 to 200 % of the window (less is faster, more smooths the edges;
the text stays sharp). RESET SETTINGS puts every setting back to its default (the car and the best runs stay); ERASE
PROGRESS forgets everything driven: the records, places, unlocks, postcards, photos, album, ghosts and a saved run.
Both want a second press.

Difficulty (Settings: EASY, NORMAL, HARD) scales the grip, the clock, the traffic and the rivals. The handling itself
is in `tuning.ini` beside the settings (written with comments on the first start): steer, centrifugal, accel, brake,
boost, grip, crash_keep, time, traffic, rivals. Change a value and restart; delete the file for the defaults.

835 biomes. The world tour (Settings > SCENERY) runs east from Ireland: the British Isles, France, Germany and the
Alps, Italy, Iberia, Morocco, West, East and Southern Africa, Arabia, Egypt and the Levant, Turkey, Greece and the
Balkans, Central and Eastern Europe, Central Asia, India and the Himalaya, Southeast and East Asia, Australia, New
Zealand and the Pacific, South and Central America, Mexico, the United States and Canada, Russia, the Baltic,
Scandinavia, Greenland and Iceland, and back to Ireland. Each stage shows its place and country: Tuscany, the
Serengeti with its herds, Cappadocia under balloons, Ha Long Bay, the Avenue of Baobabs, Route 66, Las Vegas and
Tokyo at night, the Great Wall, Uluru, the moai, the Florida Keys and seven hundred more. The big countries are
driven region by region, each with its own landscape and sights, and the stage line names the place, its state and
its country (HILL COUNTRY - TEXAS - USA): all fifty states of the United States and the capital, all sixteen German
Lander, every province and territory of Canada, every state and territory of Australia, every region of France and
Italy, every community of Spain, every state of Brazil, Mexico and India, every province of Argentina and China, every
province of South Africa, and many of Russia, Japan, Indonesia and some forty more countries. Where a state or province
begins, a sign says so. Where the country writes in its own script, the place sign is bilingual, the name in that
script above the Latin one: Cyrillic, Greek, Georgian, Armenian, Hebrew, Arabic and Persian, the scripts of India by
state, Sinhala, Thaana, Thai, Lao, Khmer, Burmese, Ge'ez, Chinese, Japanese and Korean. The famous roads have their
own: the Stelvio, the Transfagarasan, the Atlantic Road, Trollstigen, the Nordschleife, the Yungas Road, the Road of
Bones, the Dalton Highway beside its pipeline, the Nullarbor with its kangaroo, camel and cow signs, Bonneville, the
Tail of the Dragon and the Road to Hana. Twenty-eight of them are legendary roads: a banner when they begin, your best
time over each from end to end, and their signature. The passes climb in hairpins, numbered on the outside and
counting down to the top where the pass's name and height stand (48 TORNANTE up to the Passo dello Stelvio at 2757 m,
36 KEHRE up the Grossglockner, Trollstigen, the Transfagarasan, Irohazaka, Pikes Peak, Tianmen, the Gata Loops), on
one mountainside whose land lies the same seen from every leg; the legs below stay in sight. Other high mountains climb
in hairpins now and then too. The Atlantic Road has its humped bridges over the sea that seem to end in the air.
Traffic slows for the hairpins, and the checkpoint time counts them in. The roads follow their country: yellow down the middle in the Americas,
Scandinavia, China, Korea and mainland Southeast Asia, a yellow edge on the centre side in Arabia and on the outer edge
in Ireland and southern Africa; lanes from 3.65 m wide in North America to 3 m in Japan; paved shoulders, the plain
verge or the painted kerbs of the towns (black and white in Malaysia and Russia, red and white in Thailand, black and
yellow in India and the Gulf); cat's eyes in Britain and Ireland; concrete slabs on American freeways, German and
Belgian motorways and Philippine highways; and, by each country's road quality (the WEF's index), faded markings,
patches, potholes that thump the car and stretches of gravel where the asphalt ends: in Chad and Mongolia more of the
road is gravel than not. The traffic drives the country's way, as much of it as Numbeo's traffic index says: swarms
of scooters riding side by side in Vietnam, Indonesia and India; cars weaving across the lines, changing lanes on a whim
and honking in India, Egypt or Nigeria; tractors on the country roads of Europe; road trains in the outback. The air is as clean or as dirty as Numbeo's
pollution index has it, by country and for some eighty cities of their own: over Delhi, Kathmandu, Lagos or Cairo the
haze thickens and browns, the sky pales and the colours fade, and litter lies by the road, more of it the dirtier the
air (plastic bags everywhere; bags caught in thorn bushes in Africa and India, styrofoam boxes in Southeast Asia,
smouldering heaps, black bin bags in Europe, a car wreck now and then), while Helsinki and Reykjavik stay clear. On about one stage in three a patrol car of the country's police waits at the roadside,
in its own livery (a black-and-white cruiser in the United States, blue and yellow checks in Britain, a carabinieri
car in Italy, green and white in Chile, a white jeep in India and some thirty more): past it too fast and it gives chase with its lights and siren
(the long wail, or the two tones in Europe). Get 400 m ahead and you have escaped (+2500); let it pull up beside you
and the fine is five seconds (POLICE in the settings turns them off); hold B (the right stick pressed in) to look back.
The cockpit camera puts you in the driver's seat, on the left or, where they keep left, on the right (it slides over at
the border), behind a turning wheel and live dials, with what the country's drivers hang from the mirror or stand on the
dashboard: fuzzy dice in America, a nodding dog in Germany, a beckoning cat in Japan, marigolds in India, jasmine in
Thailand, a red knot in China, pompoms in Mexico, a blue eye charm in Turkey, a dashcam in Russia, a koala in Australia.
On two stages in three a hitchhiker waits by the road, thumb out: slow down beside them and they get in with a hello in their
language, then take them to the next place before their patience runs out (+3000 and five seconds, and their thanks:
DANKE, ARIGATO, SHUKRAN, ASANTE ...); too slow and they get out at the roadside. Sometimes it is a courier job instead: a courier with a fragile parcel for
the next place (hit nothing on the way, or it breaks), a paramedic with an organ for the hospital at the next checkpoint
(hurry), or a police officer whose car was just stolen: catch the thief by staying close beside or behind it for a
moment before it reaches the next checkpoint.
Now and then there is something to see (data/spectacles.txt): an airliner coming in to land low across the road near
the big airports and over the Caribbean islands, two fighter jets roaring overhead in the deserts of the American west
and the Emirates, hot-air balloons over Cappadocia, Bagan, Luxor and the Serengeti, a container ship on the Panama Canal
or off Singapore, an airship over southern Germany.
In half the stages of countries with a fast train (data/trains.txt: the Shinkansen, the TGV, the ICE, the AVE, the
Frecciarossa, the KTX, the Indian Pacific and ten more) a viaduct runs beside the road for a few kilometres, out of a
stone portal and into another, and the train races you along it: it starts just ahead and speeds up to its top speed
(at most 93 % of your car's). Pass its nose before the stretch ends for +4000 and five seconds.
On two stages in three a local hero turns up beside you before a checkpoint, in a car of your class, and challenges you to the next
one in their language (WETTEN, DASS NICHT? / SHOUBU DA! / EAT MY DUST!). Beat them there for +5000 and five seconds and a
grumble (MIST! / KUSO!); let them win and they taunt you (LAHME ENTE! / OSOI! / SUNDAY DRIVER!). What everyone says,
and the heroes' first names, are in data/phrases.txt.
Every few kilometres an ambulance or a fire engine of the country comes along on a run, lights flashing and siren
going: yellow and green checks in Britain, day-glo red stripes in Germany, the red crescent in the Gulf, a red pumper
in the United States. The traffic ahead of it pulls over; keep out of its way when it overtakes and you earn +1000.
Landmarks stand where they belong, once per stage: Big Ben and Tower Bridge,
the Arc de Triomphe, the Colosseum and St Peter's, the Leaning Tower, the Sagrada Familia, Neuschwanstein, the
Brandenburg Gate, the Acropolis, St Basil's, the Christ the Redeemer and Cristo Rei statues, the Statue of Liberty,
the Opera House, the Petronas Towers, the Space Needle, the Capitol and the Washington Monument, the HOLLYWOOD sign,
the Sphinx, the Colossi of Memnon, Borobudur, Harbin's ice castle, the burning crater of Darvaza, Great Zimbabwe,
Bled's island church and the torii of Miyajima in the sea, and over a hundred and fifty more built from the data files:
the Eiffel Tower, Cologne Cathedral, the Elbphilharmonie, the Wartburg and the Frauenkirche, the Gateway Arch, the
Golden Gate, the Alamo, Mount Rushmore and Devils Tower, the CN Tower and the Chateau Frontenac, the Matterhorn, the
Pont du Gard and Carcassonne, the Taj Mahal, the Golden Temple and the Statue of Unity, the Terracotta Army and the
Leshan Buddha, Himeji Castle and the Skytree, the Motherland Calls and Kizhi, the Hagia Sophia, Petra, Abu Simbel,
Machu Picchu, Shwedagon, Djenne's mud mosque, Preikestolen and many more. RANDOM TOUR leaps instead of driving on: after
every stage the road forks to two countries drawn at random, each in another part of the world, and never one the round
has been to; between landmasses comes the sea floor, a ferry or the car train. The round goes on from run to run (the
stage line counts it, 37 / 202 COUNTRIES) until every country is done, and then the next begins. ALL MIXED shuffles every biome, the twelve
that are not on the map included: Night Metro, Storm Marsh, Neon Horizon, Neon Rain, Dawn Expressway, Lunar Highway (low gravity),
Red Planet, Crystal Desert, Fungal Glade, Cloud Highway, Pumpkin Hollow and Total Eclipse.

Weather fronts pass over the tour (Settings > WEATHER): showers, fog banks and thunderstorms in temperate places,
downpours in the tropics, snow showers in the cold and the mountains, sandstorms in the deserts. They are announced a
little ahead, last a kilometre or two, and wet, snowy or sandy roads hold less. In the rain the road turns into a dark
mirror (the cars, the trees and the street lamps stand in it), puddles gather, and over 100 km/h the tyres float on
them: AQUAPLANING, hardly any grip until they find the road again. In the snow grey slush lies in blotches and sprays up
behind the car. In South and Southeast Asia every other tropical front is the monsoon: a downpour, and stretches where
the road is flooded from edge to edge (fountains of water, a little drag, aquaplaning at speed). Fog lies low in the
valleys of the temperate and cold places at dawn and dusk and in fog banks; over the deserts on clear days dust devils
whirl beside the road, and where it is hot the air shimmers over the asphalt at the horizon. TIME OF DAY keeps each place's own light
by default; TOUR CLOCK lets the sun follow the time zones (morning wherever the run starts, an hour more every ten
kilometres, and the longitude of each place on top: east of you it is already evening); DAWN, DAY, DUSK and NIGHT fix
the hour. Every place sounds like itself: the surf on the shore it lies on, wind over deserts and mountains, insects
and birds in the tropics, the hum and horns of a city, rain on the car, the joints of a bridge and the engine echoing
in a tunnel. Sirens and horns are Doppler-shifted, higher as they come nearer and lower as they go, and the rush of a
car going past is the higher the faster you pass it.

The seasons (Settings > SEASONS: AS TODAY by the date, AS YOU TRAVEL a year every 400 km, or OFF) reach the temperate
and cold places: in autumn the broadleaf trees, bushes, hedges and vines turn orange, red and gold, the fields to
stubble and leaves blow across the road; in winter snow lies on the land and on everything that faces up, the trees
are bare and it snows; in spring the green is fresh, some trees blossom and petals drift. When they come follows each
place's real climate (monthly temperature and rain of thirty years, `climate.txt`): snow lies where the air stayed
below freezing, so the south turns the year round by itself and Patagonia is white in July, while the tropics and the
deserts keep their one look. The showers follow the rain of the month as well: hardly any in the dry season of Mumbai,
a monsoon in July, snow in the Alps in January where the same road sees rain in July. On their dates the festivals come: Holi in India with bunting
and colour in the air, the carnival in Brazil, red lanterns for the Lunar New Year in China, Taiwan, Vietnam, Korea,
Singapore and Malaysia, Diwali with lights along the road and fireworks at night, Christmas markets with stalls, fairy
lights and trees in Germany, Austria, Switzerland, Czechia and Alsace, the Oktoberfest's tents in Bavaria, fireworks
everywhere at New Year and the northern lights in the far north from autumn to spring.

Every country keeps its own rules of the road. Where they drive on the left (Britain and Ireland, Japan, India, Southeast
Asia, Australia and the Pacific, southern and eastern Africa, much of the Caribbean) the slow traffic keeps left, cars
overtake on the right and you start in the left lane. The markings follow the country: a yellow line on the centre side
in the Americas, Scandinavia and East Asia, yellow outer edges in southern Africa and Ireland. Chevron boards are red and
white in Europe, black and white in Britain and the Commonwealth, yellow and black in the Americas, Australia and Asia;
warning signs are red triangles, yellow-backed triangles in Sweden, Finland, Poland, Greece, the Balkans, Korea and
Vietnam, black-bordered yellow triangles in China, yellow diamonds in the Americas, Australia and Japan. Each place
greets you with its name on a sign in the national style (yellow in Germany, red-bordered in France, blue in the
Netherlands, Switzerland and Japan, green in the Americas and Australia), and where the
country changes there is a border post with both flags and the new country's name. The traffic is regional: yellow cabs,
school buses, pickups and big rigs in North America, black cabs and red double-deckers in Britain, beetles and painted
chicken buses in Latin America, auto rickshaws, scooters and decorated trucks in South Asia, tuk-tuks and jeepneys in
Southeast Asia, kei trucks in Japan, road trains in the outback, matatus and land cruisers in Africa and Arabia, boxy
saloons and marshrutkas in the east, tractors on European country roads, all with the country's number plates (the
EU's blue band, Britain's yellow, America's coloured top, Japan's small green). Out in the country the animals of the
place cross the road, announced as they come: cattle and sheep (which like to stop in the middle), kangaroos, moose,
camels, elephants, zebras, llamas, bison, reindeer and yaks. After each place sign the speed limit stands, in miles per
hour in the United States and Britain and on a white board in North America; toll plazas cross the roads of France,
Italy, Spain, Japan, China, India, Mexico and others; speed cameras in Germany, Austria, Switzerland, France, the Low
Countries, Italy and Britain flash, and cost two seconds, if you pass them too fast (the limit stands before them).
The old towns of Amsterdam, Lisbon, the Romantic Road, Quebec, Cartagena and Ouro Preto are cobbled, and shake. The
houses are the region's own: canal houses in the Low Countries, machiya in Japan, white Cycladic cubes, falu-red
cottages in Scandinavia, izbas in Russia, azulejo facades in Portugal, bright casitas in Latin America, stilt houses in
Southeast Asia, shotgun houses on the Gulf and in the Caribbean, hutong gates in China, Queenslanders, Andean adobe,
New England clapboard, crofts in Scotland and Ireland and the tower houses of Yemen. Cities stand on plots in rows
some 300 m back from the road, cross streets between their blocks; in the country the houses gather in villages of two
or three rows here and there, with lone farms between. From afar (beyond 320 m ahead, or 130 m beside the road) every
prop is drawn lighter: small parts and dark panes left out, the lit windows kept, litter and tufts not at all.
Where a region sells by the road, its stands stand there, built its way, as many as it has and in their season (sixty
of them, `data/landmarks/83-stands.txt`): honesty boxes at farm gates from Bavaria to New Zealand, strawberry kiosks in
early summer, flower fields to cut yourself, farm stands, pumpkins, fireworks tents, boiled peanuts and maple syrup in
North America, fruit on three-wheelers in southern Italy, melon heaps round the Mediterranean and melon cages in
Russia, honey in the Balkans, grandmothers with buckets of berries, smoked fish, churchkhela in the Caucasus, lamb on
the spit, gozleme, tagines and fossils in Morocco, dates, samovars and tandir bread in Central Asia, dhabas, chai
stalls, sugarcane presses and fruit carts in South Asia, king coconuts in Sri Lanka, fruit stalls on bamboo, warungs
and street kitchens in Southeast Asia, unmanned vegetable shelves and vending machines in Japan, roast sweet potatoes in
China and Korea, the Andes' blankets of wares, fruit carts under parasols in Latin America, jerk drums in the
Caribbean, charcoal sacks, produce tables and kiosks in Africa, the padstal of the Karoo and the market tables of the
Pacific. The car crashes into them. Wind farms turn where the wind blows (the North Sea coasts, Spain, Texas, Gansu,
Tamil Nadu, Ceara); the Hill of Crosses stands in Lithuania, the towers of Svaneti in the Caucasus, dzongs in Bhutan,
meru towers and rice barns on Bali and Lombok; funfairs in summer, white lift bridges over Dutch canals, truck stops
and service areas, refineries with their flares, the yellow mine dumps of Johannesburg, barber shops with painted
boards and kraals in Africa, rice drying on racks in a Japanese autumn, balbals in the steppe, apachetas at the Andes'
passes. Alpine roads run under avalanche galleries, Aomori, Logan Pass, the Grossglockner and Trollstigen between walls
of snow in winter and spring, and where a plane comes in low over the road its runway lies beside it.

At most borders the road forks, as it did in OutRun. Almost two kilometres ahead two boards on the screen show where
each side goes (the place, its region and country, the distance counting down, the side you are on lit), gantries
announce it on the road, the road widens into two carriageways and a barrier with a crash cushion divides them: the
side you are on at its nose decides. The carriageways run side by side out of sight of the choice;
then the other one swings away into the fields, its traffic with it, and your road narrows again before the border.
The next stage is the place you chose, and the tour goes on from there.

Where two places lie on different landmasses (the continents, Britain, Ireland, Iceland, Japan's islands, Sicily,
Zanzibar, the Caribbean, the Pacific and every other island) and no land joins them (data/landlinks.txt: Africa and
Eurasia meet at Sinai), a stage on the sea floor lies between them, a third as long as one on land: a glass
tunnel of steel ribs with a light in every crown, the light of the sea instead of the sun, marine snow drifting
down. The sea is the one nearest the middle of the crossing, forty-two of them, each with its own floor and its own
life: the wreck of the Titanic in the North Atlantic, a U-boat in the Channel, the Christ of the Abyss in the Ligurian
Sea, the Thistlegorm on the Red Sea reef, the ghost fleet of Chuuk, Yonaguni's stone terraces, a galleon in the
Caribbean, a whale's skeleton on the sand. Shoals, sharks, hammerheads, whale sharks, humpbacks, orcas, dolphins,
mantas, turtles, jellyfish, seals, walruses, narwhals, penguins and marine iguanas swim close along the glass and over
it, sharks circle above the tube, and in the Bering Sea king crabs as wide as a car walk the bottom. The road sinks
into a walled cutting before the portal and rises out of one after it; the tube's glass is tinted, shafts of light
fall from the surface, bubbles rise, the world outside sounds muffled and now and then a whale calls. Twelve seas have
a research station on the floor.

Seven seas are crossed aboard instead: the Channel on the car train, the Irish Sea, the Baltic, the Strait of
Gibraltar, the Aegean, the Balearic Sea and the Java Sea on a ferry. The road runs straight to the quay past the signs
and the check-in, you drive into the ship's car deck (or the train's wagons) yourself, the crossing is a short ride
with the clock standing still (the ship between the coasts with its gulls, the train through its tunnel), and at the
far quay you drive off yourself.

Bridges are built the way the region builds them: concrete beams, tall viaducts over Alpine gorges, stone arches in
Europe, steel trusses in North America, suspension bridges over the Bosphorus and in New York, cable-stayed spans in
Dubai, Shanghai and Lofoten, covered wooden bridges in the Black Forest and Bhutan, and long low causeways over the
sea in the Florida Keys and the Maldives.

## Radio

Thirty-one generated stations, every track composed from its own seed and named. Genres: Sunset FM (synthwave),
Turbo House, Amiga Techno (acid bass, tracker arpeggios), Trance Gate, Jungle Pressure (drum and bass), 8-Bit Racer
(chiptune), Fusion Drive (funk fusion), Big Beat, Route 66 Radio (twang, harmonica, train beat) and Lo-Fi Cruise.
Regions: City Pop FM, Koto Wave (koto, ney, taiko), Bollywood Express (sitar, tabla, tanpura), Desert Caravan (oud,
darbuka, Hijaz), Afro Highway (kalimba, marimba, congas), Samba Avenida, Tango Turbo (bandoneon), Andes Wind (pan
flute, charango), Caribbean Steel, Mariachi Motor, Celtic Coast (whistle, fiddle, bodhran, pipes), Balkan Brass,
Gamelan Drive, Outback Drone (didgeridoo, clapsticks), Steppe Rider (fiddle, throat drone, gallop), Nordic Night, K-Pop
Neon (Korea), Fado Noite (Portugal: guitars, hardly any drums), Schlager Express (the German-speaking countries),
Polka Parade (Poland, Czechia, Slovakia) and Afrobeat Lagos (Nigeria, Ghana, Benin, Togo: horns, organ, a long
groove). Driving into a new region or country, the station sings its jingle and says where it plays from (LIVE FROM
BAVARIA, GERMANY).
A track picks its tempo, key, scale or mode (from major and minor to Hijaz, Bhairav, hirajoshi and pelog), chord
progression, two melodies, a lead instrument and an arrangement of 32 to 64 bars with fills and risers. In AUTO the
station follows the place: the countryside plays its region, cities and motorways their genre. M (gamepad: View)
switches the station, N skips to the next track; Settings > Radio and the pause menu do the same.

The generated stations follow the race: with little time left, or on the last kilometre of a race, a driving layer
of sixteenth hats and an alarm figure joins in; the pads open with the speed; checkpoint, stage and finish cues
land on the next beat.

Music files (`.ogg`, `.mp3`, `.flac`, `.wav`, `.xm`, `.mod`) in a `music` folder beside the game or in the settings
folder appear as the station MY MUSIC.

## Building

Windows (Visual Studio 2026):

    .\build.ps1            # Release, the game in bin\msvc
    .\build.ps1 -Test -Run

Linux (CMake 3.22 or newer, GCC 11 or newer; on Debian/Ubuntu `build-essential cmake libx11-dev libxrandr-dev
libxinerama-dev libxcursor-dev libxi-dev libgl-dev`):

    cmake --preset linux
    cmake --build --preset linux -j 8

The binary links the C++ runtime statically and needs only the C library; OpenGL and X11 are loaded at run time.
For the Steam Deck build it on any Linux machine (or in a distrobox on the Deck), copy `bin/linux/Sunstreak` over and
add it as a non-Steam game; it runs in Gaming Mode through XWayland. The Windows build also runs through Proton. The
game starts windowed on Windows and fullscreen on a Deck; the choice is kept in the settings.

Gamepads are read through raylib (GLFW); the rumble goes through SDL2, which CMake fetches as well.

On the Deck (or with any gamepad) the hints show the buttons as glyphs: the face buttons in their colours, shoulders
and triggers as labelled caps, the d-pad and the stick; with a keyboard they show the keys. The layout holds at 1280 x
800. Settings > PROFILE switches the performance in one go: BATTERY (40 frames at 70 % resolution, no shadows),
BALANCED (60 frames at 85 %, no shadows) or QUALITY (full resolution, shadows, as fast as the display); a first start on
the Deck takes BALANCED and the text a size larger.

Releases: `.\Deploy\build_release.ps1` builds, tests and packs `dist\Sunstreak-<version>-windows-x64.zip` and, through
WSL (Ubuntu 22.04), `dist/Sunstreak-<version>-linux-x86_64.tar.gz`, each with an empty `music` folder for your own.
On GitHub, `.github/workflows/release.yml` builds both archives for a tag `v*` and attaches them to its release.

## Settings and command line

Settings and records: `%APPDATA%\Sunstreak\settings.ini` (Windows), `~/.config/sunstreak/settings.ini` (Linux).

    --windowed | --fullscreen     this start only
    --seed N                      the endless run's track (otherwise random)
    --biome N|ID                  the first stage's biome (on the world tour: where it starts)
    --scenery world|random|mixed|N|ID   the endless run's scenery for this start (a biome's number or a place's ID such
                                  as TUSCANY: that one all the way)
    --play                        skip the title
    --championship                start a championship
    --rally N                     start rally N (1..10)
    --journey N                   start journey N (1..)
    --daily                       start the daily challenge
    --action replay|photo|reload|save|siren T   testing: the replay, the photo mode, reading the data again, saving
                                  the run or an ambulance or fire engine close by, after T seconds
    --resume                      continue the saved endless run
    --code XXXXX-XXXXX-XXXXX      drive a Track Lab code
    --screen lab|records|code|garage|passport|stamps|regions|country|postcards|album|awards|rallies|viewer|settings   open that screen
    --shape NAME                  the landmark the viewer opens on (with --screen country: the country)
    --shape-tour N                with --screen viewer: photograph N models from there on, one picture each, saved
                                  as the --screenshot prefix followed by the model's name
    --camera 0..4                 chase, far, bumper or cockpit camera; 4 from above (screenshots)
    --find tunnel|bridge|landmark|landmark2|fork|sea|seain|seaout|ferry|jump|hitchhiker|hero|train|sky|summit|hump|gallery|runway
                                 start just before the first tunnel, bridge, landmark (or the second), fork, undersea
                                 portal (or inside, or before its far portal), ferry or crest
    --season spring|summer|autumn|winter   that season (testing)
    --date MM-DD                  that date, for the seasons and the festivals (testing)
    --weather clear|rain|storm|fog|snow|sand|monsoon   that weather all the way (testing)
    --time place|clock|dawn|day|dusk|night     the time of day for this start
    --autopilot                   the car drives itself
    --travel                      travel: the world tour without a clock
    --look-back                   the camera looks back all the time (screenshots)
    --distance M                  start M metres down the road
    --size WxH                    window size for this start
    --quit-after S --screenshot F quit after S seconds, saving a screenshot
    --mute                        no sound (test runs)
    --stats                       start with the F3 performance overlay (CPU and GPU milliseconds per part of the frame)
    --bench F                     measure from six seconds into the drive, no vsync and no frame limit; with
                                  --quit-after, writes frame times, CPU parts and GPU passes to F
    --audio-test F.wav            render a scripted drive, each station and the adaptive layer offline,
                                  levels in F*.wav.txt

## Code

`src/Track` generates the road, `World` bakes it into chunks with land and props, `Car` and `Traffic` are the arcade
physics in road coordinates, `Race` the championship and its rivals, `Records` the codes, the top ten and the ghosts,
`Renderer` and `Shaders.h` draw (HDR, bloom, speed blur, sun shadows, lens flare, FXAA), `Audio` and `Music`
synthesise, `Haptics` rumbles, `Game` holds the rules and the camera and `GameUi` the screens. `tests/TestMain.cpp`
checks the generator, the car, the track codes and the championship (`ctest --preset msvc`).

## Data files

The world lives in `data/` as plain text, built into the program (CMake embeds it); set the environment variable
`SUNSTREAK_DATA` to a copy of the folder to try changes without building. F5 reads the files again while the game
runs (from `SUNSTREAK_DATA`, else from a `data` folder in the working directory, beside the program or a few folders
above it, so a build in the repository finds its own) and builds the road anew where you are; F6 opens the landmark
viewer, every landmark and creature alone under its first place's sky, to turn and zoom while you shape it.

- `places/*.txt`: every place. `place ID | NAME | COUNTRY | SKY | REGION`, then one builder step per line: the light
  (`sun`, `fog`, `clouds`, `sky`, `ranges`, `rain`, `snow`, `look`, `light`), the ground (`land`, `verge`, `water`,
  `fields`, `shine`, `road`, `kerbs`, `sand`), the land (`hills`, `sea`), the road (`drive`, `bends`, `posts`, `lamps`,
  `gantries`, `rails`, `tunnels`, `bridges`), the things beside it (`p KIND chance nearest farthest`, painted with `c`,
  varied with `v`, `pastel`, `floating`, `once`; `landmark NAME nearest farthest [scale]`; `scatter NAME ...` strews a
  landmark shape), `like ID` to start from another place or a `template`, `drop KIND` to take a rule away, `island NAME`
  for a place on an island of its own, `cobbles` for an old town, and for the seas `underwater`, `swim NAME weight
  liftMin liftMax [min max] [circle]` (circle: round over the tube, as sharks do) and `crossing ferry|train`.
  New places go into new files after the last one, so the numbers of the old ones (and the track codes that use them)
  stay; the records keep places by their IDs.
- `countries.txt`: each country's side of the road, vehicles, markings, signs, radio station, flag and landmass.
- `route.txt`: the order of the world tour and where each place is; the forks are found from it (where the country or
  the state changes and a neighbour lies a little further on).
- `seas.txt`: the seas and points in them; a crossing goes under the sea nearest its middle.
- `landlinks.txt`: landmasses joined by land (AFRICA EURASIA over Sinai); between them the tour stays on land.
- `landmarks/*.txt` and `creatures/*.txt`: shapes in a small language (`00-language.txt` describes it): boxes, tapered
  boxes, columns, cones, rods, balls, domes, roofs, windows, arches, rings, repeated, turned and mirrored. The creatures
  swim round the undersea tunnels; the ferry, the car train, the festivals' stalls and tents and the regional houses
  are shapes too.
- `festivals.txt`: the festivals, their dates and places and what they bring (bunting, lanterns, lights, lamps,
  stalls, tents, trees, confetti, fireworks, the aurora).
- `climate.txt`: the climate of each place: twelve monthly mean temperatures and twelve monthly rain sums (NASA POWER
  30-year climatology, made by `world_data.py` in the WorldData tools); places without a line keep the seasons of their
  latitude.
- `roads.txt`: each country's road customs: miles per hour, speed limit boards, toll plazas, speed cameras, number
  plates; how its roads are built and kept: quality, lane width, shoulders, kerbs, road studs, concrete, gravel; and
  how its traffic drives: how much of it (Numbeo's traffic index) and its ways (scooters, chaos, tractors, road trains);
  and how dirty its air is (Numbeo's pollution index, estimated for the countries it lacks; a city's own in its place's
  `pollution` step). The litter shapes say where they lie with a `litter` line.
- `journeys.txt`: the journeys, from a place of the route to another.
- `world/countries.txt`, `world/provinces.txt`: the outlines for the passport's map, from Natural Earth (public domain),
  made by `tools/make_world_outlines.py` from its `ne_50m_admin_0_countries` and `ne_10m_admin_1_states_provinces`.
- `script-names.txt`: the places' names in their own script (their `local` step) as strokes for the place signs, made
  by `tools/make_signs.py`: it sets each name in the Noto font of its script (shaped by HarfBuzz through Pillow), thins
  the letters to their centre lines and follows them. The Noto fonts are under the SIL Open Font License 1.1, and so
  are the strokes (`docs/OFL-Noto.txt`). Run it again after changing a `local` step.

The icon (`res/sunstreak.ico` for the Windows program, `src/IconPng.h` for the window) is drawn by `tools/make_icon.py`.
