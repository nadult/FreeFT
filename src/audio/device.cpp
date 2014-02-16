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
#include "audio/sound.h"
#include "sys/platform.h"

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
	
	namespace {
		
		const char *s_prefix = "data/sounds/";
		const char *s_suffix = ".wav";

		class DSound
		{
		public:
			DSound() :m_id(0), m_variation_count(0) { }
			
			DSound(DSound &&rhs) :m_id(rhs.m_id), m_name(std::move(rhs.m_name)) {
				m_variation_count = rhs.m_variation_count;
				rhs.m_id = 0;
			}

			~DSound() {
				if(m_id)
					alDeleteBuffers(1, &m_id);
			}

			DSound(const DSound&) = delete;
			void operator=(const DSound&) = delete;

			void load() {
				DASSERT(!isRandomDummy());
				DASSERT(!m_name.empty());
				if(isLoaded())
					return;

				Sound sound; {
					char path[1024];
					snprintf(path, sizeof(path), "%s%s%s", s_prefix, m_name.c_str(), s_suffix);
					Loader(path) >> sound;
				}

				DASSERT(sound.bits() == 8 || sound.bits() == 16);
				u32 format = sound.bits() == 8?
					sound.isStereo()? AL_FORMAT_STEREO8  : AL_FORMAT_MONO8 :
					sound.isStereo()? AL_FORMAT_STEREO16 :AL_FORMAT_MONO16;

				if(m_id == 0) {
					alGetError();
					alGenBuffers(1, &m_id);
					int last_error = alGetError();
					if(last_error != AL_NO_ERROR)
						THROW("Error while creating audio buffer. %s", OpenalErrorString(last_error).c_str());
				}

				alGetError();
				alBufferData(m_id, format, sound.data(), sound.size(), sound.frequency());
				uint error = alGetError();
				if(error != AL_NO_ERROR) {
					alDeleteBuffers(1, &m_id);
					m_id = 0;
					THROW("Error while loading data to audio::Buffer. %s", OpenalErrorString(error).c_str());
				}
			}

			bool isLoaded() const {
				return m_id != 0;
			}

			bool isRandomDummy() {
				return m_variation_count > 0;
			}

			int m_variation_count;
			ALuint m_id;
			string m_name;
		};

		std::map<string, SoundIndex> s_sound_map;
		vector<DSound> s_sounds;

		ALCdevice *s_device = nullptr;
		ALCcontext *s_context = nullptr;

		uint s_sources[max_sources] = { 0, };
		int s_free_sources[max_sources] = {0, };
		int s_num_free_sources = 0;

		bool s_is_initialized = false;
	}
	
	bool isInitialized() {
		return s_is_initialized;
	}

	void initSoundMap() {
		if(!s_sound_map.empty())
			return;

		s_sounds.clear();

		vector<FileEntry> file_entries;
		vector<std::map<string, SoundIndex>::iterator> iters;

		findFiles(file_entries, s_prefix);
		iters.resize(file_entries.size(), s_sound_map.end());

		string suffix = s_suffix;
		int sound_count = 0;

		for(int n = 0; n < (int)file_entries.size(); n++) {
			string name = (string)file_entries[n].path;
			removePrefix(name, s_prefix);

			string spec_name = toLower(name);

			if(removeSuffix(spec_name, suffix)) {
				name.resize(spec_name.size());
				file_entries[n].path = name;

				while(spec_name.back() >= '0' && spec_name.back() <= '9')
					spec_name.pop_back();

				auto it = s_sound_map.find(spec_name);
				if(it == s_sound_map.end())
					it = s_sound_map.emplace(spec_name, SoundIndex{0, 0}).first;
				it->second.variation_count++;
				iters[n] = it;
				sound_count++;
			}
		}

		sound_count += (int)s_sound_map.size(); // random dummies
		s_sounds.resize(sound_count);
		sound_count = 0;

		for(auto it = s_sound_map.begin(); it != s_sound_map.end(); ++it) {
			it->second.first_idx = sound_count;
			sound_count += it->second.variation_count + 1;
			it->second.variation_count = 0;
		}

		for(int n = 0; n < (int)iters.size(); n++) {
			auto it = iters[n];
			if(it == s_sound_map.end())
				continue;
			SoundIndex &idx = it->second;
			s_sounds[idx.first_idx + ++idx.variation_count].m_name = file_entries[n].path;
		}

		for(auto it = s_sound_map.begin(); it != s_sound_map.end(); ++it)
			s_sounds[it->second.first_idx].m_variation_count = it->second.variation_count;
	}

	//TODO: select output device
	void initDevice() {
		ASSERT(!s_is_initialized);

		try {
			s_device = alcOpenDevice(0);
			if(!s_device)
				THROW("Error in alcOpenDevice");

			s_context = alcCreateContext(s_device, 0);
			if(!s_context)
				THROW("Error in alcCreateContext");
			alcMakeContextCurrent(s_context);

			alGetError();
			alGenSources(max_sources, (ALuint*)s_sources);
			int error = alGetError();
			if(error != AL_NO_ERROR)
				THROW("Error while creating audio source: %s", OpenalErrorString(error).c_str());
	
			initSoundMap();
			s_is_initialized = true;
		}
		catch(...) {
			if(s_context) {
				alcDestroyContext(s_context);
				s_context = nullptr;
			}
			if(s_device) {
				alcCloseDevice(s_device);
				s_device = nullptr;
			}
			throw;
		}
			
		tick();
	}

	void freeDevice() {
		if(!s_is_initialized)
			return;

		alDeleteSources(max_sources, s_sources);
		memset(s_sources, 0, sizeof(s_sources));
		s_num_free_sources = 0;

		s_sound_map.clear();
		s_sounds.clear();

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

	void tick() {
		if(!s_is_initialized)
			return;

		s_num_free_sources = 0;
		for(int n = 0; n < max_sources; n++) {
			ALint state;
			alGetSourcei(s_sources[n], AL_SOURCE_STATE, &state);
			if(state != AL_PLAYING)
				s_free_sources[s_num_free_sources++] = n;
		}
	}

	void printExtensions() {
		if(!s_is_initialized)
			return;

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

	static DSound *accessSound(int sound_id) {
		if(!s_is_initialized || sound_id == -1)
			return nullptr;
		DASSERT(sound_id >= 0 && sound_id < (int)s_sounds.size());
		return &s_sounds[sound_id];
	}

	void loadSound(int sound_id) {
		DSound *sound = accessSound(sound_id);
		if(sound && !sound->isLoaded()) {
			if(sound->isRandomDummy()) {
				for(int i = 0; i < sound->m_variation_count; i++)
					loadSound(sound_id + 1 + i);
			}
			else
				sound->load();
		}
	}

	uint prepSource(int sound_id) {
		DSound *sound = accessSound(sound_id);
		if(!sound)
			return 0;

		if(sound->isRandomDummy()) {
			//TODO: more control over randomization?
			sound_id += 1 + (rand() % sound->m_variation_count);
			sound = accessSound(sound_id);
			DASSERT(sound);
		}

		if(!sound->isLoaded()) {
			loadSound(sound_id);
			if(!sound->isLoaded())
				return 0;
		}

		if(!s_num_free_sources)
			return 0;

		uint source_id = s_sources[s_free_sources[--s_num_free_sources]];
		alSourcei(source_id, AL_BUFFER, sound->m_id);
		return (int)source_id;
	}

	void playSound(int sound_id, const float3 &pos) {
		uint source_id = prepSource(sound_id);
		if(!source_id)
			return;

		alSourcef(source_id, AL_MIN_GAIN, 50.0f);
		alSourcef(source_id, AL_MAX_GAIN, 50.0f);
		alSourcef(source_id, AL_GAIN, 50.0f);
		
		alSource3f(source_id, AL_POSITION, pos.x, pos.y, pos.z);
		alSource3f(source_id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);

		alSourcePlay(source_id);
	}

	void playSound(int sound_id, float volume) {
		uint source_id = prepSource(sound_id);
		if(!source_id)
			return;
		
		alSourcef(source_id, AL_MIN_GAIN, 50.0f);
		alSourcef(source_id, AL_MAX_GAIN, 50.0f);
		alSourcef(source_id, AL_GAIN, 50.0f);
		
		alSource3f(source_id, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(source_id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);

		alSourcePlay(source_id);
	}

	const SoundIndex findSound(const char *locase_name) {
		if(s_sound_map.empty())
			return SoundIndex();

		auto it = s_sound_map.find(locase_name);
		SoundIndex out = it == s_sound_map.end()? SoundIndex() : it->second;
		return out;
	}

	void playSound(const char *locase_name, const float3 &pos) {
		playSound(findSound(locase_name), pos);
	}

	void playSound(const char *locase_name, float volume) {
		playSound(findSound(locase_name), volume);
	}

}

