set(CANDYBOX_HEADERS

        # main
        AABB.hpp
        bitwise_enum.hpp
        bresenham.hpp
        BVH.hpp
        color.hpp
        easing.hpp
        filesystem.hpp
        flex.hpp
        fontstash.hpp
        FSM.hpp
        GJK.hpp
        Heap.hpp
        Hilbert.hpp
        HSFM2.hpp
        IntervalTree.hpp
        json.hpp
        json_fwd.hpp
        Lazy.hpp
        Linear.hpp
        LockLessMultiReadPipe.hpp
        Macros.hpp
        Memory.hpp
        nanosvg.h
        NonCopyable.hpp
        optional.hpp
        profile.hpp
        ref_ptr.hpp
        Robinhood.hpp
        Scene.hpp
        simplify_path.hpp
        smart_ptr.hpp
        spatial.hpp
        TaskScheduler.hpp
        Tween.hpp
        vector.hpp
        VG.hpp
        VG_gl.hpp
        VG_gl_utils.hpp
        VG_sw.hpp
        VG_sw_utils.hpp
        VG_vtex.hpp

        # glad
        glad/glad.h
        glad/khrplatform.h
        )
set(CANDYBOX_SOURCES

        # main
        _sources/gjk/gjk.cpp
        _sources/gjk/manifold.cpp
        _sources/gjk/raycast.cpp
        _sources/gjk/utils.cpp
        _sources/BVH.cpp
        _sources/Color.cpp
        _sources/Flex.cpp
        _sources/Heap.cpp
        _sources/Scene.cpp
        _sources/simplify.cpp
        _sources/spatial.cpp
        _sources/VG.cpp
        _sources/TaskScheduler.cpp
        _sources/Tween.cpp
        _sources/VG.cpp
        _sources/VG_gl_utils.cpp

        # glad
        glad/glad.c

        # imgui
        externs/imgui/imgui.cpp
        externs/imgui/imgui_demo.cpp
        externs/imgui/imgui_draw.cpp
        externs/imgui/imgui_tables.cpp
        externs/imgui/imgui_widgets.cpp
        externs/imgui/backends/imgui_impl_glfw.cpp
        externs/imgui/backends/imgui_impl_opengl3.cpp
        )
add_library(candybox STATIC ${CANDYBOAX_HEADERS} ${CANDYBOX_SOURCES})
target_include_directories(candybox PUBLIC ../ externs/ PRIVATE externs/imgui)
add_subdirectory(externs/glm) # tag 0.9.9.8
target_link_libraries(candybox PUBLIC glm opengl32 glfw)

# test
add_subdirectory(tests)
