/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#include "net/chunk.h"

namespace net {

	Chunk::Chunk() :m_left_over(nullptr), m_type(ChunkType::invalid), m_data_size(0) {
		DASSERT((long long)this % 128 == 0);
	}

	Chunk::~Chunk() {
		if(m_left_over)
			free(m_left_over);
	}

	Chunk::Chunk(Chunk &&rhs) :m_left_over(rhs.m_left_over), m_node(rhs.m_node), m_chunk_id(rhs.m_chunk_id),
		m_type(rhs.m_type), m_channel_id(rhs.m_channel_id), m_data_size(rhs.m_data_size){
		if(m_data_size)
			memcpy(m_data, rhs.m_data, min((int)m_data_size, (int)sizeof(m_data)));
		rhs.m_left_over = nullptr;
		rhs.m_type = ChunkType::invalid;
		rhs.m_data_size = 0;
		//TODO: what about m_node?
	}

	void Chunk::setParams(ChunkType type, int chunk_id, int channel_id) {
		DASSERT(type != ChunkType::invalid);
		DASSERT(channel_id >= 0 && channel_id <= 255);
		m_type = type;
		m_chunk_id = chunk_id;
		m_channel_id = channel_id;
	}

	void Chunk::setData(const char *data, int data_size) {
		clearData();

		DASSERT(data_size >= 0 && data_size <= PacketInfo::max_size);
		if(data_size > (int)sizeof(m_data)) {
			int left_over_size = data_size - sizeof(m_data);
			m_left_over = (char*)malloc(left_over_size);
			memcpy(m_left_over, data + sizeof(m_data), left_over_size);
		}
		memcpy(m_data, data, min((int)sizeof(m_data), data_size));
		m_data_size = data_size;
	}

	void Chunk::saveData(Stream &out) const {
		out.saveData(m_data, min((int)sizeof(m_data), (int)m_data_size));
		if(m_data_size > (int)sizeof(m_data)) {
			DASSERT(m_left_over);
			out.saveData(m_left_over, m_data_size - (int)sizeof(m_data));
		}
	}

	void Chunk::clearData() {
		if(m_left_over) {
			free(m_left_over);
			m_left_over = nullptr;
		}
		m_data_size = 0;
	}

	InChunk::InChunk(const Chunk &chunk) :Stream(true), m_chunk(chunk) {
		DASSERT(m_chunk.m_type != ChunkType::invalid);
		m_size = chunk.m_data_size;
		m_pos = 0;
	}

	void InChunk::v_load(void *ptr, int count) {
		DASSERT(ptr && count <= m_size - m_pos);

		if((int)m_pos < (int)sizeof(Chunk::m_data)) {
			int tcount = min(count, (int)sizeof(Chunk::m_data) - (int)m_pos);
			memcpy(ptr, m_chunk.m_data + m_pos, tcount);
			count -= tcount;
			m_pos += tcount;
			ptr = (char*)ptr + tcount;
		}
		if(count) {
			DASSERT(m_chunk.m_left_over);
			memcpy(ptr, m_chunk.m_left_over + m_pos - (int)sizeof(Chunk::m_data), count);
			m_pos += count;
		}
	}

}
