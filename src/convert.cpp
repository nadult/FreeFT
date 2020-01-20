// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/tile.h"
#include "game/sprite.h"
#include "game/tile_map.h"
#include <unistd.h>
#include <algorithm>
#include <set>
#include <zip.h>
#include <fwk/io/file_stream.h>
#include <fwk/io/file_system.h>
#include <fwk/sys/expected.h>
#include <fwk/enum_map.h>
#include <fwk/sys/on_fail.h>

using game::Sprite;
using game::Tile;
using game::TileMap;

static bool verifyFTPath(string path) {
	return access(FilePath(path) / "core");
}

#ifdef _WIN32

#define __msxml_h__
#include <windows.h>
#include <shlobj.h>

static int CALLBACK browseFolderCB(HWND hwnd, UINT msg, LPARAM lparam, LPARAM data) {
	if (msg == BFFM_INITIALIZED) {
		LPCTSTR path = reinterpret_cast<LPCTSTR>(data);
		SendMessage(hwnd, BFFM_SETSELECTION, true, (LPARAM)path);
	}

	return 0;
}

string browseFolder(const char *message, string starting_path) {
    TCHAR path[MAX_PATH];

    BROWSEINFO bi = { 0 };
    bi.lpszTitle  = message;
    bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn       = browseFolderCB;
    bi.lParam     = (LPARAM)starting_path.c_str();

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != 0) {
        SHGetPathFromIDList(pidl, path);

        IMalloc *imalloc = 0;
        if(SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        return path;
    }

    return "";
}

static const string locateFTPath() {
	HKEY key;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\GOG.com\\GOGFALLOUTTACTICS\\"), 0, KEY_READ, &key) == ERROR_SUCCESS) {
		DWORD type;
		char path[1024];
		DWORD path_size = sizeof(path) - 1;
		if(RegQueryValueEx(key, "PATH", NULL, &type, (BYTE*)path, &path_size) == ERROR_SUCCESS) {
			path[path_size] = 0;
			if(verifyFTPath(path))
				return path;
		}
		RegCloseKey(key);
	}

	const char *std_paths[] = {
		"c:\\Program Files\\14 Degrees East\\Fallout Tactics\\",
		"c:\\Program Files (x86)\\14 Degrees East\\Fallout Tactics\\",
	};

	for(int n = 0; n < arraySize(std_paths); n++)
		if(verifyFTPath(std_paths[n]))
			return std_paths[n];

	return browseFolder("Please select folder in which Fallout Tactics is installed:", FilePath::current());
}

#endif

#ifdef USE_OPENMP
#include <omp.h>
#endif

struct TileMapProxy: public TileMap {
	void save(FileStream &sr) const {
		XmlDocument doc;
		saveToXML(doc);
		doc.save(sr).check();
	}
	// TODO: whats that for?
	void setResourceName(const char*) { }
};

struct SoundProxy {
	vector<char> data;

	template <class InputStream>
	Ex<void> legacyLoad(InputStream &sr, Str) {
		data.resize(sr.size());
		sr.loadData(data);
		return {};
	}
	void save(FileStream &sr) { sr.saveData(data); }
	void setResourceName(const char*) { }
};

DEFINE_ENUM(ResTypeId, sprite, tile, map, sound, image, music, archive);

static const EnumMap<ResTypeId, const char*> s_old_suffix = {{
	".spr",
	".til",
	".mis",
	".wav",
	".zar",
	".mp3",
	".bos"
}};

static const EnumMap<ResTypeId, const char*> s_new_suffix = {{
	".sprite",
	".tile",
	".xml",
	".wav",
	".zar",
	".mp3",
	nullptr
}};

static const EnumMap<ResTypeId, const char*> s_new_path = {{
	"data/sprites/",
	"data/tiles/",
	"data/maps/",
	"data/sounds/",
	"data/gui/",
	"data/music/",
	nullptr
}};

template <class InputStream>
Ex<void> convert(ResTypeId type, InputStream &ldr, FileStream &svr, Str name) {
	ASSERT(type != ResTypeId::archive);
	ON_FAIL("While converting '%' -> '%' (type: %)", name, svr.name(), type);

	if(type == ResTypeId::sprite) {
		Sprite res;
		EXPECT(res.legacyLoad(ldr, name));
		res.save(svr);
	}
	else if(type == ResTypeId::tile) {
		Tile res;
		EXPECT(res.legacyLoad(ldr, name));
		res.save(svr);
	}
	else if(type == ResTypeId::map) {
		TileMapProxy res;
		EXPECT(res.legacyConvert(ldr, svr));
	}
	else if(type == ResTypeId::image || type == ResTypeId::music) {
		vector<char> buffer;
		buffer.resize(ldr.size());
		ldr.loadData(buffer);
		svr.saveData(buffer);
	}
	else if(type == ResTypeId::sound) {
		SoundProxy proxy;
		EXPECT(proxy.legacyLoad(ldr, name));
		proxy.save(svr);
	}

	return {};
}

