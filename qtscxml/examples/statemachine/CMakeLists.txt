# Generated from widgets.pro.

add_subdirectory(statemachine)

if(TARGET Qt::Widgets)
    add_subdirectory(graphicsview)
    if(QT_FEATURE_animation)
        add_subdirectory(animation)
    endif()
endif()
