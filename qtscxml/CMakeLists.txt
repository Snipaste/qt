cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtScxml
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Scxml and StateMachine libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# Make sure we only use latest private CMake API, aka no compatibility wrappers.
set(QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS TRUE)

# Make sure we use the fixed BASE argument of qt_add_resource.
set(QT_USE_FIXED_QT_ADD_RESOURCE_BASE TRUE)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Gui Qml Widgets OpenGL OpenGLWidgets Network QuickTest)

qt_internal_define_repo_target_set(qtscxml)
qt_internal_define_repo_target_set(qtscxmlqml DEPENDS Scxml)
qt_internal_define_repo_target_set(qtstatemachine)
qt_internal_define_repo_target_set(qtstatemachineqml DEPENDS StateMachine)
qt_internal_prepare_single_repo_target_set_build()
qt_build_repo()
