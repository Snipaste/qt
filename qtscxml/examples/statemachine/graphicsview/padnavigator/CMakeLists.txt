# Generated from padnavigator.pro.

cmake_minimum_required(VERSION 3.16)
project(padnavigator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/padnavigator")

find_package(Qt6 REQUIRED COMPONENTS Core Gui StateMachine Widgets)
find_package(Qt6 OPTIONAL_COMPONENTS OpenGL OpenGLWidgets)

qt_add_executable(padnavigator
    flippablepad.cpp flippablepad.h
    form.ui
    main.cpp
    padnavigator.cpp padnavigator.h
    roundrectitem.cpp roundrectitem.h
    splashitem.cpp splashitem.h
)
set_target_properties(padnavigator PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(padnavigator PUBLIC
    Qt::Core
    Qt::Gui
    Qt::StateMachine
    Qt::Widgets
)


# Resources:
set(padnavigator_resource_files
    "images/artsfftscope.png"
    "images/blue_angle_swirl.jpg"
    "images/kontact_contacts.png"
    "images/kontact_journal.png"
    "images/kontact_mail.png"
    "images/kontact_notes.png"
    "images/kopeteavailable.png"
    "images/metacontact_online.png"
    "images/minitools.png"
)

qt6_add_resources(padnavigator "padnavigator"
    PREFIX
        "/"
    FILES
        ${padnavigator_resource_files}
)

if(TARGET Qt::OpenGL AND TARGET Qt::OpenGLWidgets)
    target_link_libraries(padnavigator PUBLIC
        Qt::OpenGL
        Qt::OpenGLWidgets
    )
endif()

install(TARGETS padnavigator
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
