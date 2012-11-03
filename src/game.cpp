#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"
#include "tile_map.h"
#include "tile_group.h"
#include "sys/profiler.h"
#include <fstream>
#include <unistd.h>

using namespace gfx;

class Entity {
public:
	Entity(int3 bbox, int3 pos) :m_bbox(bbox), m_pos((float3)pos) { }
	virtual ~Entity() { }

	virtual void draw() const {

	}

	float3 m_pos;
	int3 m_bbox;
};



class Actor: public Entity {
public:
	Actor(const char *spr_name, int3 pos) :Entity(int3(1, 1, 1), pos) {
		m_sprite = Sprite::mgr[spr_name];
		m_bbox = m_sprite->m_bbox;

		setSequence("Stand");
		lookAt(int3(0, 0, 0));
		m_order = oNothing;

		for(int a = 0; a < (int)m_sprite->m_anims.size(); a++) {
			const Sprite::Animation &anim = m_sprite->m_anims[a];
			printf("Anim %d: %s (%d*%d frames)\n", a, anim.m_name.c_str(),
					(int)anim.images.size() / anim.m_dir_count, anim.m_dir_count);
		}
		for(int s = 0; s < (int)m_sprite->m_sequences.size(); s++) {
			const Sprite::Sequence &seq = m_sprite->m_sequences[s];
			printf("Seq %d: %s (%d frames, anim: %d)\n", s, seq.m_name.c_str(),
					(int)seq.m_frames.size(), seq.m_anim_id);
		}
	}


	void moveTo(int3 new_pos, bool run) {
		new_pos = Max(new_pos, int3(0, 0, 0)); //TODO: clamp to map extents
		new_pos.y = 0; //TODO: y

		int x_diff = new_pos.x - (int)m_pos.x;
		int z_diff = new_pos.z - (int)m_pos.z;
		int3 dir(x_diff < 0? -1 : 1, 0, z_diff < 0? -1 : 1);
		x_diff = abs(x_diff);
		z_diff = abs(z_diff);
		int diag_diff = Min(x_diff, z_diff);
		x_diff -= diag_diff;
		z_diff -= diag_diff;

		m_path.clear();
		m_path_t = 0;
		m_path_pos = 0;
		m_order = oNothing;
		fixPos();

		int3 cur_pos = (int3)m_pos;
		if(cur_pos == new_pos)
			return;

		DAssert(diag_diff || x_diff || z_diff);

		while(diag_diff) {
			int dstep = Min(diag_diff, 3);
			m_path.push_back(cur_pos += dir * dstep);
			diag_diff -= dstep;
		}

		while(x_diff) {
			int step = Min(x_diff, 3);
			m_path.push_back(cur_pos += int3(dir.x * step, 0, 0));
			x_diff -= step;
		}
		while(z_diff) {
			int step = Min(z_diff, 3);
			m_path.push_back(cur_pos += int3(0, 0, dir.z * step));
			z_diff -= step;
		}

		DAssert(!m_path.empty());
		m_order = run? oRun : oWalk;
	}

	void think(double current_time, double time_delta) {
		if(m_order == oNothing) {
			fixPos();
			setSequence("Stand");
		}
		else if(m_order == oWalk || m_order == oRun) {
			DAssert(!m_path.empty() && m_path_pos >= 0 && m_path_pos < (int)m_path.size());

			setSequence(m_order == oWalk? "StandWalk" : "StandRun");
			float speed = m_order == oWalk? 6.0f : 20.0f;
			float dist = speed * time_delta;


			while(dist > 0.0001f) {
				int3 target = m_path[m_path_pos];
				int3 diff = target - m_last_pos;
				float3 diff_vec(diff); diff_vec = diff_vec / sqrt(LengthSq(diff_vec));
				float3 cur_pos = float3(m_last_pos) + float3(diff) * m_path_t;
				float tdist = sqrt(DistanceSq(float3(target), cur_pos));
				float new_x = cur_pos.x + diff_vec.x * dist;
				float new_z = cur_pos.z + diff_vec.z * dist;
				float new_t = diff.x? (new_x - m_last_pos.x) / float(diff.x) : (new_z - m_last_pos.z) / float(diff.z);
				
//				printf("dist:%.2f tdist:%.2f last:(%d, %d)  cur:(%.2f %.2f) diff:(%d %d) target:(%d %d) t:%.2f new_x:%.2f new_t:%.2f\n",
//						dist, tdist, m_last_pos.x, m_last_pos.z, cur_pos.x, cur_pos.z, diff.x, diff.z, target.x, target.z, m_path_t, new_x, new_t);

				if(tdist < dist) {
					dist -= tdist;
					m_last_pos = target;
					m_path_t = 0.0f;

					if(++m_path_pos == (int)m_path.size()) {
						lookAt(target);
						m_pos = target;
						m_order = oNothing;
						m_path.clear();
						break;
					}
				}
				else {
					float new_x = cur_pos.x + diff_vec.x * dist;
					float new_z = cur_pos.z + diff_vec.z * dist;
					m_path_t = diff.x? (new_x - m_last_pos.x) / float(diff.x) : (new_z - m_last_pos.z) / float(diff.z);
					float3 new_pos = (float3)m_last_pos + float3(diff) * m_path_t;
					lookAt(target);
					m_pos = new_pos;
					break;
				}
			}
		}

		animate(current_time);
	}

