// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "gfx/texture_cache.h"

#include "fwk_opengl.h"
#include "fwk/gfx/texture.h"
#include "fwk/gfx/dtexture.h"
#include <limits.h>
#include <algorithm>

//#define LOGGING

CachedTexture::CachedTexture() : m_cache(nullptr), m_id(-1) {}
CachedTexture::~CachedTexture() { unbindFromCache(); }

STexture CachedTexture::accessTexture(FRect &tex_rect, bool put_in_atlas) const {
	DASSERT(m_cache && m_id != -1);
	return m_cache->access(m_id, put_in_atlas, tex_rect);
}

void CachedTexture::unbindFromCache() const {
	if(m_cache && m_id != -1) {
		m_cache->remove(m_id);
		m_cache = nullptr;
	}
}

void CachedTexture::bindToCache(TextureCache &cache) const {
	if(m_cache == &cache)
		return;
	unbindFromCache();

	m_cache = &cache; // TODO: const correctness
	m_id = m_cache->add((CachedTexture *)this, textureSize());
}

void CachedTexture::onCacheDestroy() {
	m_id = -1;
	m_cache = nullptr;
}

static int textureMemorySize(STexture tex) {
	TextureFormat format = tex->format();
	return format.evalImageSize(tex->width(), tex->height());
}

TextureCache::TextureCache(int bytes)
	: m_memory_limit(bytes), m_memory_size(0), m_last_update(0), m_last_node(0),
	  m_atlas_counter(0) {
	int at_width = 2048, at_height = 2048;

	while(at_width * at_height > bytes / 8) {
		if(at_width > at_height)
			at_width /= 2;
		else
			at_height /= 2;
	}
	m_atlas_size = int2(at_width, at_height);
}

TextureCache::~TextureCache() {
	for(int n = 0; n < (int)m_resources.size(); n++)
		if(m_resources[n].res_ptr)
			m_resources[n].res_ptr->onCacheDestroy();
}

#define INSERT(list, list_name, id)                                                                \
	listInsert<Resource, &Resource::list_name##_node>(m_resources, list, id)
#define REMOVE(list, list_name, id)                                                                \
	listRemove<Resource, &Resource::list_name##_node>(m_resources, list, id)

void TextureCache::nextFrame() {
	if(!m_atlas) {
		int max_size = 2048;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);

		m_atlas_size = vmin(m_atlas_size, int2(max_size, max_size));
		ASSERT(m_atlas_size.x >= node_size && m_atlas_size.y >= node_size);

		m_atlas = make_shared<DTexture>(TextureFormatId::rgba, m_atlas_size);
		m_memory_limit -= textureMemorySize(m_atlas);
#ifdef LOGGING
		printf("Atlas size: %dKB\n", textureMemorySize(STexture(&m_atlas)) / 1024);
#endif

		int x_nodes = m_atlas_size.x / node_size, y_nodes = m_atlas_size.y / node_size;
		m_atlas_nodes.resize(x_nodes * y_nodes);
		for(int y = 0; y < y_nodes; y++)
			for(int x = 0; x < x_nodes; x++)
				m_atlas_nodes[x + y * x_nodes] = AtlasNode{IRect(x, y, x + 1, y + 1) * node_size};
	}

	if(m_atlas_queue.head != -1) {
		pair<int, int> indices[1024];
		int count = 0;
		AtlasNode &atlas_node = m_atlas_nodes[m_last_node];

		for(int id = atlas_node.list.head; id != -1 && count < arraySize(indices) / 2;) {
			const Resource &res = m_resources[id];
			indices[count++] = make_pair(-res.last_update, id);
			id = res.atlas_node.next;
		}

		int pixel_count = 0, max_pixels = node_size * node_size;
		for(int id = m_atlas_queue.head;
			id != -1 && count < arraySize(indices) && pixel_count < max_pixels;) {
			const Resource &res = m_resources[id];
			indices[count++] = make_pair(-res.last_update, id);
			pixel_count += res.size.x * res.size.y;
			id = res.atlas_node.next;
		}

		// TODO: handle situation in which resources already in atlas have timestamp >= than new
		// resources,
		// and there is almost no space for new resources. Such a node should be skipped
		std::sort(indices, indices + count);

		int2 pos(0, 0);
		int max_y = 0;

		// TODO: better texture fitting; something similar to buddy algorithm?
		for(int n = 0; n < count; n++) {
			int index = indices[n].second;
			Resource &res = m_resources[index];
			bool can_fit = true;

			if(res.size.y + pos.y > node_size)
				can_fit = false;
			if(can_fit && res.size.x + pos.x > node_size) {
				if(res.size.x > node_size || res.size.y + max_y + pos.y > node_size)
					can_fit = false;
				else {
					pos = int2(0, pos.y + max_y);
					max_y = 0;
				}
			}

			if(!can_fit) {
				if(res.atlas_node_id != -1) {
#ifdef LOGGING
					printf("Removing from atlas node %d: %dKB\n", m_last_node,
						   (res.size.x * res.size.y * 4) / 1024);
#endif
					REMOVE(atlas_node.list, atlas, index);
					res.atlas_node_id = -1;
					m_atlas_counter--;
				}
				continue;
			}

			Texture tex;
			res.res_ptr->cacheUpload(tex);
			DASSERT(tex.size() == res.size);
			res.atlas_pos = pos + atlas_node.rect.min();
			m_atlas->upload(tex, res.atlas_pos);

			if(res.atlas_node_id == -1) {
#ifdef LOGGING
				printf("Inserting to atlas node %d: %dKB\n", m_last_node,
					   (res.size.x * res.size.y * 4) / 1024);
#endif
				REMOVE(m_atlas_queue, atlas, index);
				INSERT(atlas_node.list, atlas, index);
				res.atlas_node_id = m_last_node;
				m_atlas_counter++;
			}

			max_y = max(max_y, res.size.y);
			pos.x += res.size.x;
		}

		m_last_node = (m_last_node + 1) % (int)m_atlas_nodes.size();
		while(m_atlas_queue.head != -1)
			REMOVE(m_atlas_queue, atlas, m_atlas_queue.head);
	}

	if(m_last_update == INT_MAX) {
		for(int n = 0; n < (int)m_resources.size(); n++)
			m_resources[n].last_update = 0;
		m_last_update = 0;
	}
	m_last_update++;
}

