#include "sys/frame_allocator.h"
#include "sys/memory.h"

namespace sys {

	size_t FrameAllocator::allocatedBlocks = 0;
	FrameAllocator *FrameAllocator::instance = 0;

#define EXCEPT(func, ...)		ThrowException("FrameAllocator::" func "(): ", __VA_ARGS__)

	FrameAllocator::FrameAllocator(size_t tMaxReserve,size_t tReserve)
		:allocated(0), allocatedAway(0), maxAllocated(0), reserve(tReserve), maxReserve(tMaxReserve) {
		if(instance)
			EXCEPT("FrameAllocator", "Only one FrameAllocator class can be instantiated.");
		instance = this;

		pool = (char*)sys::Alloc(reserve);
		endPool = pool + reserve;
		allocatedBlocks = 0;
	}

	FrameAllocator::FrameAllocator(const FrameAllocator&) {
		EXCEPT("FrameAllocator", "Only one FrameAllocator class can be instantiated.");
	}
	void FrameAllocator::operator=(const FrameAllocator&) {
		EXCEPT("operator=", "You really shouldnt do that...");
	}

	FrameAllocator::~FrameAllocator() {
		if(allocatedBlocks) {
		//	printf("OH SHIT! we will have a problem when someone will try to free these blocks!\n");
		}
		instance = 0;
		sys::Free(pool);
	}

	void FrameAllocator::BeginFrame() {
		MutexLocker locker(mutex);

		if(allocatedBlocks)
			EXCEPT("BeginFrame", "There is still some data allocated from last frame (",
					int(allocatedBlocks), " blocks)");
		allocatedBlocks = 0;

		size_t newReserve = Min(Max(reserve, maxAllocated), maxReserve);
		if(newReserve != reserve) {
			if(pool) sys::Free(pool);
			pool = (char*)sys::Alloc(reserve=newReserve);
			endPool = pool + reserve;
		}
		maxAllocated = 0; allocated = 0; allocatedAway = 0;
	}

	void *FrameAllocator::DoAlloc(size_t bytes) {
		MutexLocker locker(mutex);

		if(allocated + bytes + sizeof(size_t) <= reserve) {
			allocatedBlocks++;

			void *out = pool + allocated;
			((size_t*)out)[0] = bytes;

			allocated += bytes + sizeof(size_t);
			return ((char*)out) + sizeof(size_t);
		}
		
		allocatedAway += bytes;
		return sys::Alloc(bytes);
	}

	void FrameAllocator::DoFree(void *ptr) {
		MutexLocker locker(mutex);
		maxAllocated = Max(maxAllocated, allocated + allocatedAway);

		if(InPool(ptr)) {
			if(allocatedBlocks == 0)
				EXCEPT("FreeFromPool","Trying to free data from empty pool.");

			allocatedBlocks--;
			if(allocatedBlocks == 0) {
				allocated = 0;
				return;
			}

			size_t bytes = ((size_t*)ptr)[-1];

			if(IsLast(ptr, bytes)) allocated -= bytes + sizeof(size_t);
			else {
				// Wasted memory...
			}
		}
		else sys::Free(ptr);
	}

	bool FrameAllocator::IsInPool(void *ptr) {
		MutexLocker locker(mutex);
		return InPool(ptr);
	}
	
	bool FrameAllocator::InPool(void *ptr) const {
		return ((char*)ptr) >= pool && ((char*)ptr) < endPool;
	}
	bool FrameAllocator::IsLast(void *ptr,size_t bytes) const {
		return ((char*)ptr) + bytes >= pool + allocated;
	}


	void *FrameAllocator::Alloc(size_t bytes) {
		if(instance) return instance->DoAlloc(bytes);
		return sys::Alloc(bytes);
	}
	void FrameAllocator::Free(void *ptr) {
		if(instance) {
			instance->DoFree(ptr);
			return;
		}
		if(allocatedBlocks)
			EXCEPT("Free", "We have a problem! You are probably trying to free data allocated in frame allocator, "
					"but the\n frame allocator was destroyed!");
		sys::Free(ptr);
	}

#undef EXCEPT

}

