#ifndef NEWBORN_EDITOR_INPUT_H
#define NEWBORN_EDITOR_INPUT_H

#include "gfx/gl_window.h"

using namespace gfx;

class AppInput
{
public:
	AppInput(const GLWindow&a) : app(a), shift(0, 0)
	{
	}

	AppInput(const AppInput&i, const Vec2f&sh) : app(i.app), shift(i.shift + sh)
	{
	}

	char CharDown() const
	{
		return app.CharDown();
	}

	bool Key(uint k) const
	{
		return app.Key(k);
	}

	bool KeyDown(uint k) const
	{
		return app.KeyDown(k);
	}

	bool KeyUp(uint k) const
	{
		return app.KeyUp(k);
	}

	bool MouseKey(uint k) const
	{
		return app.MouseKey(k);
	}

	bool MouseKeyDown(uint k) const
	{
		return app.MouseKeyDown(k);
	}

	bool MouseKeyUp(uint k) const
	{
		return app.MouseKeyUp(k);
	}

	Vec3f MousePos() const
	{
		return app.MousePos() + Vec3f(shift);
	}

	Vec3f MouseMove() const
	{
		return app.MouseMove();
	}

private:
	const GLWindow&app;
	Vec2f          shift;
};

#endif
