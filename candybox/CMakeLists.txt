cmake_minimum_required(VERSION 3.14)
project(candybox LANGUAGES C CXX)

set(CANDYBOX_HEADERS
        include/candybox/AABB.hpp
        include/candybox/assert.hpp
        include/candybox/bitwise_enum.hpp
        include/candybox/bresenham.hpp
        include/candybox/BVH.hpp
        include/candybox/color.hpp
        include/candybox/easing.hpp
        include/candybox/filesystem.hpp
        include/candybox/flex.hpp
        include/candybox/fontstash.hpp
        include/candybox/FSM.hpp
        include/candybox/GJK.hpp
        include/candybox/greatest.h
        include/candybox/Heap.hpp
        include/candybox/Hilbert.hpp
        include/candybox/HSFM2.hpp
        include/candybox/IntervalTree.hpp
        include/candybox/json.hpp
        include/candybox/json_fwd.hpp
        include/candybox/Lazy.hpp
        include/candybox/linear.hpp
        include/candybox/LockLessMultiReadPipe.hpp
        include/candybox/Macros.hpp
        include/candybox/Memory.hpp
        include/candybox/nanosvg.h
        include/candybox/NonCopyable.hpp
        include/candybox/optional.hpp
        include/candybox/profile.hpp
        include/candybox/Robinhood.hpp
        include/candybox/Scene.hpp
        include/candybox/simplify_path.hpp
        include/candybox/smart_ptr.hpp
        include/candybox/spatial.hpp
        include/candybox/TaskScheduler.hpp
        include/candybox/Tween.hpp
        include/candybox/vector.hpp

        # vg
        include/candybox/vg/VG.hpp
        include/candybox/vg/VG_gl.hpp
        include/candybox/vg/VG_gl_utils.hpp
        include/candybox/vg/VG_sw.hpp
        include/candybox/vg/VG_sw_utils.hpp
        include/candybox/vg/VG_vtex.hpp

        # glad
        externs/candybox/glad/glad.h
        externs/candybox/glad/khrplatform.h
)
set(CANDYBOX_SOURCES

        # main
        sources/gjk/gjk.cpp
        sources/gjk/manifold.cpp
        sources/gjk/raycast.cpp
        sources/gjk/utils.cpp
        sources/BVH.cpp
        sources/Color.cpp
        sources/flex.cpp
        sources/Heap.cpp
        sources/Scene.cpp
        sources/simplify.cpp
        sources/spatial.cpp
        sources/VG.cpp
        sources/TaskScheduler.cpp
        sources/Tween.cpp
        sources/VG.cpp
        sources/VG_gl_utils.cpp

        # glad
        externs/candybox/glad/glad.c

        # imgui
        externs/candybox/imgui/imgui.cpp
        externs/candybox/imgui/imgui_demo.cpp
        externs/candybox/imgui/imgui_draw.cpp
        externs/candybox/imgui/imgui_tables.cpp
        externs/candybox/imgui/imgui_widgets.cpp
        externs/candybox/imgui/backends/imgui_impl_glfw.cpp
        externs/candybox/imgui/backends/imgui_impl_opengl3.cpp)
add_library(candybox STATIC ${CANDYBOX_HEADERS} ${CANDYBOX_SOURCES})
target_include_directories(candybox PUBLIC include/ externs/ PRIVATE externs/candybox/imgui)
add_subdirectory(externs/candybox/glm) # tag 0.9.9.8
target_link_libraries(candybox PUBLIC glm opengl32 glfw3)
set_target_properties(candybox PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON)

# test
add_subdirectory(externs/candybox/catch2)
add_subdirectory(tests)
