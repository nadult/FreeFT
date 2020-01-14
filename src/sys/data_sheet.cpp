// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "sys/data_sheet.h"

#include <fwk/sys/xml.h>
#include <fwk/sys/on_fail.h>

TupleParser::TupleParser(const char **columns, int num_columns, const TupleParser::ColumnMap &map)
	: m_columns(columns), m_num_columns(num_columns), m_column_map(map) {}

const char *TupleParser::get(const char *col_name) const {
	DASSERT(col_name);

	auto it = m_column_map.find(col_name);
	if(it == m_column_map.end())
		CHECK_FAILED("missing column: %s", col_name);
	return m_columns[it->second];
}

static const char *getText(CXmlNode cell_node) {
	auto val_type = cell_node.hasAttrib("office:value-type");
	if(val_type) {
		auto text_node = cell_node.child("text:p");
		if(!text_node)
			CHECK_FAILED("Unsupported node type: %s\n", val_type.c_str());
		ASSERT(text_node);
		return text_node.value().c_str();
	}

	return "";
}

void loadDataSheet(CXmlNode table_node, HashMap<string, int> &map, int (*add_func)(TupleParser &)) {
	vector<CXmlNode> rows;
	vector<ZStr> col_names;

	auto row_node = table_node.child("table:table-row");
	while(row_node) {
		rows.emplace_back(row_node);
		row_node = row_node.sibling("table:table-row");
	}

	{
		ASSERT(!rows.empty());
		auto first_row = rows.front();
		auto cell_node = first_row.child("table:table-cell");

		while(cell_node) {
			col_names.push_back(getText(cell_node));
			cell_node = cell_node.sibling("table:table-cell");
		}
	}

	HashMap<Str, int> column_map;
	for(int n = 0; n < (int)col_names.size(); n++)
		if(!col_names[n].empty()) {
			if(column_map.find(col_names[n]) != column_map.end())
				CHECK_FAILED("Duplicate argument: %s", col_names[n].c_str());
			column_map.emplace(col_names[n], n);
		}

	vector<const char *> columns;
	columns.reserve(col_names.size());

	int id_column = 0;
	{
		auto it = column_map.find("id");
		if(it == column_map.end())
			CHECK_FAILED("Id column must be defined");
		id_column = it->second;
	}

	ON_FAIL("Error while parsing sheet: %", table_node.attrib("table:name"));

	for(int r = 1; r < (int)rows.size(); r++) {
		auto row = rows[r];

		int cell_idx = 0;
		auto cell = row.child("table:table-cell");

		columns.clear();
		while(cell && cell_idx < (int)col_names.size()) {
			const char *value = getText(cell);
			int num_repeat = cell.attrib<int>("table:number-columns-repeated", 1);

			for(int i = 0; i < num_repeat; i++)
				columns.push_back(value);
			cell_idx += num_repeat;

			cell = cell.sibling("table:table-cell");
		}

		int num_columns = (int)min(col_names.size(), columns.size());
		bool is_empty = true;
		for(int n = 0; n < num_columns; n++)
			if(columns[n][0]) {
				is_empty = false;
				break;
			}

		if(is_empty)
			continue;

		TupleParser parser(columns.data(), num_columns, column_map);
		string id = columns[id_column];
		ON_FAIL("Error while parsing row: % (id: %)", r, id);

		if(id.empty())
			CHECK_FAILED("ID undefined");
		if(map.find(id) != map.end())
			CHECK_FAILED("Duplicated ID: %s", id.c_str());

		int index = add_func(parser);
		map.emplace(std::move(id), index);
	}
}
