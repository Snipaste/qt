# Generated from positioning.pro.

if(TARGET Qt::Widgets)
    qt_internal_add_example(logfilepositionsource)
endif()
if(TARGET Qt::Quick)
    qt_internal_add_example(satelliteinfo)
endif()
if(TARGET Qt::Quick)
    qt_internal_add_example(geoflickr)
endif()
if(TARGET Qt::Network AND TARGET Qt::Quick)
    qt_internal_add_example(weatherinfo)
endif()
