# Generated from sudoku.pro.

cmake_minimum_required(VERSION 3.16)
project(sudoku LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/sudoku")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Scxml)

qt_add_executable(sudoku
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(sudoku PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(sudoku PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Scxml
    Qt::Widgets
)


# Resources:
set(sudoku_resource_files
    "data/nearly-solved-sudoku.data"
    "data/sudoku.data"
    "sudoku.js"
)

qt6_add_resources(sudoku "sudoku"
    PREFIX
        "/"
    FILES
        ${sudoku_resource_files}
)


# Statecharts:
qt6_add_statecharts(sudoku
    sudoku.scxml
)

install(TARGETS sudoku
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
