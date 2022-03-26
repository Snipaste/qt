TEMPLATE = app

TARGET = tst_rhi_rendercommands

QT += 3dcore 3dcore-private 3drender 3drender-private testlib

CONFIG += testcase

SOURCES += tst_rhi_rendercommands.cpp

include(../../../core/common/common.pri)
include(../../commons/commons.pri)

# Link Against RHI Renderer Plugin
include(../rhi_render_plugin.pri)
