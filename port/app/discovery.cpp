// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: finding a player's games and files wherever they put them (discovery.h).

#include "discovery.h"
#include "media_folders.h"
#include "../eden/exefs.h"
#include "../retro/library.h"
#if defined(__PROSPERO__)
#include "../ps5/log.h"
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ps5discover
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr int kMostEntries = 250000; // a look stops after this many entries, whatever is left
		constexpr double kMostSeconds = 180;
		constexpr size_t kMostPlaces = 12;	 // a system's places at most (the most games first)
		constexpr int kFirmwareFiles = 50;	 // a firmware has a few hundred .nca files; a game's own folder a few

		std::string Lower(std::string text)
		{
			for (char& c : text)
				c = (char)std::tolower((unsigned char)c);
			return text;
		}

		bool EndsWith(const std::string& text, const char* end)
		{
			const size_t length = std::strlen(end);
			return text.size() > length && text.compare(text.size() - length, length, end) == 0;
		}

		bool Inside(const std::string& path, const std::string& folder)
		{
			return path.compare(0, folder.size(), folder) == 0 && (path.size() == folder.size() || path[folder.size()] == '/');
		}

		int Depth(const std::string& path)
		{
			return (int)std::count(path.begin(), path.end(), '/');
		}

		std::string Parent(const std::string& path)
		{
			const size_t slash = path.find_last_of('/');
			return slash == 0 || slash == std::string::npos ? std::string("/") : path.substr(0, slash);
		}

		// up to bytes of a file from offset; false when it is shorter
		bool ReadAt(const std::string& path, uint64_t offset, void* out, size_t bytes)
		{
			std::ifstream in(path, std::ios::binary);
			if (!in || !in.seekg((std::streamoff)offset))
				return false;
			in.read(static_cast<char*>(out), (std::streamsize)bytes);
			return in.gcount() == (std::streamsize)bytes;
		}

		bool MagicAt(const std::string& path, uint64_t offset, const char* magic, size_t length)
		{
			char got[16]{};
			return length <= sizeof(got) && ReadAt(path, offset, got, length) && std::memcmp(got, magic, length) == 0;
		}

		uint64_t SizeOf(const std::string& path)
		{
			struct stat info{};
			return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode) ? (uint64_t)info.st_size : 0;
		}

		bool IsFile(const std::string& path)
		{
			struct stat info{};
			return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
		}

		// a file that is there with something in it: what Copy never writes over
		bool HasFile(const std::string& path)
		{
			return SizeOf(path) > 0;
		}

		// the name of a ZIP's first file, from its local header: empty when it cannot be read
		std::string FirstZipName(const std::string& path)
		{
			unsigned char header[30]{};
			if (!ReadAt(path, 0, header, sizeof(header)) || std::memcmp(header, "PK\3\4", 4) != 0)
				return {};
			std::string name(size_t(header[26] | header[27] << 8), '\0');
			if (name.empty() || name.size() > 512 || !ReadAt(path, 30, name.data(), name.size()))
				return {};
			return Lower(name.substr(name.find_last_of('/') == std::string::npos ? 0 : name.find_last_of('/') + 1));
		}

		// the side of a loose file by its name and cheap checks of its header; zip: the name was a .zip's
		// first file's, whose header is not read
		std::string SideOfName(const std::string& path, const std::string& name, bool zip)
		{
			const std::string lower = Lower(name);
			const auto is = [&](std::initializer_list<const char*> extensions) {
				for (const char* extension : extensions)
					if (EndsWith(lower, extension))
						return true;
				return false;
			};
			if (is({".wua", ".wud", ".wux", ".wuhb"}))
			{
				if (zip || EndsWith(lower, ".wua") || EndsWith(lower, ".wuhb"))
					return "wiiu";
				return EndsWith(lower, ".wux") ? (MagicAt(path, 0, "WUX0", 4) ? "wiiu" : "") : (MagicAt(path, 0, "WUP-", 4) ? "wiiu" : "");
			}
			if (is({".3ds", ".cci"}))
				return zip || MagicAt(path, 0x100, "NCSD", 4) ? "3ds" : ""; // (a .3ds is also 3D Studio's model)
			if (is({".cxi"}))
				return zip || MagicAt(path, 0x100, "NCCH", 4) ? "3ds" : "";
			if (is({".3dsx"}))
				return zip || MagicAt(path, 0, "3DSX", 4) ? "3ds" : "";
			if (is({".cia"}))
			{
				uint32_t header = 0;
				return zip || (ReadAt(path, 0, &header, 4) && header == 0x2020) ? "3ds" : "";
			}
			if (is({".z3ds", ".zcci", ".zcxi", ".z3dsx", ".zcia"}))
				return zip || MagicAt(path, 0, "Z3DS", 4) ? "3ds" : "";
			if (is({".nds", ".srl", ".dsi"}))
			{
				// the Nintendo logo's checksum every DS game's header has
				unsigned char crc[2]{};
				return zip || (ReadAt(path, 0x15c, crc, 2) && crc[0] == 0x56 && crc[1] == 0xcf) ? "ds" : "";
			}
			if (is({".nsp"}))
				return zip ? "" : MagicAt(path, 0, "PFS0", 4) ? "switch" : "";
			if (is({".xci"}))
				return zip ? "" : MagicAt(path, 0x100, "HEAD", 4) ? "switch" : "";
			if (is({".nup"}))
				return zip ? "" : "switch2";
			if (is({".elf", ".dol", ".app", ".axf", ".rpx", ".rom"}))
				return {}; // too common a name to be told by it
			// the other consoles', by their libretro side's formats; a GameCube or Wii disc by its magic word
			int side = ps5retro::IdentifySystem(zip ? name : path); // (a .zip's by its first file's name)
			if (side < 0 && !zip)
				side = ps5retro::DiscSystem(path);
			if (side < 0 || (zip && (ps5retro::Is(side, "gamecube") || ps5retro::Is(side, "wii"))))
				return {};
			const std::string key = ps5retro::Of(side).key;
			if (zip)
				return key;
			// the cheap checks the emulators' own make (ps5retro::ValidateRom)
			unsigned char head[0x110]{};
			if (key == "nes" && is({".nes"}))
				return MagicAt(path, 0, "NES\x1a", 4) ? key : "";
			if (key == "n64")
			{
				if (!ReadAt(path, 0, head, 4))
					return {};
				const bool magic = (head[0] == 0x80 && head[1] == 0x37) || (head[0] == 0x37 && head[1] == 0x80) || (head[0] == 0x40 && head[1] == 0x12);
				return magic ? key : "";
			}
			if (key == "gb" || key == "gbc")
				return ReadAt(path, 0x104, head, 4) && head[0] == 0xce && head[1] == 0xed && head[2] == 0x66 && head[3] == 0x66 ? key : "";
			if (key == "gba")
				return ReadAt(path, 0x04, head, 4) && head[0] == 0x24 && head[1] == 0xff && head[2] == 0xae && head[3] == 0x51 ? key : "";
			return key;
		}

		// an update's or DLC's file rather than a game's, by its name (as Eden's library tells them)
		bool SwitchAddOn(const std::string& name)
		{
			for (size_t open = name.find('['); open != std::string::npos; open = name.find('[', open + 1))
			{
				if (open + 17 >= name.size() || name[open + 17] != ']')
					continue;
				bool hex = true;
				for (size_t i = open + 1; i < open + 17; i++)
					hex = hex && std::isxdigit((unsigned char)name[i]);
				if (hex)
					return (std::strtoull(name.substr(open + 1, 16).c_str(), nullptr, 16) & 0xFFF) != 0;
			}
			const std::string lower = Lower(name);
			for (const char* tag : {"[upd", "[dlc", "(upd", "(dlc", "[update", "(update"})
				if (lower.find(tag) != std::string::npos)
					return true;
			return false;
		}

		// how many disc keys a keys.txt holds: lines that start with 32 hex digits, as Cemu reads them
		int DiscKeyCount(const std::string& file)
		{
			if (SizeOf(file) > (4u << 20))
				return 0;
			std::FILE* f = std::fopen(file.c_str(), "r");
			if (!f)
				return 0;
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

		// a prod.keys: "name = hex" lines with the keys Switch games need; how new it is, by its newest master
		// key (-1: no keys file)
		int ProdKeysGeneration(const std::string& file)
		{
			const uint64_t size = SizeOf(file);
			if (size == 0 || size > (256u << 10))
				return -1;
			std::FILE* f = std::fopen(file.c_str(), "r");
			if (!f)
				return -1;
			int newest = -1;
			bool header = false;
			char line[1024];
			while (std::fgets(line, sizeof line, f))
			{
				const std::string text = Lower(line);
				header = header || text.compare(0, 10, "header_key") == 0;
				if (text.compare(0, 11, "master_key_") == 0 && text.find('=') != std::string::npos)
					newest = std::max(newest, (int)std::strtol(text.c_str() + 11, nullptr, 16));
			}
			std::fclose(f);
			return header || newest >= 0 ? std::max(newest, 0) : -1;
		}

		// a title.keys: "rights ID = title key" lines
		bool TitleKeys(const std::string& file)
		{
			const uint64_t size = SizeOf(file);
			if (size == 0 || size > (8u << 20))
				return false;
			std::FILE* f = std::fopen(file.c_str(), "r");
			if (!f)
				return false;
			char line[256];
			bool keys = std::fgets(line, sizeof line, f) && std::strchr(line, '=') && std::isxdigit((unsigned char)line[0]);
			std::fclose(f);
			return keys;
		}

		// the Wii U title an unpacked game's meta.xml names: 0005000e is an update, 0005000c DLC
		bool WiiuAddOn(const std::string& meta)
		{
			char text[4096]{};
			std::FILE* f = std::fopen(meta.c_str(), "rb");
			if (!f)
				return false;
			const size_t length = std::fread(text, 1, sizeof text - 1, f);
			std::fclose(f);
			const std::string xml = Lower(std::string(text, length));
			const size_t at = xml.find("<title_id");
			if (at == std::string::npos)
				return false;
			const size_t value = xml.find('>', at);
			return value != std::string::npos && (xml.compare(value + 1, 8, "0005000e") == 0 || xml.compare(value + 1, 8, "0005000c") == 0);
		}

		struct Candidate
		{
			std::string path;
			int rank = 0; // the better the higher: a prod.keys' newest master key, a firmware's files, keys.txt's keys
			uint64_t bytes = 0;
			int files = 1;
		};

		struct Walk
		{
			Walk(const Targets& targets, const std::atomic<bool>& stop, std::atomic<int>* progress) : targets(targets), stop(stop), progress(progress) {}
			const Targets& targets;
			const std::atomic<bool>& stop;
			std::atomic<int>* progress;
			std::set<std::string> roots;
			std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
			int looked = 0;
			bool cut = false;
			std::map<std::pair<std::string, std::string>, int> games; // (side, folder): games
			std::vector<Candidate> prodKeys, titleKeys, discKeys, firmware;
			std::map<size_t, Candidate> bios; // Targets::bios' index: the first good one

			bool Skipped(const std::string& path, const std::string& name) const
			{
				if (name.empty() || name[0] == '.' || name[0] == '$' || ps5files::MediaFolder(name))
					return true;
				// the app's caches and the emulators' own storage, which hold no games and many files
				static const char* const kHeavy[] = {"mlc01", "cache", "shadercache", "shaders", "sdmc", "nand", "logs", "screenshots", "states",
					"saves", "graphicpacks", "controllerprofiles", "sysdata", "dump", "crashdump", "reports", "unpacked", "updates", "update", "dlc",
					"system volume information", "lost+found", "node_modules"};
				const std::string lower = Lower(name);
				for (const char* heavy : kHeavy)
					if (lower == heavy)
						return true;
				for (const std::string& skip : targets.skip)
					if (Inside(path, skip))
						return true;
				return false;
			}

			void Game(const std::string& side, const std::string& folder)
			{
				if (!side.empty())
					games[{side, folder}]++;
			}

			bool Over()
			{
				if (stop || looked >= kMostEntries ||
					std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count() > kMostSeconds)
				{
					cut = true;
					return true;
				}
				return false;
			}

			void Visit(const std::string& folder, int depth, int deepest)
			{
				if (Over())
					return;
				std::vector<std::pair<std::string, std::string>> folders; // name, path
				std::vector<std::pair<std::string, std::string>> files;
				std::error_code ec;
				for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
				{
					if (++looked % 256 == 0 && progress)
						progress->store(looked);
					if (Over())
						return;
					const std::string name = it->path().filename().string();
					std::error_code type;
					if (it->is_directory(type))
						folders.emplace_back(name, it->path().string());
					else if (it->is_regular_file(type))
						files.emplace_back(name, it->path().string());
				}
				std::map<std::string, std::string> lowered; // a folder's name in small letters: its path
				for (const auto& [name, path] : folders)
					lowered[Lower(name)] = path;
				const auto has = [&](const char* name) { return lowered.count(name) != 0; };
				bool app = false;
				for (const auto& [name, path] : files)
					app = app || name == "eboot.bin";
				// another app (a homebrew's folder), unless it is where the look began
				if (app && has("sce_sys") && depth > 0)
					return;
				// a Wii U game unpacked: its code, content and meta, in any case
				if (has("code") && has("content") && has("meta"))
				{
					bool addOn = false;
					std::error_code inner;
					for (fs::directory_iterator it(lowered["meta"], inner), end; !inner && it != end; it.increment(inner))
						if (Lower(it->path().filename().string()) == "meta.xml")
							addOn = WiiuAddOn(it->path().string());
					if (!addOn)
						Game("wiiu", Parent(folder));
					return;
				}
				// a Switch 2 (or Switch) game as its ExeFS and RomFS
				if (const fs::path main = ps5eden::exefs::MainIn(folder); !main.empty())
				{
					ps5eden::exefs::Npdm npdm;
					Game(ps5eden::exefs::Read(main, npdm) && npdm.switch2 ? "switch2" : "switch", Parent(folder));
					return;
				}
				// a firmware: a folder of .nca files, and little else
				int nca = 0;
				uint64_t ncaBytes = 0;
				for (const auto& [name, path] : files)
					if (EndsWith(Lower(name), ".nca"))
					{
						nca++;
						ncaBytes += SizeOf(path);
					}
				if (nca >= kFirmwareFiles && nca * 10 >= (int)files.size() * 9)
				{
					firmware.push_back({folder, nca, ncaBytes, nca});
					return;
				}
				for (const auto& [name, path] : files)
				{
					const std::string lower = Lower(name);
					if (lower == "prod.keys")
					{
						if (const int generation = ProdKeysGeneration(path); generation >= 0)
							prodKeys.push_back({path, generation, SizeOf(path)});
						continue;
					}
					if (lower == "title.keys")
					{
						if (TitleKeys(path))
							titleKeys.push_back({path, (int)std::min<uint64_t>(SizeOf(path), 1u << 30), SizeOf(path)});
						continue;
					}
					if (lower == "keys.txt")
					{
						if (const int keys = DiscKeyCount(path); keys > 0)
							discKeys.push_back({path, keys, SizeOf(path)});
						continue;
					}
					bool named = false; // a BIOS's name: no game, whatever its size
					for (size_t i = 0; i < targets.bios.size(); i++)
					{
						const Targets::Bios& wanted = targets.bios[i];
						if (lower != Lower(wanted.name))
							continue;
						named = true;
						const uint64_t size = SizeOf(path);
						if (!bios.count(i) && (wanted.sizes.empty() ? size > 0 : std::find(wanted.sizes.begin(), wanted.sizes.end(), size) != wanted.sizes.end()))
							bios[i] = {path, 0, size};
					}
					if (named)
						continue;
					std::string side;
					if (EndsWith(lower, ".zip"))
					{
						const std::string first = FirstZipName(path);
						side = first.empty() ? std::string() : SideOfName(path, first, true);
					}
					else
						side = SideOfName(path, name, false);
					if ((side == "switch") && SwitchAddOn(name))
						side.clear();
					Game(side, folder);
				}
				if (depth >= deepest)
					return;
				for (const auto& [name, path] : folders)
					if (!Skipped(path, name) && !roots.count(path))
						Visit(path, depth + 1, deepest);
			}
		};

		// the folders a side's library looks into below its own: how far below a place another may be and be
		// found by the first's look (Eden's: one level, a folder per game; the others' four)
		int Reach(const std::string& side)
		{
			return side == "switch" || side == "switch2" ? 1 : 3;
		}

		std::vector<Place> Gather(const std::map<std::pair<std::string, std::string>, int>& games, const std::set<std::string>& roots)
		{
			std::map<std::string, std::map<std::string, int>> bySide;
			for (const auto& [key, count] : games)
				bySide[key.first][key.second] += count;
			std::vector<Place> places;
			for (auto& [side, folders] : bySide)
			{
				const int reach = Reach(side);
				for (bool merged = true; merged;)
				{
					merged = false;
					// a place inside another, near enough for that one's look to find it
					for (auto it = folders.begin(); it != folders.end() && !merged; ++it)
						for (auto& [other, count] : folders)
							if (other != it->first && Inside(it->first, other) && Depth(it->first) - Depth(other) <= reach)
							{
								count += it->second;
								folders.erase(it);
								merged = true;
								break;
							}
					if (merged)
						continue;
					// places side by side, as their folder (not a drive's or /data's top: its look would be all of it)
					std::map<std::string, std::vector<std::string>> byParent;
					for (const auto& [folder, count] : folders)
						byParent[Parent(folder)].push_back(folder);
					for (const auto& [parent, children] : byParent)
						if (children.size() >= 2 && !roots.count(parent) && parent != "/" && Depth(parent) >= 2)
						{
							int total = 0;
							for (const std::string& child : children)
							{
								total += folders[child];
								folders.erase(child);
							}
							folders[parent] += total;
							merged = true;
							break;
						}
				}
				std::vector<Place> mine;
				for (const auto& [folder, count] : folders)
					mine.push_back({side, folder, count});
				std::stable_sort(mine.begin(), mine.end(), [](const Place& a, const Place& b) { return a.games > b.games; });
				if (mine.size() > kMostPlaces)
					mine.resize(kMostPlaces);
				places.insert(places.end(), mine.begin(), mine.end());
			}
			return places;
		}

		// the best of several: the highest rank, then the nearest to near (a firmware beside the keys chosen)
		const Candidate* Best(const std::vector<Candidate>& candidates, const std::string& near = {})
		{
			const Candidate* best = nullptr;
			size_t bestShared = 0;
			for (const Candidate& candidate : candidates)
			{
				size_t shared = 0;
				while (shared < near.size() && shared < candidate.path.size() && near[shared] == candidate.path[shared])
					shared++;
				if (!best || (!near.empty() && shared != bestShared ? shared > bestShared : candidate.rank > best->rank))
				{
					best = &candidate;
					bestShared = shared;
				}
			}
			return best;
		}

		bool HoldsNca(const std::string& folder)
		{
			std::error_code ec;
			for (fs::directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec))
				if (EndsWith(Lower(it->path().filename().string()), ".nca"))
					return true;
			return false;
		}

		// what is to be copied: each kind's best, into the places still without it
		std::vector<File> Files(const Walk& walk, const Targets& targets)
		{
			std::vector<File> files;
			for (const auto& [index, found] : walk.bios)
			{
				const Targets::Bios& wanted = targets.bios[index];
				File file{File::Kind::Bios, wanted.side, wanted.name, found.path, {}, found.bytes};
				for (const std::string& folder : wanted.folders)
				{
					const std::string to = folder + "/" + wanted.name;
					if (!HasFile(to) && to != found.path)
						file.to.push_back(to);
				}
				if (!file.to.empty())
					files.push_back(file);
			}
			std::string keysFolder;
			if (const Candidate* keys = Best(walk.prodKeys))
			{
				File file{File::Kind::Keys, "switch", "prod.keys", keys->path, {}, keys->bytes};
				for (const std::string& folder : targets.keys)
					if (!HasFile(folder + "/prod.keys") && folder + "/prod.keys" != keys->path)
						file.to.push_back(folder + "/prod.keys");
				keysFolder = Parent(keys->path);
				if (!file.to.empty())
					files.push_back(file);
			}
			if (const Candidate* keys = Best(walk.titleKeys, keysFolder))
			{
				File file{File::Kind::Keys, "switch", "title.keys", keys->path, {}, keys->bytes};
				for (const std::string& folder : targets.keys)
					if (!HasFile(folder + "/title.keys") && folder + "/title.keys" != keys->path)
						file.to.push_back(folder + "/title.keys");
				if (!file.to.empty())
					files.push_back(file);
			}
			if (const Candidate* firmware = Best(walk.firmware, keysFolder))
			{
				File file{File::Kind::Firmware, "switch", fs::path(firmware->path).filename().string(), firmware->path, {}, firmware->bytes, firmware->files};
				for (const std::string& folder : targets.firmware)
					if (folder != firmware->path && !HoldsNca(folder))
						file.to.push_back(folder);
				if (!file.to.empty())
					files.push_back(file);
			}
			if (const Candidate* keys = Best(walk.discKeys); keys && !targets.discKeys.empty() && keys->path != targets.discKeys &&
				(!IsFile(targets.discKeys) || DiscKeyCount(targets.discKeys) == 0) && !HasFile(targets.discKeys))
				files.push_back({File::Kind::DiscKeys, "wiiu", "keys.txt", keys->path, {targets.discKeys}, keys->bytes});
			return files;
		}
	}

	std::string GameSide(const std::string& path)
	{
		const std::string name = fs::path(path).filename().string();
		if (EndsWith(Lower(name), ".zip"))
		{
			const std::string first = FirstZipName(path);
			return first.empty() ? std::string() : SideOfName(path, first, true);
		}
		const std::string side = SideOfName(path, name, false);
		return side == "switch" && SwitchAddOn(name) ? std::string() : side;
	}

	namespace
	{
		// the look, saying in where (under mutex) which root it is in
		Found Look(const std::vector<Root>& roots, const Targets& targets, const std::atomic<bool>& stop, std::atomic<int>* looked,
			std::string* where, std::mutex* mutex)
		{
			Walk walk(targets, stop, looked);
			for (const Root& root : roots)
				walk.roots.insert(root.folder);
			for (const Root& root : roots)
			{
				std::error_code ec;
				if (!fs::is_directory(root.folder, ec) || walk.cut)
					continue;
				if (where && mutex)
				{
					std::lock_guard lock(*mutex);
					*where = root.folder;
				}
				walk.Visit(root.folder, 0, root.depth);
			}
			Found found;
			found.places = Gather(walk.games, walk.roots);
			found.files = Files(walk, targets);
			found.looked = walk.looked;
			found.cut = walk.cut;
			found.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - walk.started).count();
			return found;
		}
	}

	Found Look(const std::vector<Root>& roots, const Targets& targets, const std::atomic<bool>& stop, std::atomic<int>* looked)
	{
		return Look(roots, targets, stop, looked, nullptr, nullptr);
	}
	
	// -- in the background -------------------------------------------------------------------------------

	namespace
	{
		std::mutex s_mutex;
		std::thread s_thread;
		std::atomic<bool> s_busy{false}, s_stop{false};
		std::atomic<int> s_looked{0};
		std::string s_where;
		bool s_ready = false;
		Found s_found;
	}

	void Start(const std::vector<Root>& roots, const Targets& targets)
	{
		if (s_busy.exchange(true))
			return;
		if (s_thread.joinable())
			s_thread.join();
		s_stop = false;
		s_looked = 0;
		s_thread = std::thread([roots, targets] {
			{
				std::lock_guard lock(s_mutex);
				s_where = roots.empty() ? std::string() : roots.front().folder;
			}
			Found all = Look(roots, targets, s_stop, &s_looked, &s_where, &s_mutex);
#if defined(__PROSPERO__)
			int games = 0;
			for (const Place& place : all.places)
				games += place.games;
			ps5log::Line("[discover] {} games in {} places, {} files to copy; {} entries in {:.1f} s{}", games, all.places.size(), all.files.size(),
				all.looked, all.seconds, all.cut ? " (stopped at a bound)" : "");
			for (const Place& place : all.places)
				ps5log::Line("[discover]   {}: {} in {}", place.side, place.games, place.folder);
			for (const File& file : all.files)
				ps5log::Line("[discover]   {} ({}): {}", file.name, file.side, file.from);
#endif
			std::lock_guard lock(s_mutex);
			s_found = std::move(all);
			s_ready = true;
			s_busy = false;
		});
	}

	bool Busy()
	{
		return s_busy;
	}

	std::string Where()
	{
		std::lock_guard lock(s_mutex);
		return s_where;
	}

	int Looked()
	{
		return s_looked;
	}

	bool Take(Found& found)
	{
		std::lock_guard lock(s_mutex);
		if (!s_ready)
			return false;
		s_ready = false;
		found = std::move(s_found);
		s_found = {};
		return true;
	}

	// -- copying -----------------------------------------------------------------------------------------

	namespace
	{
		// from into to, through to.copying, which is renamed only once all of it is written and synced, and
		// only while to is still missing or empty
		bool CopyOne(const std::string& from, const std::string& to, std::string& error, const std::atomic<bool>& stop, std::atomic<uint64_t>* done)
		{
			if (HasFile(to))
				return true; // there already: never written over
			std::error_code ec;
			fs::create_directories(Parent(to), ec);
			const std::string part = to + ".copying";
			std::ifstream in(from, std::ios::binary);
			const int out = ::open(part.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (!in || out < 0)
			{
				if (out >= 0)
					::close(out);
				error = from + ": " + std::strerror(errno);
				return false;
			}
			std::vector<char> buffer(1 << 20);
			bool ok = true;
			while (ok && in && !stop)
			{
				in.read(buffer.data(), (std::streamsize)buffer.size());
				for (std::streamsize at = 0, got = in.gcount(); ok && at < got;)
				{
					const ssize_t wrote = ::write(out, buffer.data() + at, (size_t)(got - at));
					ok = wrote > 0;
					at += ok ? wrote : 0;
					if (ok && done)
						*done += (uint64_t)wrote;
				}
			}
			ok = ok && !stop && !in.bad() && ::fsync(out) == 0;
			ok = ::close(out) == 0 && ok;
			if (ok && HasFile(to))
				ok = false; // made meanwhile: left as it is
			if (!ok || ::rename(part.c_str(), to.c_str()) != 0)
			{
				::unlink(part.c_str());
				if (!stop && error.empty())
					error = to + ": " + std::strerror(errno);
				return stop ? false : HasFile(to);
			}
			::chmod(to.c_str(), 0666);
			return true;
		}
	}

	bool Copy(const File& file, std::string& error, const std::atomic<bool>& stop, std::atomic<uint64_t>* done)
	{
		bool ok = true;
		if (file.kind != File::Kind::Firmware)
		{
			for (const std::string& to : file.to)
				ok = CopyOne(file.from, to, error, stop, done) && ok;
			return ok;
		}
		// a firmware: every .nca into a folder beside the one it fills, then each moved in, so a firmware
		// copied half (a game starting, the drive taken out) never looks like one
		std::vector<std::string> names;
		std::error_code ec;
		for (fs::directory_iterator it(file.from, ec), end; !ec && it != end; it.increment(ec))
		{
			const std::string name = it->path().filename().string();
			if (EndsWith(Lower(name), ".nca"))
				names.push_back(name);
		}
		for (const std::string& folder : file.to)
		{
			if (HoldsNca(folder))
				continue; // a firmware there already
			const std::string staging = folder + ".copying";
			fs::remove_all(staging, ec);
			bool all = true;
			for (const std::string& name : names)
				if (!(all = CopyOne(file.from + "/" + name, staging + "/" + name, error, stop, done)))
					break;
			if (all && !HoldsNca(folder))
			{
				fs::create_directories(folder, ec);
				for (const std::string& name : names)
					if (::rename((staging + "/" + name).c_str(), (folder + "/" + name).c_str()) != 0)
					{
						error = folder + ": " + std::strerror(errno);
						all = false;
						break;
					}
			}
			fs::remove_all(staging, ec);
			ok = ok && all;
			if (stop)
				return false;
		}
		return ok;
	}

	namespace
	{
		std::thread s_copyThread;
		std::atomic<bool> s_copying{false}, s_copyStop{false};
		std::atomic<uint64_t> s_copyDone{0};
		uint64_t s_copyTotal = 0;
		bool s_copied = false;
		int s_copiedCount = 0;
		std::string s_copyError;
		// a look or a copy still going on as the program ends: stopped and waited for, before their threads go
		struct AtExit
		{
			~AtExit() { Stop(); }
		} s_atExit;
	}

	void StartCopy(const std::vector<File>& files)
	{
		if (s_copying.exchange(true))
			return;
		if (s_copyThread.joinable())
			s_copyThread.join();
		s_copyStop = false;
		s_copyDone = 0;
		s_copyTotal = 0;
		for (const File& file : files)
			s_copyTotal += file.bytes * file.to.size();
		s_copyThread = std::thread([files] {
			int copied = 0;
			std::string error;
			for (const File& file : files)
			{
				std::string why;
				if (Copy(file, why, s_copyStop, &s_copyDone))
					copied++;
				else if (error.empty())
					error = why;
#if defined(__PROSPERO__)
				ps5log::Line("[discover] copied {} from {}: {}", file.name, file.from, why.empty() ? "done" : why);
#endif
				if (s_copyStop)
					break;
			}
			std::lock_guard lock(s_mutex);
			s_copiedCount = copied;
			s_copyError = s_copyStop ? std::string() : error;
			s_copied = true;
			s_copying = false;
		});
	}

	bool Copying()
	{
		return s_copying;
	}

	float CopyFraction()
	{
		return s_copyTotal ? std::min(1.0f, (float)((double)s_copyDone / (double)s_copyTotal)) : 0.0f;
	}

	bool TakeCopied(int& copied, std::string& error)
	{
		std::lock_guard lock(s_mutex);
		if (!s_copied)
			return false;
		s_copied = false;
		copied = s_copiedCount;
		error = s_copyError;
		return true;
	}

	void Stop()
	{
		s_stop = true;
		s_copyStop = true;
		if (s_thread.joinable())
			s_thread.join();
		if (s_copyThread.joinable())
			s_copyThread.join();
		s_stop = false;
		s_copyStop = false;
	}
}
