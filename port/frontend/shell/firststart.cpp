// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: the first start's setup, one question a screen (6.0). The language, what makes it easier to
// see, the layout, where A is and the reports, each its own step with a few cards to choose from; meanwhile
// Playtendoh looks for the player's games and files everywhere they might be (discover.cpp). Then, while the
// look goes on, a screen that waits for it (Cross goes on without waiting); the files it found to copy into
// place, asked once; and a summary that asks nothing: the games found and where, and only what stands in
// the way of playing them. Cross chooses and goes on, Circle goes back a step; the bar at the top right says
// how far along it is. Everything else keeps its default and is in Settings. Settings > Make it yours runs
// it again, and comes back there.

#include "shell_internal.h"
#include "../../app/esde_get.h"
#include "../../app/reports.h"
#include "../../azahar/controls.h"
#include "../../esde/esde.h"

#include <algorithm>
#include <cmath>

namespace ps5shell
{
	using namespace ui::tokens;
	using namespace ps5actions;

	namespace
	{
		constexpr float kTop = 310; // the step's cards, under its question
		constexpr float kQr = 150;

		// tr: the first start's steps, as the bar at the top right names them
		constexpr const char* kStepNames[] = {TrMark("Language"), TrMark("Accessibility"), TrMark("Layout"), TrMark("Buttons"), TrMark("Reports"),
			TrMark("Looking"), TrMark("Files"), TrMark("Ready")};

		// the steps that ask something, before the look's
		bool Question(Shell::StartStep step)
		{
			return step != Shell::StartStep::Finding && step != Shell::StartStep::Files && step != Shell::StartStep::Ready;
		}
	}

	// -- which steps, and moving between them ---------------------------------------------------------

	std::vector<Shell::StartStep> Shell::StartSteps() const
	{
		std::vector<StartStep> steps{StartStep::Language, StartStep::Easier, StartStep::Layout, StartStep::Buttons};
		if (ps5reports::Available())
			steps.push_back(StartStep::Reports);
		// then the look's: waited for while it goes on, then what it found to copy, as it was when asked
		if (m_startWaiting)
			steps.push_back(StartStep::Finding);
		if (!m_startFiles.empty())
			steps.push_back(StartStep::Files);
		steps.push_back(StartStep::Ready);
		return steps;
	}

	std::vector<std::string> Shell::StartLanguages() const
	{
		// the PS5's own first (an empty code), then the others, its own not twice
		std::vector<std::string> codes{""};
		const std::string own = ps5lang::SystemLanguage();
		for (const ps5lang::Language& language : ps5lang::Languages())
			if (language.code != own)
				codes.push_back(language.code);
		return codes;
	}

	void Shell::StartOpen(bool fromSettings)
	{
		m_startFromSettings = fromSettings;
		m_startStep = 0;
		m_startForward = true;
		m_startWaiting = false;
		m_startFiles.clear();
		// each step's focus on what is set now
		const std::vector<std::string> codes = StartLanguages();
		m_startFocus[(int)StartStep::Language] =
			std::max(0, (int)(std::find(codes.begin(), codes.end(), m_settings.ui.language) - codes.begin()) % (int)codes.size());
		m_startFocus[(int)StartStep::Easier] = 3; // Continue: one press for those who need none of them
		m_startFocus[(int)StartStep::Layout] = m_settings.ui.emulationStation ? 2 : std::clamp(m_settings.ui.layout, 0, 1);
		m_startFocus[(int)StartStep::Buttons] = ps5azahar::MappedInput(m_settings.n3ds, ps5azahar::Button::A) == ps5emu::PadInput::Circle ? 0 : 1;
		m_startFocus[(int)StartStep::Reports] = m_settings.ui.reportsAsked && !m_settings.ui.reports ? 1 : 0; // Yes first (the owner's choice)
		m_startFocus[(int)StartStep::Finding] = 0;
		m_startFocus[(int)StartStep::Files] = 0; // Copy them
		m_startFocus[(int)StartStep::Ready] = 0;
		m_startShown = (float)m_startFocus[(int)StartStep::Layout];
		m_onBar = false;
		m_message.clear();
		// the player's games and files, looked for while the questions are answered
		DiscoverStart();
		RunChecks();
		Show(ScreenId::Start);
		m_screenAt = m_now;
	}

	void Shell::StartGo(int by)
	{
		std::vector<StartStep> steps = StartSteps();
		const int next = m_startStep + by;
		if (next < 0)
		{
			if (m_startFromSettings)
			{
				m_feedback.Play(ui::Cue::Back);
				SettingsOpen("launcher", true);
			}
			else
				m_feedback.Play(ui::Cue::Edge);
			return;
		}
		// past the last question: the look's outcome, or a wait for it
		if (by > 0 && Question(steps[m_startStep]) && (next >= (int)steps.size() || !Question(steps[next])))
		{
			m_startWaiting = ps5discover::Busy();
			m_startFiles = m_startWaiting ? std::vector<ps5discover::File>() : m_discovery.found.files;
			steps = StartSteps();
		}
		if (next >= (int)steps.size())
		{
			StartFinish();
			return;
		}
		m_startStep = next;
		m_startForward = by > 0;
		if (steps[next] == StartStep::Ready)
			RunChecks(); // as things are now, with the choices made
		m_feedback.Play(by > 0 ? ui::Cue::Select : ui::Cue::Back);
		m_screenAt = m_now; // the step arrives as a screen does
	}

	void Shell::StartLookDone()
	{
		// the look finished while the player waited for it: on to what it found
		if (m_screen != ScreenId::Start || !m_startWaiting)
			return;
		const std::vector<StartStep> before = StartSteps();
		const bool waitingHere = m_startStep < (int)before.size() && before[m_startStep] == StartStep::Finding;
		m_startWaiting = false;
		m_startFiles = m_discovery.found.files;
		if (!waitingHere)
			return;
		m_startForward = true;
		m_screenAt = m_now;
		RunChecks();
		m_feedback.Play(ui::Cue::Select);
	}

