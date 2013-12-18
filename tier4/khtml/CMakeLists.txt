cmake_minimum_required(VERSION 2.8.12)

project(KHtml)

find_package(ECM 0.0.9 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
include(FeatureSummary)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)

set(REQUIRED_QT_VERSION "5.2")
find_package(Qt5 "${REQUIRED_QT_VERSION}" CONFIG REQUIRED Widgets Network DBus PrintSupport Xml Test)
include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)

set(KF5_VERSION "5.0.0")
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    find_package(KF5Archive ${KF5_VERSION} REQUIRED)
    find_package(KF5I18n ${KF5_VERSION} REQUIRED) #
    find_package(KF5CoreAddons ${KF5_VERSION} REQUIRED)
    find_package(KF5GuiAddons ${KF5_VERSION} REQUIRED)
    find_package(KF5DBusAddons ${KF5_VERSION} REQUIRED)
    find_package(KF5Service ${KF5_VERSION} REQUIRED)
    find_package(KF5WindowSystem ${KF5_VERSION} REQUIRED) #
    find_package(KF5Crash ${KF5_VERSION} REQUIRED)
    find_package(KF5Init ${KF5_VERSION} REQUIRED)
    find_package(KF5JS ${KF5_VERSION} REQUIRED)
    find_package(KF5ConfigWidgets ${KF5_VERSION} REQUIRED)
    find_package(KF5ItemViews ${KF5_VERSION} REQUIRED)
    find_package(KF5IconThemes ${KF5_VERSION} REQUIRED)
    find_package(KF5Notifications ${KF5_VERSION} REQUIRED)
    find_package(KF5Completion ${KF5_VERSION} REQUIRED)
    find_package(KF5TextWidgets ${KF5_VERSION} REQUIRED)
    find_package(KF5XmlGui ${KF5_VERSION} REQUIRED)
    find_package(KF5Parts ${KF5_VERSION} REQUIRED)
    find_package(KF5KIO ${KF5_VERSION} REQUIRED)
    find_package(KF5Wallet ${KF5_VERSION} REQUIRED)
    find_package(KF5GlobalAccel ${KF5_VERSION} REQUIRED)
else()
  set(KJS_CREATE_HASH_TABLE "${CMAKE_SOURCE_DIR}/tier1/kjs/src/kjs/create_hash_table")
endif()

ecm_setup_version(${KF5_VERSION} VARIABLE_PREFIX KHTML
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/khtml_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5KHtmlConfigVersion.cmake")

find_package(Perl)
set_package_properties(Perl PROPERTIES URL "http://www.perl.org" TYPE REQUIRED PURPOSE "Needed for building kdelibs")

find_package(X11)
set(HAVE_X11 ${X11_FOUND})

find_package(Phonon4Qt5 4.6.60 REQUIRED NO_MODULE)

find_package(JPEG)
set_package_properties(JPEG PROPERTIES DESCRIPTION "JPEG image format support"
                       URL "http://www.ijg.org"
                       TYPE REQUIRED
                       PURPOSE "Required by khtml"
                      )

find_package(GIF)
set_package_properties(GIF PROPERTIES DESCRIPTION "GIF image format support"
                       URL "http://sourceforge.net/projects/giflib"
                       TYPE REQUIRED
                       PURPOSE "Required by khtml"
                      )

find_package(PNG)
set_package_properties(PNG PROPERTIES DESCRIPTION "PNG image format support"
                       URL "http://www.libpng.org/pub/png"
                       TYPE REQUIRED
                       PURPOSE "Required by khtml"
                      )

if(NOT WIN32)
    find_package(Qt5 5.2.0 CONFIG REQUIRED X11Extras)
endif()


remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)

add_definitions(-DENABLE_SVG)
add_definitions(-DENABLE_SVG_FONTS)
add_definitions(-DWTF_PLATFORM_QT)

set(KHTML_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(KHTML_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(autotests)

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5KHtml")

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5KHtmlConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5KHtmlConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5KHtmlConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5KHtmlConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF5KHtmlTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5KHtmlTargets.cmake NAMESPACE KF5:: )

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/khtml_version.h
  DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel
)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
