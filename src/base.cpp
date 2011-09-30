#include "base.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <cstring>

float g_FloatParam[16];

float2 WorldToScreen(float3 pos) {
	float xMul = 0.8571f * 7.0f;
	float yMul = 0.4300f * 7.0f;

	return float2((pos.x - pos.z) * xMul, (pos.x + pos.z) * yMul - pos.y * 7.0f);
}

float2 ScreenToWorld(float2 pos) {
	float xMul = 0.5f / (0.8571f * 7.0f);
	float yMul = 0.5f / (0.4300f * 7.0f);

	float x = pos.x * xMul;
	float y = pos.y * yMul;

	return float2(y + x, y - x);
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
