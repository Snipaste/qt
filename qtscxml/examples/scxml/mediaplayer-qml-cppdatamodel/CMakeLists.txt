# Generated from mediaplayer-qml-cppdatamodel.pro.

cmake_minimum_required(VERSION 3.16)
project(mediaplayer-qml-cppdatamodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/mediaplayer-qml-cppdatamodel")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Scxml)

qt_add_executable(mediaplayer-qml-cppdatamodel
    mediaplayer-qml-cppdatamodel.cpp
    thedatamodel.cpp thedatamodel.h
)
set_target_properties(mediaplayer-qml-cppdatamodel PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(mediaplayer-qml-cppdatamodel PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Scxml
)


# Resources:
set_source_files_properties("../mediaplayer-common/Mediaplayer.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Mediaplayer.qml"
)
set(mediaplayer-qml-cppdatamodel_resource_files
    "../mediaplayer-common/Mediaplayer.qml"
    "mediaplayer-qml-cppdatamodel.qml"
)

qt6_add_resources(mediaplayer-qml-cppdatamodel "mediaplayer-qml-cppdatamodel"
    PREFIX
        "/"
    FILES
        ${mediaplayer-qml-cppdatamodel_resource_files}
)


# Statecharts:
qt6_add_statecharts(mediaplayer-qml-cppdatamodel
    mediaplayer-cppdatamodel.scxml
)

install(TARGETS mediaplayer-qml-cppdatamodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
