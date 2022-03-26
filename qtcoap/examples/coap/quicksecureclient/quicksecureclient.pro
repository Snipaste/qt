TEMPLATE = app

QT += qml quick coap
CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
    qmlcoapsecureclient.h

SOURCES += \
    main.cpp \
    qmlcoapsecureclient.cpp

DISTFILES += \
    FilePicker.qml

RESOURCES += qml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/coap/quicksecureclient
INSTALLS += target
