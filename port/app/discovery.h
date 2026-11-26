// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: finding a player's games and files wherever they put them (6.0). A look goes through the
// folders it is given (the USB drives and extended storage, /data/homebrew, /data and its folders, the drop
// folder), a bounded number of entries and folders deep, on a thread of its own, and says:
//  - where each system's games are, a folder a place (by their extensions, and their headers where that is
//    cheap: a .3ds is a 3DS game only with its NCSD, a .iso a GameCube or Wii disc by its magic word, a .zip by
//    its first file's name), which the launcher then has the system's library look through, the games played
//    where they are (app/found_folders.h);
//  - the files an emulator reads from a fixed place only, and that are not there yet: a console's BIOS (by its
//    name and size), the Switch's prod.keys and title.keys, its firmware (a folder of .nca files) and the Wii U's
//    keys.txt (with disc keys in it), which Copy puts in place once the player says yes.
// Nothing is moved, renamed or deleted, and a file already in place is never written over: Copy only makes
// what is missing (or empty). The look stops whenever Stop is called (a game starting), and at its bounds.

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace ps5discover
{
	// Where to look, and how many folders deep (the folder itself is 0)
	struct Root
	{
		std::string folder;
		int depth = 4;
	};

	// Where the files an emulator reads from a fixed place go, which the launcher knows
	struct Targets
	{
		struct Bios
		{
			std::string side;			  // the system's name in ps5cemu.json ("gba", "ds")
			std::string name;			  // its file's, as the emulator reads it ("gba_bios.bin")
			std::vector<uint64_t> sizes;  // the sizes a good dump has (empty: any)
			std::vector<std::string> folders; // where it goes (a classic's system folder; the NES's and the FDS's for disksys.rom)
		};
		std::vector<Bios> bios;
		std::vector<std::string> keys;	   // the folders prod.keys and title.keys go in (the Switch's, the Switch 2's)
		std::vector<std::string> firmware; // the folders a firmware's .nca files go in, while they hold none
		std::string discKeys;			   // the Wii U's keys.txt, while it has no keys
		std::vector<std::string> skip;	   // folders never looked into (the app's own data, its caches)
	};

	// A system's games in a folder
	struct Place
	{
		std::string side; // its name in ps5cemu.json
		std::string folder;
		int games = 0;
	};

	// A file (or a firmware's folder) to copy into place
	struct File
	{
		enum class Kind
		{
			Bios,
			Keys,	  // prod.keys, title.keys
			Firmware, // a folder of .nca files
			DiscKeys, // the Wii U's keys.txt
		} kind = Kind::Bios;
		std::string side;			 // the system it is for ("gba", "switch", "wiiu", "ds")
		std::string name;			 // the file's name ("prod.keys"); a firmware's: its folder's
		std::string from;			 // the file, or the firmware's folder
		std::vector<std::string> to; // the files it makes (a firmware: the folders it fills)
		uint64_t bytes = 0;
		int files = 1; // a firmware's .nca files
	};

	struct Found
	{
		std::vector<Place> places;
		std::vector<File> files; // missing where they go, found elsewhere
		int looked = 0;			 // entries looked at
		bool cut = false;		 // stopped at a bound, or asked to
		double seconds = 0;
	};

	// The look itself, on the calling thread (the tests'); where: the root it is in, as it goes
	Found Look(const std::vector<Root>& roots, const Targets& targets, const std::atomic<bool>& stop, std::atomic<int>* looked = nullptr);

	// The system a game's file is for, by its name and, where that is cheap, its header: its name in
	// ps5cemu.json, empty when it is no game (or one Playtendoh cannot tell: a .7z, a .rar)
	std::string GameSide(const std::string& path);

	// -- in the background -----------------------------------------------------------------------------
	void Start(const std::vector<Root>& roots, const Targets& targets); // a look already under way goes on
	bool Busy();
	std::string Where(); // the root being looked through
	int Looked();		 // entries so far
	// a finished look's result, once; false while none is waiting
	bool Take(Found& found);
	void Stop(); // and waits for it (a game is starting)

	// -- copying into place ----------------------------------------------------------------------------
	// One file (or firmware) into each of its places that is still missing or empty; false with why when one
	// could not be. done: bytes copied so far, as it goes
	bool Copy(const File& file, std::string& error, const std::atomic<bool>& stop, std::atomic<uint64_t>* done = nullptr);
	void StartCopy(const std::vector<File>& files);
	bool Copying();
	float CopyFraction();
	// a finished copy's outcome, once: how many it copied, and why one could not be (empty: none failed)
	bool TakeCopied(int& copied, std::string& error);
}
