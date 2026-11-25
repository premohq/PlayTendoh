# Install Playtendoh

Playtendoh is one folder, `PPSA99360`, that goes in the PS5's homebrew folder. Your homebrew loader
(ShadowMountPlus) then puts it on the home screen.

## 1. Download it {#download}

Get the latest release's ZIP (`PS5-ECAM-v….zip`) from the
[download button on the home page](../#release) or
[GitHub's releases](https://github.com/premohq/PlayTendoh/releases/latest). It is also in the
[Prospero Store](https://prosperostore.net)'s homebrew catalog.

Unzip it on your computer. Inside is one folder, **`PPSA99360`**, holding `eboot.bin`, `sce_sys` and
the rest.

## 2. Copy it to the PS5 {#copy}

Copy the **`PPSA99360` folder itself** (not just what's inside it) to **`/data/homebrew/`**, with FTP
or a file manager. When it's right, this file exists:

```text
/data/homebrew/PPSA99360/eboot.bin
```

> [!WARNING]
> **The two mistakes people make most:** a folder inside a folder (`/data/homebrew/PPSA99360/PPSA99360/`),
> or the ZIP's contents spread straight into `/data/homebrew`. Either way the PS5 can't find
> `eboot.bin`. Move things until the path above is exactly right.

### Or on a USB drive or the extended storage {#on-a-drive}

The app can live at the top of a drive instead: `homebrew/PPSA99360` on it (the PS5 sees that as
`/mnt/usb0/homebrew/PPSA99360`, or `/mnt/ext0/...` for the extended storage). Playtendoh then keeps its
own files on that drive too, in a `PS5CEMU-HAR-data` folder beside it ([Where your files go](files.md#app-on-a-drive)).

## 3. Keep one copy {#one-copy}

> [!CAUTION]
> **Have one Playtendoh on the console, never two.** A copy left in `/data/homebrew` beside one on a drive,
> or an old version you never deleted, is why "an old version comes back" after a restart: the home
> screen can start that copy instead. Delete every copy but the one you use. Playtendoh's Setup check
> warns you when it sees another.

## 4. Put it on the home screen {#home-screen}

With your HEN loaded, ShadowMountPlus adds the folders in `/data/homebrew` (and a drive's `homebrew`
folder) to the home screen. Playtendoh shows up with its own icon.

Not there? In this order:

1. Check the path again: `/data/homebrew/PPSA99360/eboot.bin`.
2. Update ShadowMountPlus to its latest version.
3. Restart the PS5, run the exploit and load the HEN again ([after every restart](jailbreak.md#after-restart)).
4. Still nothing: [Troubleshooting](troubleshooting.md#app-wont-start).

## 5. Start it {#start}

Start Playtendoh from the home screen. The first start asks a few questions, then the next page takes
it from there.

> [!TIP]
> **Updating later** is one button inside the app: it finds new releases by itself and keeps your
> games, saves and settings. See [Updating](updating.md).
