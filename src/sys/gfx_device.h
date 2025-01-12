// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "base.h"
#include "config.h"

#include <fwk/gfx/shader_compiler.h>

struct GfxDevice {
	static Ex<GfxDevice> create(ZStr name, const Config &);
	Ex<> drawFrame(Canvas2D &);

	Dynamic<ShaderCompiler> compiler;
	VInstanceRef instance_ref;
	VDeviceRef device_ref;
	VWindowRef window_ref;
};
