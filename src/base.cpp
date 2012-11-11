#include "base.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <cstring>
#include <cmath>

float g_FloatParam[16];

/* World To Screen Matrix:
 * 			    | 6  3  7|
 * 		*	    | 0 -7  6|
 * 			    |-6  3  7|
 *
 * [WX WY WZ]   [SX SY SZ]
 *
 */

float2 worldToScreen(float3 pos) {
	return float2(	6.0f * (pos.x - pos.z),
					3.0f * (pos.x + pos.z) - 7.0f * pos.y);
				//	7.0f * (pos.x + pos.z) + 6.0f * pos.y);
}

int2 worldToScreen(int3 pos) {
	return int2(	6 * (pos.x - pos.z),
					3 * (pos.x + pos.z) - 7 * pos.y);
				//	7 * (pos.x + pos.z) - 6 * pos.y);
}

float2 screenToWorld(float2 pos) {
	float x = pos.x * (1.0f / 12.0f);
	float y = pos.y * (1.0f / 6.0f);

	return float2(y + x, y - x);
}

int2 screenToWorld(int2 pos) {
	int x = pos.x / 12;
	int y = pos.y / 6;

	return int2(y + x, y - x);
}


float dot(const float2 &a, const float2 &b) { return a.x * b.x + a.y * b.y; }
float dot(const float3 &a, const float3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float dot(const float4 &a, const float4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

float lengthSq(const float3 &v) { return dot(v, v); }
float distanceSq(const float3 &a, const float3 &b) { return lengthSq(a - b); }

float length(const float3 &v) { return sqrt(lengthSq(v)); }
float distance(const float3 &a, const float3 &b) { return sqrt(distanceSq(a, b)); }


static void findFiles_(vector<string> &out, const char *dirName, const char *ext, bool recursive) {
	DIR *dp = opendir(dirName);
	if(!dp)
		THROW("Error while opening directory %s: %s", dirName, strerror(errno));

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
					findFiles_(out, fullName, ext, recursive);
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

void findFiles(vector<string> &out, const char *tDirName, const char *ext, bool recursive) {
	string dirName = tDirName;
	if(!dirName.empty() && dirName[dirName.size() - 1] == '/')
		dirName.resize(dirName.size() - 1);
	findFiles_(out, dirName.c_str(), ext, recursive);
}

using namespace rapidxml;

void addAttribute(XMLNode *node, const char *name, float value) {
	XMLDocument *doc = node->document();
	char str_value[64];
	snprintf(str_value, sizeof(str_value), "%f", value);
	XMLAttribute *attrib = doc->allocate_attribute(name, doc->allocate_string(str_value));
	node->append_attribute(attrib);
}

void addAttribute(XMLNode *node, const char *name, int value) {
	XMLDocument *doc = node->document();
	char str_value[32];
	sprintf(str_value, "%d", value);
	XMLAttribute *attrib = doc->allocate_attribute(name, doc->allocate_string(str_value));
	node->append_attribute(attrib);
}

void addAttribute(XMLNode *node, const char *name, const char *value) {
	XMLDocument *doc = node->document();
	XMLAttribute *attrib = doc->allocate_attribute(name, doc->allocate_string(value));
	node->append_attribute(attrib);
}

int getIntAttribute(XMLNode *node, const char *name) {
	XMLAttribute *attrib = node->first_attribute(name);
	return attrib? atoi(attrib->value()) : 0;
}

int getFloatAttribute(XMLNode *node, const char *name) {
	XMLAttribute *attrib = node->first_attribute(name);
	return attrib? atof(attrib->value()) : 0;
}

const char *getStringAttribute(XMLNode *node, const char *name) {
	XMLAttribute *attrib = node->first_attribute(name);
	return attrib? attrib->value() : 0;
}

