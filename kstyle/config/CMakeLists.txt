### configuration plugin
add_definitions(-DTRANSLATION_DOMAIN=\"nitrux_style_config\")
set(kstyle_nitrux_config_PART_SRCS nitruxstyleconfig.cpp)

set(kstyle_nitrux_config_PART_SRCS ${kstyle_nitrux_config_PART_SRCS} nitruxstyleconfigmodule.cpp)

kconfig_add_kcfg_files(kstyle_nitrux_config_PART_SRCS ../nitruxstyleconfigdata.kcfgc)
set(kstyle_nitrux_config_PART_FORMS ui/nitruxstyleconfig.ui)
ki18n_wrap_ui(kstyle_nitrux_config_PART_FORMS_HEADERS ${kstyle_nitrux_config_PART_FORMS})

kcoreaddons_add_plugin(nitruxstyleconfig SOURCES
    ${kstyle_nitrux_config_PART_SRCS}
    ${kstyle_nitrux_config_PART_FORMS_HEADERS}
    INSTALL_NAMESPACE "kstyle_config"
)

target_compile_options(nitruxstyleconfig PRIVATE ${NITRUX_ARCH_FLAGS})

target_link_libraries(nitruxstyleconfig
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_link_libraries(nitruxstyleconfig
    PUBLIC
        KF6::CoreAddons
        KF6::ConfigCore
        KF6::ConfigWidgets
        KF6::I18n
)
    target_link_libraries(nitruxstyleconfig PUBLIC KF6::KCMUtils)
kcmutils_generate_desktop_file(nitruxstyleconfig)

