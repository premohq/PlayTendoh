// SPDX-License-Identifier: GPL-3.0-or-later
// PS5-ECAM: the folders a front end keeps beside a collection's games (ES-DE's covers, manuals and
// videos, EmulationStation's and Batocera's images, and the like), which hold no games. The libraries'
// walks leave them out: on a network share each of their thousands of files cost the walk a call to
// the computer, and a share with them was looked through for ever, the app waiting on it. C++17, as
// melonDS's build includes it.
#pragma once
#include <string_view>

namespace ps5files
{
	inline bool MediaFolder(std::string_view name)
	{
		static constexpr std::string_view kNames[] = {"covers", "manuals", "videos", "screenshots", "titlescreens", "marquees",
			"miximages", "3dboxes", "backcovers", "fanart", "physicalmedia", "downloaded_media", "media", "images", "snaps",
			"wheel", "boxart", "box2dfront"};
		for (std::string_view known : kNames)
		{
			if (known.size() != name.size())
				continue;
			bool same = true;
			for (size_t i = 0; same && i < name.size(); i++)
			{
				const char c = name[i] >= 'A' && name[i] <= 'Z' ? char(name[i] - 'A' + 'a') : name[i];
				same = c == known[i];
			}
			if (same)
				return true;
		}
		return false;
	}
}
