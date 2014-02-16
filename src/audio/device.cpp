/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "audio/device.h"
#ifdef _WIN32
#define AL_LIBTYPE_STATIC
#endif
#include <AL/alc.h>
#include <AL/al.h>
#include <string.h>
#include "audio/buffer.h"
#include "audio/source.h"

namespace audio
{
	const string vendorName() {
		return string(alGetString(AL_VENDOR));
	}

	const string OpenalErrorString(int id) {
		switch(id) {
#define CASE(e)	case e: return string(#e);
		CASE(AL_NO_ERROR)
		CASE(AL_INVALID_NAME)
		CASE(AL_INVALID_ENUM)
		CASE(AL_INVALID_VALUE)
		CASE(AL_INVALID_OPERATION)
		CASE(AL_OUT_OF_MEMORY)
#undef CASE
		}
		return string("UNKNOWN_ERROR");
	}

	static bool s_is_initialized = false;
	static ALCdevice *s_device = nullptr;
	static ALCcontext *s_context = nullptr;
	static vector<Source> s_sources;

	//TODO: select output device
	void initDevice() {
		ASSERT(!s_is_initialized);

		s_device = alcOpenDevice(0);
		if(!s_device)
			THROW("Error in alcOpenDevice");

		s_context = alcCreateContext(s_device, 0);
		if(!s_context) {
			alcCloseDevice(s_device);
			s_device = nullptr;
			THROW("Error in alcCreateContext");
		}
		alcMakeContextCurrent(s_context);
		
		s_is_initialized = true;
	}

	void freeDevice() {
		if(!s_is_initialized)
			return;

		s_sources.clear();

		if(s_context) {
			alcDestroyContext(s_context);
			s_context = nullptr;
		}
		if(s_device) {
			alcCloseDevice(s_device);
			s_device = nullptr;
		}
		s_is_initialized = false;
	}

	void printExtensions() {
		printf("OpenAL extensions:\n");
		const char *text = alcGetString(s_device, ALC_EXTENSIONS);
		while(*text) {
			putc(*text == ' '? '\n' : *text, stdout);
			text++;
		}
		printf("\n");
	}

	void setListenerPos(const float3 &v) {
		alListener3f(AL_POSITION,v.x, v.y, v.z);
	}

	void setListenerVelocity(const float3 &v) {
		alListener3f(AL_VELOCITY,v.x, v.y, v.z);
	}

	void setListenerOrientation(const float3 &vec) {
		float v[6] = {vec.x, vec.y, vec.z, 0.0f, 1.0f, 0.0f};
		alListenerfv(AL_ORIENTATION, v);
	}

	const float3 listenerPos() {
		float x,y,z;
		alGetListener3f(AL_POSITION, &x, &y, &z);
		return float3(x, y, z);
	}

	const float3 listenerVelocity() {
		float x,y,z;
		alGetListener3f(AL_VELOCITY, &x, &y, &z);
		return float3(x, y, z);
	}

	const float3 listenerOrientation() {
		float v[6];
		alGetListenerfv(AL_ORIENTATION, v);
		return float3(v[0], v[1], v[2]);
	}

	void setUnits(float meter) {
		alSpeedOfSound(meter * 343.3);
	}


	enum { max_sources = 4 };
	
	void playSound(const char *name, const float3 &pos) {
		DASSERT(s_is_initialized);

		PSound sound = DSound::mgr[name];
		if(!sound)
			return;

		int free_idx = -1;
		for(int n = 0; n < (int)s_sources.size(); n++)
			if(!s_sources[n].isPlaying()) {
				free_idx = n;
				break;
			}
		if(free_idx == -1) {
			if(s_sources.size() == max_sources)
				return;
			free_idx = (int)s_sources.size();
			s_sources.emplace_back(std::move(Source()));
		}

		Source &source = s_sources[free_idx];
		source.assignBuffer(sound);
		source.setPos(float3(0, 0, 0));
		source.setVelocity(float3(0, 0, 0));
		source.setGain(50.0f);
		source.setMinGain(50.0f);
		source.setMaxGain(50.0f);
		source.play();
	}

	void playSound(const char *name, float volume) {
		DASSERT(s_is_initialized);
	}
}

