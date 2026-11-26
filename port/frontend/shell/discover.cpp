// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: the launcher's side of finding the player's games and files (app/discovery.h). A look runs on
// the first start while its questions are answered, at each fresh start once the side's own games are
// listed, whenever a drive is plugged in, and from the Setup check's Look now; never while a game runs (the
// launch stops it). What it finds:
//  - a system's games in a folder that is not the side's own become one its library looks through too
//    (ui.found, app/found_folders.h), unless the player chose the side's folder in Settings > Game files;
//    the side is shown in the bar, unless the player hid it;
//  - BIOS files, Switch keys and firmware and the Wii U's keys.txt that are missing where their emulators
//    read them wait for the player's yes (the first start's Your files, the Setup check), then are copied.
// Nothing is moved or deleted, and nothing in place is written over.

#include "shell_internal.h"
#include "../../app/discovery.h"
#include "../../app/found_folders.h"
#include "../../app/paths.h"
#include "../../eden/library.h"
#include "../../eglantine/launch.h"
#include "../../melonds/melonds.h"
#include "../../ps5/log.h"
#include "../../ps5/privilege.h"

#include <algorithm>

namespace ps5shell
{
	using namespace ps5actions;

	namespace
	{
		constexpr double kDrivesEvery = 3; // seconds between looks at the drives plugged in

		// a path in the console's /data: the preview's stand-in for it there (tools/preview-shell.sh)
		std::string OnConsole(const std::string& path)
		{
			const std::string root = ConsoleRoot();
			return !root.empty() && path.starts_with("/data/") ? root + path : path;
		}

		uint32_t Bit(System side)
		{
			return 1u << (int)side;
		}
	}

	std::string Shell::DropFolder()
	{
		return ConsoleRoot() + "/data/playtendoh";
	}

	std::vector<ps5discover::Root> Shell::DiscoverRoots() const
	{
		// the drop folder first, so what is put there is what is copied when a file is in more than one place
		std::vector<ps5discover::Root> roots{{DropFolder(), 6}};
		for (const std::string& drive : ConnectedDrives())
			roots.push_back({drive, 6});
		const std::string data = ConsoleRoot() + "/data";
		roots.push_back({data + "/homebrew", 5});
		roots.push_back({data, 3}); // its own folders: every system's, and those a player made there
		return roots;
	}

	ps5discover::Targets Shell::DiscoverTargets() const
	{
		ps5discover::Targets targets;
		// the classic consoles' BIOS, in the system folder of each side whose emulator reads it (as the Setup
		// check looks for them): the NES and the SNES play the Disk System's and the Satellaview's games too
		auto system = [](const char* key) { return OnConsole(std::string(ps5retro::Find(key)->data) + "/system"); };
		targets.bios = {
			{"fds", "disksys.rom", {8192}, {system("fds"), system("nes")}},
			{"satellaview", "BS-X.bin", {}, {system("satellaview"), system("snes")}},
			{"gb", "gb_bios.bin", {256}, {system("gb")}},
			{"gbc", "gbc_bios.bin", {2304}, {system("gbc")}},
			{"sgb", "sgb_bios.bin", {256}, {system("sgb")}},
			{"gba", "gba_bios.bin", {16384}, {system("gba")}},
			{"pokemini", "bios.min", {4096}, {system("pokemini")}},
			// the DS's own, which melonDS takes over its built-in ones (the sizes it takes)
			{"ds", "bios7.bin", {16384}, {ps5melonds::Root() + "/bios"}},
			{"ds", "bios9.bin", {4096}, {ps5melonds::Root() + "/bios"}},
			{"ds", "firmware.bin", {131072, 262144, 524288}, {ps5melonds::Root() + "/bios"}},
		};
		// the Switch's keys and firmware, for Eden and for the Switch 2's Lichii alike
		targets.keys = {ps5eden::GameFilesFolder() + "/keys", OnConsole(ps5eglantine::DataFolder()) + "/keys"};
		targets.firmware = {ps5eden::GameFilesFolder() + "/firmware", OnConsole(ps5eglantine::DataFolder()) + "/firmware"};
		targets.discKeys = ps5paths::DiscKeys();
		// the launcher's own: its caches, covers and logs
		for (const char* own : {"/cache", "/covers", "/logs", "/reports", "/themes", "/music"})
			targets.skip.push_back(ps5paths::Root() + own);
		return targets;
	}

	void Shell::DiscoverStart()
	{
		if (ps5discover::Busy() || !ps5privilege::Current().filesystem)
			return;
		// the drop folder, there to be found: what the guide and the empty Library name
		std::error_code ec;
		std::filesystem::create_directories(DropFolder(), ec);
		m_discovery.drives = ConnectedDrives();
		m_discovery.drivesAt = m_now;
		m_discovery.wanted = false;
		ps5log::Line("[discover] looking on {} drives and in /data", m_discovery.drives.size());
		ps5discover::Start(DiscoverRoots(), DiscoverTargets());
	}

