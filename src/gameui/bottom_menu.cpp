/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gameui/bottom_menu.h"
#include "ui/image_button.h"
#include "gfx/device.h"

using namespace game;

namespace ui
{

	ImageButtonProto slotProto(int slot_id) {
		ImageButtonProto proto(nullptr,
				slot_id == 0? "btn/lhand_up" : "btn/rhand_up",
				slot_id == 0? "btn/lhand_dn" : "btn/rhand_dn",
				nullptr, FRect(0, 0, 1, 1));
		proto.sound_name = "butn_ingame";
		return proto;
	}

	SlotButton::SlotButton(const int2 &pos, int slot_id) :ImageButton(pos, slotProto(slot_id), "", ImageButton::mode_normal) {

	}

	void SlotButton::drawContents() const {
		ImageButton::drawContents();

		if(!m_item.isDummy()) {
			FRect image_rect;
			gfx::PTexture image = m_item.guiImage(false, image_rect);
			int2 size(image->width() * image_rect.width(), image->height() * image_rect.height());
			int2 pos = rect().size() / 2 - size / 2;
			if(m_mouse_press)
				pos -= int2(3, 0);
			image->bind();
			gfx::drawQuad(pos, size, image_rect.min, image_rect.max);
		}
	}

	namespace {

		struct ButtonConfig {
			int value;
			const char *name_up, *name_down, *text;
			int2 pos;
			bool is_group_button;

			PImageButton make() const {
				ImageButtonProto proto(nullptr, (string("btn/") + name_up).c_str(), (string("btn/") + name_down).c_str(),
										text? "transformers_20" : nullptr, FRect(0.35, 0.1, 0.8, 0.9));
				proto.sound_name = "butn_arrow";
				return new ImageButton(pos, proto, text? text : "",
						is_group_button? ImageButton::mode_toggle_on : ImageButton::mode_normal);
			}
		};

		const ButtonConfig s_stance_buttons[Stance::count] = {
			{ Stance::stand,	"Stand_Up",		"Stand_Dn", 	nullptr, int2(765, 45), true },
			{ Stance::crouch,	"Crouch_Up",	"Crouch_Dn",	nullptr, int2(765, 70), true },
			{ Stance::prone,	"Prone_Up",		"Prone_Dn",		nullptr, int2(765, 95), true }
		};
		
		const ButtonConfig s_sentry_buttons[SentryMode::count] = {
			{ SentryMode::passive,		"Sentry1_Up",	"Sentry1_Dn",	nullptr, int2(803, 45), true },
			{ SentryMode::defensive,	"Sentry2_Up",	"Sentry2_Dn",	nullptr, int2(803, 70), true },
			{ SentryMode::aggresive,	"Sentry3_Up",	"Sentry3_Dn",	nullptr, int2(803, 95), true }
		};

		const int2 s_slot_positions[BottomMenu::max_slots] = {
			int2(458, 46),
			int2(613, 46)
		};

		const ButtonConfig s_sub_menu_buttons[BottomMenu::sub_menu_count] = {
			{ BottomMenu::sub_menu_inventory,	"gbar_std_up",	"gbar_std_dn", 	"INV", int2(856, 6), false },
			{ BottomMenu::sub_menu_character,	"gbar_std_up",	"gbar_std_dn", 	"CHA", int2(856, 30), false },
			{ BottomMenu::sub_menu_options,		"gbar_std_up",	"gbar_std_dn", 	"OPT", int2(856, 54), false },
			{ BottomMenu::sub_menu_exit,		"gbar_std_up",	"gbar_std_dn", 	"EXT", int2(856, 78), false },
			{ BottomMenu::sub_menu_dummy,		"gbar_std_up",	"gbar_std_dn", 	"   ", int2(856, 102), false }
		};

		const int2 s_minimap_pos(124, 13);
	
	}

