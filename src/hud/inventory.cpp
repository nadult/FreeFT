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

using namespace gfx;

namespace hud {
	
	namespace {
		const float2 s_item_size(70, 65);
		const float2 s_item_desc_size(340, 100);
		const float s_spacing = 17.0f;

		const int2 s_grid_size(4, 10);
	}	
		
	HudItemDesc::HudItemDesc(const FRect &rect) :HudWidget(rect) { }

	void HudItemDesc::draw() const {
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
			drawQuad(FRect(pos, pos + size), uv_rect, focusColor());
		}

	}

	HudInventoryItem::HudInventoryItem(const FRect &rect) :HudWidget(rect) { }

	void HudInventoryItem::draw() const {
		if(!isVisible())
			return;

		HudWidget::draw();
		FRect rect = this->rect();

		if(!m_entry.item.isDummy()) {
			FRect uv_rect;
			gfx::PTexture texture = m_entry.item.guiImage(true, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (int2)(rect.center() - size / 2);
			texture->bind();
			drawQuad(FRect(pos, pos + size), uv_rect);
		}
	}

	HudInventory::HudInventory(PWorld world, EntityRef actor_ref, const FRect &target_rect)
		:HudLayer(target_rect), m_world(world), m_actor_ref(actor_ref) {
		DASSERT(m_world);

		for(int y = 0; y < s_grid_size.y; y++)
			for(int x = 0; x < s_grid_size.x; x++) {
				float2 pos(s_item_size.x * x + s_spacing * (x + 1), s_item_size.y * y + s_spacing * (y + 1));
				PHudInventoryItem item(new HudInventoryItem(FRect(pos, pos + s_item_size)));

				attach(item.get());
				m_buttons.emplace_back(std::move(item));
			}

		m_item_desc = new HudItemDesc(FRect(s_item_desc_size) + float2(s_spacing, s_spacing));
		m_item_desc->setVisible(false);

		attach(m_item_desc.get());
	}
		
	HudInventory::~HudInventory() { }

	void HudInventory::update(bool is_active, double time_diff) {
		HudLayer::update(is_active, time_diff);
		const Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		float2 mouse_pos = float2(getMousePos()) - rect().min;

		if(actor) {
			const ActorInventory &orig = actor->inventory();
			ActorInventory temp = orig;
			temp.unequip(ItemType::armour);

			int max_count = min(temp.size(), (int)m_buttons.size());
			for(int n = 0; n < max_count; n++)
				m_buttons[n]->setEntry(temp[n]);
			for(int n = 0; n < (int)m_buttons.size(); n++)
				m_buttons[n]->setVisible(n < max_count, false);
		}
		
		int over_item = -1;
		for(int n = 0; n < (int)m_buttons.size(); n++)
			if(m_buttons[n]->isMouseOver(mouse_pos)) {
				over_item = n;
				break;
			}

		if(over_item != -1) {
			FRect button_rect = m_buttons[over_item]->rect();
			float offset = rect().min.y;

			float best_pos = button_rect.min.y - m_item_desc->rect().height() - s_spacing;
			if(best_pos + offset < 0.0f)
				best_pos = button_rect.max.y + s_spacing;

			FRect rect = m_item_desc->targetRect();
			rect += float2(0.0f, best_pos - rect.min.y);

			m_item_desc->setTargetRect(rect);
			m_item_desc->setItem(m_buttons[over_item]->entry().item);
		}
			
	//	m_item_desc->setVisible(over_item != -1);
	//	m_item_desc->setFocus(true);
	}

	float HudInventory::preferredHeight() const {
		FRect rect;

		for(int n = 0; n < (int)m_buttons.size(); n++)
			if(m_buttons[n]->isVisible())
				rect = sum(rect, m_buttons[n]->rect());

		return rect.height() + s_spacing * 2.0f;
	}
		
}
