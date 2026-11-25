# Nintendo Switch

The Switch side plays on [Eden](https://github.com/eden-emu/mirror), in
[ProsperoEden](https://github.com/Swordpdf/ProsperoEden)'s PS5 port.

## What you need {#what-you-need}

- **Your games** (`.nsp` or `.xci`), dumped from your own Switch and cartridges.
- **Your Switch's keys:** `prod.keys` and `title.keys`.
- **Your Switch's firmware:** its `.nca` files.

All three come from **your own Switch**. Playtendoh includes none, and the Switch side reads no game
without the keys.

## Your games {#games}

Put them in **`/data/eden/roms`**, or choose any folder on the Switch side's **Settings > Game files**
(on the PS5 or a drive; not a network share).

```text
/data/eden/roms/
├── Game [0100000000010000].nsp        a game on its own
└── Another Game/                      or a game in a folder of its own,
    ├── Another Game.xci               with its update and DLC beside it
    └── Another Game [Update].nsp
```

A title ID in brackets in a file's name, as dumps have it, keeps the game's settings and its place in
Recently played whatever you call the file.

## Keys and firmware {#keys-and-firmware}

```text
/data/eden/
├── keys/
│   ├── prod.keys
│   └── title.keys
└── firmware/          every .nca file of your Switch's firmware
```

> [!IMPORTANT]
> **Keys and firmware must be from the same Switch, at the same system version.** Keys from an older
> version than the firmware are the cause of **error 59** and **"Loader status 28"**: dump both again
> from your Switch, at the same time. Playtendoh says so when it sees keys that are too old.

The Setup check's **Switch keys** and **Switch firmware** rows say what's found. "prod.keys is empty"
means the file is there with nothing in it: copy yours again.

A standalone ProsperoEden's files in `/data/prosperoeden` are used while `/data/eden` has no keys.

## Updates and DLC {#updates}

No installing needed. Put an update or DLC:

- in the game's own folder, or beside the game, in any games folder;
- or in **`/data/eden/updates`** (or a `/data/eden/dlc` you make).

A game's page shows the update's version and how many DLC it found (`Update v1.2.0`, `2 DLC`).

## Mods and cheats {#mods}

In **`/data/eden/mods`**.

## Known problems {#known-problems}

These are being worked on. Until then:

- **Mario Kart 8 Deluxe goes back to its loading screen** after the title's voice line, over and over.
  No fix yet; logs from it help ([Getting help](help.md)).
- **Games on a USB drive or a folder of your own go to a black screen** on some consoles, where the
  same file plays from the internal storage. Play from `/data/eden/roms` for now.
- **After rest mode, no game starts.** Close Playtendoh and start it again.
- **With firmware 23 and its keys, games are counted but not listed.** Being looked at.

**A game that stops answering**, its menu too: hold **L1** and click the touchpad's left side. That
leaves the game, and if it won't stop, Playtendoh starts again by itself within half a minute.