	void Shell::StartFinish()
	{
		m_settings.ui.setupDone = true;
		m_settings.ui.layoutChosen = true;
		SaveSettings();
		// EmulationStation: fetched now, the app opens on it from its next start
		if (m_settings.ui.emulationStation)
		{
			if (ps5esde::Available())
				Toast(Tr("EmulationStation opens next time Playtendoh starts. Hold L1 while it starts to use this launcher instead."));
			else if (!ps5esdeget::Busy())
			{
				ps5esdeget::Start();
				Toast(Tr("Downloading EmulationStation. It opens next time Playtendoh starts."));
			}
		}
		// a look still going on says what it finds as it ends
		m_discovery.announce = ps5discover::Busy();
		ApplyTheme();
		m_feedback.SetLight(Accent());
		m_feedback.Play(ui::Cue::Select);
		if (m_startFromSettings)
			SettingsOpen("launcher", true);
		else
		{
			// the first side with games found, unless the one open has some (a side found only now was hidden)
			if (FoundGames(m_side) == 0)
				for (System side : kBarOrder)
					if (SideShown(side) && FoundGames(side) > 0)
					{
						m_side = side;
						break;
					}
			OpenSide(m_side, true);
		}
	}

	void Shell::StartToggleEasier(int which)
	{
		bool& on = which == 0 ? m_settings.ui.largerText : which == 1 ? m_settings.ui.highContrast : m_settings.ui.reduceMotion;
		on = !on;
		SaveSettings();
		m_feedback.Play(ui::Cue::Select);
	}

	void Shell::StartChoose()
	{
		const StartStep step = StartSteps()[m_startStep];
		const int focus = m_startFocus[(int)step];
		switch (step)
		{
		case StartStep::Language:
		{
			const std::vector<std::string> codes = StartLanguages();
			m_settings.ui.language = codes[std::clamp(focus, 0, (int)codes.size() - 1)];
			SaveSettings();
			ApplyLanguage();
			break;
		}
		case StartStep::Easier:
			if (focus < 3)
			{
				StartToggleEasier(focus);
				return; // a card is turned on or off; Continue goes on
			}
			break;
		case StartStep::Layout:
			m_settings.ui.emulationStation = focus == 2;
			if (focus < 2)
				m_settings.ui.layout = focus;
			SaveSettings();
			break;
		case StartStep::Buttons:
			SetRowIndex(m_settings, "abeverywhere", focus);
			SaveSettings();
			break;
		case StartStep::Reports:
			m_settings.ui.reports = focus == 0;
			m_settings.ui.reportsAsked = true;
			m_settings.ui.reportsAskedPrerelease |= ps5reports::Prerelease();
			SaveSettings();
			ps5reports::SetEnabled(ps5reports::Available() && m_settings.ui.reports);
			break;
		case StartStep::Finding:
		{
			// on without waiting: what it finds shows up as it ends (the Library, the Setup check)
			m_startWaiting = false;
			m_startFiles.clear();
			m_startForward = true;
			m_screenAt = m_now;
			RunChecks();
			m_feedback.Play(ui::Cue::Select);
			return;
		}
		case StartStep::Files:
			if (focus == 0)
				DiscoverCopy();
			break;
		case StartStep::Ready: break;
		}
		StartGo(1);
	}

	// what the summary says needs the player: what stops games, the files of a console that has games, another
	// copy of the app, and what was found to copy and not copied yet
	std::vector<const Shell::Check*> Shell::StartNeeds() const
	{
		std::vector<const Check*> needs;
		auto hasGames = [&](const char* side) {
			for (const Check& check : m_checks)
				if (check.id == std::string(side) + "-games")
					return check.games > 0;
			return false;
		};
		for (const Check& check : m_checks)
		{
			if (check.state != 1)
				continue;
			if (check.id == "storage" || check.id == "recompilers" || check.id == "copies" || check.id == "files" || check.id.ends_with("-bios") ||
				((check.id == "switch-keys" || check.id == "switch-firmware") && hasGames("switch")) ||
				((check.id == "switch2-keys" || check.id == "switch2-firmware") && hasGames("switch2")))
				needs.push_back(&check);
		}
		return needs;
	}

	void Shell::StartUpdate(const ui::Press& press)
	{
		const StartStep step = StartSteps()[m_startStep];
		int& focus = m_startFocus[(int)step];
		const int count = step == StartStep::Language ? (int)StartLanguages().size() :
			step == StartStep::Layout				  ? 3 :
			step == StartStep::Easier				  ? 4 :
			step == StartStep::Finding				  ? 1 :
			step == StartStep::Ready				  ? (int)StartNeeds().size() :
														2;
		auto move = [&](int by) {
			const int next = focus + by;
			if (next < 0 || next >= count)
				m_feedback.Play(ui::Cue::Edge, press.repeat);
			else
			{
				focus = next;
				m_feedback.Play(ui::Cue::Focus);
			}
		};
		constexpr int kLanguageColumns = 6;
		switch (press.button)
		{
		case Button::Left:
		case Button::Right:
			if (step == StartStep::Easier && focus == 3)
				m_feedback.Play(ui::Cue::Edge, press.repeat);
			else if (step == StartStep::Easier)
			{
				const int next = focus + (press.button == Button::Right ? 1 : -1);
				if (next < 0 || next > 2)
					m_feedback.Play(ui::Cue::Edge, press.repeat);
				else
				{
					focus = next;
					m_feedback.Play(ui::Cue::Focus);
				}
			}
			else if (step != StartStep::Ready && step != StartStep::Finding)
				move(press.button == Button::Right ? 1 : -1);
			break;
		case Button::Up:
		case Button::Down:
		{
			const int by = press.button == Button::Down ? 1 : -1;
			if (step == StartStep::Language)
			{
				// by slot: the PS5's own takes the first two
				const int slot = focus == 0 ? 0 : focus + 1;
				const int to = slot + by * kLanguageColumns;
				const int next = to <= 1 ? 0 : to - 1;
				if (to < 0 || next >= count)
					m_feedback.Play(ui::Cue::Edge, press.repeat);
				else
				{
					focus = next;
					m_feedback.Play(ui::Cue::Focus);
				}
			}
			else if (step == StartStep::Easier)
			{
				// the cards above, Continue below them
				if ((by > 0) == (focus == 3))
					m_feedback.Play(ui::Cue::Edge, press.repeat);
				else
				{
					focus = by > 0 ? 3 : 0;
					m_feedback.Play(ui::Cue::Focus);
				}
			}
			else if (step == StartStep::Ready)
				move(by);
			else
				m_feedback.Play(ui::Cue::Edge, press.repeat);
			break;
		}
		case Button::Cross:
			if (step == StartStep::Ready)
				StartFinish();
			else
				StartChoose();
			break;
		case Button::Triangle:
			// larger text from the first screen on, for those who need it to read the rest
			if (step == StartStep::Language || step == StartStep::Easier)
				StartToggleEasier(0);
			break;
		case Button::Circle: StartGo(-1); break;
		default: break;
		}
	}

