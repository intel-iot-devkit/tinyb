#include <iostream>
#include <cassert>
#include <cinttypes>
#include <cstring>

#include <cppunit.h>

#include <direct_bt/BasicTypes.hpp>
#include <direct_bt/BTAddress.hpp>
#include <direct_bt/FunctionDef.hpp>

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

    int func2a_member(int i) {
        int res = i+100;
        return res;;
    }
    int func2b_member(int i) {
        int res = i+1000;
        return res;;
    }
    static int Func3a_static(int i) {
        int res = i+100;
        return res;;
    }
    static int Func3b_static(int i) {
        int res = i+1000;
        return res;;
    }

    // template<typename R, typename... A>
    typedef FunctionDef<int, int> MyClassFunction;

    void test_FunctionPointer00(std::string msg, bool expEqual, int value, int expRes, MyClassFunction & f1, MyClassFunction &f2) {
        // test std::function identity
        PRINTM(msg+": FunctionPointer00 Fun f1p == f2p : " + std::to_string( f1 == f2 ) + ", f1p: " + f1.toString() + ", f2 "+f2.toString() );
        int f1r = f1.invoke(value);
        int f2r = f2.invoke(value);
        PRINTM(msg+": FunctionPointer00 Res f1r == f2r : " + std::to_string( f1r == f2r ) + ", f1r: " + std::to_string( f1r ) + ", f2r "+std::to_string( f2r ) );
        if( expEqual ) {
            CHECKM(msg, f1r, expRes);
            CHECKM(msg, f2r, expRes);
            CHECKTM(msg, f1 == f2);
        } else {
            CHECKTM(msg, f1 != f2);
        }
    }

  public:
    void single_test() override {

        {
            // FunctionDef(Cppunit_tests &base, Func1Type func)
            MyClassFunction f2a_1 = bindMemberFunc<int, Cppunit_tests, int>(this, &Cppunit_tests::func2a_member);
            MyClassFunction f2a_2 = bindMemberFunc(this, &Cppunit_tests::func2a_member);
            test_FunctionPointer00("FuncPtr2a_member_11", true, 1, 101, f2a_1, f2a_1);
            test_FunctionPointer00("FuncPtr2a_member_12", true, 1, 101, f2a_1, f2a_2);

            MyClassFunction f2b_1 = bindMemberFunc(this, &Cppunit_tests::func2b_member);
            MyClassFunction f2b_2 = bindMemberFunc(this, &Cppunit_tests::func2b_member);
            test_FunctionPointer00("FuncPtr2b_member_11", true, 1, 1001, f2b_1, f2b_1);
            test_FunctionPointer00("FuncPtr2b_member_12", true, 1, 1001, f2b_1, f2b_2);

            test_FunctionPointer00("FuncPtr2ab_member_11", false, 1, 0, f2a_1, f2b_1);
            test_FunctionPointer00("FuncPtr2ab_member_22", false, 1, 0, f2a_2, f2b_2);
        }
        {
            // FunctionDef(Func1Type func)
            MyClassFunction f3a_1 = bindPlainFunc<int, int>(&Cppunit_tests::Func3a_static);
            MyClassFunction f3a_2 = bindPlainFunc(&Cppunit_tests::Func3a_static);
            test_FunctionPointer00("FuncPtr3a_static_11", true, 1, 101, f3a_1, f3a_1);
            test_FunctionPointer00("FuncPtr3a_static_12", true, 1, 101, f3a_1, f3a_2);

            MyClassFunction f3b_1 = bindPlainFunc(&Cppunit_tests::Func3b_static);
            MyClassFunction f3b_2 = bindPlainFunc(&Func3b_static);
            test_FunctionPointer00("FuncPtr3b_static_11", true, 1, 1001, f3b_1, f3b_1);
            test_FunctionPointer00("FuncPtr3b_static_12", true, 1, 1001, f3b_1, f3b_2);

            test_FunctionPointer00("FuncPtr3ab_static_11", false, 1, 0, f3a_1, f3b_1);
            test_FunctionPointer00("FuncPtr3ab_static_22", false, 1, 0, f3a_2, f3b_2);
        }
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

