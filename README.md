# Sunstreak

An arcade racer in the spirit of *Lotus III: The Ultimate Challenge*: a low camera, a road that bends, climbs and
throws you over crests, traffic to slice through and a clock that only checkpoints refill. The endless mode drives a
track generated from a seed, one biome per 5 km stage, and it grows curvier, denser and stricter as you go. By
default it is a world tour: 732 places in all 202 countries and territories it visits, every member of the United
Nations among them, the big countries state by state and province by province, round the globe. At the borders the
road forks and you choose where to go next; between continents and to the islands you drive along the sea floor in a
glass tunnel.

Beside the endless run there is a championship against eleven rivals, a Track Lab that turns any setting into a
shareable track code, a ghost of your best drive on every code and a top ten.

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
| Camera (chase, far, bumper) | C | Y |
| Radio station / next track | M / N | View |
| Pause | Esc / P | Start |
| Window / fullscreen | F11 or Alt+Enter | |
| Performance overlay | F3 | |

The clock starts at 55 s. Every checkpoint (2.5 km) adds time, a little less each stage. Near misses (passing
traffic close and fast) fill the boost tank and raise the combo; overtakes and speed add to the score. The run ends
when the clock reaches zero; the results show the track code to drive the same road again.

Gamepads rumble on hits, crashes, landings, boosts and gear changes and buzz on kerbs and gravel (Settings >
VIBRATION; on the Deck through Steam Input).

## Modes

- **Endless run**: the clock, the checkpoints and the stages, on a fresh random track each time.
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
- **Replay and photo mode**: after a run, WATCH REPLAY plays the last minute again from roadside cameras, a helicopter
  and the chase camera. PHOTO MODE (pause menu) stops the game and lets the camera roam round the car; a photo is saved
  in the settings folder's `photos` and becomes the place's postcard.
- **Passport**: the world tour on a map (every place a dot, the ones you have seen lit, the twelve slices of the globe
  you have driven through as a bar: all twelve is a drive round the world), a stamp for every country, and an album of
  postcards: every new place you drive through sends one home, taken in sight of its landmark.
- **Records**: the top ten (with your initials), total kilometres, titles, places and countries seen. The best drive
  on each code is kept as a ghost car that races along the next time.

Unlocks: the SPARROW (light, revvy four-cylinder) after 30 km, the VORTEX (V12, fastest) after 100 km or a title, the
KESTREL (a turbocharged rally hatchback that keeps its grip on gravel and sand) after 40 countries. The GARAGE dresses
the car: fourteen paints, seven rim finishes and seven decals (twin stripes, a side stripe, a roundel, two tones,
chequers, a globe) in any paint's colour. Career points from every run, every race and every new place and country pay
for them; SUN YELLOW, DEEP BLUE and CHAMPION GOLD keep their old conditions, GLOBETROTTER needs a drive round the world,
the globe 50 countries.

Gravel, dirt and sand tracks (the Gibb River Road, the Yungas, the Sani Pass, the Road of Bones, the Sahara) have no
markings, crunch under the tyres, throw up dust and hold less than asphalt.

Assists and accessibility (Settings): STEER ASSIST (light or strong) steers where your hands are idle, keeps the road
and goes round the traffic; BRAKE ASSIST brakes for bends and obstacles taken too fast; COLOUR BLIND swaps the HUD's
red and green for vermilion and sky blue; TEXT SIZE enlarges every text to 115 or 130 %.

Difficulty (Settings: EASY, NORMAL, HARD) scales the grip, the clock, the traffic and the rivals. The handling itself
is in `tuning.ini` beside the settings (written with comments on the first start): steer, centrifugal, accel, brake,
boost, grip, crash_keep, time, traffic, rivals. Change a value and restart; delete the file for the defaults.

