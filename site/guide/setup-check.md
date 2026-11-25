# The first start and the Setup check

## The first start's questions {#questions}

The first time Playtendoh starts, it asks a few things, one a screen, and you can change every answer
later in Settings:

1. **Language:** your PS5's, unless you pick another.
2. **Layout:** Classic, Orbit or EmulationStation.
3. **Covers:** flat box art or 3D boxes.
4. **Buttons:** which button is A in the games (Cross or Circle).
5. **Reports:** whether Playtendoh may send a short report when a game ends, so the project sees what
   breaks. It never holds your name, your console's ID or your files.

Then it opens on a console's Library.

## The Setup check {#setup-check}

The Setup check answers "why doesn't it work?" in one screen. It opens by itself when a start finds
something wrong with storage or game speed, and you can open it any time: **Settings > Diagnostics >
Setup check**.

Each row is one thing Playtendoh needs, grouped:

- **Needs you** (amber): something to fix before those games can play.
- **Optional** (grey): only some games need it.
- **Ready** (green): nothing to do.

Move to a row and the panel beside it says what it means and what to do, with a **QR code**: scan it
with your phone and it opens the page of this guide about that row. **Cross** does the row's action
(choose a folder, see how), **Triangle** checks everything again after you've copied files, and
**Circle** goes on to the Library.

## Every row, and what to do {#rows}

| Row | What to do when it isn't ready |
|---|---|
| **Storage** | `/data` can't be reached. [Let Playtendoh in](jailbreak.md#app-jailbreak) through your HEN, or run an ELF loader on port 9021, then start the app again. |
| **Game speed** | Wii U games would run slowly. See [game speed](jailbreak.md#game-speed). |
| ***Console* games** | No games found yet in that console's folder. Copy them there, or press Cross to choose another folder. Each [system's page](files.md) says where. |
| **Wii U disc keys** | Only encrypted `.wud` and `.wux` need them. See [disc keys](wii-u.md#disc-keys). "No keys" on a file you filled before means it was emptied: put your copy back. |
| **3DS system files** | Optional: for the 3DS Home Menu and a few games. See [system files](3ds.md#system-files). |
| **DS BIOS** | Optional: most games run on the built-in one. See [DS BIOS](ds.md#bios). |
| ***Console* BIOS** | The Famicom Disk System and the Satellaview need theirs; for the others it's optional. See [the classics' BIOS](classics.md#bios). |
| **Switch keys**, **Switch firmware** | Switch games need both, from your own Switch. See [keys and firmware](switch.md#keys-and-firmware). |
| **Switch 2 keys**, **Switch 2 firmware** | The same for the Switch 2 side. See [its keys and firmware](switch-2.md#keys-and-firmware). |
| **Box art** | Covers download from GameTDB. See [box art](#box-art) below. |
| **System folders** | Where each system's files are kept. See [Where your files go](files.md). |
| **Another copy of Playtendoh** | A second copy of the app is on the console, and the home screen may start that one. [Keep one](troubleshooting.md#old-version). |

## Box art {#box-art}

The first time a game shows up, Playtendoh downloads its cover from [GameTDB](https://www.gametdb.com/)
in the background (a Switch game's by its name). Without an internet connection, or when GameTDB
doesn't answer, the Library shows the game's own icon and tries again next time.

**Settings > Online > Box art** turns the downloads off and on, and has **Download missing box art**.
Beside a Switch game, a picture with the game's name (`Game.png` or `Game.jpg`) is its cover instead.
