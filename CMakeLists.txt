cmake_minimum_required(VERSION 3.25)
project(nitrux-kstyle-theme VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable the optimized instruction set only on x86-64 build hosts. Keep the
# default flags untouched on other architectures so the style remains
# portable to ARM and other supported platforms.
set(NITRUX_ARCH_FLAGS)
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$"
   AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    list(APPEND NITRUX_ARCH_FLAGS -march=x86-64-v3)
endif()

find_package(ECM 5.102 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
find_package(Qt6 6.10.0 REQUIRED CONFIG COMPONENTS Core Gui Widgets)
# Tell ECM/KDEInstallDirs6 to use the Qt 6 query path (qtpaths), never Qt 5 qmake.
set(QT_MAJOR_VERSION 6)
# Qt6 discovers GNUInstallDirs while loading its package config. Mirror any
# explicitly selected CMake install paths into KDEInstallDirs variables before
# KDEInstallDirs6 is included, avoiding ambiguous-install-dir warnings.
foreach(_dir IN ITEMS
        BINDIR
        SBINDIR
        LIBDIR
        LIBEXECDIR
        INCLUDEDIR
        LOCALSTATEDIR
        SHAREDSTATEDIR
        DATAROOTDIR
        SYSCONFDIR
        RUNSTATEDIR)
    if(DEFINED CMAKE_INSTALL_${_dir} AND NOT DEFINED KDE_INSTALL_${_dir})
        set(KDE_INSTALL_${_dir} "${CMAKE_INSTALL_${_dir}}")
    endif()
endforeach()



include(KDEInstallDirs6)
include(KDECMakeSettings)
include(ECMQtDeclareLoggingCategory)
find_package(KF6 6.26.0 REQUIRED COMPONENTS
    CoreAddons
    Config
    ConfigWidgets
    I18n
    GuiAddons
    IconThemes
    ColorScheme
)
find_package(KF6KCMUtils 6.26.0 REQUIRED CONFIG)
find_package(KF6FrameworkIntegration 6.26.0 CONFIG QUIET)

set(NITRUX_HAVE_KSTYLE ${KF6FrameworkIntegration_FOUND})

add_subdirectory(kstyle)

