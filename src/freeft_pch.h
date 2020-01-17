#pragma once

#include <map>
#include <mutex>
#include <random>
#include <algorithm>
#include <fwk/enum.h>
#include <fwk/format.h>
#include <fwk/gfx/color.h>
#include <fwk/gfx_base.h>
#include <fwk/math/box.h>
#include <fwk/math/constants.h>
#include <fwk/math/ray.h>
#include <fwk/math/segment.h>
#include <fwk/math_base.h>
#include <fwk/pod_vector.h>
#include <fwk/sys/xml.h>
#include <fwk/sys/unique_ptr.h>

template <class T>
using Dynamic = fwk::UniquePtr<T>;