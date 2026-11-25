# Nintendo 3DS

The 3DS side plays on [Azahar](https://github.com/azahar-emu/azahar) with dynarmic's recompiler, as a New
3DS, at up to 10x the 3DS's resolution.

## What you need {#what-you-need}

- **Your games, decrypted.** Azahar plays no encrypted game, whatever keys it has: decrypt your dumps on
  your own 3DS first (GodMode9 does it).
- Nothing else for most games. The 3DS's own [system files](#system-files) are optional, for the Home Menu
  and a few games.

## Your games {#games}

Put them in **`/data/azahar/games`**, or choose any folder in the 3DS side's **Settings > Game files**
(its first row), on the PS5, a drive or a [network share](files.md#network-shares).

| Format | How it plays |
|---|---|
| `.3ds`, `.cci`, `.cxi`, `.app` | Straight from the folder |
| `.3dsx`, `.elf` | Homebrew, straight from the folder |
| `.z3ds`, `.zcci`, `.zcxi`, `.z3dsx` | Azahar's compressed formats, straight from the folder |
| `.zip`, `.7z`, `.rar`, `.tar` | One game an archive, unpacked the first time it starts |
| `.cia`, `.zcia` | **Installed first**, from **Settings > Game files > Install a CIA file** |

## Updates and DLC {#updates}

Updates and DLC come as `.cia` files. Put them anywhere the PS5 can read (`/data/azahar/updates` and
`/data/azahar/dlc` are there for them), and install each from **Settings > Game files > Install a CIA
file**. Azahar only installs fully decrypted CIAs. An install can be cancelled.

## The 3DS's own files {#system-files}

The 3DS Home Menu and a few games need your 3DS's system files. Copy them from your own console over the
network, with **Artic Setup**:

1. On your 3DS, start the **Artic Setup Tool**.
2. In Playtendoh, on the 3DS side, open **Settings > Online > Artic Base** (the Setup check's **3DS system
   files** row takes you there too) and run the setup, entering the address the 3DS shows.
3. Let it finish. The row then says the Home Menu can start.

It needs no keys from you: Azahar's own are built in. An `aes_keys.txt` in `/data/azahar/sysdata` takes the
place of all of them, so leave it out unless it's complete.

**Play from your 3DS:** the same **Artic Base** runs a game straight from your 3DS over the network: start
Artic Base on the 3DS and enter the address it shows.

## Settings {#settings}

| Page | What's there |
|---|---|
| **Graphics** | Resolution (1x to 10x; 6x by default), texture filters, frame generation, **3D for red and cyan glasses**, custom textures |
| **Screens** | Layouts (large top, top only, side by side, stacked), borders, rotation for book-style games, the gap between the screens, touch without the bottom screen |
| **Controls** | The DualSense as the 3DS, every button remapped, and the **remote touch screen** |
| **System** | Region, language, CPU clock and sound |
| **Game files** | The games folder, Look for games now, network shares, Install a CIA file |
| **Online** | Box art and **Artic Base** |

## Controls {#controls}

The circle pad and C-stick are the sticks, ZL and ZR are L2 and R2, motion comes from the DualSense's gyro,
and the **touchpad is the bottom screen**: move to point, click to tap, hold the click to drag.

| In a 3DS game | Does |
|---|---|
| Touchpad click + **Options** | Opens the in-game menu (the game pauses) |
| Touchpad click + **L1** | Swaps the screens |
| Touchpad click + **R1** | The next screen layout |
| Touchpad click + **Square** | Saves a state, to the slot chosen in the menu |
| Touchpad click + **Triangle**, held | Loads that state |

**Settings > Controls > Buttons** can also give any button **Fast forward** (held, or on and off) or
**Touch at the cursor**.

The in-game menu has Resume, Save state, Load state, Screens and Screen gap on top, then the border, CPU
clock, speed limit, and **Save states, cheats, amiibo**.

## Cheats, mods and textures {#cheats}

- **Cheats:** `/data/azahar/cheats/<title ID>.txt`, in the Gateway format desktop Azahar uses. Turn each
  on or off in the in-game menu (which also shows the 16-digit title ID).
- **Custom textures:** turn on **Settings > Graphics > Textures > Custom textures** and put a pack in
  `/data/azahar/load/textures/<title ID>/`.
- **Mods:** `/data/azahar/load/mods/<title ID>/`.
- **Amiibo:** in the in-game menu, **Save states, cheats, amiibo > Amiibo**, then **Take the amiibo away**
  when the game is done.

## The cameras {#camera}

The 3DS's cameras see a picture: put a PNG or JPEG in `/data/azahar/camera`, or send one from a phone
([how](files.md#from-a-phone)). Both cameras see the newest one: a QR code for Pokémon's QR Scanner, a Mii
or an Animal Crossing design to scan. With no picture there, they see black.

## Saves from Azahar or Citra on a computer {#pc-saves}

Copy `sdmc/Nintendo 3DS/<ID0>/<ID1>/title/00040000/<title ID>/data` to the same path under
`/data/azahar/sdmc/Nintendo 3DS/`. ID0 and ID1 are all zeros in Azahar, on a computer and on the PS5.

## Good to know {#good-to-know}

- The 3DS's microphone, local wireless and online play aren't supported.
- Texture filters are costly at high resolutions: if a game stutters, try **Texture filter: None** first.
