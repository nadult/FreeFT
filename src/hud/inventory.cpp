/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/inventory.h"
#include "hud/widget.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include <algorithm>

using namespace gfx;

namespace hud {
	
	namespace {
		const float2 s_item_size(70, 65);
		const float2 s_item_desc_size(250, 300);
		const float2 s_button_size(15, 15);
		const float s_bottom_size = 30.0f;

		const int2 s_grid_size(4, 10);
	}
		
	HudItemDesc::HudItemDesc(const FRect &rect) :HudWidget(rect) { }
	
	void HudItemDesc::draw() const {
		if(!isVisible())
			return;

		HudWidget::draw();
		FRect rect = this->rect();

		if(!m_item.isDummy()) {
			FRect extents = m_big_font->draw(FRect(rect.min.x, 5.0f, rect.max.x, 5.0f), {focusColor(), focusShadowColor(), HAlign::center},
											 m_item.proto().name);
			float ypos = extents.max.y + spacing;

			FRect uv_rect;
			gfx::PTexture texture = m_item.guiImage(false, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (int2)(float2(rect.center().x - size.x * 0.5f, ypos));
			texture->bind();
			drawQuad(FRect(pos, pos + size), uv_rect, focusColor());

			ypos += size.y + 10.0f;
			FRect desc_rect(rect.min.x + 5.0f, ypos, rect.max.x - 5.0f, rect.max.y - 5.0f);
			// TODO: fix drawing of text that does not fit
		
			string params_desc;
			if(m_item.type() == ItemType::weapon)
				params_desc = Weapon(m_item).paramDesc();
			else if(m_item.type() == ItemType::ammo)
				params_desc = Ammo(m_item).paramDesc();
			else if(m_item.type() == ItemType::armour)
				params_desc = Armour(m_item).paramDesc();

			m_font->draw(float2(rect.min.x + 5.0f, ypos), {focusColor(), focusShadowColor()}, params_desc);
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

			if(m_count > 1)
				m_font->draw(rect, {focusColor(), focusShadowColor(), HAlign::right}, format("%d", m_count));
		}
	}
		
	Color HudInventoryItem::backgroundColor() const {
		return lerp(HudWidget::backgroundColor(), Color::white, m_focus_time * 0.5f);
	}

	HudInventory::HudInventory(PWorld world, const FRect &target_rect)
		:HudLayer(target_rect), m_world(world), m_out_of_item_time(1.0f), m_drop_count(0), m_row_offset(0), m_min_items(0) {
		DASSERT(m_world);

		for(int y = 0; y < s_grid_size.y; y++)
			for(int x = 0; x < s_grid_size.x; x++) {
				float2 pos(s_item_size.x * x + spacing * (x + 1), s_item_size.y * y + spacing * (y + 1));
				PHudInventoryItem item(new HudInventoryItem(FRect(pos, pos + s_item_size)));

				attach(item.get());
				m_buttons.emplace_back(std::move(item));
			}

		m_button_up = new HudWidget(FRect(s_button_size));
		m_button_up->setIcon(HudIcon::up_arrow);
		m_button_up->setAccelerator(Key_pageup);

		m_button_down = new HudWidget(FRect(s_button_size));
		m_button_down->setIcon(HudIcon::down_arrow);
		m_button_down->setAccelerator(Key_pagedown);

		attach(m_button_up.get());
		attach(m_button_down.get());

		m_item_desc = new HudItemDesc(FRect(s_item_desc_size) + float2(HudLayer::spacing, HudLayer::spacing));
		m_item_desc->setVisible(false);
	}
		
	HudInventory::~HudInventory() { }
		
	void HudInventory::setActor(game::EntityRef actor_ref) {
		if(actor_ref == m_actor_ref)
			return;
		m_actor_ref = actor_ref;
		m_drop_item = Item::dummy();
	}

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
		const Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		is_active &= actor != nullptr;

		HudLayer::update(is_active, time_diff);
		float2 mouse_pos = float2(getMousePos()) - rect().min;
			
		float bottom_line = rect().height() - s_bottom_size;

		if(!is_active)
			m_drop_item = Item::dummy();

		int max_row_offset = 0;
		{
			vector<Entry> entries;
			if(actor)
				extractEntries(entries, actor->inventory());
			m_min_items = (int)entries.size();

			int max_count = (int)m_buttons.size();
			for(int n = 0; n < (int)m_buttons.size(); n++)
				if(m_buttons[n]->rect().max.y > bottom_line) {
					max_count = n;
					break;
				}

			max_count = min(max_count, (int)entries.size());
			max_row_offset = ((int)entries.size() - max_count + s_grid_size.x - 1) / s_grid_size.x;
			
			if(m_row_offset > max_row_offset)
				m_row_offset = max_row_offset;
			int item_offset = m_row_offset * s_grid_size.x;

			max_count = min(max_count, (int)entries.size() - item_offset);
			for(int n = 0; n < max_count; n++) {
				m_buttons[n]->setItem(entries[n + item_offset].item);
				m_buttons[n]->setCount(entries[n + item_offset].count);
				m_buttons[n]->setFocus(entries[n + item_offset].is_equipped);
			}

			for(int n = 0; n < (int)m_buttons.size(); n++)
				m_buttons[n]->setVisible(n < max_count, false);
		}

		{
			HudStyle button_style = m_style;
			button_style.border_offset *= 0.5f;
			m_button_up->setStyle(button_style);
			m_button_down->setStyle(button_style);
			m_button_up  ->setPos(float2(rect().width() - HudWidget::spacing * 2 - s_button_size.x * 2, bottom_line + 5.0f));
			m_button_down->setPos(float2(rect().width() - HudWidget::spacing * 1 - s_button_size.x * 1, bottom_line + 5.0f));
			m_button_up->setVisible(m_row_offset > 0);
			m_button_down->setVisible(m_row_offset < max_row_offset);

			m_button_up->setFocus(m_button_up->rect().isInside(mouse_pos) && isMouseKeyPressed(0));
			m_button_down->setFocus(m_button_down->rect().isInside(mouse_pos) && isMouseKeyPressed(0));

			if(m_button_up->isPressed(mouse_pos) && m_row_offset > 0) {
				playSound(HudSound::button);
				m_row_offset--;
			}
			if(m_button_down->isPressed(mouse_pos) && m_row_offset < max_row_offset) {
				playSound(HudSound::button);
				m_row_offset++;
			}
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
					playSound(HudSound::item_equip);
				}
				else {
					//TODO: play sounds only for items which doesn't generate any 
					if(actor->canEquipItem(item)) {
						m_world->sendOrder(new EquipItemOrder(item), m_actor_ref);
						if(item.type() != ItemType::ammo)
							playSound(HudSound::item_equip);
					}
					else {
						playSound(HudSound::error);
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
					playSound(HudSound::item_equip);
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

		m_item_desc->update(mouse_pos, time_diff);
	}
		
	void HudInventory::draw() const {
		HudLayer::draw();

		if(!m_drop_item.isDummy()) {
			for(int n = 0; n < (int)m_buttons.size(); n++) {
				const HudInventoryItem *item = m_buttons[n].get();
				if(item->item() == m_drop_item) {
					//TODO: move to HudInventoryItem impl
					m_font->draw(item->rect() + rect().min, {item->focusColor(), item->focusShadowColor(), HAlign::left, VAlign::bottom},
							   format("-%d", (int)m_drop_count));
					break;
				}
			}
		}
		
		if(isVisible()) {
			FRect layer_rect = m_item_desc->rect();
			layer_rect += float2(rect().max.x + HudLayer::spacing, rect().min.y);
			layer_rect.min -= float2(HudLayer::spacing, HudLayer::spacing);
			layer_rect.max += float2(HudLayer::spacing, HudLayer::spacing);
		
			HudLayer layer(layer_rect);
			HudStyle temp_style = m_style;
			temp_style.layer_color = Color(temp_style.layer_color, (int)(m_item_desc->alpha() * 255));
			layer.setStyle(temp_style);
			layer.attach(m_item_desc.get());
			layer.draw();
		}
	}

	float HudInventory::preferredHeight() const {
		FRect rect;

		for(int n = 0; n < min(m_min_items, (int)m_buttons.size()); n++)
			rect = sum(rect, m_buttons[n]->rect());

		return max(rect.height() + spacing * 2.0f, s_item_desc_size.y + s_bottom_size);
	}

}
