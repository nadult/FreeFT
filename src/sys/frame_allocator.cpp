/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "sys/frame_allocator.h"

size_t FrameAllocator::allocatedBlocks = 0;
FrameAllocator *FrameAllocator::instance = 0;

#define EXCEPT(func, ...) THROW("FrameAllocator::" func "(): " __VA_ARGS__)

FrameAllocator::FrameAllocator(size_t tMaxReserve, size_t tReserve)
	: allocated(0), allocatedAway(0), maxAllocated(0), reserve(tReserve), maxReserve(tMaxReserve) {
	if(instance)
		EXCEPT("FrameAllocator", "Only one FrameAllocator class can be instantiated.");
	instance = this;

	pool = (char *)malloc(reserve);
	endPool = pool + reserve;
	allocatedBlocks = 0;
}

FrameAllocator::FrameAllocator(const FrameAllocator &) {
	EXCEPT("FrameAllocator", "Only one FrameAllocator class can be instantiated.");
}
void FrameAllocator::operator=(const FrameAllocator &) {
	EXCEPT("operator=", "You really shouldnt do that...");
}

FrameAllocator::~FrameAllocator() {
	if(allocatedBlocks) {
		//	printf("OH SHIT! we will have a problem when someone will try to free these blocks!\n");
	}
	instance = 0;
	free(pool);
}

void FrameAllocator::beginFrame() {
	//	MutexLocker locker(mutex);

	if(allocatedBlocks)
		EXCEPT("BeginFrame", "There is still some data allocated from last frame (",
			   int(allocatedBlocks), " blocks)");
	allocatedBlocks = 0;

	size_t newReserve = min(max(reserve, maxAllocated), maxReserve);
	if(newReserve != reserve) {
		if(pool)
			free(pool);
		pool = (char *)malloc(reserve = newReserve);
		endPool = pool + reserve;
	}
	maxAllocated = 0;
	allocated = 0;
	allocatedAway = 0;
}

void *FrameAllocator::doAlloc(size_t bytes) {
	//	MutexLocker locker(mutex);

	if(allocated + bytes + sizeof(size_t) <= reserve) {
		allocatedBlocks++;

		void *out = pool + allocated;
		((size_t *)out)[0] = bytes;

		allocated += bytes + sizeof(size_t);
		return ((char *)out) + sizeof(size_t);
	}

	allocatedAway += bytes;
	return malloc(bytes);
}

void FrameAllocator::doFree(void *ptr) {
	//	MutexLocker locker(mutex);
	maxAllocated = max(maxAllocated, allocated + allocatedAway);

	if(inPool(ptr)) {
		if(allocatedBlocks == 0)
			EXCEPT("FreeFromPool", "Trying to free data from empty pool.");

		allocatedBlocks--;
		if(allocatedBlocks == 0) {
			allocated = 0;
			return;
		}

		size_t bytes = ((size_t *)ptr)[-1];

		if(isLast(ptr, bytes))
			allocated -= bytes + sizeof(size_t);
		else {
			// Wasted memory...
		}
	} else
		free(ptr);
}

bool FrameAllocator::isInPool(void *ptr) {
	// MutexLocker locker(mutex);
	return inPool(ptr);
}

bool FrameAllocator::inPool(void *ptr) const {
	return ((char *)ptr) >= pool && ((char *)ptr) < endPool;
}
bool FrameAllocator::isLast(void *ptr, size_t bytes) const {
	return ((char *)ptr) + bytes >= pool + allocated;
}

void *FrameAllocator::alloc(size_t bytes) {
	if(instance)
		return instance->doAlloc(bytes);
	return malloc(bytes);
}
void FrameAllocator::free(void *ptr) {
	if(instance) {
		instance->doFree(ptr);
		return;
	}
	if(allocatedBlocks)
		EXCEPT(
			"Free",
			"We have a problem! You are probably trying to free data allocated in frame allocator, "
			"but the\n frame allocator was destroyed!");
	free(ptr);
}

#undef EXCEPT
