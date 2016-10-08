/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GFX_DRAWING_H
#define GFX_DRAWING_H

#include "base.h"

void drawBBox(Renderer2D &, const FBox &box, Color col = ColorId::white, bool is_filled = false);
void drawBBox(Renderer2D &, const IBox &wbox, Color col = ColorId::white, bool is_filled = false);

void drawLine(Renderer2D&, int3 wp1, int3 wp2, Color color = ColorId::white);

#endif
