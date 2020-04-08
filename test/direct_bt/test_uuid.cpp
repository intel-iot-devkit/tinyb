#include <iostream>
#include <cassert>
#include <cinttypes>
#include <cstring>

#include <cppunit.h>

#include <direct_bt/UUID.hpp>

using namespace direct_bt;

// Test examples.
class Cppunit_tests : public Cppunit {
  public:
    void single_test() override {

        std::cout << "Hello COUT" << std::endl;
        std::cerr << "Hello CERR" << std::endl;

        uint8_t buffer[100];
        static uint8_t uuid128_bytes[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                                           0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

        {
            const uuid128_t v01 = uuid128_t(uuid128_bytes, 0, true);
            CHECK(v01.getTypeSize(), 16);
            CHECK(v01.getTypeSize(), sizeof(v01.value));
            CHECK(v01.getTypeSize(), sizeof(v01.value.data));
            CHECKT( 0 == memcmp(uuid128_bytes, v01.data(), 16) )

            put_uuid(buffer, 0, v01, true);
            std::shared_ptr<const uuid_t> v02 = uuid_t::create(uuid_t::TypeSize::UUID128_SZ, buffer, 0, true);
            CHECK(v02->getTypeSize(), 16);
            CHECKT( 0 == memcmp(v01.data(), v02->data(), 16) )
            CHECKT( v01.toString() == v02->toString() );
        }

        {
            const uuid32_t v01 = uuid32_t(uuid32_t(0x12345678));
            CHECK(v01.getTypeSize(), 4);
            CHECK(v01.getTypeSize(), sizeof(v01.value));
            CHECK(0x12345678, v01.value);

            put_uuid(buffer, 0, v01, true);
            std::shared_ptr<const uuid_t> v02 = uuid_t::create(uuid_t::TypeSize::UUID32_SZ, buffer, 0, true);
            CHECK(v02->getTypeSize(), 4);
            CHECKT( 0 == memcmp(v01.data(), v02->data(), 4) )
            CHECKT( v01.toString() == v02->toString() );
        }

        {
            const uuid16_t v01 = uuid16_t(uuid16_t(0x1234));
            CHECK(v01.getTypeSize(), 2);
            CHECK(v01.getTypeSize(), sizeof(v01.value));
            CHECK(0x1234, v01.value);

            put_uuid(buffer, 0, v01, true);
            std::shared_ptr<const uuid_t> v02 = uuid_t::create(uuid_t::TypeSize::UUID16_SZ, buffer, 0, true);
            CHECK(v02->getTypeSize(), 2);
            CHECKT( 0 == memcmp(v01.data(), v02->data(), 2) )
            CHECKT( v01.toString() == v02->toString() );
        }
    }
};

int main(int argc, char *argv[]) {
    Cppunit_tests test1;
    return test1.run();
}

