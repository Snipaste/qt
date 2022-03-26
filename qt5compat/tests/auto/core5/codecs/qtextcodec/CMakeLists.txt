# Generated from qtextcodec.pro.

#####################################################################
## tst_qtextcodec Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.txt)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qtextcodec
    SOURCES
        tst_qtextcodec.cpp
    PUBLIC_LIBRARIES
        Qt::Core5Compat
    TESTDATA ${test_data}
)
if(UNIX)
    add_subdirectory(echo)
endif()
