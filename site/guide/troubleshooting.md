# Troubleshooting

The problems people bring to the Discord most, each with its fix. Start with the Setup check
(**Settings > Diagnostics > Setup check**): it often names the problem. If nothing here helps,
[ask for help](help.md) with your logs.

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
([Let Playtendoh in](jailbreak.md#app-jailbreak)).

### OnionHEN never jailbreaks the app {#onionhen}

A comma after the last ID, an empty entry or a wrong ID makes OnionHEN drop its **whole** list. Put
`PPSA99360` last, with no comma after it, and load OnionHEN again ([OnionHEN](jailbreak.md#onionhen)).

### It worked, then stopped after a restart {#after-restart}

The jailbreak doesn't survive turning the PS5 off. Run the exploit and load the HEN again after every
power-off ([After every restart](jailbreak.md#after-restart)).

### An old version comes back {#old-version}

There are two copies of Playtendoh on the console: say one in `/data/homebrew` and one on a USB drive,
or an old one you never deleted. The home screen can start the old copy. **Delete every copy but the
one you use.** The Setup check's **Another copy of Playtendoh** row says where the other one is.

## Games not found

### A console's Library is empty {#library-empty}

- Check the games are in that console's folder ([Where your files go](files.md)), or that the folder
  you chose in **Settings > Game files** is the one they're in.
- **Settings > Game files > Look for games now** looks again.
- The Setup check's row for that console says how many games it found, and where.

### Wii U: an unpacked game is found but not listed {#wii-u-not-listed}

Name its three folders `code`, `content` and `meta`, in small letters. A `.wua` of the same game plays
meanwhile. Still missing? Send `cemu.log`: it says why a game was left out.

### Wii U: encrypted games vanished, or keys.txt is empty {#wii-u-keys}

Version 5.1.0 can empty `keys.txt` as it starts, and then every encrypted `.wud` and `.wux` leaves the
Library. Copy your keys back into `/data/cemu/keys.txt` and keep a copy on your computer; a fix is on its
way ([disc keys](wii-u.md#disc-keys)).

### Switch: games are counted but not listed {#switch-not-listed}

Usually the keys: without working `prod.keys` the side can't read a game. Check the Setup check's
**Switch keys** row. With firmware 23 and its keys, this is a known problem being looked at.

## Games that won't start

### Switch: error 59, or "Loader status 28" {#switch-error-59}

Your keys and firmware don't match: keys from an older system version than the firmware. Dump **both**
again from your own Switch, at the same time ([keys and firmware](switch.md#keys-and-firmware)).

### Breath of the Wild asks for a system update {#botw-update}

Install the game's own update (v1.6.0 or later), from **Settings > Game files > Updates, DLC and mods**
([Wii U updates](wii-u.md#updates)).

### Wii games for the Wii U don't start {#wii-on-wii-u}

Cemu doesn't play Wii games, the Wii U eShop's included. Play them on the
[GameCube and Wii](gamecube-wii.md) side.

### Switch: a black screen from a USB drive {#switch-usb}

On some consoles, Switch games on a USB drive or in a folder of your own go to a black screen, where
the same file plays from `/data/eden/roms`. Play from there for now.

### Nothing starts after rest mode {#rest-mode}

Close Playtendoh and start it again.

## Speed and crashes

### Wii U games are very slow {#slow}

The Setup check's **Game speed** row, or **Settings > Diagnostics**, probably says
`JIT unavailable (interpreter only)`. Put `PPSA99360` on your HEN's app jailbreak list
([game speed](jailbreak.md#game-speed)).

### Games on a USB drive "cannot be read" {#usb-cannot-be-read}

Your HEN opened `/data` but not the drives. Make sure an ELF loader is running on port 9021, and start
the app again: it opens the drive through it.

### One game crashes or freezes {#one-game}

Look the game up on the [compatibility list](../compatibility.html): someone may have found the
setting that fixes it. If not, report it with its logs ([Getting help](help.md)).

### The app freezes looking through a network share {#network-share}

A share that holds ES-DE's `covers`, `manuals` or `videos` folders beside the games can make the search
never end. Point the share at a folder that holds only games.
