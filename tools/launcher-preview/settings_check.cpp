// SPDX-License-Identifier: GPL-3.0-or-later
// Every console's Settings, checked as a press uses them (tools/settings-check.sh builds it from the PC
// preview's files): each row that holds a value is changed as Left or Right changes it (ChangeSetting),
// then it must
//   - show the change: it reads what it wrote;
//   - show it still after ps5settings::Save and Load: the change is kept for the next start;
//   - leave every other console's own pages as they were, but the settings every console shares (box
//     art, cheats' downloads, the remote touch screen, vibration): a row that wrote another console's
//     settings looked right and changed nothing for its own (the Switch 2's read the 3DS's, 59f7245);
// and each game's settings (its own of a row) must show and keep the game's change and leave its
// console's as it was. Prints what fails, one line each, and exits 1 when anything does.
//
// What a PC cannot judge is left out, each said below: rows dimmed until RetroAchievements is signed in,
// and those Cemu keeps in files of its own (its accounts, its USB portals), which the preview stands in
// for. Porpoise's rows write /data/porpoise: checked only in the sandbox the script gives it, a /data of
// its own (SETTINGS_CHECK_SANDBOX, when bubblewrap is there), whose Porpoise folder it empties between
// rows; else left out, and nothing in /data touched.
#include "frontend/shell/shell_internal.h"
#include "app/gameinfo.h"
#include "app/lang.h"
#include "retro/porpoise_settings.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace ps5shell;
namespace fs = std::filesystem;

namespace
{
	int failures = 0;
	bool porpoise = false; // in the script's sandbox: /data is its own, Porpoise's folder emptied between rows

	void Fail(const std::string& what)
	{
		std::printf("FAIL  %s\n", what.c_str());
		failures++;
	}

	std::string State(const Row& row)
	{
		switch (row.kind)
		{
		case Row::Kind::Toggle: return row.on ? "on" : "off";
		case Row::Kind::Slider: return std::to_string((int)(row.fraction * 1000));
		default: return std::to_string(row.index) + " " + row.value;
		}
	}

	bool HoldsValue(const Row& row)
	{
		return !row.id.empty() && (row.kind == Row::Kind::Toggle || row.kind == Row::Kind::Choice || row.kind == Row::Kind::Slider ||
									  row.kind == Row::Kind::Stepper || row.kind == Row::Kind::Segmented);
	}

	// what a PC cannot judge (above), and what restarts the app or changes which consoles there are
	bool Skipped(const Row& row)
	{
		const std::string& id = row.id;
		return id == "cheevosnotices" || id == "account" || id == "usb" || id == "ui-language" || id.rfind("ownfolder-", 0) == 0 ||
			id.rfind("side-", 0) == 0 || (!porpoise && id.rfind("porpoise", 0) == 0);
	}

	// the settings every console shares by design
	bool Shared(const std::string& id)
	{
		return id == "boxart" || id == "cheatsauto" || id == "remote" || id == "rumble";
	}

	const Row* Find(const std::vector<Row>& rows, const std::string& id)
	{
		for (const Row& row : rows)
			if (row.id == id)
				return &row;
		return nullptr;
	}

	// a clean start: the launcher's settings as new, and Porpoise's files gone (they are its settings), its
	// rows read again
	void Fresh(Shell& shell, ps5settings::Launcher& settings)
	{
		settings = ps5settings::Launcher{};
		if (porpoise)
		{
			std::error_code ec;
			fs::remove_all(ps5porpoise::Data(), ec);
			fs::create_directories(ps5porpoise::Data(), ec);
		}
		shell.m_porpoise.read = false;
	}
}