	BottomMenu::BottomMenu(const IRect &screen_rect) :Window(IRect(0, 0, 10, 10)) {
		gfx::PTexture back = gfx::DTexture::gui_mgr["back/game_bar_long"];
		setBackground(back);
		int2 pos(screen_rect.center().x - back->width() / 2, screen_rect.max.y - back->height());
		setRect(IRect(pos, pos + back->dimensions()));

		for(int n = 0; n < Stance::count; n++) {
			m_stance_buttons[n] = s_stance_buttons[n].make();
			attach(m_stance_buttons[n].get());
		}
		m_stance_buttons[0]->press(true);

		for(int n = 0; n < SentryMode::count; n++) {
			m_sentry_buttons[n] = s_sentry_buttons[n].make();
			attach(m_sentry_buttons[n].get());
		}
		m_sentry_buttons[0]->press(true);

		for(int n = 0; n < max_slots; n++) {
			m_slot_buttons[n] = new SlotButton(s_slot_positions[n], n);
			attach(m_slot_buttons[n].get());
		}
		
		for(int n = 0; n < sub_menu_count; n++) {
			m_sub_menu_buttons[n] = s_sub_menu_buttons[n].make();
			attach(m_sub_menu_buttons[n].get());
		}

		int tex_count = 5;
		for(int n = 0; n < tex_count; n++) {
			char name[256];
			snprintf(name, sizeof(name), "misc/Minimap_Noise%04d", n);
			m_minimap_noise.push_back(gfx::DTexture::gui_mgr[name]);
		}

		m_last_time = getTime() - 1.0f / 60.0f;
		m_noise_id = 0;
	}

	BottomMenu::~BottomMenu() { }

	void BottomMenu::drawContents() const {
		bool show_minimap_noise = false;

		if(show_minimap_noise) {
			if(getTime() - m_last_time > 0.15f) {
				m_last_time = getTime();
				m_noise_id = (m_noise_id + 1) % m_minimap_noise.size();
			}

			gfx::PTexture tex = m_minimap_noise[m_noise_id];
			tex->bind();
			gfx::drawQuad(s_minimap_pos, tex->dimensions());
		}
	}
		
	bool BottomMenu::onEvent(const Event &ev) {
		if(ev.type == Event::button_clicked && ev.source && ev.source->parent() == this) {
			bool is_stance = false, is_sentry = false;

			for(int n = 0; n < Stance::count; n++)
				if(m_stance_buttons[n].get() == ev.source)
					is_stance = true;
			for(int n = 0; n < SentryMode::count; n++)
				if(m_sentry_buttons[n].get() == ev.source)
					is_sentry = true;

			if(is_stance) for(int n = 0; n < Stance::count; n++)
				if(m_stance_buttons[n].get() != ev.source)
					m_stance_buttons[n]->press(false);
			if(is_sentry) for(int n = 0; n < SentryMode::count; n++)
				if(m_sentry_buttons[n].get() != ev.source)
					m_sentry_buttons[n]->press(false);
			return true;
		}

		return false;
	}
		
	void BottomMenu::setStance(Stance::Type stance) {
		for(int n = 0; n < Stance::count; n++)
			m_stance_buttons[n]->press(s_stance_buttons[n].value == stance);
	}

	Stance::Type BottomMenu::stance() const {
		Stance::Type out = Stance::stand;
		for(int n = 0; n < Stance::count; n++)
			if(m_stance_buttons[n]->isPressed())
				out = (Stance::Type)s_stance_buttons[n].value;
		return out;
	}

	void BottomMenu::setSentryMode(SentryMode::Type sentry) {
		for(int n = 0; n < SentryMode::count; n++)
			m_sentry_buttons[n]->press(s_sentry_buttons[n].value == sentry);
	}

	SentryMode::Type BottomMenu::sentryMode() const {
		SentryMode::Type out = SentryMode::passive;
		for(int n = 0; n < SentryMode::count; n++)
			if(m_sentry_buttons[n]->isPressed())
				out = (SentryMode::Type)s_sentry_buttons[n].value;
		return out;
	}

	void BottomMenu::setSlotItem(game::Item item, int slot_id) {
		DASSERT(slot_id >= 0 && slot_id < max_slots);
		m_slot_buttons[slot_id]->setItem(item);
	}

}
