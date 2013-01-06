#ifndef GFX_TEXTURE_CACHE_H
#define GFX_TEXTURE_CACHE_H

#include "base.h"
#include "gfx/device.h"

namespace gfx {

	class TextureCache;

	class CachedTexture {
	public:
		CachedTexture();
		CachedTexture(const CachedTexture&);
		void operator=(const CachedTexture&);
		virtual ~CachedTexture();

		PTexture accessTexture(FRect&) const;
		TextureCache *getCache() const { return m_cache; }
		int cacheId() const { return m_id; }
		void bindToCache(TextureCache&) const;
		void unbindFromCache() const;

		virtual void cacheUpload(Texture&) const = 0;
		virtual int2 textureSize() const = 0;

	private:
		void onCacheDestroy();
		friend class TextureCache;

		mutable TextureCache *m_cache; //TODO: remove, use TextureCache::main_cache?
		mutable int m_id;
	};

	class TextureCache {
	public:
		TextureCache(int max_bytes); 
		TextureCache(const TextureCache&) = delete;
		void operator=(const TextureCache&) = delete;
		~TextureCache();

		bool isValidId(int res_id) const { return res_id >= 0 && res_id < size(); }
		int size() const { return (int)m_resources.size(); }

		void unload(int res_id);
		PTexture access(int res_id, FRect&);

		void setMemoryLimit(int bytes) { m_memory_limit = bytes; }
		int memoryLimit() const { return m_memory_limit; }
		int memorySize() const { return m_memory_size; }
		void nextFrame();

		static TextureCache main_cache;

	protected:
		int add(CachedTexture *ptr, const int2 &size);
		void remove(int res_id);
		friend class CachedTexture;

	protected:
		struct ListNode {
			ListNode() :next(-1), prev(-1) { }

			int next, prev;
		};
		struct List {
			List() :head(-1), tail(-1) { }
			bool isEmpty() const { return head == -1; }

			int head, tail;
		};

		struct Resource {
			CachedTexture *res_ptr;
			PTexture device_texture;

			int2 size, atlas_pos;
			int atlas_node_id;	
			int last_update;
			
			ListNode main_node; // shared by two lists: m_main_list, m_free_list
			ListNode atlas_node; // shared by two lists: AtlasNode::list, m_atlas_queue
		};
		
		template<ListNode Resource::*>
		void listInsert(List &list, int id);

		template<ListNode Resource::*>
		void listRemove(List &list, int id);

		enum { node_size = 256 };

		struct AtlasNode {
			IRect rect;
			List list;
		};

		PodArray<AtlasNode> m_atlas_nodes;
		DTexture m_atlas;
		int2 m_atlas_size;
		int m_atlas_counter;

		vector<Resource> m_resources;
		int m_memory_limit;
		int m_memory_size;
		int m_last_update;
		int m_last_node;
		
		List m_main_list;
		List m_free_list;
		List m_atlas_queue;
	};

}


#endif
