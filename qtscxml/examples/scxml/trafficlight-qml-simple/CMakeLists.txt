# Generated from trafficlight-qml-simple.pro.

cmake_minimum_required(VERSION 3.16)
project(trafficlight-qml-simple LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/trafficlight-qml-simple")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Scxml)

qt_add_executable(trafficlight-qml-simple
    trafficlight-qml-simple.cpp
)
set_target_properties(trafficlight-qml-simple PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(trafficlight-qml-simple PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Scxml
)


# Resources:
set(trafficlight-qml-simple_resource_files
    "Light.qml"
    "TrafficLight.qml"
)

qt6_add_resources(trafficlight-qml-simple "trafficlight-qml-simple"
    PREFIX
        "/"
    FILES
        ${trafficlight-qml-simple_resource_files}
)


# Statecharts:
qt6_add_statecharts(trafficlight-qml-simple
    ../trafficlight-common/statemachine.scxml
)

install(TARGETS trafficlight-qml-simple
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
