#include "game/actor.h"
#include "game/world.h"
#include "gfx/sprite.h"
#include <cmath>
#include <cstdio>

namespace game {

	Order moveOrder(int3 target_pos, bool run)	{
		Order new_order(OrderId::move);
		new_order.move = Order::Move{target_pos, run};
		return new_order;
	}
	Order doNothingOrder() {
		Order new_order(OrderId::do_nothing);
		return new_order;
	}
	Order changeStanceOrder(int next_stance) {
		Order new_order(OrderId::change_stance);
		new_order.change_stance = Order::ChangeStance{next_stance};
		return new_order;
	}
	Order attackOrder(int attack_mode, const int3 &target_pos) {
		Order new_order(OrderId::attack);
		new_order.attack = Order::Attack{target_pos, attack_mode};
		return new_order;
	}
	Order interactOrder(Entity *target, InteractionMode mode) {
		Order new_order(OrderId::interact);
		DASSERT(target);
		new_order.interact = Order::Interact{target, mode, false};
		return new_order;
	}
	Order dropItemOrder(int item_id) {
		Order new_order(OrderId::drop_item);
		new_order.drop_item = Order::DropItem{item_id};
		return new_order;
	}
	Order equipItemOrder(int item_id) {
		Order new_order(OrderId::equip_item);
		new_order.equip_item = Order::EquipItem{item_id};
		return new_order;
	}
	Order unequipItemOrder(InventorySlotId::Type slot_id) {
		Order new_order(OrderId::unequip_item);
		DASSERT(slot_id >= 0 && slot_id < InventorySlotId::count);
		new_order.unequip_item = Order::UnequipItem{slot_id};
		return new_order;
	}

	static const char *s_sprite_names[ActorTypeId::count][ArmourClassId::count] = {
		{	// Male
			"characters/TribalMale",
			"characters/LeatherMale",
			"characters/MetalMale",
			"characters/Environmental",
			"characters/Power",
		},
		{	// Female
			"characters/TribalFemale",
			"characters/LeatherFemale",
			"characters/MetalFemale",
			"characters/Environmental",
			"characters/Power",
		},
		{	// Ghoul
			"characters/Ghoul",
			"characters/Ghoul",
			"characters/GhoulArmour",
			"characters/Environmental",
			"characters/Power",
		},
	};



	static const char *s_wep_names[WeaponClassId::count] = {
		"",
		"Club",
		"Heavy",
		"Knife",
		"Minigun",
		"Pistol",
		"Rifle",
		"Rocket",
		"SMG",
		"Spear",
	};

	static const char *s_attack_names[WeaponClassId::count * 2] = {
		"UnarmedOne",
		"UnarmedTwo",

		"ClubSwing",
		nullptr,

		"HeavySingle",
		"HeavyBurst",

		"KnifeSlash",
		nullptr,

		"MinigunBurst",
		nullptr,

		"PistolSingle",
		nullptr,

		"RifleSingle", 
		"RifleBurst",

		"RocketSingle",
		nullptr,

		"SMGSingle",
		"SMGBurst",

		"SpearThrow",
		nullptr,
	};

	// W konstruktorze wyszukac wszystkie animacje i trzymac id-ki zamiast nazw
	static const char *s_seq_names[ActionId::count][StanceId::count] = {
		// standing, 		crouching,			prone,
		{ "Stand%s",		"Crouch%s",			"Prone%s" },			// standing
		{ "StandWalk%s",	"CrouchWalk",		"ProneWalk" },			// walking
		{ "StandRun",		nullptr,			nullptr },				// running

		{ nullptr,			"CrouchStand", 		"ProneCrouch" },		// stance up
		{ "StandCrouch",	"CrouchProne", 		nullptr },				// stance down

		{ "StandAttack%s",	"CrouchAttack%s",	"ProneAttack%s" },		// attack 1
		{ "StandAttack%s",	"CrouchAttack%s",	"ProneAttack%s" },		// attack 2

		{ "StandPickup", 	"CrouchPickup", 	"PronePickup" },		// pickup
		{ "StandMagichigh", "CrouchMagic",	 	"ProneMagic" },			// magic 1
		{ "StandMagiclow", 	"CrouchMagic", 		"ProneMagic" },			// magic 2
	};

