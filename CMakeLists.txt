cmake_minimum_required(VERSION 3.16)
project(lightly VERSION 6.80 LANGUAGES CXX)


set(KF6_MIN_VERSION "5.240.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
set(QT_MIN_VERSION "6.6.0")

include(GenerateExportHeader)
include(WriteBasicConfigVersionFile)
include(FeatureSummary)
find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep Qt6 targets and helper functions explicitly versioned.
set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)
set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)

include(CheckCXXCompilerFlag)
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    check_cxx_compiler_flag("-march=x86-64-v3" HAVE_X86_64_V3)
    if(HAVE_X86_64_V3)
        add_compile_options(-march=x86-64-v3)
    endif()
endif()

include(CMakePackageConfigHelpers)
include(ECMInstallIcons)
include(KDECompilerSettings NO_POLICY_SCOPE)


if(WIN32 OR APPLE)
    set(WITH_DECORATIONS OFF)
endif()

set(QT_MAJOR_VERSION 6)
include(KDEInstallDirs6)
include(KDECMakeSettings)

option(WITH_DECORATIONS "Build Lightly window decorations for KWin" OFF)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS Widgets DBus)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
        CoreAddons
        ColorScheme
        Config
        GuiAddons
        I18n
        IconThemes
        WindowSystem)
    if(NOT WIN32 AND NOT APPLE)
        find_package(KF6KCMUtils ${KF6_MIN_VERSION})
        set_package_properties(KF6KCMUtils PROPERTIES
            TYPE REQUIRED
            DESCRIPTION "Helps create configuration modules"
            PURPOSE "KCMUtils used for the configuration modules or the decoration and Qt Style"
        )
    endif()

find_package(Qt6 ${QT_MIN_VERSION} OPTIONAL_COMPONENTS Quick)
    if(Qt6Quick_FOUND)
        find_package(KF6KirigamiPlatform ${KF6_MIN_VERSION} REQUIRED)
    endif()

set(LIGHTLY_HAVE_KSTYLE FALSE)
set(LIGHTLY_HAVE_QTQUICK ${Qt6Quick_FOUND})


add_subdirectory(kstyle kstyle6)
add_subdirectory(liblightlycommon liblightlycommon6)

    if(WITH_DECORATIONS)
        find_package(KDecoration2 REQUIRED)
        add_subdirectory(kdecoration)
    endif()


# create a Config.cmake and a ConfigVersion.cmake file and install them
include(ECMSetupVersion)
ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX LIGHTLY
                 PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/LightlyConfigVersion.cmake"
                  )

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/Lightly")

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/LightlyConfig.cmake.in"
                                  "${CMAKE_CURRENT_BINARY_DIR}/LightlyConfig.cmake"
                                  PATH_VARS KDE_INSTALL_FULL_DATADIR
                                  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
                              )

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/LightlyConfig.cmake"
                   "${CMAKE_CURRENT_BINARY_DIR}/LightlyConfigVersion.cmake"
            DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
            COMPONENT Devel
          )


if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE
        @ONLY
    )

    add_custom_target(uninstall
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    )
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
