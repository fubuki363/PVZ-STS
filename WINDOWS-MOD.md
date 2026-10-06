# Windows HD orb sandbox

Run `启动游戏.cmd` for adventure mode, `启动游戏-调试关卡.cmd` for the sandbox, or `启动游戏-4K.cmd` for 3840 × 2160 fullscreen. The main menu also has a debug-level button. Press F8 during a sandbox level to edit its configuration.

Adventure levels 1–3 start with Peashooter (100 Sun), Pillar Construct (50 Sun, replacing Sunflower), Lightning Orb (25 Sun, two-second recharge), Stoke Chili (125 Sun, replacing Jalapeno), and Glass Orb (25 Sun, two-second recharge). Later adventure levels replace the third, fourth, and fifth chosen cards with Lightning Orb, Stoke Chili, and Glass Orb, except conveyor levels. Click a Peashooter with the Lightning Orb card to channel into its two-slot queue. Planting a Peashooter starts with one Lightning orb; planting a Pillar starts with one Plasma orb. Full queues evoke their oldest orb and append the new orb. Lightning and Plasma evokes emit 20 ordinary-damage pea projectiles at six-frame intervals. Those projectiles deal the original 20 damage; Glass evokes use the 10-damage penetrating projectiles described below. Plasma produces 25 Sun per orb at the original Sunflower production cadence (2350–2500 frames; the initial production timer also retains the original shorter delay).

The sandbox provides all 55 card types on three pages, 1–10 card slots, 33 zombie types, 10 backgrounds, row and batch selection, spawn/clear commands, and channel/evoke tools. The tools use Lightning for Peashooters and Plasma for Pillars. Sandbox progress uses a temporary player profile, separate from adventure saves.

Textures come from the user's local Slay the Spire 2 PCK. `dist/sts2-assets-report.json` records the source entries and verified hashes. The Pillar uses the original Cubex Construct atlas, weighted setup geometry, moss1 skin, and circle eye. Peashooter uses the same construct with moss2 and square eye. Both have independent upper and lower body sections floating up and down in opposite phases over a roughly two-second cycle, with a gently floating energy core. Cards and planting previews use the corresponding construct sprites. Peas emerge from the upper section, with at least 40 pixels of ground clearance to prevent immediate ground impact. Lightning and Plasma use the original orb textures.

Re-extract the textures from the workspace root:

```powershell
python scripts/extract-sts2-assets.py 'F:\SteamLibrary\steamapps\common\Slay the Spire 2\SlayTheSpire2.pck' --index .tools/sts2-index.json --export
```

The extraction utilities require Python, Pillow, and NumPy. They read the installed game files without modifying them.

Prepare a Windows checkout with Visual Studio 2022's Desktop development with C++, CMake, Git, and Python. Install the Python dependencies:

```powershell
python -m pip install Pillow numpy pefile
```

Initialize vcpkg and install its static dependencies (the build script uses classic mode and disables automatic manifest installation):

```powershell
git clone https://github.com/microsoft/vcpkg .tools/vcpkg
& ./.tools/vcpkg/bootstrap-vcpkg.bat -disableMetrics
& ./.tools/vcpkg/vcpkg.exe install libjpeg-turbo libogg libopenmpt libpng libvorbis mpg123 sdl2 zlib --triplet x64-windows-static --classic
```

Import your local PvZ GOTY ZIP or resource directory; only the resource files are copied:

```powershell
python scripts/prepare-windows-resources.py 'C:/Games/Your-PvZ-GOTY.zip' --output dist
```

Extract the construct and orb textures from your local Slay the Spire 2 installation using the extraction command above. Replace its sample installation path with yours. Resource files and extracted textures are not stored in this Git repository.

Build and validate from an ASCII checkout path:

```powershell
.\build-windows.ps1 -DisplayTests -GameplayTests
ctest --test-dir build-windows -C Release --output-on-failure
```

For a checkout path containing non-ASCII characters, map an unused drive letter before configuring the build. For example, if `P:` is free, run `subst P: "$PWD"` and pass `-VcpkgRoot 'P:\.tools\vcpkg'` to the build script. The mapping is a local path workaround, not a required project directory.

The packaged executable supports `-sandbox-check` for a hidden runtime check, using an isolated `-savedir`. It exercises all backgrounds/cards/zombies, orb production, 20-shot bursts, targeted charging, and save/load of mixed orb queues and partial bursts. It also verifies all construct layers load and captures 24 native-rendered frames over one animation cycle. Results are written to `sandbox-verification.json`; it exits after verification.

The current Windows package is `PvZ-Windows-HD-Glass.zip`. It includes the animated layers, matching card textures, source changes, and the verified executable.

Stoke Chili costs 125 Sun and retains the original 50-second Jalapeno recharge. Click any cell in a row, including a full row, to transform each compatible plant independently into a random plant from the current seedbank. This uses each slot as one random choice, regardless of card cooldown or the selected plant's cost, and excludes Stoke and the Lightning/Glass Orb skills. It checks terrain and layer compatibility, preserves support layers when no compatible replacement exists, resolves Imitater choices, and initializes each new plant's health, timers, and orbs. No compatible targets or insufficient Sun cause no charge or cooldown. Directly planted or previously saved Jalapenos also transform instead of burning zombies.

Native regression checks now advance actual projectiles and verify normal single-orb and double-orb shots deal 20 and 40 damage, and a 20-shot evoke deals 400 damage. Stoke checks cover row targeting, costs, cooldowns, support layers, failure cases, and save/load. Per-case diagnostics are written to shooting-verification.csv and stoke-verification.csv.

Glass Orb channels into Peashooter like Lightning Orb. Each glass orb passively emits one glass projectile per normal firing cycle, independently of lightning orbs in the other slot. Each glass projectile deals 10 damage, hits up to three distinct eligible zombies, and disappears after the third. Armor and shields use the original pea rules. Repeated overlap with the same zombie cannot deal damage again, and Torchwood leaves glass projectiles unchanged. Evoking a glass orb emits 20 glass projectiles at the same six-frame spacing. Overlapping releases retain a FIFO queue of typed 20-shot batches. Both hit history and pending bursts survive v4 save/load.

The fifth card uses the real 100x100 Glass Orb image extracted from the local Slay the Spire 2 PCK. Glass card ID 75 keeps all existing plant and minigame IDs unchanged; the debug list maps its 55th selectable entry to this additional skill. Glass native checks are recorded in glass-verification.csv.
