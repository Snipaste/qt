# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_scxmlqml
    GUI
    QMLTEST
    SOURCES
        tst_scxmlqml.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

# Resources:
set(tst_scxmlqml_resource_files
    "statemachine.scxml"
)

qt_internal_add_resource(tst_scxmlqml "tst_scxmlqml"
    PREFIX
        "/"
    FILES
        ${tst_scxmlqml_resource_files}
)
