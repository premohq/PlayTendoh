#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# A folder that stands for a console's / for the launcher's preview (PREVIEW_CONSOLE, tools/preview-shell.sh)
# and the discovery's test (tools/tests/test-discovery.sh): a USB drive and /data with games, BIOS files,
# Switch keys and a firmware put where a newcomer might put them, each file only as long as its header needs
# to be told apart (app/discovery.h), and a few that look like games and are not.
#
#   make-console.py FOLDER [--empty]    --empty: a USB drive and /data with nothing in them

import os
import struct
import sys

root = sys.argv[1]
if "--empty" in sys.argv[2:]:
    os.makedirs(os.path.join(root, "mnt/usb0"), exist_ok=True)
    os.makedirs(os.path.join(root, "data/homebrew"), exist_ok=True)
    sys.exit(0)


def write(path, data=b""):
    path = os.path.join(root, path)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def at(offset, magic, size=0x400):
    data = bytearray(max(size, offset + len(magic)))
    data[offset:offset + len(magic)] = magic
    return bytes(data)


nes = b"NES\x1a" + bytes(0x200)
gba = at(4, bytes.fromhex("24ffae51"), 0x200)
gb = at(0x104, bytes.fromhex("ceed6666"), 0x200)
n64 = bytes.fromhex("80371240") + bytes(0x100)
nds = at(0x15C, bytes.fromhex("56cf"), 0x200)
n3ds = at(0x100, b"NCSD", 0x200)
nsp = b"PFS0" + bytes(0x100)

# a USB drive as many arrive: a front end's roms folder, a folder per console
for name in ("Super Mario World (USA).sfc", "The Legend of Zelda - A Link to the Past (USA).sfc", "Super Metroid (USA).sfc"):
    write(f"mnt/usb0/roms/snes/{name}", bytes(0x200))
for name in ("Super Mario Bros. (World).nes", "Metroid (USA).nes"):
    write(f"mnt/usb0/roms/nes/{name}", nes)
for name in ("Metroid Fusion (USA).gba", "Advance Wars (USA).gba"):
    write(f"mnt/usb0/roms/gba/{name}", gba)
write("mnt/usb0/roms/gb/Tetris (World).gb", gb)
# a NES game in a .zip, among the SNES games: its first file says which it is
inner = b"Kirby's Adventure (USA).nes"
write("mnt/usb0/roms/snes/Kirby's Adventure (USA).zip", b"PK\x03\x04" + bytes(22) + struct.pack("<HH", len(inner), 0) + inner)
# its covers, which hold no games and are not looked into
write("mnt/usb0/roms/snes/media/covers/Super Metroid (USA).png", b"\x89PNG")
# Wii U games unpacked, one an update (left out)
for title, game_id in (("Mario Kart 8 [AMKP01]", "000500001010ec00"), ("Mario Kart 8 Update v64", "0005000e1010ec00")):
    base = f"mnt/usb0/Wii U/{title}"
    write(f"{base}/code/app.xml", b"<app/>")
    write(f"{base}/content/data.bin", b"")
    write(f"{base}/meta/meta.xml", f"<menu><title_id type=\"hexBinary\" length=\"8\">{game_id}</title_id></menu>".encode())
# Switch games, an update beside them, and the keys and firmware from the player's console
write("mnt/usb0/switch/Super Mario Odyssey [0100000000010000][v0].nsp", nsp)
write("mnt/usb0/switch/Super Mario Odyssey [0100000000010800][v65536].nsp", nsp)
write("mnt/usb0/switch/Metroid Dread [010093801237C000].nsp", nsp)
write("mnt/usb0/switch/prod.keys", b"header_key = " + b"0" * 64 + b"\n" + b"".join(b"master_key_%02x = " % i + b"1" * 32 + b"\n" for i in range(0x13)))
write("mnt/usb0/switch/title.keys", b"01000000000100000000000000000000 = " + b"2" * 32 + b"\n")
for i in range(64):
    write(f"mnt/usb0/switch/firmware/{i:032x}.nca", bytes(64))
# a RetroArch system folder's BIOS files: the GBA's and the DS's at the sizes their emulators take
write("mnt/usb0/system/gba_bios.bin", bytes(16384))
write("mnt/usb0/system/bios7.bin", bytes(16384))
write("mnt/usb0/system/bios9.bin", bytes(4096))
write("mnt/usb0/system/scph1001.bin", bytes(524288))  # another console's, not Nintendo's
# a 3D Studio model that is no 3DS game, and a 3DS game
write("mnt/usb0/models/ship.3ds", b"MM" + bytes(0x200))
write("mnt/usb0/3ds/Super Mario 3D Land.3ds", n3ds)

# /data: the app's own folder (an app, not looked into), games put in /data/homebrew and the drop folder
write("data/homebrew/PPSA99360/eboot.bin", b"\x7fELF")
write("data/homebrew/PPSA99360/sce_sys/param.json", b"{}")
write("data/homebrew/PPSA99360/assets/game.nes", nes)
write("data/homebrew/roms/Super Mario 64 (USA).z64", n64)
write("data/playtendoh/Mario Kart DS (USA).nds", nds)
write("data/playtendoh/New Super Mario Bros. (USA).nds", nds)
