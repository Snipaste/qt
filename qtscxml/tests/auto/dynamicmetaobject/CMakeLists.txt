
#####################################################################
## tst_dynamicmetaobject Test:
#####################################################################

qt_internal_add_test(tst_dynamicmetaobject
    SOURCES
        tst_dynamicmetaobject.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Scxml
)

# Resources:
set(tst_dynamicmetaobject_resource_files
    "mediaplayer.scxml"
    "test1.scxml"
)

qt_internal_add_resource(tst_dynamicmetaobject "tst_dynamicmetaobject"
    PREFIX
        "/tst_dynamicmetaobject"
    FILES
        ${tst_dynamicmetaobject_resource_files}
)


#### Keys ignored in scope 1:.:.:dynamicmetaobject.pro:<TRUE>:
# TEMPLATE = "app"
