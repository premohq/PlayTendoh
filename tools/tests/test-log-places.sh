#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Every emulator's logs found, for Copy logs to USB and the reports (port/app/log_places.h), on a console's
# layout made on a PC, with lstat refused as /data does without a HEN too.
set -euo pipefail
root=$(cd "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
"${CXX:-c++}" -std=c++20 -Wall -Wextra -Werror -I"$root/port" "$root/tools/tests/test-log-places.cpp" -o "$work/test-log-places"
"$work/test-log-places" "$work/console"
if [[ $(uname -s) == Linux ]]; then
    "${CC:-cc}" -shared -fPIC "$root/tools/tests/refuse-lstat.c" -o "$work/refuse-lstat.so" -ldl
    echo "lstat refused with EPERM, as /data is in an app without a HEN:"
    REFUSE_LSTAT_UNDER="$work" LD_PRELOAD="$work/refuse-lstat.so" "$work/test-log-places" "$work/refused"
fi
