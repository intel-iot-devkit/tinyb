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

include_directories(${PROJECT_SOURCE_DIR}/api)

target_link_libraries (hellotinyb tinyb)
