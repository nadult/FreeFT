/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef SYS_DATA_SHEET_H
#define SYS_DATA_SHEET_H

#include "base.h"
#include <map>

class Tuple;

struct TupleParser {
	typedef std::map<CString, int> ColumnMap;
	TupleParser( const char **columns, int num_columns, const ColumnMap&);

	const char *operator()(const char *name) const;

private:
	friend class DataSheet;

	const ColumnMap &m_column_map;
	const char **m_columns;
	int m_num_columns;
};

class DataSheet {
public:
	DataSheet();
	virtual ~DataSheet() { }
	void load(const XMLDocument &doc, const char *shit_name);
	int findTuple(const string &id) const;
	const char *name() const { return m_name.c_str(); }

private:
	virtual Tuple &addTuple(TupleParser&) = 0;

	std::map<string, int> m_map;
	string m_name;
	bool m_is_loaded;
};

template <class T>
class TDataSheet: public DataSheet {
public:
	const T &operator[](int idx) const { return m_tuples[idx]; }
	int size() const { return (int)m_tuples.size(); }
	
	void connectRefs() {
		for(int n = 0; n < size(); n++)
			m_tuples[n].connect();
	}

private:
	Tuple &addTuple(TupleParser &parser) final {
		m_tuples.emplace_back(T(parser));
		m_tuples.back().idx = (int)m_tuples.size() - 1;
		return m_tuples.back();
	}

	vector<T> m_tuples;
};

struct Tuple {
	Tuple(const TupleParser&);

	string id;
	int idx;
};

template <class RT>
class TupleRef;

template <class T>
struct TupleImpl
{
	static void load(const XMLDocument &doc, const char *sheet_name) { pool.load(doc, sheet_name); }
	static int find(const string &name) { return pool.findTuple(name); }
	static int count() { return pool.size(); }
	static const T &get(int idx) {
		DASSERT(idx >= 0 && idx < (int)pool.size());
		return pool[idx];
	}

	static void connectRefs() { pool.connectRefs(); }

	// You can overload this function
	void connect() { }

protected:
	static TDataSheet<T> pool;
	friend class TupleRef<T>;
};

template <class T> TDataSheet<T> TupleImpl<T>::pool;

template <class RT>
class TupleRef {
public:
	typedef RT RefTuple;

	TupleRef() :m_idx(-1) { }
	TupleRef(const char *str) :m_id(str), m_idx(-1) { }

	bool isValid() const { return m_idx != -1; }

	void connect() {
		if(m_id.empty())
			return;

		int idx = RT::pool.findTuple(m_id);
		if(idx == -1)
			THROW("Can't find tuple with id: %s (pool: %s)", m_id.c_str(), RT::pool.name());
		m_idx = idx;
	}

	explicit operator const RT*() const {
		return m_idx == -1? nullptr : &RT::pool[m_idx];
	}

	const string &id() const { return m_id; }
	int idx() const { return m_idx; }

private:
	string m_id;
	mutable int m_idx;
};

#endif

