
#####################################################################
## tst_statemachine Test:
#####################################################################

qt_internal_add_test(tst_statemachine
    SOURCES
        tst_statemachine.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Scxml
        Qt::CorePrivate
        Qt::ScxmlPrivate
        Qt::TestPrivate
)

# Resources:
set(tst_statemachine_resource_files
    "topmachine.scxml"
    "submachineA.scxml"
    "submachineB.scxml"
    "emptylog.scxml"
    "eventoccurred.scxml"
    "historystate.scxml"
    "ids1.scxml"
    "invoke.scxml"
    "multipleinvokableservices.scxml"
    "stateDotDoneEvent.scxml"
    "statenames.scxml"
    "statenamesnested.scxml"
)

qt_internal_add_resource(tst_statemachine "tst_statemachine"
    PREFIX
        "/tst_statemachine"
    FILES
        ${tst_statemachine_resource_files}
)

qt6_add_statecharts(tst_statemachine
    topmachine.scxml
)

#### Keys ignored in scope 1:.:.:statemachine.pro:<TRUE>:
# TEMPLATE = "app"
