# Generated from invoke-static.pro.

cmake_minimum_required(VERSION 3.16)
project(invoke-static LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/invoke-static")

find_package(Qt6 COMPONENTS Core Gui Qml Scxml)

qt_add_executable(invoke-static
    invoke-static.cpp
)
set_target_properties(invoke-static PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(invoke-static PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Scxml
)

# Resources:
set_source_files_properties("../invoke-common/Button.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Button.qml"
)
set_source_files_properties("../invoke-common/MainView.qml"
    PROPERTIES QT_RESOURCE_ALIAS "MainView.qml"
)
set_source_files_properties("../invoke-common/SubView.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SubView.qml"
)
set(invoke-static_resource_files
    "../invoke-common/Button.qml"
    "../invoke-common/MainView.qml"
    "../invoke-common/SubView.qml"
    "invoke-static.qml"
)

qt6_add_resources(invoke-static "invoke-static"
    PREFIX
        "/"
    FILES
        ${invoke-static_resource_files}
)


# Statecharts:
qt6_add_statecharts(invoke-static
    ../invoke-common/statemachine.scxml
)

install(TARGETS invoke-static
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
