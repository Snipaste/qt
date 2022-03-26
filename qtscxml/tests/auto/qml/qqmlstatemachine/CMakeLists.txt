
#####################################################################
## tst_qqmlstatemachine Test:
#####################################################################

qt_internal_add_test(tst_qqmlstatemachine
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlstatemachine.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::StateMachineQmlPrivate
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlstatemachine CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmlstatemachine CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)

qt_internal_add_resource(tst_qqmlstatemachine "tst_qqmlstatemachine"
    PREFIX
        "/"
    FILES
        "data/signaltransition.qml"
)
