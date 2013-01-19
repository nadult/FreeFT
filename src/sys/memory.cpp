/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "base.h"
#include "sys/memory.h"
#include "sys/frame_allocator.h"
#include <memory.h>
#include <cstdio>
#include <cstdlib>

namespace sys {

	static FrameAllocator frmAlloc(maxFrameReserve);

	void nextFrame() {
		frmAlloc.beginFrame();
	}

	void free(void *p) {
		::free(p);
	}

	void *alloc(size_t size) {
		void *out = malloc(size);

		if(!out) {
			//TODO: backtrace
			printf("Allocation error (requested bytes: %lu)!\nTODO: write proper out of memory handler.\n",
					(unsigned long)size);
			exit(0);
		}

		return out;
	}

}

void *frameAlloc(size_t bytes) {
	return sys::frmAlloc.doAlloc(bytes);
}


