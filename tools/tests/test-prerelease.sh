#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# A pre-release's watermark (port/app/prerelease.h, tools/tests/test-prerelease.cpp): built as a release, then
# as pre-release C, with the string tables' code (port/app/lang.cpp) in English. Needs clang-18 (or g++), and
# pacbrew's fmt and RapidJSON (tools/deps.py fetch pacbrew rapidjson).
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/../env.sh"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
compiler=${CXX:-$(command -v clang++-18 || command -v clang++ || command -v g++)}
mkdir -p "$work/include"
ln -s "$PS5CEMU_PACBREW/include/fmt" "$work/include/fmt"
flags=(-std=c++20 -O1 -DFMT_HEADER_ONLY -DPS5CEMU_LAUNCHER_PREVIEW '-DPS5CEMU_VERSION="6.0.0"' "-DPS5CEMU_DATA=\"$work/data\""
    -I "$work/include" -I "$PS5CEMU_ROOT/.deps/rapidjson/include" -I "$PS5CEMU_ROOT/port" -I "$PS5CEMU_ROOT/port/app" -I "$PS5CEMU_ROOT/tools/launcher-preview")
sources=("$PS5CEMU_ROOT/tools/tests/test-prerelease.cpp" "$PS5CEMU_ROOT/port/app/prerelease.cpp" "$PS5CEMU_ROOT/port/app/lang.cpp")
"$compiler" "${flags[@]}" "${sources[@]}" -o "$work/release"
"$work/release"
"$compiler" "${flags[@]}" -DPLAYTENDOH_PRERELEASE=1 '-DPLAYTENDOH_PRERELEASE_LETTER="C"' "${sources[@]}" -o "$work/prerelease"
"$work/prerelease"