	std::vector<Hint> Shell::StartHints() const
	{
		const StartStep step = StartSteps()[m_startStep];
		const int focus = m_startFocus[(int)step];
		std::vector<Hint> hints;
		if (step == StartStep::Language)
			hints.push_back({Icon::UpDown, Tr("Move")});
		else if (step != StartStep::Ready && step != StartStep::Finding)
			hints.push_back({Icon::LeftRight, Tr("Move")});
		if (step == StartStep::Language || step == StartStep::Easier)
			// tr: the first start: Triangle makes every word a size larger, or back
			hints.push_back({Icon::Triangle, m_settings.ui.largerText ? Tr("Normal text") : Tr("Larger text")});
		// tr: the first start's setup: Cross on its last step
		hints.push_back({Icon::Cross, step == StartStep::Ready ? Tr("Start playing") :
				step == StartStep::Finding					   ? Tr("Go on without waiting") :
				step == StartStep::Easier && focus < 3		   ? Tr("Turn on or off") :
																 Tr("Choose")});
		if (m_startStep > 0 || m_startFromSettings)
			hints.push_back({Icon::Circle, Tr("Back")});
		return hints;
	}

	// -- drawing -------------------------------------------------------------------------------------

	Box Shell::StartCard(Canvas& canvas, const Box& box, bool focused, bool chosen, std::string_view key)
	{
		// a choice: glass that rises a little with the focus; a check at its corner on what is set now
		const Box drawn = box.Scaled(1.0f + 0.025f * FocusMix(FocusKey(key), focused));
		if (focused)
			canvas.Shadow(drawn, kRadiusSheet, 24, 60, LightGround() ? 0x33000000 : 0x66000000);
		Glass(canvas, drawn, kRadiusSheet, focused ? Surface2() : Surface(), focused ? 1.0f : 0.6f);
		if (focused && !m_settings.ui.highContrast)
			canvas.Rect(drawn, kRadiusSheet, ui::SetAlpha(Accent(), LightGround() ? 0.06f : 0.10f));
		if (chosen)
		{
			const Box badge{drawn.Right() - 24 - 36, drawn.y + 22, 36, 36};
			canvas.Rect(badge, 18, Accent());
			canvas.Draw(Icon::Check, badge.Inset(8), 0xffffffff);
		}
		if (focused)
			Focus(drawn, kRadiusSheet);
		return drawn;
	}

	void Shell::StartHeader(Canvas& canvas, const std::string& question, const std::string& line)
	{
		const std::vector<StartStep> steps = StartSteps();
		canvas.PushAlpha(Enter(0));
		DrawMark(canvas, kSafeX + 30, 76, 30);
		DrawWordmark(canvas, kSafeX + 74, 76, 28, kText);
		// how far along: each step a bar with its name under it, the steps done filled, this one bright
		const ui::TextStyle nameStyle = Style({18, ui::Weight::Medium, 1.0f});
		const ui::TextStyle onStyle = Style({18, ui::Weight::SemiBold, 1.0f});
		constexpr float kGap = 14, kLeast = 92;
		std::vector<float> widths;
		float total = 0;
		for (StartStep step : steps)
		{
			widths.push_back(std::max(kLeast, m_fonts.Width(onStyle, Tr(kStepNames[(int)step])) + 4));
			total += widths.back() + kGap;
		}
		float x = 1920 - kSafeX - (total - kGap);
		for (int i = 0; i < (int)steps.size(); i++)
		{
			const bool done = i < m_startStep, on = i == m_startStep;
			const uint32_t bar = on ? Accent() : done ? ui::SetAlpha(kText, 0.55f) : Wash(0.14f);
			canvas.Rect({x, 58, widths[i], 6}, 3, bar);
			canvas.Text(on ? onStyle : nameStyle, x, 76, Tr(kStepNames[(int)steps[i]]), on ? kText : done ? Secondary() : Tertiary(), widths[i] + 8, 1);
			x += widths[i] + kGap;
		}
		canvas.PopAlpha();
		// the question, and a line on it; they arrive from the side the step came from
		const float slide = (m_startForward ? 1 : -1) * 40.0f;
		canvas.PushAlpha(Enter(1));
		canvas.PushOffset((1 - Enter(1)) * slide, 0);
		canvas.Text(Style({56, ui::Weight::Bold, 1.1f, -0.8f}), kSafeX, 150, question, kText, 1920 - 2 * kSafeX, 1);
		canvas.Text(Style({24, ui::Weight::Regular, 1.35f}), kSafeX, 228, line, Secondary(), 1300, 2);
		canvas.PopOffset();
		canvas.PopAlpha();
	}

	void Shell::StartFooter(Canvas& canvas, const std::string& note)
	{
		// where it is changed later, at the left of the hints
		if (note.empty())
			return;
		canvas.PushAlpha(Enter(3));
		const float y = 1080 - kHintsBottom - 30;
		canvas.Draw(Icon::Gear, {kSafeX, y, 30, 30}, Tertiary());
		canvas.Text(Style({22, ui::Weight::Regular, 1.0f}), kSafeX + 42, y + 4, note, Tertiary(), 900, 1);
		canvas.PopAlpha();
	}