int TextureCache::add(CachedTexture *res_ptr, const int2 &size) {
	DASSERT(res_ptr);
	Resource new_res{res_ptr, STexture(nullptr), size, int2(0, 0), -1, 0};
	if(m_free_list.empty()) {
		m_resources.push_back(new_res);
		return (int)m_resources.size() - 1;
	}

	int id = m_free_list.tail;
	REMOVE(m_free_list, main, m_free_list.tail);
	m_resources[id] = new_res;
	return id;
}

void TextureCache::unload(int res_id) {
	Resource &res = m_resources[res_id];
	if(res.device_texture) {
		REMOVE(m_main_list, main, res_id);
		int size = textureMemorySize(res.device_texture);
#ifdef LOGGING
		printf("Freeing %dKB (current: %dKB / %dKB)\n", size / 1024, m_memory_size / 1024,
			   m_memory_limit / 1024);
#endif
		m_memory_size -= size;
		res.device_texture = nullptr;
	}
}

void TextureCache::remove(int res_id) {
	DASSERT(isValidId(res_id));
	Resource &res = m_resources[res_id];
	REMOVE(res.atlas_node_id == -1 ? m_atlas_queue : m_atlas_nodes[res.atlas_node_id].list, atlas,
		   res_id);
	if(res.atlas_node_id != -1) {
		m_atlas_counter--;
		res.atlas_node_id = -1;
	}

	unload(res_id);
	INSERT(m_free_list, main, res_id);
	m_resources[res_id].res_ptr = nullptr;
}

STexture TextureCache::access(int res_id, bool put_in_atlas, FRect &tex_rect) {
	DASSERT(isValidId(res_id));

	Resource &res = m_resources[res_id];
	res.last_update = m_last_update;

	if(res.atlas_node_id != -1) {
		if(res.device_texture)
			unload(res_id);
		float2 mul(1.0f / float(m_atlas_size.x), 1.0f / float(m_atlas_size.y));
		tex_rect = FRect(float2(res.atlas_pos) * mul, float2(res.atlas_pos + res.size) * mul);
		return m_atlas;
	} else if(put_in_atlas && res.atlas_node.prev == -1 && res.atlas_node.next == -1 &&
			  m_atlas_queue.head != res_id) {
		DASSERT(res.atlas_node_id == -1);
		if(res.size.x <= node_size / 2 && res.size.y <= node_size / 2)
			INSERT(m_atlas_queue, atlas, res_id);
	}

	if(!res.device_texture) {
		DASSERT(res.res_ptr);
		Texture temp_tex;
		res.res_ptr->cacheUpload(temp_tex);
		res.device_texture = make_shared<DTexture>(temp_tex);
		res.size = int2(temp_tex.width(), temp_tex.height());

		int new_size = textureMemorySize(res.device_texture);
#ifdef LOGGING
		printf("Loading %dKB (current: %dKB / %dKB)\n", new_size / 1024, m_memory_size / 1024,
			   m_memory_limit / 1024);
#endif
		m_memory_size += new_size;
		while(m_memory_size > m_memory_limit && m_main_list.tail != -1)
			unload(m_main_list.tail);

		INSERT(m_main_list, main, res_id);
	} else {
		// Moving to front
		REMOVE(m_main_list, main, res_id);
		INSERT(m_main_list, main, res_id);
	}

	auto dev_size = res.device_texture->size();
	tex_rect = dev_size.x == 0 || dev_size.y == 0?
				FRect() : FRect(float2(), float2(res.size) / float2(dev_size));
	return res.device_texture;
}

TextureCache TextureCache::main_cache(32 * 1024 * 1024);
