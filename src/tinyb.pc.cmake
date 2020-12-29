prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/include

Name: tinyb
Description: Tiny BLE library
Version: @tinyb_VERSION_STRING@

Libs: -L${libdir} -ltinyb
Cflags: -I${includedir}
