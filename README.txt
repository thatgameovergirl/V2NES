V2NES v1.8.6
Dual-Screen NES Emulator for Nintendo 3DS

Play NES games on the top screen while viewing game maps and
walkthrough guides on the bottom screen.


DISCLAIMER
----------
V2NES does not support piracy. You must dump your own legally
purchased NES cartridges to use this emulator. No copyrighted ROMs,
maps, or game assets are included.


INSTALLATION
------------
1. Copy V2NES.3dsx and V2NES.smdh to sdmc:/3ds/ on your SD card.
2. Launch via the Homebrew Launcher.


SD CARD SETUP
-------------
Create these folders on your 3DS SD card:

  sdmc:/3ds/V2NES/maps/            <-- .v2m map files
  sdmc:/3ds/V2NES/walkthroughs/    <-- .txt guide files
  sdmc:/3ds/V2NES/roms/            <-- .nes ROM files

You can also use: sdmc:/V2NES/maps/ etc.


MAPS
----
.v2m map files are not included. Use the companion app
(v2m_compiler.exe) to convert PNG, JPG, or BMP images to .v2m
format. You can download game maps from sites like NESMaps.com:

  https://nesmaps.com/maps/Zelda/Zelda.html


CONTROLS
--------
  Y / X            Zoom map in / out
  D-pad            Pan map / scroll guide
  L + Y / L + X    Load / save state (3 slots)
  Touch            Navigate menus
  B                Go back


WHAT'S NEW IN v1.8.6
--------------------
- Map zoom/pan position now persists per-map
- Fixed dungeon map UV clipping (no more missing bottom tiles)
- VRAM-aware map caching for large overworld maps
- Auto-map switching via real NES RAM values (Zelda dungeons, SMB)
- Player-position marker on overworld maps
- Companion app: map calibration dialog for player markers
- Walkthrough scroll positions renamed to .wpos (old .pos files
  can be deleted)
- Better folder organization support


WHAT'S NEXT (ETA late June)
----------------------------
- Improved companion app (more user-friendly, better navigation)
- Auto-scroll: bottom map follows the player-position marker
- Fix slow first-time loading
- Fix audio stuttering and hiccups
- Theme system: make the app more attractive


KNOWN ISSUES
------------
- Zelda second quest auto-map switching not yet working
- Map switching between overworld and large dungeons is slow
  (VRAM eviction/reload can take 1-2 seconds on Old 3DS)


THANKS
------
Author: HristValkyrja (thatgameovergirl@gmail.com)
V2NES UI and tooling developed with AI assistance.

NES core based on VirtuaNES by Norix (3DS port by bubble2k16).
Mappers from FCEUX, CaH4e3, dragon2snow, VirtualNESEx/Plus,
fanoble, Xodnizel.
Audio: emu2413 by Mitsutaka Okazaki.
Libraries: LodePNG, libctru, citro2d/citro3d (devkitPro).

Licensed under the GNU General Public License v2.0 or later.
