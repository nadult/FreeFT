// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"
#include <fwk/list_node.h>
#include <fwk/vulkan/vulkan_image.h>
#include <fwk/vulkan_base.h>

class TextureCache;

class CachedTexture {
  public:
	CachedTexture();
	CachedTexture(const CachedTexture &);
	void operator=(const CachedTexture &);
	virtual ~CachedTexture();

	PVImageView accessTexture(FRect &, bool put_in_atlas = true) const;
	int cacheId() const { return m_id; }

	bool isBind() const { return m_id != -1; }
	void bindToCache() const;
	void unbindFromCache() const;

	virtual void cacheUpload(Image &) const = 0;
	virtual int2 textureSize() const = 0;

  private:
	void onCacheDestroy();
	friend class TextureCache;

	mutable int m_id;
};

// accesed textures are valid only until nextFrame, if the texture
// was in the atlas, it's data might change, so accessTexture should be called every frame
//
// TODO: texture packing in the atlas can be greatly improved:
// - instead of round robin, select next AtlasNode based on active texture count, free space, etc.
// - when packing textures, use some smarter algorithm to save space
// - use PBO to copy textures from res.device_texture, and not from system memory
//
// Only single instance allowed
class TextureCache {
  public:
	TextureCache(VulkanDevice &, int max_bytes = 32 * 1024 * 1024);
	~TextureCache();

	TextureCache(const TextureCache &) = delete;
	void operator=(const TextureCache &) = delete;

	static TextureCache &instance() {
		DASSERT(g_instance);
		return *g_instance;
	}

	bool isValidId(int res_id) const { return res_id >= 0 && res_id < size(); }
	int size() const { return (int)m_resources.size(); }

	void unload(int res_id);
	PVImageView access(int res_id, bool put_in_atlas, FRect &);
	PVImageView atlas() { return m_atlas; }

	void setMemoryLimit(int bytes) { m_memory_limit = bytes; }
	int memoryLimit() const { return m_memory_limit; }
	int memorySize() const { return m_memory_size; }
	Ex<> nextFrame();

  private:
	static TextureCache *g_instance;

	int add(CachedTexture *ptr, const int2 &size);
	void remove(int res_id);
	friend class CachedTexture;

	struct Resource {
		CachedTexture *res_ptr;
		PVImageView device_texture;

		int2 size, atlas_pos;
		int atlas_node_id;
		int last_update;

		ListNode main_node{}; // shared by two lists: m_main_list, m_free_list
		ListNode atlas_node{}; // shared by two lists: AtlasNode::list, m_atlas_queue
	};

	static constexpr int node_size = 256;

	struct AtlasNode {
		IRect rect;
		List list{};
	};

	VulkanDevice &m_device;
	PodVector<AtlasNode> m_atlas_nodes;
	PVImageView m_atlas;
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
