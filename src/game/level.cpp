/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/level.h"
#include "sys/xml.h"
#include <algorithm>
#include <zlib.h>

namespace game {

	struct Line {
		const char *ptr;
		int size;
	};

	static vector<Line> divideIntoLines(vector<char> &data) {
		vector<Line> lines;
		Line line{data.data(), 0};

		for(int n = 0; n < (int)data.size(); n++) {
			if(data[n] == '\n') {
				data[n] = 0;
				lines.push_back(line);
				line = Line{data.data() + n + 1, 0};
			}
			else
				line.size++;
		}
		if(line.size != 0)
			lines.push_back(line);

		return lines;
	}

	static vector<char> applyPatch(vector<char> &orig, vector<char> &patch) {
		vector<Line> patch_lines = divideIntoLines(patch);
		vector<Line> out_lines;

		Line checksum_line = patch_lines.front();
		patch_lines.erase(patch_lines.begin());

		{
			char func_name[32] = "";
			unsigned int target_checksum = ~0u;
			sscanf(checksum_line.ptr, "%32s %x", func_name, &target_checksum);

			ASSERT(strcmp(func_name, "CRC32") == 0);
			unsigned int checksum = crc32(0, (const unsigned char*)orig.data(), orig.size());

			if(checksum != target_checksum)
				THROW("Checksum test failed. Expected: %08x got: %08x\nMake sure that map version is correct.",
						target_checksum, checksum);
		}
		
		vector<Line> orig_lines = divideIntoLines(orig);
		if(orig_lines.empty())
			orig_lines.push_back(Line{"", 0});

		int optr = (int)orig_lines.size() - 1;
		for(int n = 0; n < (int)patch_lines.size(); n++) {
			int line = -1, end_line = -1;
			char command = 0;

			sscanf(patch_lines[n].ptr, "%d%c", &line, &command);
			if(command == ',')
				sscanf(strchr(patch_lines[n].ptr, ',') + 1, "%d%c", &end_line, &command);

			if(end_line == -1)
				end_line = line;
			if(line != 0)
				line--;
			if(end_line != 0)
				end_line--;

			ASSERT(line >= 0 && end_line >= line);
			ASSERT(command == 'c' || command == 'a' || command == 'd');

			while(optr > end_line)
				out_lines.push_back(orig_lines[optr--]);

			if(command == 'c' || command == 'a') {
				int first_line = ++n;
				while(n < (int)patch_lines.size() && strcmp(patch_lines[n].ptr, ".") != 0)
					n++;
				for(int i = n - 1; i >= first_line; i--)
					out_lines.push_back(patch_lines[i]);
			}

			if(command == 'a')
				while(optr >= line)
					out_lines.push_back(orig_lines[optr--]);
			else
				optr = line - 1;
		}

		while(optr >= 0)
			out_lines.push_back(orig_lines[optr--]);

		vector<char> out;
		std::reverse(out_lines.begin(), out_lines.end());
		for(int n = 0; n < (int)out_lines.size(); n++) {
			out.insert(out.end(), out_lines[n].ptr, out_lines[n].ptr + out_lines[n].size);
			out.push_back('\n');
		}
		
		return out;
	}

	Level::Level() :entity_map(tile_map) {
	}

	void Level::load(const char *file_name) {
		XMLDocument doc;

		int len = strlen(file_name);
		if(len > 4 && strcmp(file_name + len - 4, ".mod") == 0) {
			string orig_file_name(file_name, file_name + len - 4);
			orig_file_name += ".xml";

			vector<char> orig, mod; {
				Loader orig_sr(orig_file_name.c_str());
				Loader mod_sr(file_name);
				orig.resize(orig_sr.size());
				mod.resize(mod_sr.size());
				orig_sr.loadData(orig.data(), orig.size());
				mod_sr.loadData(mod.data(), mod.size());
			}

			vector<char> patched = applyPatch(orig, mod);
			MemoryLoader(patched) >> doc;
		}
		else
			doc.load(file_name);

		tile_map.loadFromXML(doc);
		entity_map.loadFromXML(doc);
	}

	void Level::save(const char *file_name) const {
		XMLDocument doc;
		tile_map.saveToXML(doc);
		entity_map.saveToXML(doc);
		doc.save(file_name);
	}

}
