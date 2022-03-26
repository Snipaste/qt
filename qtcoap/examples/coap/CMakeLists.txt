# Generated from coap.pro.

qt_internal_add_example(consolecoapclient)
if(TARGET Qt::Widgets)
    qt_internal_add_example(simplecoapclient)
endif()
if(TARGET Qt::Quick)
    qt_internal_add_example(quicksecureclient)
    qt_internal_add_example(quickmulticastclient)
endif()
