# Generated from qcoapinternalreply.pro.

#####################################################################
## qcoapinternalreply Test:
#####################################################################

qt_internal_add_test(qcoapinternalreply
    SOURCES
        tst_qcoapinternalreply.cpp
    PUBLIC_LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