	void Shell::StartDraw(Canvas& canvas)
	{
		const std::vector<StartStep> steps = StartSteps();
		m_startStep = std::clamp(m_startStep, 0, (int)steps.size() - 1);
		const StartStep step = steps[m_startStep];
		const int focus = m_startFocus[(int)step];
		const float slide = (m_startForward ? 1 : -1) * 60.0f;
		auto body = [&](int part) {
			canvas.PushAlpha(Enter(part));
			canvas.PushOffset((1 - Enter(part)) * slide, 0);
		};
		auto bodyEnd = [&] {
			canvas.PopOffset();
			canvas.PopAlpha();
		};
		const ui::TextStyle nameStyle = Style({34, ui::Weight::SemiBold, 1.1f});
		const ui::TextStyle aboutStyle = Style({21, ui::Weight::Regular, 1.4f});

		switch (step)
		{
		case StartStep::Language:
		{
			// tr: the first start's first question
			StartHeader(canvas, Tr("Which language?"), Tr("Playtendoh speaks your PS5's language. Pick another if you like."));
			const std::vector<std::string> codes = StartLanguages();
			constexpr int kColumns = 6;
			constexpr float kGap = 16, kHeight = 72;
			const float width = (1920 - 2 * kSafeX - (kColumns - 1) * kGap) / kColumns;
			body(2);
			for (int i = 0; i < (int)codes.size(); i++)
			{
				// the PS5's own two cells wide, so its name and tag fit; the rest after it
				const int slot = i == 0 ? 0 : i + 1;
				const Box cell{kSafeX + (slot % kColumns) * (width + kGap), kTop + (slot / kColumns) * (kHeight + kGap), i == 0 ? 2 * width + kGap : width, kHeight};
				const bool chosen = codes[i] == m_settings.ui.language;
				const bool on = i == focus;
				const Box drawn = cell.Scaled(1.0f + 0.04f * FocusMix(FocusKey("start-language", i), on));
				Glass(canvas, drawn, drawn.h / 2, on ? Surface2() : Surface(), on ? 1.0f : 0.5f);
				const std::string name = ps5lang::NameOf(codes[i].empty() ? ps5lang::SystemLanguage() : codes[i]);
				float right = drawn.Right() - 24;
				if (codes[i].empty())
				{
					// tr: the first start's languages: a tag on the PS5's own language
					const std::string tag = Tr("PS5");
					const ui::TextBlock text = m_fonts.Layout(Style({16, ui::Weight::SemiBold, 1.0f}), tag);
					const Box pill{right - text.width - 20, drawn.CentreY() - 14, text.width + 20, 28};
					canvas.Rect(pill, 14, ui::SetAlpha(Accent(), 0.18f));
					canvas.Text(text, pill.x + 10, pill.CentreY() - text.height * 0.5f, Accent());
					right = pill.x - 10;
				}
				if (chosen)
				{
					canvas.Draw(Icon::Check, {drawn.x + 22, drawn.CentreY() - 12, 24, 24}, Accent());
				}
				const float tx = drawn.x + (chosen ? 56 : 28);
				canvas.Text(Style({22, ui::Weight::Medium, 1.0f}), tx, drawn.CentreY() - 12, name, kText, right - tx, 1);
				if (on)
					Focus(drawn, drawn.h / 2);
			}
			bodyEnd();
			StartFooter(canvas, Tr("Change it later in Settings > Language."));
			break;
		}
		case StartStep::Layout:
		{
			// tr: the first start's question: the Library's layouts (Classic, Orbit) or EmulationStation
			StartHeader(canvas, Tr("How do you want to browse your games?"), Tr("Your games, saves and settings are the same in each."));
			struct Card
			{
				const char* name;
				const char* about;
			};
			const Card cards[3] = {
				{Tr("Classic"), Tr("One console at a time. L1 and R1 switch consoles. Recently played on top, all games below.")},
				{Tr("Orbit"), Tr("Turn the dial to pick a console. The centre shows what you played lately on any console.")},
				// tr: the first start's layouts: EmulationStation, downloaded when the setup is done
				{Tr("EmulationStation"), Tr("ES-DE's menus and themes. Downloaded when you finish (about 850 MB); games still play here.")},
			};
			m_startShown += (focus - m_startShown) * std::min(1.0f, (m_settings.ui.reduceMotion ? 60.0f : 12.0f) * m_dt);
			body(2);
			const int chosen = m_settings.ui.emulationStation ? 2 : m_settings.ui.layout;
			for (int i = 0; i < 3; i++)
			{
				const float cardX = 570.0f + (i - m_startShown) * 840.0f;
				if (cardX > 1920 || cardX + 780 < 0)
					continue;
				const bool on = i == focus;
				canvas.PushAlpha(on ? 1.0f : 0.55f);
				const Box card = StartCard(canvas, {cardX, kTop, 780, 590}, on, i == chosen && m_settings.ui.layoutChosen, std::string("start-layout") + char('0' + i));
				const Box preview{card.x + 30, card.y + 30, 720, 405};
				canvas.Rect(preview, 16, 0xff0d0705);
				if (i == 0)
					LayoutClassicPreview(canvas, preview);
				else if (i == 1)
					LayoutOrbitPreview(canvas, preview);
				else
					LayoutEsdePreview(canvas, preview);
				canvas.Ring(preview, 16, 1, 0x14ffffff);
				canvas.Text(nameStyle, card.x + 36, card.y + 458, cards[i].name, kText, card.w - 72, 1);
				canvas.Text(aboutStyle, card.x + 36, card.y + 504, cards[i].about, Secondary(), card.w - 72, 2);
				canvas.PopAlpha();
			}
			bodyEnd();
			StartFooter(canvas, Tr("Change it later in Settings > Launcher > Layout."));
			break;
		}
		case StartStep::Buttons:
		{
			// tr: the first start's question: which of the PS5's buttons is a Nintendo controller's A
			StartHeader(canvas, Tr("Which button is A?"), Tr("For every game. In Playtendoh's menus, Cross always chooses."));
			const float width = 760, gap = 48, left = (1920 - 2 * width - gap) * 0.5f;
			const int chosen = ps5azahar::MappedInput(m_settings.n3ds, ps5azahar::Button::A) == ps5emu::PadInput::Circle ? 0 : 1;
			body(2);
			for (int i = 0; i < 2; i++)
			{
				const bool on = i == focus;
				const Box card = StartCard(canvas, {left + i * (width + gap), kTop + 10, width, 480}, on, i == chosen, std::string("start-buttons") + char('0' + i));
				// the face buttons as a diamond, each with the Nintendo letter it plays
				const float cx = card.CentreX(), cy = card.y + 180, apart = 82, size = 70;
				struct Face
				{
					Icon icon;
					float dx, dy;
					const char* letter;
				};
				const bool aOnCircle = i == 0;
				const Face faces[4] = {{Icon::Triangle, 0, -1, aOnCircle ? "X" : "Y"}, {Icon::Circle, 1, 0, aOnCircle ? "A" : "B"},
					{Icon::Cross, 0, 1, aOnCircle ? "B" : "A"}, {Icon::Square, -1, 0, aOnCircle ? "Y" : "X"}};
				for (const Face& face : faces)
				{
					const Box button{cx + face.dx * apart - size / 2, cy + face.dy * apart - size / 2, size, size};
					const bool a = face.letter[0] == 'A';
					canvas.Rect(button, size / 2, a ? Accent() : Wash(0.10f));
					if (!a)
						canvas.Ring(button, size / 2, 1.5f, Wash(0.22f));
					canvas.Draw(face.icon, button.Inset(22), a ? 0xffffffff : Secondary());
					// its letter, outside the diamond
					const ui::TextBlock letter = m_fonts.Layout(Style({26, ui::Weight::Bold, 1.0f}), face.letter);
					const float lx = button.CentreX() + face.dx * 64 - letter.width * 0.5f;
					const float ly = button.CentreY() + face.dy * 58 - letter.height * 0.5f;
					canvas.Text(letter, lx, ly, a ? Accent() : Tertiary());
				}
				// tr: the first start's buttons question: the two choices
				canvas.Text(nameStyle, card.x + 36, card.y + 360, i == 0 ? Tr("Circle is A") : Tr("Cross is A"), kText, card.w - 72, 1);
				canvas.Text(aboutStyle, card.x + 36, card.y + 406,
					i == 0 ? Tr("Where A is on a Nintendo controller: on the right.") : Tr("Where most PlayStation games confirm: at the bottom."),
					Secondary(), card.w - 72, 2);
			}
			bodyEnd();
			StartFooter(canvas, Tr("Change it later in each system's Controls."));
			break;
		}
		case StartStep::Reports:
		{
			StartHeader(canvas, Tr("Help Playtendoh get better?"),
				Tr("When a game ends, Playtendoh can send a short report on how it ran. Nothing is sent while you play."));
			const float width = 760, gap = 48, left = (1920 - 2 * width - gap) * 0.5f;
			body(2);
			for (int i = 0; i < 2; i++)
			{
				const bool on = i == focus;
				const Box card = StartCard(canvas, {left + i * (width + gap), kTop + 10, width, 300}, on, m_settings.ui.reportsAsked && m_settings.ui.reports == (i == 0),
					std::string("start-reports") + char('0' + i));
				const Box icon{card.x + 36, card.y + 40, 64, 64};
				canvas.Rect(icon, 32, ui::SetAlpha(i == 0 ? kGood : Tertiary(), 0.16f));
				canvas.Draw(i == 0 ? Icon::Heart : Icon::Dash, icon.Inset(16), i == 0 ? kGood : Tertiary());
				canvas.Text(nameStyle, card.x + 36, card.y + 130, i == 0 ? Tr("Yes, send reports") : Tr("Not now"), kText, card.w - 72, 1);
				canvas.Text(aboutStyle, card.x + 36, card.y + 176,
					i == 0 ? Tr("The game, how it ran and its settings, to fix problems and grow the list of working games.") :
							 Tr("Nothing is sent. You can turn it on any time."),
					Secondary(), card.w - 72, 2);
			}
			// what never is, and every field on a phone
			const Box strip{left, kTop + 10 + 300 + 32, 2 * width + gap, 200};
			Glass(canvas, strip, kRadiusSheet, Surface(), 0.5f);
			const float qr = 136;
			const std::string address = ps5reports::kExplained;
			auto code = m_qrCodes.find(address);
			if (code == m_qrCodes.end())
				code = m_qrCodes.emplace(address, ui::QrCode(address)).first;
			const Box qrBox{strip.Right() - 32 - qr, strip.CentreY() - qr / 2, qr, qr};
			canvas.Rect(qrBox.Inset(-8), 12, 0xffffffff);
			ui::DrawQr(canvas, code->second, qrBox, 8);
			const float tx = strip.x + 36, tw = qrBox.x - 40 - tx;
			canvas.Text(Style({24, ui::Weight::SemiBold, 1.2f}), tx, strip.y + 34, Tr("What a report never has"), kText, tw, 1);
			canvas.Text(aboutStyle, tx, strip.y + 76,
				Tr("Your PSN name or ID, your console's serial or addresses, your accounts, folders, saves or game list."), Secondary(), tw, 2);
			canvas.Text(Style({19, ui::Weight::Regular, 1.3f}), tx, strip.Bottom() - 50, Tr("Scan the code for every field a report has."), Tertiary(), tw, 1);
			bodyEnd();
			StartFooter(canvas, Tr("Change it later in Settings > Accounts and updates > Send reports."));
			break;
		}
		case StartStep::Easier: StartEasierDraw(canvas); break;
		case StartStep::Finding: StartFindingDraw(canvas); break;
		case StartStep::Files: StartFilesDraw(canvas); break;
		case StartStep::Ready: StartReadyDraw(canvas); break;
		}
	}

