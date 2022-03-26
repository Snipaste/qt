
#####################################################################
## tst_qstatemachine Test:
#####################################################################

qt_internal_add_test(tst_qstatemachine
    SOURCES
        tst_qstatemachine.cpp
    LIBRARIES
        Qt::StateMachine
        Qt::StateMachinePrivate
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstatemachine CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
