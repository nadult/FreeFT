/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/inventory.h"
#include "hud/widget.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "audio/device.h"
#include <algorithm>

using namespace gfx;

namespace hud {
	
	namespace {
		const float2 s_item_size(70, 65);
		const float2 s_item_desc_size(250, 300);

		const int2 s_grid_size(4, 10);
	}	
		
	HudItemDesc::HudItemDesc(const FRect &rect) :HudWidget(rect) { }
	
	void HudItemDesc::draw() const {
		if(!isVisible())
			return;

		FRect layer_rect = rect();
		layer_rect.min -= float2(HudLayer::spacing, HudLayer::spacing);
		layer_rect.max += float2(HudLayer::spacing, HudLayer::spacing);
		HudLayer layer(layer_rect);
		HudStyle temp_style = m_style;
		temp_style.layer_color = Color(temp_style.layer_color, (int)(alpha() * 255));
		layer.setStyle(temp_style);
		layer.draw();

		HudWidget::draw();
		FRect rect = this->rect();

		if(!m_item.isDummy()) {
			TextFormatter title;
			title("%s", m_item.proto().name.c_str());

			IRect extents = m_big_font->evalExtents(title.text());
			drawTitleText(float2(rect.center().x - extents.width() / 2, 5.0f), title);

			float ypos = 15.0f + extents.height();

			FRect uv_rect;
			gfx::PTexture texture = m_item.guiImage(false, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (int2)(float2(rect.center().x - size.x * 0.5f, ypos));
			texture->bind();
			drawQuad(FRect(pos, pos + size), uv_rect, focusColor());

			ypos += size.y + 10.0f;
			FRect desc_rect(rect.min.x + 5.0f, ypos, rect.max.x - 5.0f, rect.max.y - 5.0f);
		
			string params_desc;
			if(m_item.type() == ItemType::weapon)
				params_desc = Weapon(m_item).paramDesc();
			else if(m_item.type() == ItemType::ammo)
				params_desc = Ammo(m_item).paramDesc();
			else if(m_item.type() == ItemType::armour)
				params_desc = Armour(m_item).paramDesc();

			TextFormatter fmt;
			fmt("%s", params_desc.c_str());
			drawText(float2(rect.min.x + 5.0f, ypos), fmt);
		}

	}

	HudInventoryItem::HudInventoryItem(const FRect &rect) :HudWidget(rect) { }

	void HudInventoryItem::draw() const {
		if(!isVisible())
			return;

		HudWidget::draw();
		FRect rect = this->rect();

		if(!m_item.isDummy()) {
			FRect uv_rect;
			gfx::PTexture texture = m_item.guiImage(true, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (int2)(rect.center() - size / 2);
			texture->bind();
			drawQuad(FRect(pos, pos + size), uv_rect);

			if(m_count > 1) {
				TextFormatter fmt(256);
				fmt("%d", m_count);
				IRect extents = m_font->evalExtents(fmt.text());
				drawText(float2(rect.max.x - extents.width(), rect.min.y), fmt);
			}
		}
	}
		
	Color HudInventoryItem::backgroundColor() const {
		return lerp(HudWidget::backgroundColor(), Color::white, m_focus_time * 0.5f);
	}

	HudInventory::HudInventory(PWorld world, EntityRef actor_ref, const FRect &target_rect)
		:HudLayer(target_rect), m_world(world), m_actor_ref(actor_ref), m_out_of_item_time(1.0f), m_drop_count(0) {
		DASSERT(m_world);

		for(int y = 0; y < s_grid_size.y; y++)
			for(int x = 0; x < s_grid_size.x; x++) {
				float2 pos(s_item_size.x * x + spacing * (x + 1), s_item_size.y * y + spacing * (y + 1));
				PHudInventoryItem item(new HudInventoryItem(FRect(pos, pos + s_item_size)));

				attach(item.get());
				m_buttons.emplace_back(std::move(item));
			}

		m_item_desc = new HudItemDesc(FRect(s_item_desc_size) + float2(rect().width() + HudLayer::spacing * 2.0f, HudLayer::spacing));
		m_item_desc->setVisible(false);

		attach(m_item_desc.get());
	}
		
	HudInventory::~HudInventory() { }

	namespace {

		struct Entry {
			Item item;
			int count;
			bool is_equipped;

			bool operator<(const Entry &rhs) const {
				return item.type() == rhs.item.type()? item.name() < rhs.item.name() : item.type() < rhs.item.type();
			}
		};

		void extractEntries(vector<Entry> &out, const ActorInventory &inventory) {
			ActorInventory temp = inventory;
			temp.unequip(ItemType::armour);
			temp.unequip(ItemType::ammo);
			temp.unequip(ItemType::weapon);
			
			out.reserve(temp.size());
			for(int n = 0; n < temp.size(); n++) {
				bool is_equipped = inventory.armour() == temp[n].item || inventory.weapon() == temp[n].item ||
								  (inventory.ammo().item == temp[n].item && inventory.ammo().count);
				out.emplace_back(Entry{temp[n].item, temp[n].count, is_equipped});
			}

			std::sort(out.begin(), out.end());
		}

	}

	void HudInventory::update(bool is_active, double time_diff) {
		HudLayer::update(is_active, time_diff);
		const Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		float2 mouse_pos = float2(getMousePos()) - rect().min;

		if(actor) {
			vector<Entry> entries;
			extractEntries(entries, actor->inventory());

			int max_count = min((int)entries.size(), (int)m_buttons.size());
			for(int n = 0; n < max_count; n++) {
				m_buttons[n]->setItem(entries[n].item);
				m_buttons[n]->setCount(entries[n].count);
				m_buttons[n]->setFocus(entries[n].is_equipped);
			}

			for(int n = 0; n < (int)m_buttons.size(); n++)
				m_buttons[n]->setVisible(n < max_count, false);
		}
		
		int over_item = -1;
		for(int n = 0; n < (int)m_buttons.size(); n++)
			if(m_buttons[n]->isMouseOver(mouse_pos)) {
				over_item = n;
				break;
			}

		{	// Update item desc
			m_out_of_item_time = over_item == -1? min(m_out_of_item_time + (float)time_diff, 1.0f) : 0.0f;
			if(!rect().isInside(mouse_pos + rect().min))
				m_out_of_item_time = 1.0f;

			if(over_item != -1)
				m_item_desc->setItem(m_buttons[over_item]->item());

			FRect rect = m_item_desc->targetRect();
			rect.max.y = rect.min.y + this->rect().height() - HudLayer::spacing * 2.0f;
			m_item_desc->setTargetRect(rect);
			m_item_desc->setVisible(m_is_visible && (over_item != -1 || m_out_of_item_time < 1.0f));
			m_item_desc->setFocus(true);
		}
		
		if(!is_active || !m_is_visible)
			m_drop_item = Item::dummy();

		if(is_active && over_item != -1) {
			Item item = m_buttons[over_item]->item();
			const ActorInventory &inventory = actor->inventory();
			bool is_equipped = inventory.weapon() == item || inventory.armour() == item ||
								(inventory.ammo().item == item && inventory.ammo().count == inventory.weapon().proto().max_ammo);

			if(m_buttons[over_item]->isPressed(mouse_pos, 0)) {
				if(is_equipped) {
					m_world->sendOrder(new UnequipItemOrder(item.type()), m_actor_ref);
					audio::playSound("butn_pulldown", 1.0f);
				}
				else {
					//TODO: play sounds only for items which doesn't generate any 
					if(actor->canEquipItem(item)) {
						m_world->sendOrder(new EquipItemOrder(item), m_actor_ref);
						if(item.type() != ItemType::ammo)
							audio::playSound("butn_pulldown", 1.0f);
					}
					else {
						audio::playSound("butn_optionknob", 1.0f);
					}
				}

				//TODO: using containers
//				if(isKeyDown(Key_right) && m_inventory_sel >= 0)
//					m_world->sendOrder(new TransferItemOrder(container->ref(), transfer_to, m_inventory_sel, 1), m_actor_ref);
//				if(isKeyDown(Key_left))
//					m_world->sendOrder(new TransferItemOrder(container->ref(), transfer_from, m_container_sel, 1), m_actor_ref);
			}
			if(m_buttons[over_item]->isPressed(mouse_pos, 1)) {
				if(is_equipped) {
					m_world->sendOrder(new UnequipItemOrder(item.type()), m_actor_ref);
					audio::playSound("butn_pulldown", 1.0f);
				}
				else {
					m_drop_start_pos = mouse_pos;
					m_drop_item = item;
					m_drop_count = 1;
				}
			}

		}

		if(is_active && !m_drop_item.isDummy()) {
			int inv_id = actor->inventory().find(m_drop_item);
			if(inv_id != -1 && isMouseKeyPressed(1)) {
				float diff = (mouse_pos.y - m_drop_start_pos.y) / 20.0f;
				m_drop_count += (diff < 0.0f? -1.0f : 1.0f) * (diff * diff) * time_diff;
				m_drop_count = clamp(m_drop_count, 0.0, (double)actor->inventory()[inv_id].count);
			}

			if(!isMouseKeyPressed(1)) {
				if((int)m_drop_count > 0)
					m_world->sendOrder(new DropItemOrder(m_drop_item, (int)m_drop_count), m_actor_ref);
				m_drop_item = Item::dummy();
			}
		}
	}
		
	void HudInventory::draw() const {
		HudLayer::draw();

		if(!m_drop_item.isDummy()) {
			for(int n = 0; n < (int)m_buttons.size(); n++)
				if(m_buttons[n]->item() == m_drop_item) {
					TextFormatter fmt;
					fmt("-%d", (int)m_drop_count);
					FRect rect = m_buttons[n]->rect() + this->rect().min;
					m_buttons[n]->drawText(float2(rect.min.x, rect.max.y - 20.0f), fmt);
					break;
				}
		}
	}

	float HudInventory::preferredHeight() const {
		FRect rect;

		for(int n = 0; n < (int)m_buttons.size(); n++)
			if(m_buttons[n]->isVisible())
				rect = sum(rect, m_buttons[n]->rect());

		return max(rect.height() + spacing * 2.0f, s_item_desc_size.y);
	}

}
