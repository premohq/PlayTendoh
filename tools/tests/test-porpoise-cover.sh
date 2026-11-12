#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# The cover Porpoise X's in-game menu shows, given it from the launcher's (port/retro/porpoise_banner.cpp).
# Needs a C++20 compiler and the fmt headers lang.h's log takes (pacbrew's, as `make deps` fetches them). Prints
# PASS lines; exits 1 on a failure.
set -euo pipefail
root=$(cd "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fmt=$(ls -d "$root"/.deps/pacbrew-*/opt/ps5-payload-sdk/target/user/homebrew/include 2>/dev/null | head -1)
"${CXX:-c++}" -std=c++20 -Wall -Wextra -fsanitize=address,undefined -DFMT_HEADER_ONLY -I"$root/port" ${fmt:+-isystem "$fmt"} \
    -I"$root/.deps/reshade/deps/stb" "$root/tools/tests/test-porpoise-cover.cpp" "$root/port/retro/porpoise_banner.cpp" \
    "$root/port/retro/porpoise_disc.cpp" -o "$work/test"
mkdir "$work/files"
"$work/test" "$work/files"