	void draw() const {
		Sprite::Rect rect;
		Texture frame =
			m_sprite->getFrame(m_seq_id, m_frame_id, m_dir, &rect);

		DTexture sprTex;
		sprTex.SetSurface(frame);
		sprTex.Bind();

		int2 size(rect.right - rect.left, rect.bottom - rect.top);
		float2 pos = WorldToScreen(m_pos);

		DrawQuad(pos.x + rect.left - m_sprite->m_offset.x, pos.y + rect.top - m_sprite->m_offset.y, size.x, size.y);
	}

protected:
	void fixPos() {
		m_pos = (int3)(m_pos + float3(0.5f, 0, 0.5f));
	}

	// sets seq_id, frame_id and seq_name
	void setSequence(const char *name) {
		if(m_seq_name == name)
			return;

		int seq_id = m_sprite->findSequence(name);
		Assert(seq_id != -1);

		m_seq_id = seq_id;
		m_frame_id = 0;
		m_seq_name = name;
	}

	// sets direction
	void lookAt(int3 pos) { //TODO: rounding
		float dx = (float)pos.x - m_pos.x, dz = (float)pos.z - m_pos.z;
		int dir = Sprite::findDir(dx < 0? -1 : dx > 0? 1 : 0, dz < 0? -1 : dz > 0? 1 : 0);
		//TODO: sprites have varying number of directions
		// maybe a vector (int2) should be passed to sprite, and it should
		// figure out a best direction by itself
		m_dir = dir;
	}

	void animate(double current_time) {
		const Sprite::Sequence &seq	= m_sprite->m_sequences[m_seq_id];
		const Sprite::Animation &anim = m_sprite->m_anims[seq.m_anim_id];
		
		double diff_time = current_time - m_last_time;
		if(diff_time > 1 / 15.0) {
			m_frame_id = (m_frame_id + 1) % m_sprite->frameCount(m_seq_id);
			m_last_time = current_time;
		}
	}


	enum Order {
		oNothing,
		oWalk,
		oRun,
	} m_order;

	const char *m_seq_name;
	int m_seq_id, m_frame_id;
	int m_dir;

	double m_last_time;
	PSprite m_sprite;

	int3 m_last_pos;
	float m_path_t;
	int m_path_pos;
	vector<int3> m_path;
};

int safe_main(int argc, char **argv)
{
	int2 res(1400, 768);

	CreateWindow(res, false);
	SetWindowTitle("FTremake ver 0.02");
	GrabMouse(false);

//	DTexture tex;
//	Loader("../data/epic_boobs.png") & tex;

	//const char *mapName = argc > 1? argv[1] : "../data/test.map";

	SetBlendingMode(bmNormal);

	Actor actor("characters/LeatherFemale", int3(0, 0, 0));

	vector<string> file_names;
	FindFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Snow/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Rock/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/RAIDERS/", ".til", 1);
//	FindFiles(file_names, "../refs/tiles/", ".til", 1);
	//vector<string> file_names = FindFiles("../refs/tiles/RAIDERS", ".til", 1);
	//vector<string> file_names = FindFiles("../refs/tiles/VAULT/", ".til", 1);

	printf("Loading... ");
	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		Ptr<Tile> tile = Tile::mgr.Load(file_names[n]);
		tile->name = file_names[n];
		tile->LoadDTexture();
	}
	printf("\n");

	int2 view_pos(0, 0);

	PFont font = Font::mgr["font1"];
	PTexture fontTex = Font::tex_mgr["font1"];

	TileMap tile_map;
	if(access("../data/tile_map.xml", R_OK) == 0) {
		string text;
		Loader ldr("../data/tile_map.xml");
		text.resize(ldr.Size());
		ldr.Data(&text[0], ldr.Size());
		XMLDocument doc;
		doc.parse<0>(&text[0]); 
		tile_map.loadFromXML(doc);
	}

	double last_time = GetTime();

	while(PollEvents()) {
		if(IsKeyDown(Key_esc))
			break;

		if((IsKeyPressed(Key_lctrl) && IsMouseKeyPressed(0)) || IsMouseKeyPressed(2))
			view_pos -= GetMouseMove();

		if(IsMouseKeyPressed(0) && !IsKeyPressed(Key_lctrl)) {
			int3 wpos = AsXZ(ScreenToWorld(GetMousePos() + view_pos));
			actor.moveTo(wpos, true);
		}

		double time = GetTime();
		actor.think(time, time - last_time); //TODO: problem with delta in the first frame
		last_time = time;

		Clear({128, 64, 0});
		LookAt(view_pos);
		tile_map.render(IRect(int2(0, 0), res));
		actor.draw();

		/*{
			LookAt({0, 0});
			char text[256];
			fontTex->Bind();

			//double time = GetTime();
			//double frameTime = time - lastFrameTime;
			//lastFrameTime = time;
			
			string profData = Profiler::GetStats();
			Profiler::NextFrame();
		}*/

		SwapBuffers();
	}

	DestroyWindow();

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		DestroyWindow();
		printf("%s\n\nBacktrace:\n%s\n", ex.What(), CppFilterBacktrace(ex.Backtrace()).c_str());
		return 1;
	}
}

