// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include <fwk/sys/memory.h>
#include <fwk/sys_base.h>
#include <limits>

template <class T, int alignment = 64> class AlignedAllocator {
  public:
	typedef T value_type;

	template <class U> struct rebind {
		using other = AlignedAllocator<U, alignment>;
	};
	bool operator==(const AlignedAllocator &other) { return true; }

	T *allocate(size_t n) { return (T *)fwk::allocate(n * sizeof(T), alignment); }
	void deallocate(T *ptr, size_t n) { fwk::deallocate(ptr); }
};
