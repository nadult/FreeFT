// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "net/host.h"

//#define LOGGING

namespace net {

	namespace {

#ifdef LOGGING
		struct LogInfo {
			bool is_sending;
			Address address;
			int packet_id;
			vector<pair<int, int>> chunks;
			vector<int> acks;
		} static s_log_info;
#endif

		void logBegin(bool is_sending, const Address &address, int packet_id) {
#ifdef LOGGING
			s_log_info.is_sending = is_sending;
			s_log_info.address = address;
			s_log_info.packet_id = packet_id;
			s_log_info.chunks.clear();
			s_log_info.acks.clear();
#endif
		}

		void logChunk(ChunkType chunk_type, int id, int size) {
#ifdef LOGGING
			s_log_info.chunks.push_back(make_pair(id, (int)chunk_type));
#endif
		}

		void logAck(int packet_id) {
#ifdef LOGGING
			s_log_info.acks.push_back(packet_id);
#endif
		}

		void logEnd(int packet_size) {
#ifdef LOGGING
			if(s_log_info.chunks.empty())
				return;

			printf("%s(size:%d id:%d)[ ", s_log_info.is_sending? "OUT" : " IN", packet_size, s_log_info.packet_id);
			for(int n = 0; n < (int)s_log_info.chunks.size(); n++)
				printf("%d:%d ", s_log_info.chunks[n].first, s_log_info.chunks[n].second);
	/*		if(!s_log_info.acks.empty()) {
				printf("ACK: ");
				for(int n = 0; n < (int)s_log_info.acks.size(); n++)
					printf("%d ", s_log_info.acks[n]);
			} */

			printf("]\n");
#endif
		}

	}

#define INSERT(list, id) listInsert<Chunk, &Chunk::m_node>(m_chunks, list, id)
#define REMOVE(list, id) listRemove<Chunk, &Chunk::m_node>(m_chunks, list, id)

	RemoteHost::RemoteHost(const Address &address, int max_bytes_per_frame, int current_id, int remote_id)
		:m_address(address), m_max_bpf(max_bytes_per_frame), m_out_packet_id(-1), m_in_packet_id(-1),
		 m_socket(nullptr), m_current_id(current_id), m_remote_id(remote_id), m_is_verified(false),
		 m_last_timestamp(0), m_out_packet(memorySaver(limits::packet_size)) {
		DASSERT(address.isValid());
		m_channels.resize(max_channels);
		m_last_time_received = getTime();
	}
	
	double RemoteHost::timeout() const {
		return getTime() - m_last_time_received;
	}

	void RemoteHost::sendPacket() {
		logEnd(m_out_packet.size());
		m_socket->send(m_out_packet.data(), m_address);
	}

	void RemoteHost::newPacket(bool is_first) {
		int packet_idx = m_packets.alloc();
		Packet &packet = m_packets[packet_idx];

		m_out_packet_id++;
		m_out_packet.clear();
		m_out_packet << PacketInfo(m_out_packet_id, m_current_id, m_remote_id, is_first? PacketInfo::flag_first : 0);

		packet.packet_id = m_out_packet_id;
		m_packet_idx = packet_idx;
		
		logBegin(true, m_address, m_out_packet_id);
	}

	void RemoteHost::enqueChunk(CSpan<char> data, ChunkType type, int channel_id) {
	//	DASSERT(isSending());
		DASSERT(channel_id >= 0 && channel_id < (int)m_channels.size());
		Channel &channel = m_channels[channel_id];

		int chunk_idx = allocChunk();
		m_chunks[chunk_idx].setData(data);
		m_chunks[chunk_idx].setParams(type, channel.last_chunk_id++, channel_id);
		INSERT(channel.chunks, chunk_idx);
	}

