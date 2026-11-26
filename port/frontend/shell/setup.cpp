// SPDX-License-Identifier: GPL-3.0-or-later
// PS5-ECAM: the Setup check. It answers "why doesn't it work?" in one place: storage, the recompilers, each side's games and keys,
// the 3DS's system files, the DS's BIOS and box art, each with its status, what it means and what to
// do, and a QR code to the guide's page about it. It shows on the first start and whenever storage or
// the recompilers fail at start, then the app opens on a side's Library; Settings > Diagnostics opens
// it at any time. Nothing asks which side (5.3): the app opens on the side last used.

#include "shell_internal.h"
#include "../actions.h"
#include "../../app/paths.h"
#include "../../app/updates.h"
#include "../../azahar/azahar.h"
#include "../../eden/library.h"
#include "../../eglantine/launch.h"
#include "../../melonds/melonds.h"
#include "../../ps5/privilege.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/statvfs.h>

namespace ps5shell
{
	using namespace ui::tokens;
	using namespace ps5actions;

	namespace
	{
		constexpr float kListTop = 300, kListWidth = 1000, kRowHeight = 82, kRowGap = 8;
		constexpr float kAsideX = 1196, kAsideWidth = 628, kAsidePad = 36;
		constexpr const char* kSite = "premohq.github.io/PlayTendoh";

		// a page of the step-by-step guide on the project's site (site/guide), as the QR code beside a check
		// opens it: "wii-u#disc-keys"
		std::string Guide(const char* page)
		{
			return std::string("https://premohq.github.io/PlayTendoh/guide/") + page;
		}

		// how many disc keys a keys.txt holds: the lines that start with 32 hex digits, as Cemu reads them
		// (a comment may follow); -1 when it cannot be read. Opened to read only, so it is never emptied.
		int DiscKeyCount(const std::string& file)
		{
			std::FILE* f = std::fopen(file.c_str(), "r");
			if (!f)
				return -1;
			int keys = 0;
			char line[512];
			while (std::fgets(line, sizeof line, f))
			{
				const char* p = line;
				while (*p == ' ' || *p == '\t')
					p++;
				int hex = 0;
				while (hex < 32 && std::isxdigit((unsigned char)p[hex]))
					hex++;
				keys += hex == 32;
			}
			std::fclose(f);
			return keys;
		}

		// another copy of the app's version, from its sce_sys/param.json (tools/package.sh's contentVersion:
		// 05.001.000 is 5.1.0, the patch in hundreds and a letter after it); empty when it cannot be read
		std::string CopyVersion(const std::string& folder)
		{
			std::FILE* f = std::fopen((folder + "/sce_sys/param.json").c_str(), "r");
			if (!f)
				return {};
			char text[8192];
			const size_t length = std::fread(text, 1, sizeof text - 1, f);
			std::fclose(f);
			text[length] = 0;
			const char* at = std::strstr(text, "\"contentVersion\"");
			int major = 0, minor = 0, patch = 0;
			if (!at || !(at = std::strchr(at + 16, ':')) || !(at = std::strchr(at, '"')) ||
				std::sscanf(at + 1, "%d.%d.%d", &major, &minor, &patch) != 3)
				return {};
			std::string version = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch / 100);
			if (patch % 100 > 0 && patch % 100 <= 26)
				version += (char)('a' + patch % 100 - 1);
			return version;
		}

		std::string FreeSpace(const char* path)
		{
			struct statvfs info{};
			if (statvfs(path, &info) != 0 || info.f_frsize == 0)
				return {};
			const double gigabytes = (double)info.f_bavail * (double)info.f_frsize / 1e9;
			// tr: free space on the PS5's storage, as in 412 GB free
			return TrF("{0} GB free", ps5lang::Decimal(gigabytes, gigabytes >= 10 ? 0 : 1));
		}

