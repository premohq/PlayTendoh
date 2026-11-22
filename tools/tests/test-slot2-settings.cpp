// SPDX-License-Identifier: GPL-3.0-or-later
#include "frontend/settings.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
static int failures = 0;
static void Check(bool ok, const char* name) { std::cout << (ok ? "PASS " : "FAIL ") << name << '\n'; failures += !ok; }
int main()
{
	using namespace ps5settings;
	std::filesystem::create_directories(ps5paths::Root());
	Launcher settings;
	Check(!settings.nds.dsSlot2, "new Slot-2 feature defaults off");
	const uint64_t title = 0x1234;
	settings.games[GameKey("ds", title)]["dsSlot2"] = 1;
	settings.games[GameKey("ds", title)]["dsMicSource"] = 2;
	settings.games[GameKey("ds", title)]["dsTouchX"] = 10;
	settings.games[GameKey("ds", title)]["dsTouchScale"] = 75;
	Check(Save(settings), "settings serialize");
	auto loaded = Load();
	Check(!loaded.nds.dsSlot2 && ForGame(loaded, "ds", title).nds.dsSlot2, "game-specific opt-in survives save/load");
	Check(!ForGame(loaded, "ds", title + 1).nds.dsSlot2, "other DS games retain default");
	Check(!ForGame(loaded, "3ds", title).n3ds.dsSlot2 && !ForGame(loaded, "switch", title).nx.dsSlot2, "other systems do not inherit DS opt-in");
	int value = 0;
	Check(Value(ForGame(loaded, "ds", title), "ds", "dsSlot2", value) && value == 1, "per-game setting adapter resolves Slot-2");
	Check(loaded.nds.dsMicSource == 0 && ForGame(loaded, "ds", title).nds.dsMicSource == 2, "recorded microphone is opt-in per DS game and persists");
	Check(Value(ForGame(loaded, "ds", title), "ds", "dsMicSource", value) && value == 2, "microphone setting adapter resolves recorded source");
	Check(ForGame(loaded, "3ds", title).n3ds.dsMicSource == 0, "3DS does not inherit DS microphone source");
	Check(ForGame(loaded, "ds", title).nds.dsTouchX == 10 && ForGame(loaded, "ds", title).nds.dsTouchScale == 75 && loaded.nds.dsTouchScale == 100, "touch calibration persists per game with identity defaults elsewhere");
	loaded.nx2.nxBlockList = false;
	loaded.games[GameKey("switch2", title)]["nxBlockList"] = 1;
	Check(Save(loaded), "Switch 2 precompilation setting serializes");
	auto switch2 = Load();
	Check(!switch2.nx2.nxBlockList && ForGame(switch2, "switch2", title).nx2.nxBlockList,
		"Switch 2 game profile can override precompilation without changing the side default");
	const auto& switch2Settings = GameSettingNames("switch2");
	Check(std::find(switch2Settings.begin(), switch2Settings.end(), "nxBlockList") != switch2Settings.end(),
		"Switch 2 exposes precompilation in its per-game settings list");
	Check(Value(ForGame(switch2, "switch2", title), "switch2", "nxBlockList", value) && value == 1,
		"Switch 2 precompilation profile value resolves through the shared settings adapter");
	switch2.nx2.nxDeadzone = 8;
	switch2.nx2.performance = false;
	switch2.games[GameKey("switch2", title)]["nxDeadzone"] = 20;
	switch2.games[GameKey("switch2", title)]["performance"] = 1;
	Check(Save(switch2), "Switch 2 deadzone and HUD profiles serialize");
	const auto controls = Load();
	const auto perGame = ForGame(controls, "switch2", title);
	Check(perGame.nx2.nxDeadzone == 20 && perGame.nx2.performance && controls.nx2.nxDeadzone == 8 && !controls.nx2.performance,
		"Switch 2 title profile overrides controls and HUD without changing side defaults");
	Check(Value(perGame, "switch2", "nxDeadzone", value) && value == 20 &&
		  Value(perGame, "switch2", "performance", value) && value == 1,
		"Switch 2 deadzone and HUD resolve through the shared per-game settings adapter");
	auto after = perGame;
	SetValue(after, "switch2", "volume", 30);
	SaveChanges("switch2", title, perGame, after);
	const auto saved = Load();
	Check(saved.nx2.nxDeadzone == 8 && !saved.nx2.performance,
		"Changing another menu setting does not copy the game's controls or HUD to side defaults");
	Check(ForGame(saved, "switch2", title).nx2.nxDeadzone == 20 && ForGame(saved, "switch2", title).nx2.performance,
		"Saving menu changes preserves the game's controls and HUD overrides");
	// The Switch 2 side's GPU shortcuts are EglantinePS5's own (fast GPU on, reactive flushing off); a
	// file saved with the Switch's gets them once, and what the player chooses after that stays
	Check(Launcher{}.nx2.nxFastGpu && !Launcher{}.nx2.nxReactiveFlushing && !Launcher{}.nx.nxFastGpu && Launcher{}.nx.nxReactiveFlushing,
		"Switch 2 starts from EglantinePS5's GPU shortcuts, the Switch from its own");
	{
		std::ofstream old(ps5paths::Root() + "/ps5cemu.json");
		old << R"({"switch2": {"nxFastGpu": false, "nxReactiveFlushing": true, "nxDocked": false}})";
	}
	auto migrated = Load();
	Check(migrated.nx2.nxFastGpu && !migrated.nx2.nxReactiveFlushing && !migrated.nx2.nxDocked,
		"settings saved with the Switch's GPU shortcuts get EglantinePS5's once, the rest kept");
	migrated.nx2.nxFastGpu = false;
	migrated.nx2.nxReactiveFlushing = true;
	Check(Save(migrated), "the player's own GPU shortcuts serialize");
	const auto chosen = Load();
	Check(!chosen.nx2.nxFastGpu && chosen.nx2.nxReactiveFlushing, "after that, the player's own GPU shortcuts stay");
	// The Switch 2 pages' rows are the Switch 2's own, not the 3DS's: a side's settings by its name
	Launcher sides;
	Check(&Handheld(sides, "switch2") == &sides.nx2 && &Handheld(sides, "switch") == &sides.nx && &Handheld(sides, "3ds") == &sides.n3ds &&
		  &Handheld(sides, "ds") == &sides.nds && &Handheld(sides, ps5retro::kSystems[0].key) == &sides.retro[0],
		"a side's settings by its name: the Switch 2's are nx2");
	// a game's page, as the shell does it (KeepGameSetting): the row changes the game's view, the view's value is kept
	sides.nx2.nxResolution = 2;
	sides.nx2.nxPacking = 0;
	Launcher view = ForGame(sides, "switch2", title);
	Check(Handheld(view, "switch2").nxResolution == 2, "a Switch 2 game's page shows the side's resolution until the game has its own");
	Handheld(view, "switch2").nxResolution = 1;
	Handheld(view, "switch2").nxPacking = 1;
	Check(Value(view, "switch2", "nxResolution", value) && value == 1, "a Switch 2 game's resolution changes on its page");
	sides.games[GameKey("switch2", title)]["nxResolution"] = value;
	Check(Value(view, "switch2", "nxPacking", value) && value == 1, "a Switch 2 game's memory packing changes on its page");
	sides.games[GameKey("switch2", title)]["nxPacking"] = value;
	Check(Save(sides), "a Switch 2 game's own resolution and packing serialize");
	const auto reread = Load();
	const auto game = ForGame(reread, "switch2", title);
	Check(Handheld(game, "switch2").nxResolution == 1 && game.nx.nxResolution == 1 && game.nx.nxPacking == 1 && reread.nx2.nxResolution == 2 &&
		  reread.nx2.nxPacking == 0,
		"a Switch 2 game's own values show on its page and reach the launch (nx), the side's stay");
	Check(reread.n3ds.nxResolution == Launcher{}.n3ds.nxResolution, "the 3DS's settings are not the Switch 2's");
	// anti-aliasing, FSR sharpening and the speed limit: on the Switch 2's pages, each game's own as well
	for (const char* name : {"nxAntiAliasing", "nxSharpening", "nxSpeedLimit"})
		Check(std::find(switch2Settings.begin(), switch2Settings.end(), name) != switch2Settings.end(), name);
	sides.games[GameKey("switch2", title)]["nxAntiAliasing"] = 2;
	sides.games[GameKey("switch2", title)]["nxSharpening"] = 50;
	sides.games[GameKey("switch2", title)]["nxSpeedLimit"] = 4;
	const auto effects = ForGame(sides, "switch2", title);
	Check(effects.nx.nxAntiAliasing == 2 && effects.nx.nxSharpening == 50 && effects.nx.nxSpeedLimit == 4 && sides.nx2.nxAntiAliasing == 0,
		"a Switch 2 game's anti-aliasing, sharpening and speed limit reach the launch (nx), the side's stay");
	// Settings > Performance > Memory: texture memory, how textures go, the shaders kept, the side's and a game's
	Check(Launcher{}.nx2.nxTextureMemory == 2 && Launcher{}.nx2.nxGiveBack == 2 && !Launcher{}.nx2.nxKeepShaders,
		"Switch 2's memory rows start at 2 GB of textures, all after 1 s, shaders let go");
	for (const char* name : {"nxTextureMemory", "nxGiveBack", "nxKeepShaders"})
		Check(std::find(switch2Settings.begin(), switch2Settings.end(), name) != switch2Settings.end(), name);
	sides.nx2.nxTextureMemory = 1;
	sides.games[GameKey("switch2", title)]["nxGiveBack"] = 4;
	sides.games[GameKey("switch2", title)]["nxKeepShaders"] = 1;
	sides.games[GameKey("switch2", title)]["nxTextureMemory"] = 9; // past its steps: clamped
	Check(Save(sides), "Switch 2's memory rows serialize");
	const auto memory = Load();
	const auto memoryGame = ForGame(memory, "switch2", title);
	Check(memory.nx2.nxTextureMemory == 1 && memory.nx2.nxGiveBack == 2 && !memory.nx2.nxKeepShaders && memoryGame.nx.nxGiveBack == 4 &&
			  memoryGame.nx.nxKeepShaders && memoryGame.nx.nxTextureMemory == 3,
		"a Switch 2 game's own memory rows reach the launch (nx), the side's stay, out of range clamped");
	return failures ? 1 : 0;
}