	bool RemoteHost::enqueUChunk(CSpan<char> data, ChunkType type, int identifier, int channel_id) {
		if(!canFit(data.size()))
			return false;

		sendChunks(channel_id - 1);

		DASSERT(isSending());
		DASSERT(channel_id >= 0 && channel_id < (int)m_channels.size());
		Channel &channel = m_channels[channel_id];

		int chunk_idx = allocUChunk();
		UChunk &chunk = m_uchunks[chunk_idx];
		chunk.chunk_id = identifier;
		chunk.channel_id = channel_id;
		return sendUChunk(chunk_idx, data, type);
	}
		
	void RemoteHost::sendChunks(int max_channel) {
		//TODO: this function shouldnt be called every time we try to send an unreliable packet
		DASSERT(max_channel < (int)m_channels.size());

		bool finished = false;

		for(int c = 0; c <= max_channel && !finished; c++) {
			Channel &channel = m_channels[c];

			int chunk_idx = channel.chunks.head;
			while(chunk_idx != -1) {
				Chunk &chunk = m_chunks[chunk_idx];
				int next_idx = chunk.m_node.next;

				if(!sendChunk(chunk_idx)) {
					finished = true;
					break;
				}

				chunk_idx = next_idx;
			}
		}
	}
		
	bool RemoteHost::sendChunk(int chunk_idx) {
		DASSERT(chunk_idx >= 0 && chunk_idx < (int)m_chunks.size());
		Chunk &chunk = m_chunks[chunk_idx];

		if(!canFit(chunk.size()))
			return false;

		if(m_out_packet.capacityLeft() < estimateSize(chunk.size())) {
			sendPacket();
			newPacket(false);
		}

		logChunk(chunk.m_type, chunk.m_chunk_id, chunk.size());

		int prev_pos = m_out_packet.pos();
		m_out_packet << chunk.m_type;
		encodeInt(m_out_packet, chunk.m_chunk_id);
		encodeInt(m_out_packet, 2 * chunk.m_channel_id + 1);
		encodeInt(m_out_packet, chunk.size());
		chunk.saveData(m_out_packet);
		m_bytes_left -= m_out_packet.pos() - prev_pos;

		REMOVE(m_channels[chunk.m_channel_id].chunks, chunk_idx);
		INSERT(m_packets[m_packet_idx].chunks, chunk_idx);

		return true;
	}

	bool RemoteHost::sendUChunk(int chunk_idx, CSpan<char> data, ChunkType type) {
		DASSERT(chunk_idx >= 0 && chunk_idx < (int)m_uchunks.size());
		DASSERT(type != ChunkType::ack && type != ChunkType::invalid && type != ChunkType::multiple_chunks);

		if(!canFit(data.size()))
			return false;

		if(m_out_packet.capacityLeft() < estimateSize(data.size())) {
			sendPacket();
			newPacket(false);
		}

		UChunk &chunk = m_uchunks[chunk_idx];
		logChunk(type, chunk.chunk_id, data.size());

		int prev_pos = m_out_packet.pos();
		m_out_packet << type;
		encodeInt(m_out_packet, chunk.chunk_id);
		encodeInt(m_out_packet, 2 * chunk.channel_id);
		encodeInt(m_out_packet, data.size());
		m_out_packet.saveData(data);
		m_bytes_left -= m_out_packet.pos() - prev_pos;
		listInsert<UChunk, &UChunk::node>(m_uchunks, m_packets[m_packet_idx].uchunks, chunk_idx);

		return true;
	}

	void RemoteHost::beginSending(Socket *socket) {
		DASSERT(socket && !isSending());
		m_socket = socket;

		newPacket(true);

		int num_acks = min((int)max_ack_per_frame, (int)m_out_acks.size());
		for(int n = 0; n < num_acks; n++)
			logAck(m_out_acks[n]);

		encodeInt(m_out_packet, num_acks);
		if(num_acks) {
			m_out_packet << m_out_acks[0];
			for(int i = 1; i < num_acks;) {
				int diff = m_out_acks[i] - m_out_acks[i - 1];
				int count = 1;
				while(i + count < num_acks && int(m_out_acks[i + count]) == int(m_out_acks[i + count - 1]) + 1)
					count++;

				encodeInt(m_out_packet, diff * 2 + (count > 1? 1 : 0));
				if(count > 1)
					encodeInt(m_out_packet, count);
				i += count;
			}

			if(m_out_acks.size() > max_unacked_packets) {
				int drop_acks = (int)m_out_acks.size() - max_unacked_packets;
				m_out_acks.erase(m_out_acks.begin(), m_out_acks.begin() + drop_acks);
			}
		}

		m_bytes_left = m_max_bpf - m_out_packet.pos();
	}
	
