// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "ui/window.h"

namespace ui { 

	DEFINE_ENUM(MessageBoxMode, yes_no, ok_cancel, ok);

	class MessageBox: public Window
	{
	public:
		using Mode = MessageBoxMode;
		MessageBox(const IRect &rect, const char *message, Mode mode);
		const char *typeName() const override { return "MessageBox"; }

		bool onEvent(const Event &event) override;
		void drawContents(Renderer2D&) const override;

	private:
		Mode m_mode;
	};

}
