/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef AUDIO_SOUND_H
#define AUDIO_SOUND_H

#include "base.h"

namespace audio {

	class Sound
	{
	public:
		Sound();

		void save(Stream&) const;
		void load(Stream&);

		void setData(const char *data, int size, int frequency, int bits, bool is_stereo);
		const char *data() const { return m_data.data(); }

		int size() const { return m_data.size(); }
		int frequency() const { return m_frequency; }
		int bits() const { return m_bits; }
		bool isStereo() const { return m_is_stereo; }

		double lengthInSeconds() const;

	protected:
		PodArray<char> m_data;

		int m_frequency;
		int m_bits;
		bool m_is_stereo;
	};

}

#endif
