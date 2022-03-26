# Generated from qcoapreply.pro.

#####################################################################
## qcoapreply Test:
#####################################################################

qt_internal_add_test(qcoapreply
    SOURCES
        tst_qcoapreply.cpp
    PUBLIC_LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
