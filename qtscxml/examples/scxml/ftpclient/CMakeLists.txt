# Generated from ftpclient.pro.

cmake_minimum_required(VERSION 3.16)
project(ftpclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/scxml/ftpclient")

find_package(Qt6 COMPONENTS Core Scxml Network)

qt_add_executable(ftpclient
    ftpcontrolchannel.cpp ftpcontrolchannel.h
    ftpdatachannel.cpp ftpdatachannel.h
    main.cpp
)
set_target_properties(ftpclient PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(ftpclient PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Scxml
)

# Statecharts:
qt6_add_statecharts(ftpclient
    simpleftp.scxml
)

install(TARGETS ftpclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