	void RemoteHost::finishSending() {
		DASSERT(isSending());
		sendChunks((int)m_channels.size() - 1);
		if(m_out_packet.pos() > PacketInfo::header_size)
			sendPacket();
		m_socket = nullptr;
	}

	void RemoteHost::beginReceiving() {
		m_current_ichunk_indices.clear();
		m_in_acks.clear();
	}

	struct OrderIChunks {
		OrderIChunks(const Chunk *chunks) :chunks(chunks) { }

		bool operator()(int c1, int c2) {
			const Chunk &chunk1 = chunks[c1];
			const Chunk &chunk2 = chunks[c2];

			return  chunk1.m_channel_id == chunk2.m_channel_id?
					chunk1.m_chunk_id   <  chunk2.m_chunk_id :
					chunk1.m_channel_id <  chunk2.m_channel_id;
		}

		const Chunk *chunks;
	};

	void RemoteHost::receive(InPacket packet, int timestamp, double time) {
		m_last_timestamp = timestamp;
		m_last_time_received = time;

		if(m_remote_id == -1)
			m_remote_id = packet.currentId();

		m_out_acks.push_back(packet.packetId());
		int idx = m_in_packets.alloc();
		m_in_packets[idx] = move(packet);
	}

	void RemoteHost::handlePacket(InPacket &packet) {
		if(packet.packetId() < m_in_packet_id)
			return;
		m_in_packet_id = packet.packetId();

		logBegin(false, m_address, packet.packetId()); 

		if(packet.flags() & PacketInfo::flag_first) {
			int num_acks = packet.decodeInt();
			if(num_acks < 0 || num_acks > max_ack_per_frame)
				goto ERROR;

			if(num_acks) {
				int offset = (int)m_in_acks.size();
				m_in_acks.resize(offset + num_acks);
				packet >> m_in_acks[offset];
				for(int i = 1; i < num_acks;) {
					int diff, count;
					diff = packet.decodeInt();
					if(diff & 1)
						count = packet.decodeInt();
					else
						count = 1;
					diff /= 2;

					m_in_acks[offset + i] = SeqNumber(int(m_in_acks[offset + i - 1]) + diff);
					for(int j = 1; j < count; j++)
						m_in_acks[offset + i + j] = SeqNumber(m_in_acks[offset + i + j - 1] + 1);
					i += count;
				}
			
				for(int n = 0; n < num_acks; n++)
					logAck(m_in_acks[offset + n]);
			}

		}


		while(!packet.atEnd()) {
			ChunkType type;
			packet >> type;

			int chunk_id = packet.decodeInt();
			int channel_id = packet.decodeInt();
			bool is_reliable = channel_id & 1;
			channel_id >>= 1;
			
			int data_size = packet.decodeInt();
			logChunk(type, chunk_id, data_size);

			if(data_size < 0 || data_size > PacketInfo::max_size ||
				channel_id < 0 || channel_id >= (int)m_channels.size())
				goto ERROR;

			char data[PacketInfo::max_size];
			packet.loadData({data, data_size});
			//TODO: load directly into chunk

			int chunk_idx = allocChunk();
			Chunk &new_chunk = m_chunks[chunk_idx];
			new_chunk.setData(cspan(data, data_size));
		   	new_chunk.setParams(type, chunk_id, channel_id);
			DASSERT(type != ChunkType::invalid);

			if(is_reliable) {
				m_current_ichunk_indices.push_back(chunk_idx);
			}
			else
				INSERT(m_out_ichunks, chunk_idx);
		}	

ERROR:;
	  //TODO: record error stats and do something if too high?
	  logEnd(packet.size());
	}

