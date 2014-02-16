/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef FREEFT_AUDIO_SOURCE_H
#define FREEFT_AUDIO_SOURCE_H

#include "base.h"
#include "audio/buffer.h"
#include <deque>

namespace audio {

	class Source: public RefCounter
	{
	public:
		Source();
		~Source();
		Source(Source&&);
		Source(const Source&) = delete;
		void operator=(const Source&) = delete;

		inline uint id() const { return m_id; }

		void assignBuffer(PSound);
		void play();
		void stop();

		void enqueBuffer(PSound);
		PSound unqueueBuffer();

		void setPos(const float3&);
		void setVelocity(const float3&);

		const float3 pos() const;
		const float3 velocity() const;

		void setGain(float);
		void setPitch(float);
		void setMaxDistance(float);
		void setRefDistance(float);
		void setMinGain(float);
		void setMaxGain(float);
		
		float gain() const;
		float pitch() const;
		float maxDistance() const;
		float refDistance() const;
		float minGain() const;
		float maxGain() const;

		bool isPlaying() const;

	private:
		std::deque<PSound> m_queue;
		u32 m_id;
	};
	
	typedef Ptr<Source> PSource;

}

#endif
