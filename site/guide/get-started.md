# Get started

Before anything else, find out whether your PS5 can run homebrew, and how. That depends on one thing
only: its **firmware version**. This page also explains the words the rest of the guide uses, and what
to have ready.

## 1. Find your firmware {#firmware}

On the PS5: **Settings > System > System Software > Console Information**. Write the version down, for
example `11.60`.

> [!CAUTION]
> **Don't update your PS5 from now on.** Turn off automatic updates in **Settings > System > System
> Software > System Software Update and Settings**, and don't accept an update a game or a disc offers.
> A PS5 can't go back to an older firmware, and an update can take the jailbreak away for good.

## 2. Can it run Playtendoh? {#can-it}

Every firmware from **1.00 to 13.60** has at least one public way to run homebrew, on every model, with
a disc drive or without. **14.00 and newer has none yet**, and nothing Playtendoh could do would change
that: wait, and don't update.

| Your firmware | The way in | What you need |
|---|---|---|
| 1.00 – 5.50 | The web browser (**UMTX**) | Nothing extra |
| 4.03 – 10.01 | The YouTube app (**Y2JB**), then **Lapse** | An old version of the YouTube app |
| 9.00 – 12.40 | The YouTube app (**Y2JB**), then **P2JB** | An old version of the YouTube app |
| up to 7.61 | A Blu-ray disc (**BD-JB**), then **UMTX2** | A disc drive and a Blu-ray you burn |
| up to 12.00 | A Blu-ray disc (**BD-JB**), then **Poopsploit** | A disc drive and a Blu-ray you burn |
| 7.00 – 13.60 | The web browser (**Relapse**) | Nothing extra; Slim and Pro too |
| **14.00 and newer** | **None yet** | Wait, and don't update |

Where two rows fit your firmware, either works; the browser ones need the least. This table is a
snapshot of a scene that moves fast, so check a current source such as
[wololo.net's PS5 jailbreak page](https://wololo.net/ps5-jailbreak-and-custom-firmware/) before you
start.

> [!WARNING]
> **Y2JB on a console that isn't jailbroken yet** needs the YouTube app at an old version: 01.000.003 for
> firmwares 4.03 to 12.40, 01.000.030 from 12.60. Getting it there means restoring a backup from USB,
> which **resets the PS5 to its factory settings**. Back up your saves first, and follow the
> [Y2JB project's instructions](https://github.com/Gezine/Y2JB) exactly.

## 3. The words this guide uses {#words}

| Word | What it means |
|---|---|
| **Exploit** | The way in: the web page, YouTube app or Blu-ray that first runs outside code on the PS5. |
| **Kernel exploit** | What the exploit runs next, to take over the system (UMTX, Lapse, P2JB, Relapse...). |
| **HEN** | What loads after it and runs homebrew apps: etaHEN, OnionHEN, Kstuff. |
| **ELF loader (elfldr)** | A small server, usually on port **9021**, that runs programs sent to it. Most HENs include one. |
| **Payload** | A program you send to the PS5 to run, such as a HEN or `elfldr.elf`. |
| **ShadowMountPlus** | What puts homebrew app folders on the PS5's home screen. |
| **Title ID** | An app's ID. Playtendoh's is always **`PPSA99360`**. |
| **Dump** | A copy of a game, key or file taken from a console or game you own, with homebrew on that console. |
| **Keys, firmware, BIOS** | A console's own files that some emulators need. Each system's page says which. |

## 4. What you'll need {#what-you-need}

- **A PS5 on firmware 13.60 or lower**, any model.
- **A HEN or an ELF loader:** etaHEN, OnionHEN, Kstuff, or any exploit host that runs `elfldr` on port
  9021. The [next page](jailbreak.md) covers each.
- **A computer** to unzip the app and copy files, and **a way to copy them to the PS5**:
  - an FTP client such as FileZilla or WinSCP, with your HEN's FTP server (etaHEN has one); or
  - a USB drive and a file manager on the PS5.
  Once Playtendoh runs, your phone or computer can also send games straight to it in a browser
  ([how](files.md#from-a-phone)).
- **Room for your games**, on the PS5's own storage, an extended storage drive or a USB drive.
- **Your own dumps** of each system you want to play: its games, and for some systems keys, firmware or
  a BIOS. Each system's page says what it needs:

| System | Needs besides games |
|---|---|
| [Wii U](wii-u.md) | Disc keys, only for encrypted `.wud` and `.wux` dumps |
| [3DS](3ds.md) | Nothing (games must be decrypted); system files optional |
| [DS](ds.md) | Nothing; your DS's BIOS optional |
| [Switch](switch.md) | **Keys and firmware**, from your own Switch |
| [Switch 2](switch-2.md) | **Keys and firmware**, from your own console |
| [GameCube and Wii](gamecube-wii.md) | Nothing |
| [The classics](classics.md) | A BIOS only for the Famicom Disk System and the Satellaview |

> [!NOTE]
> **The jailbreak is tethered.** It doesn't survive turning the PS5 off. After every full power-off you run
> the exploit and load the HEN again, *then* start Playtendoh. Rest mode keeps it, so you can usually put
> the PS5 in rest mode instead.
