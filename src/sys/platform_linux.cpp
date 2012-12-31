#include "sys/platform.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <cstring>

#ifndef _DIRENT_HAVE_D_TYPE
#error dirent without d_type not supported
#endif

Path::Element Path::extractRoot(const char *str) {
	if(str[0] == '/')
		return Element{str, 1};
	return Element{nullptr, 0};
}

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

	return S_ISDIR(buf.st_mode) || S_ISLNK(buf.st_mode);
}

static void findFiles(vector<FileEntry> &out, const Path &path, const Path &append, int flags) {
	DIR *dp = opendir(path.c_str());
	if(!dp)
		THROW("Error while opening directory %s: %s", path.c_str(), strerror(errno));
	bool is_root = path.isRoot();

	try {
		struct dirent *dirp;

		while ((dirp = readdir(dp))) {
			if(strcmp(dirp->d_name, ".") == 0)
				continue;

			if(dirp->d_type == DT_UNKNOWN) {
				char full_path[FILENAME_MAX];
				struct stat buf;

				snprintf(full_path, sizeof(full_path), "%s/%s", path.c_str(), dirp->d_name);
				stat(full_path, &buf);
				if(S_ISDIR(buf.st_mode))
					dirp->d_type = DT_DIR;
				else if(S_ISREG(buf.st_mode))
					dirp->d_type = DT_REG;
			}

			if(dirp->d_type == DT_LNK)
				dirp->d_type = DT_DIR;

			bool do_accept =	( (flags & FindFiles::regular_file) && dirp->d_type == DT_REG ) ||
								( (flags & FindFiles::directory) && (dirp->d_type == DT_DIR));
	//		printf("found in %s: %s (%d)\n", path.c_str(), dirp->d_name, (int)dirp->d_type);

			if(do_accept && is_root && dirp->d_type == DT_DIR && strcmp(dirp->d_name, "..") == 0)
				do_accept = false;

			if(do_accept) {
				FileEntry entry;
				entry.path = append / Path(dirp->d_name);
				entry.is_dir = dirp->d_type == DT_DIR;
				out.push_back(entry);
			}

			if(dirp->d_type == DT_DIR && (flags & FindFiles::recursive) && strcmp(dirp->d_name, ".."))
				findFiles(out, path / Path(dirp->d_name), append / Path(dirp->d_name), flags);
		}
	}
	catch(...) {
		closedir(dp);
		throw;
	}
	closedir(dp);
}

void findFiles(vector<FileEntry> &out, const Path &path, int flags) {
	Path append =	flags & FindFiles::relative? "." :
					flags & FindFiles::absolute? path.absolute() : path;

	findFiles(out, path.absolute(), append, flags);
}

void mkdirRecursive(const Path &path) {
	Path parent = path.parent();
	if(access(parent.c_str(), R_OK) != 0)
			mkdirRecursive(parent);
	int ret = mkdir(path.c_str(), 0775);
	if(ret != 0)
		THROW("Cannot create directory: %s error: %s\n", path.c_str(), strerror(errno));
}
