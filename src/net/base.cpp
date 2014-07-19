/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "net/base.h"

namespace net {

	void TempPacket::v_save(const void *ptr, int count) {
		if(m_pos + count > (int)sizeof(m_data))
			THROW("not enough space in buffer (%d space left, %d needed)", spaceLeft(), (int)count);

		memcpy(m_data + m_pos, ptr, count);
		m_pos += count;
		if(m_pos > m_size)
			m_size = m_pos;
	}

	DEFINE_ENUM(RefuseReason,
		"wrong password",
		"server full"
	);

	void encodeInt3(Stream &sr, const int3 &value) {
		sr.encodeInt(value.x);
		sr.encodeInt(value.y);
		sr.encodeInt(value.z);
	}

	const int3 decodeInt3(Stream &sr) {
		int3 out;
		out.x = sr.decodeInt();
		out.y = sr.decodeInt();
		out.z = sr.decodeInt();
		return out;
	}

	void ServerStatusChunk::save(Stream &sr) const {
		sr.pack(address.ip, address.port, game_mode, is_passworded);
		sr << server_name << map_name;
		sr.encodeInt(num_players);
		sr.encodeInt(max_players);
	}

	void ServerStatusChunk::load(Stream &sr) {
		sr.unpack(address.ip, address.port, game_mode, is_passworded);
		sr >> server_name >> map_name;
		num_players = sr.decodeInt();
		max_players = sr.decodeInt();
	}


	void LevelInfoChunk::save(Stream &sr) const {
		sr << game_mode << map_name;
	}

	void LevelInfoChunk::load(Stream &sr) {
		sr >> game_mode >> map_name;
	}

}
