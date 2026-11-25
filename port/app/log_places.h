// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: where every emulator in the app keeps logs of its own, in one table, for both places logs
// are gathered: Settings > Diagnostics > Copy logs to USB (frontend/actions.cpp) and a game's report
// (app/reports.h, its Session::logs). Each had a list of its own, and each missed some: Lichii's logs on
// the Switch 2, Eden's core log, Porpoise's debug folder, EmulationStation's, the N64's and the NES's own
// programs. melonDS and the libretro cores write into the boot log, which both take whole.
//
// The places come from Roots, which frontend/actions.cpp fills from the app's paths (LogRoots), and
// tools/tests/test-log-places.sh from a console's layout made on a PC. C++17, header only.
#pragma once
#include "own_status.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace ps5logplaces
{
	// The folders the places are in, as the app has them
	struct Roots
	{
		std::string logs;	   // the launcher's: boot logs, and Cemu's log of each session (ps5paths::Logs)
		std::string cemu;	   // Cemu's data folder: its log.txt
		std::string azahar;	   // Azahar's: log/
		std::string eden;	   // the Switch's program: logs/ and its core's user/log/
		std::string eglantine; // the Switch 2's (Lichii), laid out as Eden's
		std::string porpoise;  // the GameCube and Wii's data folder: logs/ and debug/
		std::string esde;	   // EmulationStation's home: its own log and ES-DE's
		std::string sandbox;   // the app's download0: what Eden, Lichii and Porpoise log before they reach /data
		// the programs of their own the other sides start (the N64's and the NES's), by name: their data folders
		std::vector<std::pair<std::string, std::string>> programs;
	};

	// A file, or a folder: all of it (an emulator's logs folder), else the logs in it three deep
	// (IsLogName), past the folders that hold games
	struct Place
	{
		std::string path;
		std::string tag; // what a copy's name starts with on a USB drive ("eden-")
		bool all = false;
	};

	// A log file's name: .log, .txt or .tsv with log, crash, trace, hang, thread, stop-limit or stderr in it:
	// the hang, thread and stop reports Eden and Lichii leave in logs/ have no "log" in their names
	inline bool IsLogName(std::string name)
	{
		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		auto ends = [&](const char* end) { const std::string e = end; return name.size() > e.size() && name.compare(name.size() - e.size(), e.size(), e) == 0; };
		if (!ends(".log") && !ends(".txt") && !ends(".tsv"))
			return false;
		for (const char* word : {"log", "crash", "trace", "hang", "thread", "stop-limit", "stderr"})
			if (name.find(word) != std::string::npos)
				return true;
		return false;
	}

	// a log that says what went wrong, which goes first: a crash's or a hang's
	inline bool IsProblemLog(std::string name)
	{
		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return name.find("crash") != std::string::npos || name.find("hang") != std::string::npos;
	}

	// the folders that hold no logs and can hold thousands of files: a library of games, or a front end's
	// gamelists and media, used up the walk before it came to the logs
	inline bool HoldsNoLogs(std::string name)
	{
		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		for (const char* folder : {"games", "roms", "covers", "firmware", "gamelists", "downloaded_media", "themes", "shaders", "shadercache",
				 "nand", "sdmc", "saves", "mlc01"})
			if (name == folder)
				return true;
		return false;
	}

	// Each emulator's places, by the name a report gives it: "cemu", "azahar", "eden", "eglantine",
	// "porpoise", "esde", or one of Roots::programs. melonDS and the libretro cores have none: they log
	// into the boot log
	inline std::vector<Place> For(const std::string& emulator, const Roots& r)
	{
		if (emulator == "cemu")
			return {{r.logs + "/cemu.prev.txt", "cemu-"}, {r.cemu + "/log.txt", "cemu-"}};
		if (emulator == "azahar")
			return {{r.azahar + "/log", "azahar-", true}};
		if (emulator == "eden" || emulator == "eglantine")
		{
			// logs/ (stderr, the crash, hang, thread and stop reports, the boot trace), the core's own log in
			// user/log/, and what the program keeps in the app's sandbox until it reaches /data
			const std::string data = emulator == "eden" ? r.eden : r.eglantine, tag = emulator + "-";
			return {{data + "/logs", tag, true}, {data + "/user/log", tag + "core-", true}, {r.sandbox + "/boot-trace.txt", "sandbox-"},
				{r.sandbox + "/boot-trace.prev.txt", "sandbox-"}, {r.sandbox + "/eden-headless-g7", "sandbox-", true}};
		}
		if (emulator == "porpoise")
			// its trace and Dolphin's log; its debug folder, there while Settings' Debug logs is on, all of it
			return {{r.porpoise + "/logs", "porpoise-", true}, {r.porpoise + "/debug", "porpoise-debug-", true}, {r.sandbox + "/porpoise-trace.txt", "sandbox-"}};
		if (emulator == "esde")
			return {{r.esde, "esde-"}};
		for (const auto& [name, data] : r.programs)
			if (emulator == name)
				return {{data, name + "-"}};
		return {};
	}

	// Every emulator with places of its own
	inline std::vector<std::string> Emulators(const Roots& r)
	{
		std::vector<std::string> names{"cemu", "azahar", "eden", "eglantine", "porpoise", "esde"};
		for (const auto& program : r.programs)
			names.push_back(program.first);
		return names;
	}

	struct Found
	{
		std::string path;
		std::string copy; // its name on a USB drive: the place's tag, then its path in the place's folder with - for /
		bool problem = false;
	};

	namespace detail
	{
		inline void Walk(const std::string& folder, const std::string& tag, bool all, int depth, int& budget, std::vector<Found>& found)
		{
			std::error_code ec;
			for (std::filesystem::directory_iterator it(folder, ec), end; !ec && it != end && budget > 0; it.increment(ec))
			{
				budget--;
				const std::string path = it->path().string(), name = it->path().filename().string();
				// a link's own status (a linked folder is not walked into), where /data refuses lstat
				std::error_code type;
				const std::filesystem::file_status status = ps5files::OwnStatus(path, type);
				if (std::filesystem::is_directory(status))
				{
					// a logs folder's are logs too, whatever their names
					if (depth > 1 && (all || !HoldsNoLogs(name)))
						Walk(path, tag + name + "-", all, depth - 1, budget, found);
					continue;
				}
				if (std::filesystem::is_regular_file(status) && (all || IsLogName(name)))
					found.push_back({path, tag + name, IsProblemLog(name)});
			}
		}
	}

	// The log files in places, each once (a place's file, or what a folder holds), at most budget entries
	// looked at in all
	inline std::vector<Found> Find(const std::vector<Place>& places, int budget = 4000)
	{
		std::vector<Found> found;
		for (const Place& place : places)
		{
			std::error_code ec;
			const std::filesystem::file_status status = std::filesystem::status(place.path, ec);
			if (ec)
				continue;
			const size_t before = found.size();
			if (std::filesystem::is_directory(status))
				detail::Walk(place.path, place.tag, place.all, 3, budget, found);
			else if (std::filesystem::is_regular_file(status))
			{
				const std::string name = std::filesystem::path(place.path).filename().string();
				found.push_back({place.path, place.tag + name, IsProblemLog(name)});
			}
			// a file two places reach (the sandbox's, Eden's and Lichii's alike) once
			found.erase(std::remove_if(found.begin() + (std::ptrdiff_t)before, found.end(),
							[&](const Found& one) {
								return std::any_of(found.begin(), found.begin() + (std::ptrdiff_t)before, [&](const Found& other) { return other.path == one.path; });
							}),
				found.end());
		}
		return found;
	}
}
