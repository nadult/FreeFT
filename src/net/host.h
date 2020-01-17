// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "net/socket.h"
#include "net/chunk.h"
#include "sys/aligned_allocator.h"
#include <fwk/list_node.h>
#include <list>

namespace net {

	struct UChunk {
		int chunk_id;
		int channel_id;
		ListNode node;
	};

	template <class Accessor, class Container>
	int freeListAlloc(Accessor acc, Container &container, List &free_list) {
		int idx;
		if(free_list.empty()) {
			container.emplace_back();
			idx = (int)container.size() - 1;
		} else {
			idx = free_list.head;
			listRemove(acc, free_list, idx);
		}
		return idx;
	}

	template <class Object> class LinkedVector {
	  public:
		typedef pair<ListNode, Object> Elem;
		LinkedVector() : m_list_size(0) {}

		Object &operator[](int idx) { return m_objects[idx].second; }
		const Object &operator[](int idx) const { return m_objects[idx].second; }
		int size() const { return m_objects.size(); }
		int listSize() const { return m_list_size; }


		int alloc() {
			auto acc = [&](int idx) -> ListNode & { return m_objects[idx].first; };

			int idx = freeListAlloc(acc, m_objects, m_free);
			listInsert(acc, m_active, idx);
			m_list_size++;
			return idx;
		}

		void free(int idx) {
			auto acc = [&](int idx) -> ListNode & { return m_objects[idx].first; };

			DASSERT(idx >= 0 && idx < m_objects.size());
			listRemove(acc, m_active, idx);
			listInsert(acc, m_free, idx);
			m_list_size--;
		}

		int next(int idx) const { return m_objects[idx].first.next; }
		int prev(int idx) const { return m_objects[idx].first.prev; }

		int head() const { return m_active.head; }
		int tail() const { return m_active.tail; }

	  protected:
		vector<Elem> m_objects;
		List m_active, m_free;
		int m_list_size;
	};

	class RemoteHost {
	public:
		RemoteHost(const Address &address, int max_bytes_per_frame, int current_id, int remote_id);

		enum {
			max_channels = 8,
			max_unacked_packets = 16, //TODO: max ack time would be better
			max_ack_per_frame = max_unacked_packets * 2,
		};

		struct Packet {
			Packet() :packet_id(0) { }

			SeqNumber packet_id;
			List chunks, uchunks;
		};

		struct Channel {
			Channel() :data_size(0), last_chunk_id(0), last_ichunk_id(0) { }

			List chunks;
			int data_size;
			int last_chunk_id;
			int last_ichunk_id;
		};

		void beginSending(Socket *socket);
		void  enqueChunk(CSpan<char>, ChunkType type, int channel_id);

		// Have to be in sending mode, to enque UChunk
		// TODO: better name, we're not really enquing anything
		bool enqueUChunk(CSpan<char>, ChunkType type, int identifier, int channel_id);
		void finishSending();

		void beginReceiving();
		void receive(InPacket, int timestamp, double time);
		void handlePacket(InPacket& packet);
		void finishReceiving();
		const Chunk *getIChunk();

		const Address address() const { return m_address; }
		bool isVerified() const { return m_is_verified; }
		void verify(bool value) { m_is_verified = value; }

		vector<int> &lostUChunks() { return m_lost_uchunk_indices; }
		int lastTimestamp() const { return m_last_timestamp; }

		int memorySize() const;
		int currentId() const { return m_current_id; }

		double timeout() const;

	protected:
		void sendChunks(int max_channel);

		bool sendChunk(int chunk_idx);
		bool sendUChunk(int chunk_idx, CSpan<char> data, ChunkType type);

		int estimateSize(int data_size) const;
		bool canFit(int data_size) const;
		bool isSending() const { return m_socket != nullptr; }

		int allocChunk();
		int allocUChunk();

		void freeChunk(int idx);
		void freeUChunk(int idx);

		void newPacket(bool is_first);
		void sendPacket();

		void resendPacket(int packet_idx);
		void acceptPacket(int packet_idx);

		Address m_address;
		std::vector<Chunk, AlignedAllocator<Chunk, 128>> m_chunks;
		vector<UChunk> m_uchunks;
		vector<Channel> m_channels;
		vector<int> m_ichunk_indices;
		vector<SeqNumber> m_out_acks, m_in_acks;

		LinkedVector<Packet> m_packets;
		LinkedVector<InPacket> m_in_packets;

		vector<int> m_lost_uchunk_indices;

		List m_free_chunks;
		List m_free_uchunks;
		List m_out_ichunks;

		double m_last_time_received;
		int m_last_timestamp;
		int m_max_bpf;
		int m_current_id, m_remote_id;
		SeqNumber m_out_packet_id, m_in_packet_id;

		//TODO: special rules for un-verified hosts (limit packets per frame, etc.)
		bool m_is_verified;

		// Sending context
		Socket *m_socket;

		// TODO: what if we cross packet_size boundary?
		MemoryStream m_out_packet;
		int m_packet_idx;
		int m_bytes_left;

		// Receiving context
		vector<int> m_current_ichunk_indices;

		friend class LocalHost;
	};

	//TODO: network data verification (so that the game won't crash under any circumstances)
	class LocalHost {
	public:
		enum {
			max_remote_hosts = 32,
			max_unverified_hosts = 4,
		};

		LocalHost(const net::Address &address);

		void receive();

		bool getLobbyPacket(InPacket &out);
		void sendLobbyPacket(CSpan<char>);

		// Returns -1 if cannot add more hosts
		int addRemoteHost(const Address &address, int remote_id);
		const RemoteHost *getRemoteHost(int id) const;
		RemoteHost *getRemoteHost(int id);
		int numRemoteHosts() const { return (int)m_remote_hosts.size(); }
		void removeRemoteHost(int remote_id);

		void beginSending(int remote_id);

		// Reliable chunks will get queued
		// Unreliable will be sent immediately or discarded (false will be returned)
		// Channels with lower id have higher priority;
		// If you are sending both reliable and unreliable packets in the same frame,
		// then you should add reliable packets first, so that they will be sent first
		// if they have higher priority
		void  enqueChunk(CSpan<char> data, ChunkType type, int channel_id);
		bool enqueUChunk(CSpan<char> data, ChunkType type, int identifier, int channel_id);
		
		void finishSending();
		int timestamp() const { return m_timestamp; }

		void printStats() const;

	protected:
		net::Socket m_socket;
		vector<Dynamic<RemoteHost>> m_remote_hosts;
		std::list<InPacket> m_lobby_packets;
		int m_unverified_count, m_remote_count;

		int m_current_id;
		int m_timestamp; //TODO: fix timeout updating
	};

}
