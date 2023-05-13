cmake_minimum_required(VERSION 3.16)

set(KF_VERSION "5.107.0") # handled by release scripts
set(KF_DEP_VERSION "5.106.0") # handled by release scripts
project(KHtml VERSION ${KF_VERSION})

include(FeatureSummary)
find_package(ECM 5.106.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)


set(REQUIRED_QT_VERSION 5.15.2)
find_package(Qt5 "${REQUIRED_QT_VERSION}" CONFIG REQUIRED Widgets Network DBus PrintSupport Xml)
include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)

# Most of KJS doesn't even require Qt, thus we can't use override
string(REPLACE "-Wsuggest-override" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

find_package(KF5Archive ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Codecs ${KF_DEP_VERSION} REQUIRED)
find_package(KF5I18n ${KF_DEP_VERSION} REQUIRED)
find_package(KF5IconThemes ${KF_DEP_VERSION} REQUIRED)
find_package(KF5KIO ${KF_DEP_VERSION} REQUIRED)
find_package(KF5JS ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Notifications ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Parts ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Sonnet ${KF_DEP_VERSION} REQUIRED)
find_package(KF5TextWidgets ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Wallet ${KF_DEP_VERSION} REQUIRED)
find_package(KF5WidgetsAddons ${KF_DEP_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF_DEP_VERSION} REQUIRED)
find_package(KF5XmlGui ${KF_DEP_VERSION} REQUIRED)

if(NOT WIN32 AND NOT APPLE)
  find_package(KF5GlobalAccel ${KF_DEP_VERSION} REQUIRED)
endif()

set(HAVE_KGLOBALACCEL ${KF5GlobalAccel_FOUND})

ecm_setup_version(PROJECT VARIABLE_PREFIX KHTML
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/khtml_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5KHtmlConfigVersion.cmake"
                        SOVERSION 5)

find_package(Perl)
set_package_properties(Perl PROPERTIES
                       URL "https://www.perl.org/"
                       TYPE REQUIRED
                       PURPOSE "Required for generating JS bindings for elements"
                      )

if(NOT WIN32 AND NOT APPLE)
  option(WITH_X11 "Build with X11 integration" ON)
  if(WITH_X11)
    find_package(X11 REQUIRED)
    set(HAVE_X11 1)
    find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED X11Extras)
  endif()
endif()

find_package(Phonon4Qt5 4.6.60)
set_package_properties(Phonon4Qt5 PROPERTIES
                      DESCRIPTION "Qt Multimedia Library"
                      URL "https://phonon.kde.org/"
                      TYPE REQUIRED
                      PURPOSE "Required for HTML5 multimedia elements"
                     )

find_package(JPEG)
set_package_properties(JPEG PROPERTIES DESCRIPTION "JPEG decoding library"
                       URL "https://www.ijg.org"
                       TYPE REQUIRED
                       PURPOSE "Required for decoding and displaying JPEG images"
                      )

find_package(GIF)
set_package_properties(GIF PROPERTIES DESCRIPTION "GIF decoding library"
                       URL "https://sourceforge.net/projects/giflib"
                       TYPE REQUIRED
                       PURPOSE "Required for decoding and displaying GIF images"
                      )

find_package(PNG)
set_package_properties(PNG PROPERTIES DESCRIPTION "PNG decoding library"
                       URL "http://www.libpng.org/pub/png"
                       TYPE REQUIRED
                       PURPOSE "Required for decoding and displaying PNG images"
                      )

find_package(Gperf REQUIRED)

# remove definitions set by KDEFrameworkCompilerSettings which we fail to meet
remove_definitions(-DQT_NO_KEYWORDS)
remove_definitions(-DQT_NO_FOREACH)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
# ideally we would support QT_NO_KEYWORDS, but at least we can do this
add_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)

add_definitions(-DENABLE_SVG)
add_definitions(-DENABLE_SVG_FONTS)
add_definitions(-DWTF_PLATFORM_QT)

# avoid warning for "this use of "defined" may not be portable" for all the #if PLATFORM(Foo)
set(_flag "-Wno-expansion-to-defined")
check_cxx_compiler_flag(${_flag} _flag_present)
if(_flag_present)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_flag}")
endif()


set(KHTML_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(KHTML_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)

add_definitions(-DTRANSLATION_DOMAIN=\"khtml5\")
ki18n_install(po)
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
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KHtml COMPONENT Devel
)

feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)
