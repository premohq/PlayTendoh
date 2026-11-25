// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: a pre-release says so on the screen at all times, a release never. A pre-release is a nightly
// (make's CHANNEL: a build not made from a release's tag or Playtendoh, the main branch) with its letter, A to Z, the pre-releases
// of a version counted (make's PRERELEASE: 6.0.0 C is 6.0.0's third). Its watermark is its words in the
// screen's bottom-left corner, small and faint, over everything: the launcher's (shell.cpp), the in-game
// layers' (ps5menu::DrawWatermark: the Wii U's, the 3DS's, the DS's and the classics', and the Switch's and
// Switch 2's programs, told by their session file). Read here alone, as the reports' build values are
// (app/reports.h): the app's other target was never given the launcher's.

#pragma once

#include <string>

namespace ps5prerelease
{
	// The watermark, on the launcher's 1920x1080 layout: its words in the smallest type (the overline's, 18
	// SemiBold capitals) and text-2, on the scrim (both the dark theme's, so it reads the same over the light
	// theme and over any game), a chip's corners, this far from the screen's left and bottom edges
	constexpr float kMargin = 16, kPadX = 8, kPadY = 4;

	// This build is a pre-release
	bool Is();
	// Its letter, 'A' to 'Z'; 0 in a release
	char Letter();
	// What its watermark says, in the menus' language ("Pre-release 6.0.0 C"); empty in a release
	std::string Words();
}
