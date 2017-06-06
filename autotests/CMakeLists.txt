include(ECMMarkAsTest)
include(ECMAddTests)

find_package(Qt5Test "${REQUIRED_QT_VERSION}" CONFIG REQUIRED)
set_package_properties(Qt5Test PROPERTIES PURPOSE "Required for autotests")

include_directories(
  ${KHTML_SOURCE_DIR}/
  ${KHTML_SOURCE_DIR}/misc
  ${KHTML_SOURCE_DIR}/dom
  ${KHTML_SOURCE_DIR}/xml
  ${KHTML_SOURCE_DIR}/html
  ${KHTML_SOURCE_DIR}/rendering
  ${KHTML_SOURCE_DIR}/ecma
  ${KHTML_SOURCE_DIR}/imload
  ${KHTML_SOURCE_DIR}/imload/decoders
  ${KHTML_SOURCE_DIR}/java
  ${KHTML_SOURCE_DIR}/svg
  ${KHTML_SOURCE_DIR}/svg/graphics
  ${KHTML_SOURCE_DIR}/platform/graphics
  ${KHTML_SOURCE_DIR}/platform/text
  ${KHTML_SOURCE_DIR}/platform
  ${KHTML_SOURCE_DIR}/compat
  ${KHTML_SOURCE_DIR}/css
)

ecm_add_test(khtmlparttest.cpp
    LINK_LIBRARIES Qt5::Test KF5::KHtml Qt5::Widgets KF5::XmlGui KF5::TextWidgets KF5::Parts
    TEST_NAME parttest
    NAME_PREFIX "khtml-"
    GUI
)

set(kencodingdetector_SRCS
    ${KHTML_SOURCE_DIR}/misc/kencodingdetector.cpp
    ${KHTML_SOURCE_DIR}/misc/guess_ja.cpp
)
ecm_add_test(kencodingdetectortest.cpp ${kencodingdetector_SRCS}
    LINK_LIBRARIES KF5::Codecs KF5::I18n KF5::KHtml Qt5::Test
    TEST_NAME kencodingdetectortest
    NAME_PREFIX "khtml-"
    GUI
)
