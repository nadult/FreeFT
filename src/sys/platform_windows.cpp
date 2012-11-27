#include "sys/platform.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <cstring>

const Path &Path::current() {
	//TODO: write me
	return Path(".");
}

bool Path::isRegularFile() const {
	//TODO: write me
	return false;
}

bool Path::isDirectory() const {
	//TODO: write me
	return false;
}

static void findFiles(vector<FileEntry> &out, const Path &path, const Path &append, int flags) {
	//TODO: write me
}

void findFiles(vector<FileEntry> &out, const Path &path, int flags) {
	Path append =	flags & FindFiles::relative? "." :
					flags & FindFiles::absolute? path.absolute() : path;

	findFiles(out, path.absolute(), append, flags);
}

