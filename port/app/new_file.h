// SPDX-License-Identifier: GPL-3.0-or-later
// PS5-ECAM: a file made only where there is none, never one emptied. The console's fopen is Sony's
// libc, a Dinkumware stdio (_Fopen, _Foprep), which reads a mode's 'r', 'w' or 'a' and then '+' and 'b'
// and stops at anything else: C11's "wx" opens as "w" there, and "w" empties the file that is there.
// 5.1.0's start did that to the Wii U's keys.txt every time. Here the kernel decides instead: open with
// O_CREAT and O_EXCL refuses a file that exists, on the console as on a PC. C++17, as melonDS's build
// includes it.
#pragma once
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ps5files
{
	// open for writing a file that is not there yet; null when it is (or anything is in its place), or
	// when it cannot be made. What is there is never opened, so never emptied
	inline FILE* CreateNew(const char* path)
	{
		// a path stat can see, or one it is refused, is left alone: only "no such file" goes on
		struct stat status;
		if (::stat(path, &status) == 0 || errno != ENOENT)
			return nullptr;
		const int fd = ::open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
		if (fd < 0)
			return nullptr;
		FILE* file = ::fdopen(fd, "w");
		if (!file)
			::close(fd);
		return file;
	}
}
