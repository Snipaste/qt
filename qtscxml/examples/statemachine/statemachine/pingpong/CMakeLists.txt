# Generated from pingpong.pro.

cmake_minimum_required(VERSION 3.16)
project(pingpong LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/statemachine/pingpong")

find_package(Qt6 COMPONENTS StateMachine)
find_package(Qt6 COMPONENTS Core)

qt_add_executable(pingpong
    main.cpp
)
set_target_properties(pingpong PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)
target_link_libraries(pingpong PUBLIC
    Qt::Core
    Qt::StateMachine
)

install(TARGETS pingpong
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
