/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/tile.h"
#include "game/sprite.h"
#include "game/tile_map.h"
#include "sys/platform.h"
#include "sys/xml.h"
#include <unistd.h>
#include <algorithm>
#include <set>
#include <zip.h>

#ifndef _WIN32
#define USE_OPENMP
#endif

#ifdef USE_OPENMP
#include <omp.h>
#endif

using namespace game;



struct TileMapProxy: public TileMap {
	void serialize(Serializer &sr) {
		ASSERT(sr.isSaving());
		
		XMLDocument doc;
		saveToXML(doc);
		sr & doc;
	}
	void setResourceName(const char*) { }
};

namespace ResTypeId {
	enum Type {
		sprite,
		tile,
		map,
		archive,

		count
	};

	static const char *s_old_suffix[count] = {
		".spr",
		".til",
		".mis",
		".bos"
	};

	static const char *s_new_suffix[count] = {
		".sprite",
		".tile",
		".xml",
		nullptr
	};

	static const char *s_new_path[count] = {
		"data/sprites/",
		"data/tiles/",
		"data/maps/",
		nullptr,
	};
};

void convert(ResTypeId::Type type, Serializer &ldr, Serializer &svr) {
	ASSERT(type != ResTypeId::archive);

	try {
		if(type == ResTypeId::sprite) {
			Sprite res;
			res.legacyLoad(ldr);
			res.serialize(svr);
		}
		else if(type == ResTypeId::tile) {
			Tile res;
			res.legacyLoad(ldr, svr.name());
			res.serialize(svr);
		}
		else if(type == ResTypeId::map) {
			TileMapProxy res;
			res.legacyConvert(ldr, svr);
		}
	}
	catch(const Exception &ex) {
		printf("Error while converting %s:\n%s\n\n", ldr.name(), ex.what());
	}
}

template <class TResource>
void convert(const char *src_dir, const char *dst_dir, const char *old_ext, const char *new_ext,
			bool detailed, const string &filter) {
	Path main_path = Path(src_dir).absolute();
	vector<FileEntry> file_names;
	
	printf("Scanning...\n");
	findFiles(file_names, main_path, FindFiles::regular_file | FindFiles::recursive);
	std::sort(file_names.begin(), file_names.end());
	int total_before = 0, total_after = 0;

	printf("Recreating directories...\n");
	for(int n = 0; n < (int)file_names.size(); n++) {
		Path path = file_names[n].path.relative(main_path);
		Path dir = Path(dst_dir) / path.parent();
		if(access(dir.c_str(), R_OK) != 0)
			mkdirRecursive(dir.c_str());
	}
	
	if(!detailed) {
		printf("Converting");
		fflush(stdout);
	}

#pragma omp parallel for
	for(uint n = 0; n < file_names.size(); n++) {
		if(!detailed && n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}
		Path full_path = file_names[n].path;
		if(((const string&)full_path).find(filter) == string::npos)
			continue;
		
#pragma omp task
		{	
			Path path = full_path.relative(main_path);
			string name = path.fileName();

			if(removeSuffix(name, old_ext)) {
				Path new_path = Path(dst_dir) / path.parent() / (name + new_ext);
				Path parent = new_path.parent();

				try {
					TResource resource;
					Loader source(full_path);
					double time = getTime();
					resource.legacyLoad(source);
					Saver target(new_path);
					resource.serialize(target);
					resource.setResourceName((Path(name).fileName()).c_str()); // TODO: this isprobably not needed

					if(detailed)
						printf("%40s  %6dKB -> %6dKB   %9.4f ms\n", name.c_str(),
								(int)(source.size()/1024), (int)(target.size()/1024), (getTime() - time) * 1024.0);

#pragma omp atomic
						total_before += source.size();
#pragma omp atomic
						total_after += target.size();
				} catch(const Exception &ex) {
					printf("Error while converting: %s:\n%s\n\n", full_path.c_str(), ex.what());
				}
			}
		}
	}

#pragma omp barrier
	if(!detailed)
		printf("\n");
	printf("Total: %6dKB -> %6dKB\n", total_before/1024, total_after/1024);
}

struct ResPath {
	const char *prefix;
	ResTypeId::Type type;
};

