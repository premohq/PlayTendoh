# Nintendo DS

The DS side plays on [melonDS](https://github.com/melonDS-emu/melonDS) 1.1, with its recompiler, both
screens at 4K, and the touchpad as the touch screen.

## What you need {#what-you-need}

- **Your games** (`.nds`), dumped from your own cards.
- Nothing else: melonDS's own replacements run most games. Your DS's BIOS is [optional](#bios).

## Your games {#games}

Put them in **`/data/melonds/games`**, or choose any folder in the DS side's **Settings > Game files**
(its first row), on the PS5, a drive or a [network share](files.md#network-shares). A game can stay in its
`.zip`, `.7z`, `.rar` or `.tar`: it's read into memory as it starts, and nothing is unpacked.

**Saves:** a `.sav` beside a game is taken the first time the game starts. From then on, its saves are in
`/data/melonds/saves`.

## Your DS's BIOS {#bios}

Most games are fine with the built-in BIOS and firmware. For the few that aren't, put these files, dumped
from your own DS, in **`/data/melonds/bios`**:

- `bios7.bin`
- `bios9.bin`
- `firmware.bin`

**Settings > System** uses them when they're there, and the Setup check's **DS BIOS** row says when
they're found. Playtendoh includes no BIOS.

## Settings {#settings}

| Page | What's there |
|---|---|
| **Graphics** | A **Sharp**, **Smooth** or **Square pixels** filter |
| **Screens** | Layouts and borders, rotation for book-style games (Hotel Dusk), the gap between the screens, touch without the bottom screen, and **Touch**: calibration and the right stick as the stylus |
| **Controls** | The DualSense as the DS, every button remapped, and the **remote touch screen** |
| **System** | Language, BIOS, the **microphone**, **Slot-2** and sound |
| **Game files** | The games folder, Look for games now, network shares |
| **Online** | Box art |

## Controls {#controls}

A is on Circle and B on Cross by default (or as you chose at the first start), the left stick is a second
D-pad, and the **touchpad is the touch screen**: move to point, click to tap, hold the click to drag.

| In a DS game | Does |
|---|---|
| Touchpad click + **Options** | Opens the in-game menu |
| Touchpad click + **L1** | Swaps the screens |
| Touchpad click + **R1** | The next screen layout |
| Touchpad click + **Square** | Saves a state (five slots) |
| Touchpad click + **Triangle**, held | Loads it |
| **R3**, held | Blows into the microphone |

**Settings > Controls > Buttons** can give any button **Fast forward**, **Rewind** (the last 15 seconds,
while held) or **Touch at the cursor**.

- **The right stick as the stylus** (**Settings > Screens > Touch**): pushed, it touches the bottom screen
  and drags; let go, the touch lifts. Metroid Prime Hunters aims this way.
- **Touch calibration** (same page): moves and scales the touchpad's cursor, for a game where touches land
  off the mark.

## The microphone {#microphone}

What the DS hears while **R3** is held, in **Settings > System**: **Noise**, **Silence**, or **A
recording**: a WAV file (16-bit mono, up to 16 MB) in `/data/melonds/microphone/<the game's file
name>.wav`, or `/data/melonds/microphone.wav` for every game.

## Game Boy Advance games in Slot-2 {#slot-2}

For the DS games that read a GBA cartridge in their second slot (Pokémon Diamond and Pearl's Pal Park):
turn on **Settings > System > Slot-2**, and put the GBA game in `/data/melonds/slot2`, named as the DS
game's file but ending `.gba`, with its `.sav` beside it if it has one. It doesn't play GBA games: those
are on the [Game Boy Advance side](classics.md).

## Cheats {#cheats}

melonDS's `.mch` cheats: `/data/melonds/cheats/<the game's file name>.mch`, turned on or off in the in-game
menu.

## Good to know {#good-to-know}

DSi games and the DS's wireless aren't supported.
