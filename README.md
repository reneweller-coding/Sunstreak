# Sunstreak

An arcade racer in the spirit of *Lotus III: The Ultimate Challenge*: a low camera, a road that bends, climbs and
throws you over crests, traffic to slice through and a clock that only checkpoints refill. The endless mode drives a
track generated from a seed, one biome per 5 km stage, and it grows curvier, denser and stricter as you go.

C++20 on [raylib 5.5](https://github.com/raysan5/raylib) (fetched by CMake). Everything you see and hear is made in
code: the low-poly models, the sky, the stroke font, the V8 with its turbo, and the soundtrack.

## Playing

| | Keyboard | Gamepad / Steam Deck |
|---|---|---|
| Accelerate | Up / W | RT or A |
| Brake | Down / S | LT or X |
| Steer | Left Right / A D | left stick, d-pad |
| Boost | Space / Shift | B |
| Gear down / up (manual) | Q / E | LB / RB |
| Camera (chase, far, bumper) | C | Y |
| Pause | Esc / P | Start |
| Window / fullscreen | F11 or Alt+Enter | |

The clock starts at 55 s. Every checkpoint (2.5 km) adds time, a little less each stage. Near misses (passing
traffic close and fast) fill the boost tank and raise the combo; overtakes and speed add to the score. The run ends
when the clock reaches zero; the results show the track code to drive the same road again.

The eight biomes: Green Valley, Alpine Pass, Red Canyon, Frozen Peaks (snow, less grip), Sunset Coast, Night Metro,
Storm Marsh (rain, fog, lightning, puddles) and Neon Horizon.

Music files (`.ogg`, `.mp3`, `.flac`, `.wav`, `.xm`, `.mod`) in a `music` folder beside the game or in the settings
folder replace the generated soundtrack.

## Building

Windows (Visual Studio 2026):

    .\build.ps1            # Release, the game in bin\msvc
    .\build.ps1 -Test -Run

Linux and the Steam Deck (X11 development packages for raylib/GLFW needed):

    cmake --preset linux
    cmake --build --preset linux

On the Steam Deck the Windows build also runs through Proton (add it as a non-Steam game). The game starts windowed
on Windows and fullscreen on a Deck; the choice is kept in the settings.

## Settings and command line

Settings and records: `%APPDATA%\Sunstreak\settings.ini` (Windows), `~/.config/sunstreak/settings.ini` (Linux).

    --windowed | --fullscreen     this start only
    --seed N                      the endless run's track (otherwise random)
    --biome 0..7                  the first stage's biome
    --play                        skip the title
    --autopilot                   the car drives itself
    --distance M                  start M metres down the road
    --size WxH                    window size for this start
    --quit-after S --screenshot F quit after S seconds, saving a screenshot
    --mute                        no sound (test runs)
    --audio-test F.wav            render a scripted drive's sound offline, with levels in F.wav.txt

## Code

`src/Track` generates the road, `World` bakes it into chunks with land and props, `Car` and `Traffic` are the arcade
physics in road coordinates, `Renderer` and `Shaders.h` draw (HDR, bloom, speed blur, FXAA), `Audio` synthesises,
`Game` holds the rules, the camera and the screens. `tests/TestMain.cpp` checks the generator and the car
(`ctest --preset msvc`).
