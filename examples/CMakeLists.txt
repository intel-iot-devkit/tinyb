include_directories(${PROJECT_SOURCE_DIR}/api)

#include_directories(
#  ${PROJECT_SOURCE_DIR}/api
#  ${direct_bt_LIB_INCLUDE_DIRS}
#  ${tinyb_LIB_INCLUDE_DIRS}
#  ${GLIB2_INCLUDE_DIRS}
#  ${GIO_INCLUDE_DIRS}
#  ${GIO-UNIX_INCLUDE_DIRS}
#)

add_executable (hellotinyb tinyb/hellotinyb.cpp)
set_target_properties(hellotinyb
    PROPERTIES
    CXX_STANDARD 11)

add_executable (checkinit tinyb/checkinit.cpp)
set_target_properties(checkinit
    PROPERTIES
    CXX_STANDARD 11)

add_executable (asynctinyb tinyb/asynctinyb.cpp)
set_target_properties(asynctinyb
    PROPERTIES
    CXX_STANDARD 11)

add_executable (esstinyb tinyb/esstinyb.cpp)
set_target_properties(esstinyb
    PROPERTIES
    CXX_STANDARD 11)

add_executable (notifications tinyb/notifications.cpp)
set_target_properties(notifications
    PROPERTIES
    CXX_STANDARD 11)

add_executable (uuid tinyb/uuid.cpp)
set_target_properties(uuid
    PROPERTIES
    CXX_STANDARD 11)

add_executable (list_mfg tinyb/list_mfg.cpp)
set_target_properties(list_mfg
    PROPERTIES
    CXX_STANDARD 11)

add_executable (dbt_scanner00 direct_bt_scanner00/dbt_scanner00.cpp)
set_target_properties(list_mfg
    PROPERTIES
    CXX_STANDARD 11)

add_executable (dbt_scanner01 direct_bt_scanner01/dbt_scanner01.cpp)
set_target_properties(list_mfg
    PROPERTIES
    CXX_STANDARD 11)

target_link_libraries (hellotinyb tinyb)
target_link_libraries (checkinit tinyb)
target_link_libraries (asynctinyb tinyb)
target_link_libraries (esstinyb tinyb)
target_link_libraries (notifications tinyb)
target_link_libraries (uuid tinyb)
target_link_libraries (list_mfg tinyb)
target_link_libraries (dbt_scanner00 direct_bt)
target_link_libraries (dbt_scanner01 direct_bt)

install(TARGETS dbt_scanner00 dbt_scanner01 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

