cmake_minimum_required(VERSION 3.0)
project(Infinity_Express LANGUAGES C CXX)

# See https://cmake.org/cmake/help/latest/policy/CMP0054.html
cmake_policy(SET CMP0054 NEW)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Directory for easier includes
# Anywhere you see include(...) you can check <root>/cmake for that file
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# -------------------------------------------------------------------------------
# LIBOPUS_IS_MAIN determines whether the project is being used from root
# or if it is added as a dependency (through add_subdirectory for example).
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(IS_MAIN_ TRUE)
else ()
    set(IS_MAIN_ FALSE)
endif ()

set(EXE_ Infinity_Express)

# -------------------------------------------------------------------------------
option(PROFILE_ "Turn ON to use Tracy to profile library and other." OFF)
option(STRICT_CODE_ "" ON)

# -------------------------------------------------------------------------------
include(FetchContent)

macro(FETCH name src commit)
    FetchContent_Declare(
            ${name}
            GIT_REPOSITORY ${src}
            GIT_TAG ${commit}
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(${name})
endmacro()

# third party libraries.
add_subdirectory(modules/predicates) # predicates (libigl).
#fetch(sfml https://github.com/SFML/SFML.git 2.5.1)
#fetch(box2d https://github.com/erincatto/box2d.git v2.4.1)
add_subdirectory(modules/box2c)
#fetch(enkits https://github.com/dougbinks/enkiTS.git v1.11)
fetch(entt https://github.com/skypjack/entt.git v3.12.2)
fetch(glm https://github.com/g-truc/glm.git 0.9.9.8)
fetch(tracy https://github.com/wolfpld/tracy.git v0.10)
fetch(libtess2 https://github.com/memononen/libtess2.git master)
fetch(imgui https://github.com/ocornut/imgui.git v1.90)

# -------------------------------------------------------------------------------
set(INCLUDES_
        include/utils/AABB.hpp
        include/utils/Array.hpp
        include/utils/BVH.hpp
        include/utils/Color.hpp
        include/utils/Easing.hpp
        include/utils/Flex.hpp
        include/utils/FSM.hpp
        include/utils/GJK.hpp
        include/utils/Heap.hpp
        include/utils/Hilbert.hpp
        include/utils/HSFM2.hpp
        include/utils/IntervalTree.hpp
        include/utils/Lazy.hpp
        include/utils/Linear.hpp
        include/utils/LockLessMultiReadPipe.hpp
        include/utils/Macros.hpp
        include/utils/Memory.hpp
        include/utils/Profile.hpp
        include/utils/Robinhood.hpp
        include/utils/Scene.hpp
        include/utils/Spatial.hpp
        include/utils/TaskScheduler.hpp
        include/utils/Tween.hpp
        )
set(SOURCES_
        src/main/main.cpp

        # utils
        src/utils/gjk/gjk.cpp
        src/utils/gjk/manifold.cpp
        src/utils/gjk/raycast.cpp
        src/utils/gjk/utils.cpp
        src/utils/BVH.cpp
        src/utils/Color.cpp
        src/utils/Flex.cpp
        src/utils/Heap.cpp
        src/utils/Scene.cpp
        src/utils/TaskScheduler.cpp
        src/utils/Tween.cpp

        # nanovg
        modules/nanovgxc/src/nanovg.c

        # imgui
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp

        # libtess2
        ${libtess2_SOURCE_DIR}/Source/bucketalloc.c
        ${libtess2_SOURCE_DIR}/Source/dict.c
        ${libtess2_SOURCE_DIR}/Source/geom.c
        ${libtess2_SOURCE_DIR}/Source/mesh.c
        ${libtess2_SOURCE_DIR}/Source/priorityq.c
        ${libtess2_SOURCE_DIR}/Source/sweep.c
        ${libtess2_SOURCE_DIR}/Source/tess.c

        # glad
        modules/glad/glad.h
        modules/glad/glad.c
        modules/glad/khrplatform.h)
add_executable(${EXE_} ${INCLUDES_} ${SOURCES_})
target_include_directories(${EXE_} PRIVATE include modules)
target_include_directories(${EXE_} PRIVATE
        modules/stb
        modules/nanovgxc/src/
        ${imgui_SOURCE_DIR}/
        ${libtess2_SOURCE_DIR}/Include)
target_link_libraries(${EXE_} PUBLIC glm predicates) # math
target_link_libraries(${EXE_} PUBLIC box2d) # physics
target_link_libraries(${EXE_} PUBLIC # graphics
        opengl32
        glfw
        )
target_link_libraries(${EXE_} PUBLIC enkiTS)

target_compile_definitions(${EXE_} PRIVATE -DINFINITY_EXPRESS_WORKING_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
message(NOTICE "Defined INFINITY_EXPRESS_WORKING_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")

# set warning level
#target_compile_options(${EXE_} PRIVATE
#        $<$<CXX_COMPILER_ID:MSVC>:/W4> # /WX
#        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic> # -Werror
#        )

# profile?
if (LIBOPUS_PROFILE)
    target_compile_definitions(${EXE_} PRIVATE -DTRACY_ENABLE)
    target_link_libraries(${EXE_} PUBLIC tracy)
endif ()

# prefer posix threads.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
target_link_libraries(${EXE_} PUBLIC ${CMAKE_THREAD_LIBS_INIT}) # Link thread lib.
if (CMAKE_USE_PTHREADS_INIT)
    set_target_properties(${EXE_} PROPERTIES CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    set_target_properties(${EXE_} PROPERTIES CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()

# -------------------------------------------------------------------------------
# Read version string from "./VERSION" file and store it in "VERSION_" variable.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_)
    string(STRIP "${VERSION_}" VERSION_)
    target_compile_definitions(${EXE_} PRIVATE -DINFINITY_EXPRESS_VERSION_STR="${VERSION_}")
    message(NOTICE "Defined INFINITY_EXPRESS_VERSION_STR=${VERSION_}")
else ()
    message(FATAL_ERROR "File ${CMAKE_CURRENT_SOURCE_DIR}/VERSION not found")
endif ()
