find_package(JNI REQUIRED)

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
endif (JNI_FOUND)

set (tinyb_LIB_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/api
    ${PROJECT_SOURCE_DIR}/api/direct_bt
    ${PROJECT_SOURCE_DIR}/api/tinyb
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/java/jni
)

include_directories(
    ${JNI_INCLUDE_DIRS}
    ${tinyb_LIB_INCLUDE_DIRS}
    ${JNI_HEADER_PATH}
)

set (tinyb_JNI_SRCS
  ${PROJECT_SOURCE_DIR}/src/direct_bt/BasicTypes.cpp
  ${PROJECT_SOURCE_DIR}/java/jni/JNIMem.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/helper_base.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/BluetoothFactory.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/BluetoothUtils.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/helper_tinyb.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/DBusAdapter.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/DBusDevice.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/DBusEvent.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/DBusGattCharacteristic.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/DBusGattDescriptor.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/DBusGattService.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/DBusManager.cxx
  ${PROJECT_SOURCE_DIR}/java/jni/tinyb/DBusObject.cxx
)

set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")

add_library (javatinyb SHARED ${tinyb_JNI_SRCS})
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

