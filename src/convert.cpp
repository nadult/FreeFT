#include "gfx/tile.h"
#include "gfx/sprite.h"
#include "sys/platform.h"
#include <unistd.h>

using namespace gfx;

//TODO: openmp
template <class TResource>
void convert(const char *src_dir, const char *dst_dir, const char *old_ext, const char *new_ext) {
	vector<FileEntry> file_names;
	printf("Converting"); fflush(stdout);
	Path main_path = Path(src_dir).absolute();
	findFiles(file_names, main_path, FindFiles::regular_file | FindFiles::recursive);

	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		try {
			Path path = file_names[n].path.relative(main_path);
			string name = path.fileName();

			if(removeSuffix(name, old_ext)) {
				Path new_path = Path(dst_dir) / path.parent() / (name + new_ext);
				Path parent = new_path.parent();
			
				if(access(parent.c_str(), R_OK) != 0)
					mkdirRecursive(parent.c_str());

				Loader source(file_names[n].path);
				Saver target(new_path);

				TResource resource;
				resource.legacyLoad(source);
				resource.serialize(target);
			}
		} catch(const Exception &ex) {
			printf("Error: %s\n", ex.what());
		}
	}
	printf("\n");
}

int safe_main(int argc, char **argv) {
	if(argc < 2) {
		printf("Usage:\n%s [options]\nOptions:\n-t    Convert tiles\n-s    Convert sprites\n\n", argv[0]);
		return 0;
	}

	if(strcmp(argv[1], "-t") == 0)
		convert<Tile>("refs/tiles/", "data/tiles", ".til", ".tile");
	else if(strcmp(argv[1], "-s") == 0)
		convert<Sprite>("refs/sprites/", "data/sprites/", ".spr", ".sprite");

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

