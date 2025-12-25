# DREAM-VCC Qt Application

set(DREAM_QT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/emulatorwidget.cpp
)

set(DREAM_QT_HEADERS
    include/mainwindow.h
    include/emulatorwidget.h
)

add_executable(dream-vcc
    ${DREAM_QT_SOURCES}
    ${DREAM_QT_HEADERS}
)

target_include_directories(dream-vcc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dream-vcc PRIVATE
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
    target_compile_definitions(dream-vcc PRIVATE GL_SILENCE_DEPRECATION)

    set_target_properties(dream-vcc PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.dream.vcc"
        MACOSX_BUNDLE_BUNDLE_NAME "DREAM-VCC"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()
