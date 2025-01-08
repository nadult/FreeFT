// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/sprite.h"
#include "game/tile.h"
#include "game/tile_map.h"

#include <fwk/io/file_stream.h>
#include <fwk/io/file_system.h>
#include <fwk/sys/expected.h>
#include <fwk/sys/on_fail.h>

#include <algorithm>
#include <set>
#include <zip.h>

using game::Sprite;
using game::Tile;
using game::TileMap;

static bool verifyFTPath(string path) { return access(FilePath(path) / "core"); }

#ifdef _WIN32

#include <shlobj.h>
#include <windows.h>

static int toUnicode(Str src, Span<WCHAR> dst) {
	int result =
		MultiByteToWideChar(CP_UTF8, 0, src.data(), src.size() + 1, dst.data(), dst.size());
	ASSERT(result > 0);
	return result - 1;
}

static string fromUnicode(Span<WCHAR> src) {
	int dst_len = WideCharToMultiByte(CP_UTF8, 0, src.data(), -1, nullptr, 0, 0, 0);
	vector<char> out(dst_len + 1, 0);
	int result = WideCharToMultiByte(CP_UTF8, 0, src.data(), -1, out.data(), out.size() + 1, 0, 0);
	ASSERT(result > 0);
	return out.data();
}

static int CALLBACK browseFolderCB(HWND hwnd, UINT msg, LPARAM lparam, LPARAM data) {
	if(msg == BFFM_INITIALIZED) {
		LPCTSTR path = reinterpret_cast<LPCTSTR>(data);
		SendMessage(hwnd, BFFM_SETSELECTION, true, (LPARAM)path);
	}

	return 0;
}

string browseFolder(const char *message, string starting_path) {
	WCHAR wmessage[1024];
	toUnicode(message, wmessage);
	WCHAR path[MAX_PATH];

	BROWSEINFO bi;
	memset(&bi, 0, sizeof(bi));
	bi.lpszTitle = wmessage;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = browseFolderCB;
	bi.lParam = (LPARAM)starting_path.c_str();

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if(pidl != 0) {
		SHGetPathFromIDList(pidl, path);

		IMalloc *imalloc = 0;
		if(SUCCEEDED(SHGetMalloc(&imalloc))) {
			imalloc->Free(pidl);
			imalloc->Release();
		}

		return fromUnicode(path);
	}
	return "";
}

static const string locateFT() {
	vector<string> paths;
	HKEY key;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1440152063\\"),
					0, KEY_READ, &key) == ERROR_SUCCESS) {
		std::byte data[1024 + 1];
		DWORD data_size = sizeof(data) - 1;
		if(RegGetValue(key, NULL, L"PATH", RRF_RT_REG_SZ, NULL, data, &data_size) ==
		   ERROR_SUCCESS) {
			Span<WCHAR> wchars((WCHAR *)data, data_size + 1);
			wchars[data_size] = 0;
			paths.emplace_back(fromUnicode(wchars));
		}
		RegCloseKey(key);
	}

	insertBack(paths, {
						  "C:\\Program Files\\14 Degrees East\\Fallout Tactics\\",
						  "C:\\Program Files (x86)\\14 Degrees East\\Fallout Tactics\\",
					  });

	for(auto path : paths)
		if(verifyFTPath(path))
			return path;

	auto current = FilePath::current().get();
	return browseFolder("Please select the folder in which Fallout Tactics is installed:", current);
}

#endif

struct SoundProxy {
	vector<char> data;

	template <class InputStream> Ex<void> legacyLoad(InputStream &sr, Str) {
		data.resize(sr.size());
		sr.loadData(data);
		return {};
	}
	void save(FileStream &sr) { sr.saveData(data); }
	void setResourceName(const char *) {}
};

DEFINE_ENUM(ResTypeId, sprite, tile, map, sound, image, music, archive);

static const EnumMap<ResTypeId, const char *> s_old_suffix = {
	{".spr", ".til", ".mis", ".wav", ".zar", ".mp3", ".bos"}};

static const EnumMap<ResTypeId, const char *> s_new_suffix = {
	{".sprite", ".tile", ".xml", ".wav", ".zar", ".mp3", nullptr}};

static const EnumMap<ResTypeId, const char *> s_new_path = {{"data/sprites/", "data/tiles/",
															 "data/maps/", "data/sounds/",
															 "data/gui/", "data/music/", nullptr}};

