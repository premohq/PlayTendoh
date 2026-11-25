# Nintendo 3DS

The 3DS side plays on [Azahar](https://github.com/azahar-emu/azahar), at up to 10x the 3DS's resolution.

## What you need {#what-you-need}

- **Your games, decrypted.** Azahar plays no encrypted game, whatever keys it has: decrypt your dumps on
  your own 3DS first (with GodMode9).
- Nothing else for most games. The 3DS's own [system files](#system-files) are optional.

## Your games {#games}

Put them in **`/data/azahar/games`**, or choose any folder on the 3DS side's **Settings > Game files**.

| Format | How it plays |
|---|---|
| `.3ds`, `.cci`, `.cxi`, `.app` | Straight from the folder |
| `.3dsx`, `.elf` | Homebrew, straight from the folder |
| `.z3ds`, `.zcci`, `.zcxi`, `.z3dsx` | Azahar's compressed formats, straight from the folder |
| `.zip`, `.7z`, `.rar` | One game an archive, unpacked the first time it starts |
| `.cia`, `.zcia` | **Installed first**, from **Settings > Game files > Install a CIA file** |

## Updates and DLC {#updates}

Updates and DLC come as `.cia` files. Put them in `/data/azahar/updates` or `/data/azahar/dlc` (or
anywhere), and install each from **Settings > Game files > Install a CIA file**. Azahar only installs
fully decrypted CIAs.

## The 3DS's own files {#system-files}

The 3DS Home Menu and a few games need your 3DS's system files. Copy them from your own console over
the network:

1. On your 3DS, start the **Artic Setup Tool**.
2. In Playtendoh, on the 3DS side, choose **Artic Base** (the Setup check's **3DS system files** row
   has it too) and connect to the address the 3DS shows.
3. Let it finish. The row then says the Home Menu can start.

**Play from your 3DS** (Artic Base) can also run a game straight from the 3DS's own card or storage.

## Saves from Azahar or Citra on a computer {#pc-saves}

Copy `sdmc/Nintendo 3DS/<ID0>/<ID1>/title/00040000/<title ID>/data` to the same path under
`/data/azahar/sdmc/Nintendo 3DS/`. ID0 and ID1 are all zeros in Azahar, on a computer and on the PS5.

## Cheats, mods and textures {#cheats}

- Cheats: `/data/azahar/cheats/<title ID>.txt`
- Custom textures: `/data/azahar/load/textures`
- Mods: `/data/azahar/load/mods`
