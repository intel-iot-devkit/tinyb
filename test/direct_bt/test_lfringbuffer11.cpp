#include <iostream>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <memory>
#include <thread>
#include <pthread.h>

#include <cppunit.h>

#include <direct_bt/UUID.hpp>
#include <direct_bt/Ringbuffer.hpp>
#include <direct_bt/LFRingbuffer.hpp>

using namespace direct_bt;

class Integer {
    public:
        int value;

        Integer(int v) : value(v) {}

        Integer(const Integer &o) noexcept = default;
        Integer(Integer &&o) noexcept = default;
        Integer& operator=(const Integer &o) noexcept = default;
        Integer& operator=(Integer &&o) noexcept = default;

        operator int() const {
            return value;
        }
        int intValue() const { return value; }
        static Integer valueOf(const int i) { return Integer(i); }
};

std::shared_ptr<Integer> NullInteger = nullptr;

typedef std::shared_ptr<Integer> SharedType;
typedef Ringbuffer<SharedType> SharedTypeRingbuffer;
typedef LFRingbuffer<SharedType, nullptr> SharedTypeLFRingbuffer;

// Test examples.
class Cppunit_tests : public Cppunit {
  private:

    std::shared_ptr<SharedTypeRingbuffer> createEmpty(int initialCapacity) {
        return std::shared_ptr<SharedTypeRingbuffer>(new SharedTypeLFRingbuffer(initialCapacity));
    }
    std::shared_ptr<SharedTypeRingbuffer> createFull(const std::vector<std::shared_ptr<Integer>> & source) {
        return std::shared_ptr<SharedTypeRingbuffer>(new SharedTypeLFRingbuffer(source));
    }

    std::vector<SharedType> createIntArray(const int capacity, const int startValue) {
        std::vector<SharedType> array(capacity);
        for(int i=0; i<capacity; i++) {
            array[i] = SharedType(new Integer(startValue+i));
        }
        return array;
    }

    void getThreadType01(const std::string msg, std::shared_ptr<Ringbuffer<SharedType>> rb, int len, int startValue) {
        // std::thread::id this_id = std::this_thread::get_id();
        // pthread_t this_id = pthread_self();

        fprintf(stderr, "%s: Created / %s\n", msg.c_str(), rb->toString().c_str());
        for(int i=0; i<len; i++) {
            SharedType svI = rb->getBlocking();
            CHECKTM(msg+": Empty at read #"+std::to_string(i+1)+": "+rb->toString(), svI!=nullptr);
            fprintf(stderr, "%s: Got %d / %s\n",
                    msg.c_str(), svI->intValue(), rb->toString().c_str());
            if( 0 <= startValue ) {
                CHECKM(msg+": %s: Wrong value at read #"+std::to_string(i+1)+": "+rb->toString(), startValue+i, svI->intValue());
            }
        }
        fprintf(stderr, "%s: Dies / %s\n", msg.c_str(), rb->toString().c_str());
    }

    void putThreadType01(const std::string msg, std::shared_ptr<Ringbuffer<SharedType>> rb, int len, int startValue) {
        // std::thread::id this_id = std::this_thread::get_id();
        // pthread_t this_id = pthread_self();

        fprintf(stderr, "%s: Created / %s\n", msg.c_str(), rb->toString().c_str());
        int preSize = rb->getSize();
        (void)preSize;

        for(int i=0; i<len; i++) {
            Integer * vI = new Integer(startValue+i);
            fprintf(stderr, "%s: Putting %d ... / %s\n",
                    msg.c_str(), vI->intValue(), rb->toString().c_str());
            rb->putBlocking( SharedType( vI ) );
        }
        fprintf(stderr, "%s: Dies / %s\n", msg.c_str(), rb->toString().c_str());
    }

  public:

    void test01_Read1Write1() {
        fprintf(stderr, "\n\ntest01_Read1Write1\n");
        int capacity = 100;
        std::shared_ptr<SharedTypeRingbuffer> rb = createEmpty(capacity);
        CHECKM("Not empty size "+rb->toString(), 0, rb->getSize());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        std::thread getThread01(&Cppunit_tests::getThreadType01, this, "test01.get01", rb, capacity, 0);
        std::thread putThread01(&Cppunit_tests::putThreadType01, this, "test01.put01", rb, capacity, 0);
        putThread01.join();
        getThread01.join();

        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
        CHECKM("Not empty size "+rb->toString(), 0, rb->getSize());
    }

    void test02_Read4Write1() {
        fprintf(stderr, "\n\ntest02_Read4Write1\n");
        int capacity = 400;
        std::shared_ptr<SharedTypeRingbuffer> rb = createEmpty(capacity);
        CHECKM("Not empty size "+rb->toString(), 0, rb->getSize());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        std::thread getThread01(&Cppunit_tests::getThreadType01, this, "test02.get01", rb, capacity/4, -1);
        std::thread getThread02(&Cppunit_tests::getThreadType01, this, "test02.get02", rb, capacity/4, -1);
        std::thread putThread01(&Cppunit_tests::putThreadType01, this, "test02.put01", rb, capacity, 0);
        std::thread getThread03(&Cppunit_tests::getThreadType01, this, "test02.get03", rb, capacity/4, -1);
        std::thread getThread04(&Cppunit_tests::getThreadType01, this, "test02.get04", rb, capacity/4, -1);
        putThread01.join();
        getThread01.join();
        getThread02.join();
        getThread03.join();
        getThread04.join();

        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
        CHECKM("Not empty size "+rb->toString(), 0, rb->getSize());
    }

    void test03_Read8Write2() {
        fprintf(stderr, "\n\ntest03_Read8Write2\n");
        int capacity = 800;
        std::shared_ptr<SharedTypeRingbuffer> rb = createEmpty(capacity);
        CHECKM("Not empty size "+rb->toString(), 0, rb->getSize());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        std::thread getThread01(&Cppunit_tests::getThreadType01, this, "test03.get01", rb, capacity/8, -1);
        std::thread getThread02(&Cppunit_tests::getThreadType01, this, "test03.get02", rb, capacity/8, -1);
        std::thread putThread01(&Cppunit_tests::putThreadType01, this, "test03.put01", rb, capacity/2,  0);
        std::thread getThread03(&Cppunit_tests::getThreadType01, this, "test03.get03", rb, capacity/8, -1);
        std::thread getThread04(&Cppunit_tests::getThreadType01, this, "test03.get04", rb, capacity/8, -1);

        std::thread getThread05(&Cppunit_tests::getThreadType01, this, "test03.get05", rb, capacity/8, -1);
        std::thread getThread06(&Cppunit_tests::getThreadType01, this, "test03.get06", rb, capacity/8, -1);
        std::thread putThread02(&Cppunit_tests::putThreadType01, this, "test03.put02", rb, capacity/2,  400);
        std::thread getThread07(&Cppunit_tests::getThreadType01, this, "test03.get07", rb, capacity/8, -1);
        std::thread getThread08(&Cppunit_tests::getThreadType01, this, "test03.get08", rb, capacity/8, -1);

        putThread01.join();
        putThread02.join();
        getThread01.join();
        getThread02.join();
        getThread03.join();
        getThread04.join();
        getThread05.join();
        getThread06.join();
        getThread07.join();
        getThread08.join();

        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
        CHECKM("Not empty size "+rb->toString(), 0, rb->getSize());
    }

    void test_list() override {
        test01_Read1Write1();
        test02_Read4Write1();
        test03_Read8Write2();
    }
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    Cppunit_tests test1;
    return test1.run();
}

