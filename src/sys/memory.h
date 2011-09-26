#ifndef SYS_MEMORY_H
#define SYS_MEMORY_H

namespace sys {

	enum { maxFrameReserve = 1024 * 1024 * 32 };

	// Everything allocated by frame alloc must be freed
	// before calling this function; It will increase the size of
	// the pool so all memory allocated in the last frame would fit in it
	void NextFrame();

	// free / malloc + statistics

	void Free(void *ptr);
	void *Alloc(size_t bytes);

}

// Quickly allocates memory on the pool (if it fits)
// you just have to be sure that you will deallocate before
// (you can do it normally with delete or delete[])
// calling NextFrame()
void *FrameAlloc(size_t bytes);

inline void operator delete(void *p) { sys::Free(p); }
inline void operator delete[](void *p) { sys::Free(p); }
inline void *operator new(size_t size) { return sys::Alloc(size); }
inline void *operator new[](size_t size) { return sys::Alloc(size); }

#endif

