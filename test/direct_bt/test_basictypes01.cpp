#include <iostream>
#include <cassert>
#include <cinttypes>
#include <cstring>

#include <cppunit.h>

#include <direct_bt/BasicTypes.hpp>
#include <direct_bt/BTAddress.hpp>

using namespace direct_bt;

// Test examples.
class Cppunit_tests : public Cppunit {
  private:

    void test_int32_t(const std::string msg, const int32_t v, const int expStrLen, const std::string expStr) {
        const std::string str = int32SeparatedString(v);
        PRINTM(msg+": has '"+str+"', len "+std::to_string(str.length()));
        PRINTM(msg+": exp '"+expStr+"', len "+std::to_string(expStr.length())+", equal: "+std::to_string(str==expStr));
        CHECKM(msg, str.length(), expStrLen);
        CHECKTM(msg, str == expStr);
    }

    void test_uint32_t(const std::string msg, const uint32_t v, const int expStrLen, const std::string expStr) {
        const std::string str = uint32SeparatedString(v);
        PRINTM(msg+": has '"+str+"', len "+std::to_string(str.length()));
        PRINTM(msg+": exp '"+expStr+"', len "+std::to_string(expStr.length())+", equal: "+std::to_string(str==expStr));
        CHECKM(msg, str.length(), expStrLen);
        CHECKTM(msg, str == expStr);
    }

    void test_uint64_t(const std::string msg, const uint64_t v, const int expStrLen, const std::string expStr) {
        const std::string str = uint64SeparatedString(v);
        PRINTM(msg+": has '"+str+"', len "+std::to_string(str.length()));
        PRINTM(msg+": exp '"+expStr+"', len "+std::to_string(expStr.length())+", equal: "+std::to_string(str==expStr));
        CHECKM(msg, str.length(), expStrLen);
        CHECKTM(msg, str == expStr);
    }

  public:
    void single_test() override {
        {
            test_int32_t("INT32_MIN", INT32_MIN, 14, "-2,147,483,648");
            test_int32_t("int32_t -thousand", -1000, 6, "-1,000");
            test_int32_t("int32_t one", 1, 1, "1");
            test_int32_t("int32_t thousand", 1000, 5, "1,000");
            test_int32_t("INT32_MAX", INT32_MAX, 13, "2,147,483,647");

            test_uint32_t("UINT32_MIN", 0, 1, "0");
            test_uint32_t("uint32_t one", 1, 1, "1");
            test_uint32_t("uint32_t thousand", 1000, 5, "1,000");
            test_uint32_t("UINT32_MAX", UINT32_MAX, 13, "4,294,967,295");

            test_uint64_t("UINT64_MIN", 0, 1, "0");
            test_uint64_t("uint64_t one", 1, 1, "1");
            test_uint64_t("uint64_t thousand", 1000, 5, "1,000");
            test_uint64_t("UINT64_MAX", UINT64_MAX, 26, "18,446,744,073,709,551,615");
        }
        {
            EUI48 mac01;
            PRINTM("EUI48 size: whole0 "+std::to_string(sizeof(EUI48)));
            PRINTM("EUI48 size: whole1 "+std::to_string(sizeof(mac01)));
            PRINTM("EUI48 size:  data1 "+std::to_string(sizeof(mac01.b)));
            CHECKM("EUI48 struct and data size not matching", sizeof(EUI48), sizeof(mac01));
            CHECKM("EUI48 struct and data size not matching", sizeof(mac01), sizeof(mac01.b));
        }

    }
};

int main(int argc, char *argv[]) {
    Cppunit_tests test1;
    return test1.run();
}

