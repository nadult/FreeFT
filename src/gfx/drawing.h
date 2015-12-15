/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GFX_DRAWING_H
#define GFX_DRAWING_H

#include "fwk_opengl.h"
#include "base.h"

void drawLine(int3 wp1, int3 wp2, Color color = Color::white);
void drawLine(int2 p1, int2 p2, Color color = Color::white);

void drawBBox(const FBox &box, Color col = Color::white, bool is_filled = false);
void drawBBox(const IBox &wbox, Color col = Color::white, bool is_filled = false);

void setScissorRect(const IRect &rect);
void setScissorTest(bool is_enabled);

void initViewport(int2 size);
void clear(Color);
void lookAt(int2 pos);

void drawQuad(int2 pos, int2 size, Color color = Color::white);
void drawQuad(int2 pos, int2 size, const float2 &uv0, const float2 &uv1, Color color = Color::white);
void drawQuad(const FRect &rect, const FRect &uv_rect, Color colors[4]);
void drawQuad(const FRect &rect, const FRect &uv_rect, Color color = Color::white);
inline void drawQuad(const FRect &rect, Color color = Color::white) { drawQuad(rect, FRect(0, 0, 1, 1), color); }

#endif
