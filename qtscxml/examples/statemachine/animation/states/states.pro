QT += statemachine widgets

SOURCES += main.cpp
RESOURCES += states.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/statemachine/animation/states
INSTALLS += target
