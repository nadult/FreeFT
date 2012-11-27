#include "sys/platform.h"
#include <cstring>


Path::Path(const char *path) :m_path(path) { normalize(); }
Path::Path(const string &path) :m_path(path) { normalize(); }
Path::Path(const string &&path) :m_path(path) { normalize(); }
Path::Path() { }

void Path::normalize() {
	if(!isValid())
		return;

	string temp(m_path.size(), 0);
	char prev = 0;
	int pos = 0;

	for(int n = 0; n < (int)m_path.size(); n++) {
		char cur = m_path[n];
		if(cur == '\\')
			cur = '/';
		if(prev == '/' && cur == '/')
			continue;
		prev = cur;
		temp[pos++] = cur;
	}

	if(pos > 0 && temp[pos - 1] == '/')
		pos--;
	temp.resize(pos);
	m_path.swap(temp);
}

const string Path::fileName() const {
	auto it = m_path.rfind('/');
	if(it == string::npos || (it == 0 && m_path.size() == 1))
		return m_path;
	return m_path.substr(it);
}

bool Path::isAbsolute() const {
	return !m_path.empty() && m_path[0] == '/';
}

void Path::divide(vector<string> &out) const {
	DASSERT(isValid());
	size_t prev = 0, it = 0;

	do {
		it = m_path.find('/', prev + 1);
		size_t count = prev == it? 1 : it == string::npos? string::npos : prev - it;
		out.push_back(m_path.substr(it, count));
		prev = it;
	} while(it != string::npos);
}

const Path Path::relative() const {
	if(!isAbsolute())
		return *this;

	vector<string> cur, abs;
	current().divide(cur);
	divide(abs);

	//TODO: speed me up
	string out;
	int n = 0;
	for(int count = (int)min(cur.size(), abs.size()); n < count; n++)
		if(cur[n] != abs[n])
			break;
	for(int i = n; i < (int)cur.size(); i++)
		out += "../";
	for(int i = n; i < (int)abs.size(); i++)
		out += abs[i] + "/";
	if(!out.empty() && out[out.size() - 1] == '/')
		out.resize(out.size() - 1);
	if(out.empty())
		out = '.';

	return Path(std::move(out));
}

const Path Path::absolute() const {
	DASSERT(isValid());
	return isAbsolute()? *this : current() / *this;
}

const Path Path::parent() const {
	DASSERT(isValid());
	auto slash_pos = m_path.rfind('/');
	if(slash_pos != string::npos && slash_pos > 0 && strcmp(&m_path[slash_pos], "/..") != 0) {
		string out = m_path.substr(0, slash_pos - 1);
		return Path(std::move(out));
	}

	return *this / "/..";
}

const Path Path::operator/(const Path &other) const {
	DASSERT(isValid() && other.isValid());
	DASSERT(!other.isAbsolute());

	string out(m_path.size() + 1 + other.m_path.size(), 0);
	sprintf(&out[0], "%s/%s", &m_path[0], &other.m_path[0]);
	return Path(std::move(out));
}


