

#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("quicktemplates2-hover" PRIVATE
    SECTION "Quick Templates 2"
    LABEL "Hover support"
    PURPOSE "Provides support for hover effects."
)
qt_feature("quicktemplates2-multitouch" PRIVATE
    SECTION "Quick Templates 2"
    LABEL "Multi-touch support"
    PURPOSE "Provides support for multi-touch."
)
qt_configure_add_summary_section(NAME "Qt Quick Templates 2")
qt_configure_add_summary_entry(ARGS "quicktemplates2-hover")
qt_configure_add_summary_entry(ARGS "quicktemplates2-multitouch")
qt_configure_end_summary_section() # end of "Qt Quick Templates 2" section
