/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "audio/device.h"
#include "audio/source.h"

#ifdef _WIN32
#define AL_LIBTYPE_STATIC
#endif
#include <AL/al.h>


namespace audio
{

	Source::Source() {
		alGetError();
		alGenSources(1, (ALuint*)&m_id);
		int error = alGetError();
		if(error != AL_NO_ERROR)
			THROW("Error while creating audio source: %s", OpenalErrorString(error).c_str());
	}

	Source::Source(Source &&rhs) :m_id(rhs.m_id) {
		rhs.m_id = 0;
	}

	Source::~Source() {
		if(m_id)
			alDeleteBuffers(1, (ALuint*)&m_id);
	}

	void Source::assignBuffer(PSound buffer) {
		if(!buffer) return;

		alGetError();
		alSourcei(m_id, AL_BUFFER, buffer->id());

		int error = alGetError();
		if(error != AL_NO_ERROR)
			THROW("Error while assigning buffer to source: %s", OpenalErrorString(error).c_str());
	}

	void Source::enqueBuffer(PSound buffer) {
		if(!buffer) return;

		alGetError();
		u32 bid = buffer->id();
		alSourceQueueBuffers(m_id, 1, &bid);

		int error = alGetError();
		if(error != AL_NO_ERROR)
			THROW("Error while queing buffer: %s", OpenalErrorString(error).c_str());

		m_queue.push_front(buffer);
	}

	PSound Source::unqueueBuffer() {
		i32 num;
		alGetSourcei(m_id, AL_BUFFERS_PROCESSED, &num);

		if(num <= 0 || m_queue.empty())
			return 0;

		PSound out = m_queue.back();
		m_queue.pop_back();
		u32 bid = out->id();
		alSourceUnqueueBuffers(m_id, 1, &bid);
		return out;
	}

	void Source::play() { alSourcePlay(m_id); }
	void Source::stop() { alSourceStop(m_id); }

	void Source::setGain(float v) { alSourcef(m_id,AL_GAIN,v); }
	void Source::setPitch(float v) { alSourcef(m_id,AL_PITCH,v); }
	void Source::setMaxDistance(float v) { alSourcef(m_id,AL_MAX_DISTANCE,v); }
	void Source::setRefDistance(float v) { alSourcef(m_id,AL_REFERENCE_DISTANCE,v); }
	void Source::setMinGain(float v) { alSourcef(m_id,AL_MIN_GAIN,v); }
	void Source::setMaxGain(float v) { alSourcef(m_id,AL_MAX_GAIN,v); }

	float Source::gain() const { float out; alGetSourcef(m_id,AL_GAIN,&out); return out; }
	float Source::pitch() const { float out; alGetSourcef(m_id,AL_PITCH,&out); return out; }
	float Source::maxDistance() const { float out; alGetSourcef(m_id,AL_MAX_DISTANCE,&out); return out; }
	float Source::refDistance() const { float out; alGetSourcef(m_id,AL_REFERENCE_DISTANCE,&out); return out; }
	float Source::minGain() const { float out; alGetSourcef(m_id,AL_MIN_GAIN,&out); return out; }
	float Source::maxGain() const { float out; alGetSourcef(m_id,AL_MAX_GAIN,&out); return out; }

	const float3 Source::pos() const {
		float x, y, z;
		alGetSource3f(m_id,AL_POSITION, &x, &y, &z);
		return float3(x, y, z);
	}

	const float3 Source::velocity() const {
		float x, y, z;
		alGetSource3f(m_id,AL_VELOCITY, &x, &y, &z);
		return float3(x, y, z);
	}

	void Source::setPos(const float3 &pos) {
		alSource3f(m_id, AL_POSITION, pos.x, pos.y, pos.z);
	}

	void Source::setVelocity(const float3 &vel) {
		alSource3f(m_id, AL_VELOCITY, vel.x, vel.y, vel.z);
	}

	bool Source::isPlaying() const {
		ALint state;
		alGetSourcei(m_id, AL_SOURCE_STATE, &state);
		return state == AL_PLAYING;
	}

}

