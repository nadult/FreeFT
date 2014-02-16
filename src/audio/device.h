/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef FREEFT_AUDIO_DEVICE_H
#define FREEFT_AUDIO_DEVICE_H

#include "base.h"

namespace audio {

	const string vendorName();	
	const string OpenalErrorString(int id);

	void initDevice();
	void freeDevice();

	void printExtensions();

	void setListenerPos(const float3&);
	void setListenerVelocity(const float3&);
	void setListenerOrientation(const float3&);

	const float3 listenerPos();
	const float3 listenerVelocity();
	const float3 listenerOrientation();

	void setUnits(float unitsPerMeter);

	void playSound(const char *name, const float3 &pos);
	void playSound(const char *name, float volume);

}

#endif
