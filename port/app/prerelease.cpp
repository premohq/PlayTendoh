// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: whether this build is a pre-release, and its watermark's words (prerelease.h).

#include "prerelease.h"
#include "lang.h"

namespace ps5prerelease
{
	bool Is()
	{
		return Letter() != 0;
	}

	char Letter()
	{
#if defined(PLAYTENDOH_PRERELEASE) && defined(PLAYTENDOH_PRERELEASE_LETTER)
		constexpr char letter = PLAYTENDOH_PRERELEASE_LETTER[0];
		static_assert(letter >= 'A' && letter <= 'Z' && PLAYTENDOH_PRERELEASE_LETTER[1] == 0, "a pre-release's letter is one of A to Z");
		return letter;
#else
		return 0;
#endif
	}

	std::string Words()
	{
		if (!Is())
			return {};
#ifdef PS5CEMU_VERSION
		const std::string version = PS5CEMU_VERSION;
#else
		const std::string version = "dev";
#endif
		// tr: the watermark a pre-release (a test build) shows in a corner at all times: {0} is its version and letter, as "6.0.0 C"
		return ps5lang::TrF("Pre-release {0}", version + " " + Letter());
	}
}
