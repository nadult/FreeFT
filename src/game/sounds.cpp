/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/sounds.h"
#include "audio/device.h"

namespace game {

	SoundId::SoundId(const char *sound_name) :m_id(audio::findSound(sound_name)) { }

	DEFINE_STRINGS(WeaponSoundId,
		"single",
		"burst",
		"reload",
		"outofammo"
	)
	
	const SoundId getWeaponSoundId(const char *prefix, WeaponSoundId::Type type) {
		char name[256];
		snprintf(name, sizeof(name), "%s%s", prefix, WeaponSoundId::toString(type));
		return SoundId(name);
	}

}
