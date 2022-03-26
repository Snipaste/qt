# Generated from mediaplayer-qml-dynamic.pro.

cmake_minimum_required(VERSION 3.16)
project(mediaplayer-qml-dynamic LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/mediaplayer-qml-dynamic")

find_package(Qt6 COMPONENTS Core Gui Qml Scxml)

qt_add_executable(mediaplayer-qml-dynamic
    mediaplayer-qml-dynamic.cpp
)
set_target_properties(mediaplayer-qml-dynamic PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(mediaplayer-qml-dynamic PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Scxml
)

# Resources:
set_source_files_properties("../mediaplayer-common/Mediaplayer.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Mediaplayer.qml"
)
set_source_files_properties("../mediaplayer-common/mediaplayer.scxml"
    PROPERTIES QT_RESOURCE_ALIAS "mediaplayer.scxml"
)
set(mediaplayer-qml-dynamic_resource_files
    "../mediaplayer-common/Mediaplayer.qml"
    "../mediaplayer-common/mediaplayer.scxml"
    "mediaplayer-qml-dynamic.qml"
)

qt6_add_resources(mediaplayer-qml-dynamic "mediaplayer-qml-dynamic"
    PREFIX
        "/"
    FILES
        ${mediaplayer-qml-dynamic_resource_files}
)

install(TARGETS mediaplayer-qml-dynamic
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
