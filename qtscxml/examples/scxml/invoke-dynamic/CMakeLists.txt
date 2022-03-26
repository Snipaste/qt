# Generated from invoke-dynamic.pro.

cmake_minimum_required(VERSION 3.16)
project(invoke-dynamic LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/invoke-dynamic")

find_package(Qt6 COMPONENTS Core Gui Qml Scxml)

qt_add_executable(invoke-dynamic
    invoke-dynamic.cpp
)
set_target_properties(invoke-dynamic PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(invoke-dynamic PUBLIC
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
set_source_files_properties("../invoke-common/statemachine.scxml"
    PROPERTIES QT_RESOURCE_ALIAS "statemachine.scxml"
)
set(invoke-dynamic_resource_files
    "../invoke-common/Button.qml"
    "../invoke-common/MainView.qml"
    "../invoke-common/SubView.qml"
    "../invoke-common/statemachine.scxml"
    "invoke-dynamic.qml"
)

qt6_add_resources(invoke-dynamic "invoke-dynamic"
    PREFIX
        "/"
    FILES
        ${invoke-dynamic_resource_files}
)

install(TARGETS invoke-dynamic
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
