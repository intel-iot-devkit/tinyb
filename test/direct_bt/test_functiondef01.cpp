#include <iostream>
#include <cassert>
#include <cinttypes>
#include <cstring>

#include <cppunit.h>

#include <direct_bt/FunctionDef.hpp>

using namespace direct_bt;

// Test examples.
class Cppunit_tests : public Cppunit {
  private:

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

    struct IntOffset {
        int value;
        IntOffset(int v) : value(v) {}

        IntOffset(const IntOffset &o)
        : value(o.value)
        {
            PRINTM("IntOffset::copy_ctor");
        }
        IntOffset(IntOffset &&o)
        : value(std::move(o.value))
        {
            PRINTM("IntOffset::move_ctor");
        }
        IntOffset& operator=(const IntOffset &o) {
            PRINTM("IntOffset::copy_assign");
            if( &o == this ) {
                return *this;
            }
            value = o.value;
            return *this;
        }
        IntOffset& operator=(IntOffset &&o) {
            PRINTM("IntOffset::move_assign");
            value = std::move(o.value);
            (void)value;
            return *this;
        }

        bool operator==(const IntOffset& rhs) const {
            if( &rhs == this ) {
                return true;
            }
            return value == rhs.value;
        }

        bool operator!=(const IntOffset& rhs) const
        { return !( *this == rhs ); }

    };

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
    void test_FunctionPointer01(std::string msg, bool expEqual, MyClassFunction & f1, MyClassFunction &f2) {
        // test std::function identity
        PRINTM(msg+": FunctionPointer00 Fun f1p == f2p : " + std::to_string( f1 == f2 ) + ", f1p: " + f1.toString() + ", f2 "+f2.toString() );
        if( expEqual ) {
            CHECKTM(msg, f1 == f2);
        } else {
            CHECKTM(msg, f1 != f2);
        }
    }

