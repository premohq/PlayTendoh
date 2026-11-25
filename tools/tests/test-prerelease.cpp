// SPDX-License-Identifier: GPL-3.0-or-later
// A pre-release's watermark (port/app/prerelease.h), built as a release and as pre-release C
// (tools/tests/test-prerelease.sh): a release has no words, so it never shows one; a pre-release's say its
// version and letter. Prints PASS lines; exits 1 on a failure.
#include "app/prerelease.h"
#include "ps5/log.h"

#include <cstdint>
#include <cstdio>
#include <string>

// what the string tables' code (lang.cpp) asks of the console, as the PC preview stands in for it
extern "C" int32_t sceSystemServiceParamGetInt(int32_t, int32_t* value)
{
	*value = 1; // English
	return 0;
}
namespace ps5log
{
	void Write(std::string_view) {}
}

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char* what) {
		std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
		failures += ok ? 0 : 1;
	};
#ifdef PLAYTENDOH_PRERELEASE
	check(ps5prerelease::Is() && ps5prerelease::Letter() == 'C', "pre-release C: it is one, its letter C");
	check(ps5prerelease::Words() == "Pre-release 6.0.0 C", "its watermark: its version and letter");
#else
	check(!ps5prerelease::Is() && ps5prerelease::Letter() == 0, "a release: no pre-release, no letter");
	check(ps5prerelease::Words().empty(), "a release: no watermark");
#endif
	return failures ? 1 : 0;
}
