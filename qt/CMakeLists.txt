# CutieCoCo Qt Application

set(CUTIECOCO_QT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/emulatorwidget.cpp
)

set(CUTIECOCO_QT_HEADERS
    include/mainwindow.h
    include/emulatorwidget.h
)

add_executable(cutiecoco
    ${CUTIECOCO_QT_SOURCES}
    ${CUTIECOCO_QT_HEADERS}
)

target_include_directories(cutiecoco PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cutiecoco PRIVATE
    dream-core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# macOS-specific settings
if(APPLE)
    # Silence OpenGL deprecation warnings on macOS
    target_compile_definitions(cutiecoco PRIVATE GL_SILENCE_DEPRECATION)

    set_target_properties(cutiecoco PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.cutiecoco.app"
        MACOSX_BUNDLE_BUNDLE_NAME "CutieCoCo"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()
