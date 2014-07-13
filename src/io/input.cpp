/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/input.h"

namespace io
{
	InputEvent::InputEvent(Type key_type, int key, int iter)
		:m_type(key_type), m_key(key), m_iteration(iter), m_mouse_pos(0.0f, 0.0f) {
		DASSERT(isKeyEvent());
	}

	InputEvent::InputEvent(Type mouse_type, int key, const int2 &mouse_move)
		:m_type(mouse_type), m_key(key), m_mouse_pos(0.0f, 0.0f), m_mouse_move(mouse_move) {
		DASSERT(isMouseEvent());
	}
		
	void InputEvent::setMousePos(const int2 &pos) {
		m_mouse_pos = pos;
	}

	void InputEvent::setModifiers(int flags) {
		m_modifiers = flags;
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
		return m_type == key_down_auto && m_key == key && (period == 1 || m_iteration % period == 0);
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
