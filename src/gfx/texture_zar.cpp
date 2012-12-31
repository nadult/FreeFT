#include "gfx/texture.h"
#include "base.h"

namespace gfx
{

	void Texture::loadZAR(Serializer &sr) {
		sr.signature("<zar>", 6);

		char zar_type, dummy1, has_palette;
		u32 img_width, img_height;

		sr(zar_type, dummy1, img_width, img_height, has_palette);

		if(zar_type != 0x33 && zar_type != 0x34)
			THROW("Wrong zar type: %d", (int)zar_type);

		vector<Color> palette;
		if(has_palette) {
			sr & palette;
			for(uint n = 0; n < palette.size(); n++)
				palette[n] = swapBR(palette[n]);
		}

		Color default_col(0, 0, 0);
		if(zar_type == 0x34 || zar_type == 0x33) {
			u32 def; sr & def; //TODO: there is something else here
			def = def & 255;
			ASSERT(def < palette.size());
			default_col = palette[def];
		}

		resize(img_width, img_height);

		Color* dst = this->line(0);
		Color* end = dst + img_width * img_height;

		while(dst < end) {
			u8 cmd; sr & cmd;
			int n_pixels = cmd >> 2;
			int command = cmd & 3;
			u8 buf[128];

			if(dst + n_pixels > end)
				n_pixels = end - dst;

			if(command == 0)
				for(int n = 0; n < n_pixels; n++)
					*dst++ = Color(0x00000000);
			else if(command == 1) {
				sr.data(buf, n_pixels);
				
				for(int n = 0; n < n_pixels; n++) {
					ASSERT(buf[n * 2] < palette.size());
					Color col = palette[buf[n]];
					col.a = 0xff;
					*dst++ = col;
				}
			}
			else if(command == 2) {
				sr.data(buf, n_pixels * 2);
				for(int n = 0; n < n_pixels; n++) {
					ASSERT(buf[n * 2] < palette.size());
					Color col = palette[buf[n * 2]];
					col.a = buf[n * 2 + 1];
					*dst++ = col;
				}
			}
			else { // command == 3
				sr.data(buf, n_pixels);
				for(int n = 0; n < n_pixels; n++) {
					Color col = default_col;
					col.a = buf[n];
					*dst++ = col;
				}
			}
		}

		/* //draw palette
		dst = (Color*)DataPointer();
		for(int n = 0; n < palette.size(); n++) {
			int x = n & 15, y = n / 16;

			Color col = palette[n];
			col.a = 255;
			for(int ky = 0; ky < 4; ky++) {
				Color* ptr = dst + (y * 4 + ky) * img_width + x * 4;
				for(int kx = 0; kx < 4; kx++)
					ptr[kx] = col;
			}
		}*/
	}

}	
