/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_SOUNDS_H
#define GAME_SOUNDS_H

#include "game/enums.h"

namespace game {

	class SoundId {
	public:
		SoundId() :m_id(-1) { }
		SoundId(const char *sound_name);
		operator int() const { return m_id; }

	protected:
		int m_id;
	};

	const SoundId getStepSoundId(StanceId::Type, ArmourClassId::Type, SurfaceId::Type, bool is_heavy);

	DECLARE_ENUM(WeaponSoundId,
		fire_single,
		fire_burst,
		reload,
		out_of_ammo
	);

	//TODO: punches
	const SoundId getWeaponSoundId(const char *prefix, WeaponSoundId::Type);

}

#endif