template <class InputStream>
Ex<void> convertResource(ResTypeId type, InputStream &ldr, FileStream &svr, Str name) {
	ASSERT(type != ResTypeId::archive);
	ON_FAIL("While converting '%' -> '%' (type: %)", name, svr.name(), type);

	if(type == ResTypeId::sprite) {
		Sprite res;
		EXPECT(res.legacyLoad(ldr, name));
		res.save(svr);
	} else if(type == ResTypeId::tile) {
		Tile res;
		EXPECT(res.legacyLoad(ldr, name));
		res.save(svr);
	} else if(type == ResTypeId::map) {
		EXPECT(TileMap::legacyConvert(ldr, svr));
	} else if(type == ResTypeId::image || type == ResTypeId::music) {
		vector<char> buffer;
		buffer.resize(ldr.size());
		ldr.loadData(buffer);
		svr.saveData(buffer);
	} else if(type == ResTypeId::sound) {
		SoundProxy proxy;
		EXPECT(proxy.legacyLoad(ldr, name));
		proxy.save(svr);
	}

	return {};
}

Pair<i64> convertResource(ResTypeId res_type, ZStr src_file_name, ZStr dst_file_name, Str name) {
	// TODO: errors
	auto source = fileLoader(src_file_name);
	if(!source)
		print("Error while opening file for reading: '%'\n", src_file_name);

	auto target = fileSaver(dst_file_name);
	if(!target)
		print("Error while opening file for writing: '%'\n", dst_file_name);
	auto result = convertResource(res_type, *source, *target, name);
	if(!result) {
		print("Error while converting % '%' -> '%':\n", res_type, src_file_name, dst_file_name);
		result.error().print();
	}
	return {source ? source->size() : 0, target ? target->size() : 0};
}

template <ResTypeId res_type>
void convertDir(const char *src_dir, const char *dst_dir, const char *old_ext, const char *new_ext,
				bool detailed, const string &filter) {
	FilePath main_path = FilePath(src_dir).absolute().get();

	printf("Scanning...\n");
	auto file_names = findFiles(main_path, FindFileOpt::regular_file | FindFileOpt::recursive);
	makeSorted(file_names);
	int total_before = 0, total_after = 0;

	printf("Recreating directories...\n");
	for(auto &fname : file_names) {
		FilePath path = fname.path.relative(main_path);
		FilePath dir = FilePath(dst_dir) / path.parent();
		mkdirRecursive(dir).check();
	}

	if(!detailed) {
		printf("Converting");
		fflush(stdout);
	}

	for(int n = 0; n < file_names.size(); n++) {
		if(!detailed && n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}
		FilePath full_path = file_names[n].path;
		if(((const string &)full_path).find(filter) == string::npos)
			continue;

		FilePath path = full_path.relative(main_path);
		string name = path.fileName();
		string lo_name = toLower(name);

		if(removeSuffix(lo_name, old_ext)) {
			name.resize(lo_name.size());

			FilePath new_path = FilePath(dst_dir) / path.parent() / (name + new_ext);
			FilePath parent = new_path.parent();

			double time = getTime();
			auto [src_size, dst_size] = convertResource(res_type, full_path, new_path, name);
			time = getTime() - time;
			if(detailed)
				printf("%40s  %6dKB -> %6dKB   %9.4f ms\n", name.c_str(), (int)(src_size / 1024),
					   (int)(dst_size / 1024), time * 1024.0);
			total_before += src_size;
			total_after += dst_size;
		}
	}

	if(!detailed)
		printf("\n");
	printf("Total: %6dKB -> %6dKB\n", total_before / 1024, total_after / 1024);
}

struct ResPath {
	const char *prefix;
	ResTypeId type;
};

static ResPath s_paths[] = {{"tiles/", ResTypeId::tile},
							{"sprites/", ResTypeId::sprite},
							{"campaigns/missions/core/", ResTypeId::map},
							{"campaigns/missions/tutorials/", ResTypeId::map},
							{"campaigns/missions/demo/", ResTypeId::map},
							{"missions/", ResTypeId::map},
							{"sound/game/", ResTypeId::sound},
							{"gui/", ResTypeId::image},
							{"music/", ResTypeId::music},
							{"", ResTypeId::archive}};

class Archive {
  public:
	Archive(const char *file_name) {
		m_zip = zip_open(file_name, 0, 0);
		ASSERT(m_zip);
		m_file_count = zip_get_num_entries(m_zip, 0);
	}
	~Archive() { zip_close(m_zip); }
	Archive(const Archive &) = delete;
	void operator=(const Archive &) = delete;

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

