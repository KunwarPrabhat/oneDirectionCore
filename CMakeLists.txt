cmake_minimum_required(VERSION 3.20)
project(oneDirectionCore VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(BUILD_SHARED_LIBS OFF)

set(GLFW_DIR "${CMAKE_SOURCE_DIR}/3rdparty/glfw")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/3rdparty/imgui")

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(od_core STATIC
    core/driver/capture_windows.c
    core/dsp/classifier.c
    core/dsp/dsp.c
)
target_include_directories(od_core PUBLIC ${CMAKE_SOURCE_DIR})

add_library(od_osd STATIC
    overlay/osd_radar.cpp
    ${IMGUI_SOURCES}
)
target_include_directories(od_osd PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_DIR}/include
)

add_executable(ODC-windows WIN32
    ui/windows/main_windows.cpp
)
target_link_libraries(ODC-windows PRIVATE od_core comctl32)

add_executable(ODC-overlay-win
    overlay/windows/overlay_standalone_win.cpp
)
target_include_directories(ODC-overlay-win PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_DIR}/include
)
target_link_libraries(ODC-overlay-win PRIVATE
    od_core
    od_osd
    ${GLFW_DIR}/lib-vc2022/glfw3.lib
    opengl32
    ole32
    uuid
)
