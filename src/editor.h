#ifndef EDITOR_H
#define EDITOR_H


#include "gfx/tile.h"
#include "gfx/font.h"



//TODO: maybe this is a bad idea?
struct EditorCtx
{
	vector<Tile> m_tiles;
	Font m_font;
	DTexture m_font_texture;

};



#endif
