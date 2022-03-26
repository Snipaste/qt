# Generated from qcoaprequest.pro.

#####################################################################
## qcoaprequest Test:
#####################################################################

qt_internal_add_test(qcoaprequest
    SOURCES
        tst_qcoaprequest.cpp
    PUBLIC_LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