		// the covers fetched so far, both sides'
		int Covers()
		{
			int covers = 0;
			for (const char* side : {"wiiu", "3ds", "ds"})
			{
				bool ok = false;
				for (const std::string& name : ListEntries(ps5paths::Covers() + "/boxart/" + side, false, ok))
					covers += Lower(name).ends_with(".tga");
			}
			return covers;
		}
	}

	// -- the Setup check ------------------------------------------------------------------------------

	bool Shell::SetupNeeded() const
	{
		const ps5privilege::Result& privileges = ps5privilege::Current();
		return !privileges.filesystem || (!privileges.jit && !privileges.executable);
	}

	void Shell::SetupOpen(bool first)
	{
		m_setupFirst = first;
		if (!first)
			m_setupFrom = m_screen == ScreenId::Settings ? ScreenId::Settings : ScreenId::Library;
		else if (!m_settings.ui.setupDone)
		{
			// shown once (and again whenever storage or the recompilers fail at start)
			m_settings.ui.setupDone = true;
			SaveSettings();
		}
		RunChecks();
		// what needs you open, Optional and Ready folded; the focus on the first item (the first that needs you)
		m_setupOpen[1] = m_setupOpen[2] = false;
		m_setupRow = 0;
		m_onBar = false;
		Show(ScreenId::Setup);
	}

	std::vector<Shell::SetupItem> Shell::SetupItems() const
	{
		// what needs you, a row each; then Optional and Ready, a row each that opens on the rows it holds
		std::vector<SetupItem> items;
		for (int i = 0; i < (int)m_checks.size(); i++)
			if (CheckGroup(m_checks[i]) == 0)
				items.push_back({i, 0});
		for (int group : {1, 2})
		{
			bool any = false;
			for (const Check& check : m_checks)
				any = any || CheckGroup(check) == group;
			if (!any)
				continue;
			items.push_back({-1, group});
			if (m_setupOpen[group])
				for (int i = 0; i < (int)m_checks.size(); i++)
					if (CheckGroup(m_checks[i]) == group)
						items.push_back({i, group});
		}
		return items;
	}

	const Shell::Check* Shell::SetupFocused() const
	{
		const std::vector<SetupItem> items = SetupItems();
		if (m_setupRow < 0 || m_setupRow >= (int)items.size() || items[m_setupRow].check < 0)
			return nullptr;
		return &m_checks[items[m_setupRow].check];
	}

	void Shell::FinishStart()
	{
		m_setupFirst = false;
		// (a start that went straight to the Setup check has not been asked which layout yet)
		if (!m_settings.ui.layoutChosen)
			LayoutOpen([this] { OpenSide(m_side, true); });
		else
			OpenSide(m_side, true);
	}

	void Shell::RunChecks()
	{
		m_checks.clear();
		const ps5privilege::Result& privileges = ps5privilege::Current();
		const std::string henGuide = Guide("jailbreak#app-jailbreak");

		// where everything is kept
		Check storage{"storage", Tr("Storage")};
		storage.guide = henGuide;
		// tr: what the QR code opens: "Scan it for the HEN setup guide"
		storage.guideTitle = Tr("the HEN setup guide");
		if (privileges.filesystem)
		{
			const std::string free = FreeSpace(ps5paths::Root().c_str());
			storage.state = 0;
			// where the data is when it is on the app's drive (#52); in /data, that /data is open
			storage.detail = Join({ps5paths::Root() == ps5paths::kDefaultRoot ? std::string(Tr("/data is reachable")) : ps5paths::Root(), free}, " · ");
			storage.aboutTitle = Tr("Where everything is kept");
			storage.about = TrF("Settings, saves, caches and logs are kept in {0}, which your HEN unlocks for Playtendoh.",
				ps5paths::Root());
		}
		else
		{
			storage.state = 1;
			storage.detail = Tr("/data can't be reached: nothing can be saved");
			storage.aboutTitle = Tr("Give Playtendoh access to /data");
			storage.about = Tr("Add PPSA99360 to your HEN's app jailbreak list (etaHEN: its app jailbreak list; OnionHEN: the end of "
							   "exact_title_ids in config.ini, with no comma after it), or have elfldr listening on port 9021. Then start "
							   "Playtendoh again.");
		}
		m_checks.push_back(storage);

		// what runs the games' code
		Check recompilers{"recompilers", Tr("Game speed")};
		recompilers.guide = Guide("jailbreak#game-speed");
		recompilers.guideTitle = Tr("the HEN setup guide");
		recompilers.aboutTitle = Tr("Whether games can run at full speed");
		if (privileges.jit || privileges.executable)
		{
			recompilers.state = 0;
			recompilers.detail = privileges.jit ? Tr("JIT memory from your HEN: games run at full speed") :
												  Tr("Executable memory: games run at full speed, no HEN grant needed");
			recompilers.about = Tr("Wii U and 3DS games need special memory from your HEN, or that Playtendoh makes itself, to run at full speed.");
		}
		else
		{
			recompilers.state = 1;
			recompilers.detail = Tr("No JIT memory: Wii U games will run much slower");
			recompilers.about = Tr("Wii U games will run much slower. Add PPSA99360 to your HEN's app jailbreak list, then start Playtendoh again. Settings > Diagnostics shows what it got.");
		}
		m_checks.push_back(recompilers);

		// each side's games: as its last scan found them, else as its folder holds them
		for (int index = 0; index < kSides; index++)
		{
			const System side = (System)index;
			if (!SideShown(side))
				continue; // a side the bar does not offer (Settings > Launcher) has nothing to check
			if (ps5retro::IsRetro(index))
			{
				// the other sides': as the first four's, and their emulator in the package
				const std::string& folder = GamesFolder(side);
				int count = std::max((int)ps5catalog::Games((ps5catalog::System)index).size(), Handheld(side).gameCount);
				bool readable = true;
				if (count <= 0)
				{
					std::string error;
					count = (int)ps5retro::Scan(ps5retro::Of(index).key, folder, error).size();
					readable = error.empty();
				}
				const bool runtime = ps5retro::Available(index, Handheld(side).retroBackend);
				Check games{std::string(ps5launcher::SideName(side)) + "-games", TrF("{0} games", SystemName(side))};
				games.action = Tr("Choose a folder");
				// and where Playtendoh found more of them (discover.cpp); its own folder not made yet reads as none
				count = std::max(count, FoundGames(side));
				readable = readable || !ps5settings::ChoseGamesFolder(m_settings, ps5launcher::SideName(side));
				// no games yet is no problem (the player may have none for it); an emulator missing or a folder
				// that can't be read is
				games.state = !runtime || !readable ? 1 : count > 0 ? 0 : 2;
				// where they are: the place they were found, when all of them were found there
				const std::string where = ShortPath(FoundGames(side) >= count && !FoundWhere(side).empty() ? FoundWhere(side) : PlaceName(folder), 44);
				games.detail = !runtime ? Tr("This system's emulator is not in this copy of Playtendoh.") :
					count > 0			? TrP(count, "{0} game in {1}", "{0} games in {1}", where) :
					readable			? TrF("None in {0} yet", where) :
										  TrF("{0} cannot be read", where);
				games.games = runtime ? count : 0;
				games.aboutTitle = TrF("{0} games", SystemName(side));
				games.about = ps5settings::ChoseGamesFolder(m_settings, ps5launcher::SideName(side)) ?
					TrF("This system's games are in {0}, the folder you chose. Subfolders are searched too.", folder) :
					// tr: {0} is the drop folder, /data/playtendoh
					TrF("Put this system's games anywhere on a USB drive or in {0}: Playtendoh finds them. Or choose a folder of your own.", "/data/playtendoh");
				const bool porpoise = ps5retro::Is(index, "gamecube") || ps5retro::Is(index, "wii");
				games.guide = Guide(porpoise ? "gamecube-wii#games" : "classics#games");
				// tr: what the QR code opens: "Scan it for the GameCube and Wii page"
				games.guideTitle = porpoise ? Tr("the GameCube and Wii page") : Tr("the classic consoles page");
				m_checks.push_back(games);
				continue;
			}
			const bool n3ds = side == System::N3ds, nds = side == System::Nds, nx = side == System::Switch, nx2 = side == System::Switch2;
			const std::string& folder = GamesFolder(side);
			const int catalogued = (int)ps5catalog::Games((ps5catalog::System)SideIndex(side)).size();
			int count = std::max(catalogued, side == System::WiiU ? m_settings.gameCount : Handheld(side).gameCount);
			bool readable = true;
			if (count <= 0)
			{
				count = n3ds ? Count3dsGames(folder) : nds ? CountDsGames(folder) : nx ? ps5eden::CountGames(folder) : nx2 ? ps5eden::CountGames(folder, true) : CountGames(folder);
				readable = count >= 0;
			}
			// and where Playtendoh found more of them (discover.cpp)
			count = std::max(count, FoundGames(side));
			// the side's own words, so each keeps its translation
			const char* gamesTitle = n3ds ? Tr("3DS games") : nds ? Tr("DS games") : nx ? Tr("Switch games") : nx2 ? Tr("Switch 2 games") : Tr("Wii U games");
			Check games{std::string(ps5launcher::SideName(side)) + "-games", gamesTitle};
			games.action = Tr("Choose a folder");
			// the side's own folder not made yet (a side not opened yet) reads as no games; one the player chose
			// that can't be read needs a look
			readable = readable || !ps5settings::ChoseGamesFolder(m_settings, ps5launcher::SideName(side));
			count = std::max(count, 0);
			games.state = !readable ? 1 : count > 0 ? 0 : 2;
			games.games = count;
			// where they are: the place they were found, when all of them were found there
			const std::string where = ShortPath(FoundGames(side) >= count && !FoundWhere(side).empty() ? FoundWhere(side) : PlaceName(folder), 44);
			// tr: {1} is a folder
			games.detail = count > 0 ? TrP(count, "{0} game in {1}", "{0} games in {1}", where) :
				readable		   ? TrF("None in {0} yet", where) :
									 TrF("{0} cannot be read", where);
			games.aboutTitle = n3ds ? Tr("Where your 3DS games go") :
				nds					? Tr("Where your DS games go") :
				nx					? Tr("Where your Switch games go") :
									  Tr("Where your Wii U games go");
			games.about = n3ds ? TrF("Put them in {0}, or choose any folder, such as one on a USB drive. Subfolders are searched too. Install CIA files from Settings > Game files.",
									 folder) :
				nds			   ? TrF("Put them in {0}, or choose any folder, such as one on a USB drive. Subfolders are searched too.",
									 folder) :
				nx			   ? TrF("Put .nsp and .xci files in {0}, each alone or in a folder with its update and DLC. You also need your Switch's keys and firmware.",
									 ps5eden::ScanFolder(folder)) :
								 TrF("Put them in {0}, or choose any folder, such as one on a USB drive. Install updates and DLC from Settings > Game files (a .wua already has them).",
									 folder);
			games.chips = n3ds ? std::vector<std::string>{".3ds", ".cci", ".cxi", ".3dsx", ".z3ds", ".cia", ".7z", ".zip"} :
				nds			   ? std::vector<std::string>{".nds", ".srl", ".7z", ".zip"} :
				nx			   ? std::vector<std::string>{".nsp", ".xci"} :
								 std::vector<std::string>{".wua", ".wud", ".wux", Tr("code · content · meta"), ".rpx", ".7z", ".zip"};
			games.guide = Guide(n3ds ? "3ds#games" : nds ? "ds#games" : nx ? "switch#games" : "wii-u#games");
			// tr: what the QR code opens: "Scan it for the Wii U page"
			games.guideTitle = n3ds ? Tr("the 3DS page") : nds ? Tr("the DS page") : nx ? Tr("the Switch page") : Tr("the Wii U page");
			if (nx2)
			{
				games.aboutTitle = Tr("Where your Switch 2 games go");
				games.about = TrF("Put them in {0}, or choose any folder, such as one on a USB drive. Subfolders are searched too.", folder);
				games.chips = {"exefs/main", "romfs/", "base/", "updates/", "dlc/", "firmware/"};
				games.guide = Guide("switch-2#games");
				games.guideTitle = Tr("the Switch 2 page");
			}
			m_checks.push_back(games);
		}

		// the keys encrypted dumps need
		// a keys.txt with no keys in it (an empty one: players saw 5.1.0 empty theirs) says so, rather than "found"
		const bool discKeys = IsFile(ps5paths::DiscKeys());
		const int discKeyCount = discKeys ? DiscKeyCount(ps5paths::DiscKeys()) : 0;
		Check wiiuKeys{"wiiu-keys", Tr("Wii U disc keys")};
		wiiuKeys.state = !discKeys ? 1 : discKeyCount == 0 ? 2 : 0;
		// tr: the Setup check, Wii U disc keys: {0} is how many keys keys.txt holds
		wiiuKeys.detail = !discKeys	   ? std::string(Tr("keys.txt is missing: only encrypted .wud and .wux need it")) :
			discKeyCount == 0		   ? std::string(Tr("keys.txt has no keys: only encrypted .wud and .wux need them")) :
			discKeyCount > 0		   ? TrP(discKeyCount, "keys.txt found · {0} key", "keys.txt found · {0} keys") :
										 std::string(Tr("keys.txt found"));
		wiiuKeys.action = discKeyCount > 0 ? "" : Tr("How to");
		wiiuKeys.aboutTitle = Tr("Keys for encrypted discs");
		wiiuKeys.about = TrF("Encrypted .wud and .wux need their disc keys in {0}/keys.txt, one per line. Other formats need none.",
			ps5paths::CemuRoot());
		wiiuKeys.guide = Guide("wii-u#disc-keys");
		wiiuKeys.guideTitle = Tr("the Wii U page");
		if (SideShown(System::WiiU))
			m_checks.push_back(wiiuKeys);
		// no 3DS keys to check: Azahar's own are built in (tools/build-azahar.sh), and it plays only
		// decrypted games whatever keys it has

		// the 3DS's own files, which only the Home Menu and a few games need. Azahar looks for them in its
		// own files, which only the 3DS side opens (asked from elsewhere, its logging would start and run
		// on beside a Wii U game): elsewhere, the check says what they are for
		ps5emu::Game homeMenu;
		const bool systemFiles =
			ps5azahar::Available() && m_prepared == System::N3ds && ps5azahar::HomeMenu(m_settings.n3ds.region, homeMenu);
		Check system{"3ds-system", Tr("3DS system files")};
		system.state = systemFiles ? 0 : 2;
		system.detail = systemFiles ? Tr("Artic Setup is done: the Home Menu can start") : Tr("Artic Setup copies them from your 3DS, for the Home Menu");
		system.action = systemFiles || !ps5azahar::Available() ? "" : Tr("Artic Base");
		system.aboutTitle = Tr("Your 3DS's own files");
		system.about = Tr("The 3DS Home Menu and a few games need your 3DS's system files. Start the Artic Setup Tool on your 3DS, then Artic Base here.");
		system.guide = Guide("3ds#system-files");
		system.guideTitle = Tr("the 3DS page");
		if (SideShown(System::N3ds))
			m_checks.push_back(system);

		// a DS's own BIOS, which DS games run without (melonDS's FreeBIOS) but a few run better with
		const bool dsBios = ps5melonds::OwnBiosFound();
		Check nds{"ds-bios", "DS BIOS"};
		nds.state = dsBios ? 0 : 2;
		nds.detail = dsBios ? Tr("Your DS BIOS was found and is used") : Tr("Using the built-in BIOS. Your own DS BIOS is optional");
		nds.action = dsBios ? "" : Tr("How to");
		nds.aboutTitle = Tr("Your DS's own BIOS and firmware");
		nds.about = TrF("DS games run on a built-in BIOS and firmware, which most games are fine with. For the few that aren't, put bios7.bin, bios9.bin and firmware.bin from your own DS in {0}/bios. Playtendoh does not include a BIOS.",
			ps5melonds::Root());
		nds.guide = Guide("ds#bios");
		nds.guideTitle = Tr("the DS page");
		if (SideShown(System::Nds))
			m_checks.push_back(nds);

		// the classic consoles' BIOS files, where their emulator in the app reads one from the side's system
		// folder (libretro/game.h): the two that cannot start without theirs, and the ones with a built-in
		// stand-in that use the console's own when it is there
		struct BiosFile
		{
			const char* key; // ps5retro::kSystems'
			const char* file;
			bool needed;
			uintmax_t size; // 0: any
		};
		static constexpr BiosFile kBiosFiles[] = {
			{"fds", "disksys.rom", true, 8192},		   // Mesen's Disk System
			{"satellaview", "BS-X.bin", true, 0},	   // Mesen-S's
			{"gb", "gb_bios.bin", false, 0},		   // mGBA's boot ROMs: its own stand-in without
			{"gbc", "gbc_bios.bin", false, 0},
			{"sgb", "sgb_bios.bin", false, 0},
			{"gba", "gba_bios.bin", false, 0},
			{"pokemini", "bios.min", false, 0},		   // PokeMini's FreeBIOS without
		};
		for (const BiosFile& bios : kBiosFiles)
		{
			const ps5retro::System* system = ps5retro::Find(bios.key);
			const int index = system ? ps5retro::kFirst + (int)(system - ps5retro::kSystems.data()) : -1;
			if (index < 0 || !SideShown((System)index) || !ps5libretro::Plays(index) || Handheld((System)index).retroBackend != 0)
				continue;
			const std::filesystem::path folder = std::filesystem::path(system->data) / "system";
			std::error_code ec;
			const bool found = std::filesystem::is_regular_file(folder / bios.file, ec) && !ec &&
				(bios.size == 0 || (std::filesystem::file_size(folder / bios.file, ec) == bios.size && !ec));
			const std::string name = SystemName((System)index);
			const bool games = !ps5catalog::Games((ps5catalog::System)index).empty() || Handheld((System)index).gameCount > 0;
			Check check{std::string(bios.key) + "-bios", std::string(ps5launcher::SideTitle((System)index)) + " BIOS"};
			// missing and needed is a problem once the console has games; before, as an optional one, it is a note
			check.state = found ? 0 : bios.needed && games ? 1 : 2;
			// tr: the Setup check, a console's BIOS file: {0} is the file's name, {1} the console
			check.detail = found		 ? TrF("{0} found", bios.file) :
						   bios.needed ? TrF("{0} is missing: {1} games need it", bios.file, name) :
										 TrF("Using the built-in BIOS. {0} is optional", bios.file);
			check.action = found ? "" : Tr("How to");
			// tr: the Setup check's panel for a console's BIOS file
			check.aboutTitle = Tr("The console's own BIOS");
			// tr: {0} is a console, {1} its BIOS file's name, {2} the folder it goes in
			check.about = bios.needed ?
				TrF("{0} games need the console's BIOS: put {1} from your own console in {2}. Playtendoh does not include a BIOS.", name, bios.file, folder.string()) :
				TrF("{0} games run on a built-in BIOS, which most games are fine with. To use the console's own, put {1} in {2}. Playtendoh does not include a BIOS.",
					name, bios.file, folder.string());
			check.guide = Guide("classics#bios");
			check.guideTitle = Tr("the classic consoles page");
			m_checks.push_back(check);
		}

		// the Switch's keys and firmware, which Eden needs and none of which come with the app: dumped from
		// the player's own Switch
		const std::string edenFiles = ps5eden::GameFilesFolder();
		const bool nxKeysFile = ps5eden::KeysFound(), nxFirmware = ps5eden::FirmwareFound();
		struct stat keysInfo{};
		const bool nxKeysEmpty = nxKeysFile && stat((edenFiles + "/keys/prod.keys").c_str(), &keysInfo) == 0 && keysInfo.st_size == 0;
		const bool nxKeys = nxKeysFile && !nxKeysEmpty;
		Check switchKeys{"switch-keys", Tr("Switch keys")};
		switchKeys.state = nxKeys ? 0 : 1;
		switchKeys.detail = nxKeys ? Tr("prod.keys found") :
			nxKeysEmpty			   ? Tr("prod.keys is empty: Switch games need your keys in it") :
									 Tr("prod.keys is missing: Switch games need it");
		switchKeys.action = nxKeys ? "" : Tr("How to");
		switchKeys.aboutTitle = Tr("Your Switch's keys");
		switchKeys.about = TrF("Switch games need your own console's keys: prod.keys (and title.keys) from your Switch, in {0}/keys. Playtendoh does not include keys.",
			edenFiles);
		switchKeys.guide = Guide("switch#keys-and-firmware");
		switchKeys.guideTitle = Tr("the Switch page");
		Check switchFirmware{"switch-firmware", Tr("Switch firmware")};
		switchFirmware.state = nxFirmware ? 0 : 1;
		switchFirmware.detail = nxFirmware ? Tr("Firmware found") :
											 Tr("No firmware yet: many games need it to start");
		switchFirmware.action = nxFirmware ? "" : Tr("How to");
		switchFirmware.aboutTitle = Tr("Your Switch's firmware");
		switchFirmware.about = TrF("Many Switch games need firmware: the .nca files from your Switch, in {0}/firmware. Playtendoh does not include firmware.",
			edenFiles);
		switchFirmware.guide = switchKeys.guide;
		switchFirmware.guideTitle = switchKeys.guideTitle;
		if (SideShown(System::Switch))
		{
			m_checks.push_back(switchKeys);
			m_checks.push_back(switchFirmware);
		}
		Check switch2Keys{"switch2-keys", Tr("Switch 2 keys")};
		switch2Keys.state = ps5eglantine::KeysFound() ? 0 : 1;
		switch2Keys.detail = ps5eglantine::KeysFound() ? Tr("prod.keys found for Lichii") : Tr("prod.keys is missing: Lichii needs your own Switch keys");
		switch2Keys.action = ps5eglantine::KeysFound() ? "" : Tr("How to");
		switch2Keys.aboutTitle = Tr("Your Switch 2 keys");
		switch2Keys.about = TrF("Put your own prod.keys in {0}/keys. Playtendoh and Lichii do not include keys.", ps5eglantine::DataFolder());
		switch2Keys.guide = Guide("switch-2#keys-and-firmware");
		switch2Keys.guideTitle = Tr("the Switch 2 page");
		Check switch2Firmware{"switch2-firmware", Tr("Switch 2 firmware")};
		switch2Firmware.state = ps5eglantine::FirmwareFound() ? 0 : 1;
		switch2Firmware.detail = ps5eglantine::FirmwareFound() ? Tr("Firmware found for Lichii") : Tr("Firmware is missing: some games need it");
		switch2Firmware.action = ps5eglantine::FirmwareFound() ? "" : Tr("How to");
		switch2Firmware.aboutTitle = Tr("Your Switch 2 firmware");
		switch2Firmware.about = TrF("Put your own Switch firmware files in {0}/firmware. Playtendoh and Lichii do not include firmware.", ps5eglantine::DataFolder());
		switch2Firmware.guide = switch2Keys.guide;
		switch2Firmware.guideTitle = switch2Keys.guideTitle;
		if (SideShown(System::Switch2))
		{
			m_checks.push_back(switch2Keys);
			m_checks.push_back(switch2Firmware);
		}

		// covers
		Check boxArt{"boxart", Tr("Box art")};
		boxArt.guide = Guide("setup-check#box-art");
		boxArt.guideTitle = Tr("what Playtendoh fetches online");
		boxArt.aboutTitle = Tr("Covers from GameTDB");
		boxArt.about = Tr("Covers for new games come from GameTDB. Offline, game icons are shown and Playtendoh tries again next time.");
		const int covers = Covers();
		if (!m_settings.boxArt)
		{
			boxArt.state = 2;
			boxArt.detail = Tr("Off: the games' icons are shown instead");
			boxArt.action = Tr("Turn on");
		}
		else if (ps5boxart::Answered() == 0)
		{
			boxArt.state = 1;
			boxArt.detail = Tr("GameTDB did not answer: the games' icons are shown instead");
		}
		else
		{
			boxArt.state = 0;
			boxArt.detail = ps5boxart::Answered() == 1 ? TrP(covers, "GameTDB is reachable · {0} cover", "GameTDB is reachable · {0} covers") :
				covers > 0								  ? TrP(covers, "On · {0} cover", "On · {0} covers") :
															std::string(Tr("On: covers download as games are added"));
		}
		m_checks.push_back(boxArt);

		// another copy of the app in /data/homebrew or a drive's homebrew folder, beside the one running: the
		// home screen's tile can start that one instead, so an older version seems to come back after a
		// restart. Told apart by their eboot.bin, as updates are (ps5paths::RunsFrom: none when the running
		// one cannot be read).
		{
			std::vector<std::string> others;
			for (const std::string& folder : ps5paths::InstallFolders())
				if (IsFile(folder + "/eboot.bin") && !ps5paths::RunsFrom(folder))
					others.push_back(folder);
			if (!others.empty())
			{
				std::string places;
				for (const std::string& folder : others)
					places += (places.empty() ? "" : ", ") + PlaceName(folder);
				const std::string version = CopyVersion(others.front());
				const std::string where = ShortPath(PlaceName(others.front()), 40);
				Check copies{"copies", Tr("Another copy of Playtendoh")};
				copies.state = 1;
				// tr: the Setup check, another copy of the app: {0} is its version, {1} where it is
				copies.detail = version.empty() ? TrF("In {0}: the home screen may start it instead", where) :
												  TrF("{0} in {1}: the home screen may start it instead", version, where);
				copies.aboutTitle = Tr("Why an old version comes back");
				// tr: {0} is where the other copies are
				copies.about = TrF("Playtendoh is also in {0}. The PS5's home screen can start that copy instead of this one, so an older "
								   "version seems to come back after a restart. Keep one: delete the copy you don't use, with FTP or a file manager.",
					places);
				copies.guide = Guide("troubleshooting#old-version");
				copies.guideTitle = Tr("the troubleshooting page");
				m_checks.push_back(copies);
			}
		}
		// the look for games and files everywhere (discover.cpp): how it went, Cross looks again
		{
			Check look{"discover", Tr("Find games and files")};
			look.action = Tr("Look now");
			int found = 0;
			for (const ps5discover::Place& place : m_discovery.found.places)
				found += place.games;
			look.state = m_discovery.ran || ps5discover::Busy() ? 0 : 2;
			// tr: the Setup check's row for the look for games: what the last look found
			look.detail = ps5discover::Busy() ? DiscoverStatus() :
				m_discovery.ran				 ? TrP(found, "Found {0} game on your drives and the PS5", "Found {0} games on your drives and the PS5") :
											   std::string(Tr("Looks on your USB drives and in /data for games, BIOS and keys"));
			look.aboutTitle = Tr("Put your games anywhere");
			// tr: {0} is the drop folder, /data/playtendoh
			look.about = TrF("Playtendoh looks on your USB drives and extended storage, in /data/homebrew and in {0}, in any folders, for every system's games, BIOS files, Switch keys and firmware, and Wii U disc keys. Games are played where they are. A folder you chose in Settings > Game files is used as it is.",
				"/data/playtendoh");
			look.guide = Guide("setup-check#finding-games");
			// tr: what the QR code opens: "Scan it for the Setup check page"
			look.guideTitle = Tr("the Setup check page");
			m_checks.push_back(look);
			if (!m_discovery.found.files.empty())
			{
				Check files{"files", Tr("Files to copy")};
				files.state = 1;
				files.action = m_discovery.copying ? "" : Tr("Copy");
				files.detail = m_discovery.copying ?
					TrF("Copying your files into place: {0}", ps5lang::Percent((int)(ps5discover::CopyFraction() * 100))) :
					FilesSummary(m_discovery.found.files);
				files.aboutTitle = Tr("Found, and not in place yet");
				std::string list;
				for (const ps5discover::File& file : m_discovery.found.files)
				{
					const std::string to = file.kind == ps5discover::File::Kind::Firmware ? file.to.front() : file.to.front().substr(0, file.to.front().find_last_of('/'));
					// tr: a file found and where it goes: "Switch keys: from USB drive 1 / switch / prod.keys to /data/eden/keys"
					list += (list.empty() ? "" : "\n") + TrF("{0}: from {1} to {2}", FileWords(file), PlaceName(file.from), PlaceName(to));
				}
				files.about = Tr("Their emulators only read these from folders of their own. Cross copies them there: yours stay where they are, and nothing already there is replaced.") +
					std::string("\n\n") + list;
				files.guide = Guide("setup-check#files-to-copy");
				files.guideTitle = Tr("the Setup check page");
				m_checks.push_back(files);
			}
		}

		// 6.0: what needs a look first, then what is optional, then what is ready, each in the order made
		std::stable_sort(m_checks.begin(), m_checks.end(), [](const Check& a, const Check& b) {
			auto rank = [](int state) { return state == 1 ? 0 : state == 2 ? 1 : 2; };
			return rank(a.state) < rank(b.state);
		});
	}

	void Shell::SetupUpdate(const ui::Press& press)
	{
		const std::vector<SetupItem> items = SetupItems();
		const int count = (int)items.size();
		switch (press.button)
		{
		case Button::Up:
		case Button::Down:
		{
			const int next = m_setupRow + (press.button == Button::Down ? 1 : -1);
			if (next < 0 || next >= count)
				m_feedback.Play(ui::Cue::Edge, press.repeat);
			else
			{
				m_setupRow = next;
				m_feedback.Play(ui::Cue::Focus);
			}
			break;
		}
		case Button::Cross:
		{
			if (m_setupRow < 0 || m_setupRow >= count)
			{
				m_feedback.Play(ui::Cue::Denied);
				break;
			}
			// a folded group: opened, or folded again
			if (items[m_setupRow].check < 0)
			{
				bool& open = m_setupOpen[items[m_setupRow].group];
				open = !open;
				m_feedback.Play(open ? ui::Cue::Select : ui::Cue::Back);
				break;
			}
			const Check& check = m_checks[items[m_setupRow].check];
			if (check.action.empty())
			{
				m_feedback.Play(ui::Cue::Denied);
				break;
			}
			m_feedback.Play(ui::Cue::Select);
			if (check.id.ends_with("-games"))
			{
				m_filesSide = ps5launcher::SideNamed(check.id.substr(0, check.id.find('-')));
				FilesOpen(0); // back here when it is done
			}
			else if (check.id == "3ds-system")
				ArticOpen();
			else if (check.id == "discover")
			{
				m_discovery.announce = true;
				DiscoverStart();
				RunChecks();
			}
			else if (check.id == "files")
			{
				DiscoverCopy();
				RunChecks();
			}
			else if (check.id == "boxart")
			{
				m_settings.boxArt = true;
				ps5boxart::SetEnabled(true);
				SaveSettings();
				RunChecks();
			}
			else
				OpenHelp(check.aboutTitle, check.about);
			break;
		}
		case Button::Triangle:
			RunChecks();
			m_feedback.Play(ui::Cue::Select);
			Toast(Tr("Checked again"));
			break;
		case Button::Circle:
			m_feedback.Play(ui::Cue::Back);
			if (m_setupFirst)
				FinishStart();
			else if (m_setupFrom == ScreenId::Library)
				ShowTab(ScreenId::Library);
			else
				SettingsOpen("diagnostics", true);
			break;
		default: break;
		}
	}

	void Shell::SetupDraw(Canvas& canvas)
	{
		if (m_checks.empty())
			RunChecks();
		const std::vector<SetupItem> items = SetupItems();
		const int count = (int)items.size();
		m_setupRow = std::clamp(m_setupRow, 0, std::max(0, count - 1));

		// what this is, and in one line whether anything needs you (6.0: no bar, no count of every check)
		canvas.PushAlpha(Enter(0));
		canvas.Text(Style(kOverlineStyle), kSafeX, 92, m_setupFirst ? Tr("Welcome to Playtendoh") : Tr("Help"), Secondary(), 1100, 1);
		canvas.Text(Style({56, ui::Weight::Bold, 1.1f, -0.8f}), kSafeX, 122, Tr("Let’s check your setup"), kText, 1100, 1);
		const int needs = (int)std::count_if(m_checks.begin(), m_checks.end(), [](const Check& check) { return check.state == 1; });
		{
			const Box card{kSafeX, 206, kListWidth, 64};
			Glass(canvas, card, 32, Surface());
			const uint32_t tone = needs ? kWarn : kGood;
			const Box dot{card.x + 14, card.y + 12, 40, 40};
			canvas.Rect(dot, 20, ui::SetAlpha(tone, 0.18f));
			canvas.Draw(needs ? Icon::Warning : Icon::Check, dot.Inset(9), tone);
			// tr: the Setup check's summary: how many checks need the player, or that every one is ready
			const std::string headline = needs ? TrP(needs, "{0} thing needs you", "{0} things need you") : std::string(Tr("Everything is ready"));
			const ui::TextBlock head = canvas.Text(Style({24, ui::Weight::SemiBold, 1.0f}), dot.Right() + 16, card.CentreY() - 13, headline, kText, 420, 1);
			// tr: the Setup check's summary, after how many things need the player: what fixing them does
			const std::string line = needs ? Tr("Fix these and your games can play.") : Tr("Your games can play.");
			canvas.Text(Style({20, ui::Weight::Regular, 1.0f}), dot.Right() + 16 + head.width + 18, card.CentreY() - 11, line, Secondary(),
				card.Right() - 28 - (dot.Right() + 16 + head.width + 18), 1);
		}
		canvas.PopAlpha();

		// what needs you, a full row each; then Optional and Ready, a slim row each, folded until opened
		// (SetupItems). The list glides with the focus.
		canvas.PushAlpha(Enter(1));
		canvas.PushOffset(0, (1 - Enter(1)) * 24);
		constexpr float kListBottom = 1080 - 96, kSlim = 60, kGroupGap = 46;
		std::vector<float> tops(count);
		float at = 0;
		for (int i = 0; i < count; i++)
		{
			// "Needs you" above its rows; a little room before each folded group's row
			if (i == 0 && items[i].group == 0)
				at += kGroupGap;
			else if (items[i].check < 0)
				at += i == 0 ? 12 : 24;
			tops[i] = at;
			at += (items[i].group == 0 ? kRowHeight : kSlim) + kRowGap;
		}
		const float listHeight = at - kRowGap;
		const float view = kListBottom - kListTop;
		m_setupScroll.target = count ? std::clamp(tops[m_setupRow] - view * 0.4f, 0.0f, std::max(0.0f, listHeight - view)) : 0.0f;
		m_setupScroll.omega = m_settings.ui.reduceMotion ? 60.0f : kScrollOmega;
		m_setupScroll.Update(m_dt);
		const float listScroll = m_setupScroll.value;
		canvas.PushClip({0, kListTop - 12, kSafeX + kListWidth + 24, kListBottom - kListTop + 24});
		auto inGroup = [&](int group) {
			return (int)std::count_if(m_checks.begin(), m_checks.end(), [&](const Check& check) { return CheckGroup(check) == group; });
		};
		for (int i = 0; i < count; i++)
		{
			const SetupItem& item = items[i];
			const bool slim = item.group != 0;
			const bool focused = i == m_setupRow;
			const Box row{kSafeX + (item.check >= 0 && slim ? 24.0f : 0.0f), kListTop + tops[i] - listScroll, kListWidth - (item.check >= 0 && slim ? 24.0f : 0.0f),
				slim ? kSlim : kRowHeight};
			if (i == 0 && item.group == 0)
				// tr: the Setup check: above what needs the player; {0} how many
				canvas.Text(Style(kOverlineStyle), row.x + 6, row.y - 32, TrF("Needs you · {0}", inGroup(0)), kWarn, 600, 1);
			Glass(canvas, row, slim ? 18 : kRadiusCard, focused ? Surface2() : slim ? ui::WithAlpha(Surface(), 0.7f) : Surface(), focused ? 1.0f : 0.6f);
			if (focused && !m_settings.ui.highContrast)
				canvas.Rect(row, slim ? 18 : kRadiusCard, ui::SetAlpha(Accent(), LightGround() ? 0.07f : 0.10f));
			const float iconSize = slim ? 34 : 44;
			const Box icon{row.x + 20, row.CentreY() - iconSize * 0.5f, iconSize, iconSize};
			if (item.check < 0)
			{
				// a folded group: what it is in a word, how many, and Show or Hide
				const bool ready = item.group == 2, open = m_setupOpen[item.group];
				const uint32_t colour = ready ? kGood : Tertiary();
				canvas.Rect(icon, iconSize * 0.5f, ready ? ui::SetAlpha(colour, 0.16f) : Wash(0.08f));
				canvas.Draw(ready ? Icon::Check : Icon::Dash, icon.Inset(8), colour);
				const float tx = icon.Right() + 20;
				// tr: the Setup check's folded groups: optional checks and ready ones
				const ui::TextBlock title = canvas.Text(Style({23, ui::Weight::SemiBold, 1.0f}), tx, row.CentreY() - 12, ready ? Tr("Ready") : Tr("Optional"), kText, 300, 1);
				// tr: beside the Setup check's folded groups' names: {0} how many checks each holds
				const std::string words = ready ? TrP(inGroup(2), "{0} thing works", "{0} things work") : TrP(inGroup(1), "{0} thing only some games need", "{0} things only some games need");
				canvas.Text(Style({19, ui::Weight::Regular, 1.0f}), tx + title.width + 16, row.CentreY() - 10, words, Secondary(), 520, 1);
				// tr: the Setup check's folded groups: what Cross does
				const ui::TextBlock action = m_fonts.Layout(Style({21, ui::Weight::SemiBold, 1.0f}), open ? Tr("Hide") : Tr("Show"), 200, 1);
				float right = row.Right() - 28 - 24;
				canvas.Draw(open ? Icon::ChevronDown : Icon::ChevronRight, {right, row.CentreY() - 12, 24, 24}, focused ? kText : Secondary());
				right -= 12 + action.width;
				canvas.Text(action, right, row.CentreY() - action.height * 0.5f, focused ? kText : Secondary());
				if (focused)
					Focus(row, 18);
				continue;
			}
			const Check& check = m_checks[item.check];
			// its status
			const uint32_t colour = check.state == 0 ? kGood : check.state == 1 ? kWarn : Tertiary();
			canvas.Rect(icon, iconSize * 0.5f, check.state == 2 ? Wash(0.08f) : ui::SetAlpha(colour, 0.16f));
			canvas.Draw(check.state == 0 ? Icon::Check : check.state == 1 ? Icon::Warning : Icon::Dash, icon.Inset(slim ? 8 : 10), colour);
			// what Cross does, on the right (an opened group's row's, only while it has the focus)
			float right = row.Right() - 28;
			if (!check.action.empty() && (!slim || focused))
			{
				const ui::TextBlock action = m_fonts.Layout(Style({21, ui::Weight::SemiBold, 1.0f}), check.action, 280, 1);
				right -= action.width;
				canvas.Text(action, right, row.CentreY() - action.height * 0.5f, focused ? kText : Secondary());
				if (focused)
				{
					canvas.Draw(Icon::Cross, {right - 36, row.CentreY() - 13, 26, 26}, kText);
					right -= 36;
				}
				right -= 24;
			}
			// what it is, and what was found: under it on a full row, after it on a slim one (the look's and the
			// copy's as they go)
			const std::string detail = check.id == "discover" && ps5discover::Busy() ? DiscoverStatus() :
				check.id == "files" && m_discovery.copying ? TrF("Copying your files into place: {0}", ps5lang::Percent((int)(ps5discover::CopyFraction() * 100))) :
															 check.detail;
			const float tx = icon.Right() + 20;
			if (slim)
			{
				const ui::TextBlock title = canvas.Text(Style({23, ui::Weight::SemiBold, 1.0f}), tx, row.CentreY() - 12, check.title, kText,
					std::max(100.0f, (right - tx) * 0.45f), 1);
				canvas.Text(Style({19, ui::Weight::Regular, 1.0f}), tx + title.width + 16, row.CentreY() - 10, detail, Secondary(),
					std::max(60.0f, right - tx - title.width - 16), 1);
			}
			else
			{
				canvas.Text(Style({25, ui::Weight::SemiBold, 1.2f}), tx, row.y + 11, check.title, kText, std::max(100.0f, right - tx), 1);
				canvas.Text(Style({20, ui::Weight::Regular, 1.25f}), tx, row.y + 45, detail, Secondary(), std::max(100.0f, right - tx), 1);
			}
			if (focused)
				Focus(row, slim ? 18 : kRadiusCard);
		}
		canvas.PopClip();
		canvas.PopOffset();
		canvas.PopAlpha();
		if (count == 0)
			return;

		// the focused check, beside the list: what it means, what to do, and the guide's page; a folded group's,
		// what it holds
		Check group{"group", items[m_setupRow].group == 2 ? Tr("Ready") : Tr("Optional")};
		group.aboutTitle = items[m_setupRow].group == 2 ? Tr("Nothing to do") : Tr("Only for some games");
		group.about = items[m_setupRow].group == 2 ?
			Tr("Each of these works as it is. Cross shows them, with what was found: your games, keys and box art.") :
			Tr("These matter only to a few games or features: a console's own BIOS, the 3DS's system files and the like. Cross shows them.");
		const Check& check = items[m_setupRow].check >= 0 ? m_checks[items[m_setupRow].check] : group;
		const float inner = kAsideWidth - 2 * kAsidePad;
		const ui::TextBlock heading = m_fonts.Layout(Style({32, ui::Weight::SemiBold, 1.2f}), check.aboutTitle, inner, 2);
		const ui::TextBlock about = m_fonts.Layout(Style({21, ui::Weight::Regular, 1.55f}), check.about, inner, 9);
		float chipsHeight = 0;
		{
			float cx = 0;
			int lines = check.chips.empty() ? 0 : 1;
			for (const std::string& chip : check.chips)
			{
				const float width = m_fonts.Width(Style({20, ui::Weight::SemiBold, 1.0f}), chip) + 32;
				if (cx > 0 && cx + width > inner)
				{
					cx = 0;
					lines++;
				}
				cx += width + 8;
			}
			chipsHeight = lines ? lines * 38 + (lines - 1) * 8 + 18 : 0;
		}
		const float qr = check.guide.empty() ? 0.0f : 150.0f;
		const float height = kAsidePad + 18 + 10 + heading.height + 14 + about.height + chipsHeight + (qr > 0 ? 28 + 2 + 26 + qr : 0.0f) + kAsidePad;
		const Box aside{kAsideX, kListTop + 10, kAsideWidth, height};
		canvas.PushAlpha(Enter(2));
		canvas.PushOffset((1 - Enter(2)) * 30, 0);
		canvas.Shadow(aside, kRadiusSheet, 24, 70, 0x40000000);
		Glass(canvas, aside, kRadiusSheet, m_settings.ui.highContrast ? kInk2 : ui::SetAlpha(kInk1, 0.80f));
		float y = aside.y + kAsidePad;
		const float ax = aside.x + kAsidePad;
		canvas.Text(Style(kOverlineStyle), ax, y, check.title, Secondary(), inner, 1);
		y += 18 + 10;
		canvas.Text(heading, ax, y, kText);
		y += heading.height + 14;
		canvas.Text(about, ax, y, Secondary());
		y += about.height;
		if (!check.chips.empty())
		{
			y += 18;
			float cx = ax;
			for (const std::string& chip : check.chips)
			{
				const float width = m_fonts.Width(Style({20, ui::Weight::SemiBold, 1.0f}), chip) + 32;
				if (cx > ax && cx + width > ax + inner)
				{
					cx = ax;
					y += 38 + 8;
				}
				Chip(canvas, cx, y, chip);
				cx += width + 8;
			}
			y += 38;
		}
		if (qr <= 0)
		{
			canvas.PopOffset();
			canvas.PopAlpha();
			return;
		}
		y += 28;
		canvas.Rect({ax, y, inner, 1.5f}, 0, kLine);
		y += 2 + 26;
		auto code = m_qrCodes.find(check.guide);
		if (code == m_qrCodes.end())
			code = m_qrCodes.emplace(check.guide, ui::QrCode(check.guide)).first;
		ui::DrawQr(canvas, code->second, {ax, y, qr, qr}, 12);
		const float tx = ax + qr + 24, tw = aside.Right() - kAsidePad - tx;
		const ui::TextBlock guideTitle = m_fonts.Layout(Style({22, ui::Weight::SemiBold, 1.3f}), Tr("The guide, on your phone"), tw, 2);
		// tr: {0} is what the QR code opens ("the HEN setup guide"), {1} the site
		const ui::TextBlock guideText =
			m_fonts.Layout(Style({19, ui::Weight::Regular, 1.5f}), TrF("Scan it for {0}: {1}", check.guideTitle, kSite), tw, 4);
		const float ty = y + (qr - guideTitle.height - 6 - guideText.height) * 0.5f;
		canvas.Text(guideTitle, tx, ty, kText);
		canvas.Text(guideText, tx, ty + guideTitle.height + 6, Secondary());
		canvas.PopOffset();
		canvas.PopAlpha();
	}

	// -- what the Setup check stands on: the sides, meeting at the seam -----------------

	void Shell::DrawBrandBackdrop(Canvas& canvas, float seam)
	{
		const Box screen{0, 0, 1920, 1080};
		// the banner's dark brown over a dark theme; a theme with its own motif, or a light one, its own inks
		const bool themed = ui::CurrentMotif() != ui::Motif::Side, light = ui::LightTheme();
		canvas.LinearGradient(screen, 0, themed || light ? kInk1 : 0xff160c08, kInk0, 0, 0, 0, 1080);
		// the Wii U's blue far behind the left, the 3DS's gold behind the right, as the banner has them, and
		// the DS's green rising from below the middle (the theme's colours for them)
		const uint32_t wiiu = themed || light ? kWiiu : 0xffdf9f3b;
		canvas.RadialGradient(screen, 0, ui::SetAlpha(wiiu, 0.30f), ui::SetAlpha(wiiu, 0), 0.18f * 1920, 0.40f * 1080, 605, 529);
		canvas.RadialGradient(screen, 0, ui::SetAlpha(kN3ds, 0.20f), ui::SetAlpha(kN3ds, 0), 0.88f * 1920, 0.60f * 1080, 605, 529);
		canvas.RadialGradient(screen, 0, ui::SetAlpha(kNds, 0.14f), ui::SetAlpha(kNds, 0), 0.5f * 1920, 1.05f * 1080, 700, 420);
		if (themed)
		{
			// a theme's own motif across the whole screen, no seam
			if (ui::CurrentMotif() == ui::Motif::Eden)
				DrawEden(canvas, 0.8f);
		}
		else
		{
			// bubbles on the one side of the seam, waves on the other
			canvas.PushClip({0, 0, seam, 1080});
			DrawBubbles(canvas, 0.6f);
			canvas.PopClip();
			canvas.PushClip({seam, 0, 1920 - seam, 1080});
			DrawWaves(canvas, 0.8f);
			canvas.PopClip();
			canvas.LinearGradient({seam - 1, 0, 2, 540}, 0, 0x00ffffff, 0x38ffffff, seam, 0, seam, 540);
			canvas.LinearGradient({seam - 1, 540, 2, 540}, 0, 0x38ffffff, 0x00ffffff, seam, 540, seam, 1080);
		}
		if (!m_settings.ui.highContrast)
			canvas.Grain(screen, 0x0cffffff);
		if (!light) // the edges darkened, behind a dark theme's light text
			canvas.RadialGradient(screen, 0, 0x00000000, 0x8c000000, 960, 432, 1248, 1026, 0.55f);
	}
}
