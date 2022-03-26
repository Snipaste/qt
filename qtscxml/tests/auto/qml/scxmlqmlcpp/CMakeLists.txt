qt_internal_add_test(tst_scxmlqmlcpp
    SOURCES
        tst_scxmlqmlcpp.cpp
    LIBRARIES
        Qt::Scxml
        Qt::ScxmlQmlPrivate
        Qt::CorePrivate
        Qt::TestPrivate
)

qt_internal_add_resource(tst_scxmlqmlcpp "tst_compiled"
    PREFIX
        "/"
    FILES
        "submachineA.scxml"
        "submachineB.scxml"
        "topmachine.scxml"
        "statemachine.scxml"
        "brokenstatemachine.scxml"
)

qt6_add_statecharts(tst_scxmlqmlcpp
    topmachine.scxml
)
