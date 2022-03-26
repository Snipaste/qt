TEMPLATE = app

QT += qml quick coap
CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.cpp \
        qmlcoapmulticastclient.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

target.path = $$[QT_INSTALL_EXAMPLES]/coap/quickmulticastclient
INSTALLS += target

HEADERS += \
    qmlcoapmulticastclient.h
