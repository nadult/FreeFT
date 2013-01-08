#ifndef SYS_PLATFORM_H
#define SYS_PLATFORM_H


#include "base.h"

class Path {
public:
	Path(const char*);
	Path(const string&);
	Path(Path&&);
	Path(const Path&);
	Path();

	void operator=(const Path &rhs) { m_path = rhs.m_path; }
	void operator=(Path &&rhs) { m_path.swap(rhs.m_path); }

	bool isRoot() const;
	bool isAbsolute() const;

	const string fileName() const;
	bool isDirectory() const;
	bool isRegularFile() const;

	const Path relative(const Path &relative_to = current()) const;
	const Path absolute() const;
	const Path parent() const;

	const Path operator/(const Path &other) const;
	const Path &operator/=(const Path &other);

	static const Path current();

	operator const string&() const { return m_path; }
	const char *c_str() const { return m_path.c_str(); }
	int size() const { return (int)m_path.size(); }

	bool operator<(const Path &rhs) const {
		return m_path < rhs.m_path;
	}

private:
	struct Element {
		bool isDot() const;
		bool isDots() const;
		bool isRoot() const;
		bool operator==(const Element &rhs) const;

		const char *ptr;
		int size;
	};

	static Element extractRoot(const char*);
	static void divide(const char*, vector<Element>&);
	static void simplify(const vector<Element> &src, vector<Element> &dst);
	void construct(const vector<Element>&);

	string m_path; // its always non-empty
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

		relative		= 8,		// all paths relative to given path
		absolute		= 16,		// all paths absolute
	};
};

typedef bool (*FindFilesFilter)(const char *path, const char* name, bool is_dir);


void findFiles(vector<FileEntry> &out, const Path &path, int flags = FindFiles::regular_file);
bool removeSuffix(string &str, const string &suffix);
void mkdirRecursive(const Path &path);


#endif
