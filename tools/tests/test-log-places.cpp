// SPDX-License-Identifier: GPL-3.0-or-later
// Every emulator's logs found (port/app/log_places.h), on a console's layout made on a PC: each program's
// own log files, by the names they write (Cemu's log.txt, Azahar's log/, Eden's and Lichii's logs/ and their
// core's user/log/, Porpoise's logs/ and debug/, EmulationStation's, the N64's and the NES's programs', and
// what they leave in the app's sandbox), beside thousands of games, gamelists and covers that are not
// logs, which must not use up the walk.
#include "app/log_places.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
static int failures = 0;
static void Check(bool ok, const std::string& what)
{
	std::cout << (ok ? "PASS  " : "FAIL  ") << what << '\n';
	failures += !ok;
}
static void Put(const fs::path& file, const std::string& text = "a line\n")
{
	fs::create_directories(file.parent_path());
	std::ofstream(file, std::ios::binary) << text;
}
static void Many(const fs::path& folder, int count, const char* extension)
{
	for (int i = 0; i < count; i++)
		Put(folder / ("Game " + std::to_string(i) + extension), "");
}

int main(int argc, char** argv)
{
	if (argc != 2)
		return 2;
	const fs::path root = argv[1];
	ps5logplaces::Roots r;
	r.logs = (root / "ps5cemu/logs").string();
	r.cemu = (root / "cemu").string();
	r.azahar = (root / "azahar").string();
	r.eden = (root / "eden").string();
	r.eglantine = (root / "eglantineps5").string();
	r.porpoise = (root / "porpoise").string();
	r.esde = (root / "ps5cemu/es-de").string();
	r.sandbox = (root / "download0").string();
	r.programs = {{"mupen64plus", (root / "mupen64plus").string()}, {"fceumm", (root / "fceumm").string()}};

	// each program's logs, as it names them, by emulator
	const std::vector<std::pair<std::string, std::vector<std::string>>> logs = {
		{"cemu", {"ps5cemu/logs/cemu.prev.txt", "cemu/log.txt"}},
		{"azahar", {"azahar/log/azahar_log.txt", "azahar/log/azahar_log.old.txt"}},
		{"eden", {"eden/logs/stderr.log", "eden/logs/stderr.prev.log", "eden/logs/crash-20261124-101010.txt", "eden/logs/crash-index.txt",
					 "eden/logs/crash-note.txt", "eden/logs/hang-20261124-101010.txt", "eden/logs/threads-20261124-101010.txt",
					 "eden/logs/stop-limit.txt", "eden/logs/boot-trace.txt", "eden/logs/framegen.txt", "eden/logs/heap.log",
					 "eden/user/log/eden_log.txt", "eden/user/log/eden_log.txt.old.txt", "download0/boot-trace.txt",
					 "download0/boot-trace.prev.txt", "download0/eden-headless-g7/stderr.log"}},
		{"eglantine", {"eglantineps5/logs/boot-trace.txt", "eglantineps5/logs/boot-trace.sandbox-prev.txt", "eglantineps5/logs/crash-20261123-090000.txt",
						  "eglantineps5/logs/mkw-transcode-42.log", "eglantineps5/logs/mkw-image-alloc-42.tsv", "eglantineps5/logs/stderr.log",
						  "eglantineps5/user/log/eden_log.txt", "download0/boot-trace.txt", "download0/boot-trace.prev.txt",
						  "download0/eden-headless-g7/stderr.log"}},
		{"porpoise", {"porpoise/logs/trace.txt", "porpoise/logs/trace.prev.txt", "porpoise/logs/core.log", "porpoise/logs/core.prev.log",
						 "porpoise/debug/app.log", "porpoise/debug/crash.txt", "porpoise/debug/games/GALE01.log", "download0/porpoise-trace.txt"}},
		{"esde", {"ps5cemu/es-de/es-de-ps5.log", "ps5cemu/es-de/es-de-ps5.1.log", "ps5cemu/es-de/ES-DE/logs/es_log.txt"}},
		{"mupen64plus", {"mupen64plus/logs/mupen64plus.log"}},
		{"fceumm", {"fceumm/fceumm.log"}},
	};
	for (const auto& [emulator, files] : logs)
		for (const std::string& file : files)
			Put(root / file);

	// what is beside them and no log: games, gamelists, covers, firmware, saves, settings
	Many(root / "eden/roms", 3000, ".nsp");
	Many(root / "eglantineps5/games", 3000, ".nup");
	Many(root / "eglantineps5/firmware", 500, ".nca");
	Many(root / "porpoise/games/GameCube", 1500, ".iso");
	Many(root / "mupen64plus/games", 3000, ".z64");
	for (int system = 0; system < 24; system++)
		Many(root / ("ps5cemu/es-de/gamelists/system" + std::to_string(system)), 100, ".xml");
	Many(root / "ps5cemu/es-de/downloaded_media/nes/covers", 2000, ".png");
	Put(root / "cemu/settings.xml");
	Put(root / "azahar/sdmc/Nintendo 3DS/save.bin");
	Put(root / "fceumm/saves/game.srm");

	std::set<std::string> copies;
	for (const std::string& emulator : ps5logplaces::Emulators(r))
	{
		const auto found = ps5logplaces::Find(ps5logplaces::For(emulator, r));
		std::set<std::string> paths;
		for (const auto& one : found)
		{
			paths.insert(fs::relative(one.path, root).generic_string());
			copies.insert(one.copy);
		}
		const auto wanted = std::find_if(logs.begin(), logs.end(), [&](const auto& entry) { return entry.first == emulator; });
		Check(wanted != logs.end(), emulator + ": a log of its own in the test");
		if (wanted == logs.end())
			continue;
		std::string missing;
		for (const std::string& file : wanted->second)
			if (!paths.count(file))
				missing += " " + file;
		Check(missing.empty(), emulator + ": every log of its own found" + (missing.empty() ? std::string() : ":" + missing + " missing"));
		std::string extra;
		for (const std::string& path : paths)
			if (std::find(wanted->second.begin(), wanted->second.end(), path) == wanted->second.end())
				extra += " " + path;
		Check(extra.empty(), emulator + ": nothing else (games, covers, settings)" + (extra.empty() ? std::string() : ":" + extra));
		Check(std::none_of(found.begin(), found.end(), [](const auto& one) { return one.copy.find('/') != std::string::npos; }),
			emulator + ": each copy's name a file's, with no folder in it");
	}
	// a problem's log goes first in a report
	const auto eden = ps5logplaces::Find(ps5logplaces::For("eden", r));
	Check(std::count_if(eden.begin(), eden.end(), [](const auto& one) { return one.problem; }) == 4, "Eden's crash and hang reports are told apart (four)");
	// Copy logs to USB takes every emulator's: each file once, under a name of its own
	std::vector<ps5logplaces::Place> all;
	for (const std::string& emulator : ps5logplaces::Emulators(r))
		for (const auto& place : ps5logplaces::For(emulator, r))
			all.push_back(place);
	const auto everything = ps5logplaces::Find(all);
	std::set<std::string> paths, names;
	for (const auto& one : everything)
	{
		paths.insert(one.path);
		names.insert(one.copy);
	}
	size_t expected = 0;
	std::set<std::string> distinct;
	for (const auto& entry : logs)
		for (const std::string& file : entry.second)
			distinct.insert(file);
	expected = distinct.size();
	Check(everything.size() == expected && paths.size() == expected, "every emulator's logs in one walk, each once (" + std::to_string(everything.size()) + " of " +
																		  std::to_string(expected) + ")");
	Check(names.size() == everything.size(), "each with a name of its own on the drive");
	// melonDS and the cores: the boot log, nothing of their own
	Check(ps5logplaces::For("melonds", r).empty() && ps5logplaces::For("mgba", r).empty(), "melonDS and the libretro cores write into the boot log");
	Check(ps5logplaces::IsLogName("stop-limit.txt") && ps5logplaces::IsLogName("threads-1.txt") && ps5logplaces::IsLogName("stderr.log") &&
			  !ps5logplaces::IsLogName("settings.xml") && !ps5logplaces::IsLogName("dialog.png"),
		"log names: the reports' words, and no picture or settings");
	return failures ? 1 : 0;
}