  public:
    void single_test() override {

        {
            PRINTM("FuncPtr2_member: bindMemberFunc<int, Cppunit_tests, int>: START");
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
            PRINTM("FuncPtr2_member: bindMemberFunc<int, Cppunit_tests, int>: END");
        }
        {
            PRINTM("FuncPtr3_plain: bindPlainFunc<int, int>: START");
            // FunctionDef(Func1Type func)
            MyClassFunction f3a_1 = bindPlainFunc<int, int>(&Cppunit_tests::Func3a_static);
            MyClassFunction f3a_2 = bindPlainFunc(&Cppunit_tests::Func3a_static);
            test_FunctionPointer00("FuncPtr3a_plain_11", true, 1, 101, f3a_1, f3a_1);
            test_FunctionPointer00("FuncPtr3a_plain_12", true, 1, 101, f3a_1, f3a_2);

            MyClassFunction f3b_1 = bindPlainFunc(&Cppunit_tests::Func3b_static);
            MyClassFunction f3b_2 = bindPlainFunc(&Func3b_static);
            test_FunctionPointer00("FuncPtr3b_plain_11", true, 1, 1001, f3b_1, f3b_1);
            test_FunctionPointer00("FuncPtr3b_plain_12", true, 1, 1001, f3b_1, f3b_2);

            test_FunctionPointer00("FuncPtr3ab_plain_11", false, 1, 0, f3a_1, f3b_1);
            test_FunctionPointer00("FuncPtr3ab_plain_22", false, 1, 0, f3a_2, f3b_2);
            PRINTM("FuncPtr3_plain: bindPlainFunc<int, int>: END");
        }
        {
            PRINTM("FuncPtr4_stdlambda: bindStdFunc<int, int>: START");
            // FunctionDef(Func1Type func) <int, int>
            std::function<int(int i)> func4a_stdlambda = [](int i)->int {
                int res = i+100;
                return res;;
            };
            std::function<int(int i)> func4b_stdlambda = [](int i)->int {
                int res = i+1000;
                return res;;
            };
            MyClassFunction f4a_1 = bindStdFunc<int, int>(100, func4a_stdlambda);
            MyClassFunction f4a_2 = bindStdFunc(100, func4a_stdlambda);
            test_FunctionPointer00("FuncPtr4a_stdlambda_11", true, 1, 101, f4a_1, f4a_1);
            test_FunctionPointer00("FuncPtr4a_stdlambda_12", true, 1, 101, f4a_1, f4a_2);

            MyClassFunction f4b_1 = bindStdFunc(200, func4b_stdlambda);
            MyClassFunction f4b_2 = bindStdFunc(200, func4b_stdlambda);
            test_FunctionPointer00("FuncPtr4b_stdlambda_11", true, 1, 1001, f4b_1, f4b_1);
            test_FunctionPointer00("FuncPtr4b_stdlambda_12", true, 1, 1001, f4b_1, f4b_2);

            test_FunctionPointer00("FuncPtr4ab_stdlambda_11", false, 1, 0, f4a_1, f4b_1);
            test_FunctionPointer00("FuncPtr4ab_stdlambda_22", false, 1, 0, f4a_2, f4b_2);

            MyClassFunction f4a_0 = bindStdFunc<int, int>(100);
            MyClassFunction f4b_0 = bindStdFunc<int, int>(200);
            test_FunctionPointer01("FuncPtr4a_stdlambda_01", true, f4a_0, f4a_1);
            test_FunctionPointer01("FuncPtr4a_stdlambda_02", true, f4a_0, f4a_2);
            test_FunctionPointer01("FuncPtr4b_stdlambda_01", true, f4b_0, f4b_1);
            test_FunctionPointer01("FuncPtr4b_stdlambda_02", true, f4b_0, f4b_2);
            test_FunctionPointer01("FuncPtr4ab_stdlambda_00", false, f4a_0, f4b_0);
            test_FunctionPointer01("FuncPtr4ab_stdlambda_01", false, f4a_0, f4b_1);
            test_FunctionPointer01("FuncPtr4ab_stdlambda_10", false, f4a_1, f4b_0);
            PRINTM("FuncPtr4_stdlambda: bindStdFunc<int, int>: END");
        }
        {
            PRINTM("FuncPtr5_capture: bindCaptureFunc<int, int, int>: START");
            // bindCaptureFunc(I& data, R(*func)(I&, A...))
            // FunctionDef(Func1Type func) <int, int>
            int offset100 = 100;
            int offset1000 = 1000;

            int(*func5a_capture)(int&, int) = [](int& offset, int i)->int {
                int res = i+10000+offset;
                return res;
            };
            int(*func5b_capture)(int&, int) = [](int& offset, int i)->int {
                int res = i+100000+offset;
                return res;
            };

#if 0
            MyClassFunction f5a_o100_0 = bindCaptureFunc<int, int, int>(offset100,
                    [](int& offset, int i)->int {
                        int res = i+10000+offset;
                        return res;;
                    } );
            test_FunctionPointer01("FuncPtr5a_o100_capture_00", true, f5a_o100_0, f5a_o100_0);
#endif
            MyClassFunction f5a_o100_1 = bindCaptureFunc<int, int, int>(offset100, func5a_capture);
            MyClassFunction f5a_o100_2 = bindCaptureFunc(offset100, func5a_capture);
            test_FunctionPointer01("FuncPtr5a_o100_capture_12", true, f5a_o100_1, f5a_o100_2);
            test_FunctionPointer00("FuncPtr5a_o100_capture_11", true, 1, 10101, f5a_o100_1, f5a_o100_1);
            test_FunctionPointer00("FuncPtr5a_o100_capture_12", true, 1, 10101, f5a_o100_1, f5a_o100_2);
            // test_FunctionPointer01("FuncPtr5a_o100_capture_01", false, f5a_o100_0, f5a_o100_1);
            MyClassFunction f5a_o1000_1 = bindCaptureFunc(offset1000, func5a_capture);
            MyClassFunction f5a_o1000_2 = bindCaptureFunc(offset1000, func5a_capture);
            test_FunctionPointer01("FuncPtr5a_o1000_capture_12", true, f5a_o1000_1, f5a_o1000_2);
            test_FunctionPointer01("FuncPtr5a_o100_o1000_capture_11", false, f5a_o100_1, f5a_o1000_1);

            MyClassFunction f5b_o100_1 = bindCaptureFunc(offset100, func5b_capture);
            MyClassFunction f5b_o100_2 = bindCaptureFunc(offset100, func5b_capture);
            test_FunctionPointer00("FuncPtr5b_o100_capture_11", true, 1, 100101, f5b_o100_1, f5b_o100_1);
            test_FunctionPointer00("FuncPtr5b_o100_capture_12", true, 1, 100101, f5b_o100_1, f5b_o100_2);

            test_FunctionPointer00("FuncPtr5ab_o100_capture_11", false, 1, 0, f5a_o100_1, f5b_o100_1);
            test_FunctionPointer00("FuncPtr5ab_o100_capture_22", false, 1, 0, f5a_o100_2, f5b_o100_2);
            PRINTM("FuncPtr5_capture: bindCaptureFunc<int, int, int>: END");
        }
        {
            PRINTM("FuncPtr6_capture: bindCaptureFunc<int, std::shared_ptr<IntOffset>, int>: START");
            // bindCaptureFunc(I& data, R(*func)(I&, A...))
            // FunctionDef(Func1Type func) <int, int>
            std::shared_ptr<IntOffset> offset100(new IntOffset(100));
            std::shared_ptr<IntOffset> offset1000(new IntOffset(1000));

            int(*func6a_capture)(std::shared_ptr<IntOffset>&, int) = [](std::shared_ptr<IntOffset>& sharedOffset, int i)->int {
                int res = i+10000+sharedOffset->value;
                return res;
            };
            int(*func6b_capture)(std::shared_ptr<IntOffset>&, int) = [](std::shared_ptr<IntOffset>& sharedOffset, int i)->int {
                int res = i+100000+sharedOffset->value;
                return res;
            };

#if 0
            MyClassFunction f6a_o100_0 = bindCaptureFunc<int, std::shared_ptr<IntOffset>, int>(offset100,
                    [](std::shared_ptr<IntOffset>& sharedOffset, int i)->int {
                        int res = i+10000+sharedOffset->value;
                        return res;;
                    } );
            test_FunctionPointer01("FuncPtr6a_o100_capture_00", true, f6a_o100_0, f6a_o100_0);
#endif
            MyClassFunction f6a_o100_1 = bindCaptureFunc<int, std::shared_ptr<IntOffset>, int>(offset100, func6a_capture);
            MyClassFunction f6a_o100_2 = bindCaptureFunc(offset100, func6a_capture);
            test_FunctionPointer01("FuncPtr6a_o100_capture_12", true, f6a_o100_1, f6a_o100_2);
            test_FunctionPointer00("FuncPtr6a_o100_capture_11", true, 1, 10101, f6a_o100_1, f6a_o100_1);
            test_FunctionPointer00("FuncPtr6a_o100_capture_12", true, 1, 10101, f6a_o100_1, f6a_o100_2);
            // test_FunctionPointer01("FuncPtr6a_o100_capture_01", false, f6a_o100_0, f6a_o100_1);
            MyClassFunction f6a_o1000_1 = bindCaptureFunc(offset1000, func6a_capture);
            MyClassFunction f6a_o1000_2 = bindCaptureFunc(offset1000, func6a_capture);
            test_FunctionPointer01("FuncPtr6a_o1000_capture_12", true, f6a_o1000_1, f6a_o1000_2);
            test_FunctionPointer01("FuncPtr6a_o100_o1000_capture_11", false, f6a_o100_1, f6a_o1000_1);

            MyClassFunction f6b_o100_1 = bindCaptureFunc(offset100, func6b_capture);
            MyClassFunction f6b_o100_2 = bindCaptureFunc(offset100, func6b_capture);
            test_FunctionPointer00("FuncPtr6b_o100_capture_11", true, 1, 100101, f6b_o100_1, f6b_o100_1);
            test_FunctionPointer00("FuncPtr6b_o100_capture_12", true, 1, 100101, f6b_o100_1, f6b_o100_2);

            test_FunctionPointer00("FuncPtr6ab_o100_capture_11", false, 1, 0, f6a_o100_1, f6b_o100_1);
            test_FunctionPointer00("FuncPtr6ab_o100_capture_22", false, 1, 0, f6a_o100_2, f6b_o100_2);
            PRINTM("FuncPtr6_capture: bindCaptureFunc<int, std::shared_ptr<IntOffset>, int>: END");
        }
        {
            PRINTM("FuncPtr7_capture: bindCaptureFunc<int, IntOffset, int>: START");
            // bindCaptureFunc(I& data, R(*func)(I&, A...))
            // FunctionDef(Func1Type func) <int, int>
            IntOffset offset100(100);
            IntOffset offset1000(1000);

            int(*func7a_capture)(IntOffset&, int) = [](IntOffset& sharedOffset, int i)->int {
                int res = i+10000+sharedOffset.value;
                return res;
            };
            int(*func7b_capture)(IntOffset&, int) = [](IntOffset& sharedOffset, int i)->int {
                int res = i+100000+sharedOffset.value;
                return res;
            };

#if 0
            MyClassFunction f7a_o100_0 = bindCaptureFunc<int, IntOffset, int>(offset100,
                    [](IntOffset& sharedOffset, int i)->int {
                        int res = i+10000+sharedOffset.value;
                        return res;;
                    } );
            test_FunctionPointer01("FuncPtr7a_o100_capture_00", true, f7a_o100_0, f7a_o100_0);
#endif
            PRINTM("f7a_o100_1 copy_ctor");
            MyClassFunction f7a_o100_1 = bindCaptureFunc<int, IntOffset, int>(offset100, func7a_capture);
            PRINTM("f7a_o100_1 copy_ctor done");
            PRINTM("f7a_o100_2 move_ctor");
            MyClassFunction f7a_o100_2 = bindCaptureFunc(IntOffset(100), func7a_capture);
            PRINTM("f7a_o100_2 move_ctor done");
            test_FunctionPointer01("FuncPtr7a_o100_capture_12", true, f7a_o100_1, f7a_o100_2);
            test_FunctionPointer00("FuncPtr7a_o100_capture_11", true, 1, 10101, f7a_o100_1, f7a_o100_1);
            test_FunctionPointer00("FuncPtr7a_o100_capture_12", true, 1, 10101, f7a_o100_1, f7a_o100_2);
            // test_FunctionPointer01("FuncPtr7a_o100_capture_01", false, f7a_o100_0, f7a_o100_1);
            MyClassFunction f7a_o1000_1 = bindCaptureFunc(offset1000, func7a_capture);
            MyClassFunction f7a_o1000_2 = bindCaptureFunc(offset1000, func7a_capture);
            test_FunctionPointer01("FuncPtr7a_o1000_capture_12", true, f7a_o1000_1, f7a_o1000_2);
            test_FunctionPointer01("FuncPtr7a_o100_o1000_capture_11", false, f7a_o100_1, f7a_o1000_1);

            MyClassFunction f7b_o100_1 = bindCaptureFunc(offset100, func7b_capture);
            MyClassFunction f7b_o100_2 = bindCaptureFunc(offset100, func7b_capture);
            test_FunctionPointer00("FuncPtr7b_o100_capture_11", true, 1, 100101, f7b_o100_1, f7b_o100_1);
            test_FunctionPointer00("FuncPtr7b_o100_capture_12", true, 1, 100101, f7b_o100_1, f7b_o100_2);

            test_FunctionPointer00("FuncPtr7ab_o100_capture_11", false, 1, 0, f7a_o100_1, f7b_o100_1);
            test_FunctionPointer00("FuncPtr7ab_o100_capture_22", false, 1, 0, f7a_o100_2, f7b_o100_2);
            PRINTM("FuncPtr7_capture: bindCaptureFunc<int, IntOffset, int>: END");
        }
    }
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    Cppunit_tests test1;
    return test1.run();
}

