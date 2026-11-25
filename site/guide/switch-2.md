# Switch 2 (experimental)

The Switch 2 side plays on [EglantinePS5](https://github.com/heydemoura/EglantinePS5), a PS5 port of
[Eglantine](https://github.com/Djihads80/Eglantine) and [Lichii](https://github.com/zortos293/Lichii).
It's new and **experimental**: some games play, many don't yet, and it's being worked on all the time.

## Turn the side on {#turn-on}

The Switch 2 side starts hidden. Show it from **Settings > Launcher**, then it's in the bar beside the
other consoles.

## What you need {#what-you-need}

- **Your games**, dumped from your own Switch 2 and cartridges, as game folders.
- **Switch keys** (`prod.keys`) and **firmware**, from your own console.

## Your games {#games}

Put each game in **a folder of its own** in **`/data/eglantineps5/roms`**, or choose any folder on the
side's **Settings > Game files**. A game is its ExeFS and RomFS folders:

```text
/data/eglantineps5/roms/
└── Game/
    ├── exefs/       main, main.npdm, rtld, sdk and every subsdk*
    └── romfs/       the whole RomFS
```

Keep a game's update and DLC with its folder. A `cover.png` or `cover.jpg` in the game's folder is its
cover.

## Keys and firmware {#keys-and-firmware}

```text
/data/eglantineps5/
├── keys/          prod.keys
└── firmware/      your firmware's files
```

The Setup check's **Switch 2 keys** and **Switch 2 firmware** rows say what's found. Neither Playtendoh
nor Lichii includes them.

## What to expect {#known-problems}

- **Big area loads can run out of memory** and end the game (Ocarina of Time's Hyrule Castle Town market,
  for one). This is the main thing being worked on.
- **At 0.75x resolution**, a line or a bar can show across the picture, and the picture can shift now
  and then.
- **The first start of a game** can sit on a black screen for a long time while its shaders are made.
- **A game's settings** are on its page and in its in-game menu (touchpad click + Options), like every
  other side's.

The Switch 2 side is [Phononenom](https://github.com/Phononenom)'s work. Reports help most with the
game, your firmware, your settings and the logs: see [Getting help](help.md).
