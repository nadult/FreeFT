/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GFX_DRAWING_H
#define GFX_DRAWING_H

#include "fwk_opengl.h"
#include "base.h"

void drawLine(int3 wp1, int3 wp2, Color color);
void drawBBox(const FBox &box, Color col = Color::white, bool is_filled = false);
void drawBBox(const IBox &wbox, Color col = Color::white, bool is_filled = false);

#endif
