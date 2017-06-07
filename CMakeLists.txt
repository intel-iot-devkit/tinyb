cmake_minimum_required (VERSION 3.0.0 FATAL_ERROR)
project (tinyb)

FIND_PACKAGE (Threads REQUIRED)
FIND_PACKAGE (PkgConfig REQUIRED)

PKG_CHECK_MODULES (GLIB2 REQUIRED glib-2.0>=2.40)
PKG_CHECK_MODULES (GIO REQUIRED gio-2.0>=2.40)
PKG_CHECK_MODULES (GIO-UNIX REQUIRED gio-unix-2.0>=2.40)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -fno-omit-frame-pointer -DDEBUG")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set (LIB_INSTALL_DIR "lib${LIB_SUFFIX}" CACHE PATH "Installation path for libraries")

# Set CMAKE_LIB_INSTALL_DIR if not defined
include(GNUInstallDirs)

# Appends the cmake/modules path to MAKE_MODULE_PATH variable.
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

# Make a version file containing the current version from git.
include (GetGitRevisionDescription)
git_describe (VERSION "--tags")

if ("x_${VERSION}" STREQUAL "x_GIT-NOTFOUND" OR "x_${VERSION}" STREQUAL "x_HEAD-HASH-NOTFOUND" OR "x_${VERSION}" STREQUAL "x_-128-NOTFOUND")
  message (WARNING " - Install git to compile a production libtinyb!")
  set (VERSION "v0.5.0-dirty")
endif ()

message (INFO " - libtinyb Version ${VERSION}")

#parse the version information into pieces.
string (REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string (REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string (REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string (REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+\\-([0-9]+).*" "\\1" VERSION_COMMIT "${VERSION}")
string (REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+\\-(.*)" "\\1" VERSION_SHA1 "${VERSION}")
set (VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set (VERSION_API "${VERSION_MAJOR}.${VERSION_MINOR}")
string(TIMESTAMP BUILD_TSTAMP "%Y-%m-%d %H:%M:%S")

if ("${VERSION_COMMIT}" MATCHES "^v.*")
  set (VERSION_COMMIT "")
endif()

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/version.c.in
                ${CMAKE_CURRENT_BINARY_DIR}/src/version.c)

# this is the library version, independant of git revision
set (tinyb_VERSION_MAJOR ${VERSION_MAJOR})
set (tinyb_VERSION_MINOR ${VERSION_MINOR})
set (tinyb_VERSION_PATCH ${VERSION_PATCH})
set (tinyb_VERSION_STRING "${tinyb_VERSION_MAJOR}.${tinyb_VERSION_MINOR}.${tinyb_VERSION_PATCH}")

set (CMAKE_SWIG_FLAGS "")

set (tinyb_DOCS_DIR documentation)

IF(WARNINGS)
    IF(UNIX)
        IF(CMAKE_COMPILER_IS_GNUCC)
            ADD_DEFINITIONS(-Wall -Wextra)
        ENDIF(CMAKE_COMPILER_IS_GNUCC)
    ENDIF(UNIX)
ENDIF(WARNINGS)

IF(DEBUG)
    IF(UNIX)
        IF(CMAKE_COMPILER_IS_GNUCC)
            ADD_DEFINITIONS(-g -ggdb)
        ENDIF(CMAKE_COMPILER_IS_GNUCC)
    ENDIF(UNIX)
ENDIF(DEBUG)

find_path (SYSTEM_USR_DIR "stdlib.h")
include_directories (${SYSTEM_USR_DIR})

option (BUILDJAVA "Build Java API." OFF)

IF(BUILDJAVA)
    configure_file (${CMAKE_CURRENT_SOURCE_DIR}/java/manifest.txt.in ${CMAKE_CURRENT_BINARY_DIR}/java/manifest.txt)
    add_subdirectory (java)
    add_subdirectory (examples/java)
ENDIF(BUILDJAVA)

# add a target to generate API documentation with Doxygen
find_package (Doxygen)
if (DOXYGEN_FOUND)
  configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.cpp.out @ONLY)
  if (BUILDJAVA)
    configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.java.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.java.out @ONLY)
    add_custom_command (OUTPUT ${tinyb_DOCS_DIR}/cpp ${tinyb_DOCS_DIR}/java
      COMMAND ${CMAKE_COMMAND} -E make_directory ${tinyb_DOCS_DIR}
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.cpp.out
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.java.out
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/api/tinyb/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/java/*.java
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
    add_custom_target(doc
      DEPENDS ${tinyb_DOCS_DIR}/cpp ${tinyb_DOCS_DIR}/java)
  else ()
    add_custom_command (OUTPUT ${tinyb_DOCS_DIR}/cpp
      COMMAND ${CMAKE_COMMAND} -E make_directory ${tinyb_DOCS_DIR}
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.cpp.out
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/api/
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
    add_custom_target(doc
      DEPENDS ${tinyb_DOCS_DIR}/cpp)
  endif ()
endif (DOXYGEN_FOUND)

add_subdirectory (src)

if (BUILDEXAMPLES)
  add_subdirectory (examples)
endif (BUILDEXAMPLES)
