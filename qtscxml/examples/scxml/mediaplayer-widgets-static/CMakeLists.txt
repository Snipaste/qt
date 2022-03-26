# Generated from mediaplayer-widgets-static.pro.

cmake_minimum_required(VERSION 3.16)
project(mediaplayer-widgets-static LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/mediaplayer-widgets-static")

find_package(Qt6 COMPONENTS Core Gui Widgets Scxml)

qt_add_executable(mediaplayer-widgets-static
    ../mediaplayer-common/mainwindow.cpp ../mediaplayer-common/mainwindow.h ../mediaplayer-common/mainwindow.ui
    mediaplayer-widgets-static.cpp
)
set_target_properties(mediaplayer-widgets-static PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(mediaplayer-widgets-static PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Scxml
    Qt6::Widgets
)

# Statecharts:
qt6_add_statecharts(mediaplayer-widgets-static
    ../mediaplayer-common/mediaplayer.scxml
)

install(TARGETS mediaplayer-widgets-static
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
