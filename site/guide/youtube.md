# The YouTube app

**Y2JB** runs its exploit from inside the PS5's YouTube app, at an old version of the app. It covers
the firmwares between the browser's two ranges, so a PS5 without a disc drive on 6.xx has a way in
too.

| Your firmware | The exploit after Y2JB | The YouTube app's version |
|---|---|---|
| 4.03 – 10.01 | **Lapse** | 01.000.003 |
| 9.00 – 12.40 | **P2JB** (some report it up to 12.70) | 01.000.003; 01.000.030 from 12.60 |

Its projects: [Y2JB](https://github.com/Gezine/Y2JB), and [P2JB on Y2JB](https://github.com/matem6/P2JB-Y2JB-Porting).
Not sure this is your way in? [Pick your firmware](get-started.md#pick).

> [!CAUTION]
> **This way in erases a PS5 that isn't jailbroken yet.** The only way to put the old YouTube app on such
> a console is restoring a backup from USB, and a restore **resets the PS5 to factory settings**:
> everything on it goes. If [the web browser](browser.md) or [a Blu-ray disc](blu-ray.md) fits your
> firmware, use that instead.

> [!IMPORTANT]
> Read the whole page before you start, then follow it in order. This guide doesn't host exploits:
> for the backup and the payloads, [Y2JB's own instructions](https://github.com/Gezine/Y2JB) have the
> last word. Follow them exactly.

## What you need {#what-you-need}

- **A PS5 on 4.03 to 12.40**, any model, with or without a disc drive.
- **A USB drive** for the YouTube app's backup, formatted and laid out as Y2JB's instructions say.
- **Y2JB's backup of the old YouTube app** (title ID `PPSA01650`), at the version in the table for
  your firmware, from its project's page.
- **Your saves backed up:** to PS Plus's cloud storage or a USB drive. The restore erases the console.
- **A computer on the same network**, to copy Playtendoh to the PS5 later, and to send payloads if
  your Y2JB setup sends them from one (its instructions say).

## Section I: Prep work {#prep}

1. Check your firmware once more: **Settings > System > System Software > Console Information**.
2. Back up your saves. Anything not backed up is gone after the restore.
3. Turn off automatic updates, for the system and for apps: **Settings > System > System Software >
   System Software Update and Settings**, and the automatic downloads of app updates. The YouTube app
   has to stay at its old version: if it updates, the exploit is gone until you restore it again.

## Section II: Put the old YouTube app on {#old-app}

Only on a PS5 that isn't jailbroken yet. Already jailbroken? Y2JB's instructions have a way without
the restore.

1. Copy Y2JB's backup to the USB drive, in the folders its instructions give.
2. Plug the drive into the PS5 and restore it: **Settings > System > System Software > Back Up and
   Restore**. The console is erased, restores the backup and starts again.
3. Set the console up again, offline if Y2JB's instructions say so, so the YouTube app isn't updated.
4. Check the YouTube app's version: highlight it on the home screen, press **Options** and choose
   **Information**. It must match the table above.

## Section III: Run the exploit {#run}

Run the one for your firmware.

### Lapse, 4.03 to 10.01 {#lapse}

1. Start the YouTube app. Y2JB runs as it opens.
2. Load Lapse as Y2JB's instructions say, and wait for the notification that it's done.

### P2JB, 9.00 to 12.40 {#p2jb}

1. Start the YouTube app. Y2JB runs as it opens.
2. Load P2JB as its instructions say, and wait for the notification that it's done.

> [!TIP]
> **A kernel exploit doesn't always work the first time.** If the app stops, the PS5 freezes or it
> restarts, nothing is harmed: restart the PS5 and run it again. It can take a few tries.

## Section IV: Load the HEN {#hen}

Load the HEN for your firmware, the way Y2JB's instructions send payloads:

- **Up to 12.70: etaHEN.** It's the HEN Playtendoh is tested with most.
- **Above 12.70: Kstuff Lite**, with an ELF loader on port 9021.

Wait for the HEN's notification before you go on. The next page has each HEN, and the one setting
Playtendoh needs in it.

## After every restart {#again}

The jailbreak doesn't survive turning the PS5 off. After every full power-off: start the YouTube app
again, run the exploit, load your HEN, then start Playtendoh. Rest mode keeps the jailbreak, so you can
usually put the PS5 in rest mode instead.
