# Wii U

The Wii U side plays on [Cemu](https://github.com/cemu-project/Cemu), with its x64 recompiler, at the
PS5's 4K output and 120 Hz on a TV that has it.

## What you need {#what-you-need}

- **Your games**, dumped from your own Wii U and discs (with a dumping tool on the Wii U, such as
  Dumpling).
- **Disc keys**, only if your dumps are encrypted `.wud` or `.wux` files ([below](#disc-keys)).
- Nothing else: no firmware, no BIOS. Updates and DLC are optional.

## Your games {#games}

Put them in **`/data/cemu/games`**, or choose any folder in the Wii U side's **Settings > Game files**
(its first row), on the PS5, a USB drive, the extended storage or a [network share](files.md#network-shares).
Subfolders are searched.

| Format | What it is |
|---|---|
| `.wua` | A Wii U archive: the game, its update and its DLC in one file. The easiest to keep. |
| `.wud`, `.wux` | A disc image. Encrypted ones need their [disc key](#disc-keys). |
| An unpacked game | A folder holding `code`, `content` and `meta`. |
| `.zip`, `.7z`, `.rar`, `.tar` | An unpacked game in an archive, one game an archive. |

```text
/data/cemu/games/
├── Game.wua
├── Another Game.wux
└── A Third Game/
    ├── code/
    ├── content/
    └── meta/
```

**Games in archives** are unpacked the first time they start, with the progress on screen (Circle
cancels), into `/data/ps5cemu/cache/archives`, and kept for the next start. Unpacking needs the game's
size plus 256 MB free. **Settings > Diagnostics > Unpacked games** shows how much they take, and holding
Cross deletes them. In a `.7z` or `.rar`, give the game's folder a name without accents or symbols such
as ™.

> [!TIP]
> **An unpacked game is found but not listed?** Check it has all three folders, `code`, `content` and
> `meta` (in small letters or capitals), with the game's files in them. A `.wua` of the same game always
> works. Still missing: send `cemu.log`, which says why a game was left out.

## Disc keys {#disc-keys}

Encrypted `.wud` and `.wux` dumps need their disc key, in **`/data/cemu/keys.txt`**, one per line: 32
letters and digits, then the game's name if you like.

```text
0123456789abcdef0123456789abcdef  # Game name
```

`.wua` files and unpacked games need no keys. The Setup check's **Wii U disc keys** row says how many keys
the file holds.

> [!WARNING]
> **Keep a copy of your `keys.txt` on your computer.** Version 5.1.0 could empty it as the app started. If
> the Setup check says "keys.txt has no keys" after you filled it, copy yours back.

## Updates, DLC and mods {#updates}

- **A `.wua`** already holds them: nothing to do.
- **Unpacked** updates and DLC: put them in `/data/cemu/updates` and `/data/cemu/dlc` (or on a USB drive),
  then open **Settings > Game files > Updates, DLC and mods > Updates and DLC**. It looks on every drive,
  the games folder and `/data`, lists what it finds by game with what's installed now, and installs one
  with **Cross**, or every new one with **Square**; **Triangle** picks a folder by hand.
- A game's Options menu has its own updates and DLC, and **Remove its update or DLC** takes an installed
  one out again (one that stops the game starting, say).
- They can't be installed from inside an archive: unpack them first.

> [!NOTE]
> **Breath of the Wild asks for a system update?** It needs its own update installed (v1.6.0 or later), as
> above. The same goes for any game that asks for one.

### Mods {#mods}

A Cemu mod is a folder with a `rules.txt`, or a folder with `content` and `aoc` folders. To add them:

1. On a computer, unzip each mod into its own folder inside **`ps5cemu/mods`** on a USB drive.
2. Plug the drive into the PS5 and choose **Settings > Game files > Updates, DLC and mods > Mods from
   USB**.
3. Each mod then appears, off, on its game's **Graphic packs** page: turn it on there.

Most mods made for Cemu on a PC work as they are; mods for the Wii U itself (SDCafiine, Aroma plugins)
don't. The [full user guide](https://github.com/premohq/PlayTendoh/blob/Playtendoh/docs/GUIDE.md#mods)
has more.

## Settings {#settings}

| Page | What's there |
|---|---|
| **Graphics** | **Upscaling to 4K** (Bicubic by default, or **FSR 1** for sharper), **Frame rate** (an even 60 at 120 Hz or 30 at 60 Hz, and **frame generation**, which shows a 30 fps game at 60), ReShade, the performance overlay, shaders |
| **Controls** | Each of up to four players' controller, vibration, a turbo button, and every button remapped |
| **System** | The sound, **USB devices** (Skylanders, Disney Infinity and LEGO Dimensions portals) and the **remote touch screen** |
| **Game files** | The games folder, Look for games now, network shares, disc keys, updates, DLC and mods |
| **Wii U accounts** | Up to twelve accounts, each with its own saves, or **One account for each PS5 user** |
| **Online** | Box art, and the **Community graphic packs** |

## Controllers {#controllers}

- **Player 1's DualSense is the GamePad**; other signed-in players are Pro Controllers, up to four.
  Each player can be a GamePad, Pro Controller, Classic Controller, or Wii Remote with or without a
  Nunchuk, with motion, rumble and button mapping.
- **Real Wii Remotes** connect through a Mayflash **DolphinBar** in mode 4.
- **The touchpad is the GamePad's touch screen:** move to point, click to tap, hold the click to drag.

| In a Wii U game | Does |
|---|---|
| Touchpad click + **Options** | Opens the in-game menu |
| Touchpad click + **L1** | Switches the main screen between the TV and the GamePad |
| Touchpad click + **R1** | Shows or hides the other screen in a corner |

The in-game menu's quick actions are **Resume**, **Screens**, **Graphic packs** and **Amiibo**; its
Graphics page has Cemu's Accurate barriers and Async shader compile.

## Graphic packs {#graphic-packs}

The Cemu community graphic packs (resolution, frame rate, fixes) download by themselves the first time
the Wii U side starts online (about 3 MB). **Settings > Online > Community graphic packs** fetches newer
ones. A game's **Graphic packs** page, in its Options menu or the in-game menu, turns each on, with a
choice for each preset.

## Wii games and the Wii U's Virtual Console {#wii}

Cemu doesn't play Wii games, and that includes the Wii titles sold on the Wii U's eShop. Play them on the
[GameCube and Wii](gamecube-wii.md) side.

## Saves from Cemu on a computer {#pc-saves}

With the game closed, copy `mlc01/usr/save/00050000/<title ID>/user/<account>` from your computer to
`/data/cemu/mlc01/usr/save/00050000/<title ID>/user/80000001`.

## Good to know {#good-to-know}

- **Going back to the Library from a Wii U game restarts the app.** That's normal: Cemu keeps the screen.
- **A game's first start can take a few minutes** while Cemu builds its shaders, with the loading screen
  counting them. The next starts are quick.
- **Batman: Arkham Origins** needs its 60 FPS graphic pack off
  ([#15](https://github.com/premohq/PlayTendoh/issues/15)).
