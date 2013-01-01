#include "gfx/tile.h"
#include "gfx/sprite.h"
#include "sys/platform.h"
#include <unistd.h>

using namespace gfx;

template <class TResource>
void convert(const char *src_dir, const char *dst_dir, const char *old_ext, const char *new_ext,
			bool detailed, bool fast, const string &filter) {
	vector<FileEntry> file_names;
	if(!detailed) {
		printf("Converting");
		fflush(stdout);
	}
	Path main_path = Path(src_dir).absolute();
	findFiles(file_names, main_path, FindFiles::regular_file | FindFiles::recursive);

#pragma omp parallel for num_threads(4)
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
					
				if(access(parent.c_str(), R_OK) != 0)
					mkdirRecursive(parent.c_str());

				try {
					TResource resource;
					Loader source(full_path);
					double time = getTime();
					resource.legacyLoad(source, fast);
					if(detailed)
						printf("%30s  %6dKB -> %6dKB   %9.4f ms\n", name.c_str(),
								(int)(source.size()/1024), resource.memorySize()/1024, (getTime() - time) * 1024.0);
					Saver target(new_path);
					resource.serialize(target);
				} catch(const Exception &ex) {
					printf("Error while converting: %s:\n%s\n\n", full_path.c_str(), ex.what());
				}
			}
		}
	}

#pragma omp barrier
	if(!detailed)
		printf("\n");
}

int safe_main(int argc, char **argv) {
	bool fast = false;
	bool conv_tiles = false;
	bool conv_sprites = false;
	string filter;

	for(int n = 1; n < argc; n++) {
		if(strcmp(argv[n], "--fast") == 0)
			fast = true;
		else if(strcmp(argv[n], "-f") == 0 && n + 1 < argc)
			filter = argv[++n];
		else if(strcmp(argv[n], "tiles") == 0)
			conv_tiles = true;
		else if(strcmp(argv[n], "sprites") == 0)
			conv_sprites = true;
	}

	if(!conv_tiles && !conv_sprites) {
		printf("Usage:\n%s [options] tiles|sprites\nOptions:\n"
				"--fast    	Fast compression\n"
				"-f filter	Converting only those files that match given filter\n\n", argv[0]);
		return 0;
	}

	if(conv_tiles)
		convert<Tile>("refs/tiles/", "data/tiles", ".til", ".tile", false, fast, filter);
	else if(conv_sprites)
		convert<Sprite>("refs/sprites/", "data/sprites/", ".spr", ".sprite", true, fast, filter);

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

