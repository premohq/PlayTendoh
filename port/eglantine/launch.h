// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../app/emulator.h"
#include "../frontend/settings.h"

namespace ps5eglantine
{
	bool Available();
	bool KeysFound();
	bool FirmwareFound();
	std::string DataFolder();
	void LaunchGame(const ps5emu::Game& game, const ps5settings::Launcher& settings, std::string& error);
	// A game's shader cache (Lichii's lichii-cache.v2.bin and vulkan.bin, with their locks), built again as it
	// plays: what deleting it did, as its row says it. Lichii isn't running while Playtendoh is.
	std::string ClearShaderCache(uint64_t titleId);
}
