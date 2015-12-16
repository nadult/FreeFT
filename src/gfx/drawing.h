/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GFX_DRAWING_H
#define GFX_DRAWING_H

#include "base.h"

void drawBBox(Renderer2D &, const FBox &box, Color col = Color::white, bool is_filled = false);
void drawBBox(Renderer2D &, const IBox &wbox, Color col = Color::white, bool is_filled = false);

#endif
