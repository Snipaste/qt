# Generated from qxml.pro.

#####################################################################
## tst_qxml Test:
#####################################################################

# Collect test data
list(APPEND test_data "0x010D.xml")

qt_internal_add_test(tst_qxml
    SOURCES
        tst_qxml.cpp
    PUBLIC_LIBRARIES
        Qt::Core5Compat
    TESTDATA ${test_data}
)