	// -- the steps drawn in a function of their own ------------------------------------------------------

	void Shell::StartEasierDraw(Canvas& canvas)
	{
		const int focus = m_startFocus[(int)StartStep::Easier];
		// tr: the first start's question: larger text, high contrast, less motion
		StartHeader(canvas, Tr("Make Playtendoh easier to see?"),
			Tr("Turn on whatever helps, or go on. Triangle makes the text larger on any of these screens."));
		const float slide = (m_startForward ? 1 : -1) * 60.0f;
		canvas.PushAlpha(Enter(2));
		canvas.PushOffset((1 - Enter(2)) * slide, 0);
		struct Card
		{
			const char* name;
			const char* about;
			bool on;
		};
		const Card cards[3] = {
			{Tr("Larger text"), Tr("Every word a size larger, in every menu."), m_settings.ui.largerText},
			{Tr("High contrast"), Tr("Solid panels, text at full strength and a thicker focus ring."), m_settings.ui.highContrast},
			{Tr("Reduce motion"), Tr("Fades instead of movement, and a background that keeps still."), m_settings.ui.reduceMotion},
		};
		constexpr float kGap = 40, kHeight = 430;
		const float width = (1920 - 2 * kSafeX - 2 * kGap) / 3;
		for (int i = 0; i < 3; i++)
		{
			const bool on = focus == i;
			const Box card = StartCard(canvas, {kSafeX + i * (width + kGap), kTop, width, kHeight}, on, false, std::string("start-easier") + char('0' + i));
			// what it does, shown: the letters larger, ink on paper, a ball that keeps still
			const Box art{card.x + 32, card.y + 32, card.w - 64, 140};
			canvas.Rect(art, 16, Wash(0.06f));
			if (i == 0)
			{
				const ui::TextBlock small = m_fonts.Layout({44, ui::Weight::SemiBold, 1.0f}, "Aa");
				const ui::TextBlock large = m_fonts.Layout({92, ui::Weight::Bold, 1.0f}, "Aa");
				const float x = art.CentreX() - (small.width + 36 + large.width) * 0.5f;
				canvas.Text(small, x, art.CentreY() + large.height * 0.32f - small.height, Secondary());
				canvas.Text(large, x + small.width + 36, art.CentreY() - large.height * 0.5f, kText);
			}
			else if (i == 1)
			{
				const float size = 96;
				const Box dark{art.CentreX() - size - 8, art.CentreY() - size / 2, size, size}, light{art.CentreX() + 8, art.CentreY() - size / 2, size, size};
				canvas.Rect(dark, 18, 0xff000000);
				canvas.Rect(light, 18, 0xffffffff);
				canvas.Ring(light, 18, 2, 0xff000000);
				const ui::TextBlock letter = m_fonts.Layout({56, ui::Weight::Bold, 1.0f}, "A");
				canvas.Text(letter, dark.CentreX() - letter.width / 2, dark.CentreY() - letter.height / 2, 0xffffffff);
				canvas.Text(letter, light.CentreX() - letter.width / 2, light.CentreY() - letter.height / 2, 0xff000000);
			}
			else
			{
				// a ball and its fading trail, the trail gone when motion is reduced
				const float r = 26, cy = art.CentreY();
				for (int t = 3; t >= 1 && !m_settings.ui.reduceMotion; t--)
					canvas.Rect({art.CentreX() - r - t * 46.0f, cy - r, 2 * r, 2 * r}, r, ui::SetAlpha(Accent(), 0.10f * (4 - t)));
				canvas.Rect({art.CentreX() - r, cy - r, 2 * r, 2 * r}, r, Accent());
			}
			const float tx = card.x + 36, tw = card.w - 72;
			canvas.Text(Style({32, ui::Weight::SemiBold, 1.1f}), tx, card.y + 200, cards[i].name, kText, tw, 1);
			canvas.Text(Style({21, ui::Weight::Regular, 1.4f}), tx, card.y + 248, cards[i].about, Secondary(), tw, 3);
			DrawToggle(canvas, tx, card.Bottom() - 36 - 36, cards[i].on, on);
			canvas.Text(Style({22, ui::Weight::SemiBold, 1.0f}), tx + 64 + 16, card.Bottom() - 36 - 31, cards[i].on ? Tr("On") : Tr("Off"),
				cards[i].on ? kText : Secondary(), tw - 80, 1);
		}
		// and on, below them, where the focus starts
		const std::string label = Tr("Continue");
		const float buttonWidth = m_fonts.Width(Style({28, ui::Weight::SemiBold, 1.0f}), label) + 2 * 72 * 0.55f;
		PillButton(canvas, 960 - buttonWidth / 2, kTop + kHeight + 52, label, true, focus == 3);
		canvas.PopOffset();
		canvas.PopAlpha();
		StartFooter(canvas, Tr("Change it later in Settings > Accessibility."));
	}

