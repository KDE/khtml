include(ECMMarkAsTest)

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

add_executable(khtmlparttest khtmlparttest.cpp)
add_test(khtml-parttest khtmlparttest)
target_link_libraries(khtmlparttest Qt5::Test KF5::KHtml Qt5::Widgets KF5::XmlGui KF5::TextWidgets KF5::Parts)
ecm_mark_as_test(khtmlparttest)

set(kencodingdetector_SRCS
    ${KHTML_SOURCE_DIR}/misc/kencodingdetector.cpp
    ${KHTML_SOURCE_DIR}/misc/guess_ja.cpp
)

add_executable(kencodingdetectortest kencodingdetectortest.cpp ${kencodingdetector_SRCS})
add_test(khtml-kencodingdetectortest kencodingdetectortest)
target_link_libraries(kencodingdetectortest KF5::Codecs KF5::I18n KF5::KHtml Qt5::Test)
ecm_mark_as_test(kencodingdetectortest)
