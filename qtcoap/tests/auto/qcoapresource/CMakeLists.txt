# Generated from qcoapresource.pro.

#####################################################################
## qcoapresource Test:
#####################################################################

qt_internal_add_test(qcoapresource
    SOURCES
        tst_qcoapresource.cpp
    PUBLIC_LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