	std::string Shell::DiscoverStatus() const
	{
		// tr: the Setup check and the first start while Playtendoh looks for games: {0} is where ("USB drive 1")
		const std::string where = PlaceName(ps5discover::Where());
		return TrF("Looking in {0}…", where.empty() ? std::string("/data") : where);
	}

	void Shell::DiscoverPoll()
	{
		// a drive plugged in since the last look: looked through as it is (and one taken out forgotten)
		if (m_now - m_discovery.drivesAt > kDrivesEvery)
		{
			m_discovery.drivesAt = m_now;
			const std::vector<std::string> drives = ConnectedDrives();
			bool added = false;
			for (const std::string& drive : drives)
				added = added || std::find(m_discovery.drives.begin(), m_discovery.drives.end(), drive) == m_discovery.drives.end();
			m_discovery.drives = drives;
			if (added && m_settings.ui.discover && m_discovery.ran)
			{
				ps5log::Line("[discover] a drive was plugged in");
				m_discovery.announce = true;
				DiscoverStart();
			}
		}
		// a later start's look: once the side opened on has listed its games
		if (m_discovery.wanted && !SystemScanning() && m_listed && m_now - m_start > 2)
		{
			m_discovery.announce = true;
			DiscoverStart();
		}
		ps5discover::Found found;
		if (ps5discover::Take(found))
		{
			DiscoverApply(found);
			StartLookDone();
			if (m_screen == ScreenId::Setup || m_screen == ScreenId::Start)
				RunChecks();
		}
		int copied = 0;
		std::string error;
		if (ps5discover::TakeCopied(copied, error))
		{
			m_discovery.copying = false;
			ps5log::Line("[discover] {} copied{}{}", copied, error.empty() ? "" : ": ", error);
			// what is in place now is no longer to copy
			std::erase_if(m_discovery.found.files, [](const ps5discover::File& file) {
				return std::all_of(file.to.begin(), file.to.end(), [&](const std::string& to) {
					return file.kind == ps5discover::File::Kind::Firmware ? !std::filesystem::is_empty(to) : IsFile(to);
				});
			});
			Toast(error.empty() ? TrP(copied, "Copied {0} file into place", "Copied {0} files into place") :
								  // tr: {0} says what could not be copied, with the system's own words
								  TrF("A file could not be copied: {0}", error));
			RunChecks();
		}
	}

	void Shell::DiscoverApply(const ps5discover::Found& found)
	{
		m_discovery.ran = true;
		m_discovery.found = found;
		std::vector<std::pair<System, int>> news; // the sides given new places, and their games there
		std::vector<System> shown;				  // the sides turned on
		for (const ps5discover::Place& place : found.places)
		{
			const System side = ps5launcher::SideNamed(place.side);
			if (ps5launcher::SideName(side) != place.side)
				continue; // a name the launcher does not know
			// a folder the player chose stays the side's only one
			if (ps5settings::ChoseGamesFolder(m_settings, place.side))
				continue;
			// already looked through: the side's own folder (the Wii U's from before too; the Switch's as Eden has
			// it), or one found before
			std::string own = GamesFolder(side);
			if (side == System::Switch || side == System::Switch2)
				own = ps5eden::ScanFolder(own, side == System::Switch2);
			std::vector<std::string> looked = side == System::WiiU ? ps5paths::GameFolders({own}) : ps5found::With(place.side, own);
			bool known = false;
			for (const std::string& folder : looked)
				known = known || ps5found::detail::Inside(place.folder, folder);
			if (!known)
			{
				std::vector<std::string>& folders = m_settings.ui.found[place.side];
				if (folders.size() < 16)
				{
					folders.push_back(place.folder);
					auto it = std::find_if(news.begin(), news.end(), [&](const auto& n) { return n.first == side; });
					if (it == news.end())
						news.push_back({side, place.games});
					else
						it->second += place.games;
				}
			}
			// every system with games is in the bar, unless the player hid it
			if (!SideShown(side) && !(m_settings.ui.sidesHidden & Bit(side)))
			{
				m_settings.ui.sides |= Bit(side);
				shown.push_back(side);
			}
		}
		// a found folder that is gone (deleted, moved) while its drive is there is forgotten; one on a drive taken
		// out is kept, for when the drive is back
		const std::vector<std::string> drives = ConnectedDrives();
		for (auto& [side, folders] : m_settings.ui.found)
			std::erase_if(folders, [&](const std::string& folder) {
				const bool onDrive = folder.starts_with(ConsoleRoot() + "/mnt/");
				const bool driveHere = std::any_of(drives.begin(), drives.end(), [&](const std::string& drive) { return folder.starts_with(drive + "/") || folder == drive; });
				return (!onDrive || driveHere) && !IsFolder(folder);
			});
		ps5settings::ApplyFound(m_settings);
		SaveSettings();
		for (const auto& [side, games] : news)
			ps5log::Line("[discover] the {} side looks in {} more places", SideTitle(side), m_settings.ui.found[ps5launcher::SideName(side)].size());
		// the side on screen looks again now; the others as they open
		if (std::any_of(news.begin(), news.end(), [this](const auto& n) { return n.first == m_side; }))
		{
			if (IsWiiU())
			{
				if (m_status.coreReady)
				{
					ApplyCemuOptions();
					m_scanning = true;
				}
			}
			else
				LookForGames();
		}
		// a later start says what it found; the first start shows it on its last step
		if (m_discovery.announce)
		{
			for (size_t i = 0; i < news.size() && i < 3; i++)
			{
				std::string where;
				for (const ps5discover::Place& place : found.places)
					if (place.side == ps5launcher::SideName(news[i].first))
					{
						where = PlaceName(place.folder);
						break;
					}
				// tr: a toast after Playtendoh found games by itself: {1} is a console, {2} where ("USB drive 1 / roms")
				Toast(TrP(news[i].second, "Found {0} {1} game in {2}", "Found {0} {1} games in {2}", SystemName(news[i].first), ShortPath(where, 40)));
			}
			if (!found.files.empty())
				Toast(TrP((long long)found.files.size(), "Found {0} file to copy into place: see Settings > Finding games",
					"Found {0} files to copy into place: see Settings > Finding games"));
		}
		m_discovery.announce = false;
		if (!shown.empty() || !news.empty())
			SidesChanged();
	}

