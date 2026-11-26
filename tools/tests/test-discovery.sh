#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Finding a player's games and files (port/app/discovery.cpp) in a folder standing for a console's /, made by
# tools/launcher-preview/make-console.py: tools/tests/test-discovery.cpp. Prints PASS lines; exits 1 on a failure.
set -euo pipefail
root=$(cd "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
sanitize=()
if [[ ${ECAM_TEST_SANITIZE:-0} == 1 ]]; then sanitize=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
"${CXX:-c++}" "${sanitize[@]}" -std=c++20 -pthread -Wall -Wextra -Werror -Wno-deprecated-declarations -DFMT_HEADER_ONLY \
 -DPS5CEMU_DATA="\"$work/console/data/ps5cemu\"" -I"$root/port" -I"${ECAM_RAPIDJSON_INCLUDE:-$root/.deps/rapidjson/include}" \
 "$root/tools/tests/test-discovery.cpp" "$root/port/app/discovery.cpp" "$root/port/retro/library.cpp" \
 "$root/port/frontend/settings.cpp" -o "$work/test-discovery"
python3 -B "$root/tools/launcher-preview/make-console.py" "$work/console"
"$work/test-discovery" "$work/console"
# and with lstat refused, as /data is in an app without a HEN (port/app/own_status.h)
if [[ $(uname -s) == Linux && ${#sanitize[@]} == 0 ]]; then
    "${CC:-cc}" -shared -fPIC "$root/tools/tests/refuse-lstat.c" -o "$work/refuse-lstat.so" -ldl
    rm -rf "$work/console" && python3 -B "$root/tools/launcher-preview/make-console.py" "$work/console"
    echo "lstat refused with EPERM, as /data is in an app without a HEN:"
    REFUSE_LSTAT_UNDER="$work" LD_PRELOAD="$work/refuse-lstat.so" "$work/test-discovery" "$work/console"
fi
