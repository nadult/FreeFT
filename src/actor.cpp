#include "actor.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"

using namespace gfx;

Actor::Actor(const char *spr_name, int3 pos) :Entity(int3(1, 1, 1), pos) {
	m_sprite = Sprite::mgr[spr_name];
	m_bbox = m_sprite->m_bbox;
	m_issue_next_order = false;
	m_stance = sStanding;

	setSequence(aStanding);
	lookAt(int3(0, 0, 0));
	m_order = m_next_order = makeDoNothingOrder();

//	m_sprite->printInfo();
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
		DAssert(!m_path.empty() && m_path_pos >= 0 && m_path_pos < (int)m_path.size());
		
		setSequence(m_order.m_flags? aRunning : aWalking);
		float speed = m_order.m_flags? 20.0f:
			m_stance == sStanding? 10.0f : m_stance == sCrouching? 6.0f : 3.5f;

		float dist = speed * time_delta;
		m_issue_next_order = false;

		while(dist > 0.0001f) {
			int3 target = m_path[m_path_pos];
			int3 diff = target - m_last_pos;
			float3 diff_vec(diff); diff_vec = diff_vec / Length(diff_vec);
			float3 cur_pos = float3(m_last_pos) + float3(diff) * m_path_t;
			float tdist = Distance(float3(target), cur_pos);

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

	PTexture spr_tex = new DTexture;
	spr_tex->SetSurface(m_sprite->getFrame(m_seq_id, m_frame_id, m_dir, &rect));

	out.add(spr_tex, IRect(rect.left, rect.top, rect.right, rect.bottom) - m_sprite->m_offset, m_pos, m_bbox);
	out.addBBox(boundingBox());
}

void Actor::issueNextOrder() {
	if(m_order.m_id == oChangeStance) {
		m_stance = (StanceId)(m_stance - m_order.m_flags);
		DAssert(m_stance >= 0 && m_stance < stanceCount);
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
	DAssert(order_id == oMove);

	new_pos = Max(new_pos, int3(0, 0, 0)); //TODO: clamp to map extents

	int3 cur_pos = (int3)m_pos;
	if(cur_pos == new_pos) {
		m_order = makeDoNothingOrder();
		return;
	}

	m_last_pos = cur_pos;

	int x_diff = new_pos.x - cur_pos.x;
	int z_diff = new_pos.z - cur_pos.z;
	int3 dir(x_diff < 0? -1 : 1, 0, z_diff < 0? -1 : 1);
	x_diff = abs(x_diff);
	z_diff = abs(z_diff);
	int diag_diff = Min(x_diff, z_diff);
	x_diff -= diag_diff;
	z_diff -= diag_diff;

	m_path.clear();
	m_path_t = 0;
	m_path_pos = 0;
	fixPos();

	m_order = m_next_order;

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

	if(m_path.size() <= 1 || m_stance != sStanding)
		m_order.m_flags = 0;

	DAssert(!m_path.empty());
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
	DAssert(action < actionCount && m_stance < stanceCount);
	const char *name = s_seq_names[action][m_stance];
	DAssert(name);

	if(m_seq_name == name)
		return;

	int seq_id = m_sprite->findSequence(name);
	Assert(seq_id != -1);

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