	ActorAnimMap::ActorAnimMap(gfx::PSprite sprite) {
		DASSERT(sprite);
		m_seq_ids.resize(ActionId::count * WeaponClassId::count * StanceId::count, -1);

		for(int a = 0; a < ActionId::count; a++)
			for(int w = 0; w < WeaponClassId::count; w++)
				for(int s = 0; s < StanceId::count; s++) {
					const string &name = sequenceName((StanceId::Type)s, (ActionId::Type)a, (WeaponClassId::Type)w);
					if(!name.empty()) {
						int seq_idx = s + (a * WeaponClassId::count + w) * StanceId::count;
						m_seq_ids[seq_idx] = sprite->findSequence(name.c_str());
//						if(m_seq_ids[seq_idx] == -1)
//							printf("missing seq: %s\n", name.c_str());
					}

				}
	}

	string ActorAnimMap::sequenceName(StanceId::Type stance, ActionId::Type action, WeaponClassId::Type weapon) const {
		char seq_name[64] = "";

		if(action == ActionId::attack1 || action == ActionId::attack2) {
			int attack_id = weapon * 2 + (action == ActionId::attack1? 0 : 1);
			if(s_attack_names[attack_id])
				snprintf(seq_name, sizeof(seq_name), s_seq_names[action][stance], s_attack_names[attack_id]);
		}
		else if(s_seq_names[action][stance])
			snprintf(seq_name, sizeof(seq_name), s_seq_names[action][stance], s_wep_names[weapon]);

		return seq_name;
	}

	int ActorAnimMap::sequenceId(StanceId::Type stance, ActionId::Type action, WeaponClassId::Type weapon) const {
		if(m_seq_ids.empty())
			return -1;
		return m_seq_ids[stance + (action * WeaponClassId::count + weapon) * StanceId::count];
	}

	Actor::Actor(ActorTypeId::Type type_id, const float3 &pos)
		:Entity(s_sprite_names[type_id][ArmourClassId::none], pos), m_type_id(type_id) {
		//m_sprite->printInfo();
		m_anim_map = ActorAnimMap(m_sprite);

		m_issue_next_order = false;
		m_stance_id = StanceId::standing;

		m_armour_class_id = ArmourClassId::none;
		m_weapon_class_id = WeaponClassId::unarmed;
		setSequence(ActionId::standing);
		lookAt(float3(0, 0, 0), true);
		m_order = m_next_order = doNothingOrder();
	}

	void Actor::updateArmour() {
		const ArmourDesc *desc = m_inventory.armour().desc();
		ArmourClassId::Type class_id = desc? desc->class_id : ArmourClassId::none;
		DASSERT(canEquipArmour(class_id));

		if(class_id != m_armour_class_id) {
			m_armour_class_id = class_id;
			changeSprite(s_sprite_names[m_type_id][class_id], true);
		}
	}

	void Actor::updateWeapon() {
		const WeaponDesc *weapon_desc = m_inventory.weapon().desc();
		WeaponClassId::Type class_id = weapon_desc? weapon_desc->class_id : WeaponClassId::unarmed;
		DASSERT(canEquipWeapon(class_id));

		if(class_id != m_weapon_class_id) {
			m_weapon_class_id = class_id;
			setSequence(m_action_id);
		}
	}

	bool Actor::canEquipItem(int item_id) const {
		DASSERT(item_id >= 0 && item_id < m_inventory.size());
		const Item &item = m_inventory[item_id].item;
		if(item.typeId() == ItemTypeId::weapon)
			return canEquipWeapon(Weapon(item).classId());
		else if(item.typeId() == ItemTypeId::armour)
			return canEquipArmour(Armour(item).classId());

		return true;
	}