	void RemoteHost::finishReceiving() {
		int last_first = -1;

		for(int idx = m_in_packets.head(); idx != -1; idx = m_in_packets.next(idx))
			if(m_in_packets[idx].flags() & PacketInfo::flag_first) {
				if(idx == m_in_packets.tail())
					break;
				last_first = idx;
				break;
			}
 
		for(int idx = m_in_packets.tail(); idx != last_first; ) {
			int next = m_in_packets.prev(idx);
			handlePacket(m_in_packets[idx]);
			m_in_packets.free(idx);
			idx = next;
		}

		vector<int> temp(m_ichunk_indices.size() + m_current_ichunk_indices.size());

		std::sort(m_current_ichunk_indices.begin(), m_current_ichunk_indices.end(), OrderIChunks(m_chunks.data()));
		std::merge(m_ichunk_indices.begin(), m_ichunk_indices.end(),
					m_current_ichunk_indices.begin(), m_current_ichunk_indices.end(), temp.begin());
		temp.swap(m_ichunk_indices);
		m_current_ichunk_indices.clear();

		for(int n = 0; n < (int)m_ichunk_indices.size(); n++) {
			int idx = m_ichunk_indices[n];
			Chunk &chunk = m_chunks[idx];
			Channel &channel = m_channels[chunk.m_channel_id];

			if(channel.last_ichunk_id == chunk.m_chunk_id) {
				channel.last_ichunk_id++;
				INSERT(m_out_ichunks, idx);
				m_ichunk_indices[n] = -1;
			}
		}

		m_ichunk_indices.resize(
			std::remove(m_ichunk_indices.begin(), m_ichunk_indices.end(), -1) - m_ichunk_indices.begin());

		//TODO: make client verification more secure
		
		//TODO: unsafe, theoretically a cycle is possible 
		std::stable_sort(m_in_acks.begin(), m_in_acks.end());
		m_in_acks.resize(std::unique(m_in_acks.begin(), m_in_acks.end()) - m_in_acks.begin());

		int packet_idx = m_packets.tail();
		int ack_idx = 0;
		while(packet_idx != -1 && ack_idx < (int)m_in_acks.size()) {
			SeqNumber acked = m_in_acks[ack_idx];
			Packet &packet = m_packets[packet_idx];

			if(acked == packet.packet_id) {
				int next_idx = m_packets.prev(packet_idx);
				acceptPacket(packet_idx);
				packet_idx = next_idx;
				ack_idx++;
			}
			else if(acked > packet.packet_id) {
				int next_idx = m_packets.prev(packet_idx);
				resendPacket(packet_idx);
				packet_idx = next_idx;
			}
			else
				ack_idx++;
		}

		while(m_packets.listSize() > max_unacked_packets)
			resendPacket(m_packets.tail());	
	}

	void RemoteHost::acceptPacket(int packet_idx) {
		Packet &packet = m_packets[packet_idx];
		int chunk_idx = packet.chunks.head;
		while(chunk_idx != -1) {
			Chunk &chunk = m_chunks[chunk_idx];
			int next_idx = chunk.m_node.next;
			chunk.m_node = ListNode();
			INSERT(m_free_chunks, chunk_idx);
			chunk_idx = next_idx;
		}
		chunk_idx = packet.uchunks.head;
		while(chunk_idx != -1) {
			UChunk &chunk = m_uchunks[chunk_idx];
			int next_idx = chunk.node.next;
			chunk.node = ListNode();
			listInsert<UChunk, &UChunk::node>(m_uchunks, m_free_uchunks, chunk_idx);
			chunk_idx = next_idx;
		}

		packet.uchunks = List();
		packet.chunks = List();
		m_packets.free(packet_idx);
	}
	
