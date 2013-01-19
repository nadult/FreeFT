/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "sys/platform.h"
#include <cstring>
#include <cstdio>
#include <windows.h>
#include <sys/stat.h>

Path::Element Path::extractRoot(const char *str) {
	if((str[0] >= 'a' && str[0] <= 'z') || (str[0] >= 'A' && str[0] <= 'Z'))
		if(str[1] == ':' && (str[2] == '/' || str[2] == '\\'))
			return Element{str, 3};

	return Element{nullptr, 0};
}

const Path Path::current() {
	char buf[MAX_PATH];
	GetCurrentDirectory(sizeof(buf), buf);
	return Path(buf);
}

bool Path::isRegularFile() const {
	struct _stat buf;
	_stat(c_str(), &buf);
	return S_ISREG(buf.st_mode);
}

bool Path::isDirectory() const {
	struct _stat buf;
	_stat(c_str(), &buf);
	return S_ISDIR(buf.st_mode);
}

static void findFiles(vector<FileEntry> &out, const Path &path, const Path &append, int flags) {
	WIN32_FIND_DATA data;
	char tpath[MAX_PATH];
	snprintf(tpath, sizeof(tpath), "%s/*", path.c_str());
	HANDLE handle = FindFirstFile(tpath, &data);
	
	bool is_root = path.isRoot();

	if(handle != INVALID_HANDLE_VALUE) do {
		if(strcmp(data.cFileName, ".") == 0)
			continue;

		bool is_dir  = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		bool is_file = !is_dir;
			
		bool do_accept =( (flags & FindFiles::regular_file) && is_file) ||
						( (flags & FindFiles::directory)    && is_dir);

		if(do_accept && is_root && is_dir && strcmp(data.cFileName, "..") == 0)
			do_accept = false;
		
		if(do_accept) {
			FileEntry entry;
			entry.path = append / Path(data.cFileName);
			entry.is_dir = is_dir;
			out.push_back(entry);
		}
			
		if(is_dir && (flags & FindFiles::recursive) && strcmp(data.cFileName, ".."))
			findFiles(out, path / Path(data.cFileName), append / Path(data.cFileName), flags);
	} while(FindNextFile(handle, &data));
}

void findFiles(vector<FileEntry> &out, const Path &path, int flags) {
	Path append =	flags & FindFiles::relative? "." :
					flags & FindFiles::absolute? path.absolute() : path;

	findFiles(out, path.absolute(), append, flags);
}

