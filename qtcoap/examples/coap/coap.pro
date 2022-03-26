TEMPLATE = subdirs

SUBDIRS += consolecoapclient

qtHaveModule(widgets): SUBDIRS += simplecoapclient

qtHaveModule(quick) {
    SUBDIRS += \
        quicksecureclient \
        quickmulticastclient
}
