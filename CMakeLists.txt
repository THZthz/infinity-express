cmake_minimum_required(VERSION 3.0)
project(Infinity_Express LANGUAGES C CXX)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Directory for easier includes
# Anywhere you see include(...) you can check <root>/cmake for that file
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# -------------------------------------------------------------------------------
if (${CMAKE_GENERATOR} MATCHES "Visual Sudio*")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build/.vs)
    set(LIBRARY_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH})
endif ()

# -------------------------------------------------------------------------------
# determines whether the project is being used from root
# or if it is added as a dependency (through add_subdirectory for example).
if (NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    message(FATAL_ERROR "Should be the root project!!!")
endif ()

set(IE_EXE Infinity_Express)

# -------------------------------------------------------------------------------
option(IE_PROFILE "Turn ON to use Tracy to profile library and other." OFF)
option(IE_STRICT_CODE "" OFF)

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
add_subdirectory(modules/catch2)
add_subdirectory(modules/predicates) # predicates (libigl).
#fetch(sfml https://github.com/SFML/SFML.git 2.5.1)
#fetch(box2d https://github.com/erincatto/box2d.git v2.4.1)
# add_subdirectory(modules/boost)
# add_subdirectory(modules/SFML)
add_subdirectory(modules/box2c) # latest version
add_subdirectory(modules/tracy) # tag v0.10
#add_subdirectory(modules/libtess2) # latest version
#add_subdirectory(modules/imgui) # tag v1.90
add_subdirectory(modules/entt)
#set(glbinding_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules/glbinding)
#add_subdirectory(modules/glbinding)
#add_subdirectory(modules/globjects)

add_subdirectory(candybox)

# -------------------------------------------------------------------------------
set(IE_INCLUDES
        include/CellAutomata.hpp
        include/World.hpp
        include/DebugDraw.hpp
        include/Main.hpp


        )
set(IE_SOURCES
        src/main.cpp
        src/CellAutomata.cpp
        src/DebugDraw.cpp

        # vg_test
        src/vg_test/demo.cpp
        src/vg_test/perf.cpp

        # libtess2
        modules/libtess2/Source/bucketalloc.c
        modules/libtess2/Source/dict.c
        modules/libtess2/Source/geom.c
        modules/libtess2/Source/mesh.c
        modules/libtess2/Source/priorityq.c
        modules/libtess2/Source/sweep.c
        modules/libtess2/Source/tess.c

        # clipper2
        #        modules/clipper2_1.3.0/src/clipper.engine.cpp
        #        modules/clipper2_1.3.0/src/clipper.offset.cpp
        #        modules/clipper2_1.3.0/src/clipper.rectclip.cpp
)
add_executable(${IE_EXE} ${IE_INCLUDES} ${IE_SOURCES})
target_include_directories(${IE_EXE} PRIVATE include modules)
target_include_directories(${IE_EXE} PRIVATE
        modules/libtess2/Include)
target_link_libraries(${IE_EXE} PUBLIC predicates box2d candybox)
#target_link_libraries(${IE_EXE} PRIVATE boost)

target_compile_definitions(${IE_EXE} PRIVATE -DINFINITY_EXPRESS_WORKING_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
message(NOTICE "Defined INFINITY_EXPRESS_WORKING_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")

# set warning level
if (IE_STRICT_CODE)
    target_compile_options(${IE_EXE} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
            )
endif ()
if (MSVC)
    target_compile_options(${IE_EXE} PRIVATE
            "/wd4819;"
            "/wd4996;" # This function or variable may be unsafe. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
            "/wd4244;"
            "/wd4101;" # unused local variable
            "/wd4305;"
            )
endif ()

# profile
if (IE_PROFILE)
    target_compile_definitions(${IE_EXE} PRIVATE -DTRACY_ENABLE)
    target_link_libraries(${IE_EXE} PUBLIC tracy)
endif ()

# prefer posix threads.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
target_link_libraries(${IE_EXE} PUBLIC ${CMAKE_THREAD_LIBS_INIT}) # Link thread lib.
if (CMAKE_USE_PTHREADS_INIT)
    set_target_properties(${IE_EXE} PROPERTIES CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    set_target_properties(${IE_EXE} PROPERTIES CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()

# -------------------------------------------------------------------------------
#find_package(PythonInterp REQUIRED) # find python interpreter
## add command to create the header and source files
#add_custom_command(
#        OUTPUT
#        "${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/gl3w.c"
#        "${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/gl3w.h"
#        "${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/glcorearb.h"
#        "${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/khrplatform.h"
#        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/gl3w_gen.py
#        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gl3w_gen.py
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/
#)
## add pseudo target that depends on the generated files
#add_custom_target(
#        gl3w_gen ALL
#        DEPENDS
#        "${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/gl3w.c"
#        "${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/gl3w.h"
#        "${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/glcorearb.h"
#        "${CMAKE_CURRENT_SOURCE_DIR}/modules/gl3w/khrplatform.h"
#)
## make gl3w target depend on the generator target
#add_dependencies(${IE_EXE} gl3w_gen)

# -------------------------------------------------------------------------------
# Read version string from "./VERSION" file and store it in "VERSION_" variable.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_)
    string(STRIP "${VERSION_}" VERSION_)
    target_compile_definitions(${IE_EXE} PRIVATE -DINFINITY_EXPRESS_VERSION_STR="${VERSION_}")
    message(NOTICE "Defined INFINITY_EXPRESS_VERSION_STR=${VERSION_}")
else ()
    message(FATAL_ERROR "File ${CMAKE_CURRENT_SOURCE_DIR}/VERSION not found")
endif ()
