#include "base.h"
#include "sys/memory.h"
#include "sys/frame_allocator.h"
#include <memory.h>

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
			printf("Allocation error (requested bytes: %llu)!\nTODO: write proper out of memory handler.\n",
					(unsigned long long)size);
			exit(0);
		}

		return out;
	}

}

void *frameAlloc(size_t bytes) {
	return sys::frmAlloc.doAlloc(bytes);
}


