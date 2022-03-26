
#####################################################################
## tst_statemachineinfo Test:
#####################################################################

qt_internal_add_test(tst_statemachineinfo
    SOURCES
        tst_statemachineinfo.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::ScxmlPrivate
)

# Resources:
set(tst_statemachineinfo_resource_files
    "statemachine.scxml"
)

qt_internal_add_resource(tst_statemachineinfo "tst_statemachineinfo"
    PREFIX
        "/tst_statemachineinfo"
    FILES
        ${tst_statemachineinfo_resource_files}
)


#### Keys ignored in scope 1:.:.:statemachineinfo.pro:<TRUE>:
# TEMPLATE = "app"
