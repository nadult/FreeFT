// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "net/socket.h"
#include <fwk/list_node.h>

namespace net {

enum class ChunkType : char {
	// These are reserved for internal use in RemoteHost, it's illegal to use them
	invalid,
	multiple_chunks,
	ack,

	join,
	join_accept,
	join_refuse,
	leave,
	ping,

	level_info,
	level_loaded,

	timestamp,
	entity_full,
	entity_delete,
	entity_update,
	message,
};

struct Chunk {
	Chunk();
	Chunk(const Chunk &) = delete;
	void operator=(const Chunk &) = delete;
	Chunk(Chunk &&);
	~Chunk();

	void setData(CSpan<char>);
	void setParams(ChunkType type, int chunk_id, int channel_id);
	void saveData(MemoryStream &) const;
	void clearData();
	vector<char> data() const;

	int size() const { return (int)m_data_size; }

  protected:
	char *m_left_over;

  public:
	// It can be on one of 3 lists:
	// packet list:  waiting for an ack
	// channel list: waiting to be sent
	// free list: unused chunk
	ListNode m_node;
	int m_chunk_id;
	ChunkType m_type;
	u8 m_channel_id;

  protected:
	u16 m_data_size;
	static constexpr int header_size = sizeof(m_left_over) + sizeof(m_node) + sizeof(m_chunk_id) +
									   sizeof(m_data_size) + sizeof(m_type) + sizeof(m_channel_id);

	char m_data[128 - header_size];

	//TODO: make chunks smaller, so that 64 bytes will be enough
	friend class InChunk;
};

static_assert(sizeof(Chunk) == 128, "Chunk is not properly sized");

class InChunk : public MemoryStream {
  public:
	InChunk(const Chunk &immutable_chunk);

	int chunkId() const { return m_chunk.m_chunk_id; }
	int channelId() const { return m_chunk.m_channel_id; }
	ChunkType type() const { return m_chunk.m_type; }

  protected:
	const Chunk &m_chunk;
};

}
