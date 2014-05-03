/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_LOOP_H
#define IO_LOOP_H

#include "base.h"

namespace io {

	class Loop {
	public:
		virtual ~Loop() = default;
		virtual bool tick(double time_diff) = 0;
	};

	typedef unique_ptr<Loop> PLoop;

}

#endif
