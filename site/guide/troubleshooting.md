# Troubleshooting

The problems people bring to the Discord most, each with its fix. Start with the Setup check
(**Settings > Diagnostics > Setup check**): it often names the problem, with a QR code to its fix. If
nothing here helps, [ask for help](help.md) with your logs.

## Starting the app

### It isn't on the home screen, or it won't open {#app-wont-start}

1. The path must be exactly `/data/homebrew/PPSA99360/eboot.bin`: no folder inside a folder
   ([Install](install.md#copy)).
2. Your HEN must be loaded, with `PPSA99360` on its app jailbreak list, or an ELF loader on port 9021
   ([Let Playtendoh in](jailbreak.md#app-jailbreak)).
3. Update ShadowMountPlus, restart the PS5, run the exploit and load the HEN again.
4. Still nothing: send `boot.log` from `/data/ps5cemu/logs` ([Getting help](help.md)).

### "/data can't be reached" {#cannot-reach-data}

No HEN opened `/data` for the app, and no ELF loader was there to do it. Put `PPSA99360` on your HEN's
list, or start an ELF loader on port 9021, then start Playtendoh again
([Let Playtendoh in](jailbreak.md#app-jailbreak)). **Settings > Diagnostics** shows what it got
([reading it](jailbreak.md#check)).

### OnionHEN never jailbreaks the app {#onionhen}

A comma after the last ID, an empty entry or a wrong ID makes OnionHEN drop its **whole** list. Put
`PPSA99360` last, with no comma after it, and load OnionHEN again ([OnionHEN](jailbreak.md#onionhen)).

### It worked, then stopped after a restart {#after-restart}

The jailbreak doesn't survive turning the PS5 off. Run the exploit and load the HEN again after every
power-off ([After every restart](jailbreak.md#after-restart)).

### An old version comes back {#old-version}

There are two copies of Playtendoh on the console: say one in `/data/homebrew` and one on a USB drive, or
an old one you never deleted. The home screen can start the old copy, even after you updated the other.
**Delete every copy but the one you use.** The Setup check's **Another copy of Playtendoh** row says
where the other one is, and its version.

### It opens EmulationStation, not Playtendoh's launcher {#esde}

You picked EmulationStation as the layout. Hold **L1** while Playtendoh starts to get its own launcher,
then **Settings > Launcher > Layout** to switch back for good.

## Games not found

### I can't find the GameCube, Wii, NES, N64... {#hidden-systems}

They appear in the bar as soon as Playtendoh finds their games. Before that, or if you hid one, turn it on
in **Settings > Launcher > Systems** ([how](setup-check.md#systems)).

### A system's Library is empty {#library-empty}

- Its empty Library's **Look for games** looks on your drives and in `/data` now. Games can be anywhere on a
  USB drive or in `/data/playtendoh`, in any folders ([Where your files go](files.md#anywhere)).
- Check their format is one the system plays (each system's page lists them): a game in a `.7z` or `.rar`
  is only found in the system's own folder or one you chose, as which console it is for can't be told from
  outside.
- If you chose a folder in **Settings > Game files**, that folder is the system's only one: check the games
  are in it, or choose its own folder again to have the ones found elsewhere back.
- The Setup check's row for that system says how many games it found, and where most of them are.

### A game on a USB drive is gone {#drive-games-gone}

Its drive was taken out, or the PS5 gave it another number. Put it back: Playtendoh looks again as the
drive comes in, and the games return. Nothing about them was forgotten.

### Wii U: an unpacked game is found but not listed {#wii-u-not-listed}

Check it has all three folders, `code`, `content` and `meta`, with the game's files in them (5.1.0 also
missed them when they were named in capitals; 6.0 doesn't). A `.wua` of the same game plays meanwhile.
Still missing? Send `cemu.log`: it says why a game was left out.

### Wii U: encrypted games vanished, or keys.txt is empty {#wii-u-keys}

Version 5.1.0 could empty `keys.txt` as it started, and then every encrypted `.wud` and `.wux` left the
Library. Copy your keys back into `/data/cemu/keys.txt`, and keep a copy on your computer
([disc keys](wii-u.md#disc-keys)).

### Switch: games are counted but not listed {#switch-not-listed}

Usually the keys: without working `prod.keys` the side can't read a game. Check **Settings > Game files >
Keys and firmware** and the Setup check's **Switch keys** row. With firmware 23 and its keys, this is a
known problem being looked at.

## Games that won't start

### Switch: error 59, or "Loader status 28" {#switch-error-59}

Your keys and firmware don't match: keys from an older system version than the firmware. Dump **both**
again from your own Switch, at the same time ([keys and firmware](switch.md#keys-and-firmware)). Playtendoh
copies keys and firmware it finds only where none are yet, so it never replaces a pair that doesn't match:
take the old ones out of `/data/eden/keys` and `/data/eden/firmware`, put the new dump anywhere, and the
Setup check's **Find games and files**, then **Files to copy**, puts it in place.

### Breath of the Wild asks for a system update {#botw-update}

Install the game's own update (v1.6.0 or later), from **Settings > Game files > Updates, DLC and mods**
([Wii U updates](wii-u.md#updates)).

### Wii games for the Wii U don't start {#wii-on-wii-u}

Cemu doesn't play Wii games, the Wii U eShop's included. Play them on the [GameCube and Wii](gamecube-wii.md)
side.

### Switch: a black screen from a USB drive {#switch-usb}

On some consoles, Switch games on a USB drive or in a folder of your own go to a black screen, where the
same file plays from `/data/eden/roms`. Play from there for now.

### Nothing starts after rest mode {#rest-mode}

Close Playtendoh and start it again.

### A Famicom Disk System or Satellaview game won't start {#bios}

They need their console's BIOS: `disksys.rom` or `BS-X.bin` ([the classics' BIOS](classics.md#bios)).

## Speed, freezes and crashes

### Wii U games are very slow {#slow}

The Setup check's **Game speed** row, or **Settings > Diagnostics**, probably says
`JIT unavailable (interpreter only)`. Put `PPSA99360` on your HEN's app jailbreak list
([game speed](jailbreak.md#game-speed)).

### A game takes minutes to start the first time {#first-start-slow}

A Wii U, 3DS or Switch game builds its shaders the first time, with the loading screen counting them. The
next starts are quick. A Switch 2 game can sit on a black screen for a long time while it does.

### A game froze {#frozen}

- **A Switch or Switch 2 game, its menu too:** hold **L1** and click the touchpad's left side (any of the
  touchpad, on the Switch 2). That leaves the game, and if it won't stop, Playtendoh starts again by itself
  within half a minute.
- **On the other systems:** close Playtendoh from the PS button's menu.

### One game crashes or misbehaves {#one-game}

Look the game up on the [compatibility list](../compatibility.html): someone may have found the setting
that fixes it. If not, report it with its logs ([Getting help](help.md)).

### Games on a USB drive "cannot be read" {#usb-cannot-be-read}

Your HEN opened `/data` but not the drives. Make sure an ELF loader is running on port 9021, and start the
app again: it opens the drive through it.

### A network share's games don't show {#network-share}

A share is looked through only when you ask: **Settings > Game files > Look for games now**, or turn on
**Look on network shares at every start**. Check the PS5 and the computer are on the same network, and
that the share's address, name and login are right. Switch and Switch 2 games can't play from a share.

## Phones and the network

### The phone can't open the remote touch screen page {#remote-touch}

- The phone must be on the **same network** as the PS5. Guest Wi-Fi often keeps devices apart: use the
  main one.
- Use the address the setting shows, with its port (`:8080`).
- The page says "Waiting for video" until a game is running; that's normal.

### Covers are missing {#covers}

Box art downloads in the background as games are added, and needs the PS5 online. **Settings > Online >
Download missing box art** tries again ([box art](setup-check.md#box-art)).
