SOURCES += main.cpp \
    roundrectitem.cpp \
    flippablepad.cpp \
    padnavigator.cpp \
    splashitem.cpp

HEADERS += \
    roundrectitem.h \
    flippablepad.h \
    padnavigator.h \
    splashitem.h

RESOURCES += \
    padnavigator.qrc

FORMS += \
    form.ui

QT += statemachine widgets
requires(qtConfig(treewidget))
qtHaveModule(opengl): QT += opengl openglwidgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/statemachine/graphicsview/padnavigator
INSTALLS += target

CONFIG += console
