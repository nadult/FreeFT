#include "actor.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"
#include "tile_map.h"
#include "navigation_map.h"

using namespace gfx;

Actor::Actor(const char *spr_name, int3 pos) :Entity(int3(1, 1, 1), pos) {
	m_sprite = Sprite::mgr[spr_name];
//	m_sprite->printInfo();

	m_bbox = m_sprite->m_bbox;
	m_issue_next_order = false;
	m_stance = sStanding;

	setSequence(aStanding);
	lookAt(int3(0, 0, 0));
	m_order = m_next_order = makeDoNothingOrder();
}

void Actor::setNextOrder(Order order) {
	m_next_order = order;
}

void Actor::think(double current_time, double time_delta) {
	if(m_issue_next_order)
		issueNextOrder();

	OrderId order_id = m_order.m_id;
	if(order_id == oDoNothing) {
		fixPos();
		setSequence(aStanding);
		m_issue_next_order = true;
	}
	else if(order_id == oMove) {
		DASSERT(!m_path.empty() && m_path_pos >= 0 && m_path_pos < (int)m_path.size());
		
		setSequence(m_order.m_flags? aRunning : aWalking);
		float speed = m_order.m_flags? 20.0f:
			m_stance == sStanding? 10.0f : m_stance == sCrouching? 6.0f : 3.5f;

		float dist = speed * time_delta;
		m_issue_next_order = false;

		while(dist > 0.0001f) {
			int3 target = m_path[m_path_pos];
			int3 diff = target - m_last_pos;
			float3 diff_vec(diff); diff_vec = diff_vec / length(diff_vec);
			float3 cur_pos = float3(m_last_pos) + float3(diff) * m_path_t;
			float tdist = distance(float3(target), cur_pos);

			if(tdist < dist) {
				dist -= tdist;
				m_last_pos = target;
				m_path_t = 0.0f;

				if(++m_path_pos == (int)m_path.size() || m_next_order.m_id != oDoNothing) {
					lookAt(target);
					setPos(target);
					m_path.clear();
					m_issue_next_order = true;
					break;
				}
			}
			else {
				float new_x = cur_pos.x + diff_vec.x * dist;
				float new_z = cur_pos.z + diff_vec.z * dist;
				m_path_t = diff.x? (new_x - m_last_pos.x) / float(diff.x) : (new_z - m_last_pos.z) / float(diff.z);
				float3 new_pos = (float3)m_last_pos + float3(diff) * m_path_t;
				lookAt(target);
				setPos(new_pos);
				break;
			}
		}
	}
	else if(order_id == oChangeStance)
		setSequence(m_order.m_flags < 0? aStanceDown : aStanceUp);

	animate(current_time);
}

void Actor::addToRender(gfx::SceneRenderer &out) const {
	Sprite::Rect rect;

	//TODO: do not allocate texture every frame
	PTexture spr_tex = new DTexture;
	gfx::Texture tex = m_sprite->getFrame(m_seq_id, m_frame_id, m_dir, &rect);
	spr_tex->setSurface(tex);

	out.add(spr_tex, IRect(rect.left, rect.top, rect.right, rect.bottom) - m_sprite->m_offset, m_pos, m_bbox);
	ASSERT(!m_tile_map->isOverlapping(boundingBox()));
//	out.addBox(boundingBox(), m_tile_map && m_tile_map->isOverlapping(boundingBox())?
//				Color(255, 0, 0) : Color(255, 255, 255));
}

void Actor::issueNextOrder() {
	if(m_order.m_id == oChangeStance) {
		m_stance = (StanceId)(m_stance - m_order.m_flags);
		//TODO: different bboxes for stances
//		m_bbox = m_sprite->m_bbox;
//		if(m_stance == sCrouching && m_bbox.y == 9)
//			m_bbox.y = 5;
//		if(m_stance == sProne && m_bbox.y == 9)
//			m_bbox.y = 2;
		DASSERT(m_stance >= 0 && m_stance < stanceCount);
	}

	if(m_next_order.m_id == oMove)
		issueMoveOrder();
	else {
		if(m_next_order.m_id == oChangeStance) {
			if( m_next_order.m_flags == 0 ||
						(m_next_order.m_flags < 0 && m_stance == sProne) ||
						(m_next_order.m_flags > 0 && m_stance == sStanding) )
				m_next_order = makeDoNothingOrder();
		}

		m_order = m_next_order;
	}

	m_issue_next_order = false;
	m_next_order = makeDoNothingOrder();
}

