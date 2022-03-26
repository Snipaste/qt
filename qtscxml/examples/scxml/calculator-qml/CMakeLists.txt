# Generated from calculator-qml.pro.

cmake_minimum_required(VERSION 3.16)
project(calculator-qml LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/calculator-qml")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Scxml)

qt_add_executable(calculator-qml
    calculator-qml.cpp
)
set_target_properties(calculator-qml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(calculator-qml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Scxml
)


# Resources:
set(calculator-qml_resource_files
    "Button.qml"
    "calculator-qml.qml"
)

qt6_add_resources(calculator-qml "calculator-qml"
    PREFIX
        "/"
    FILES
        ${calculator-qml_resource_files}
)


# Statecharts:
qt6_add_statecharts(calculator-qml
    ../calculator-common/statemachine.scxml
)

install(TARGETS calculator-qml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
