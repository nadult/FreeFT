/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NET_CHUNK_H
#define NET_CHUNK_H

#include "net/socket.h"

namespace net {

	enum class ChunkType: char {
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

		actor_order,
	};

	struct Chunk {
		Chunk();
		Chunk(const Chunk&) = delete;
		void operator=(const Chunk&) = delete;
		Chunk(Chunk&&);
		~Chunk();

		void setData(const char *data, int data_size);
		void setParams(ChunkType type, int chunk_id, int channel_id);
		void saveData(Stream&) const;
		void clearData();

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
		enum { header_size = sizeof(m_left_over) + sizeof(m_node) + sizeof(m_chunk_id) +
							 sizeof(m_data_size) + sizeof(m_type) + sizeof(m_channel_id) };

		char m_data[128 - header_size];

		//TODO: make chunks smaller, so that 64 bytes will be enough
		friend class InChunk;
	} __attribute__((aligned(128)));

	static_assert(sizeof(Chunk) == 128, "Chunk is not properly aligned");

	class InChunk :public Stream {
	public:
		InChunk(const Chunk &immutable_chunk);

		bool end() const { return m_pos == m_size; }

		int size() const { return m_size; }
		int chunkId() const { return m_chunk.m_chunk_id; }
		int channelId() const { return m_chunk.m_channel_id; }
		ChunkType type() const { return m_chunk.m_type; }

	protected:
		virtual void v_load(void *ptr, int count) final;

		const Chunk &m_chunk;
	};

}

#endif