786 biomes. The world tour (Settings > SCENERY) runs east from Ireland: the British Isles, France, Germany and the
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
begins, a sign says so. The famous roads have their
own: the Stelvio, the Transfagarasan, the Atlantic Road, Trollstigen, the Nordschleife, the Yungas Road, the Road of
Bones, the Dalton Highway beside its pipeline, the Nullarbor with its kangaroo, camel and cow signs, Bonneville, the
Tail of the Dragon and the Road to Hana. Landmarks stand where they belong, once per stage: Big Ben and Tower Bridge,
the Arc de Triomphe, the Colosseum and St Peter's, the Leaning Tower, the Sagrada Familia, Neuschwanstein, the
Brandenburg Gate, the Acropolis, St Basil's, the Christ the Redeemer and Cristo Rei statues, the Statue of Liberty,
the Opera House, the Petronas Towers, the Space Needle, the Capitol and the Washington Monument, the HOLLYWOOD sign,
the Sphinx, the Colossi of Memnon, Borobudur, Harbin's ice castle, the burning crater of Darvaza, Great Zimbabwe,
Bled's island church and the torii of Miyajima in the sea, and over a hundred and fifty more built from the data files:
the Eiffel Tower, Cologne Cathedral, the Elbphilharmonie, the Wartburg and the Frauenkirche, the Gateway Arch, the
Golden Gate, the Alamo, Mount Rushmore and Devils Tower, the CN Tower and the Chateau Frontenac, the Matterhorn, the
Pont du Gard and Carcassonne, the Taj Mahal, the Golden Temple and the Statue of Unity, the Terracotta Army and the
Leshan Buddha, Himeji Castle and the Skytree, the Motherland Calls and Kizhi, the Hagia Sophia, Petra, Abu Simbel,
Machu Picchu, Shwedagon, Djenne's mud mosque, Preikestolen and many more. ALL MIXED shuffles every biome, the twelve
that are not on the map included: Night Metro, Storm Marsh, Neon Horizon, Neon Rain, Dawn Expressway, Lunar Highway (low gravity),
Red Planet, Crystal Desert, Fungal Glade, Cloud Highway, Pumpkin Hollow and Total Eclipse.

Weather fronts pass over the tour (Settings > WEATHER): showers, fog banks and thunderstorms in temperate places,
downpours in the tropics, snow showers in the cold and the mountains, sandstorms in the deserts. They are announced a
little ahead, last a kilometre or two, and wet, snowy or sandy roads hold less. TIME OF DAY keeps each place's own light
by default; TOUR CLOCK lets the sun follow the time zones (morning wherever the run starts, an hour more every ten
kilometres, and the longitude of each place on top: east of you it is already evening); DAWN, DAY, DUSK and NIGHT fix
the hour. Every place sounds like itself: the surf on the shore it lies on, wind over deserts and mountains, insects
and birds in the tropics, the hum and horns of a city, rain on the car, the joints of a bridge and the engine echoing
in a tunnel.

Every country keeps its own rules of the road. Where they drive on the left (Britain and Ireland, Japan, India, Southeast
Asia, Australia and the Pacific, southern and eastern Africa, much of the Caribbean) the slow traffic keeps left, cars
overtake on the right and you start in the left lane. The markings follow the country: a yellow line on the centre side
in the Americas, Scandinavia and East Asia, yellow outer edges in southern Africa and Ireland. Chevron boards are red and
white in Europe, black and white in Britain and the Commonwealth, yellow and black in the Americas, Australia and Asia;
warning signs are red triangles, yellow-backed triangles in Sweden, Finland, Poland and the Balkans, yellow diamonds in
the Americas, Australia and Japan. Each place greets you with its name on a sign in the national style (yellow in
Germany, red-bordered in France, blue in the Netherlands and Japan, green in the Americas and Australia), and where the
country changes there is a border post with both flags and the new country's name. The traffic is regional: yellow cabs,
school buses, pickups and big rigs in North America, black cabs and red double-deckers in Britain, beetles and painted
chicken buses in Latin America, auto rickshaws, scooters and decorated trucks in South Asia, tuk-tuks and jeepneys in
Southeast Asia, kei trucks in Japan, road trains in the outback, matatus and land cruisers in Africa and Arabia, boxy
saloons and marshrutkas in the east, tractors on European country roads. Out in the country the animals of the place cross
the road, announced as they come: cattle and sheep (which like to stop in the middle), kangaroos, moose, camels,
elephants, zebras, llamas, bison, reindeer and yaks.

At most borders the road forks, as it did in OutRun. A gantry announces where each side goes (THE ROAD FORKS
< BELGIUM  FRANCE > on the screen as well), the road widens into two carriageways and a barrier with a crash cushion
divides them: the side you are on at its nose decides. The carriageways run side by side out of sight of the choice;
then the other one swings away into the fields, its traffic with it, and your road narrows again before the border.
The next stage is the place you chose, and the tour goes on from there.

Where two places lie on different landmasses (the continents, Britain, Ireland, Iceland, Japan's islands, Sicily,
Zanzibar, the Caribbean, the Pacific and every other island), a stage on the sea floor lies between them: a glass
tunnel of steel ribs with a light in every crown, the light of the sea instead of the sun, marine snow drifting
down. The sea is the one nearest the middle of the crossing, forty-two of them, each with its own floor and its own
life: the wreck of the Titanic in the North Atlantic, a U-boat in the Channel, the Christ of the Abyss in the Ligurian
Sea, the Thistlegorm on the Red Sea reef, the ghost fleet of Chuuk, Yonaguni's stone terraces, a galleon in the
Caribbean, a whale's skeleton on the sand. Shoals, sharks, hammerheads, whale sharks, humpbacks, orcas, dolphins,
mantas, turtles, jellyfish, seals, walruses, narwhals, penguins and marine iguanas swim along the tunnel and over it,
and in the Bering Sea king crabs as wide as a car walk the bottom.

