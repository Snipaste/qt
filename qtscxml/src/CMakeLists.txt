
add_subdirectory(scxml)
add_subdirectory(statemachine)
if(TARGET Qt::Qml)
    add_subdirectory(statemachineqml)
    add_subdirectory(scxmlqml)
endif()
add_subdirectory(plugins)
