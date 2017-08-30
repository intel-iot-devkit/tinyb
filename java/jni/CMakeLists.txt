find_package(JNI REQUIRED)

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
endif (JNI_FOUND)

set (tinyb_LIB_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/api
    ${PROJECT_SOURCE_DIR}/api/tinyb
    ${PROJECT_SOURCE_DIR}/include
)

include_directories(
    ${JNI_INCLUDE_DIRS}
    ${tinyb_LIB_INCLUDE_DIRS}
    ${JNI_HEADER_PATH}
)

file(GLOB JNI_SOURCES "*.cxx")

set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")

add_library (javatinyb SHARED ${JNI_SOURCES})
target_link_libraries(javatinyb ${JNI_LIBRARIES} tinyb)

set_target_properties(
    javatinyb
    PROPERTIES
    SOVERSION ${tinyb_VERSION_MAJOR}
    VERSION ${tinyb_VERSION_STRING}
    CXX_STANDARD 11
    COMPILE_FLAGS "-Wall -Wextra"
)

install(TARGETS javatinyb LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

