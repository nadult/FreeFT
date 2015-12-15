/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef SYS_DATA_SHEET_H
#define SYS_DATA_SHEET_H

#include "base.h"
#include <map>

struct TupleParser {
	using ColumnMap = std::map<StringRef, int>;
	TupleParser( const char **columns, int num_columns, const ColumnMap&);

	const char *get(const char *name) const;
	const char *operator()(const char *name) const { return get(name); }

	template <class T>
	T get(const char *name) const {
		auto *string = get(name);
		if(!string[0])
			return T();
		return xml_conversions::fromString<T>(string);
	}

private:
	friend class DataSheet;

	const ColumnMap &m_column_map;
	const char **m_columns;
	int m_num_columns;
};

void loadDataSheet(XMLNode table_node, std::map<string, int> &map, int (*add_func)(TupleParser&));

#endif

