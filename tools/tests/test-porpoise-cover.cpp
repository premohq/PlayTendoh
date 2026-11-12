// SPDX-License-Identifier: GPL-3.0-or-later
// The cover Porpoise X's in-game menu shows (port/retro/porpoise_banner.cpp's ShareCover) and the quotes its
// loading screen is handed (porpoise_settings.cpp's WriteSession), on made-up files. Argument: an empty folder.
#include "app/tga.h"
#include "retro/porpoise_banner.h"
#include "retro/porpoise_settings.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace ps5log
{
	void Write(std::string_view) {}
}

static int failures = 0;
#define CHECK(what) do { const bool ok_ = (what); std::printf("%s %s\n", ok_ ? "PASS" : "FAIL", #what); failures += !ok_; } while (0)

static std::string Head(const fs::path& file, size_t bytes)
{
	std::ifstream in(file, std::ios::binary);
	std::string head(bytes, '\0');
	in.read(head.data(), (std::streamsize)bytes);
	return head.substr(0, (size_t)in.gcount());
}

int main(int argc, char** argv)
{
	const fs::path work = argc > 1 ? argv[1] : ".";
	const fs::path covers = work / "porpoise" / "covers";
	// the launcher's cover: a TGA, as GameTDB's are kept
	const fs::path tga = work / "GALE01.tga";
	std::vector<uint8_t> rgba(16 * 22 * 4, 200);
	CHECK(ps5tga::Save(tga.string(), rgba, 16, 22));

	// none there: a PNG by the disc's ID, which Porpoise's Library::cover_path reads
	CHECK(ps5porpoise::ShareCover(covers.string(), "GALE01", "Super Smash Bros. Melee", tga.string()));
	CHECK(Head(covers / "GALE01.png", 8) == std::string("\x89PNG\r\n\x1a\n", 8));
	CHECK(!fs::exists(covers / "GALE01.png.new"));
	// one there already, by its ID or the file's name: kept, whatever the launcher has
	std::ofstream(covers / "GZLE01.jpg") << "porpoise's own";
	CHECK(ps5porpoise::ShareCover(covers.string(), "GZLE01", "The Wind Waker", "/nowhere.tga"));
	CHECK(Head(covers / "GZLE01.jpg", 64) == "porpoise's own");
	std::ofstream(covers / "Homebrew.png") << "by name";
	CHECK(ps5porpoise::ShareCover(covers.string(), "", "Homebrew", tga.string()) && Head(covers / "Homebrew.png", 64) == "by name");
	// a PNG or JPEG of the launcher's is copied as it is; no ID: by the file's name
	const fs::path jpeg = work / "own.jpeg";
	std::ofstream(jpeg) << "a folder's picture";
	CHECK(ps5porpoise::ShareCover(covers.string(), "", "Ikaruga", jpeg.string()) && Head(covers / "Ikaruga.jpeg", 64) == "a folder's picture");
	// nothing to give, no name, or a name that would leave the folder: nothing written
	CHECK(!ps5porpoise::ShareCover(covers.string(), "RMCE01", "", ""));
	CHECK(!ps5porpoise::ShareCover(covers.string(), "", "", tga.string()));
	CHECK(!ps5porpoise::ShareCover(covers.string(), "../RMCE01", "", tga.string()) && !fs::exists(covers.parent_path() / "RMCE01.png"));
	CHECK(!ps5porpoise::ShareCover(covers.string(), "RSBE01", "", (work / "missing.tga").string()) && !fs::exists(covers / "RSBE01.png"));

	std::printf(failures ? "%d failed\n" : "porpoise cover: all passed\n", failures);
	return failures ? 1 : 0;
}
