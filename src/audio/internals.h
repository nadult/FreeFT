// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#ifdef _WIN32
#define AL_LIBTYPE_STATIC
#endif
#include <AL/alc.h>
#include <AL/al.h>
#include <fwk/audio_base.h>

namespace audio {

	const char *errorToString(int id);
	void testError(const char *message);

	void uploadToBuffer(const fwk::Sound &, unsigned buffer_id);

	void tickMusic(double time_delta);
	void freeMusicDevice();
	void initMusicDevice();

}
