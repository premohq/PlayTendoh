# GameCube and Wii

The GameCube and the Wii play on Dolphin, in [Porpoise X](https://github.com/premohq/Porpoise-X), each a
side of its own with Playtendoh's in-game menu, loading screen and settings. Phononenom brought them in.

## Turn them on {#turn-on}

> [!IMPORTANT]
> The GameCube and Wii sides start **hidden**. Show them from **Settings > Launcher > Systems**: turn on
> **GameCube** and **Wii**, and they're in the bar.

## What you need {#what-you-need}

- **Your games**, dumped from your own discs (with CleanRip on a Wii, for example).
- Nothing else: no BIOS, no keys.

## Your games {#games}

| Console | Folder | Formats |
|---|---|---|
| GameCube | **`/data/porpoise/games/GameCube`** | `.iso`, `.gcm`, `.rvz`, `.ciso`, `.gcz`, `.wia` |
| Wii | **`/data/porpoise/games/Wii`** | the same, and `.wbfs`, `.wad`, `.dol`, `.elf` |

Or choose any folder on each side's **Settings > Game files**. Each side reads a disc's header and lists
only its own console's games, so a Wii disc never shows on the GameCube side. `.rvz` keeps a disc small and
plays the same. A Wii game shows its banner and title screen in the launcher, and GameTDB's title and
description.

> [!TIP]
> **Wii games bought on the Wii U's eShop** don't play on the Wii U side (Cemu doesn't run Wii games).
> Dumped as Wii games, they play here.

## Settings {#settings}

| Page | What's there |
|---|---|
| **Graphics** | Dolphin's picture settings for every GameCube and Wii game, widescreen among them |
| **Controls** | GameCube: the DualSense as the GameCube controller. Wii: as the Wii Remote, or as the GameCube controller. Any player in any port |
| **Sound** | Game sound, and how it handles slowdowns |
| **GBA** (GameCube) | A Game Boy Advance on a GameCube port: set one in Controls, under Ports |
| **System** | The emulated console, and how it runs |
| **Game files** | The games folder, Look for games now |
| **Memory cards and saves** (GameCube), **Saves** (Wii) | Copy a game's save to a USB drive, or move it aside |
| **Online** | Box art |

### A game's own pages {#per-game}

**Options** on a game, then **Game settings**, has the pages above for that game, and three more:

- **Setups:** **Save as a setup** keeps this game's Graphics settings to use on other games, and **Use
  setup** applies a saved one to this game.
- **Cheats and patches:** each on or off.
- **Auto Boost:** the settings that run this game best on the PS5.

Memory cards and saves are kept **per game**, so one game's card never fills up another's.

## Controllers {#controllers}

The DualSense plays both. Real **Wii Remotes** connect through a Mayflash **DolphinBar** in a USB port. A
GameCube game that links to a **Game Boy Advance** can have one on the link cable, with its own page.

**Touchpad click + Options** opens the in-game menu, with its **Buttons** page and live controller test,
as on every system.
