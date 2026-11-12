// SPDX-License-Identifier: GPL-3.0-or-later
// Wii banner (opening.bnr) extraction for launcher images. Based on Porpoise-X's porpoise_banner.cpp. And the
// covers Porpoise X's in-game menu shows, from the launcher's.
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ps5porpoise
{
	// Extract the widescreen banner (first frame) from a Wii disc's opening.bnr.
	// Returns true if banner was successfully extracted and saved as PNG at output_path.
	// The output PNG will be 16:9 (e.g., 608x456 or similar).
	bool ExtractWiiBanner(const std::string& disc_path, const std::string& output_path, std::string& error);

	// Porpoise X's in-game menu shows the cover its own library finds, in its covers folder by the disc's ID or
	// the file's name, as a PNG or JPEG (its Library::cover_path), never the launcher's (a TGA in the app's
	// folder, the one its loading screen is handed). With none there, the launcher's cover goes there: a
	// TGA's pixels as <ID>.png, a PNG or JPEG copied. True when Porpoise has one there now.
	bool ShareCover(const std::string& coversFolder, const std::string& id, const std::string& stem, const std::string& cover);
}