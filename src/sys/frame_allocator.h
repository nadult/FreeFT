#ifndef SYS_FRAME_ALLOCATOR_H
#define SYS_FRAME_ALLOCATOR_H

#include "base.h"
#include <limits>
#include <baselib_threads.h>

namespace sys {

	// Allocator storing data used from one frame to another; Registers maximum
	// memory usage, and automaticly resizes itself so the memory in next frame
	// can be allocated in one block (If you will try to allocate more than 
	// FrameAllocator can hold in internal buffer, memory will be allocated by
	// operator new[] )
	//
	// Use it only for memory allocated and freed during the time of single frame,
	// especially make sure that everything is deallocated before the allocator
	// is destroyed
	//
	// Allocator will throw and exception if you forget to free something before
	// calling BeginFrame()
	//
	// Its thread safe
	class FrameAllocator
	{
	public:
		static FrameAllocator *instance;

		FrameAllocator(size_t tMaxReserve=1024*1024*32,size_t tReserve=16*1024);
		FrameAllocator(const FrameAllocator&);
		void operator=(const FrameAllocator&);
		~FrameAllocator();

		void beginFrame();
		static void *alloc(size_t bytes);
		static void free(void *ptr);

		bool isInPool(void *ptr);
		void *doAlloc(size_t bytes);
		void doFree(void *p);

	private:
		bool inPool(void *ptr) const;
		bool isLast(void *ptr,size_t bytes) const;

	//	Mutex mutex;
		static size_t allocatedBlocks;

		char *pool,*endPool;
		size_t allocated,allocatedAway;
		size_t maxAllocated;
		size_t reserve,maxReserve;
	};


	template <class T>
	class TFrameAllocator
	{
	public:
		typedef size_t size_type;
		typedef size_t difference_type;
		typedef T* pointer;
		typedef const T* const_pointer;
		typedef T& reference;
		typedef const T& const_reference;
		typedef T value_type;

		template <class U> struct rebind { typedef TFrameAllocator<T> other; };

		template <class U> inline bool operator==(const TFrameAllocator &other) { return true; }
		template <class U> inline bool operator!=(const TFrameAllocator &other) { return false; }

		inline pointer address(reference r) const				{ return &r; }
		inline const_pointer address(const_reference c)			{ return &c; }
		inline size_type max_size() const						{ return std::numeric_limits<size_t>::max() / sizeof(T); }

		inline pointer allocate(size_type n,const void *t=0)	{ return (pointer)FrameAllocator::alloc(n*sizeof(T)); }
		inline void deallocate(pointer p,size_type n) 			{ FrameAllocator::free(p); }
		inline void construct(pointer p,const_reference c)		{ new( reinterpret_cast<void*>(p) ) T(c); }
		inline void destroy(pointer p)							{ (p)->~T(); }
	};

}

#endif