static ResPath s_paths[] = {
		{ "tiles/", ResTypeId::tile },
		{ "sprites/", ResTypeId::sprite },
		{ "campaigns/missions/core/", ResTypeId::map },
		{ "campaigns/missions/tutorials/", ResTypeId::map },
		{ "", ResTypeId::archive }
};

class Archive {
public:
	Archive(const char *file_name) {
		m_zip = zip_open(file_name, 0, 0);
		ASSERT(m_zip);
		m_file_count = zip_get_num_entries(m_zip, 0);
	}
	~Archive() {
		zip_close(m_zip);
	}
	Archive(const Archive&) = delete;
	void operator=(const Archive&) = delete;

	void readFile(int index, vector<char> &data) {
		DASSERT(index >= 0 && index < m_file_count);
		
		struct zip_stat stat;
		int ret = zip_stat_index(m_zip, index, 0, &stat);
		ASSERT(ret == 0);
		ASSERT(stat.size < 128 * 1024 * 1024); //TODO: safety check, increase if needed
		data.resize(stat.size);
		
		struct zip_file *file = zip_fopen_index(m_zip, index, 0);
		ASSERT(file);
		zip_fread(file, data.data(), data.size());
		zip_fclose(file);
	}

	int fileCount() const {
		return m_file_count;
	}

	vector<string> getNames() const {
		vector<string> out;
		for(int n = 0; n < m_file_count; n++) {
			const char *name = zip_get_name(m_zip, n, 0);
			ASSERT(name);
			out.push_back(name);
		}

		return out;
	}

private:
	struct zip *m_zip;
	int m_file_count;
};

void convertAll(const char *fot_path) {
	Path core_path = (Path(fot_path) / "core").absolute();

	printf("FOT core: %s\n", core_path.c_str());

	vector<FileEntry> all_files;
	printf("Scanning...\n");
	findFiles(all_files, core_path, FindFiles::regular_file | FindFiles::recursive);
	std::sort(all_files.begin(), all_files.end());
	printf("Found: %d files\n", (int)all_files.size());

	std::set<string> files[COUNTOF(s_paths)];
	bool only_archives = 0;
	unsigned long long bytes = 0;
	
	for(int n = 0; n < (int)all_files.size(); n++) {
		for(int t = 0; t < COUNTOF(s_paths); t++) {
			if(only_archives && s_paths[t].type != ResTypeId::archive)
				continue;

			string name = all_files[n].path.relative(core_path);
			if(removePrefix(name, s_paths[t].prefix) && removeSuffix(name, ResTypeId::s_old_suffix[s_paths[t].type])) {
				files[t].insert(name);
				break;
			}
		}
	}

	printf("Converting plain files...\n");
	for(int t = 0; t < COUNTOF(s_paths); t++) {
		ResTypeId::Type type = s_paths[t].type;
		if(type == ResTypeId::archive)
			continue;

		Path target_dir = ResTypeId::s_new_path[type];
		Path src_main_path = core_path / s_paths[t].prefix;
		for(auto it = files[t].begin(); it != files[t].end(); ++it) {
			Path dir = target_dir / Path(*it).parent();
			if(access(dir.c_str(), R_OK) != 0)
				mkdirRecursive(dir.c_str());
		}

		for(auto it = files[t].begin(); it != files[t].end(); ++it) {
			char src_path[512], dst_path[512];
			snprintf(src_path, sizeof(src_path), "%s/%s%s", src_main_path.c_str(), it->c_str(), ResTypeId::s_old_suffix[type]);
			snprintf(dst_path, sizeof(src_path), "%s/%s%s", ResTypeId::s_new_path[type], it->c_str(), ResTypeId::s_new_suffix[type]);

			Loader ldr(src_path);
			Saver svr(dst_path);

			if(type != ResTypeId::tile || bytes > 1024 * 1024) {
				if(type == ResTypeId::tile) { printf("."); fflush(stdout); }
				else printf("%s\n", it->c_str());
				bytes = 0;
			}
			else 
				bytes += ldr.size();
			convert(type, ldr, svr);
		}
	}
	
	printf("Converting archives...\n");
	for(int t = 0; t < COUNTOF(s_paths); t++) {
		ResTypeId::Type type = s_paths[t].type;
		if(type != ResTypeId::archive)
			continue;

		Path src_path = core_path / s_paths[t].prefix;
		for(auto it = files[t].begin(); it != files[t].end(); ++it) {
			char archive_path[512];
			snprintf(archive_path, sizeof(archive_path), "%s/%s%s", src_path.c_str(), it->c_str(), ResTypeId::s_old_suffix[type]);

			Archive archive(archive_path);
	//		printf("Archive %s: %d\n", archive_path, archive.fileCount());

			vector<string> names = archive.getNames();
			for(int n = 0; n < (int)names.size(); n++) {
				string name;
				int tindex = -1;

				for(int t = 0; t < COUNTOF(s_paths); t++) {
					name = names[n];
					if(removePrefix(name, s_paths[t].prefix) && removeSuffix(name, ResTypeId::s_old_suffix[s_paths[t].type])) {
						tindex = t;
						break;
					}
				}

				if(tindex == -1 || files[tindex].find(name) != files[tindex].end())
					continue;
				ResTypeId::Type type = s_paths[tindex].type;

				vector<char> data;
				archive.readFile(n, data);

				char dst_path[512];
				snprintf(dst_path, sizeof(dst_path), "%s/%s%s", ResTypeId::s_new_path[type], name.c_str(), ResTypeId::s_new_suffix[type]);

				Path dir = Path(dst_path).parent();
				if(access(dir.c_str(), R_OK) != 0)
					mkdirRecursive(dir.c_str());

				Loader ldr(new DataStream(data));
				Saver svr(dst_path);

				if(type != ResTypeId::tile || bytes > 1024 * 1024) {
					if(type == ResTypeId::tile) { printf("."); fflush(stdout); }
					else printf("%s\n", name.c_str());
					bytes = 0;
				}
				else 
					bytes += ldr.size();
				convert(type, ldr, svr);
			}
		}
	}

	printf("\nDone.\n");
}

