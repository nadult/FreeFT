#include "gfx/tile.h"
#include "gfx/sprite.h"
#include "sys/platform.h"

using namespace gfx;

int safe_main(int argc, char **argv) {
	printf("Converting tiles...\n");
	vector<FileEntry> file_names;
	
	Path tiles_path = Path("refs/tiles/").absolute();
	findFiles(file_names, tiles_path, FindFiles::regular_file | FindFiles::recursive);

	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		try {
			Path tile_path = file_names[n].path.relative(tiles_path);
			string tile_name = tile_path.fileName();

			if(removeSuffix(tile_name, ".til")) {
				Path new_path = Path("data/tiles") / tile_path.parent() / (tile_name + ".tile");
				Path parent = new_path.parent();
			
				if(access(parent.c_str(), R_OK) != 0)
					mkdirRecursive(parent.c_str());

				Loader source(file_names[n].path);
				Saver target(new_path);

				Tile tile;
				tile.serializeTil(source);
				tile.serialize(target);
			}
		} catch(const Exception &ex) {
			printf("Error: %s\n", ex.what());
		}
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

