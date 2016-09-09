include_directories(
  ${tinyb_LIB_INCLUDE_DIRS}
  ${GLIB2_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
)

add_executable (hellotinyb hellotinyb.cpp)
set_target_properties(hellotinyb
    PROPERTIES
    CXX_STANDARD 11)

add_executable (checkinit checkinit.cpp)
set_target_properties(checkinit
    PROPERTIES
    CXX_STANDARD 11)

add_executable (asynctinyb asynctinyb.cpp)
set_target_properties(asynctinyb
    PROPERTIES
    CXX_STANDARD 11)

add_executable (esstinyb esstinyb.cpp)
set_target_properties(esstinyb
    PROPERTIES
    CXX_STANDARD 11)

add_executable (notifications notifications.cpp)
set_target_properties(notifications
    PROPERTIES
    CXX_STANDARD 11)

add_executable (uuid uuid.cpp)
set_target_properties(uuid
    PROPERTIES
    CXX_STANDARD 11)

add_executable (list_mfg list_mfg.cpp)
set_target_properties(list_mfg
    PROPERTIES
    CXX_STANDARD 11)

include_directories(${PROJECT_SOURCE_DIR}/api)

target_link_libraries (hellotinyb tinyb)
target_link_libraries (checkinit tinyb)
target_link_libraries (asynctinyb tinyb)
target_link_libraries (esstinyb tinyb)
target_link_libraries (notifications tinyb)
target_link_libraries (uuid tinyb)
target_link_libraries (list_mfg tinyb)
