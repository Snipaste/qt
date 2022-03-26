# Generated from qtcoap.pro.

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtCoap
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Coap Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX # special case
)

# Make sure we only use latest private CMake API, aka no compatibility wrappers.
set(QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS TRUE)

# Make sure we use the fixed BASE argument of qt_add_resource.
set(QT_USE_FIXED_QT_ADD_RESOURCE_BASE TRUE)

# special case begin
find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Network)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Gui Widgets Quick Qml)
# special case end

qt_build_repo()
