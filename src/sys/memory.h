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

#ifndef SYS_MEMORY_H
#define SYS_MEMORY_H

namespace sys {

	enum { maxFrameReserve = 1024 * 1024 * 32 };

	// Everything allocated by frame alloc must be freed
	// before calling this function; It will increase the size of
	// the pool so all memory allocated in the last frame would fit in it
	void nextFrame();

	// free / malloc + statistics

	void free(void *ptr);
	void *alloc(size_t bytes);

}

// Quickly allocates memory on the pool (if it fits)
// you just have to be sure that you will deallocate before
// (you can do it normally with delete or delete[])
// calling nextFrame()
void *frameAlloc(size_t bytes);

inline void operator delete(void *p) { sys::free(p); }
inline void operator delete[](void *p) { sys::free(p); }
inline void *operator new(size_t size) { return sys::alloc(size); }
inline void *operator new[](size_t size) { return sys::alloc(size); }

#endif