Bridges are built the way the region builds them: concrete beams, tall viaducts over Alpine gorges, stone arches in
Europe, steel trusses in North America, suspension bridges over the Bosphorus and in New York, cable-stayed spans in
Dubai, Shanghai and Lofoten, covered wooden bridges in the Black Forest and Bhutan, and long low causeways over the
sea in the Florida Keys and the Maldives.

## Radio

Twenty-six generated stations, every track composed from its own seed and named. Genres: Sunset FM (synthwave),
Turbo House, Amiga Techno (acid bass, tracker arpeggios), Trance Gate, Jungle Pressure (drum and bass), 8-Bit Racer
(chiptune), Fusion Drive (funk fusion), Big Beat, Route 66 Radio (twang, harmonica, train beat) and Lo-Fi Cruise.
Regions: City Pop FM, Koto Wave (koto, ney, taiko), Bollywood Express (sitar, tabla, tanpura), Desert Caravan (oud,
darbuka, Hijaz), Afro Highway (kalimba, marimba, congas), Samba Avenida, Tango Turbo (bandoneon), Andes Wind (pan
flute, charango), Caribbean Steel, Mariachi Motor, Celtic Coast (whistle, fiddle, bodhran, pipes), Balkan Brass,
Gamelan Drive, Outback Drone (didgeridoo, clapsticks), Steppe Rider (fiddle, throat drone, gallop) and Nordic Night.
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
    --biome N                     the first stage's biome (on the world tour: where it starts)
    --scenery world|mixed|N|ID    the endless run's scenery for this start (a biome's number or a place's ID such
                                  as TUSCANY: that one all the way)
    --play                        skip the title
    --championship                start a championship
    --rally N                     start rally N (1..10)
    --daily                       start the daily challenge
    --action replay|photo T       testing: the replay or the photo mode after T seconds of racing
    --code XXXXX-XXXXX-XXXXX      drive a Track Lab code
    --screen lab|records|code|garage|passport|stamps|postcards|rallies   open that screen
    --camera 0..2                 chase, far or bumper camera
    --find tunnel|bridge|landmark|landmark2|fork
                                 start just before the first tunnel, bridge, landmark (or the second) or fork
    --weather clear|rain|storm|fog|snow|sand   that weather all the way (testing)
    --time place|clock|dawn|day|dusk|night     the time of day for this start
    --autopilot                   the car drives itself
    --distance M                  start M metres down the road
    --size WxH                    window size for this start
    --quit-after S --screenshot F quit after S seconds, saving a screenshot
    --mute                        no sound (test runs)
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
`SUNSTREAK_DATA` to a copy of the folder to try changes without building.

- `places/*.txt`: every place. `place ID | NAME | COUNTRY | SKY | REGION`, then one builder step per line: the light
  (`sun`, `fog`, `clouds`, `sky`, `ranges`, `rain`, `snow`, `look`, `light`), the ground (`land`, `verge`, `water`,
  `fields`, `shine`, `road`, `kerbs`, `sand`), the land (`hills`, `sea`), the road (`drive`, `bends`, `posts`, `lamps`,
  `gantries`, `rails`, `tunnels`, `bridges`), the things beside it (`p KIND chance nearest farthest`, painted with `c`,
  varied with `v`, `pastel`, `floating`, `once`; `landmark NAME nearest farthest [scale]`; `scatter NAME ...` strews a
  landmark shape), `like ID` to start from another place or a `template`, `drop KIND` to take a rule away, `island NAME`
  for a place on an island of its own, and for the seas `underwater` and `swim NAME weight liftMin liftMax [min max]`.
  New places go into new files after the last one, so the numbers of the old ones (and the track codes that use them)
  stay; the records keep places by their IDs.
- `countries.txt`: each country's side of the road, vehicles, markings, signs, radio station, flag and landmass.
- `route.txt`: the order of the world tour and where each place is; the forks are found from it (where the country or
  the state changes and a neighbour lies a little further on).
- `seas.txt`: the seas and points in them; a crossing goes under the sea nearest its middle.
- `landmarks/*.txt` and `creatures/*.txt`: shapes in a small language (`00-language.txt` describes it): boxes, tapered
  boxes, columns, cones, rods, balls, domes, roofs, windows, arches, rings, repeated, turned and mirrored. The creatures
  swim round the undersea tunnels.
