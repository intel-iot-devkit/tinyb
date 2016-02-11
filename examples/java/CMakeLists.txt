find_package(Java REQUIRED)
include(UseJava)


add_jar(HelloTinyB SOURCES HelloTinyB.java INCLUDE_JARS "${CMAKE_CURRENT_BINARY_DIR}/../../java/tinyb.jar" ENTRY_POINT HelloTinyB)

add_custom_command(TARGET HelloTinyB
        POST_BUILD
        COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/HelloTinyB.dir/HelloTinyB.class" "${CMAKE_CURRENT_BINARY_DIR}"
)