	void Shell::DiscoverCopy()
	{
		if (m_discovery.found.files.empty() || ps5discover::Copying())
			return;
		m_discovery.copying = true;
		ps5discover::StartCopy(m_discovery.found.files);
	}

	std::string Shell::FileWords(const ps5discover::File& file) const
	{
		using Kind = ps5discover::File::Kind;
		switch (file.kind)
		{
		// tr: what Playtendoh found to copy into place, as the first start and the Setup check list it
		case Kind::Keys: return file.name == "title.keys" ? Tr("Switch title keys") : Tr("Switch keys");
		case Kind::Firmware: return TrP(file.files, "Switch firmware ({0} file)", "Switch firmware ({0} files)");
		case Kind::DiscKeys: return Tr("Wii U disc keys");
		case Kind::Bios: break;
		}
		const System side = ps5launcher::SideNamed(file.side);
		// tr: a console's BIOS file found to copy: {0} is the console, {1} its file's name
		return TrF("{0} BIOS ({1})", SideTitle(side), file.name);
	}

	std::string Shell::FilesSummary(const std::vector<ps5discover::File>& files) const
	{
		std::string words;
		for (size_t i = 0; i < files.size() && i < 3; i++)
			words += (words.empty() ? "" : ", ") + FileWords(files[i]);
		if (files.size() > 3)
			// tr: after the first three of what was found: "Switch keys, GBA BIOS, DS BIOS and 2 more"
			words += " " + TrP((long long)files.size() - 3, "and {0} more", "and {0} more");
		return words;
	}

	std::string Shell::FoundWhere(System side) const
	{
		// where most of the side's games are, as a person names it ("USB drive 1 / roms")
		const std::string name = ps5launcher::SideName(side);
		const ps5discover::Place* most = nullptr;
		for (const ps5discover::Place& place : m_discovery.found.places)
			if (place.side == name && (!most || place.games > most->games))
				most = &place;
		return most ? PlaceName(most->folder) : std::string();
	}

	int Shell::FoundGames(System side) const
	{
		int games = 0;
		const std::string name = ps5launcher::SideName(side);
		for (const ps5discover::Place& place : m_discovery.found.places)
			if (place.side == name)
				games += place.games;
		return games;
	}

	void Shell::FoundFolderRow(Row& row) const
	{
		// a side whose folder is still its own, and that Playtendoh found games of elsewhere: where, on the row
		const std::vector<std::string> found = ps5found::Of(ps5launcher::SideName(m_side));
		if (found.empty())
			return;
		std::string where = ShortPath(PlaceName(found.front()), 26);
		if (found.size() > 1)
			// tr: Settings > Game files: where Playtendoh found a system's games, and how many places besides
			where += " " + TrP((long long)found.size() - 1, "and {0} more", "and {0} more");
		// tr: Settings > Game files' folder row, when Playtendoh found the system's games by itself: {0} is where
		row.value = TrF("Found in {0}", where);
		// tr: {0} is the system's own games folder
		row.description = TrF("Its own folder, {0}, and where Playtendoh found its games. Cross picks one folder of your own instead.",
			ShortPath(PlaceName(GamesFolder(m_side)), 40));
	}

	bool Shell::AnyGames() const
	{
		for (int i = 0; i < kSides; i++)
			if (SideShown((System)i) && !ps5catalog::Games((ps5catalog::System)i).empty())
				return true;
		return false;
	}
}
