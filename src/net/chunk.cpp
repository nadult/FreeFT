// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.


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

	void Chunk::setData(CSpan<char> data) {
		clearData();

		DASSERT(data.size() >= 0 && data.size() <= PacketInfo::max_size);
		if(data.size() > (int)sizeof(m_data)) {
			int left_over_size = data.size() - sizeof(m_data);
			m_left_over = (char*)malloc(left_over_size);
			memcpy(m_left_over, data.data() + sizeof(m_data), left_over_size);
		}
		memcpy(m_data, data.data(), min((int)sizeof(m_data), data.size()));
		m_data_size = data.size();
	}

	void Chunk::saveData(MemoryStream &out) const {
		out.saveData(cspan(m_data, min((int)sizeof(m_data), (int)m_data_size)));
		if(m_data_size > (int)sizeof(m_data)) {
			DASSERT(m_left_over);
			out.saveData(cspan(m_left_over, m_data_size - (int)sizeof(m_data)));
		}
	}

	void Chunk::clearData() {
		if(m_left_over) {
			free(m_left_over);
			m_left_over = nullptr;
		}
		m_data_size = 0;
	}

	vector<char> Chunk::data() const {
		PodVector<char> out(m_data_size);
		// TODO: operations on spans are not so convenient...
		copy(out, cspan(m_data, min((int)sizeof(m_data), (int)m_data_size)));
		if(m_data_size > (int)sizeof(m_data)) {
			DASSERT(m_left_over);
			memcpy(out.data(), m_left_over, m_data_size - (int)sizeof(m_data));
		}
		vector<char> vout;
		out.unsafeSwap(vout);
		return vout;
	}


	// TODO: lot's of redundant layers
	InChunk::InChunk(const Chunk &chunk) :MemoryStream(chunk.data()), m_chunk(chunk) {
		DASSERT(m_chunk.m_type != ChunkType::invalid);
	}
}
