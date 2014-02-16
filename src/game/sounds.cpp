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

	static const char *s_step_stance[StanceId::count] = {
		"stand",
		"stand",
		"prone"
	};

	static const char *s_step_armour[ArmourClassId::count] = {
		"",
		"leath",
		"metal",
		"metal",
		"pow",
	};
	
	const SoundId getStepSoundId(StanceId::Type stance, ArmourClassId::Type armour, SurfaceId::Type surf, bool is_heavy) {
		char name[256];
		snprintf(name, sizeof(name), "%s%s%s%s", s_step_stance[stance],is_heavy? "heavy" : "normal",
				s_step_armour[armour], SurfaceId::toString(surf));
		return SoundId(name);
	}

}
