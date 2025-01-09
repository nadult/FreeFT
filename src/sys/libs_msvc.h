// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "fwk/sys/platform.h"

#ifdef FWK_PLATFORM_MSVC

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "OpenAL32.lib")

#endif
