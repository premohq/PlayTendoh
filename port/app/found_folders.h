// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: the folders Playtendoh found a system's games in by itself (app/discovery.h), which its
// library looks through beside its own games folder: games are played where they are, nothing is moved.
// Set by the launcher (ps5settings::ApplyFound) only for a side whose games folder is still its own one: a
// folder the player chose in Settings > Game files is the side's only folder, as before. By the side's name
// in ps5cemu.json ("wiiu", "3ds", "snes"). C++17, as melonDS's build includes it.
#pragma once
#include <mutex>
#include <string>
#include <vector>

namespace ps5found
{
	namespace detail
	{
		struct Side
		{
			std::string side;
			std::vector<std::string> folders;
		};
		inline std::mutex& Mutex()
		{
			static std::mutex mutex;
			return mutex;
		}
		inline std::vector<Side>& Sides()
		{
			static std::vector<Side> sides;
			return sides;
		}
		inline bool Inside(const std::string& path, const std::string& folder)
		{
			return path.compare(0, folder.size(), folder) == 0 && (path.size() == folder.size() || path[folder.size()] == '/');
		}
	}

	// a side's found folders, replacing those it had (none: it looks in its own folder only)
	inline void Set(const std::string& side, const std::vector<std::string>& folders)
	{
		std::lock_guard<std::mutex> lock(detail::Mutex());
		for (detail::Side& known : detail::Sides())
			if (known.side == side)
			{
				known.folders = folders;
				return;
			}
		detail::Sides().push_back({side, folders});
	}

	inline std::vector<std::string> Of(const std::string& side)
	{
		std::lock_guard<std::mutex> lock(detail::Mutex());
		for (const detail::Side& known : detail::Sides())
			if (known.side == side)
				return known.folders;
		return {};
	}

	// What a side's scan looks through: its own folder first, then the found ones, each once and none
	// that the own folder holds already
	inline std::vector<std::string> With(const std::string& side, const std::string& folder)
	{
		std::vector<std::string> folders{folder};
		for (const std::string& found : Of(side))
		{
			bool covered = false;
			for (const std::string& kept : folders)
				covered = covered || detail::Inside(found, kept);
			if (!covered)
				folders.push_back(found);
		}
		return folders;
	}

	// Whether a folder is one Playtendoh found rather than the side's own: there a game in an archive whose
	// first file is not the side's is left to the side it is for (a mixed folder of every console's .zip files)
	inline bool IsFound(const std::string& side, const std::string& folder)
	{
		for (const std::string& found : Of(side))
			if (detail::Inside(folder, found))
				return true;
		return false;
	}
}
