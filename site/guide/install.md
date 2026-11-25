# Install Playtendoh

Playtendoh is one folder, `PPSA99360`, that goes in the PS5's homebrew folder. ShadowMountPlus then puts
it on the home screen like any other app.

## 1. Download it {#download}

Get the ZIP (`PS5-ECAM-v….zip`) from the [download button on the home page](../#release) or from
[GitHub's releases](https://github.com/premohq/PlayTendoh/releases/latest). It is also in the
[Prospero Store](https://prosperostore.net)'s homebrew catalog, which tells your console when there's a
new version.

> [!TIP]
> The release notes list the ZIP's **SHA-256**. If a download keeps failing to unzip, compare it: a
> mismatch means the download was cut short, so download it again.

Unzip it on your computer. Inside is one folder, **`PPSA99360`**:

```text
PPSA99360/
├── eboot.bin          the app
├── sce_sys/           its name, icon and version
├── sce_module/        the libraries it loads
├── assets/            the launcher's art, fonts and each emulator's files
├── eden/              the Switch's program
├── eglantine/         the Switch 2's program
├── retro/             the GameCube and Wii's program
└── sandbox-elevator.elf, tile-installer...   small helpers it runs through elfldr
```

## 2. Copy it to the PS5 {#copy}

Copy the **`PPSA99360` folder itself**, not just what's inside it, into **`/data/homebrew/`**. Make
`/data/homebrew` first if it isn't there.

**With FTP** (etaHEN's FTP server, for example):

1. Connect your FTP client to the PS5's address (**Settings > Network > Connection Status > View
   Connection Status** on the PS5 shows it) and the FTP server's port.
2. Go to `/data/homebrew`.
3. Upload the whole `PPSA99360` folder. It's a few hundred files; let it finish.

**With a USB drive:** copy `PPSA99360` onto the drive, plug it into the PS5, and copy it to
`/data/homebrew/` with a file manager on the PS5.

When it's right, this file exists:

```text
/data/homebrew/PPSA99360/eboot.bin
```

> [!WARNING]
> **The two mistakes people make most:** a folder inside a folder (`/data/homebrew/PPSA99360/PPSA99360/`),
> or the ZIP's contents spread straight into `/data/homebrew`. Either way the PS5 can't find `eboot.bin`.
> Move things until the path above is exactly right.

### Or on a USB drive or the extended storage {#on-a-drive}

The app can live at the top of a drive instead: in a `homebrew/PPSA99360` folder on it (the PS5 sees that
as `/mnt/usb0/homebrew/PPSA99360`, or `/mnt/ext0/homebrew/PPSA99360` for the extended storage).
Playtendoh then keeps its own files on that drive too, in a `PS5CEMU-HAR-data` folder beside it, with each
system's folder beside that ([Where your files go](files.md#app-on-a-drive)). Use this when the PS5's own
storage is full; otherwise `/data/homebrew` is simpler.

## 3. Keep one copy {#one-copy}

> [!CAUTION]
> **Have one Playtendoh on the console, never two.** A copy left in `/data/homebrew` beside one on a drive,
> or an old one you never deleted, is why "an old version comes back" after a restart: the home screen
> can start that copy instead of the one you updated. Delete every copy but the one you use. Playtendoh's
> Setup check warns you when it sees another, and says where it is.

## 4. Put it on the home screen {#home-screen}

With your HEN loaded, ShadowMountPlus adds the folders in `/data/homebrew` (and a drive's `homebrew`
folder) to the home screen. Playtendoh shows up with its own icon: every console's controller in a ring.

Not there? In this order:

1. Check the path again: `/data/homebrew/PPSA99360/eboot.bin`.
2. Update ShadowMountPlus to its latest version.
3. Restart the PS5, run the exploit and load the HEN again ([after every restart](jailbreak.md#after-restart)).
4. Still nothing: [Troubleshooting](troubleshooting.md#app-wont-start).

## 5. Start it {#start}

Start Playtendoh from the home screen. The first time, it asks five questions, one a screen, and the
[next page](setup-check.md) goes through them.

> [!TIP]
> **Updating later** is one button inside the app: it finds new releases by itself and keeps your games,
> saves and settings. See [Updating](updating.md).