	int fileCount() const { return m_file_count; }

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

	for(int n = 0; n < all_files.size(); n++) {
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
			mkdirRecursive(dir).check();
		}

		for(auto it = files[t].begin(); it != files[t].end(); ++it) {
			auto src_path = format("%/%", core_path, it->second);
			auto dst_path = format("%/%%", s_new_path[type], it->first, s_new_suffix[type]);
			auto [src_size, dst_size] = convertResource(type, src_path, dst_path, src_path);

			if(type != ResTypeId::tile || bytes > 1024 * 1024) {
				if(type == ResTypeId::tile) {
					printf(".");
					fflush(stdout);
				} else {
					printf("%s\n", it->first.c_str());
				}
				bytes = 0;
			} else {
				bytes += src_size;
			}
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
			snprintf(archive_path, sizeof(archive_path), "%s/%s%s", src_path.c_str(),
					 it->first.c_str(), s_old_suffix[type]);

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
				snprintf(dst_path, sizeof(dst_path), "%s/%s%s", s_new_path[type], name.c_str(),
						 s_new_suffix[type]);

				FilePath dir = FilePath(dst_path).parent();
				mkdirRecursive(dir).check();

				auto ldr = memoryLoader(data);
				auto svr = std::move(fileSaver(dst_path).get());

				if((type != ResTypeId::tile && type != ResTypeId::sound) || bytes > 1024 * 1024) {
					if(type == ResTypeId::tile || type == ResTypeId::sound) {
						printf(".");
						fflush(stdout);
					} else {
						printf("%s\n", name.c_str());
					}
					bytes = 0;
				} else
					bytes += ldr.size();
				auto result = convertResource(type, ldr, svr, name);
				if(!result)
					result.error().print();
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
		} else if(strcmp(argv[n], "-p") == 0 && n + 1 < argc) {
			ASSERT(path.empty());
			path = argv[++n];
		} else {
			ASSERT(command.empty());
			command = argv[n];
		}
	}

#ifdef _WIN32
	if(argc == 1) {
		path = locateFT();

		if(path.empty()) {
			printf("Cannot find Fallout Tactics installation. Please install FT and then rerun "
				   "this program.\n");
			return 0;
		}

		printf("Found FT installed in: %s\n", path.c_str());
		command = "all";
	}
#endif

	if(command == "tiles") {
		if(path.empty())
			path = "refs/tiles/";
		convertDir<ResTypeId::tile>(path.c_str(), "data/tiles", ".til", ".tile", 0, filter);
	} else if(command == "maps") {
		if(path.empty())
			path = "refs/maps/";
		convertDir<ResTypeId::map>(path.c_str(), "data/maps/", ".mis", ".xml", 1, filter);
	} else if(command == "sprites") {
		if(path.empty())
			path = "refs/sprites/";
		convertDir<ResTypeId::sprite>(path.c_str(), "data/sprites/", ".spr", ".sprite", 1, filter);
	} else if(command == "sounds") {
		if(path.empty())
			path = "refs/sound/game/";
		convertDir<ResTypeId::sound>(path.c_str(), "data/sounds/", ".wav", ".wav", 1, filter);
	}

	else if(command == "all") {
		if(path.empty())
			path = "refs/";
		if(!verifyFTPath(path)) {
#ifdef _WIN32
			MessageBox(0, L"Invalid path specified!", L"Error", MB_OK);
#endif
			printf("Invalid path specified\n");
			return 0;
		}
		convertAll(path.c_str(), filter);

#ifdef _WIN32
		MessageBox(0, L"All done!", L"Message", MB_OK);
#endif
	} else {
		if(!command.empty())
			printf("Unknown command: %s\n", command.c_str());

		printf(
			"Usage:\n%s [options] tiles|sprites|maps|all\n\n"
			"tiles:       converting tiles in .til format from refs/tiles/ directory\n"
			"sprites:     converting sprites in .spr format from refs/sprites/ directory\n"
			"maps:        converting maps in .mis format from refs/maps directory\n"
			"sounds:      copying    sounds in .wav format from refs/sound/game directory\n"
			"all:         converting everyting (also from .bos files), path has to be specified\n\n"
			"Options:\n"
			"-f filter    Converting only those files that match given filter\n"
			"-p path      Specify different path\n"
			"\n",
			argv[0]);
		return 0;
	}

	return 0;
}
