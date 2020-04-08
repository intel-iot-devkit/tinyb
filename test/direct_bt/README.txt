Rudimentary unit testing for our direct_bt
using the very flat cppunit <https://github.com/cppunit/cppunit.git>.

The 'cppunit.h' file has been dropped to: ${PROJECT_SOURCE_DIR}/include/cppunit/cppunit.h

After a normal build, you may invoke testing via 'ctest'

To see the normal test stdout/stderr, invoke 'ctest -V'.
Sadly I haven't seen a way to inject this into the CMakeLists.txt file.