int main()
{
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	ps5settings::Launcher settings;
	// as the PC preview does before the launcher: run from the repository's root
	ps5gameinfo::SetFolder("port/app/gametdb");
	ps5lang::SetFolder("port/lang");
	ps5lang::Load(settings.ui.language);
	if (const char* sandbox = std::getenv("SETTINGS_CHECK_SANDBOX"); sandbox && std::string(sandbox) == "1")
	{
		std::error_code ec;
		fs::create_directories(ps5porpoise::Data(), ec);
		porpoise = !ec && fs::is_directory(ps5porpoise::Data());
	}
	std::printf("Porpoise's rows: %s\n", porpoise ? "checked, in a /data of the sandbox's own" : "left out (no sandbox with a /data of its own)");
	ps5launcher::Status status;
	status.coreReady = true; // Cemu's core up, as after the launcher's start: its rows not dimmed
	Shell shell(settings, status, [](ps5launcher::System) {});
	shell.m_themes = ui::BuiltInThemes(); // the launcher's start lists them
	const auto side = [&](int index) { return (ps5launcher::System)index; };
	const auto name = [&](int index) { return std::string(ps5launcher::SideName(side(index))); };

	// every console's own pages' rows, as they show now, but one console's
	auto snapshot = [&](int except) {
		std::map<std::string, std::string> all;
		for (int other = 0; other < ps5launcher::kSideCount; other++)
		{
			if (other == except)
				continue;
			shell.m_side = side(other);
			for (const auto& page : shell.SidePages())
				for (const Row& row : shell.SideRows(page.id, settings))
					if (HoldsValue(row) && !Skipped(row))
						all[name(other) + " " + page.id + " " + row.id] = State(row);
		}
		return all;
	};

	int checked = 0, games = 0;
	for (int index = 0; index < ps5launcher::kSideCount; index++)
	{
		Fresh(shell, settings);
		const auto others = snapshot(index);
		shell.m_side = side(index);
		shell.m_gameTitle = 0;
		for (const auto& page : shell.SettingsPages())
		{
			Fresh(shell, settings);
			shell.m_side = side(index);
			const std::vector<Row> rows = shell.SideRows(page.id, settings);
			for (const Row& row : rows)
			{
				if (!HoldsValue(row) || Skipped(row))
					continue;
				const std::string where = name(index) + " " + page.id + " " + row.id + " (" + row.label + ")";
				std::string after;
				for (int step : {1, -1})
				{
					Fresh(shell, settings);
					shell.m_side = side(index);
					const std::vector<Row> fresh = shell.SideRows(page.id, settings);
					const Row* now = Find(fresh, row.id);
					if (!now)
						break;
					const Row press = *now;
					shell.ChangeSetting(press, step, false);
					const std::vector<Row> changed = shell.SideRows(page.id, settings);
					if (const Row* shown = Find(changed, row.id); shown && State(*shown) != State(press))
					{
						after = State(*shown);
						break;
					}
				}
				checked++;
				if (after.empty())
				{
					Fail(where + ": changes nothing it shows");
					continue;
				}
				// no other console's own page changed, but what they share: the GameCube and the Wii are both
				// Porpoise, whose settings are one file, as Dolphin's are
				const bool porpoiseRow = row.id.rfind("porpoise", 0) == 0;
				if (!Shared(row.id))
					for (const auto& [key, value] : snapshot(index))
						if (const auto was = others.find(key); was != others.end() && was->second != value &&
							!(porpoiseRow && (key.rfind("gamecube ", 0) == 0 || key.rfind("wii ", 0) == 0)))
							Fail(where + ": changed another console's " + key + " (" + was->second + " to " + value + ")");
				shell.m_side = side(index);
				ps5settings::Save(settings);
				settings = ps5settings::Load();
				const std::vector<Row> kept = shell.SideRows(page.id, settings);
				if (const Row* shown = Find(kept, row.id); !shown || State(*shown) != after)
					Fail(where + ": not kept for the next start (" + after + ", read back as " + (shown ? State(*shown) : "no row") + ")");
			}
		}

		// a game's own settings, on its console
		const uint64_t title = 0x0005000010199999ull;
		auto open = [&] {
			shell.m_side = side(index);
			shell.m_gameTitle = title;
			shell.m_gameName = "A game";
			shell.m_gameView = ps5settings::ForGame(settings, name(index), title);
			shell.m_gameRef = {index, title, ps5porpoise::Data() + "/games/A game.iso"}; // Porpoise's file for it is named from it
			shell.m_porpoise.read = false;
		};
		Fresh(shell, settings);
		open();
		for (const auto& page : shell.SettingsPages())
		{
			Fresh(shell, settings);
			open();
			const std::vector<Row> rows = shell.SideRows(page.id, shell.m_gameView);
			for (const Row& row : rows)
			{
				if (!HoldsValue(row) || Skipped(row) || row.setting.empty())
					continue;
				const std::string where = name(index) + " game " + page.id + " " + row.id + " (" + row.label + ")";
				std::string after;
				for (int step : {1, -1})
				{
					Fresh(shell, settings);
					open();
					const std::vector<Row> fresh = shell.SideRows(page.id, shell.m_gameView);
					const Row* now = Find(fresh, row.id);
					if (!now)
						break;
					const Row press = *now;
					shell.ChangeSetting(press, step, false);
					const std::vector<Row> changed = shell.SideRows(page.id, shell.m_gameView);
					if (const Row* shown = Find(changed, row.id); shown && State(*shown) != State(press))
					{
						after = State(*shown);
						break;
					}
				}
				games++;
				if (after.empty())
				{
					Fail(where + ": changes nothing it shows");
					continue;
				}
				ps5settings::Save(settings);
				settings = ps5settings::Load();
				open();
				const std::vector<Row> kept = shell.SideRows(page.id, shell.m_gameView);
				if (const Row* shown = Find(kept, row.id); !shown || State(*shown) != after)
					Fail(where + ": the game's not kept (" + after + ", read back as " + (shown ? State(*shown) : "no row") + ")");
				// its console's own row: as it was (Porpoise's every-game file is its own matter)
				shell.m_gameTitle = 0;
				const ps5settings::Launcher plain;
				const std::vector<Row> console = shell.SideRows(page.id, settings), defaults = shell.SideRows(page.id, plain);
				const Row *own = Find(console, row.id), *was = Find(defaults, row.id);
				if (own && was && State(*own) != State(*was) && row.id.rfind("porpoise", 0) != 0)
					Fail(where + ": changed the console's own too (" + State(*was) + " to " + State(*own) + ")");
			}
		}
	}
	std::printf("%d rows of %d consoles and %d game rows checked: %s\n", checked, ps5launcher::kSideCount, games,
		failures ? (std::to_string(failures) + " failed").c_str() : "every one changes, is kept and stays its console's");
	return failures ? 1 : 0;
}
