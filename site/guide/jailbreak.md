# Jailbreak and HEN

Playtendoh doesn't jailbreak your PS5: it runs on top of the jailbreak and the HEN you load, and it works
with **any** of them, even with no HEN at all as long as an ELF loader is running. This page gets you to
a running HEN, then has you let Playtendoh in, then shows how to check it worked.

## 1. Run the exploit {#exploit}

Use the way in that the [table on the last page](get-started.md#can-it) gave for your firmware, and
follow that project's own instructions: this guide doesn't host exploits, and they change often. Good
places to start:

- [wololo.net's PS5 jailbreak page](https://wololo.net/ps5-jailbreak-and-custom-firmware/): every route,
  kept up to date.
- [Y2JB](https://github.com/Gezine/Y2JB) (the YouTube app) and [BD-JB](https://github.com/Gezine/BD-JB-1250) (a Blu-ray disc).
- [PSFree and Lapse](https://github.com/EchoStretch/PSFree/blob/main/README.md) (the browser, older firmwares).
- [Relapse, 7.00 to 13.60](https://gbatemp.net/threads/relapse-ps5-kernel-exploit-brings-homebrew-to-firmware-7-00-13-60.684829/) (the browser).

> [!TIP]
> Exploits don't work every time. If the PS5 freezes or the page says it failed, restart the console and
> try again; a few tries is normal.

## 2. Load a HEN {#hen}

The exploit then loads a HEN, which runs homebrew apps:

| HEN | Firmware | App jailbreak list | ELF loader on 9021 |
|---|---|---|---|
| **etaHEN** | up to 12.70 | Yes (most tested with Playtendoh) | Included |
| **OnionHEN** | see its own notes | Yes, in `config.ini` | Included |
| **Kstuff Lite** | up to 13.60 | No: Playtendoh helps itself | Load `elfldr.elf` with it |
| Any other exploit host | any | Usually not | Load `elfldr.elf` |

Whatever you use, make sure an **ELF loader (elfldr) is listening on port 9021**. Playtendoh uses it to
open `/data` when no HEN does, to open USB drives, and to put home screen tiles in place.

## 3. Let Playtendoh in {#app-jailbreak}

Playtendoh needs `/data`, where it keeps its settings, saves and your games. Your HEN opens it for apps on
its **app jailbreak list**, and Playtendoh's title ID is **`PPSA99360`**. Do the steps for your HEN.

### etaHEN

1. Open etaHEN's settings (its **advanced** or **debug** settings, depending on the version).
2. Add `PPSA99360` to its **app jailbreak list**.
3. Turn the **app jailbreak** option on.

### OnionHEN

1. Open OnionHEN's `config.ini` and find the `[app_jailbreak]` section.
2. Set `enabled=true`. Set `debug_notifications=true` too: OnionHEN then says each time it jailbreaks an
   app, so you can see it worked.
3. Add `PPSA99360` at the **end** of `exact_title_ids`, with **no comma after it**:

   ```ini
   [app_jailbreak]
   enabled=true
   debug_notifications=true
   exact_title_ids=ITEM00001,NPXS39041,PKGI13337,PKGI12345,TOOL00001,PPSA99360
   ```

4. Load OnionHEN again (or restart the console and load it again), so it reads the new list.

> [!CAUTION]
> OnionHEN throws away the **whole** list when one entry is wrong: a comma at the end
> (`...,PPSA99360,`), an empty entry, a space, or an ID that isn't exactly 9 capital letters and digits.
> Then no app is jailbroken at all. It takes 20 entries at most.

### Kstuff, or any HEN without an app jailbreak list

Nothing to add. Make sure an **ELF loader is listening on port 9021**: Playtendoh then runs its own
helper through it to open `/data`, and makes the memory its emulators need itself. If your loader
listens on another port, run the standard `elfldr` beside it.

## 4. Check it worked {#check}

Once Playtendoh is installed ([next page](install.md)), open **Settings > Diagnostics**. Its first lines
say what the app got:

| Diagnostics says | What it means |
|---|---|
| `HEN: ok (HEN took the request ...)` | Your HEN jailbroke the app. |
| `HEN: no (no HEN took the request ...)` | No HEN answered: `PPSA99360` isn't on its list (check OnionHEN's comma), or the HEN has none. **Fine if the next two lines are good.** |
| `/data reachable` | Good. |
| `/data unreachable (elevation helper: ...)` | Neither the HEN nor elfldr opened `/data`: [fix it](troubleshooting.md#cannot-reach-data). |
| `... opened by the elevation helper` | A drive or folder was opened through elfldr. Good. |

The Setup check's **Storage** and **Game speed** rows say the same in plain words.

## Game speed {#game-speed}

Wii U and 3DS games run fast because their emulators turn the game's code into the PS5's own as they go,
which needs memory that code can run from. Playtendoh takes it from your HEN when the HEN gives it
(**JIT**), and otherwise makes it itself, which needs no HEN. Either way games run at full speed.

| Diagnostics says | What it means |
|---|---|
| `JIT available (the HEN's)` | Full speed, from the HEN. |
| `JIT available (executable direct memory, no HEN needed)` | Full speed, without the HEN. |
| `JIT unavailable (interpreter only)` | Wii U games run very slowly. Add `PPSA99360` to your HEN's list; if it still says this, [ask for help](help.md) with your firmware, HEN and logs. |
| `...; no executable direct memory: 3DS games run on Azahar's interpreter` | The HEN gave JIT but not the other kind; 3DS games run slower. |

## After every restart {#after-restart}

The jailbreak doesn't survive a full power-off. Each time you turn the PS5 on:

1. Run the exploit again.
2. Load your HEN again (and `elfldr` if it isn't part of it).
3. Then start Playtendoh.

Rest mode keeps everything, so putting the PS5 in rest mode instead of turning it off saves you this.
