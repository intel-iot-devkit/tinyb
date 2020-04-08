#include <iostream>
#include <cassert>
#include <cinttypes>
#include <cstring>

#include <cppunit.h>

#include <direct_bt/UUID.hpp>
// #include <direct_bt/BTAddress.hpp>
// #include <direct_bt/HCITypes.hpp>
#include <direct_bt/ATTPDUTypes.hpp>
// #include <direct_bt/GATTHandler.hpp>
// #include <direct_bt/GATTIoctl.hpp>

using namespace direct_bt;

// Test examples.
class Cppunit_tests: public Cppunit {
    void single_test() override {
        const uuid16_t uuid16 = uuid16_t(uuid16_t(0x1234));
        const AttReadByNTypeReq req(true /* group */, 1, 0xffff, uuid16);

        std::shared_ptr<const uuid_t> uuid16_2 = req.getNType();
        CHECK(uuid16.getTypeSize(), 2);
        CHECK(uuid16_2->getTypeSize(), 2);
        CHECKT( 0 == memcmp(uuid16.data(), uuid16_2->data(), 2) )
        CHECKT( uuid16.toString() == uuid16_2->toString() );

        CHECK(req.getStartHandle(), 1);
        CHECK(req.getEndHandle(), 0xffff);
    }
};

int main(int argc, char *argv[]) {
    Cppunit_tests test1;
    return test1.run();
}