	void Shell::StartFindingDraw(Canvas& canvas)
	{
		// tr: the first start, while Playtendoh looks for games on the drives and in /data
		StartHeader(canvas, Tr("Looking for your games"),
			Tr("On your USB drives and the PS5's storage, in any folder. It takes a moment the first time."));
		const float slide = (m_startForward ? 1 : -1) * 60.0f;
		canvas.PushAlpha(Enter(2));
		canvas.PushOffset((1 - Enter(2)) * slide, 0);
		const std::vector<ps5discover::Root> roots = DiscoverRoots();
		const std::string where = ps5discover::Where();
		int at = -1;
		for (int i = 0; i < (int)roots.size(); i++)
			if (roots[i].folder == where)
				at = i;
		constexpr float kRow = 84;
		const Box panel{kSafeX, kTop, 1040, 56 + roots.size() * kRow};
		Glass(canvas, panel, kRadiusSheet, Surface(), 0.7f);
		const std::string data = ConsoleRoot() + "/data";
		for (int i = 0; i < (int)roots.size(); i++)
		{
			const std::string& folder = roots[i].folder;
			const bool drive = folder.starts_with(ConsoleRoot() + "/mnt/");
			const float y = panel.y + 28 + i * kRow;
			const Box icon{panel.x + 32, y + 14, 52, 52};
			canvas.Rect(icon, 26, Wash(0.08f));
			canvas.Draw(drive ? (folder.find("/usb") != std::string::npos ? Icon::Usb : Icon::Drive) : Icon::Folder, icon.Inset(13), kText);
			// tr: the first start's look: the PS5's own storage, as a row of where it looks
			const std::string name = drive ? DriveName(folder) : folder == data ? std::string(Tr("The PS5's storage (/data)")) : PlaceName(folder);
			canvas.Text(Style({24, ui::Weight::SemiBold, 1.0f}), icon.Right() + 20, y + 26, name, kText, 640, 1);
			// how far it is: looked through, being looked through, next
			const Box state{panel.Right() - 32 - 40, y + 20, 40, 40};
			if (at > i || (at < 0 && !ps5discover::Busy()))
			{
				canvas.Rect(state, 20, ui::SetAlpha(kGood, 0.16f));
				canvas.Draw(Icon::Check, state.Inset(9), kGood);
			}
			else if (at == i)
			{
				// a ring of dots, turning (still when motion is reduced)
				const float turn = m_settings.ui.reduceMotion ? 0.0f : (float)std::fmod(m_now * 1.2, 1.0);
				for (int k = 0; k < 8; k++)
				{
					const float angle = 6.2831853f * k / 8, fade = std::fmod(1.0f + k / 8.0f - turn, 1.0f);
					const float dx = std::cos(angle) * 14, dy = std::sin(angle) * 14;
					canvas.Rect({state.CentreX() + dx - 4, state.CentreY() + dy - 4, 8, 8}, 4, ui::SetAlpha(Accent(), 0.25f + 0.75f * fade));
				}
			}
			else
				canvas.Draw(Icon::Dash, state.Inset(10), Tertiary());
		}
		// beside it: how much it has looked at, and what happens to what it finds
		const Box aside{panel.Right() + 40, kTop, 1920 - kSafeX - panel.Right() - 40, 360};
		Glass(canvas, aside, kRadiusSheet, Surface(), 0.7f);
		const float ax = aside.x + 36, aw = aside.w - 72;
		canvas.Text(Style(kOverlineStyle), ax, aside.y + 34, Tr("So far"), Secondary(), aw, 1);
		canvas.Text(Style({44, ui::Weight::Bold, 1.0f}), ax, aside.y + 72, ps5lang::Decimal(ps5discover::Looked(), 0), kText, aw, 1);
		// tr: under a number: how many files and folders the look has seen so far
		canvas.Text(Style({21, ui::Weight::Regular, 1.3f}), ax, aside.y + 132, Tr("files and folders looked at"), Secondary(), aw, 1);
		canvas.Text(Style({21, ui::Weight::Regular, 1.45f}), ax, aside.y + 200,
			Tr("Games are played where they are. Nothing is moved, renamed or deleted."), Secondary(), aw, 4);
		canvas.PopOffset();
		canvas.PopAlpha();
		StartFooter(canvas, Tr("Settings > Finding games looks again any time."));
	}

