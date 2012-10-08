#include "base.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <cstring>

float g_FloatParam[16];

float2 WorldToScreen(float3 pos) {
	return float2(6.0f * (pos.x - pos.z), 3.0f * (pos.x + pos.z) - 7.0f * pos.y);
}

int2 WorldToScreen(int3 pos) {
	return int2(6 * (pos.x - pos.z), 3 * (pos.x + pos.z) - 7 * pos.y);
}

float2 ScreenToWorld(float2 pos) {
	float x = pos.x * (1.0f / 12.0f);
	float y = pos.y * (1.0f / 6.0f);

	return float2(y + x, y - x);
}

int2 ScreenToWorld(int2 pos) {
	int x = pos.x / 12;
	int y = pos.y / 6;

	return int2(y + x, y - x);
}

static void FindFiles(vector<string> &out, const char *dirName, const char *ext, bool recursive) {
	DIR *dp = opendir(dirName);
	if(!dp)
		ThrowException("Error while opening directory ", dirName, ": ", strerror(errno));


	try {
		size_t extLen = strlen(ext);
		struct dirent *dirp;

		while ((dirp = readdir(dp))) {
			char fullName[FILENAME_MAX];
			struct stat fileInfo;

			snprintf(fullName, sizeof(fullName), "%s/%s", dirName, dirp->d_name);
			if(lstat(fullName, &fileInfo) < 0)
				continue; //TODO: handle error

			if(S_ISDIR(fileInfo.st_mode) && recursive) {
				if(strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, ".."))
					FindFiles(out, fullName, ext, recursive);
			}
			else {
				size_t len = strlen(dirp->d_name);
				if(len >= extLen && strcmp(dirp->d_name + len - extLen, ext) == 0)
					out.push_back(string(dirName) + '/' + dirp->d_name);
			}
		}
	}
	catch(...) {
		closedir(dp);
		throw;
	}
	closedir(dp);
}

const vector<string> FindFiles(const char *tDirName, const char *ext, bool recursive) {
	string dirName = tDirName;
	if(!dirName.empty() && dirName[dirName.size() - 1] == '/')
		dirName.resize(dirName.size() - 1);

	vector<string> out;
	FindFiles(out, dirName.c_str(), ext, recursive);
	return out;
}

/* 

#include <wx/app.h>
#include <wx/utils.h>
#include <wx/filedlg.h> 

string OpenFileDialog(bool save) {
	wxInitialize();

	wxString fileName = wxFileSelector("Choose file", "", "", ".map", ".", save?wxFD_SAVE : wxFD_OPEN);
	return fileName.fn_str();
}

#include <gtkmm.h>

namespace
{

	Gtk::Main *gtkMain = 0;

	void FreeGtk() {
		if(gtkMain) {
			delete gtkMain;
			gtkMain = 0;
		}
	}

}

string OpenFileDialog(bool save) {
	if(!gtkMain) {
		gtkMain = new Gtk::Main(0, 0);
		atexit(FreeGtk);
	}

	string fileName; {
		Gtk::FileChooserDialog dialog("Please choose a file",
			  save? Gtk::FILE_CHOOSER_ACTION_SAVE : Gtk::FILE_CHOOSER_ACTION_OPEN);

		//Add response buttons the the dialog:
		dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dialog.add_button(save? "Save" : "Open", Gtk::RESPONSE_OK);

		Gtk::FileFilter filter;
		filter.set_name("tile map files");
		filter.add_pattern("*.tilemap");
		dialog.add_filter(filter);

		if(dialog.run() == Gtk::RESPONSE_OK)
			fileName = dialog.get_filename().c_str();
	}
	delete gtkMain;

	return fileName;
}

*/