	bool Actor::canEquipWeapon(WeaponClassId::Type class_id) const {
		for(int s = 0; s < StanceId::count; s++) {
			if(m_anim_map.sequenceId((StanceId::Type)s, ActionId::standing, class_id) == -1)
				return false;
			if(m_anim_map.sequenceId((StanceId::Type)s, ActionId::walking, class_id) == -1)
				return false;
		}

		return true;
	}

	bool Actor::canEquipArmour(ArmourClassId::Type class_id) const {
		return true;
	}

	void Actor::setNextOrder(const Order &order) {
		m_next_order = order;
	}

	void Actor::think() {
		DASSERT(m_world);
		double time_delta = m_world->timeDelta();

		if(m_issue_next_order)
			issueNextOrder();

		OrderId::Type order_id = m_order.id;
		if(order_id == OrderId::do_nothing) {
			roundPos();
			m_issue_next_order = true;
		}
		else if(order_id == OrderId::move) {
			DASSERT(!m_path.empty() && m_path_pos >= 0 && m_path_pos < (int)m_path.size());
			
			float speed = m_order.move.run? 20.0f:
				m_stance_id == StanceId::standing? 10.0f : m_stance_id == StanceId::crouching? 6.0f : 3.5f;

			float dist = speed * time_delta;
			m_issue_next_order = false;
			float3 new_pos;

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

					bool stop_moving = m_next_order.id != OrderId::do_nothing &&
							!(m_next_order.id == OrderId::interact && m_next_order.interact.waiting_for_move);

					if(++m_path_pos == (int)m_path.size() || stop_moving) {
						lookAt(target);
						new_pos = target;
						m_path.clear();
						m_issue_next_order = true;
						break;
					}
				}
				else {
					float new_x = cur_pos.x + diff_vec.x * dist;
					float new_z = cur_pos.z + diff_vec.z * dist;
					m_path_t = diff.x? (new_x - m_last_pos.x) / float(diff.x) : (new_z - m_last_pos.z) / float(diff.z);
					new_pos = (float3)m_last_pos + float3(diff) * m_path_t;
					lookAt(target);
					break;
				}
			}

