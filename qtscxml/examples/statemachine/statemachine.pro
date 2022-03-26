requires(qtHaveModule(widgets))

TEMPLATE = subdirs
CONFIG += no_docs_target

SUBDIRS = graphicsview \
          statemachine
qtConfig(animation): SUBDIRS += animation
