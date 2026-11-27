# Jailbreak and HEN

Playtendoh doesn't jailbreak your PS5: it runs on top of the jailbreak and the HEN you load. This page
gets you to a running HEN, then has you let Playtendoh in. It works with **any** HEN, and even with no
HEN at all as long as an ELF loader is running.

## 1. Run the exploit {#exploit}

Done on the last page: [the web browser](browser.md), [the YouTube app](youtube.md) or
[a Blu-ray disc](blu-ray.md), whichever [fits your firmware](get-started.md#pick). Each page has what
you need, the exploit's steps and what to do if it fails. For the latest on every route,
[wololo.net's PS5 jailbreak page](https://wololo.net/ps5-jailbreak-and-custom-firmware/) is kept up to
date.

## 2. Load a HEN {#hen}

The exploit then loads a HEN, the part that runs homebrew. Any HEN will do for Playtendoh, as long as
it also runs an **ELF loader (elfldr) on port 9021**: the etaHEN and OnionHEN payloads include one, and
most exploit hosts can load `elfldr.elf`.

| Your firmware | The HEN | What Playtendoh needs from it |
|---|---|---|
| up to 12.70 | **etaHEN**, the most tested | `PPSA99360` on its app jailbreak list ([etaHEN](#etahen)) |
| above 12.70, to 13.60 | **Kstuff Lite** | Nothing to add: an ELF loader on port 9021 ([Kstuff](#kstuff)) |
| where it runs | **OnionHEN** | `PPSA99360` at the end of its `exact_title_ids` ([OnionHEN](#onionhen)) |

## 3. Let Playtendoh in {#app-jailbreak}

Playtendoh needs to reach `/data`, where it keeps its settings, saves and your games. Your HEN opens it
for apps on its **app jailbreak list**, and Playtendoh's title ID is **`PPSA99360`**. Do the steps for
your HEN:

### etaHEN

1. Open etaHEN's settings (its **advanced** or **debug** settings, depending on the version).
2. Add `PPSA99360` to its **app jailbreak list**, and turn the **app jailbreak** option on.

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

### Kstuff, or any HEN without an app jailbreak list {#kstuff}

Nothing to add: Playtendoh looks after itself. Make sure an **ELF loader is listening on port 9021**.
Playtendoh then runs its own helper through it to open `/data`. If your loader listens on another port,
run the standard `elfldr` beside it.

## Game speed {#game-speed}

Wii U and 3DS games run fast because their emulators turn the game's code into the PS5's own as they
go. That needs memory the code can run from. Playtendoh takes it from your HEN when the HEN gives it
(**JIT**), and otherwise makes it itself, which needs no HEN at all. Either way games run at full
speed.

Only when neither works do Wii U games fall back to a much slower way of running. The Setup check's
**Game speed** row and **Settings > Diagnostics** say which one you got:

| Diagnostics says | What it means |
|---|---|
| `JIT available (the HEN's)` | Full speed, from the HEN. |
| `JIT available (executable direct memory, no HEN needed)` | Full speed, without the HEN. |
| `JIT unavailable (interpreter only)` | Wii U games run very slowly. Add `PPSA99360` to your HEN's list (above); if it still says this, [ask for help](help.md) with your firmware, HEN and logs. |

## After every restart {#after-restart}

The jailbreak doesn't survive a full power-off. Each time you turn the PS5 on:

1. Run the exploit again.
2. Load your HEN again.
3. Then start Playtendoh.

Rest mode keeps everything, so you can usually skip this by putting the PS5 in rest mode instead of
turning it off.
