# Generated from calculator-widgets.pro.

cmake_minimum_required(VERSION 3.16)
project(calculator-widgets LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/calculator-widgets")

find_package(Qt6 COMPONENTS Core Gui Widgets Scxml)

qt_add_executable(calculator-widgets
    calculator-widgets.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
)
set_target_properties(calculator-widgets PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(calculator-widgets PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Scxml
    Qt6::Widgets
)

# Statecharts:
qt6_add_statecharts(calculator-widgets
    ../calculator-common/statemachine.scxml
)

install(TARGETS calculator-widgets
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
