// SPDX-License-Identifier: GPL-3.0-or-later
// Finding a player's games and files (port/app/discovery.cpp) in a folder standing for a console's /, made by
// tools/launcher-preview/make-console.py: where each system's games are, what is left out (an update, an app,
// a model named .3ds, a front end's covers), the BIOS, keys and firmware to copy, and that copying makes only
// what is missing. Then a found folder looked through by a classic console's library (archives by their first
// file) and the settings that keep the found folders, a folder the player chose winning.
#include "app/discovery.h"
#include "app/found_folders.h"
#include "app/paths.h"
#include "frontend/settings.h"
#include "retro/library.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
namespace ps5lang { const char* Tr(const char* english) { return english; } std::string Tr(const std::string& english) { return english; } }
namespace ps5porpoise { bool ExtractWiiBanner(const std::string&, const std::string&, std::string& error) { error = "not in this test"; return false; } }

static int failures = 0;
static void Check(bool ok, const std::string& text)
{
	std::cout << (ok ? "PASS " : "FAIL ") << text << '\n';
	failures += !ok;
}

static int Games(const ps5discover::Found& found, const std::string& side, const std::string& folder = {})
{
	int games = 0;
	for (const ps5discover::Place& place : found.places)
		if (place.side == side && (folder.empty() || place.folder == folder))
			games += place.games;
	return games;
}

static const ps5discover::File* FileNamed(const ps5discover::Found& found, const std::string& name)
{
	for (const ps5discover::File& file : found.files)
		if (file.name == name)
			return &file;
	return nullptr;
}

