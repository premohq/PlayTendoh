# Where your files go

Each system keeps its files in a folder of its own in `/data`, and Playtendoh's launcher keeps its own
in `/data/ps5cemu`. This page is the whole map. The app makes the folders itself the first time it
starts, so you only copy files in.

## Every system at a glance {#folders}

| System | Your games | Keys, firmware, BIOS | Updates and DLC |
|---|---|---|---|
| [Wii U](wii-u.md) | `/data/cemu/games` | `/data/cemu/keys.txt`: only for encrypted `.wud` and `.wux` | unpacked in `/data/cemu/updates` and `/data/cemu/dlc`, then installed from the app |
| [3DS](3ds.md) | `/data/azahar/games` | none (games must be decrypted) | `.cia` files, installed from the app |
| [DS](ds.md) | `/data/melonds/games` | optional: `/data/melonds/bios` | none |
| [Switch](switch.md) | `/data/eden/roms` | `/data/eden/keys` (`prod.keys`, `title.keys`) and `/data/eden/firmware` | beside the game, or in `/data/eden/updates` |
| [Switch 2](switch-2.md) | `/data/eglantineps5/roms`, a folder each | `/data/eglantineps5/keys` and `/data/eglantineps5/firmware` | with the game's folder |
| [GameCube](gamecube-wii.md) | `/data/porpoise/games/GameCube` | none | none |
| [Wii](gamecube-wii.md) | `/data/porpoise/games/Wii` | none | none |
| [The classics](classics.md) | `/data/PS5NES/games`, `/data/PS5SNES/games`... ([the list](classics.md#games)) | a BIOS in the system's `system` folder, for two of them | none |

**Any other folder works too**, on the PS5, a USB drive or the extended storage: on each console's side,
**Settings > Game files** chooses it, and subfolders are searched. Wii U, 3DS, DS and classic games can
also come from a computer or NAS on your network (**Settings > Game files > Network shares**); Switch
games can't.

> [!TIP]
> After you copy games in, a console's Library finds them the next time you open it, or at once with
> **Settings > Game files > Look for games now**.

## Everything else {#everything-else}

| What | Where |
|---|---|
| Amiibo dumps (`.bin`) | `/data/ps5cemu/amiibo` |
| Your own launcher music (`.mp3`, `.wav`) | `/data/ps5cemu/music` |
| Logs (`boot.log` and the emulators') | `/data/ps5cemu/logs`; copy them with **Settings > Diagnostics > Copy logs to USB** |
| Wii U saves | `/data/cemu/mlc01/usr/save` |
| 3DS saves | `/data/azahar/sdmc` |
| DS saves | `/data/melonds/saves` |
| Switch saves | `/data/eden/user/nand/user/save` |
| Wii U graphic packs | `/data/cemu/graphicPacks` (the community packs download by themselves) |
| 3DS cheats, mods and textures | `/data/azahar/cheats`, `/data/azahar/load` |
| Switch mods and cheats | `/data/eden/mods` |

**Back up your saves** with **Settings > Accounts and saves > Back up to USB**: every console's saves and
your settings, never the games.

## Getting files onto the PS5 {#copying}

- **FTP:** an FTP client on your computer (FileZilla, WinSCP...) and your HEN's FTP server.
- **A USB drive:** copy the files onto it on your computer, then into place with a file manager on
  the PS5.
- **Your phone or computer, through Playtendoh,** once it runs: below.

### From a phone or computer {#from-a-phone}

Turn on **Settings > Controls > Remote touch screen** (on the Wii U side, **Settings > System > Remote
touch screen**), open the address it shows (`http://<your PS5's address>:8080/`) in a browser on the same
network, and choose **Add files to the PS5**. Pick where they go (a console's games, Switch keys or
firmware, amiibo, music), choose the files and **Send**. No FTP and no USB drive needed.

## The app on a USB drive or the extended storage {#app-on-a-drive}

When Playtendoh itself is installed on a drive ([Install](install.md#on-a-drive)), its files are kept
there instead: a **`PS5CEMU-HAR-data`** folder at the top of the drive in place of `/data/ps5cemu`, and
`cemu`, `azahar`, `melonds` and `eden` folders beside it in place of `/data`'s. The one exception: Switch
keys and saves already in `/data/eden` stay there, so nothing is left behind.

The Setup check's **Storage** row names the folder in use.

## An install from before {#older-install}

An install from a version before the folders were split keeps everything in `/data/ps5cemu`, and keeps
working there: Cemu's files at its top, the 3DS's, DS's and Switch's in its `azahar`, `melonds` and
`eden` folders. At a start the app offers to give each system its own folder (**Reorganize** or
**Later**). It moves the files in a moment, copies nothing and deletes nothing, and the same row in
**Settings > Game files** puts them back.

> [!NOTE]
> Nothing here comes with the app: no games, keys, firmware or BIOS. Dump them from consoles and games
> you own.
