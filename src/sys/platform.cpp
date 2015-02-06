/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "sys/platform.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

#ifdef _WIN32

#include <windows.h>
#include <io.h>
#include <direct.h>

#else

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#endif

namespace sys {

	bool Path::Element::isDot() const {
		return size == 1 && ptr[0] == '.';
	}

	bool Path::Element::isDots() const {
		return size == 2 && ptr[0] == '.' && ptr[1] == '.';
	}

	bool Path::Element::isRoot() const {
		return size && (ptr[size - 1] == '/' || ptr[size - 1] == '\\');
	}

	bool Path::Element::operator==(const Element &rhs) const {
		return size == rhs.size && strncmp(ptr, rhs.ptr, size) == 0;
	}

	Path::Path(const char *path) {
		vector<Element> elements;
		elements.reserve(32);
		divide(path, elements);
		construct(elements);
	}
	Path::Path(const string &path) {
		vector<Element> elements;
		elements.reserve(32);
		divide(path.c_str(), elements);
		construct(elements);
	}
	Path::Path(Path &&ref) { ref.m_path.swap(m_path); }
	Path::Path(const Path &rhs) :m_path(rhs.m_path) { }
	Path::Path() :m_path(".") { }

	void Path::divide(const char *ptr, vector<Element> &out) {
		DASSERT(ptr);

		Element root = extractRoot(ptr);
		if(root.ptr) {
			ptr += root.size;
			out.push_back(root);
		}

		const char *prev = ptr;
		do {
			if(*ptr == '/' || *ptr == '\\' || *ptr == 0) {
				if(ptr - prev > 0)
					out.push_back(Element{prev, (int)(ptr - prev)});
				if(!*ptr)
					break;
				prev = ptr + 1;
			}
			ptr++;
		} while(true);
	}

	void Path::simplify(const vector<Element> &src, vector<Element> &dst) {
		for(int n = 0; n < (int)src.size(); n++) {
			if(src[n].isDot())
				continue;
			if(src[n].isDots() && !dst.empty() && !dst.back().isDots()) {
				DASSERT(!dst.back().isRoot());
				dst.pop_back();
			}
			else
				dst.push_back(src[n]);
		}
	}

	void Path::construct(const vector<Element> &input) {
		vector<Element> elements;
		elements.reserve(input.size());
		simplify(input, elements);

		if(elements.empty()) {
			m_path = ".";
			return;
		}

		int length = 0;
		for(int n = 0; n < (int)elements.size(); n++)
			length += elements[n].size;
		length += (int)elements.size() - 1;
		if(elements.size() > 1 && elements.front().isRoot())
			length--;

		string new_path(length, ' ');
		length = 0;
		for(int n = 0; n < (int)elements.size(); n++) {
			const Element &elem = elements[n];
			memcpy(&new_path[length], elem.ptr, elem.size);
			if(elem.isRoot()) {
				for(int i = 0; i < elem.size; i++) {
					char c = new_path[length + i];
					if(c >= 'a' && c <= 'z')
						c = c + 'A' - 'a';
					if(c == '\\')
						c = '/';
					new_path[length + i] = c;
				}
			}
			length += elem.size;

			if(n + 1 < (int)elements.size() && !elem.isRoot())
				new_path[length++] = '/';
		}

		new_path[length] = 0;
		m_path.swap(new_path);
	}

	const string Path::fileName() const {
		auto it = m_path.rfind('/');
		if(it == string::npos || (it == 0 && m_path.size() == 1))
			return m_path;
		return m_path.substr(it + 1);
	}

	bool Path::isRoot() const {
		// Ending backslash is stripped from folder paths
		return m_path.back() == '/';
	}

	bool Path::isAbsolute() const {
		return extractRoot(c_str()).size != 0;
	}

	const Path Path::relative(const Path &ref) const {
		if(!isAbsolute())
			return absolute().relative(ref);
		DASSERT(ref.isAbsolute());

		vector<Element> celems, relems;
		celems.reserve(32);
		relems.reserve(32);
		divide(m_path.c_str(), celems);
		divide(ref.m_path.c_str(), relems);

		vector<Element> oelems;
		oelems.reserve(32);

		int n = 0;
		for(int count = min(celems.size(), relems.size()); n < count; n++)
			if(! (celems[n] == relems[n]) )
				break;
		for(int i = n; i < (int)relems.size(); i++)
			oelems.push_back(Element{"..", 2});
		for(int i = n; i < (int)celems.size(); i++)
			oelems.push_back(celems[i]);

		Path out;
		out.construct(oelems);
		return std::move(out);
	}

	bool Path::isRelative(const Path &ref) const {
		DASSERT(ref.isAbsolute() && isAbsolute());

		vector<Element> celems, relems;
		celems.reserve(32);
		relems.reserve(32);
		divide(m_path.c_str(), celems);
		divide(ref.m_path.c_str(), relems);

		for(int count = min(celems.size(), relems.size()), n = 0; n < count; n++)
			if(! (celems[n] == relems[n]) )
				return false;
		return true;
	}

	const Path Path::absolute() const {
		return isAbsolute()? *this : current() / *this;
	}

	const Path Path::parent() const {
		return *this / "..";
	}

	const Path Path::operator/(const Path &other) const {
		Path out = *this;
		out /= other;
		return std::move(out);
	}

	const Path &Path::operator/=(const Path &other) {
		DASSERT(!other.isAbsolute());

		vector<Element> elems;
		elems.reserve(32);
		divide(m_path.c_str(), elems);
		divide(other.m_path.c_str(), elems);

		construct(elems);
		return *this;
	}

	bool removeSuffix(string &str, const string &suffix) {
		if(str.size() >= suffix.size() && suffix == str.c_str() + str.size() - suffix.size()) {
			str = str.substr(0, str.size() - suffix.size());
			return true;
		}

		return false;
	}

	bool removePrefix(string &str, const string &prefix) {
		if(str.size() >= prefix.size() && memcmp(str.c_str(), prefix.c_str(), prefix.size()) == 0) {
			str = str.substr(prefix.size());
			return true;
		}

		return false;
	}

	const string toLower(const string &str) {
		string out(str);
		std::transform(out.begin(), out.end(), out.begin(), ::tolower);
		return std::move(out);
	}

	bool access(const Path &path) {
#ifdef _WIN32
		return _access(path.c_str(), 0) == 0;
#else
		return ::access(path.c_str(), F_OK) == 0;
#endif
	}

	void mkdirRecursive(const Path &path) {
		Path parent = path.parent();

		if(!access(parent))
			mkdirRecursive(parent);

#ifdef _WIN32
		int ret = mkdir(path.c_str());
#else
		int ret = mkdir(path.c_str(), 0775);
#endif

		if(ret != 0)
			THROW("Cannot create directory: \"%s\" error: %s\n", path.c_str(), strerror(errno));
	}
	
	
	static void (*s_handler)() = 0;

#ifdef _WIN32
	static BOOL shandler(DWORD type) {
		if(type == CTRL_C_EVENT && s_handler) {
			s_handler();
			return TRUE;
		}

		return FALSE;
	}
#else	
	static void shandler(int) {
		if(s_handler)
			s_handler();
	}
#endif

	void handleCtrlC(void (*handler)()) {
		s_handler = handler;
#ifdef _WIN32
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)shandler, TRUE);
#else
		struct sigaction sig_int_handler;

		sig_int_handler.sa_handler = shandler;
		sigemptyset(&sig_int_handler.sa_mask);
		sig_int_handler.sa_flags = 0;
		sigaction(SIGINT, &sig_int_handler, NULL);   
#endif
	}

}