	void Shell::StartFilesDraw(Canvas& canvas)
	{
		const int focus = m_startFocus[(int)StartStep::Files];
		// tr: the first start's question: the BIOS files, keys and firmware found, copied where their emulators read them
		StartHeader(canvas, Tr("Copy your keys and BIOS into place?"),
			Tr("Their emulators only read them from folders of their own. Copies go there: yours stay where they are, and nothing already there is replaced."));
		const float slide = (m_startForward ? 1 : -1) * 60.0f;
		canvas.PushAlpha(Enter(2));
		canvas.PushOffset((1 - Enter(2)) * slide, 0);
		const float width = 760, gap = 48, left = (1920 - 2 * width - gap) * 0.5f, height = 500;
		for (int i = 0; i < 2; i++)
		{
			const bool on = focus == i;
			const Box card = StartCard(canvas, {left + i * (width + gap), kTop + 10, width, height}, on, false, std::string("start-files") + char('0' + i));
			const Box icon{card.x + 36, card.y + 40, 64, 64};
			canvas.Rect(icon, 32, ui::SetAlpha(i == 0 ? kGood : Tertiary(), 0.16f));
			canvas.Draw(i == 0 ? Icon::Download : Icon::Dash, icon.Inset(16), i == 0 ? kGood : Tertiary());
			// tr: the first start's files question: the two choices
			canvas.Text(Style({34, ui::Weight::SemiBold, 1.1f}), card.x + 36, card.y + 130, i == 0 ? Tr("Copy them") : Tr("Not now"), kText, card.w - 72, 1);
			if (i == 1)
			{
				canvas.Text(Style({21, ui::Weight::Regular, 1.4f}), card.x + 36, card.y + 180, Tr("Settings > Finding games can copy them any time."), Secondary(),
					card.w - 72, 2);
				continue;
			}
			// what, and from where
			float y = card.y + 186;
			for (size_t f = 0; f < m_startFiles.size() && f < 5; f++)
			{
				const ps5discover::File& file = m_startFiles[f];
				canvas.Rect({card.x + 40, y + 10, 8, 8}, 4, Secondary());
				const ui::TextBlock what = canvas.Text(Style({22, ui::Weight::SemiBold, 1.0f}), card.x + 62, y, FileWords(file), kText, 330, 1);
				const std::string from = file.kind == ps5discover::File::Kind::Firmware ? file.from : file.from.substr(0, file.from.find_last_of('/'));
				canvas.Text(Style({19, ui::Weight::Regular, 1.0f}), card.x + 62 + what.width + 14, y + 2, ShortPath(PlaceName(from), 36), Secondary(),
					card.Right() - 36 - (card.x + 62 + what.width + 14), 1);
				y += 50;
			}
			if (m_startFiles.size() > 5)
				canvas.Text(Style({19, ui::Weight::Regular, 1.0f}), card.x + 62, y, TrP((long long)m_startFiles.size() - 5, "and {0} more", "and {0} more"), Secondary(),
					card.w - 98, 1);
		}
		canvas.PopOffset();
		canvas.PopAlpha();
		StartFooter(canvas, Tr("Settings > Setup check shows each file and where it goes."));
	}

