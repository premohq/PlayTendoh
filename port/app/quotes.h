// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: words from the people who made Nintendo's games, for the loading screens while a game's
// shaders are made ready (loading_screen.h, every side's, and Porpoise X's, handed them in its session file:
// retro/porpoise_settings.h). One after another, from a different one each start. Each as it was said in
// English, or as it was translated into English, with who said it and where where that is known. One is
// famous under Miyamoto's name though no source of it has been found: it says so ("credited").
// Needs nothing of the menus' drawing, so the launcher includes it too.
#pragma once

#include "lang.h"

#include <cstddef>
#include <ctime>
#include <iterator>
#include <string>

namespace ps5quotes
{
	struct Quote
	{
		const char* words;	   // marked for translation
		const char* who;	   // a name, the same in every language
		const char* where;	   // where it was said ("GDC 2005"); empty: not known
		bool credited = false; // long credited to them, its source not found
	};

	// tr: a quote on a game's loading screen, from someone who made Nintendo's games (kept in English where its
	// tr: words are a famous line, such as "My body is ready.")
	inline constexpr Quote kQuotes[] = {
		// Satoru Iwata, his keynote "Heart of a Gamer"
		{ps5lang::TrMark("On my business card, I am a corporate president. In my mind, I am a game developer. But in my heart, I am a gamer."),
			"Satoru Iwata", "GDC 2005"},
		{ps5lang::TrMark("Above all, video games are meant to be just one thing: fun."), "Satoru Iwata", ""},
		// as Iwata often told it, from Miyamoto
		{ps5lang::TrMark("A good idea is something that does not solve just one single problem, but rather can solve multiple problems at once."),
			"Shigeru Miyamoto", ""},
		{ps5lang::TrMark("A delayed game is eventually good, but a rushed game is forever bad."), "Shigeru Miyamoto", "", true},
		{ps5lang::TrMark("If it's really nice outside, you should go play outside."), "Shigeru Miyamoto", ""},
		{ps5lang::TrMark("My name is Reggie. I'm about kicking ass, I'm about taking names, and we're about making games."), "Reggie Fils-Aimé",
			"E3 2004"},
		{ps5lang::TrMark("My body is ready."), "Reggie Fils-Aimé", "E3 2007"},
		// from a Nikkei xTREND interview, translated
		{ps5lang::TrMark("Creating what you love is the easiest way to grasp how to make something entertaining."), "Masahiro Sakurai", ""},
		// his way with the Game & Watch and the Game Boy: 枯れた技術の水平思考
		{ps5lang::TrMark("Lateral thinking with withered technology."), "Gunpei Yokoi", ""},
	};
	inline constexpr size_t kCount = std::size(kQuotes);

	// Its words in the menus' language, in that language's quotation marks
	inline std::string Words(const Quote& quote)
	{
		// tr: a quote's words in quotation marks, on a game's loading screen: use your language's marks
		return ps5lang::TrF("“{0}”", ps5lang::Tr(quote.words));
	}

	// Who said it, and where
	inline std::string Credit(const Quote& quote)
	{
		// tr: under a quote on a game's loading screen, for a line long said to be theirs whose source is not known: {0} is the name
		std::string credit = quote.credited ? ps5lang::TrF("Often credited to {0}", quote.who) : std::string(quote.who);
		if (*quote.where)
			credit += std::string("  ·  ") + quote.where;
		return credit;
	}

	// The one to show first for a game being started: a different one each start
	inline size_t First()
	{
		return (size_t)std::time(nullptr) % kCount;
	}
}
