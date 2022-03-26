# Generated from simplecoapclient.pro.

cmake_minimum_required(VERSION 3.16)
project(simplecoapclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/coap/simplecoapclient")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Coap)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(simplecoapclient
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    optiondialog.cpp optiondialog.h optiondialog.ui
)
set_target_properties(simplecoapclient PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_compile_definitions(simplecoapclient PUBLIC
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(simplecoapclient PUBLIC
    Qt::Coap
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
)

install(TARGETS simplecoapclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
