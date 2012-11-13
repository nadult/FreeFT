#include "gfx/font.h"
#include "gfx/device.h"
#include <GL/gl.h>
#include <cstring>
#include <cwchar>
#include <cstdarg>

namespace gfx
{
	void Font::serialize(Serializer &sr) {
		ASSERT(sr.isLoading());

		string text;
		text.resize(sr.size());
		sr.data(&text[0], sr.size());

		XMLDocument doc;
		doc.parse<0>(&text[0]); 
		loadFromXML(doc);
	}

	void Font::loadFromXML(const XMLDocument &doc) {
		m_chars.clear();
		m_kernings.clear();
		m_texture = nullptr;
		m_face_name.clear();

		XMLNode *font_node = doc.first_node("font");
		ASSERT(font_node);

		XMLNode *info_node = font_node->first_node("info");
		XMLNode *pages_node = font_node->first_node("pages");
		XMLNode *chars_node = font_node->first_node("chars");
		XMLNode *common_node = font_node->first_node("common");
		ASSERT(info_node && pages_node && chars_node && common_node);

		const char *face_name = getStringAttribute(info_node, "face");
		ASSERT(face_name);
		m_face_name = face_name;

		m_tex_size = int2(getIntAttribute(common_node, "scaleW"), getIntAttribute(common_node, "scaleH"));
		m_line_height = getIntAttribute(common_node, "lineHeight");
		m_text_base = getIntAttribute(common_node, "base");

		int page_count = getIntAttribute(common_node, "pages");
		ASSERT(page_count == 1);

		XMLNode *first_page_node = pages_node->first_node("page");
		ASSERT(first_page_node);
		ASSERT(getIntAttribute(first_page_node, "id") == 0);

		const char *tex_file_name = getStringAttribute(first_page_node, "file");
		ASSERT(tex_file_name);

		m_texture = tex_mgr[tex_file_name];
		ASSERT(m_texture && m_texture->size() == m_tex_size);

		int chars_count = getIntAttribute(chars_node, "count");
		XMLNode *char_node = chars_node->first_node();

		while(char_node) {
			if(strcmp(char_node->name(), "char") == 0) {
				Character chr;
				int id = getIntAttribute(char_node, "id");
				chr.tex_pos = int2(getIntAttribute(char_node, "x"), getIntAttribute(char_node, "y"));
				chr.size = int2(getIntAttribute(char_node, "width"), getIntAttribute(char_node, "height"));
				chr.offset = int2(getIntAttribute(char_node, "xoffset"), getIntAttribute(char_node, "yoffset"));
				chr.x_advance = getIntAttribute(char_node, "xadvance");
				m_chars[id] = chr;

				chars_count--;
			}
			
			char_node = char_node->next_sibling();
		}
		ASSERT(chars_count == 0);
		ASSERT(m_chars.find((int)' ') != m_chars.end());

		XMLNode *kernings_node = font_node->first_node("kernings");
		if(kernings_node) {
			int kernings_count = getIntAttribute(kernings_node, "count");

			XMLNode *kerning_node = kernings_node->first_node();
			while(kerning_node) {
				if(strcmp(kerning_node->name(), "kerning") == 0) {
					int first = getIntAttribute(kerning_node, "first");
					int second = getIntAttribute(kerning_node, "second");
					int value = getIntAttribute(kerning_node, "amount");
					m_kernings[make_pair(first, second)] = value;
					kernings_count--;
				}

				kerning_node = kerning_node->next_sibling();
			}
			ASSERT(kernings_count == 0);
		}
	}

	static int convertToWChar(const char *str, wchar_t *wstr, int buffer_size) {
		mbstate_t ps;
		memset(&ps, 0, sizeof(ps));

		size_t len = mbsrtowcs(wstr, &str, buffer_size, &ps);
		return (int)(len == (size_t)-1? 0 : len);
	}

	IRect Font::evalExtents(const char *str) const {
		wchar_t wstr[1024];
		int len = convertToWChar(str, wstr, sizeof(wstr) / sizeof(wchar_t));

		IRect rect(0, 0, 0, 0);
		int2 pos(0, 0);

		for(int n = 0; n < len; n++) {
			if(wstr[n] == '\n') {
				pos.x = 0;
				pos.y += m_line_height;
				continue;
			}

			auto char_it = m_chars.find(wstr[n]);
			if(char_it == m_chars.end()) {
				char_it = m_chars.find((int)' ');
				DASSERT(char_it != m_chars.end());
			}

			const Character &chr = char_it->second;
		
			IRect new_rect(pos + chr.offset, pos + chr.offset + chr.size);
			rect = n == 0? new_rect : rect + new_rect;
			if(n + 1 < len) {
				pos.x += chr.x_advance;
				auto kerning_it = m_kernings.find(make_pair((int)wstr[n], (int)wstr[n + 1]));
				if(kerning_it != m_kernings.end())
					pos.x += kerning_it->second;
			}
		}

		return rect;
	}

