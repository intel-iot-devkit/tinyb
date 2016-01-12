find_package(Java REQUIRED)
include(UseJava)


add_jar(Hellotinyb SOURCES Hellotinyb.java INCLUDE_JARS "${CMAKE_CURRENT_BINARY_DIR}/../../java/tinyb.jar" ENTRY_POINT Hellotinyb)

add_custom_command(TARGET Hellotinyb
        POST_BUILD
        COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/Hellotinyb.dir/Hellotinyb.class" "${CMAKE_CURRENT_BINARY_DIR}"
)
