# Wii U

The Wii U side plays on [Cemu](https://github.com/cemu-project/Cemu), at up to 4K and 120 Hz.

## What you need {#what-you-need}

- **Your games**, dumped from your own Wii U and discs (with a dumping tool on the Wii U, such as
  Dumpling).
- **Disc keys**, only if your dumps are encrypted `.wud` or `.wux` files ([below](#disc-keys)).
- Nothing else: no firmware, no BIOS.

## Your games {#games}

Put them in **`/data/cemu/games`**, or choose any folder on the Wii U side's **Settings > Game files**.

| Format | What it is |
|---|---|
| `.wua` | A Wii U archive: the game, its update and its DLC in one file. The easiest to keep. |
| `.wud`, `.wux` | A disc image. Encrypted ones need their [disc key](#disc-keys). |
| An unpacked game | A folder holding `code`, `content` and `meta`. |
| `.zip`, `.7z`, `.rar` | An unpacked game in an archive, one game an archive. It is unpacked the first time it starts. |

```text
/data/cemu/games/
├── Game.wua
├── Another Game.wux
└── A Third Game/
    ├── code/
    ├── content/
    └── meta/
```

> [!TIP]
> **An unpacked game is found but not listed?** Check its three folders are named `code`, `content` and
> `meta`, in small letters. A `.wua` of the same game always works while that's being sorted out.

## Disc keys {#disc-keys}

Encrypted `.wud` and `.wux` dumps need their disc key, in **`/data/cemu/keys.txt`**, one per line: 32
letters and digits, then the game's name if you like.

```text
0123456789abcdef0123456789abcdef  # Game name
```

`.wua` files and unpacked games need no keys. The Setup check's **Wii U disc keys** row says how many
keys the file holds.

> [!WARNING]
> **Keep a copy of your `keys.txt` on your computer.** Version 5.1.0 can empty it when the app starts.
> If the Setup check says "keys.txt has no keys" after you filled it, copy yours back; a fix is on its way.

## Updates and DLC {#updates}

- **A `.wua`** already holds them: nothing to do.
- **Unpacked** updates and DLC go in `/data/cemu/updates` and `/data/cemu/dlc`. Then install them from
  **Settings > Game files > Updates, DLC and mods**, which puts them into the Wii U's own storage.
- They can't be installed from inside an archive: unpack them first.

> [!NOTE]
> **Breath of the Wild asks for a system update?** It needs its own update installed (v1.6.0 or later),
> as above. The same goes for any game that asks for one.

## Graphic packs {#graphic-packs}

The Cemu community graphic packs (resolution, frame rate, fixes) download by themselves the first time
the Wii U side starts online. **Settings > Online > Community graphic packs** fetches a newer set. A
game's page lists the packs for it.

## Wii games and the Wii U's Virtual Console {#wii}

Cemu doesn't play Wii games, and that includes the Wii titles sold on the Wii U's eShop. Play them on the
[GameCube and Wii](gamecube-wii.md) side.

## Saves from Cemu on a computer {#pc-saves}

With the game closed, copy `mlc01/usr/save/00050000/<title ID>/user/<account>` from your computer to
`/data/cemu/mlc01/usr/save/00050000/<title ID>/user/80000001`.

## Mods {#mods}

Graphic packs and mods have their own section in the [full user guide](https://github.com/premohq/PlayTendoh/blob/main/docs/GUIDE.md#mods).
