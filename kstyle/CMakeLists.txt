find_package(XCB COMPONENTS XCB)
set_package_properties(XCB PROPERTIES TYPE OPTIONAL DESCRIPTION "X protocol C-language Binding" PURPOSE "Required for X11 native window integration")
if(UNIX AND NOT APPLE AND XCB_XCB_FOUND)
    find_package(Qt6 REQUIRED CONFIG COMPONENTS X11Extras)
    set(LIGHTLY_HAVE_X11 TRUE)
else()
    set(LIGHTLY_HAVE_X11 FALSE)
endif()

find_package(KF6Wayland CONFIG)
set(LIGHTLY_HAVE_KWAYLAND ${KF6Wayland_FOUND})

################# includes #################
include_directories(
  animations
  debug
)

include_directories(${CMAKE_SOURCE_DIR}/liblightlycommon)
include_directories(${CMAKE_BINARY_DIR}/liblightlycommon6)

################# configuration #################
configure_file(config-lightly.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-lightly.h )

########### next target ###############
set(lightly_PART_SRCS
    animations/lightlyanimation.cpp
    animations/lightlyanimations.cpp
    animations/lightlyanimationdata.cpp
    animations/lightlybaseengine.cpp
    animations/lightlybusyindicatordata.cpp
    animations/lightlybusyindicatorengine.cpp
    animations/lightlydialdata.cpp
    animations/lightlydialengine.cpp
    animations/lightlyenabledata.cpp
    animations/lightlygenericdata.cpp
    animations/lightlyheaderviewdata.cpp
    animations/lightlyheaderviewengine.cpp
    animations/lightlymenubardata.cpp
    animations/lightlymenubarengine.cpp
    animations/lightlymenudata.cpp
    animations/lightlymenuengine.cpp
    animations/lightlyscrollbardata.cpp
    animations/lightlyscrollbarengine.cpp
    animations/lightlyspinboxengine.cpp
    animations/lightlyspinboxdata.cpp
    animations/lightlystackedwidgetdata.cpp
    animations/lightlystackedwidgetengine.cpp
    animations/lightlytabbarengine.cpp
    animations/lightlytabbardata.cpp
    animations/lightlytoolboxengine.cpp
    animations/lightlytransitiondata.cpp
    animations/lightlytransitionwidget.cpp
    animations/lightlywidgetstateengine.cpp
    animations/lightlywidgetstatedata.cpp
    debug/lightlywidgetexplorer.cpp
    lightlyaddeventfilter.cpp
    lightlyblurhelper.cpp
    lightlyframeshadow.cpp
    lightlyhelper.cpp
    lightlymdiwindowshadow.cpp
    lightlymnemonics.cpp
    lightlypropertynames.cpp
    lightlyshadowhelper.cpp
    lightlysplitterproxy.cpp
    lightlystyle.cpp
    lightlystyleplugin.cpp
    lightlytileset.cpp
    lightlywindowmanager.cpp
)

kconfig_add_kcfg_files(lightly_PART_SRCS ../kdecoration/lightlysettings.kcfgc)
kconfig_add_kcfg_files(lightly_PART_SRCS lightlystyleconfigdata.kcfgc)

add_library(lightly6 MODULE ${lightly_PART_SRCS})

target_include_directories(lightly6 PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/kdecoration
    ${CMAKE_BINARY_DIR}/liblightlycommon6
)

target_link_libraries(lightly6
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
)

if( LIGHTLY_HAVE_QTQUICK )
    target_link_libraries(lightly6
        Qt6::Quick
        KF6::CoreAddons
    )
endif()

target_link_libraries(lightly6
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::GuiAddons
    KF6::IconThemes
    KF6::WindowSystem
)

target_link_libraries(lightly6 KF6::ColorScheme)
if(LIGHTLY_HAVE_QTQUICK)
    target_link_libraries(lightly6 KF6::KirigamiPlatform)
endif()


target_link_libraries(lightly6 lightlycommon6)

if(KF6FrameworkIntegration_FOUND)
    target_link_libraries(lightly6  KF6::Style)
endif()

if(LIGHTLY_HAVE_X11)
    target_link_libraries(lightly6 Qt6::X11Extras XCB::XCB)
endif()
if(LIGHTLY_HAVE_KWAYLAND)
    target_link_libraries(lightly6 KF6::WaylandClient)
endif()


if (WIN32)
    # As stated in https://docs.microsoft.com/en-us/cpp/c-runtime-library/math-constants M_PI only gets defined
    # when if _USE_MATH_DEFINES is defined
    target_compile_definitions(lightly6 PRIVATE _USE_MATH_DEFINES _BSD_SOURCE)
endif()


########### install files ###############
install(TARGETS lightly6 DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/styles/)
install(FILES lightly.themerc  DESTINATION  ${KDE_INSTALL_DATADIR}/kstyle/themes)

########### subdirectories ###############

if (QT_MAJOR_VERSION EQUAL "6" AND TARGET "KF6::KCMUtils")
    add_subdirectory(config)
endif()
