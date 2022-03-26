# Generated from mediaplayer-qml-static.pro.

cmake_minimum_required(VERSION 3.16)
project(mediaplayer-qml-static LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/mediaplayer-qml-static")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Scxml)

qt_add_executable(mediaplayer-qml-static
    mediaplayer-qml-static.cpp
)
set_target_properties(mediaplayer-qml-static PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(mediaplayer-qml-static PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Scxml
)


# Resources:
set_source_files_properties("../mediaplayer-common/Mediaplayer.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Mediaplayer.qml"
)
set(mediaplayer-qml-static_resource_files
    "../mediaplayer-common/Mediaplayer.qml"
    "mediaplayer-qml-static.qml"
)

qt6_add_resources(mediaplayer-qml-static "mediaplayer-qml-static"
    PREFIX
        "/"
    FILES
        ${mediaplayer-qml-static_resource_files}
)


# Statecharts:
qt6_add_statecharts(mediaplayer-qml-static
    ../mediaplayer-common/mediaplayer.scxml
)

install(TARGETS mediaplayer-qml-static
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
