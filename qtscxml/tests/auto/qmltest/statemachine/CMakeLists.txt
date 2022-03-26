# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_statemachineqml
    GUI
    QMLTEST
    SOURCES
        tst_statemachineqml.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)
