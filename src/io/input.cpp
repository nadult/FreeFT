/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/input.h"

namespace io
{

	// TODO: use system functions to get proper characters
	static int translateKey(int key, bool shift) {
		if(key == Key::space)
			return ' ';

		char numerics_s[11] = ")!@#$%^&*(";
		char map[][2] = {
			{ '-', '_' },
			{ '`', '~' },
			{ '=', '+' },
			{ '[', '{' },
			{ ']', '}' },
			{ ';', ':' },
			{ '\'', '"' },
			{ ',', '<' },
			{ '.', '>' },
			{ '/', '?' },
			{ '\\', '|' },
		};

		for(int i = 0; i < (int)sizeof(map) / 2; i++)
			if(key == map[i][0])
				return map[i][shift? 1 : 0];

		if(key >= 'A' && key <= 'Z')
			return shift ? key : key - 'A' + 'a';
		if(key >= '0' && key <= '9')
			return shift? numerics_s[key - '0'] : key;

		return key;
	}


	InputEvent::InputEvent(Type key_type, int key, int iter)
		:m_type(key_type), m_key(key), m_iteration(iter), m_mouse_pos(0.0f, 0.0f) {
		DASSERT(isKeyEvent());
	}

	InputEvent::InputEvent(Type mouse_type, int key, const int2 &mouse_move)
		:m_type(mouse_type), m_key(key), m_mouse_pos(0.0f, 0.0f), m_mouse_move(mouse_move) {
		DASSERT(isMouseEvent());
	}
		
	void InputEvent::init(int flags, const int2 &mouse_pos) {
		m_mouse_pos = mouse_pos;
		m_modifiers = flags;
		if(isKeyEvent())
			m_key_char = translateKey(m_key, m_modifiers & (mod_lshift | mod_rshift));
	}
		
	void InputEvent::translate(const float2 &offset) {
		if(isMouseEvent())
			m_mouse_pos += offset;
	}

	bool InputEvent::keyDown(int key) const {
		return m_type == key_down && m_key == key;
	}

	bool InputEvent::keyUp(int key) const {
		return m_type == key_up && m_key == key;
	}
	
	bool InputEvent::keyPressed(int key) const {
		return m_type == key_pressed && m_key == key;
	}

	bool InputEvent::keyDownAuto(int key, int period) const {
		DASSERT(period >= 1);
		return m_key == key && m_type == key_down_auto && (period == 1 || m_iteration % period == 0);
	}
		
	bool InputEvent::mouseKeyDown(int key) const {
		return m_type == mouse_key_down && m_key == key;
	}

	bool InputEvent::mouseKeyUp(int key) const {
		return m_type == mouse_key_up && m_key == key;
	}

	bool InputEvent::mouseKeyPressed(int key) const {
		return m_type == mouse_key_pressed && m_key == key;
	}
		
}