static std::string Text(const fs::path& file)
{
	std::ifstream in(file, std::ios::binary);
	return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv)
{
	if (argc != 2)
		return 2;
	const fs::path console = fs::canonical(argv[1]);
	const std::string usb = (console / "mnt/usb0").string(), data = (console / "data").string();
	ps5discover::Targets targets;
	targets.bios = {{"gba", "gba_bios.bin", {16384}, {data + "/PS5GBA/system"}}, {"ds", "bios7.bin", {16384}, {data + "/melonds/bios"}},
		{"ds", "bios9.bin", {4096}, {data + "/melonds/bios"}}};
	targets.keys = {data + "/eden/keys", data + "/eglantineps5/keys"};
	targets.firmware = {data + "/eden/firmware", data + "/eglantineps5/firmware"};
	targets.discKeys = data + "/cemu/keys.txt";
	// the Switch's keys there already, the Switch 2's an empty file (5.1.0's emptied keys look so)
	fs::create_directories(data + "/eden/keys");
	std::ofstream(data + "/eden/keys/prod.keys") << "header_key = 00\n";
	fs::create_directories(data + "/eglantineps5/keys");
	std::ofstream(data + "/eglantineps5/keys/prod.keys").close();
	const std::vector<ps5discover::Root> roots{{data + "/playtendoh", 6}, {usb, 6}, {data + "/homebrew", 5}, {data, 3}};
	std::atomic<bool> stop{false};
	const ps5discover::Found found = ps5discover::Look(roots, targets, stop);

	Check(Games(found, "snes", usb + "/roms/snes") == 3, "SNES games where they are, their covers' folder not looked into");
	Check(Games(found, "nes") == 3, "NES games: two by their header, one in a .zip among the SNES games by its first file's name");
	Check(Games(found, "nes", data + "/homebrew/PPSA99360/assets") == 0, "an app's own folder in /data/homebrew is not looked into");
	Check(Games(found, "gba", usb + "/roms/gba") == 2 && Games(found, "gb", usb + "/roms/gb") == 1, "Game Boy Advance and Game Boy games by their headers");
	Check(Games(found, "wiiu", usb + "/Wii U") == 1, "an unpacked Wii U game found, its update left out");
	Check(Games(found, "switch", usb + "/switch") == 2, "Switch games found, the update beside them left out");
	Check(Games(found, "3ds") == 1, "a 3DS game by its NCSD, a 3D Studio model named .3ds left out");
	Check(Games(found, "n64", data + "/homebrew/roms") == 1, "an N64 game in /data/homebrew");
	Check(Games(found, "ds", data + "/playtendoh") == 2, "DS games in the drop folder");
	Check(!found.cut, "the look finished within its bounds");

	const ps5discover::File* gbaBios = FileNamed(found, "gba_bios.bin");
	Check(gbaBios && gbaBios->to == std::vector<std::string>{data + "/PS5GBA/system/gba_bios.bin"}, "the GBA BIOS, to its system folder");
	Check(FileNamed(found, "bios7.bin") && FileNamed(found, "bios9.bin"), "the DS BIOS files, at the sizes melonDS takes");
	const ps5discover::File* keys = FileNamed(found, "prod.keys");
	Check(keys && keys->to == std::vector<std::string>{data + "/eglantineps5/keys/prod.keys"}, "prod.keys to the Switch 2's empty one only, the Switch's left as it is");
	Check(FileNamed(found, "title.keys") && FileNamed(found, "title.keys")->to.size() == 2, "title.keys to both");
	const ps5discover::File* firmware = FileNamed(found, "firmware");
	Check(firmware && firmware->files == 64 && firmware->to.size() == 2, "a firmware of 64 .nca files, to both empty firmware folders");
	Check(!FileNamed(found, "keys.txt"), "no keys.txt with disc keys anywhere: nothing to copy");

	// copying: what is missing made, what is there kept
	for (const ps5discover::File& file : found.files)
	{
		std::string error;
		Check(ps5discover::Copy(file, error, stop), "copied " + file.name + (error.empty() ? "" : ": " + error));
	}
	Check(Text(data + "/eden/keys/prod.keys") == "header_key = 00\n", "the Switch's own prod.keys is not written over");
	Check(Text(data + "/eglantineps5/keys/prod.keys") == Text(usb + "/switch/prod.keys"), "the empty one has the keys now");
	Check(fs::file_size(data + "/PS5GBA/system/gba_bios.bin") == 16384 && fs::exists(usb + "/system/gba_bios.bin"), "the BIOS copied, the player's own still there");
	int ncas = 0;
	for (const auto& entry : fs::directory_iterator(data + "/eden/firmware"))
		ncas += entry.path().extension() == ".nca";
	Check(ncas == 64 && !fs::exists(data + "/eden/firmware.copying"), "the firmware's files moved in, the folder they were copied to gone");
	const ps5discover::Found again = ps5discover::Look(roots, targets, stop);
	Check(again.files.empty(), "a second look has nothing left to copy");

	// a classic console's library looks through a found folder too: there a .zip goes by its first file
	ps5found::Set("snes", {usb + "/roms/snes"});
	fs::create_directories(data + "/PS5SNES/games");
	ps5retro::StartScan(11, data + "/PS5SNES/games"); // the SNES side
	ps5retro::FinishScans();
	const auto snes = ps5retro::ListGames(11);
	Check(snes.size() == 3, "the SNES side lists the three found games, not the NES game's .zip beside them");
	std::string error;
	Check(ps5retro::Scan("snes", usb + "/roms/snes", error).size() == 4, "the same folder chosen by the player lists every .zip, as before");

	// the settings: the found folders kept, and used only while the side's folder is its own
	ps5settings::Launcher settings;
	settings.ui.found["snes"] = {usb + "/roms/snes"};
	Check(!ps5settings::ChoseGamesFolder(settings, "snes") && ps5settings::FoundFolders(settings, "snes").size() == 1, "a new install's SNES folder is its own: the found one used");
	settings.retro[7].gamesFolder = usb + "/my snes";
	Check(ps5settings::ChoseGamesFolder(settings, "snes") && ps5settings::FoundFolders(settings, "snes").empty(), "a folder the player chose is the side's only one");
	ps5settings::ApplyFound(settings);
	Check(ps5found::Of("snes").empty(), "and the library is given none");
	settings.ui.sidesHidden = 1 << 11;
	fs::create_directories(ps5paths::Root());
	Check(ps5settings::Save(settings), "saved");
	const ps5settings::Launcher loaded = ps5settings::Load();
	Check(loaded.ui.found.count("snes") && loaded.ui.found.at("snes").size() == 1 && loaded.ui.sidesHidden == 1 << 11 && loaded.ui.discover,
		"the found folders, the hidden sides and the look's switch read back");
	// a ps5cemu.json from before 6.0: a side shown by default and hidden there was hidden by the player
	{
		std::ofstream(ps5paths::LauncherSettings()) << "{\"ui\": {\"sides\": 1, \"switchOffered\": true, \"switch2Offered\": true}}";
		const ps5settings::Launcher before = ps5settings::Load();
		Check(before.ui.sidesHidden == ((2 | 4 | 8 | 1 << 20)) && before.ui.discover,
			"from before 6.0: the 3DS, DS, Switch and Switch 2 hidden there count as hidden by the player, the classics not");
	}
	return failures ? 1 : 0;
}
