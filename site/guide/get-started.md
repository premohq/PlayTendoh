# Get started

Before anything else, find out whether your PS5 can run homebrew. That depends on one thing only: its
**firmware version**.

## Find your firmware {#firmware}

On the PS5: **Settings > System > System Software > Console Information**. Write the version down
(for example `11.60`).

> [!CAUTION]
> **Don't update your PS5 from now on.** Turn off automatic updates (**Settings > System > System
> Software > System Software Update and Settings**). A PS5 can't be put back on an older firmware, and
> an update can take away the jailbreak for good.

## Can it run Playtendoh? {#can-it}

Every firmware from **1.00 to 13.60** has at least one public way to run homebrew, on every model,
with or without a disc drive. **14.00 and newer has none yet**, and nothing Playtendoh could do would
change that.

| Your firmware | The way in | What you need |
|---|---|---|
| 1.00 – 5.50 | The web browser (**UMTX**) | Nothing extra |
| 4.03 – 10.01 | The YouTube app (**Y2JB**), then **Lapse** | An old version of the YouTube app |
| 9.00 – 12.40 | The YouTube app (**Y2JB**), then **P2JB** | An old version of the YouTube app |
| up to 7.61 | A Blu-ray disc (**BD-JB**), then **UMTX2** | A disc drive and a Blu-ray you burn |
| up to 12.00 | A Blu-ray disc (**BD-JB**), then **Poopsploit** | A disc drive and a Blu-ray you burn |
| 7.00 – 13.60 | The web browser (**Relapse**) | Nothing extra; Slim and Pro too |
| **14.00 and newer** | **None yet** | Wait, and don't update |

Where two rows fit your firmware, either works. The browser routes need the least. This table is a
snapshot: the scene moves fast, so check a current source such as
[wololo.net's PS5 jailbreak page](https://wololo.net/ps5-jailbreak-and-custom-firmware/) before you
start.

> [!WARNING]
> **Y2JB on a console that isn't jailbroken yet** needs the YouTube app at an old version, and getting it
> there means restoring a backup from USB, which **resets the PS5 to factory settings**. Back up your
> saves first and follow the [Y2JB project's instructions](https://github.com/Gezine/Y2JB) exactly.

## What you'll need {#what-you-need}

- **A PS5 on firmware 13.60 or lower**, any model.
- **A HEN or an ELF loader:** etaHEN, OnionHEN, Kstuff, or any exploit host that runs `elfldr` on port
  9021. The [next page](jailbreak.md) covers each.
- **A way to copy files to the PS5:** an FTP client on a computer (etaHEN has an FTP server), a file
  manager on the PS5, or a USB drive. Once Playtendoh runs, your phone can send games to it too
  ([Where your files go](files.md#from-a-phone)).
- **Room for your games** on the PS5's storage, an extended storage drive or a USB drive.
- **Your own dumps** of each system you want to play: games, and for some systems keys, firmware or a
  BIOS. Each [system's page](wii-u.md) says what it needs, and how you dump it from your own console.

> [!NOTE]
> **The jailbreak is tethered.** It doesn't survive turning the PS5 off. After every full power-off you
> run the exploit and load the HEN again, *then* start Playtendoh. Rest mode keeps it.
