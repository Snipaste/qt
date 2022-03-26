# Generated from trafficlight-qml-static.pro.

cmake_minimum_required(VERSION 3.16)
project(trafficlight-qml-static LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/trafficlight-qml-static")

find_package(Qt6 COMPONENTS Core Gui Qml Scxml)

qt_add_executable(trafficlight-qml-static
    trafficlight-qml-static.cpp
)
set_target_properties(trafficlight-qml-static PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(trafficlight-qml-static PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Scxml
)


# Resources:
set_source_files_properties("../trafficlight-common/Button.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Button.qml"
)
set_source_files_properties("../trafficlight-common/Lights.ui.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Lights.ui.qml"
)
set_source_files_properties("../trafficlight-common/TrafficLight.qml"
    PROPERTIES QT_RESOURCE_ALIAS "TrafficLight.qml"
)
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
set_source_files_properties("../trafficlight-common/yellow.png"
    PROPERTIES QT_RESOURCE_ALIAS "yellow.png"
)
set(trafficlight-qml-static_resource_files
    "../trafficlight-common/Button.qml"
    "../trafficlight-common/Lights.ui.qml"
    "../trafficlight-common/TrafficLight.qml"
    "../trafficlight-common/background.png"
    "../trafficlight-common/green.png"
    "../trafficlight-common/pause.png"
    "../trafficlight-common/play.png"
    "../trafficlight-common/red.png"
    "../trafficlight-common/yellow.png"
    "trafficlight-qml-static.qml"
)

qt6_add_resources(trafficlight-qml-static "trafficlight-qml-static"
    PREFIX
        "/"
    FILES
        ${trafficlight-qml-static_resource_files}
)


# Statecharts:
qt6_add_statecharts(trafficlight-qml-static
    ../trafficlight-common/statemachine.scxml
)

install(TARGETS trafficlight-qml-static
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