	void RemoteHost::resendPacket(int packet_idx) {
		Packet &packet = m_packets[packet_idx];
		int chunk_idx = packet.chunks.head;
		while(chunk_idx != -1) {
			Chunk &chunk = m_chunks[chunk_idx];
			int next_idx = chunk.m_node.next;
			chunk.m_node = ListNode();
			INSERT(m_channels[chunk.m_channel_id].chunks, chunk_idx);
			chunk_idx = next_idx;
		}
		chunk_idx = packet.uchunks.head;
		while(chunk_idx != -1) {
			UChunk &chunk = m_uchunks[chunk_idx];
			int next_idx = chunk.node.next;
			chunk.node = ListNode();
			listInsert<UChunk, &UChunk::node>(m_uchunks, m_free_uchunks, chunk_idx);
			m_lost_uchunk_indices.push_back(chunk.chunk_id);
			chunk_idx = next_idx;
		}

		packet.uchunks = List();
		packet.chunks = List();
		m_packets.free(packet_idx);
	}

	const Chunk *RemoteHost::getIChunk() {
		if(m_out_ichunks.empty())
			return nullptr;

		int idx = m_out_ichunks.tail;
		Chunk *out = &m_chunks[idx];
		DASSERT(out->m_type != ChunkType::invalid);
		REMOVE(m_out_ichunks, idx);
		freeChunk(idx);

		return out;
	}

	bool RemoteHost::canFit(int data_size) const {
		return estimateSize(data_size) <= m_bytes_left;
	}
		
	int RemoteHost::estimateSize(int data_size) const {
		return data_size + 8;
	}
	
	LocalHost::LocalHost(const net::Address &in_address)
	  :m_current_id(-1), m_unverified_count(0), m_timestamp(0) {
		net::Address address = in_address;

		int retries = 0;
		while(!m_socket.isValid()) {
			if(auto result = Socket::make(address))
				m_socket = move(*result);
			else {
				address = Address(address.ip, randomPort());
				if(++retries == 100) {
					result.error().print();
					FATAL("Error while constructing socket.\nTODO: handle it properly"); // TODO: return Ex<> ?
				}
			}
		}
	}

	bool LocalHost::getLobbyPacket(InPacket &out) {
		if(m_lobby_packets.empty())
			return false;
		out = move(m_lobby_packets.front());
		m_lobby_packets.pop_front();
		return true;
	}
		
	void LocalHost::sendLobbyPacket(CSpan<char> data) {
		// TODO: return error? cache address?
		if(auto addr = lobbyServerAddress())
			m_socket.send(data, *addr);
	}

	void LocalHost::receive() {
		double current_time = getTime();

		m_unverified_count = 0;
		for(int n = 0; n < (int)m_remote_hosts.size(); n++) {
			RemoteHost *remote = m_remote_hosts[n].get();
			if(!remote)
				continue;

			if(!remote->isVerified())
				m_unverified_count++;
			remote->beginReceiving();
		}

		InPacket packet;
		while(true) {
			Address source;

			auto result = m_socket.receive(packet, source);
			if(result == RecvResult::empty)
				break;
			if(result == RecvResult::invalid)
				continue;
			
			if(packet.info.flags & PacketInfo::flag_lobby) {
				m_lobby_packets.emplace_back(move(packet));
				continue;
			}

			int remote_id = packet.info.current_id;
			int current_id = packet.info.remote_id;

			if(current_id == -1 && m_unverified_count < max_unverified_hosts) {
				for(int r = 0; r < (int)m_remote_hosts.size(); r++) {
					if(m_remote_hosts[r] && m_remote_hosts[r]->address() == source) {
						current_id = r;
						break;
					}
				}

				//TODO: blacklist filtering?
				current_id = addRemoteHost(source, remote_id);
				m_unverified_count++;
			}

			if(current_id >= 0 && current_id < numRemoteHosts()) {
				RemoteHost *remote = m_remote_hosts[current_id].get();
				if(remote && remote->address() == source)
					m_remote_hosts[current_id]->receive(move(packet), m_timestamp, current_time);
			}
		}

		for(int n = 0; n < (int)m_remote_hosts.size(); n++) {
			RemoteHost *remote = m_remote_hosts[n].get();
			if(remote)
				remote->finishReceiving();
		}
	}

