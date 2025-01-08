// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/class.h"

#include "game/character.h"
#include "game/inventory.h"
#include "game/pc_controller.h"

#include "game/world.h"
#include "gfx/drawing.h"

#include <algorithm>
#include <fwk/gfx/canvas_2d.h>
#include <fwk/gfx/font.h>

namespace hud {

namespace {
	const float2 s_button_size(15, 15);
	const float s_bottom_size = 30.0f;
	const float s_item_height = 65.0f;
	const float s_min_item_width = 50.0f;
	int s_max_buttons = 4;
}

HudClassButton::HudClassButton(const FRect &rect) : HudRadioButton(rect, -1, 1) {}

void HudClassButton::onDraw(Canvas2D &out) const {
	HudButton::onDraw(out);
	FRect rect = this->rect();

	if(m_id != -1) {
		const CharacterClass &char_class = CharacterClass::get(m_id);
		ActorInventory inv = char_class.inventory(true);
		float2 pos(spacing + 30.0f, rect.center().y);

		vector<pair<Item, int>> items;
		if(inv.isEquipped(ItemType::weapon))
			items.emplace_back(inv.weapon(), 1);
		if(inv.isEquipped(ItemType::ammo))
			items.emplace_back(inv.ammo().item, inv.ammo().count);
		if(inv.isEquipped(ItemType::armour))
			items.emplace_back(inv.armour(), 1);
		for(int n = 0; n < inv.size(); n++) {
			const auto &entry = inv[n];
			bool added = false;

			for(int i = 0; i < (int)items.size(); i++)
				if(items[i].first == entry.item) {
					added = true;
					items[i].second += entry.count;
				}
			if(!added)
				items.emplace_back(entry.item, entry.count);
		}

		for(auto &item : items) {
			FRect uv_rect;
			auto texture = item.first.guiImage(true, uv_rect);
			auto tex_size = texture->size2D();
			float2 size(tex_size.x * uv_rect.width(), tex_size.y * uv_rect.height());

			FRect irect(pos.x, rect.y(), pos.x + max(s_min_item_width, size.x), rect.ey());
			if(irect.ex() > rect.width())
				break;

			out.setMaterial(texture);
			out.addFilledRect(FRect(irect.center() - size * 0.5f, irect.center() + size * 0.5f),
							  uv_rect);

			if(item.second > 1) {
				FRect trect = {irect.min(), {irect.ex(), irect.ey() - 5.0f}};
				m_font->draw(out, trect,
							 {textColor(), textShadowColor(), HAlign::right, VAlign::bottom},
							 format("%d", item.second));
			}

			pos.x += irect.width() + spacing;
		}

		m_font->draw(out, rect, {textColor(), textShadowColor(), HAlign::left, VAlign::top},
					 char_class.name());
	}
}

Color HudClassButton::backgroundColor() const {
	return (Color)lerp((FColor)HudButton::backgroundColor(), FColor(ColorId::white),
					   m_enabled_time * 0.5f);
}

HudClass::HudClass(const FRect &target_rect) : HudLayer(target_rect), m_offset(0) {

	setTitle("Class selection:");

	for(int n = 0; n < s_max_buttons; n++) {
		float diff = s_item_height + spacing * 2;
		float2 pos(HudButton::spacing,
				   HudButton::spacing + (s_item_height + HudButton::spacing) * n + topOffset());
		FRect rect(pos, pos + float2(target_rect.width() - HudButton::spacing * 2, s_item_height));
		auto button = make_shared<HudClassButton>(rect);
		attach(button);
		m_buttons.push_back(std::move(button));
	}

	m_button_up = make_shared<HudClickButton>(FRect(s_button_size));
	m_button_up->setIcon(HudIcon::up_arrow);
	m_button_up->setAccelerator(InputKey::pageup);
	m_button_up->setButtonStyle(HudButtonStyle::small);

	m_button_down = make_shared<HudClickButton>(FRect(s_button_size));
	m_button_down->setIcon(HudIcon::down_arrow);
	m_button_down->setAccelerator(InputKey::pagedown);
	m_button_down->setButtonStyle(HudButtonStyle::small);

	attach(m_button_up);
	attach(m_button_down);
}

HudClass::~HudClass() {}

bool HudClass::canShow() const {
	// TODO: This should be visible even when pc_controller is inactive
	return m_pc_controller && m_pc_controller->world().isClient();
}

bool HudClass::onEvent(const HudEvent &event) {
	if(event.type == HudEvent::button_clicked) {
		int max_offset = ((int)m_class_ids.size() + s_max_buttons - 1) / s_max_buttons - 1;

		if(m_button_up == event.source && m_offset > 0)
			m_offset--;
		if(m_button_down == event.source && m_offset < max_offset)
			m_offset++;
		if(isOneOf(event.source, m_buttons)) {
			HudButton *button = dynamic_cast<HudButton *>(event.source);
			if(m_pc_controller)
				m_pc_controller->setClassId(button->id());
		}

		needsLayout();

		return true;
	}

	return false;
}

void HudClass::onLayout() {
	HudLayer::onLayout();

	m_class_ids.clear();
	if(m_pc_controller) {
		const string &proto_id = m_pc_controller->pc().character().proto().id;

		for(int n = 0; n < CharacterClass::count(); n++)
			if(n != CharacterClass::defaultId() && CharacterClass::get(n).isValidForActor(proto_id))
				m_class_ids.emplace_back(n);
	}

	int max_offset = ((int)m_class_ids.size() + s_max_buttons - 1) / s_max_buttons - 1;

	float bottom = 0.0f;
	for(int n = 0; n < (int)m_buttons.size(); n++) {
		int id = n + m_offset;
		if(id >= (int)m_class_ids.size())
			id = -1;

		m_buttons[n]->setVisible(id != -1, false);
		m_buttons[n]->setId(id == -1 ? -1 : m_class_ids[id]);

		if(id != -1)
			bottom = max(bottom, m_buttons[n]->rect().ey() + spacing);
	}

	m_button_up->setPos(float2(rect().width() - spacing * 2 - s_button_size.x * 2, bottom + 5.0f));
	m_button_down->setPos(
		float2(rect().width() - spacing * 1 - s_button_size.x * 1, bottom + 5.0f));
	m_button_up->setGreyed(m_offset == 0);
	m_button_down->setGreyed(m_offset >= max_offset);
}

void HudClass::onPCControllerSet() { needsLayout(); }

void HudClass::onUpdate(double time_diff) {
	int class_id = m_pc_controller ? m_pc_controller->classId() : -1;
	for(auto &button : m_buttons) {
		button->setEnabled(button->id() == class_id);
		button->setGreyed(!m_pc_controller);
	}
}

}
