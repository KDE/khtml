cmake_minimum_required(VERSION 3.0)

set(KF5_VERSION "5.49.0") # handled by release scripts
set(KF5_DEP_VERSION "5.49.0") # handled by release scripts
project(KHtml VERSION ${KF5_VERSION})

include(FeatureSummary)
find_package(ECM 5.49.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)


set(REQUIRED_QT_VERSION 5.8.0)
find_package(Qt5 "${REQUIRED_QT_VERSION}" CONFIG REQUIRED Widgets Network DBus PrintSupport Xml)
include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)

# Most of KJS doesn't even require Qt, thus we can't use override
string(REPLACE "-Wsuggest-override" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

find_package(KF5Archive ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Codecs ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5GlobalAccel ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5I18n ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5IconThemes ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5KIO ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5JS ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Notifications ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Parts ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Sonnet ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5TextWidgets ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Wallet ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5WidgetsAddons ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5XmlGui ${KF5_DEP_VERSION} REQUIRED)

ecm_setup_version(PROJECT VARIABLE_PREFIX KHTML
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/khtml_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5KHtmlConfigVersion.cmake"
                        SOVERSION 5)

find_package(Perl)
set_package_properties(Perl PROPERTIES
                       URL "http://www.perl.org"
                       TYPE REQUIRED
                       PURPOSE "Required for generating JS bindings for elements"
                      )

if(NOT APPLE)
  find_package(X11)
  set(HAVE_X11 ${X11_FOUND})
endif()

find_package(Phonon4Qt5 4.6.60)
set_package_properties(Phonon4Qt5 PROPERTIES
                      DESCRIPTION "Qt Multimedia Library"
                      URL "http://phonon.kde.org"
                      TYPE REQUIRED
                      PURPOSE "Required for HTML5 multimedia elements"
                     )

find_package(JPEG)
set_package_properties(JPEG PROPERTIES DESCRIPTION "JPEG decoding library"
                       URL "http://www.ijg.org"
                       TYPE REQUIRED
                       PURPOSE "Required for decoding and displaying JPEG images"
                      )

find_package(GIF)
set_package_properties(GIF PROPERTIES DESCRIPTION "GIF decoding library"
                       URL "http://sourceforge.net/projects/giflib"
                       TYPE REQUIRED
                       PURPOSE "Required for decoding and displaying GIF images"
                      )

find_package(PNG)
set_package_properties(PNG PROPERTIES DESCRIPTION "PNG decoding library"
                       URL "http://www.libpng.org/pub/png"
                       TYPE REQUIRED
                       PURPOSE "Required for decoding and displaying PNG images"
                      )

if(NOT WIN32 AND NOT APPLE AND X11_FOUND)
    find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED X11Extras)
endif()

find_package(Gperf REQUIRED)

remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)

add_definitions(-DENABLE_SVG)
add_definitions(-DENABLE_SVG_FONTS)
add_definitions(-DWTF_PLATFORM_QT)

set(KHTML_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(KHTML_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)

add_definitions(-DTRANSLATION_DOMAIN=\"khtml5\")
if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
endif()
add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(tests)
  add_subdirectory(autotests)
endif()

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5KHtml")

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
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5} COMPONENT Devel
)

install(FILES khtml.categories DESTINATION ${KDE_INSTALL_CONFDIR})

feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)
