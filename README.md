## getting started

All dependencies of the project is retrieved from github in CMakeLists by FetchContent.

But some tweaks must been made to get them tailored for this project:

- Add `#include <initializer_list>` at the start of `TaskScheduler.h` of enkits.
- Add `#define STB_IMAGE_STATIC` before `nanovg.c` include `stb_image.h'.