	void Shell::StartReadyDraw(Canvas& canvas)
	{
		const int focus = m_startFocus[(int)StartStep::Ready];
		const std::vector<const Check*> needs = StartNeeds();
		const ui::TextStyle nameStyle = Style({34, ui::Weight::SemiBold, 1.1f});
		const ui::TextStyle aboutStyle = Style({21, ui::Weight::Regular, 1.4f});
		// the games found, a console each: in its own folder and wherever else they were found
		struct Found
		{
			System side;
			int games;
			std::string where;
		};
		std::vector<Found> found;
		int total = 0;
		for (const Check& check : m_checks)
			if (check.id.ends_with("-games"))
			{
				const System side = ps5launcher::SideNamed(check.id.substr(0, check.id.size() - 6));
				const int games = std::max(check.games, FoundGames(side));
				if (games > 0)
				{
					found.push_back({side, games, FoundWhere(side)});
					total += games;
				}
			}
		// tr: the first start's last step: a summary, nothing to answer
		StartHeader(canvas, !needs.empty() ? Tr("Almost ready") : total > 0 ? Tr("You're ready to play") : Tr("Now add your games"),
			!needs.empty() ? TrP((long long)needs.size(), "One thing to sort out before some games will play.", "{0} things to sort out before some games will play.") :
			total > 0	   ? TrP(total, "Found {0} game. Every system with games is in the bar.", "Found {0} games. Every system with games is in the bar.") :
							 Tr("Everything else Playtendoh needs is here."));
		const float slide = (m_startForward ? 1 : -1) * 60.0f;
		canvas.PushAlpha(Enter(2));
		canvas.PushOffset((1 - Enter(2)) * slide, 0);
		const Box games{kSafeX, kTop, 1000, 520};
		Glass(canvas, games, kRadiusSheet, Surface(), 0.7f);
		canvas.Text(Style(kOverlineStyle), games.x + 36, games.y + 34, Tr("Your games"), Secondary(), games.w - 72, 1);
		const bool looking = ps5discover::Busy();
		if (found.empty())
		{
			const Box icon{games.x + 36, games.y + 90, 72, 72};
			canvas.Rect(icon, 36, Wash(0.08f));
			canvas.Draw(looking ? Icon::Search : Icon::Folder, icon.Inset(18), Secondary());
			canvas.Text(nameStyle, games.x + 36, games.y + 190, looking ? Tr("Still looking…") : Tr("No games found yet"), kText, games.w - 72, 1);
			canvas.Text(aboutStyle, games.x + 36, games.y + 238,
				// tr: {0} is the drop folder, /data/playtendoh
				TrF("Put your games anywhere on a USB drive or in {0}, in any folders: each console's in one, or all together. Playtendoh finds them each time it starts, and when you plug in a drive.",
					"/data/playtendoh"),
				Secondary(), games.w - 72, 5);
		}
		else
		{
			constexpr int kColumns = 3;
			const float cellW = (games.w - 72) / kColumns, cellH = 112;
			for (int i = 0; i < (int)found.size() && i < 12; i++)
			{
				const float x = games.x + 36 + (i % kColumns) * cellW, y = games.y + 82 + (i / kColumns) * cellH;
				DrawBareConsole(canvas, found[i].side, {x, y + 8, 76, 76}, true);
				canvas.Text(Style({22, ui::Weight::SemiBold, 1.0f}), x + 94, y + 18, SystemName(found[i].side), kText, cellW - 108, 1);
				canvas.Text(Style({19, ui::Weight::Regular, 1.0f}), x + 94, y + 50, TrP(found[i].games, "{0} game", "{0} games", found[i].games), Secondary(),
					cellW - 108, 1);
				if (!found[i].where.empty())
					canvas.Text(Style({17, ui::Weight::Regular, 1.0f}), x + 94, y + 76, ShortPath(found[i].where, 30), Tertiary(), cellW - 108, 1);
			}
			canvas.Text(aboutStyle, games.x + 36, games.Bottom() - 62,
				looking ? DiscoverStatus() + " " + Tr("What it finds joins the Library.") :
						  std::string(Tr("Games are played where they are. Add more anywhere on a drive, any time.")),
				Secondary(), games.w - 72, 1);
		}
		// what stands in the way, or that nothing does
		const Box side{kSafeX + 1000 + 40, kTop, 1920 - kSafeX - (kSafeX + 1000 + 40), 520};
		Glass(canvas, side, kRadiusSheet, Surface(), 0.7f);
		canvas.Text(Style(kOverlineStyle), side.x + 36, side.y + 34, needs.empty() ? Tr("Checked") : Tr("Before you play"), needs.empty() ? kGood : kWarn,
			side.w - 72, 1);
		float y = side.y + 76;
		if (needs.empty())
		{
			const Box icon{side.x + 36, y + 10, 72, 72};
			canvas.Rect(icon, 36, ui::SetAlpha(kGood, 0.16f));
			canvas.Draw(Icon::Check, icon.Inset(18), kGood);
			canvas.Text(Style({24, ui::Weight::SemiBold, 1.2f}), side.x + 36, y + 110, Tr("Storage and game speed are fine"), kText, side.w - 72, 2);
			y += 160;
		}
		for (int i = 0; i < (int)needs.size() && i < 3; i++)
		{
			const Check& check = *needs[i];
			const bool on = i == focus;
			const Box row{side.x + 20, y, side.w - 40, 84};
			if (on)
			{
				canvas.Rect(row, 18, ui::SetAlpha(Accent(), LightGround() ? 0.08f : 0.14f));
				Focus(row, 18);
			}
			const Box icon{row.x + 16, row.CentreY() - 20, 40, 40};
			canvas.Rect(icon, 20, ui::SetAlpha(kWarn, 0.16f));
			canvas.Draw(Icon::Warning, icon.Inset(9), kWarn);
			canvas.Text(Style({22, ui::Weight::SemiBold, 1.1f}), icon.Right() + 16, row.y + 14, check.title, kText, row.Right() - icon.Right() - 32, 1);
			canvas.Text(Style({18, ui::Weight::Regular, 1.2f}), icon.Right() + 16, row.y + 46, check.detail, Secondary(), row.Right() - icon.Right() - 32, 1);
			y += 92;
		}
		// the files being copied, and EmulationStation's download, said before it starts
		if (m_discovery.copying)
		{
			y += 8;
			canvas.Draw(Icon::Download, {side.x + 36, y + 2, 28, 28}, Secondary());
			// tr: the first start's summary, while the files found are copied into place: {0} is how far, "40%"
			canvas.Text(Style({19, ui::Weight::Regular, 1.35f}), side.x + 76, y, TrF("Copying your files into place: {0}", ps5lang::Percent((int)(ps5discover::CopyFraction() * 100))),
				Secondary(), side.w - 112, 2);
			y += 44;
		}
		if (m_settings.ui.emulationStation && !ps5esde::Available())
		{
			y += 8;
			canvas.Draw(Icon::Download, {side.x + 36, y + 2, 28, 28}, Secondary());
			canvas.Text(Style({19, ui::Weight::Regular, 1.35f}), side.x + 76, y,
				Tr("EmulationStation downloads when you start (about 850 MB). It opens next time Playtendoh starts."), Secondary(), side.w - 112, 3);
		}
		// the focused one's guide, on a phone
		if (!needs.empty())
		{
			const Check& check = *needs[std::clamp(focus, 0, (int)needs.size() - 1)];
			const Box qrBox{side.x + 36, side.Bottom() - 36 - kQr, kQr, kQr};
			auto code = m_qrCodes.find(check.guide);
			if (code == m_qrCodes.end())
				code = m_qrCodes.emplace(check.guide, ui::QrCode(check.guide)).first;
			canvas.Rect(qrBox.Inset(-8), 12, 0xffffffff);
			ui::DrawQr(canvas, code->second, qrBox, 8);
			const float tx = qrBox.Right() + 28, tw = side.Right() - 36 - tx;
			const ui::TextBlock title = m_fonts.Layout(Style({22, ui::Weight::SemiBold, 1.3f}), check.aboutTitle, tw, 2);
			const ui::TextBlock about = m_fonts.Layout(Style({18, ui::Weight::Regular, 1.4f}), Tr("Scan it for how, on your phone."), tw, 2);
			const float ty = qrBox.y + (kQr - title.height - 6 - about.height) * 0.5f;
			canvas.Text(title, tx, ty, kText);
			canvas.Text(about, tx, ty + title.height + 6, Secondary());
		}
		canvas.PopOffset();
		canvas.PopAlpha();
		StartFooter(canvas, Tr("Settings > Setup check has the details any time."));
	}
}