int safe_main(int argc, char **argv) {
	string command, path, filter;
	int jobs = 0;

	for(int n = 1; n < argc; n++) {
		if(strcmp(argv[n], "-f") == 0 && n + 1 < argc) {
			ASSERT(filter.empty());
			filter = argv[++n];
		}
#ifdef USE_OPENMP
		else if(strcmp(argv[n], "-j") == 0 && n + 1 < argc) {
			ASSERT(jobs == 0);
			jobs = atoi(argv[++n]);
			ASSERT(jobs >= 1);
		}
#endif
		else if(strcmp(argv[n], "-p") == 0 && n + 1 < argc) {
			ASSERT(path.empty());
			path = argv[++n];
		}
		else {
			ASSERT(command.empty());
			command = argv[n];
		}
	}

#ifdef USE_OPENMP
	if(command == "maps") // ResourceManager<Tile> is single-threaded (used in tile_map_legacy.cpp)
		jobs = 1;
	if(jobs)
		omp_set_num_threads(jobs);
#endif

	if(command == "tiles") {
		if(path.empty())
			path = "refs/tiles/";
		convert<Tile>(path.c_str(), "data/tiles", ".til", ".tile", 0, filter);
	}
//	else if(command == "maps") {
//		if(path.empty())
//			path = "refs/maps/";
//		convert<TileMapProxy>(path.c_str(), "data/maps/", ".mis", ".xml", 1, filter);
//	}
	else if(command == "sprites") {
		if(path.empty())
			path = "refs/sprites/";
		convert<Sprite>(path.c_str(), "data/sprites/", ".spr", ".sprite", 1, filter);
	}
	else if(command == "all") {
		ASSERT(!path.empty());
		convertAll(path.c_str());
	}
	else {
		if(!command.empty())
			printf("Unknown command: %s\n", command.c_str());

		printf("Usage:\n%s [options] tiles|sprites|maps|all\n\n"
				"tiles:       converting tiles in .til format from refs/tiles/ directory\n"
				"sprites:     converting sprites in .spr format from refs/sprites/ directory\n"
				"maps:        converting maps in .mis format from refs/maps directory\n"
				"all:         converting everyting (also from .bos files), path has to be specified\n\n"
				"Options:\n"
				"-f filter    Converting only those files that match given filter\n"
				"-p path      Specify different path\n"
#ifdef USE_OPENMP
				"-j jobcnt    Use jobcnt threads during conversion\n"
#endif
				"\n", argv[0]);
		return 0;
	}

	return 0;
}


int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
	catch(...) { return 1; }
}