	int Font::genQuads(const char *str, float2 *out_pos, float2 *out_uv, int buffer_size) const {
		DASSERT(buffer_size >= 0 && str);
		
		wchar_t wstr[1024];
		int len = convertToWChar(str, wstr, sizeof(wstr) / sizeof(wchar_t));

		int count = min(len, buffer_size);
		float2 pos(0, 0);
		float2 tex_scale(1.0f / float(m_tex_size.x), 1.0f / float(m_tex_size.y));

		int gen_count = 0;
		for(int n = 0; n < count; n++) {
			if(wstr[n] == '\n') {
				pos.x = 0;
				pos.y += m_line_height;
				continue;
			}

			auto char_it = m_chars.find(wstr[n]);
			if(char_it == m_chars.end()) {
				char_it = m_chars.find((int)' ');
				DASSERT(char_it != m_chars.end());
			}

			const Character &chr = char_it->second;

			float2 spos = pos + (float2)chr.offset;
			out_pos[0] = spos + float2(0.0f,		0.0f);
			out_pos[1] = spos + float2(chr.size.x,	0.0f);
			out_pos[2] = spos + float2(chr.size.x,	chr.size.y);
			out_pos[3] = spos + float2(0.0f,		chr.size.y);
			
			float2 tpos = (float2)chr.tex_pos;
			out_uv[0] = tpos + float2(0.0f,		0.0f);
			out_uv[1] = tpos + float2(chr.size.x,	0.0f);
			out_uv[2] = tpos + float2(chr.size.x,	chr.size.y);
			out_uv[3] = tpos + float2(0.0f,		chr.size.y);
			for(int i = 0; i < 4; i++) {
				out_uv[i].x *= tex_scale.x;
				out_uv[i].y *= tex_scale.y;
			}

			if(n + 1 < count) {
				pos.x += chr.x_advance;
				auto kerning_it = m_kernings.find(make_pair((int)wstr[n], (int)wstr[n + 1]));
				if(kerning_it != m_kernings.end())
					pos.x += kerning_it->second;
			}
			
			out_pos	+= 4;
			out_uv += 4;
			gen_count++;
		}

		return gen_count;
	}

	void Font::draw(int2 pos, Color color, const char *format, ...) const {
		char text[256];
		va_list ap;
		va_start(ap, format);
		vsnprintf(text, sizeof(text), format, ap);
		va_end(ap);

		ASSERT(m_texture);
		m_texture->bind();

		enum { buf_size = 1024 };
		float2 pos_buf[buf_size], uv_buf[buf_size];
		int quad_count = genQuads(text, pos_buf, uv_buf, buf_size);

		glBegin(GL_QUADS);
		glColor4ub(color.r, color.g, color.b, color.a);
		for(int n = 0, count = quad_count * 4; n < count; n++) {
			glTexCoord2f( uv_buf[n].x,  uv_buf[n].y);
			glVertex2f  (pos_buf[n].x + pos.x, pos_buf[n].y + pos.y);
		}
		glEnd();
	}

	void Font::drawShadowed(int2 pos, Color color, Color shadow, const char *format, ...) const {
		char text[256];
		va_list ap;
		va_start(ap, format);
		vsnprintf(text, sizeof(text), format, ap);
		va_end(ap);

		ASSERT(m_texture);
		m_texture->bind();

		enum { buf_size = 1024 };
		float2 pos_buf[buf_size], uv_buf[buf_size];
		int quad_count = genQuads(text, pos_buf, uv_buf, buf_size);

		glBegin(GL_QUADS);
		glColor4ub(shadow.r, shadow.g, shadow.b, shadow.a);
		pos += int2(1, 1);
		for(int n = 0, count = quad_count * 4; n < count; n++) {
			glTexCoord2f( uv_buf[n].x,  uv_buf[n].y);
			glVertex2f  (pos_buf[n].x + pos.x, pos_buf[n].y + pos.y);
		}

		pos -= int2(1, 1);
		glColor4ub(color.r, color.g, color.b, color.a);
		for(int n = 0, count = quad_count * 4; n < count; n++) {
			glTexCoord2f( uv_buf[n].x,  uv_buf[n].y);
			glVertex2f  (pos_buf[n].x + pos.x, pos_buf[n].y + pos.y);
		}
		glEnd();
	}

	ResourceMgr<Font> Font::mgr("../data/fonts/", ".fnt"); 
	ResourceMgr<DTexture> Font::tex_mgr("../data/fonts/", "");
}
