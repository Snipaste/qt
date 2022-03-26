QT -= gui
QT += network coap

TARGET = testapp

HEADERS += \
    coaphandler.h

SOURCES += main.cpp \
    coaphandler.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/coap/consolecoapclient
INSTALLS += target
