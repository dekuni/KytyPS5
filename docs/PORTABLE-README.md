# KytyPS5 for Demon's Souls: read me first

This is a Windows build of the KytyPS5 PlayStation 5 emulator, tuned for one game: Demon's Souls
(PS5, PPSA01341). It comes from https://github.com/exploitz/KytyPS5, where newer builds are listed
under Releases.

It is a fork of chenxiao07's Demon's Souls branch of KytyPS5 (https://github.com/chenxiao07/KytyPS5).
Nearly all of the work that makes Demon's Souls run well in this emulator is chen's: the table path
(GPU-driven draws), native XPR, the shader precompile and the graphics presets. This fork adds further
speed-ups, fixes, display options and its own launcher on top. The repository's front page lists them,
with performance measurements.

You need a dump of your own copy of the game. This package contains no game files.


## Quick start

1. Unzip this package into a folder of its own on an SSD with about 20 GB free, for example
   `C:\Games\KytyPS5`. Everything the emulator makes (shader caches, logs, your saves) is kept inside
   this folder, so do not put it in `C:\Program Files`, the Downloads or Temp folder, or OneDrive.
2. Double-click **KytyPS5 Launcher.exe**. If Windows says "Windows protected your PC", click
   "More info", then "Run anyway" (the programs are not signed; see Troubleshooting).
3. The setup runs once:
   - **PC**: a quick check of your processor, graphics card, memory, page file and disk, with a hint
     for anything that needs attention.
   - **Game**: choose your game dump, the folder that directly contains `eboot.bin` and the `sce_sys`
     folder (usually named `PPSA01341-app0`), or a `.zar` archive of it.
   - **Display**: settings picked for your screen and processor. You can change them later.
   - **Shaders**: "Precompile now" (recommended) compiles all of the game's shaders once, in the
     launcher, while you wait or do something else.
4. Click **Play**.

After that, start the launcher and click Play. It remembers your settings.


## Supported game version

- Game: Demon's Souls, title ID PPSA01341 (content ID EP9000-PPSA01341_00-DEMONSSOULS00000).
- Version 1.07 (01.007.000): tested with this fork.
- Version 1.05 (01.005.000): supported by chen's branch; this fork's additions were tested on 1.07 only.
- The US release (PPSA01342) gets the same game patches and settings; it is untested.

Each version keeps shader caches of its own, so the precompile is done once per version. The launcher
reads the game's `sce_sys\param.json` and warns when the version is not one of these. Other versions,
regions and games are untested and may not run.


## System requirements

These are recommendations. Much slower PCs may not run the game or may stutter badly.

- Windows: Windows 10 (22H2) or 11, 64-bit. This build is for Windows only; Linux is planned, macOS
  is not supported.
- Processor (CPU): one with AVX2 (Intel Core 4th generation, AMD Ryzen, or newer). Its single-thread
  speed matters most, because the emulator's render thread runs on one core. More threads make the
  shader precompile faster.
- Graphics card (GPU): Vulkan 1.3 with 12 GB of video memory recommended. 8 GB works, with lower frame
  rates in heavy areas (from chen's notes). Tested on NVIDIA RTX; AMD and Intel GPUs are untested.
  Install the latest graphics driver (Vulkan comes with it).
- Memory (RAM): 32 GB recommended. 16 GB works, but loading takes longer, because Windows moves part of
  the game's memory to the page file.
- Page file: Windows must be able to give the emulator about 34 GB of RAM plus page file. Keep the page
  file on "System managed size" (see Troubleshooting); the launcher warns when it is too small.
- Disk: about 20 GB free for the shader caches (static precompile about 7 GB, translation store about
  5 GB, driver cache about 1 GB), on top of the game dump. An NTFS drive (not FAT32 or exFAT: the
  precompiled shaders are one ~7 GB file), an SSD recommended.


## The launcher

The launcher has three pages, in the bar on the left.

- **Home**: the game, the **Play** button, and the shader state ("Shaders ready", or "Shaders not
  precompiled" with a "Precompile" button). While the game starts, the Play button shows what it is
  doing; when the game window appears, the launcher minimizes, and it comes back when you quit. A
  precompile shows its progress, the time left and a Stop button here. "Show log" opens the emulator's
  live output, "Report a problem" makes a report for a bug (see Troubleshooting).
- **Settings**: every setting below. Point at a setting to see what it does at the bottom of the page.
  "Run setup again" repeats the first-run setup.
- **Help**: this guide, the logs folder, Report a problem, Export shader hints, the Releases page and the
  version.

Keyboard: Enter on Home starts the game, Esc closes dialogs and the log, Tab moves between controls.


## Settings

Performance numbers come from the test PC (Intel Core i9-12900K, NVIDIA GeForce RTX 3080 Ti, 64 GB RAM);
yours will differ.

### Output

How large the game draws its 3D scene and what size picture it sends to your screen.

- "4K (PS5)": the PS5's Performance mode. The 3D scene is drawn at 2560x1440 and the game builds a
  3840x2160 (4K) picture from it, with its HUD; that picture is then scaled to fit your window or
  screen. Best on 1440p and 4K screens.
- "1080p Quality": the same 1440p scene, scaled down straight to 1920x1080, with the HUD at its normal
  size. The sharpest picture on a 1080p screen; about 3% slower than 1080p Performance.
- "1080p Performance": the 3D scene drawn at 1920x1080. Faster on weaker graphics cards; the picture is
  softer.

The two 1080p modes rely on small patches to the game's code, applied when it starts and located by the
game's original bytes; they keep the HUD at its normal size. They are made for version 1.07.

### 3D resolution (4K output only)

Draws the 3D scene smaller inside the 4K picture ("1440p (PS5)" down to "900p"); the HUD stays sharp.
For weaker graphics cards on a 1440p or 4K screen.

### Graphics detail

How much the game draws. Each object drawn costs the emulator processor time, so these presets help most
when your CPU limits the frame rate.

- "Standard (PS5)": everything the PS5 draws.
- "Fewer shadows" (this fork's): no ambient shadowing (the soft shading where objects meet), no shadows
  from torches and other small lights (they still light the scene), and sun shadows only up to 60 m.
  The scenery is left as it is. The first thing to try on a slower CPU: +12% at Boletaria 1-1, +15% in
  the Tower of Latria, +17% in the Nexus on the test PC.
- "Low" (chen's): shadows drawn from simpler meshes, simpler versions of distant objects chosen sooner,
  sun shadows up to 125 m. +9% to +15%.
- "Lowest" (chen's): simpler distant objects still sooner, sun shadows up to 60 m, no ambient shadowing
  and no small-light shadows. +23% to +30%.
  Low and Lowest also swap some foreground objects, such as dead trees and a cart, for simpler meshes or
  none, and the Nexus loses its glowing runes. Use them when "Fewer shadows" is not enough.

### Anti-aliasing

- "Game TAA" (default): the game's own anti-aliasing, as on PS5.
- "FSR 4 (experimental)": replaces the game's TAA at the same resolution (it does not upscale): cleaner
  edges and less shimmer, about 13% slower. It is built from an older community-hosted FSR 4 "v07"
  source, not AMD's official FSR 4, so expect occasional image problems. It needs a graphics card with
  certain shader features (8-bit integer dot products, 16-bit floats and compute-shader derivatives;
  NVIDIA RTX 30 series cards have them). On a card without them, the game keeps its own TAA and the log
  says why.

DLSS is not part of this release: NVIDIA's SDK license is not compatible with shipping it inside a GPL
program. It may come later as a separate, optional add-on.

### Frame generation

- "Off": every frame you see is one the game rendered.
- "FSR 3.1": AMD FidelityFX frame generation, on any graphics card. A generated frame is shown between
  each two real ones, so motion looks smoother. The HUD is kept out of generated frames, so text and
  bars do not smear; generation pauses in menus, movies and loading screens. It adds a little input lag
  and some graphics card time; best when the game already runs steadily near 60.

### Display mode

Fullscreen at your screen's resolution, or a window of a fixed size. F11 or Alt+Enter switches between
window and full screen while playing.

### Keep 16:9

In fullscreen on a screen that is not 16:9 (ultrawide, 16:10): black bars instead of stretching.

### Frame rate limit

- "60 (PS5)" (default): the game's own limit.
- "120": lets the game run above 60 where your PC is fast enough. The game keeps its normal speed, but
  the pre-rendered movies play faster.

### VSync

Off: the lowest input lag; tearing (a horizontal split line in motion) is possible. On: no tearing; on a
G-SYNC or FreeSync display the refresh rate keeps adapting to the game.

### HDR

HDR10 output for an HDR display. Turn on "Use HDR" for that display in Windows first (Settings > System
> Display); without it the picture stays SDR.

### Performance overlay

The emulator's own panel at the top right of the game. "Frame rate": the frames per second and a graph of
the last frames' times (60 and 30 fps lines, hitches in red, the 1% low and the longest frame of the
last 10 seconds). "Detailed": adds the render thread's time in each frame, the graphics card (load,
clock, temperature, power and video memory on NVIDIA cards), the processor's load per core and the memory
in use.

### Language

The console language the game sees: its text and voices follow it where the game has them.

### Memory protection

Keep on. It protects the game's memory from a Windows behaviour that otherwise corrupts it at random
(crashes, odd behaviour).

### Precompile on E-cores

On an Intel processor with efficiency cores: the shader precompile runs on those only, so the PC stays
responsive. It takes longer.


## First launch

The first Play takes a few extra steps, shown on the Play button:

1. The game's shaders are listed from its files, once (about a minute).
2. The inputs for the background shader preparation are made, once per graphics card and driver version
   (about a minute).
3. The game window appears. The start-up screen shows the loading progress; in the game, the top right
   corner shows "Preparing shaders xx%" while shaders are prepared in the background. On the first
   launch the screen can stay black for about a minute while shaders are prepared: this is normal.

Later launches go straight into the game. To quit, close the game window. If the game crashes, the
launcher shows what happened and offers Report a problem.


## Shader precompile

The emulator translates the game's shaders and compiles them for your graphics card. When a scene,
effect or enemy appears for the first time and its shaders are not ready, the game stutters for a
fraction of a second to a few seconds, most noticeably on entering a new area. Once compiled, a shader is
remembered and does not stutter again.

The emulator keeps three kinds of shader work, all inside `_PipelineCache`:

1. Translation store (automatic, added by this fork): translated shaders are kept on disk between
   launches. The first launch translates about 20,000 shaders (a few minutes); later launches load them.
   About 5 GB.
2. Background shader preparation (automatic): after every launch, the game's shaders are prepared in the
   background while the top right corner shows the progress. The CPU is busy meanwhile.
3. Full precompile (recommended once): compiles every shader of the game ahead of time. Afterwards,
   most shader stutter is gone; a few effects are still built in the background the first time they
   appear (a short hitch, once). About 7 GB.
   - Start it in the setup, or with "Precompile" on the Home page.
   - It takes from under an hour to a few hours, depending on the CPU and the graphics driver: 2 h 34 min
     on the test PC (Core i9-12900K, 24 threads), about 40 minutes on chen's PC. The launcher shows an
     estimate for your PC and, once it runs, the time left. The CPU is fully loaded meanwhile, so run it when you do
     not need the PC. You can play while it runs, but new scenes stutter until it is done.
   - Stop ends it; the next run continues where it stopped.
   - After a graphics driver update or a new graphics card it must be done again; the launcher says so.
   - Places and effects the test PC never recorded (bosses' deaths, for one) can still compile when they
     first appear. "Export shader hints" under Help writes what your own play recorded, without the game's
     shader code, to `my-hints-<title>_<version>.hints` (a few hundred KB) and shows it in Explorer:
     attached to an issue, it goes into the next releases' precompile.

The graphics driver's own cache for the emulator (about 1 GB) is kept in `_PipelineCache` too.


## Performance tips

First find out what limits your frame rate. Turn on the Performance overlay, then set Output to "1080p
Performance". If the frame rate rises clearly, your graphics card is the limit. If it hardly changes,
your CPU is the limit; that is the more common case with this emulator.

- Slower CPU: set Graphics detail to "Fewer shadows" first (+12% to +17% on the test PC, scenery kept).
  If that is not enough, try "Low" or "Lowest" (more speed, but some objects are simplified).
- Slower graphics card: use Output "1080p Performance" (or a lower 3D resolution with the 4K output),
  and keep Anti-aliasing on Game TAA (FSR 4 costs about 13%).
- Smoother motion: if the game runs steadily but below your display's refresh rate, try Frame generation
  "FSR 3.1". It does not make the controls respond faster.
- Run the shader precompile once, and again after each graphics driver update.
- Close memory-hungry programs (web browsers, for example), especially with 16 GB of RAM.

What to expect: on the test PC (Core i9-12900K, RTX 3080 Ti) with the normal 60 fps limit and Output
"4K (PS5)", most areas hold 60 fps while moving; the heaviest views, such as Boletaria 1-1 looking at the
castle, run in the high 40s. Results vary by PC.


## Controls

A controller is recommended. PS5 (DualSense), PS4 (DualShock 4) and Xbox controllers work; plug one in. A
DualSense connected by USB also gives the game its vibration, its speaker and the adaptive triggers.
Face buttons go by position, as on a DualSense: the bottom button is Cross, right is Circle, left is
Square and top is Triangle (on an Xbox controller: A, B, X, Y). Xbox controllers have no touch pad
button; use Backspace or Tab on the keyboard for it.

Default keyboard layout:

| Key | Does |
| --- | --- |
| W A S D | Left stick (move) |
| T F G H | Right stick (camera: T up, G down, F left, H right) |
| J / L / K / I | Cross / Circle / Square / Triangle |
| Q / E | L1 / R1 |
| Shift / Ctrl | L3 / R3 |
| Arrow keys | D-pad |
| Enter | OPTIONS |
| Backspace / Tab | Left / right half of the touch pad |
| F7 | Camera with the mouse (press again to release the mouse) |
| Pause/Break | Pause or resume the emulation |
| F11, Alt+Enter | Switch between window and full screen |
| F9 / F10, F8 | Debug warp (see below) |

The keyboard has no L2 / R2 by default: use a controller when you need them.

Debug warp, for testing other areas: F9 and F10 show the list of spawn points of every map and move
through it; F8 arms the one shown (press F8 again to cancel). Then quit to the title screen (OPTIONS >
Settings, the gear tab > Exit Game > Save and Exit Game) and choose Continue: the game loads on that map.
Your character keeps everything else (level, items, world progress). Some spawn points are at a boss's
fog gate. Back up `_SaveData` before using it.


## Troubleshooting

The game closes by itself, or Windows says it is low on memory
- The emulator needs about 34 GB of RAM plus page file. Set the page file to be managed by Windows:
  Settings > System > About > Advanced system settings > Performance: Settings > Advanced > Virtual
  memory: Change. Tick "Automatically manage paging file size for all drives" (or choose "System managed
  size" for the system drive), click OK and restart the PC.
- Keep some free space on the system drive, so the page file can grow. Close other programs while
  playing.

New areas or effects stutter
- Run the shader precompile ("Precompile" on Home). Run it again after every graphics driver update or a
  new graphics card.

"Untested version" or "Untested region"
- Your dump is not PPSA01341 version 1.07 or 1.05. It may not run, or may fail later. Update your copy of
  the game to 1.07 and dump it again.

Black screen at start
- On the first launch, shader preparation can take about a minute before the picture appears. "Show log"
  on Home shows the progress.

"Windows protected your PC", or the antivirus removes the emulator
- The programs are not signed, which some security software treats as suspicious. Click "More info",
  then "Run anyway". Before unzipping, you can also right-click the zip > Properties > tick "Unblock".
- If your antivirus quarantines `kyty_emulator.exe`, the launcher says the package is incomplete: restore
  the file and add an exception for this folder, but only for a zip downloaded from the official Releases
  page.

Reporting a bug
- Click "Report a problem" (Home or Help). It saves `KytyPS5-report-<date>.zip` to your Desktop, with the
  newest logs and a summary of your PC, and opens a new issue at
  https://github.com/exploitz/KytyPS5/issues: attach the zip and say what happened and where.
- Please report problems with this fork there, not to chen or to the KytyPS5 project.


## Where files go

Everything stays inside this folder:

- `KytyPS5 Launcher.exe` and the `Qt6*.dll` files next to it, with `platforms\` and `tls\`: the launcher.
- `kyty_emulator.exe`: the emulator. Start it through the launcher; on its own it lacks its launch
  options. `run.cmd` and `precompile.cmd` (with `run-windows.ps1` and `precompile-windows.ps1`) start
  the game and the precompile from the command line, for advanced use.
- `_SaveData`: your saves, created by the first run. Back them up now and then.
- `_PipelineCache`: the shader caches (precompile, translation store, driver cache; about 12 GB after a
  full precompile). They are valid only for this PC's graphics card and driver: do not copy them to
  another PC. Deleting this folder does not touch your saves; the caches are made again, and the
  precompile has to be run again.
- `logs`: the run logs.
- `launcher-settings.json`: the launcher's settings.
- `seeds-<title>_<version>.seeds` and `hinted-<title>_<version>.seeds`: the shader lists made from your
  game files for the precompile. They contain the game's shader code: do not share them.
- `launch.json`: the emulator's switches. Normally left alone.
- `patches`: the 1080p HUD patches. `deps`: the FidelityFX and FSR 4 files.
- `_TempData`, `_DownloadData`, `_DevLog`, `_Textures`: the emulator's working folders, usually empty.

While the game runs, the emulator also keeps working files in the Windows temp folder (a few GB).
Windows deletes them when the emulator closes.

Updating to a newer release: unzip it into a new folder, then move `_SaveData` and `_PipelineCache`
(and `launcher-settings.json`) over from the old one. A precompile of the new release then compiles only
what changed.


## Uninstalling

Delete this folder. Copy `_SaveData` somewhere else first if you want to keep your saves.

The emulator installs nothing. Apart from a few small files whose names start with "kyty" in the Windows
temp folder (type `%TEMP%` in the Explorer address bar), which you can delete, it leaves nothing behind.


## Credits and license

- chenxiao07 (https://github.com/chenxiao07/KytyPS5): the Demon's Souls branch this fork builds on, and
  nearly all of the emulator's Demon's Souls performance work (table path and GPU-driven draws, native
  XPR, the shader precompile, the render presets, the DualSense support, the overlay) and many fixes.
- The KytyPS5 developers (https://github.com/KytyPS5/KytyPS5): the emulator.
- InoriRus (https://github.com/InoriRus/Kyty): the original Kyty, which KytyPS5 is based on.
- AMD FidelityFX SDK (MIT): FSR 3.1 frame generation.
- FSR-Vulkan (MIT): the Vulkan provider for the experimental FSR 4 option.
- Qt (LGPL-3.0): the launcher's toolkit.

License: GNU General Public License version 2 (`LICENSE`). The original Kyty MIT notice and every
third-party component's license are in the `LICENSES` folder. The source code is at
https://github.com/exploitz/KytyPS5.

This package includes no game files. Use only a dump of your own copy of the game. This project is not
affiliated with or endorsed by Sony Interactive Entertainment, Bluepoint Games, NVIDIA or AMD.
