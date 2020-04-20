find_package(Java REQUIRED)
include(UseJava)

add_jar(HelloTinyB SOURCES HelloTinyB.java INCLUDE_JARS "${CMAKE_CURRENT_BINARY_DIR}/../../java/tinyb2.jar" ENTRY_POINT HelloTinyB)

add_custom_command(TARGET HelloTinyB
        POST_BUILD
        COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/HelloTinyB.dir/HelloTinyB.class" "${CMAKE_CURRENT_BINARY_DIR}"
)

add_jar(AsyncTinyB SOURCES AsyncTinyB.java INCLUDE_JARS "${CMAKE_CURRENT_BINARY_DIR}/../../java/tinyb2.jar" ENTRY_POINT AsyncTinyB)

add_custom_command(TARGET AsyncTinyB
        POST_BUILD
        COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/AsyncTinyB.dir/AsyncTinyB.class" "${CMAKE_CURRENT_BINARY_DIR}"
)

add_jar(Notification SOURCES Notification.java INCLUDE_JARS "${CMAKE_CURRENT_BINARY_DIR}/../../java/tinyb2.jar" ENTRY_POINT Notification)

add_custom_command(TARGET Notification
        POST_BUILD
        COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/Notification.dir/Notification.class" "${CMAKE_CURRENT_BINARY_DIR}"
)

add_jar(ScannerTinyB00 SOURCES ScannerTinyB00.java INCLUDE_JARS "${CMAKE_CURRENT_BINARY_DIR}/../../java/tinyb2.jar" ENTRY_POINT Notification)

add_custom_command(TARGET ScannerTinyB00
        POST_BUILD
        COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/ScannerTinyB00.dir/ScannerTinyB00.class" "${CMAKE_CURRENT_BINARY_DIR}"
)

add_jar(ScannerTinyB01 SOURCES ScannerTinyB01.java INCLUDE_JARS "${CMAKE_CURRENT_BINARY_DIR}/../../java/tinyb2.jar" ENTRY_POINT Notification)

add_custom_command(TARGET ScannerTinyB01
        POST_BUILD
        COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/ScannerTinyB01.dir/ScannerTinyB01.class" "${CMAKE_CURRENT_BINARY_DIR}"
)