			if(m_world->isColliding(boundingBox() + new_pos - pos(), this, collider_dynamic | collider_dynamic_nv)) {
				//TODO: response to collision
				m_issue_next_order = true;
				m_path.clear();
			}
			else
				setPos(new_pos);
		}
	}

	void Actor::issueNextOrder() {
		if(m_order.id == OrderId::do_nothing && m_next_order.id == OrderId::do_nothing)
			return;
		
		if(m_order.id == OrderId::change_stance) {
			m_stance_id = (StanceId::Type)(m_stance_id - m_order.change_stance.next_stance);
			//TODO: different bboxes for stances
			//TODO: support for non-quadratic actor bboxes
	//		m_bbox = m_sprite->m_bbox;
	//		if(m_stance_id == StanceId::crouching && m_bbox.y == 9)
	//			m_bbox.y = 5;
	//		if(m_stance_id == StanceId::prone && m_bbox.y == 9)
	//			m_bbox.y = 2;
			DASSERT(m_stance_id >= 0 && m_stance_id < StanceId::count);
		}
		
		if(m_next_order.id == OrderId::move) {
			issueMoveOrder();
			m_next_order = doNothingOrder();
		}
		else if(m_next_order.id == OrderId::interact) {
			IBox my_box(boundingBox());
			IBox other_box = enclosingIBox(m_next_order.interact.target->boundingBox());

			bool are_adjacent = distanceSq(	FRect(my_box.min.xz(), my_box.max.xz()),
											FRect(other_box.min.xz(), other_box.max.xz())) <= 0.0f;
			if(are_adjacent) {
				m_order = m_next_order;
				m_next_order = doNothingOrder();
				ActionId::Type action = m_order.interact.mode == interact_pickup? ActionId::pickup :
					other_box.max.y < my_box.max.y * 2 / 3? ActionId::magic2 : ActionId::magic1;
				setSequence(action);
				lookAt(other_box.center());
			}
			else {
				if(m_next_order.interact.waiting_for_move) {
					m_order = m_next_order = doNothingOrder();
				}
				else {
					int2 target_pos = my_box.min.xz();
					if(my_box.max.x < other_box.min.x)
						target_pos.x = other_box.min.x - my_box.width();
					else if(my_box.min.x > other_box.max.x)
						target_pos.x = other_box.max.x;
					if(my_box.max.z < other_box.min.z)
						target_pos.y = other_box.min.z - my_box.depth();
					else if(my_box.min.z > other_box.max.z)
						target_pos.y = other_box.max.z;
					
					target_pos = m_world->naviMap().findClosestCorrectPos(target_pos,
							IRect(other_box.min.xz(), other_box.max.xz()));
					Order order = m_next_order;
					order.interact.waiting_for_move = true;
					m_next_order = moveOrder(asXZY(target_pos, 1), true);
					issueMoveOrder();
					if(m_order.id == OrderId::move)
						m_next_order = order;
				}
			}
		}
		else {
			if(m_next_order.id == OrderId::change_stance) {
				int next_stance = m_next_order.change_stance.next_stance;

				if(next_stance > 0 && m_stance_id != StanceId::standing)
					setSequence(ActionId::stance_up);
				else if(next_stance < 0 && m_stance_id != StanceId::prone)
					setSequence(ActionId::stance_down);
				else
					m_next_order = doNothingOrder();
			}
			else if(m_next_order.id == OrderId::attack) {
				roundPos();
				lookAt(m_next_order.attack.target_pos);
				setSequence(ActionId::attack1);
			}
			else if(m_next_order.id == OrderId::drop_item) {
				int item_id = m_next_order.drop_item.item_id;
				if(item_id >= 0 && item_id < m_inventory.size())
					setSequence(ActionId::pickup);
				else
					m_next_order = doNothingOrder();
			}
			else if(m_next_order.id == OrderId::equip_item || m_next_order.id == OrderId::unequip_item) {
				InventorySlotId::Type changed_slot = InventorySlotId::invalid;

				if(m_next_order.id == OrderId::equip_item) {
					int item_id = m_next_order.equip_item.item_id;
					if(item_id >= 0 && item_id < m_inventory.size() && canEquipItem(item_id))
						changed_slot = m_inventory.equip(item_id);
				}
				else {
					InventorySlotId::Type slot_id = m_next_order.unequip_item.slot_id;
					if(m_inventory.unequip(slot_id) != -1)
						changed_slot = slot_id;
				}

				m_next_order = doNothingOrder();

				if(changed_slot == InventorySlotId::armour)
					updateArmour();
				else if(changed_slot == InventorySlotId::weapon)
					updateWeapon();
			}


			m_order = m_next_order;
			m_next_order = doNothingOrder();
		}
		
		if(m_order.id == OrderId::do_nothing)	
			setSequence(ActionId::standing);

		m_issue_next_order = false;
	}

	void Actor::issueMoveOrder() {
		OrderId::Type order_id = m_next_order.id;
		int3 new_pos = m_next_order.move.target_pos;
		DASSERT(order_id == OrderId::move);

		new_pos = max(new_pos, int3(0, 0, 0)); //TODO: clamp to map extents

		int3 cur_pos = (int3)pos();
		vector<int2> tmp_path = m_world->findPath(cur_pos.xz(), new_pos.xz());

		if(cur_pos == new_pos || tmp_path.empty()) {
			m_order = doNothingOrder();
			return;
		}

		m_last_pos = cur_pos;

		m_path.clear();
		m_path_t = 0;
		m_path_pos = 0;
		roundPos();

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
			
		if(m_path.size() <= 1 || m_stance_id != StanceId::standing)
			m_order.move.run = 0;
		setSequence(m_order.move.run? ActionId::running : ActionId::walking);

		DASSERT(!m_path.empty());
	}

	// sets seq_id, frame_id and seq_name
	void Actor::setSequence(ActionId::Type action_id) {
		DASSERT(action_id < ActionId::count && m_stance_id < StanceId::count);
		
		int seq_id = m_anim_map.sequenceId(m_stance_id, action_id, m_weapon_class_id);
		if(seq_id == -1) {
			printf("Sequence: %s not found!\n",
					m_anim_map.sequenceName(m_stance_id, action_id, m_weapon_class_id).c_str());
			ASSERT(seq_id != -1);
		}

		playSequence(seq_id);
		m_action_id = action_id;
	}

	// sets direction
	void Actor::lookAt(const float3 &pos, bool at_once) { //TODO: rounding
		float3 cur_pos = this->pos();
		float2 dir(pos.x - cur_pos.x, pos.z - cur_pos.z);
		dir = dir / length(dir);
		m_target_angle = vectorToAngle(dir);
		if(at_once)
			setDirAngle(m_target_angle);
	}

	static float angleDist(float a, float b) {
		float diff = fabs(a - b);
		return min(diff, constant::pi * 2.0f - diff);
	}

	void Actor::nextFrame() {
		Entity::nextFrame();

		float dir_angle = dirAngle();
		if(dir_angle != m_target_angle) {
			float step = constant::pi / 4.0f;
			float new_ang1 = dir_angle + step, new_ang2 = dir_angle - step;
			if(new_ang1 < 0.0f)
				new_ang1 += constant::pi * 2.0f;
			if(new_ang2 < 0.0f)
				new_ang2 += constant::pi * 2.0f;
			float new_angle = angleDist(new_ang1, m_target_angle) < angleDist(new_ang2, m_target_angle)?
					new_ang1 : new_ang2;
			if(angleDist(dir_angle, m_target_angle) < step)
				new_angle = m_target_angle;

			setDirAngle(new_angle);
		}
	}

	void Actor::onAnimFinished() {
		if(m_order.id == OrderId::change_stance || m_order.id == OrderId::attack || m_order.id == OrderId::drop_item)
			m_issue_next_order = true;
		else if(m_order.id == OrderId::interact) {
			if(m_order.interact.mode == interact_normal) {
				m_order.interact.target->interact(this);
			}
			m_issue_next_order = true;
		}
	}

	void Actor::onPickupEvent() {
		//TODO: magic_hi animation when object to be picked up is high enough
		if(m_order.id == OrderId::interact) {
			DASSERT(m_order.interact.target->entityType() == entity_item);
			ItemEntity *item_entity = static_cast<ItemEntity*>(m_order.interact.target);
			Item item = item_entity->item();
			item_entity->remove();
			m_inventory.add(item, 1);
		}
		else if(m_order.id == OrderId::drop_item) {
			int item_id = m_order.drop_item.item_id;
			DASSERT(item_id >= 0 && item_id < m_inventory.size());
			Item item = m_inventory[item_id].item;
			m_inventory.remove(item_id, 1);
			m_world->addEntity(new ItemEntity(item, pos())); 
		}
	}
		
	void Actor::onFireEvent(const int3 &off) {
		const Weapon &weapon = m_inventory.weapon();
		if(!weapon.isValid() || m_order.id != OrderId::attack)
			return;

		//	printf("off: %d %d %d   ang: %.2f\n", off.x, off.y, off.z, dirAngle());
		float3 pos = boundingBox().center();
		pos.y = this->pos().y;
		float3 offset = asXZY(rotateVector(float2(off.x, off.z), dirAngle() - constant::pi * 0.5f), off.y);
		PProjectile projectile(new Projectile(weapon.projectileTypeId(), weapon.projectileSpeed(),
												pos + offset, m_order.attack.target_pos, this));
		m_world->spawnProjectile(std::move(projectile));
	}

	void Actor::onSoundEvent() {
	//	if(m_weapon_class_id == WeaponClassId::rifle && m_order.id == OrderId::attack)
	//		printf("Playing sound: plasma!\n");
	}

}
