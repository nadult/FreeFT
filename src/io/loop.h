/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_LOOP_H
#define IO_LOOP_H

#include "base.h"

namespace io {

	class Loop {
	public:
		Loop() :m_is_closing(false) { }
		virtual ~Loop() = default;
		virtual bool tick(double time_diff) = 0;
		virtual void close() { m_is_closing = true; }

	protected:
		bool m_is_closing;
	};

	typedef unique_ptr<Loop> PLoop;

}

#endif
