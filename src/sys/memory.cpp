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


