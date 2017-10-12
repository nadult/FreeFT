/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "net/base.h"

namespace net {

	void TempPacket::v_save(const void *ptr, int count) {
		if(m_pos + count > (int)sizeof(m_data))
			CHECK_FAILED("not enough space in buffer (%d space left, %d needed)", spaceLeft(), (int)count);

		memcpy(m_data + m_pos, ptr, count);
		m_pos += count;
		if(m_pos > m_size)
			m_size = m_pos;
	}

	static const EnumMap<RefuseReason, const char*> s_descs = {
		"wrong password",
		"nick name already used",
		"server full"
	};

	const char *describe(RefuseReason reason) {
		return s_descs[reason];
	}

	void encodeInt3(Stream &sr, const int3 &value) {
		encodeInt(sr, value.x);
		encodeInt(sr, value.y);
		encodeInt(sr, value.z);
	}

	const int3 decodeInt3(Stream &sr) {
		int3 out;
		out.x = decodeInt(sr);
		out.y = decodeInt(sr);
		out.z = decodeInt(sr);
		return out;
	}

	void ServerStatusChunk::save(Stream &sr) const {
		sr.pack(address.ip, address.port, game_mode, is_passworded);
		sr << server_name << map_name;
		encodeInt(sr, num_players);
		encodeInt(sr, max_players);
	}

	void ServerStatusChunk::load(Stream &sr) {
		sr.unpack(address.ip, address.port, game_mode, is_passworded);
		sr >> server_name >> map_name;
		num_players = decodeInt(sr);
		max_players = decodeInt(sr);
	}


	void LevelInfoChunk::save(Stream &sr) const {
		sr << game_mode << map_name;
	}

	void LevelInfoChunk::load(Stream &sr) {
		sr >> game_mode >> map_name;
	}

}
