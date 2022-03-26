# Generated from mediaplayer-widgets-dynamic.pro.

cmake_minimum_required(VERSION 3.16)
project(mediaplayer-widgets-dynamic LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/mediaplayer-widgets-dynamic")

find_package(Qt6 COMPONENTS Core Gui Widgets Scxml)

qt_add_executable(mediaplayer-widgets-dynamic
    ../mediaplayer-common/mainwindow.cpp ../mediaplayer-common/mainwindow.h ../mediaplayer-common/mainwindow.ui
    mediaplayer-widgets-dynamic.cpp
)
set_target_properties(mediaplayer-widgets-dynamic PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(mediaplayer-widgets-dynamic PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Scxml
    Qt6::Widgets
)

# Resources:
set_source_files_properties("../mediaplayer-common/mediaplayer.scxml"
    PROPERTIES QT_RESOURCE_ALIAS "mediaplayer.scxml"
)
set(mediaplayer_resource_files
    "../mediaplayer-common/mediaplayer.scxml"
)

qt6_add_resources(mediaplayer-widgets-dynamic "mediaplayer"
    PREFIX
        "/"
    FILES
        ${mediaplayer_resource_files}
)

install(TARGETS mediaplayer-widgets-dynamic
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
