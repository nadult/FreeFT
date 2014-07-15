/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/inventory.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/game_mode.h"
#include "game/pc_controller.h"
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
		const float s_desc_visible_time = 1.0f;

		const int2 s_grid_size(4, 4);
	}
		
	bool HudItemEntry::operator<(const HudItemEntry &rhs) const {
		return item.type() == rhs.item.type()? item.name() < rhs.item.name() : item.type() < rhs.item.type();
	}
	
	bool HudItemEntry::operator==(const HudItemEntry &rhs) const {
		return item == rhs.item && count == rhs.count && is_equipped == rhs.is_equipped;
	}

	HudItemDesc::HudItemDesc(const FRect &rect)
		:HudButton(rect) { }
	
	void HudItemDesc::onDraw() const {
		HudButton::onDraw();
		FRect rect = this->rect();

		if(!m_item.isDummy()) {
			FRect extents = m_big_font->draw(FRect(rect.min.x, 5.0f, rect.max.x, 5.0f), {textColor(), textShadowColor(), HAlign::center},
											 m_item.proto().name);
			float ypos = extents.max.y + spacing;

			FRect uv_rect;
			gfx::PTexture texture = m_item.guiImage(false, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (int2)(float2(rect.center().x - size.x * 0.5f, ypos));
			texture->bind();
			drawQuad(FRect(pos, pos + size), uv_rect, textColor());

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

			m_font->draw(float2(rect.min.x + 5.0f, ypos), {textColor(), textShadowColor()}, params_desc);
		}
	}

	HudItemButton::HudItemButton(const FRect &rect) :HudButton(rect), m_drop_count(-1) {
		setClickSound(HudSound::none);
	}
		
	void HudItemButton::setEntry(const HudItemEntry &entry) {
		if(m_entry == entry)
			return;
		m_entry = entry;
		m_drop_count = -1.0;
		setEnabled(entry.is_equipped, false);
	}

	void HudItemButton::onDraw() const {
		HudButton::onDraw();
		FRect rect = this->rect();

		if(!m_entry.item.isDummy()) {
			FRect uv_rect;
			gfx::PTexture texture = m_entry.item.guiImage(true, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (int2)(rect.center() - size / 2);
			texture->bind();
			drawQuad(FRect(pos, pos + size), uv_rect);

			if(m_entry.count > 1)
				m_font->draw(rect, {textColor(), textShadowColor(), HAlign::right}, format("%d", m_entry.count));
			if(isDropping())
				m_font->draw(rect, {textColor(), textShadowColor(), HAlign::left, VAlign::bottom}, format("-%d", dropCount()));
		}
	}
		
	Color HudItemButton::backgroundColor() const {
		return lerp(HudButton::backgroundColor(), Color::white, m_enabled_time * 0.5f);
	}
		
	bool HudItemButton::onInput(const io::InputEvent &event) {
		bool mouse_over = isMouseOver(event);

		if(event.mouseOver()) {
			setHighlighted(mouse_over);
			if(mouse_over)
				handleEvent(this, HudEvent::item_focused);


		}
		if(event.mouseKeyDown(0) && mouse_over)
			handleEvent(this, isEnabled()? HudEvent::item_unequip : HudEvent::item_equip);

		if(event.mouseKeyDown(1) && mouse_over) {
			if(isEnabled()) {
				handleEvent(this, HudEvent::item_unequip);
			}
			else {
				m_drop_count = 1.0;
				m_drop_start_pos = event.mousePos();
			}
		}

		if(event.mouseKeyPressed(1) && isDropping())
			m_drop_diff = (event.mousePos().y - m_drop_start_pos.y) / 20.0f;
		if(event.mouseKeyUp(1) && isDropping()) {
			if(dropCount() > 0)
				handleEvent(this, HudEvent::item_drop, dropCount());
			m_drop_count = -1.0f;
		}

		return false;
	}
		
	void HudItemButton::onUpdate(double time_diff) {
		HudButton::onUpdate(time_diff);
		if(isDropping()) {
			m_drop_count += (m_drop_diff < 0.0f? -1.0f : 1.0f) * (m_drop_diff * m_drop_diff) * time_diff;
			m_drop_count = clamp(m_drop_count, 0.0, (double)m_entry.count);
		}
	}

	HudInventory::HudInventory(const FRect &target_rect)
		:HudLayer(target_rect), m_out_of_item_time(s_desc_visible_time), m_row_offset(0) {

		for(int y = 0; y < s_grid_size.y; y++)
			for(int x = 0; x < s_grid_size.x; x++) {
				float2 pos(s_item_size.x * x + item_spacing * (x + 1), s_item_size.y * y + item_spacing * (y + 1));
				PHudItemButton item(new HudItemButton(FRect(pos, pos + s_item_size)));

				attach(item.get());
				m_buttons.emplace_back(std::move(item));
			}

		m_button_up = new HudClickButton(FRect(s_button_size), -1);
		m_button_up->setIcon(HudIcon::up_arrow);
		m_button_up->setAccelerator(Key::pageup);
		m_button_up->setButtonStyle(HudButtonStyle::small);

		m_button_down = new HudClickButton(FRect(s_button_size), 1);
		m_button_down->setIcon(HudIcon::down_arrow);
		m_button_down->setAccelerator(Key::pagedown);
		m_button_down->setButtonStyle(HudButtonStyle::small);

		attach(m_button_up.get());
		attach(m_button_down.get());

		m_item_desc = new HudItemDesc(FRect(s_item_desc_size) + float2(layer_spacing, layer_spacing));
		m_item_desc->setVisible(false, false);
	}
		
	HudInventory::~HudInventory() { }
		
	bool HudInventory::canShow() const {
		return m_pc_controller && m_pc_controller->actor();
	}

	bool HudInventory::onInput(const io::InputEvent &event) {
		if(event.mouseOver() && !isMouseOver(event))
			m_out_of_item_time = s_desc_visible_time;
		return false;
	}

	bool HudInventory::onEvent(const HudEvent &event) {
		ASSERT(m_pc_controller);

		if(!canShow())
			return false;

		if(event.type == HudEvent::button_clicked) {
			if(m_button_up == event.source && m_row_offset > 0)
				m_row_offset--;
			if(m_button_down == event.source && m_row_offset < m_max_row_offset)
				m_row_offset++;
				
			needsLayout();
			return true;
		}
		
		HudItemButton *item_button = dynamic_cast<HudItemButton*>(event.source);
		const Item &item = item_button? item_button->entry().item : Item();

		if(item_button && !item.isDummy()) {
			if(event.type == HudEvent::item_focused) {
				m_item_desc->setItem(item);
				m_out_of_item_time = 0.0f;
			}
			if(event.type == HudEvent::item_unequip) {
				playSound(HudSound::item_equip);
				m_pc_controller->unequipItem(item);
			}
			if(event.type == HudEvent::item_equip) {
				if(m_pc_controller->canEquipItem(item)) {
					//TODO: play sounds only for items which doesn't generate any 
					if(item.type() != ItemType::ammo)
						playSound(HudSound::item_equip);
					m_pc_controller->equipItem(item);
				}
				else {
					playSound(HudSound::error);
				}
			}
			if(event.type == HudEvent::item_drop) {
				m_pc_controller->dropItem(item, event.value);
			}
				
			//TODO: using containers
		}

		return false;
	}
		
	void HudInventory::updateData() {
		vector<HudItemEntry> new_entries;
	
		if(!canShow())
			return;

		const Actor *actor = m_pc_controller->actor();
		const ActorInventory inventory = actor? actor->inventory() : ActorInventory();

		game::ActorInventory temp = inventory;
		temp.unequip(ItemType::armour);
		temp.unequip(ItemType::ammo);
		temp.unequip(ItemType::weapon);
		
		new_entries.reserve(temp.size());
		for(int n = 0; n < temp.size(); n++) {
			bool is_equipped = inventory.armour() == temp[n].item || inventory.weapon() == temp[n].item ||
							  (inventory.ammo().item == temp[n].item && inventory.ammo().count);
			new_entries.emplace_back(HudItemEntry{temp[n].item, temp[n].count, is_equipped});
		}

		std::sort(new_entries.begin(), new_entries.end());

		if(new_entries != m_entries) {
			m_entries = new_entries;
			needsLayout();
		}
	}

	void HudInventory::onLayout() {
		int max_count = min(m_buttons.size(), m_entries.size());
		m_max_row_offset = ((int)m_entries.size() - max_count + s_grid_size.x - 1) / s_grid_size.x;
		
		if(m_row_offset > m_max_row_offset)
			m_row_offset = m_max_row_offset;
		int item_offset = m_row_offset * s_grid_size.x;

		float bottom = 0.0f;
		max_count = min(max_count, (int)m_entries.size() - item_offset);
		for(int n = 0; n < max_count; n++) {
			m_buttons[n]->setEntry(m_entries[n + item_offset]);
			bottom = max(bottom, m_buttons[n]->rect().max.y + spacing);
		}

		for(int n = 0; n < (int)m_buttons.size(); n++)
			m_buttons[n]->setVisible(n < max_count, false);

		m_button_up  ->setPos(float2(rect().width() - spacing * 2 - s_button_size.x * 2, bottom + 5.0f));
		m_button_down->setPos(float2(rect().width() - spacing * 1 - s_button_size.x * 1, bottom + 5.0f));

		m_button_up->setGreyed(m_row_offset == 0);
		m_button_down->setGreyed(m_row_offset >= m_max_row_offset);
	}

	void HudInventory::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);
		float2 mouse_pos = float2(getMousePos()) - rect().min;
			
		updateData();

		FRect desc_rect = m_item_desc->targetRect();
		desc_rect.setHeight(rect().height() - layer_spacing * 2.0f);
		m_item_desc->setRect(desc_rect);

		m_out_of_item_time += time_diff;
		m_item_desc->setVisible(isVisible() && !isHiding() && m_out_of_item_time < s_desc_visible_time);
		m_item_desc->setEnabled(true, false);
		m_item_desc->update(time_diff);
		m_item_desc->layout();
	}
		
	void HudInventory::onDraw() const {
		HudLayer::onDraw();

		FRect layer_rect = m_item_desc->rect();
		layer_rect += float2(rect().max.x + layer_spacing, rect().min.y);
		layer_rect.inset(float2(layer_spacing, layer_spacing), float2(layer_spacing, layer_spacing));
		
		HudLayer layer(layer_rect);
		HudStyle temp_style = m_style;
		temp_style.layer_color = Color(temp_style.layer_color, (int)(m_item_desc->alpha() * 255));
		layer.setStyle(temp_style);
		layer.attach(m_item_desc.get());
		layer.layout();
		layer.draw();
	}

}
