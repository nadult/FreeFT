// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"
#include <fwk_audio.h>

namespace audio {

	class MP3Decoder {
	public:
		MP3Decoder(const string &file_name);
		~MP3Decoder();

		MP3Decoder(const MP3Decoder&) = delete;
		void operator=(const MP3Decoder&) = delete;

		bool decode(fwk::Sound &out, int max_size);
		bool isFinished() const { return m_is_finished; }

		int bytesPerSecond() const;
		
		//TODO: preloading data? (to avoid reading from disk while playing)
		//alternative: move audio device to separate thread
	private:
		Dynamic<FileStream> m_stream;
		void *m_handle = nullptr;
		int m_sample_rate, m_num_channels;
		bool m_is_finished, m_need_data;
	};

}
