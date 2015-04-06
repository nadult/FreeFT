/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "sys/data_sheet.h"

TupleParser::TupleParser(const char **columns, int num_columns, const TupleParser::ColumnMap &map)
		:m_columns(columns), m_num_columns(num_columns), m_column_map(map) { }

const char *TupleParser::operator()(const char *col_name) const {
	DASSERT(col_name);

	auto it = m_column_map.find(col_name);
	if(it == m_column_map.end())
		THROW("missing column: %s", col_name);
	return m_columns[it->second];
}
	
static const char *getText(XMLNode cell_node) {
	const char *val_type = cell_node.hasAttrib("office:value-type");
	if(val_type) {
		XMLNode text_node = cell_node.child("text:p");
		if(!text_node)
			THROW("Unsupported node type: %s\n", val_type);
		ASSERT(text_node);
		return text_node.value();
	}

	return "";
}

void loadDataSheet(XMLNode table_node, std::map<string, int> &map, int (*add_func)(TupleParser&)) {
	vector<XMLNode> rows;
	vector<StringRef> col_names;

	XMLNode row_node = table_node.child("table:table-row");
	while(row_node) {
		rows.emplace_back(row_node);
		row_node = row_node.sibling("table:table-row");
	}

	{
		ASSERT(!rows.empty());
		XMLNode first_row = rows.front();
		XMLNode cell_node = first_row.child("table:table-cell");

		while(cell_node) {
			col_names.push_back(getText(cell_node));
			cell_node = cell_node.sibling("table:table-cell");
		}
	}
	
	std::map<StringRef, int> column_map;
	for(int n = 0; n < (int)col_names.size(); n++)
		if(!col_names[n].isEmpty()) {
			if(column_map.find(col_names[n]) != column_map.end())
				THROW("Duplicate argument: %s", col_names[n]);
			column_map.emplace(col_names[n], n);
		}

	vector<const char*> columns;
	columns.reserve(col_names.size());
	bool errors = false;

	int id_column = 0; {
		auto it = column_map.find("id");
		if(it == column_map.end())
			THROW("Id column must be defined");
		id_column = it->second;
	}

	for(int r = 1; r < (int)rows.size(); r++) {
		XMLNode row = rows[r];

		int cell_idx = 0;
		XMLNode cell = row.child("table:table-cell");
		
		columns.clear();
		while(cell && cell_idx < (int)col_names.size()) {
			const char *value = getText(cell);
		
			const char *repeat_attrib = cell.hasAttrib("table:number-columns-repeated");
			int num_repeat = repeat_attrib? toInt(repeat_attrib) : 1;

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
		try {
			string id = columns[id_column];
			if(id.empty())
				THROW("ID undefined");
			if(map.find(id) != map.end())
				THROW("Duplicated ID: %s", id.c_str());

			int index = add_func(parser);
			map.emplace(std::move(id), index);
		}
		catch(const Exception &ex) {
			errors = true;
			printf("Error while parsing row: %d (id: %s):\n%s\n",
					r, columns[id_column], ex.what());
		}
	}

	if(errors)
		THROW("Errors while parsing sheet: %s\n", table_node.attrib("table:name"));
}
