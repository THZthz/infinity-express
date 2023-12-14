#pragma once

// Tracy profiler.
#ifdef TRACY_ENABLE
#	include <tracy/Tracy.hpp>
#else
#	define ZoneNamed(x, y)
#	define ZoneScopedN(x)
#	define FrameMarkNamed(x)
#	define FrameMark do { } while (0)
#endif // TRACY_ENABLE

