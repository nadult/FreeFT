#include "base.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <cstring>
#include <cmath>

float g_FloatParam[16];


typedef float Matrix3[3][3];

bool inverse(const Matrix3 &mat) {
	float3 out[3];

    out[0].x = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];
    out[0].y = mat[0][2] * mat[2][1] - mat[0][1] * mat[2][2];
    out[0].z = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
    out[1].x = mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2];
    out[1].y = mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0];
    out[1].z = mat[0][2] * mat[1][0] - mat[0][0] * mat[1][2];
    out[2].x = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];
    out[2].y = mat[0][1] * mat[2][0] - mat[0][0] * mat[2][1];
    out[2].z = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

	float det = mat[0][0] * out[0].x + mat[0][1] * out[1].x + mat[0][2] * out[2].x;
	//TODO what if det close to 0?
	float idet = 1.0f / det;

	out[0] = out[0] * idet;
	out[1] = out[1] * idet;
	out[2] = out[2] * idet;

	for(int i = 0; i < 3; i++) {
		printf("%f %f %f\n", out[i].x, out[i].y, out[i].z);
	}
	exit(0);
	return 1;
}

float mat[3][3] = {
	{6, 3, 7},
	{0, -7, 6},
	{-6, 3, 7}
};

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

const IRect worldToScreen(const IBox &bbox) {
	int2 corners[4] = {
			worldToScreen(int3(bbox.max.x, bbox.min.y, bbox.min.z)),
			worldToScreen(int3(bbox.min.x, bbox.min.y, bbox.max.z)),
			worldToScreen(int3(bbox.max.x, bbox.min.y, bbox.max.z)),
			worldToScreen(int3(bbox.min.x, bbox.max.y, bbox.min.z)) };

	return IRect(corners[1].x, corners[3].y, corners[0].x, corners[2].y);
}

float intersection(const Ray &ray, const Box<float3> &box) {
	//TODO: check if works correctly for (+/-)INF
	float3 inv_dir = ray.invDir();
	float3 origin = ray.origin();

	float l1   = inv_dir.x * (box.min.x - origin.x);
	float l2   = inv_dir.x * (box.max.x - origin.x);
	float lmin = min(l1, l2);
	float lmax = max(l1, l2);

	l1   = inv_dir.y * (box.min.y - origin.y);
	l2   = inv_dir.y * (box.max.y - origin.y);
	lmin = max(min(l1, l2), lmin);
	lmax = min(max(l1, l2), lmax);

	l1   = inv_dir.z * (box.min.z - origin.z);
	l2   = inv_dir.z * (box.max.z - origin.z);
	lmin = max(min(l1, l2), lmin);
	lmax = min(max(l1, l2), lmax);

	return lmin < lmax? lmin : 1.0f / 0.0f;
}

Ray screenRay(int2 screen_pos) {
	float3 origin = asXZ(screenToWorld((float2)screen_pos));
	float3 dir = float3(-1.0f / 6.0f, -1.0f / 7.0f, -1.0f / 6.0f);
	return Ray(origin, dir / length(dir));
}

float dot(const float2 &a, const float2 &b) { return a.x * b.x + a.y * b.y; }
float dot(const float3 &a, const float3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float dot(const float4 &a, const float4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

float lengthSq(const float2 &v) { return dot(v, v); }
float lengthSq(const float3 &v) { return dot(v, v); }
float distanceSq(const float3 &a, const float3 &b) { return lengthSq(a - b); }

float length(const float2 &v) { return sqrt(lengthSq(v)); }
float length(const float3 &v) { return sqrt(lengthSq(v)); }
float distance(const float3 &a, const float3 &b) { return sqrt(distanceSq(a, b)); }

MoveVector::MoveVector(const int2 &start, const int2 &end) {
	int2 diff = end - start;
	vec.x = diff.x < 0? -1 : diff.x > 0? 1 : 0;
	vec.y = diff.y < 0? -1 : diff.y > 0? 1 : 0;
	dx = abs(diff.x);
	dy = abs(diff.y);
	ddiag = min(dx, dy);
	dx -= ddiag;
	dy -= ddiag;
}
MoveVector::MoveVector() :vec(0, 0), dx(0), dy(0), ddiag(0) { }


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

