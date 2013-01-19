/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "game/tile.h"
#include "game/sprite.h"
#include "game/tile_map.h"
#include "sys/platform.h"
#include <unistd.h>
#include <algorithm>

#ifndef _WIN32
#define USE_OPENMP
#endif

#ifdef USE_OPENMP
#include <omp.h>
#endif

using namespace game;

template <class TResource>
void convert(const char *src_dir, const char *dst_dir, const char *old_ext, const char *new_ext,
			bool detailed, const string &filter) {
	vector<FileEntry> file_names;
	printf("Scanning...\n");
	Path main_path = Path(src_dir).absolute();
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
					if(detailed)
						printf("%55s  %6dKB -> %6dKB   %9.4f ms\n", name.c_str(),
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

int safe_main(int argc, char **argv) {
	bool conv_tiles = false;
	bool conv_sprites = false;
	bool conv_maps = false;
	int jobs = 0;
	string filter;

	for(int n = 1; n < argc; n++) {
		if(strcmp(argv[n], "-f") == 0 && n + 1 < argc)
			filter = argv[++n];
#ifdef USE_OPENMP
		else if(strcmp(argv[n], "-j") == 0 && n + 1 < argc)
			jobs = atoi(argv[++n]);
#endif
		else if(strcmp(argv[n], "tiles") == 0)
			conv_tiles = true;
		else if(strcmp(argv[n], "sprites") == 0)
			conv_sprites = true;
		else if(strcmp(argv[n], "maps") == 0)
			conv_maps = true;
	}

	if(!conv_tiles && !conv_sprites && !conv_maps) {
		printf("Usage:\n%s [options] tiles|sprites|maps\nOptions:\n"
				"-f filter    Converting only those files that match given filter\n"
#ifdef USE_OPENMP
				"-j jobcnt    Use jobcnt threads during conversion\n"
#endif
				"\n", argv[0]);
		return 0;
	}

	if(conv_maps) // ResourceManager<Tile> is single-threaded (used in tile_map_legacy.cpp)
		jobs = 1;
#ifdef USE_OPENMP
	if(jobs)
		omp_set_num_threads(jobs);
#endif

	if(conv_tiles)
		convert<Tile>("refs/tiles/", "data/tiles", ".til", ".tile", 0, filter);
	else if(conv_sprites)
		convert<Sprite>("refs/sprites/", "data/sprites/", ".spr", ".sprite", 1, filter);
	else if(conv_maps)
		convert<TileMap>("refs/maps/", "data/maps/", ".mis", ".xml", 1, filter);

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

