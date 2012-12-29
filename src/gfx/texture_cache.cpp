#include "gfx/texture_cache.h"
#include "gfx/device.h"


namespace gfx {

	static int textureMemorySize(PTexture tex) {
		//TODO: mipmaps
		TextureFormat format = tex->format();
		return format.evalImageSize(tex->width(), tex->height());
	}

	TextureCache::TextureCache(int bytes, TextureCache::AllocFunction alloc_func)
		:m_memory_limit(bytes), m_alloc_func(alloc_func), m_memory_size(0), m_head_id(-1), m_tail_id(-1) {
		DASSERT(alloc_func);
	}

	int TextureCache::add(const void *res_ptr) {
		DASSERT(res_ptr);
		Resource new_res{res_ptr, PTexture(nullptr), -1, -1};
		if(m_free_list.empty()) {
			m_resources.push_back(new_res);
			return (int)m_resources.size() - 1;
		}
		
		int id = m_free_list.back();
		m_free_list.pop_back();
		m_resources[id] = new_res;
		return id;
	}

	void TextureCache::unload(int res_id) {
		Resource &res = m_resources[res_id];
		if(m_head_id == res_id)
			m_head_id = res.next_id;
		if(m_tail_id == res_id)
			m_tail_id = res.prev_id;
		link(res.prev_id, res.next_id);
		res.prev_id = res.next_id = -1;
		int size = textureMemorySize(res.device_texture);
	//	printf("Freeing %dKB (current: %dKB / %dKB)\n", size/1024, m_memory_size/1024, m_memory_limit/1024);
		m_memory_size -= size;
		res.device_texture = nullptr;
	}

	void TextureCache::remove(int res_id) {
		DASSERT(isValidId(res_id));
		unload(res_id);
		m_free_list.push_back(res_id);
		m_resources[res_id].res_ptr = nullptr;
	}

	PTexture TextureCache::access(int res_id) {
		DASSERT(isValidId(res_id));

		Resource &res = m_resources[res_id];
		if(!res.device_texture) {
			DASSERT(res.res_ptr);
			DASSERT(res.next_id == -1 && res.prev_id == -1);
			res.device_texture = m_alloc_func(res.res_ptr);
			DASSERT(res.device_texture);

			int new_size = textureMemorySize(res.device_texture);
		//	printf("Loading %dKB (current: %dKB / %dKB)\n", new_size/1024, m_memory_size/1024, m_memory_limit/1024);
			m_memory_size += new_size;
			while(m_memory_size > m_memory_limit && m_tail_id != -1)
				unload(m_tail_id);

			link(res_id, m_head_id);
			m_head_id = res_id;
			if(m_tail_id == -1)
				m_tail_id = res_id;
		}
		else if(m_head_id != res_id) {
			link(res.prev_id, res.next_id);
			DASSERT(res.prev_id != -1);
			if(m_tail_id == res_id)
				m_tail_id = res.prev_id;
			link(res_id, m_head_id);
			res.prev_id = -1;
			m_head_id = res_id;
		}

		return res.device_texture;
	}

	void TextureCache::link(int a, int b) {
		if(a != -1)
			m_resources[a].next_id = b;
		if(b != -1)
			m_resources[b].prev_id = a;
	}

}
