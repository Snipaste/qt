# Generated from qcoapclient.pro.

#####################################################################
## qcoapclient Test:
#####################################################################

set (QT_TESTSERVER_COMPOSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../testserver/docker-compose.yml")

qt_internal_add_test(qcoapclient
    EXCEPTIONS # special case
    GUI
    SOURCES
        ../coapnetworksettings.h
        tst_qcoapclient.cpp
    PUBLIC_LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
    QT_TEST_SERVER_LIST "californium" "freecoap"
)

#### Keys ignored in scope 1:.:.:qcoapclient.pro:<TRUE>:
# QT_TEST_SERVER_LIST = "californium" "freecoap"

## Scopes:
#####################################################################

# special case begin
qt_internal_extend_target(qcoapclient CONDITION DEFINED ENV{COAP_TEST_SERVER_IP}
    DEFINES
        COAP_TEST_SERVER_IP=\\\"$ENV{COAP_TEST_SERVER_IP}\\\"
)
# special case end
