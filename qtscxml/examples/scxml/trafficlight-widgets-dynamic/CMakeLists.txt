# Generated from trafficlight-widgets-dynamic.pro.

cmake_minimum_required(VERSION 3.16)
project(trafficlight-widgets-dynamic LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/trafficlight-widgets-dynamic")

find_package(Qt6 COMPONENTS Core Gui Widgets Scxml)

qt_add_executable(trafficlight-widgets-dynamic
    ../trafficlight-common/trafficlight.cpp ../trafficlight-common/trafficlight.h
    trafficlight-widgets-dynamic.cpp
)
set_target_properties(trafficlight-widgets-dynamic PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(trafficlight-widgets-dynamic PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Scxml
    Qt6::Widgets
)


# Resources:
set_source_files_properties("../trafficlight-common/background.png"
    PROPERTIES QT_RESOURCE_ALIAS "background.png"
)
set_source_files_properties("../trafficlight-common/green.png"
    PROPERTIES QT_RESOURCE_ALIAS "green.png"
)
set_source_files_properties("../trafficlight-common/pause.png"
    PROPERTIES QT_RESOURCE_ALIAS "pause.png"
)
set_source_files_properties("../trafficlight-common/play.png"
    PROPERTIES QT_RESOURCE_ALIAS "play.png"
)
set_source_files_properties("../trafficlight-common/red.png"
    PROPERTIES QT_RESOURCE_ALIAS "red.png"
)
set_source_files_properties("../trafficlight-common/statemachine.scxml"
    PROPERTIES QT_RESOURCE_ALIAS "statemachine.scxml"
)
set_source_files_properties("../trafficlight-common/yellow.png"
    PROPERTIES QT_RESOURCE_ALIAS "yellow.png"
)
set(trafficlight-widgets-dynamic_resource_files
    "../trafficlight-common/background.png"
    "../trafficlight-common/green.png"
    "../trafficlight-common/pause.png"
    "../trafficlight-common/play.png"
    "../trafficlight-common/red.png"
    "../trafficlight-common/statemachine.scxml"
    "../trafficlight-common/yellow.png"
)

qt6_add_resources(trafficlight-widgets-dynamic "trafficlight-widgets-dynamic"
    PREFIX
        "/"
    FILES
        ${trafficlight-widgets-dynamic_resource_files}
)

install(TARGETS trafficlight-widgets-dynamic
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
