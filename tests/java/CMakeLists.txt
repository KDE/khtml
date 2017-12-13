include_directories( ${KHTML_SOURCE_DIR}/java ${KHTML_SOURCE_DIR} )

include(ECMMarkAsTest)

########### next target ###############

set(testserver_SRCS testkjavaappletserver.cpp
    ${KHTML_SOURCE_DIR}/java/kjavaappletcontext.cpp
    ${KHTML_SOURCE_DIR}/java/kjavaappletviewer.cpp
    ${KHTML_SOURCE_DIR}/java/kjavaprocess.cpp
    ${KHTML_SOURCE_DIR}/java/kjavaapplet.cpp
    ${KHTML_SOURCE_DIR}/java/kjavaappletwidget.cpp
    ${KHTML_SOURCE_DIR}/java/kjavadownloader.cpp
    ${KHTML_SOURCE_DIR}/java/kjavaappletserver.cpp)

ecm_qt_declare_logging_category(testserver_SRCS HEADER kjavaappletviewer_debug.h IDENTIFIER KJAVAAPPLETVIEWER_LOG CATEGORY_NAME kf5.khtml.javaappletviewer)
add_executable(testserver ${testserver_SRCS})
ecm_mark_as_test(testserver)

target_link_libraries(testserver
    KF5::Parts
    KF5::I18n
    KF5::XmlGui
    KF5::IconThemes
    KF5::WindowSystem
    Qt5::Network
)



