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

