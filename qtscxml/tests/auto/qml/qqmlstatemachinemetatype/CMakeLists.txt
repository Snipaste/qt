
#####################################################################
## tst_qqmlstatemachinemetatype Test:
#####################################################################

qt_internal_add_test(tst_qqmlstatemachinemetatype
    SOURCES
        tst_qqmlstatemachinemetatype.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
)

## Scopes:
#####################################################################
