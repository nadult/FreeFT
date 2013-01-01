#ifndef GFX_TEXTURE_CACHE_H
#define GFX_TEXTURE_CACHE_H

#include "base.h"

namespace gfx {

	class DTexture;
	typedef Ptr<DTexture> PTexture;

	class TextureCache {
	public:
		typedef PTexture (*AllocFunction)(const void *res_ptr);

		TextureCache(int max_bytes, AllocFunction); 
		~TextureCache();

		int add(const void *res_ptr);
		void remove(int res_id);
		void unload(int res_id);
		bool isValidId(int res_id) const { return res_id >= 0 && res_id < size(); }
		int size() const { return (int)m_resources.size(); }

		PTexture access(int res_id);

		void setMemoryLimit(int bytes) { m_memory_limit = bytes; }
		int memoryLimit() const { return m_memory_limit; }
		int memorySize() const { return m_memory_size; }

	protected:
		void link(int a, int b);

		struct Resource {
			const void *res_ptr;
			PTexture device_texture;
			int next_id, prev_id;
		};

		AllocFunction m_alloc_func;
		vector<Resource> m_resources;
		vector<int> m_free_list;
		int m_head_id, m_tail_id; // list of resources with loaded textures in LRU order
		int m_memory_limit;
		int m_memory_size;
	};

}


#endif
