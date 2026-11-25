# Switch 2 (experimental)

The Switch 2 side plays on [EglantinePS5](https://github.com/heydemoura/EglantinePS5), a PS5 port of
[Eglantine](https://github.com/Djihads80/Eglantine), with [Lichii](https://github.com/zortos293/Lichii)'s
core. It's new and **experimental**: some games play, many don't yet, and it's being worked on all the
time, by [Phononenom](https://github.com/Phononenom).

It's in the bar from the first start, beside the Switch. Like the Switch side, it uses Playtendoh's own
loading screen and in-game menu.

## What you need {#what-you-need}

- **Your games**, dumped from your own console and cartridges: as game folders, or as `.nup` files.
- **Keys** (`prod.keys`) and **firmware**, from your own console.

## Your games {#games}

Put them in **`/data/eglantineps5/roms`**, or choose any folder in the side's **Settings > Game files**.
Each game is either **a `.nup` file**, or **a folder of its own** holding its ExeFS and RomFS:

```text
/data/eglantineps5/
├── roms/
│   ├── Game.nup
│   └── Another Game/
│       ├── exefs/      main, main.npdm, rtld, sdk and every subsdk*
│       └── romfs/      the whole RomFS
├── updates/            the games' updates
└── dlc/                their DLC
```

A game folder's `main.npdm` gives its title ID. A `cover.png` or `cover.jpg` in the game's folder is its
cover; a `.nup` shows its own icon.

## Keys and firmware {#keys-and-firmware}

```text
/data/eglantineps5/
├── keys/          prod.keys
└── firmware/      your firmware's files
```

**Settings > Game files > Keys and firmware**, and the Setup check's **Switch 2 keys** and **Switch 2
firmware** rows, say what's found. Neither Playtendoh nor Lichii includes them.

## Settings {#settings}

The Switch 2 side has the Switch side's pages ([Switch settings](switch.md#settings)): resolution,
upscaling, **anti-aliasing**, **FSR sharpening** and the **speed limit**, docked or handheld, and more.
Its **Performance** page also has **Memory**, for the big games that run short of it:

| Row | What it does |
|---|---|
| **Memory packing** | Packs the game's memory it isn't using: **Faster** (LZ4) or **Smaller** (zstd) |
| **Texture memory** | How much memory textures get: Automatic, 2.5, **2 GB** (the default) or 1.5 GB |
| **When memory runs short** | How much is given back at once, from a little at a time to all at once |
| **Keep loaded shaders** | Off by default; on keeps them, using more memory |

A game's own **Performance** page has **Clear shader cache**.

## In a game {#in-game}

- **Touchpad click + Options** opens the in-game menu; **Quit to library** there leaves the game.
- **A game that stops answering**, its menu too: hold **L1** and click the touchpad. That leaves the game,
  and if it won't stop, Playtendoh starts again by itself within half a minute.

## What to expect {#known-problems}

- **Big area loads can run out of memory** and end the game (Ocarina of Time's Hyrule Castle Town market,
  for one). This is the main thing being worked on; the Memory settings above help.
- **At 0.75x resolution**, a line or a bar can show across the picture, and the picture can shift now and
  then.
- **A game's first start** can sit on a black screen for a long time while its shaders are made.
- Mario Kart World and other big games may not run yet on the current core.

Reports help most with the game, your firmware, your settings and the logs: see [Getting help](help.md).
