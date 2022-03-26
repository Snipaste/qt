# Generated from statemachine.pro.

qt_internal_add_example(factorial)
qt_internal_add_example(pingpong)
if(TARGET Qt::Widgets)
    qt_internal_add_example(eventtransitions)
    qt_internal_add_example(rogue)
    qt_internal_add_example(trafficlight)
    qt_internal_add_example(twowaybutton)
endif()
