// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "audio/sound.h"
#include "audio/device.h"
#include "audio/internals.h"
#include "audio/mp3_decoder.h"

namespace audio
{

	//TODO: limit number of streams played at once (for ambient sounds)
	vector<PPlayback> s_playbacks;

	void tickMusic(double time_delta) {
		for(int n = 0; n < (int)s_playbacks.size(); n++) {
			s_playbacks[n]->update(time_delta);
			if(!s_playbacks[n]->isPlaying()) {
				s_playbacks[n--] = s_playbacks.back();
				s_playbacks.pop_back();
			}
		}
	}

	void freeMusicDevice() {
		for(int n = 0; n < (int)s_playbacks.size(); n++)
			s_playbacks[n]->free();
		s_playbacks.clear();
	}

	void initMusicDevice() {

	}

	//TODO: make it robust
	Playback::Playback(const string &file_name, float volume) :m_file_name(file_name) {
		DASSERT(isInitialized());

		memset(m_buffer_ids, 0, sizeof(m_buffer_ids));
		m_source_id = 0;

		{
			alGetError();

			alGenBuffers(max_buffers, (ALuint*)m_buffer_ids);
			testError("Error while creating buffers for music playback.");

			alGenSources(1, (ALuint*)&m_source_id);
			testError("Error while creating source for music playback.");
		
			m_decoder.reset(new MP3Decoder(m_file_name));
			for(int n = 0; n < max_buffers; n++)
				feedMoreData(m_buffer_ids[n]);

			alSource3f(m_source_id, AL_POSITION,		0.0f, 0.0f, 0.0f);
			alSource3f(m_source_id, AL_VELOCITY,		0.0f, 0.0f, 0.0f);
			alSource3f(m_source_id, AL_DIRECTION,		0.0f, 0.0f, 0.0f);
			alSourcef (m_source_id, AL_ROLLOFF_FACTOR,	0.0f);
			alSourcei (m_source_id, AL_SOURCE_RELATIVE, AL_TRUE);
			
			alSourcePlay(m_source_id);
			m_mode = mode_playing;
		}
	}

	Playback::~Playback() {
		free();
	}

	void Playback::stop(float blend_out) {
		if(blend_out <= 0.0f) {
			m_mode = mode_stopped;
			alSourceStop(m_source_id);
		}
		else {
			m_mode = mode_blending_out;
			m_blend_time = blend_out;
			m_blend_pos = 0.0f;
		}
	}

	void Playback::free() {
		m_decoder.reset(nullptr);
		if(m_source_id) {
			alSourceStop(m_source_id);
			alDeleteSources(1, (ALuint*)&m_source_id);
		}
		for(int n = 0; n < max_buffers; n++)
			if(m_buffer_ids[n])
				alDeleteBuffers(1, (ALuint*)(m_buffer_ids + n));
	}

	void Playback::feedMoreData(uint buffer_id) {
		Sound sound;
		m_decoder->decode(sound, m_decoder->bytesPerSecond() / 2);
		uploadToBuffer(sound, buffer_id);
				
		alSourceQueueBuffers(m_source_id, 1, &buffer_id);
		testError("Error while enqueing buffers.");
	}

	void Playback::update(double time_delta) {
		if(!m_source_id)
			return;

		alGetError();
		float blend = 1.0f;

		if(m_mode == mode_blending_out) {
			blend = 1.0f - m_blend_pos / m_blend_time;
			m_blend_pos += time_delta;
			if(m_blend_pos > m_blend_time) {
				m_mode = mode_stopped;
				alSourceStop(m_source_id);
				return;
			}
		}
		else if(m_mode == mode_blending_in) {
			blend = m_blend_pos / m_blend_time;
			m_blend_pos += time_delta;
			if(m_blend_pos > m_blend_time)
				m_mode = mode_playing;
		}

		alSourcef(m_source_id, AL_GAIN, blend);

		ALint num_processed = 0;
    	alGetSourcei(m_source_id, AL_BUFFERS_PROCESSED, &num_processed);

		while(num_processed--) {
			ALuint buffer_id = 0;
			alSourceUnqueueBuffers(m_source_id, 1, &buffer_id);
			testError("Error when dequing buffers.");

			if(!m_decoder->isFinished())
				feedMoreData(buffer_id);
		}

		alGetError();
		
	}

	bool Playback::isPlaying() const {
		ALint param;
		alGetSourcei(m_source_id, AL_SOURCE_STATE, &param);
		return param == AL_PLAYING;
	}

	const PPlayback playMusic(const string &file_name, float volume) {
		if(!isInitialized())
			return PPlayback();

		PPlayback out(new Playback(file_name, volume));
		s_playbacks.emplace_back(out);
		return out;
	}

}