template <class TResource>
void convert(const char *src_dir, const char *dst_dir, const char *old_ext, const char *new_ext,
			bool detailed, const string &filter) {
	FilePath main_path = FilePath(src_dir).absolute().get();
	
	printf("Scanning...\n");
	auto file_names = findFiles(main_path, FindFileOpt::regular_file | FindFileOpt::recursive);
	std::sort(begin(file_names), end(file_names));
	int total_before = 0, total_after = 0;

	printf("Recreating directories...\n");
	for(int n = 0; n < (int)file_names.size(); n++) {
		FilePath path = file_names[n].path.relative(main_path);
		FilePath dir = FilePath(dst_dir) / path.parent();
		if(access(dir.c_str(), R_OK) != 0)
			mkdirRecursive(dir.c_str()).check();
	}
	
	if(!detailed) {
		printf("Converting");
		fflush(stdout);
	}

#pragma omp parallel for
	for(int n = 0; n < file_names.size(); n++) {
		if(!detailed && n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}
		FilePath full_path = file_names[n].path;
		if(((const string&)full_path).find(filter) == string::npos)
			continue;
		
#pragma omp task
		{	
			FilePath path = full_path.relative(main_path);
			string name = path.fileName();
			string lo_name = toLower(name);

			if(removeSuffix(lo_name, old_ext)) {
				name.resize(lo_name.size());

				FilePath new_path = FilePath(dst_dir) / path.parent() / (name + new_ext);
				FilePath parent = new_path.parent();

				// TODO: handle errors
				//try {
					TResource resource;
					auto source = move(fileLoader(full_path).get());
					double time = getTime();
					resource.legacyLoad(source, name).check();
					auto target = move(fileSaver(new_path).get());
					resource.save(target);
					resource.setResourceName((FilePath(name).fileName()).c_str()); // TODO: this isprobably not needed

					if(detailed)
						printf("%40s  %6dKB -> %6dKB   %9.4f ms\n", name.c_str(),
								(int)(source.size()/1024), (int)(target.size()/1024), (getTime() - time) * 1024.0);

#pragma omp atomic
						total_before += source.size();
#pragma omp atomic
						total_after += target.size();
				//} catch(const Exception &ex) {
				//	printf("Error while converting: %s:\n%s\n%s\n\n", full_path.c_str(), ex.what(), ex.backtrace().c_str());
				//}
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
	ResTypeId type;
};

static ResPath s_paths[] = {
	{ "tiles/", ResTypeId::tile },
	{ "sprites/", ResTypeId::sprite },
	{ "campaigns/missions/core/", ResTypeId::map },
	{ "campaigns/missions/tutorials/", ResTypeId::map },
	{ "missions/", ResTypeId::map },
	{ "sound/game/", ResTypeId::sound },
	{ "gui/", ResTypeId::image },
	{ "music/", ResTypeId::music },
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

void convertAll(const char *fot_path, const string &filter) {
	FilePath core_path = (FilePath(fot_path) / "core").absolute().get();

	printf("FOT core: %s\n", core_path.c_str());

	printf("Scanning...\n");
	auto all_files = findFiles(core_path, FindFileOpt::regular_file | FindFileOpt::recursive);
	std::sort(begin(all_files), end(all_files));
	printf("Found: %d files\n", (int)all_files.size());

	std::map<string, string> files[arraySize(s_paths)];
	bool only_archives = 0;
	unsigned long long bytes = 0;
	
	for(int n = 0; n < (int)all_files.size(); n++) {
		for(int t = 0; t < arraySize(s_paths); t++) {
			if(only_archives && s_paths[t].type != ResTypeId::archive)
				continue;

			string name = all_files[n].path.relative(core_path);
			string orig_name = name;

			if(name.find(filter) == string::npos)
				continue;

			if(removePrefix(name, s_paths[t].prefix)) {
				string lo_name = toLower(name);
				if(removeSuffix(lo_name, s_old_suffix[s_paths[t].type])) {
					name.resize(lo_name.size());
					files[t][name] = orig_name;
					break;
				}
			}
		}
	}

	printf("Converting plain files...\n");
	for(int t = 0; t < arraySize(s_paths); t++) {
		ResTypeId type = s_paths[t].type;
		if(type == ResTypeId::archive)
			continue;

		FilePath target_dir = s_new_path[type];
		FilePath src_main_path = core_path / s_paths[t].prefix;

		for(auto it = files[t].begin(); it != files[t].end(); ++it) {
			FilePath dir = target_dir / FilePath(it->first).parent();
			if(access(dir.c_str(), R_OK) != 0)
				mkdirRecursive(dir.c_str()).check();
		}

		for(auto it = files[t].begin(); it != files[t].end(); ++it) {
			auto src_path = format("%/%", core_path, it->second);
			auto dst_path = format("%/%%", s_new_path[type], it->first, s_new_suffix[type]);

			auto ldr = move(fileLoader(src_path).get());
			auto svr = move(fileSaver(dst_path).get());

			if(type != ResTypeId::tile || bytes > 1024 * 1024) {
				if(type == ResTypeId::tile) { printf("."); fflush(stdout); }
				else printf("%s\n", it->first.c_str());
				bytes = 0;
			}
			else 
				bytes += ldr.size();
			convert(type, ldr, svr, src_path).check(); // TODO
		}
	}
	
	printf("Converting archives...\n");
	for(int t = 0; t < arraySize(s_paths); t++) {
		ResTypeId type = s_paths[t].type;
		if(type != ResTypeId::archive)
			continue;

		FilePath src_path = core_path / s_paths[t].prefix;
		for(auto it = files[t].begin(); it != files[t].end(); ++it) {
			char archive_path[512];
			snprintf(archive_path, sizeof(archive_path), "%s/%s%s",
					src_path.c_str(), it->first.c_str(), s_old_suffix[type]);

			Archive archive(archive_path);
			printf("Archive %s: %d\n", it->first.c_str(), archive.fileCount());

			vector<string> names = archive.getNames();
			for(int n = 0; n < (int)names.size(); n++) {
				string name;
				int tindex = -1;

				for(int t = 0; t < arraySize(s_paths); t++) {
					name = names[n];
					string lo_name = toLower(name);
					if(removePrefix(lo_name, s_paths[t].prefix)) {
						name = name.substr(name.size() - lo_name.size());
						lo_name = toLower(name);
					   	if(removeSuffix(lo_name, s_old_suffix[s_paths[t].type])) {
							name.resize(lo_name.size());
							tindex = t;
							break;
						}
					}
				}

				if(tindex == -1 || files[tindex].find(name) != files[tindex].end())
					continue;
				ResTypeId type = s_paths[tindex].type;

				vector<char> data;
				archive.readFile(n, data);

				char dst_path[512];
				snprintf(dst_path, sizeof(dst_path), "%s/%s%s", s_new_path[type], name.c_str(), s_new_suffix[type]);

				FilePath dir = FilePath(dst_path).parent();
				if(access(dir.c_str(), R_OK) != 0)
					mkdirRecursive(dir.c_str()).check();

				auto ldr = memoryLoader(data);
				auto svr = move(fileSaver(dst_path).get());

				if((type != ResTypeId::tile && type != ResTypeId::sound) || bytes > 1024 * 1024) {
					if(type == ResTypeId::tile || type == ResTypeId::sound) {
						printf(".");
						fflush(stdout);
					}
					else
						printf("%s\n", name.c_str());
					bytes = 0;
				}
				else 
					bytes += ldr.size();
				convert(type, ldr, svr, name).check();
			}
				
			printf("\n");
		}
	}

	printf("\nDone.\n");
}

int main(int argc, char **argv) {
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

#ifdef _WIN32
	if(argc == 1) {
		path = locateFTPath();

		if(path.empty()) {
			printf("Cannot find Fallout Tactics installation. Please install FT and then rerun this program.\n");
			return 0;
		}

		printf("Found FT installed in: %s\n", path.c_str());
		command = "all";
	}
#endif

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
	else if(command == "sounds") {
		if(path.empty())
			path = "refs/sound/game/";
		convert<SoundProxy>(path.c_str(), "data/sounds/", ".wav", ".wav", 1, filter);
	}

	else if(command == "all") {
		if(path.empty())
			path = "refs/";
		if(!verifyFTPath(path)) {
#ifdef _WIN32
			MessageBox(0, "Invalid path specified!", "Error", MB_OK);
#endif
			printf("Invalid path specified\n");
			return 0;
		}
		convertAll(path.c_str(), filter);

#ifdef _WIN32
		MessageBox(0, "All done!", "Message", MB_OK);
#endif
	}
	else {
		if(!command.empty())
			printf("Unknown command: %s\n", command.c_str());

		printf("Usage:\n%s [options] tiles|sprites|maps|all\n\n"
				"tiles:       converting tiles in .til format from refs/tiles/ directory\n"
				"sprites:     converting sprites in .spr format from refs/sprites/ directory\n"
				"maps:        converting maps in .mis format from refs/maps directory\n"
				"sounds:      copying    sounds in .wav format from refs/sound/game directory\n"
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
