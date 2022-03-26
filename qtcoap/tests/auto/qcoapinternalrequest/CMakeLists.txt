# Generated from qcoapinternalrequest.pro.

#####################################################################
## qcoapinternalrequest Test:
#####################################################################

qt_internal_add_test(qcoapinternalrequest
    SOURCES
        tst_qcoapinternalrequest.cpp
    PUBLIC_LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
