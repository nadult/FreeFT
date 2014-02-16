/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef FREEFT_AUDIO_SOUND_H
#define FREEFT_AUDIO_SOUND_H

#include "base.h"

namespace audio {

	//TODO: better name
	class Sound
	{
	public:
		enum { max_size = 16 * 1024 * 1024 };

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

	class DSound: public Resource
	{
	public:
		DSound();
		~DSound();
		DSound(DSound&&);
		DSound(const DSound&) = delete;
		void operator=(const DSound&) = delete;

		enum { max_size = 16 * 1024 * 1024 };
		uint id() const { return m_id; }

		//! Serializuje dane z pliku do bufora, serializacja
		//! z bufora do pliku nie jest dostepna
		void save(Stream&) const;
		void load(Stream&);

		void setData(const Sound&);

		double lengthInSeconds() const;
		int frequency() const;
		int bits() const;
		int channels() const;
		int size() const;

		static ResourceMgr<DSound> mgr;

	private:
		uint m_id;
	};

	typedef Ptr<DSound> PSound;

}

#endif
