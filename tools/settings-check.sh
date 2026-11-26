#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Every console's Settings, as a press uses them, on the PC (tools/launcher-preview/settings_check.cpp
# says what is checked): each row with a value changes, is kept for the next start and touches no other
# console's settings, and so does each game's own. Prints what fails; exits 1 when anything does. Run it
# after a change to Settings' pages or rows, or to how the settings are kept.
#
#   tools/settings-check.sh
#
# Built from the PC preview's files (tools/preview-shell.sh's list), so it needs what that needs. With
# bubblewrap (bwrap) it runs in a sandbox whose /data is its own, empty, so Porpoise's rows, which write
# /data/porpoise, are checked too; without it they are left out and nothing in /data is touched.

set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"
cd "$PS5CEMU_ROOT"
out=$PS5CEMU_BUILD/settings-check
mkdir -p "$out/include" "$out/obj" "$out/data"
ln -sfn "$PS5CEMU_PACBREW/include/fmt" "$out/include/fmt"

# the preview's files but its own main (tools/launcher-preview/shell.cpp's, renamed)
read -r -a sources < <(python3 - <<'PY'
import re
script = open('tools/preview-shell.sh').read()
print(' '.join(re.search(r'^sources=\((.*?)\)', script, re.S | re.M).group(1).split()))
PY
)
version=$(sed -n 's/^VERSION := *\([^ ]*\).*/\1/p' Makefile)
flags=(-std=c++20 -O1 -w -DFMT_HEADER_ONLY -DPS5CEMU_LAUNCHER_PREVIEW "-DPS5CEMU_VERSION=\"$version\"" "-DPS5CEMU_DATA=\"$out/data\""
    "-DPLAYTENDOH_REPORT_URL=\"https://reports.example.invalid/v1/report\"" -I "$out/include" -I "$PS5CEMU_SYSROOT/include" -I port -I port/app
    -I tools/launcher-preview -I "$PS5CEMU_ROOT/.deps/reshade/deps/stb" $(pkg-config --cflags freetype2))
objects=()
stale=()
for source in "${sources[@]}" tools/launcher-preview/settings_check.cpp; do
    object=$out/obj/$(echo "${source%.cpp}" | tr / _).o
    objects+=("$object")
    [[ -f $object && $object -nt $source ]] || stale+=("$source:$object")
done
if ((${#stale[@]})); then
    printf '%s\n' "${stale[@]}" | xargs -P "$JOBS" -I{} bash -c 'source=${1%%:*}; object=${1#*:}; shift
        extra=(); [[ $source == tools/launcher-preview/shell.cpp ]] && extra=(-Dmain=preview_main)
        clang++-18 "$@" "${extra[@]}" -c "$source" -o "$object"' _ {} "${flags[@]}"
fi
if ((${#stale[@]})) || [[ ! -x $out/settings-check ]]; then
    clang++-18 "${objects[@]}" -lvulkan -lfreetype -lz -lzstd -lpthread -o "$out/settings-check"
fi

if command -v bwrap >/dev/null; then
    # a root of its own: the system's folders read-only, the repository and the PC's temporary files as
    # they are, and an empty /data
    sandbox=(--tmpfs / --ro-bind /usr /usr --ro-bind /etc /etc --proc /proc --dev /dev --bind /tmp /tmp --dir /data)
    # the folders that are links on some systems (/home to var/home, /lib to usr/lib) first, so what is
    # bound below them goes where the links lead
    for folder in /bin /sbin /lib /lib64 /lib32 /home; do
        if [[ -L $folder ]]; then sandbox+=(--symlink "$(readlink "$folder")" "$folder")
        elif [[ -d $folder && $folder != /home ]]; then sandbox+=(--ro-bind "$folder" "$folder"); fi
    done
    root=$(readlink -f "$PS5CEMU_ROOT") # at its real place: the links above lead to it
    sandbox+=(--bind "$root" "$root" --chdir "$root" --setenv SETTINGS_CHECK_SANDBOX 1)
    # .deps may be a link to a checkout elsewhere: there too, read-only
    deps=$(readlink -f "$PS5CEMU_ROOT/.deps" 2>/dev/null || true)
    [[ -n $deps && $deps != "$PS5CEMU_ROOT"/* ]] && sandbox+=(--ro-bind "$deps" "$deps")
    bwrap "${sandbox[@]}" "$out/settings-check" 2>"$out/settings-check.log"
else
    "$out/settings-check" 2>"$out/settings-check.log"
fi
