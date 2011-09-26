#include "base.h"
#include "sys/memory.h"
#include "sys/frame_allocator.h"
#include <memory.h>

namespace sys {

	static FrameAllocator frmAlloc(maxFrameReserve);

	void NextFrame() {
		frmAlloc.BeginFrame();
	}

	void Free(void *p) {
		free(p);
	}

	void *Alloc(size_t size) {
		void *out = malloc(size);

		if(!out) {
			printf("Allocation error!\nTODO: write proper out of memory handler.\n");
			exit(0);
		}

		return out;
	}

}

void *FrameAlloc(size_t bytes) {
	return sys::frmAlloc.DoAlloc(bytes);
}


