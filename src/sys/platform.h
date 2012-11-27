#ifndef SYS_PLATFORM_H
#define SYS_PLATFORM_H


#include "base.h"

class Path {
public:
	Path(const char*);
	Path(const string&);
	Path(const string&&);
	Path();

	bool isValid() const { return !m_path.empty(); }
	bool isAbsolute() const;

	const string fileName() const;
	bool isDirectory() const;
	bool isRegularFile() const;

	void divide(vector<string>&) const;

	const Path relative() const;
	const Path absolute() const;
	const Path parent() const;
	const Path operator/(const Path &other) const;

	static const Path current();

	operator const string&() const { return m_path; }
	const char *c_str() const { return m_path.c_str(); }

	bool operator<(const Path &rhs) const {
		return m_path < rhs.m_path;
	}

private:
	void normalize();

	string m_path;
};

struct FileEntry {
	Path path;
	bool is_dir;

	bool operator<(const FileEntry &rhs) const {
		return is_dir == rhs.is_dir? path < rhs.path : is_dir > rhs.is_dir;
	}
};

namespace FindFiles {
	enum Flags {
		regular_file	= 1,
		directory		= 2,
		recursive		= 4,
	};
};

typedef bool (*FindFilesFilter)(const char *path, const char* name, bool is_dir);


void findFiles(vector<FileEntry> &out, const Path &path, FindFilesFilter filter = nullptr, int flags = FindFiles::regular_file);


#endif
