# CutieCoCo Windows Native Application
# Uses Win32 API, DirectX/GDI for rendering

set(CUTIECOCO_WIN_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/win32renderer.cpp
    src/win32audio.cpp
    src/win32input.cpp
)

set(CUTIECOCO_WIN_HEADERS
    include/mainwindow.h
    include/win32renderer.h
    include/win32audio.h
    include/win32input.h
)

# Windows resource file for application icon and manifest
set(CUTIECOCO_WIN_RESOURCES
    resources/cutiecoco.rc
)

add_executable(cutiecoco-win WIN32
    ${CUTIECOCO_WIN_SOURCES}
    ${CUTIECOCO_WIN_HEADERS}
    ${CUTIECOCO_WIN_RESOURCES}
)

target_include_directories(cutiecoco-win PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cutiecoco-win PRIVATE
    cutie-emulation
)

# Windows system libraries
target_link_libraries(cutiecoco-win PRIVATE
    user32
    gdi32
    winmm
    ole32
    comdlg32
)

# DirectX libraries (optional, for hardware acceleration)
# target_link_libraries(cutiecoco-win PRIVATE d3d11 dxgi)

target_compile_definitions(cutiecoco-win PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# Enable UTF-8 source files
if(MSVC)
    target_compile_options(cutiecoco-win PRIVATE /utf-8)
endif()

set_target_properties(cutiecoco-win PROPERTIES
    OUTPUT_NAME "CutieCoCo"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
