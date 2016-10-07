/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/inventory.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/game_mode.h"
#include "game/pc_controller.h"
#include "gfx/drawing.h"
#include <algorithm>

namespace hud {
	
	namespace {
		const float2 s_item_size(70, 65);
		const float2 s_item_desc_size(280, 300);
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
		:HudLayer(rect) {
		m_anim_speed = 10.0f;
	}
		
	const FRect HudItemDesc::rect() const {
		return targetRect();
	}
		
	float HudItemDesc::alpha() const {
		return m_visible_time * HudLayer::alpha();
	}
		
	void HudItemDesc::setItem(const Item &item) {
		m_item = item;
		setTitle(m_item.proto().name);
	}
	
	void HudItemDesc::onDraw(Renderer2D &out) const {
		HudLayer::onDraw(out);
		FRect rect = this->rect();

		if(!m_item.isDummy()) {
			float ypos = topOffset() + rect.min.y;

			FRect uv_rect;
			auto texture = m_item.guiImage(false, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (float2)(int2)(float2(rect.center().x - size.x * 0.5f, ypos));
			out.addFilledRect(FRect(pos, pos + size), uv_rect, {texture, mulAlpha(Color::white, alpha())});

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

			m_font->draw(out, float2(rect.min.x + 5.0f, ypos), {titleColor(), titleShadowColor()}, params_desc);
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

	void HudItemButton::onDraw(Renderer2D &out) const {
		HudButton::onDraw(out);
		FRect rect = this->rect();

		if(!m_entry.item.isDummy()) {
			FRect uv_rect;
			auto texture = m_entry.item.guiImage(true, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (float2)(int2)(rect.center() - size / 2);
			out.addFilledRect(FRect(pos, pos + size), texture);

			if(m_entry.count > 1)
				m_font->draw(out, rect, {textColor(), textShadowColor(), HAlign::right}, format("%d", m_entry.count));
			if(isDropping())
				m_font->draw(out, rect, {textColor(true), textShadowColor(), HAlign::left, VAlign::bottom}, format("-%d", dropCount()));
		}
	}
		
	Color HudItemButton::backgroundColor() const {
		return lerp(HudButton::backgroundColor(), Color::white, m_enabled_time * 0.5f);
	}
		
	bool HudItemButton::onInput(const InputEvent &event) {
		bool is_mouse_over = isMouseOver(event);
		if(event.isMouseOverEvent()) {
			setHighlighted(is_mouse_over);
			if(is_mouse_over && isVisible())
				handleEvent(this, HudEvent::item_focused);


		}
		if(event.mouseButtonDown(InputButton::left) && is_mouse_over)
			handleEvent(this, isEnabled()? HudEvent::item_unequip : HudEvent::item_equip);

		if(event.mouseButtonDown(InputButton::right) && is_mouse_over) {
			if(isEnabled()) {
				handleEvent(this, HudEvent::item_unequip);
			}
			else {
				m_drop_count = 1.0;
				m_drop_start_pos = (float2)event.mousePos();
			}
		}

		if(event.mouseButtonPressed(InputButton::right) && isDropping())
			m_drop_diff = (event.mousePos().y - m_drop_start_pos.y) / 20.0f;
		if(event.mouseButtonUp(InputButton::right) && isDropping()) {
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
		setTitle("Inventory:");

		for(int y = 0; y < s_grid_size.y; y++)
			for(int x = 0; x < s_grid_size.x; x++) {
				float2 pos(s_item_size.x * x + item_spacing * (x + 1), s_item_size.y * y + item_spacing * (y + 1) + topOffset());
				PHudItemButton item(new HudItemButton(FRect(pos, pos + s_item_size)));

				attach(item);
				m_buttons.emplace_back(std::move(item));
			}

		m_button_up = make_shared<HudClickButton>(FRect(s_button_size), -1);
		m_button_up->setIcon(HudIcon::up_arrow);
		m_button_up->setAccelerator(InputKey::pageup);
		m_button_up->setButtonStyle(HudButtonStyle::small);

		m_button_down = make_shared<HudClickButton>(FRect(s_button_size), 1);
		m_button_down->setIcon(HudIcon::down_arrow);
		m_button_down->setAccelerator(InputKey::pagedown);
		m_button_down->setButtonStyle(HudButtonStyle::small);

		attach(m_button_up);
		attach(m_button_down);

		m_item_desc = make_shared<HudItemDesc>(FRect(s_item_desc_size) + float2(layer_spacing, layer_spacing));
		m_item_desc->setVisible(false, false);
	}
		
	HudInventory::~HudInventory() { }
		
	bool HudInventory::canShow() const {
		return m_pc_controller && m_pc_controller->actor();
	}

	bool HudInventory::onInput(const InputEvent &event) {
		if(event.isMouseOverEvent() && !isMouseOver(event))
			m_out_of_item_time = s_desc_visible_time;
		if(event.isMouseEvent())
			m_last_mouse_pos = event.mousePos();
		return false;
	}

	bool HudInventory::onEvent(const HudEvent &event) {
		ASSERT(m_pc_controller);

		if(!canShow())
			return false;

		if(event.type == HudEvent::button_clicked) {
			if(m_button_up.get() == event.source && m_row_offset > 0)
				m_row_offset--;
			if(m_button_down.get() == event.source && m_row_offset < m_max_row_offset)
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

		for(int n = 0; n < (int)m_buttons.size(); n++) {
			auto &button = m_buttons[n];
			button->setVisible(n < max_count, false);
			if(!button->isHighlighted() && !button->isEnabled())
				button->setHighlighted(false, false);
		}

		m_button_up  ->setPos(float2(rect().width() - spacing * 2 - s_button_size.x * 2, bottom + 5.0f));
		m_button_down->setPos(float2(rect().width() - spacing * 1 - s_button_size.x * 1, bottom + 5.0f));

		m_button_up->setGreyed(m_row_offset == 0);
		m_button_down->setGreyed(m_row_offset >= m_max_row_offset);
		m_button_up->setVisible(!m_entries.empty());
		m_button_down->setVisible(!m_entries.empty());
	}

	void HudInventory::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);
		float2 mouse_pos = float2(m_last_mouse_pos) - rect().min;
			
		updateData();

		FRect cur_rect = rect();
		FRect desc_rect(float2(m_item_desc->targetRect().width(), cur_rect.height()));
		desc_rect += float2(cur_rect.max.x + layer_spacing, cur_rect.min.y);
		m_item_desc->setRect(desc_rect);

		m_out_of_item_time += time_diff;
		m_item_desc->setVisible(isVisible() && !isHiding() && m_out_of_item_time < s_desc_visible_time);
		m_item_desc->update(time_diff);
		m_item_desc->layout();
	}
		
	void HudInventory::onDraw(Renderer2D &out) const {
		HudLayer::onDraw(out);
		m_item_desc->draw(out);
	}

}
