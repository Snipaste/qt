# Generated from quickmulticastclient.pro.

cmake_minimum_required(VERSION 3.16)
project(quickmulticastclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/coap/quickmulticastclient")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Coap)

qt_add_executable(quickmulticastclient
    main.cpp
    qmlcoapmulticastclient.cpp qmlcoapmulticastclient.h
)
set_target_properties(quickmulticastclient PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_compile_definitions(quickmulticastclient PUBLIC
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(quickmulticastclient PUBLIC
    Qt::Coap
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(qml_resource_files
    "main.qml"
)

qt6_add_resources(quickmulticastclient "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS quickmulticastclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