	int LocalHost::addRemoteHost(const Address &address, int remote_id) {
		int idx = -1;
		for(int n = 0; n < (int)m_remote_hosts.size(); n++) {
			if(m_remote_hosts[n]) {
				if(m_remote_hosts[n]->address() == address)
					return n;
			}
			else if(idx == -1)
				idx = n;
		}

		if(idx == -1) {
			if(m_remote_hosts.size() == max_remote_hosts)
				return -1;
			m_remote_hosts.emplace_back();
			idx = (int)m_remote_hosts.size() - 1;
		}

		m_remote_hosts[idx] =
			Dynamic<RemoteHost>(new RemoteHost(address, PacketInfo::max_size * 4, idx, remote_id));
		
		return idx;
	}

	const RemoteHost *LocalHost::getRemoteHost(int id) const {
		DASSERT(id >= 0 && id < numRemoteHosts());
		return m_remote_hosts[id].get();
	}
	
	RemoteHost *LocalHost::getRemoteHost(int id) {
		DASSERT(id >= 0 && id < numRemoteHosts());
		return m_remote_hosts[id].get();
	}

	int RemoteHost::memorySize() const {
		int sum = sizeof(*this);
		sum += sizeof(Chunk) * m_chunks.size();
		sum += sizeof(UChunk) * m_uchunks.size();
		sum += (sizeof(Packet) + sizeof(ListNode)) * m_packets.size();
		sum += (sizeof(InPacket) + sizeof(ListNode)) * m_in_packets.size();
		sum += sizeof(int) * (m_ichunk_indices.size() + m_lost_uchunk_indices.size());
		sum += sizeof(SeqNumber) * (m_out_acks.size() + m_in_acks.size());
		return sum;
	}

#undef INSERT
#undef REMOVE
		
	void LocalHost::removeRemoteHost(int remote_id) {
		DASSERT(remote_id >= 0 && remote_id < numRemoteHosts());
		delete m_remote_hosts[remote_id].release();
	}
		
	void LocalHost::beginSending(int remote_id) {
		DASSERT(m_current_id == -1);
		DASSERT(remote_id >= 0 && remote_id < (int)m_remote_hosts.size());
		DASSERT(m_remote_hosts[remote_id]);

		m_current_id = remote_id;
		m_remote_hosts[remote_id]->beginSending(&m_socket);
	}
		
		
	void LocalHost::enqueChunk(CSpan<char> data, ChunkType type, int channel_id) {
		DASSERT(m_current_id != -1);
		m_remote_hosts[m_current_id]->enqueChunk(data, type, channel_id);
	}

	bool LocalHost::enqueUChunk(CSpan<char> data, ChunkType type, int identifier, int channel_id) {
		DASSERT(m_current_id != -1);
		return m_remote_hosts[m_current_id]->enqueUChunk(data, type, identifier, channel_id);
	}

	void LocalHost::finishSending() {
		DASSERT(m_current_id != -1);
		m_remote_hosts[m_current_id]->finishSending();
		m_current_id = -1;
	}

	void LocalHost::printStats() const {
		int nchunks = 0, data_size = 0;

		for(int n = 0; n < (int)m_remote_hosts.size(); n++) {
			const RemoteHost *host = m_remote_hosts[n].get();
			if(!host)
				continue;

			nchunks += (int)host->m_chunks.size();
			data_size += host->memorySize();
		}

		printf("Network memory info:\n");
		printf("  chunks(%d): %d KB\n", nchunks, (nchunks * (int)sizeof(Chunk)) / 1024);
		printf("  total memory: %d KB\n", data_size / 1024);
	}

}
