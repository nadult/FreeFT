// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "ui/window.h"

namespace ui {

class EditBox : public Window {
  public:
	EditBox(const IRect &rect, int max_size, Str label = {}, FColor col = ColorId::transparent);
	const char *typeName() const override { return "EditBox"; }

	void setText(string32);
	const string32 &text() const { return m_text; }
	void reset(bool is_editing);

	void drawContents(Renderer2D &) const override;
	void onInput(const InputState &) override;
	bool onEvent(const Event &) override;
	//TODO: what if text character is not available in the font?

  private:
	void setCursorPos(int2);

	const Font &m_font;
	string32 m_text, m_old_text;
	string32 m_label;
	int m_cursor_pos;
	int m_max_size;
	bool m_is_editing;
};

using PEditBox = shared_ptr<EditBox>;

}
