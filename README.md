# Sunstreak

An arcade racer in the spirit of *Lotus III: The Ultimate Challenge*: a low camera, a road that bends, climbs and
throws you over crests, traffic to slice through and a clock that only checkpoints refill. The endless mode drives a
track generated from a seed, one biome per 5 km stage, and it grows curvier, denser and stricter as you go.

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
- **Track Lab**: mode (time attack or a race against the eleven), scenery (mixed or one biome), curves, sharpness,
  hills, traffic and obstacles (0..9 or AUTO), length (2.5..37.5 km, or endless in time attack) and the seed. Every
  combination is a 14-character code such as `K3ZQ-8M2T-VX41`; CODE types one in, the results show the code of every
  run.
- **Records**: the top ten (with your initials), total kilometres, titles, biomes seen. The best drive on each code
  is kept as a ghost car that races along the next time.

Unlocks: the SPARROW (light, revvy four-cylinder) after 30 km, the VORTEX (V12, fastest) after 100 km or a title;
the paints SUN YELLOW (15 km), DEEP BLUE (50 km) and CHAMPION GOLD (a title).

Difficulty (Settings: EASY, NORMAL, HARD) scales the grip, the clock, the traffic and the rivals. The handling itself
is in `tuning.ini` beside the settings (written with comments on the first start): steer, centrifugal, accel, brake,
boost, grip, crash_keep, time, traffic, rivals. Change a value and restart; delete the file for the defaults.

38 biomes, shuffled per track code, each stage another: Green Valley, Alpine Pass, Red Canyon, Frozen Peaks, Sunset
Coast, Night Metro, Storm Marsh, Neon Horizon, Amber Woods, Sakura Hills, Jungle Run, Golden Savanna, Volcano Ridge
(lava), Aurora Tundra (northern lights), Lavender Vale, Tulip Polder, Misty Highlands, Fjord Light, Rice Terraces,
Salt Flats, Mesa Sunset, Suburbia, Steel Harbor, Neon Rain, Dawn Expressway (sound walls, sign gantries), Lunar
Highway (the Earth in the sky, low gravity), Red Planet, Crystal Desert, Fungal Glade, Cloud Highway (a ridge above
the clouds), Iceberg Bay, Bayou Dusk, Wheat Plains, Chalk Cliffs, Pumpkin Hollow, Frosted Village, Rainbow Ridge and
Total Eclipse.

## Radio

Eight generated stations, every track composed from its own seed and named: Sunset FM (synthwave), Turbo House,
Amiga Techno (acid bass, tracker arpeggios), Trance Gate, Jungle Pressure (drum and bass), 8-Bit Racer (chiptune),
Fusion Drive (funk fusion) and Big Beat. In AUTO the station follows the biome. M (gamepad: View) switches the
station, N skips to the next track; Settings > Radio and the pause menu do the same.

The generated stations follow the race: with little time left, or on the last kilometre of a race, a driving layer
of sixteenth hats and an alarm figure joins in; the pads open with the speed; checkpoint, stage and finish cues
land on the next beat.

Music files (`.ogg`, `.mp3`, `.flac`, `.wav`, `.xm`, `.mod`) in a `music` folder beside the game or in the settings
folder appear as the station MY MUSIC. `Deploy\render_music.ps1` fills `bin\msvc\music` with pieces from the VRAudio
generators (Totality, Parhelion, Skybreak), levelled to the stations' loudness.

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

Releases: `.\Deploy\build_release.ps1` builds, tests and packs `dist\Sunstreak-<version>-windows-x64.zip` and, through
WSL (Ubuntu 22.04), `dist/Sunstreak-<version>-linux-x86_64.tar.gz`, each with the MY MUSIC folder if there is one.
On GitHub, `.github/workflows/release.yml` builds both archives for a tag `v*` and attaches them to its release.

## Settings and command line

Settings and records: `%APPDATA%\Sunstreak\settings.ini` (Windows), `~/.config/sunstreak/settings.ini` (Linux).

    --windowed | --fullscreen     this start only
    --seed N                      the endless run's track (otherwise random)
    --biome 0..37                 the first stage's biome
    --play                        skip the title
    --championship                start a championship
    --code XXXX-XXXX-XXXX         drive a Track Lab code
    --screen lab|records|code     open that screen
    --camera 0..2                 chase, far or bumper camera
    --find tunnel|bridge          start just before the first tunnel or bridge
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
