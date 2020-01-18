// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"
#include "game/base.h"
#include <fwk/audio_base.h>

namespace audio {

	// All of the functions can be called without initializing the audio device.
	// They won't do anything (or simply return -1 / empty string) in such case.
	//
	// Sound names are case-insensitive.
	//
	// Sounds with the same name modulo numerical suffix are grouped together.
	// For example: empburst1.wav and empburst2.wav.
	//
	//TODO: handle non-existent sounds silently in release build (or log them to a file)
	//TODO: add upper limit for sound buffers memory
	//TODO: add possibility to access specific sound variation (by it's name)

	static constexpr int max_sources = 16;

	using Sound = fwk::Sound;

	bool isInitialized();

	void initSoundMap();
	void initDevice();
	void freeDevice();

	void printInfo();

	void setListener(const float3 &pos, const float3 &vel, const float3 &dir);

	void setUnits(float unitsPerMeter);

	void tick();

	struct SoundIndex {
		SoundIndex() :first_idx(-1), variation_count(0) { }
		SoundIndex(int first_idx, int var_count) :first_idx(first_idx), variation_count(var_count) { }

		void save(MemoryStream&) const;
		void load(MemoryStream&);

		operator int() const { return first_idx; }

		int specificId(int index) const {
			DASSERT(index >= 0 && index < variation_count);
			return first_idx + index + 1;
		}

		int first_idx;
		int variation_count;
	};

	const SoundIndex findSound(const char *locase_name);

	void loadSound(int id);
	void playSound(int id, game::SoundType sound_type, const float3 &pos, const float3 &vel = float3(0.0f, 0.0f, 0.0f));
	void playSound(int id, float volume = 1.0f);
	void playSound(const char *locase_name, float volume = 1.0f);

	class MP3Decoder;

	//TODO: make audio device multithreaded?
	class Playback {
	public:
		Playback(const string &file_name, float volume);
		~Playback();
		
		Playback(const Playback&) = delete;
		void operator=(const Playback&) = delete;

		void stop(float blend_out = 0.0f);
		void update(double time_delta);
		void free();
		
		bool isPlaying() const;

	private:
		static constexpr int max_buffers = 4;

		enum Mode {
			mode_playing,
			mode_blending_in,
			mode_blending_out,
			mode_stopped,
		} m_mode;

		void feedMoreData(uint buffer_id);

		string m_file_name;
		Dynamic<MP3Decoder> m_decoder;
		uint m_buffer_ids[max_buffers];
		uint m_source_id;
		float m_blend_time, m_blend_pos;
	};

	using PPlayback = shared_ptr<Playback>;

	const PPlayback playMusic(const string &file_name, float volume);

}
