/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gameui/hud.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"

using namespace game;
using namespace gfx;

namespace {

	const int2 s_hud_character_size(75, 100);
	const int2 s_hud_weapon_size(150, 100);

	Color s_back_color(30, 255, 60);

	void drawGradQuad(const FRect &rect, Color a, Color b, bool is_vertical) {
		Color colors[4] = { a, b, b, a };
		if(is_vertical)
			swap(colors[1], colors[3]);
		gfx::drawQuad(rect, FRect(0, 0, 1, 1), colors);
	}

	void drawLine(float2 p1, float2 p2, Color a, Color b) {
		if(p1.x > p2.x) {
			swap(p1.x, p2.x);
			swap(a, b);
		}
		if(p1.y > p2.y) {
			swap(p1.y, p2.y);
			swap(a, b);
		}

		bool is_vertical = false;
		if(p1.x == p2.x) {
			p1.x--;
			p2.x++;
			is_vertical = true;
		}
		else {
			p1.y--;
			p2.y++;
		}

		drawGradQuad(FRect(p1, p2), a, b, is_vertical);
	}

	void drawBorder(const FRect &rect, Color color, const float2 &offset, float width, bool is_left) {
		float2 min = rect.min - offset, max = rect.max + offset;
		width = ::min(width, (max.x - min.x) * 0.5f - 2.0f);

		DTexture::bind0();
		if(is_left) {
			drawLine(float2(min.x, min.y), float2(min.x, max.y), color, color);
			drawLine(float2(min.x + width, min.y), float2(min.x, min.y), Color::transparent, color);
			drawLine(float2(min.x + width, max.y), float2(min.x, max.y), Color::transparent, color);
		}
		else {
			drawLine(float2(max.x, min.y), float2(max.x, max.y), color, color);
			drawLine(float2(max.x - width, min.y), float2(max.x, min.y), Color::transparent, color);
			drawLine(float2(max.x - width, max.y), float2(max.x, max.y), Color::transparent, color);
		}
	}

	static void drawBack(const IRect &rect, int off_size) {
	}

};

namespace ui {

	HUDButton::HUDButton(const FRect &rect, float max_offset)
		:m_target_rect(rect), m_max_offset(max_offset) {
	}
		
	void HUDButton::update(double time_diff) {
	}

	void HUDButton::draw() const {
		DTexture::bind0();
		drawQuad(m_target_rect, Color(s_back_color, 127));

		drawBorder(m_target_rect, Color::green, float2(m_max_offset, m_max_offset), 20.0f, true);
		drawBorder(m_target_rect, Color::green, float2(m_max_offset, m_max_offset), 20.0f, false);
	}

	bool HUDButton::isMouseOver() const {
		return rect().isInside((float2)getMousePos());
	}
		
	const FRect HUDButton::rect() const {
		return m_target_rect;
	}

	HUDCharacter::HUDCharacter(const FRect &target_rect)
		:HUDButton(target_rect, 5.0f), m_current_hp(0), m_max_hp(0) { }

	void HUDCharacter::update(double time_diff) {
		HUDButton::update(time_diff);
	}

	void HUDCharacter::draw() const {
		HUDButton::draw();
		FRect rect = this->rect();
	
		PTexture icon = m_character? m_character->icon() : Character::defaultIcon();
		icon->bind();
		float2 pos = rect.center(), icon_size(icon->dimensions());
		pos -= icon_size * 0.5f;

		float hp_value = m_max_hp? clamp(float(m_current_hp) / m_max_hp, 0.0f, 1.0f) : 1.0f;
		Color color(lerp((float4)Color(Color::red), (float4)Color(Color::green), hp_value));

		drawQuad(FRect(pos, pos + icon_size), color);

		if(m_max_hp) {
			PFont font = Font::mgr["transformers_20"];
			char text[256];
			snprintf(text, sizeof(text), hp_value <= 0.0f? "DEAD" : "%d/%d", m_current_hp, m_max_hp);
			IRect extents = font->evalExtents(text);

			float2 pos(rect.max.x - extents.width(), rect.min.y);
			font->drawShadowed((int2)pos, Color::white, Color::black, "%s", text);
		}
	}
		
	HUDWeapon::HUDWeapon(const FRect &target_rect)
		:HUDButton(target_rect, 5.0f), m_attack_mode(AttackMode::undefined), m_ammo_count(0) { }
		
	void HUDWeapon::update(double time_diff) {
		HUDButton::update(time_diff);

	}

	void HUDWeapon::draw() const {
		HUDButton::draw();

	}

	HUD::HUD(PWorld world, EntityRef actor_ref) :m_world(world), m_actor_ref(actor_ref) {
		int2 res = gfx::getWindowSize();
		float spacing = 15.0f, bottom = res.y - spacing;

		FRect char_rect({0.0f, 0.0f}, s_hud_character_size);
		char_rect += float2(spacing, bottom - char_rect.height());

		FRect weapon_rect({0.0f, 0.0f}, s_hud_weapon_size);
		weapon_rect += float2(char_rect.max.x + spacing, bottom - weapon_rect.height());

		m_hud_character.reset(new HUDCharacter(char_rect));
		m_hud_weapon.reset(new HUDWeapon(weapon_rect));

	//	m_hud_character.reset(new HUDCharacter(FRect(
	//	m_rect = IRect(0, res.y - s_rect_size.y, s_rect_size.x, res.y) + s_rect_offset;
	}

	HUD::~HUD() { }

	void HUD::update(double time_diff) {
		const Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		if(!actor)
			return;

		m_hud_character->setCharacter(actor->character());
		m_hud_character->setHP(actor->hitPoints(), actor->proto().actor->hit_points);

		m_hud_character->update(time_diff);
		m_hud_weapon->update(time_diff);
	}

	void HUD::draw() const {
		m_hud_character->draw();
		m_hud_weapon->draw();
	}

	bool HUD::isMouseOver() const {
		return	m_hud_character->isMouseOver() ||
				m_hud_weapon->isMouseOver();
	}

}
