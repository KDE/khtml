
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


add_library(kjavaappletviewer MODULE ${kjavaappletviewer_PART_SRCS})

target_link_libraries(kjavaappletviewer
                      Qt5::Network
                      KF5::Parts
                      KF5::WindowSystem
                      KF5::WidgetsAddons
                      KF5::XmlGui
                      KF5::I18n
                      KF5::IconThemes )

install(TARGETS kjavaappletviewer  DESTINATION ${PLUGIN_INSTALL_DIR} )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kjava.policy.in.cmake ${CMAKE_CURRENT_BINARY_DIR}/kjava.policy )
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/kjava.policy )
add_definitions(-DMAKE_KHTML_LIB)

########### install files ###############

# kjava.policy
install( FILES kjava.jar ${CMAKE_CURRENT_BINARY_DIR}/kjava.policy pluginsinfo  DESTINATION  ${DATA_INSTALL_DIR}/kjava/ )
install( FILES kjavaappletviewer.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
