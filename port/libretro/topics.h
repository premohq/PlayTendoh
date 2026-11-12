// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: what a libretro core's option is about, so Settings and the in-game menu can put a core's
// picture options with the picture's and its sound options with the sound's, and leave under Emulator only
// what is the emulated console's. Cores name their options as they like and few group them, so this goes
// by the words in an option's key, label and group (the core's own category), sound before picture: an
// audio filter is a sound option. Needs nothing of the frontend.
#pragma once

#include <cctype>
#include <string>
#include <string_view>

namespace ps5libretro
{
	enum class Topic
	{
		Emulator, // the console itself: its model, its BIOS, its speed, its controllers
		Picture,
		Sound,
	};

	inline Topic TopicOf(std::string_view key, std::string_view label, std::string_view category)
	{
		std::string words;
		for (std::string_view part : {category, label, key})
		{
			for (char c : part)
				words += c == '_' || c == '-' ? ' ' : (char)std::tolower((unsigned char)c);
			words += ' ';
		}
		const auto has = [&](std::initializer_list<const char*> any) {
			for (const char* word : any)
				if (words.find(word) != std::string::npos)
					return true;
			return false;
		};
		if (has({"audio", "sound", "volume", "low pass", "lowpass", "stereo", "reverb", "resampl", "dsp ", "speaker", "channel"}))
			return Topic::Sound;
		if (has({"video", "colo", "palette", "blend", "ghost", "border", "filter", "aspect", "overscan", "crop", "scanline", "ntsc", "resolution",
				"upscal", "render", "gfx", "graphic", "brightness", "contrast", "gamma", "sprite", "screen", "anaglyph", "3d ", "texture", "dither", "deinterlac",
				"interlac", "frameskip", "frame skip", "lcd", "backlight", "widescreen", "hires", "hi res", "msaa", "antialias", "bilinear"}))
			return Topic::Picture;
		return Topic::Emulator;
	}
}
