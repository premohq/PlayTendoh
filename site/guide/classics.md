# The classics

The older Nintendo systems play on libretro cores built into Playtendoh, each on a side of its own with
the same Library, settings and in-game menu as the others.

## What you need {#what-you-need}

- **Your games**, dumped from your own cartridges and disks.
- A **BIOS** only for the Famicom Disk System and the Satellaview ([below](#bios)).

## Every system's folder {#games}

| System | Games folder | Formats | Core |
|---|---|---|---|
| Nintendo 64 | `/data/PS5N64/games` | `.z64`, `.n64`, `.v64` | ParaLLEl-N64 |
| iQue Player | `/data/PS5iQue/games` | decrypted iQue games | ParaLLEl-N64 |
| NES | `/data/PS5NES/games` | `.nes`, `.unf` | Mesen |
| Famicom Disk System | `/data/PS5FDS/games` | `.fds` | Mesen |
| VS. System | `/data/PS5VS/games` | `.nes` | Mesen |
| Super Nintendo | `/data/PS5SNES/games` | `.sfc`, `.smc` | Mesen-S |
| Satellaview | `/data/PS5Satellaview/games` | `.bs` | Mesen-S |
| Game Boy | `/data/PS5GBC/games` | `.gb` | mGBA |
| Game Boy Color | `/data/PS5GBC/games` | `.gbc` | mGBA |
| Super Game Boy | `/data/PS5SGB/games` | `.gb` | mGBA |
| Game Boy Advance | `/data/PS5GBA/games` | `.gba` | mGBA |
| Virtual Boy | `/data/PS5VB/games` | `.vb`, `.vboy` | Beetle VB |
| Pokémon mini | `/data/PS5PokeMini/games` | `.min` | PokeMini |
| Game & Watch | `/data/PS5GW/games` | `.mgw` | gw-libretro |

Each side's **Settings > Game files** says its folder, and can choose another.

> [!NOTE]
> The Game Boy and Game Boy Color sides don't read games inside `.zip` or `.7z` archives yet (the Game Boy
> Advance and the NES do). Unzip those games for now.

## BIOS files {#bios}

Each goes in the **`system`** folder beside that system's `games` folder, dumped from your own console:

| System | File | Needed? |
|---|---|---|
| Famicom Disk System | `disksys.rom` (8 KB) in `/data/PS5FDS/system` | **Yes** |
| Satellaview | `BS-X.bin` in `/data/PS5Satellaview/system` | **Yes** |
| Game Boy | `gb_bios.bin` in `/data/PS5GBC/system` | Optional |
| Game Boy Color | `gbc_bios.bin` in `/data/PS5GBC/system` | Optional |
| Super Game Boy | `sgb_bios.bin` in `/data/PS5SGB/system` | Optional |
| Game Boy Advance | `gba_bios.bin` in `/data/PS5GBA/system` | Optional |
| Pokémon mini | `bios.min` in `/data/PS5PokeMini/system` | Optional |

The optional ones have a built-in stand-in that most games are fine with. The Setup check has a row for
each.

## NES HD packs {#hd-packs}

Mesen's HD texture packs work: unzip a pack into `/data/PS5NES/system/HdPacks/<the game file's name>/`
(its `hires.txt` directly in there), or put the pack's `.zip` in `HdPacks` as it is.
