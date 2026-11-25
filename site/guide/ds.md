# Nintendo DS

The DS side plays on [melonDS](https://github.com/melonDS-emu/melonDS), with the touchpad as the touch
screen.

## What you need {#what-you-need}

- **Your games** (`.nds`), dumped from your own cards.
- Nothing else. Your DS's own BIOS is [optional](#bios).

## Your games {#games}

Put them in **`/data/melonds/games`**, or choose any folder on the DS side's **Settings > Game files**.
A game can stay in its `.zip`, `.7z` or `.rar`: it's read as it starts, and nothing is unpacked.

**Saves:** a `.sav` beside a game is taken the first time the game starts. From then on, saves are in
`/data/melonds/saves`.

## Your DS's BIOS {#bios}

DS games run on a built-in BIOS and firmware, and most are fine with it. For the few that aren't, put
these files, dumped from your own DS, in **`/data/melonds/bios`**:

- `bios7.bin`
- `bios9.bin`
- `firmware.bin`

The Setup check's **DS BIOS** row says when they're found and used.

## Game Boy Advance games in Slot-2 {#slot-2}

Games that read a GBA cartridge in the DS's second slot find it in `/data/melonds/slot2`, as
`<the DS game's file name>.gba`.

## Cheats {#cheats}

`/data/melonds/cheats/<the game's file name>.mch`
