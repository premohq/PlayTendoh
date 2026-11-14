#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the real artwork queue/path helpers without starting network workers."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'port/app/boxart.cpp').read_text()


def function(signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


# Glass and the two case primitives must keep distinct CPU/GPU identifiers.
header = (root / 'port/ui/gfx.h').read_text().split('enum Kind', 1)[1].split('};', 1)[0]
kinds = dict((name, int(value)) for name, value in re.findall(r'(k\w+)\s*=\s*(\d+)', header))
assert len(set(kinds.values())) == len(kinds)
shader = (root / 'port/ui/shaders/ui.frag').read_text()
for name in ('kFrost', 'kImageQuad', 'kEllipse'):
    assert shader.count(f'kind == {kinds[name]}') == 1

helpers = '\n'.join(function(signature) for signature in (
    'std::string Base(', 'std::string_view Bare(', 'bool Here(', 'bool Valid(',
    'std::string MediaPath(', 'std::string Path(', 'void Fetch(', 'void FetchMedia(', 'void Refetch('))
harness = r'''
#include "app/boxart.h"
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <thread>
#include <fmt/format.h>
namespace fs = std::filesystem;
static std::string directory;
namespace ps5paths { const std::string& Covers() { return directory; } }
namespace ps5log { template<class... T> void Line(const char*, const T&...) {} }
namespace ps5boxart {
constexpr std::string_view kMedia = "media/", kBack = "back/";
static std::mutex s_mutex;
static std::condition_variable s_wake;
static std::deque<std::pair<System, std::string>> s_queue;
static std::set<std::string> s_queued;
static bool s_enabled = true, s_stopped = false, s_started = true, s_retryNow = false;
static int s_answered;
static void Worker() { assert(false && "test must not start a network worker"); }
static const char* Folder(System) { return "wiiu"; }
static bool RecentlyNone(System, const std::string&) { return false; }
HELPERS
}
int main(int argc, char** argv) {
    assert(argc == 2); directory = argv[1];
    using namespace ps5boxart;
    const auto system = System::WiiU;
    assert(Bare("back/ALZE01") == "ALZE01");
    bool media = false;
    assert(Bare("media/ALZE01", &media) == "ALZE01" && media);
    const std::string front = Base(system, "ALZE01") + ".tga";
    const std::string back = Base(system, "back/ALZE01") + ".tga";
    const std::string disc = Base(system, "media/ALZE01") + ".tga";
    fs::create_directories(fs::path(disc).parent_path());
    std::ofstream(front).put('f'); std::ofstream(back).put('b'); std::ofstream(disc).put('d');
    assert(Path(system, "ALZE01") == front);
    assert(Path(system, "ALZE01", Art::Back) == back);
    assert(Path(system, "ALZE01", Art::Disc) == disc);
    assert(Path(system, "../bad", Art::Back).empty());
    assert(Path(system, "../bad", Art::Disc).empty());
    // The front was fetched earlier; only extra artwork is currently queued.
    s_queued.insert("wiiu/ALZE01");
    s_queue.emplace_back(system, "back/ALZE01");
    s_queue.emplace_back(system, "media/ALZE01");
    Refetch(system, {"ALZE01"}, true);
    assert(s_queue.size() == 3 && s_queue.back().second == "ALZE01");
    assert(!fs::exists(front) && fs::exists(back) && fs::exists(disc));
    assert(s_retryNow && s_answered == -1);
    fs::remove(back); fs::remove(disc);
    s_queue.clear(); s_queued.clear();
    Fetch(system, {"ALZE01", "../bad"}, Art::Back);
    Fetch(system, {"ALZE01"}, Art::Back);
    Fetch(system, {"ALZE01"}, Art::Disc);
    FetchMedia(system, {"ALZE01"});
    assert(s_queue.size() == 2);
    assert(s_queue.front().second == "back/ALZE01" && s_queue.back().second == "media/ALZE01");
    s_enabled = false;
    Fetch(system, {"AMCE01"}, Art::Back); Fetch(system, {"AMCE01"}, Art::Disc);
    assert(s_queue.size() == 2);
}
'''.replace('HELPERS', helpers)
with tempfile.TemporaryDirectory(prefix='playtendoh-case-art-') as directory:
    work = Path(directory)
    (work / 'test.cpp').write_text(harness)
    subprocess.run(shlex.split(os.environ.get('CXX', 'c++')) + [
        '-std=c++20', '-Wall', '-Wextra', '-Werror', '-DFMT_HEADER_ONLY',
        '-I' + str(root / 'port'), str(work / 'test.cpp'), '-pthread', '-o', str(work / 'test')], check=True)
    subprocess.run([str(work / 'test'), str(work / 'files')], check=True)
print('PASS artwork paths, extra-art deduplication, front refetch, disabled downloads and shader-kind separation')
