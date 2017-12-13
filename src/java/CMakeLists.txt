
include_directories( ${KHTML_SOURCE_DIR} )

########### next target ###############

set(kjavaappletviewer_PART_SRCS
   kjavaapplet.cpp
   kjavaappletcontext.cpp
   kjavaappletserver.cpp
   kjavaappletwidget.cpp
   kjavaprocess.cpp
   kjavadownloader.cpp
   kjavaappletviewer.cpp
)
ecm_qt_declare_logging_category(kjavaappletviewer_PART_SRCS HEADER kjavaappletviewer_debug.h IDENTIFIER KJAVAAPPLETVIEWER_LOG CATEGORY_NAME kf5.khtml.javaappletviewer)

add_library(kjavaappletviewer MODULE ${kjavaappletviewer_PART_SRCS})

target_link_libraries(kjavaappletviewer
                      Qt5::Network
                      KF5::Parts
                      KF5::WindowSystem
                      KF5::WidgetsAddons
                      KF5::XmlGui
                      KF5::I18n
                      KF5::IconThemes )

install(TARGETS kjavaappletviewer  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kjava.policy.in.cmake ${CMAKE_CURRENT_BINARY_DIR}/kjava.policy )
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/kjava.policy )
add_definitions(-DMAKE_KHTML_LIB)

########### install files ###############

# kjava.policy
install( FILES kjava.jar ${CMAKE_CURRENT_BINARY_DIR}/kjava.policy pluginsinfo  DESTINATION  ${KDE_INSTALL_DATADIR_KF5}/kjava/ )
install( FILES kjavaappletviewer.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
