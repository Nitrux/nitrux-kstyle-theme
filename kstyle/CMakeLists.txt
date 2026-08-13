include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/animations
    ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(config-nitrux.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-nitrux.h)

set(nitrux_PART_SRCS
    animations/nitruxanimation.cpp
    animations/nitruxanimations.cpp
    animations/nitruxanimationdata.cpp
    animations/nitruxbaseengine.cpp
    animations/nitruxbusyindicatordata.cpp
    animations/nitruxbusyindicatorengine.cpp
    animations/nitruxdialdata.cpp
    animations/nitruxdialengine.cpp
    animations/nitruxenabledata.cpp
    animations/nitruxgenericdata.cpp
    animations/nitruxheaderviewdata.cpp
    animations/nitruxheaderviewengine.cpp
    animations/nitruxscrollbardata.cpp
    animations/nitruxscrollbarengine.cpp
    animations/nitruxspinboxdata.cpp
    animations/nitruxspinboxengine.cpp
    animations/nitruxstackedwidgetdata.cpp
    animations/nitruxstackedwidgetengine.cpp
    animations/nitruxtabbardata.cpp
    animations/nitruxtabbarengine.cpp
    animations/nitruxtoolboxengine.cpp
    animations/nitruxtransitiondata.cpp
    animations/nitruxtransitionwidget.cpp
    animations/nitruxwidgetstatedata.cpp
    animations/nitruxwidgetstateengine.cpp
    nitruxaddeventfilter.cpp
    nitruxhelper.cpp
    nitruxmnemonics.cpp
    nitruxpropertynames.cpp
    nitruxsplitterproxy.cpp
    nitruxstyle.cpp
    nitruxstyleplugin.cpp
    nitruxtoolsareamanager.cpp
    nitruxwindowmanager.cpp
)

kconfig_add_kcfg_files(nitrux_PART_SRCS nitruxstyleconfigdata.kcfgc)
add_library(nitrux6 MODULE ${nitrux_PART_SRCS})
set_target_properties(nitrux6 PROPERTIES
    OUTPUT_NAME nitrux
    AUTOMOC ON
)

ecm_qt_declare_logging_category(nitrux6
    HEADER nitrux_logging.h
    IDENTIFIER NITRUX
    CATEGORY_NAME nitrux
    DEFAULT_SEVERITY Warning
)

target_compile_options(nitrux6 PRIVATE ${NITRUX_ARCH_FLAGS})

target_link_libraries(nitrux6 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::GuiAddons
    KF6::IconThemes
    KF6::ColorScheme
)

if (NITRUX_HAVE_KSTYLE)

    target_link_libraries(nitrux6 PRIVATE KF6::Style)
endif()

install(TARGETS nitrux6 DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/styles)
install(FILES nitrux.themerc DESTINATION ${KDE_INSTALL_DATADIR}/kstyle/themes)
install(FILES nitrux.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
add_subdirectory(config)