void Actor::issueMoveOrder() {
	OrderId order_id = m_next_order.m_id;
	int3 new_pos = m_next_order.m_pos;
	DASSERT(order_id == oMove && m_navigation_map);

	new_pos = max(new_pos, int3(0, 0, 0)); //TODO: clamp to map extents

	int3 cur_pos = (int3)m_pos;
	vector<int2> tmp_path = m_navigation_map->findPath(cur_pos.xz(), new_pos.xz());

	if(cur_pos == new_pos || tmp_path.empty()) {
		m_order = makeDoNothingOrder();
		return;
	}

	m_last_pos = cur_pos;

	m_path.clear();
	m_path_t = 0;
	m_path_pos = 0;
	fixPos();

	m_order = m_next_order;

	for(int n = 1; n < (int)tmp_path.size(); n++) {
		cur_pos = asXZY(tmp_path[n - 1], 1);
		new_pos = asXZY(tmp_path[n], 1);
		if(new_pos == cur_pos)
			continue;

		MoveVector mvec(tmp_path[n - 1], tmp_path[n]);

		while(mvec.dx) {
			int step = min(mvec.dx, 3);
			m_path.push_back(cur_pos += int3(mvec.vec.x * step, 0, 0));
			mvec.dx -= step;
		}
		while(mvec.dy) {
			int step = min(mvec.dy, 3);
			m_path.push_back(cur_pos += int3(0, 0, mvec.vec.y * step));
			mvec.dy -= step;
		}
		while(mvec.ddiag) {
			int dstep = min(mvec.ddiag, 3);
			m_path.push_back(cur_pos += asXZ(mvec.vec) * dstep);
			mvec.ddiag -= dstep;
		}
	}
		
	if(m_path.size() <= 1 || m_stance != sStanding)
		m_order.m_flags = 0;

	DASSERT(!m_path.empty());
}

// W konstruktorze wyszukac wszystkie animacje i trzymac id-ki zamiast nazw
static const char *s_seq_names[Actor::actionCount][Actor::stanceCount] = {
	// standing, 		crouching,			prone,
	{ "Stand", 			"Crouch",			"Prone" },			// standing
	{ "StandWalk", 		"CrouchWalk",		"ProneWalk" },		// walking
	{ "StandRun",		nullptr,			nullptr },			// running

	{ nullptr,			"CrouchStand", 		"ProneCrouch" },	// stance up
	{ "StandCrouch",	"CrouchProne", 		nullptr },			// stance down
};

// sets seq_id, frame_id and seq_name
void Actor::setSequence(ActionId action) {
	DASSERT(action < actionCount && m_stance < stanceCount);
	const char *name = s_seq_names[action][m_stance];
	DASSERT(name);

	if(m_seq_name == name)
		return;

	int seq_id = m_sprite->findSequence(name);
	ASSERT(seq_id != -1);

	m_seq_id = seq_id;
	m_frame_id = 0;
	m_seq_name = name;

	m_looped_anim = action != aStanceUp && action != aStanceDown;
}

// sets direction
void Actor::lookAt(int3 pos) { //TODO: rounding
	float dx = (float)pos.x - m_pos.x, dz = (float)pos.z - m_pos.z;
	int dir = Sprite::findDir(dx < 0? -1 : dx > 0? 1 : 0, dz < 0? -1 : dz > 0? 1 : 0);
	//TODO: sprites have varying number of directions
	// maybe a vector (int2) should be passed to sprite, and it should
	// figure out a best direction by itself
	m_dir = dir;
}

void Actor::animate(double current_time) {
	//const Sprite::Sequence &seq	= m_sprite->m_sequences[m_seq_id];
	//const Sprite::Animation &anim = m_sprite->m_anims[seq.m_anim_id];
	
	double diff_time = current_time - m_last_time;
	if(diff_time > 1 / 15.0) {
		int frame_count = m_sprite->frameCount(m_seq_id);
		int next_frame_id = (m_frame_id + 1) % frame_count;
		m_last_time = current_time;

		bool finished = next_frame_id < m_frame_id;
		if(finished)
			onAnimFinished();

		if(!finished || m_looped_anim)
			m_frame_id = next_frame_id;
	}
}

void Actor::onAnimFinished() {
	if(m_order.m_id == oChangeStance)
		m_issue_next_order = true;
}
