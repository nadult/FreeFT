#include "sys/platform.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <cstring>

#ifndef _DIRENT_HAVE_D_TYPE
#error dirent without d_type not supported
#endif


const Path Path::current() {
	char *pwd = getenv("PWD");
	ASSERT(pwd && pwd[0] == '/');
	return Path(pwd);
}

bool Path::isRegularFile() const {
	struct stat buf;
	if(lstat(c_str(), &buf) != 0)
		return false;

	return S_ISREG(buf.st_mode);
}

bool Path::isDirectory() const {
	struct stat buf;
	if(lstat(c_str(), &buf) != 0)
		return false;

	return S_ISDIR(buf.st_mode);
}

void findFiles(vector<FileEntry> &out, const Path &path, FindFilesFilter filter, int flags) {
	DIR *dp = opendir(path.c_str());
	if(!dp)
		THROW("Error while opening directory %s: %s", path.c_str(), strerror(errno));

	try {
		struct dirent *dirp;

		while ((dirp = readdir(dp))) {
			if(strcmp(dirp->d_name, ".") == 0)
				continue;

			bool do_accept =	( (flags & FindFiles::regular_file) && dirp->d_type == DT_REG ) ||
								( (flags & FindFiles::directory) && dirp->d_type == DT_DIR);

			if(do_accept && (!filter || filter(path.c_str(), dirp->d_name, dirp->d_type == DT_DIR))) {
				FileEntry entry;
				entry.path = path / Path(dirp->d_name);
				entry.is_dir = dirp->d_type == DT_DIR;
				out.push_back(entry);
			}

			if(dirp->d_type == DT_DIR && (flags & FindFiles::recursive) && strcmp(dirp->d_name, ".."))
				findFiles(out, path / Path(dirp->d_name), filter, flags);
		}
	}
	catch(...) {
		closedir(dp);
		throw;
	}
	closedir(dp);
}

