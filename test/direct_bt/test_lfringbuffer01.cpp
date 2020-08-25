#include <iostream>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <memory>

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

    void readTestImpl(Ringbuffer<SharedType> &rb, bool clearRef, int capacity, int len, int startValue) {
        (void) clearRef;

        int preSize = rb.getSize();
        CHECKM("Wrong capacity "+rb.toString(), capacity, rb.capacity());
        CHECKTM("Too low capacity to read "+std::to_string(len)+" elems: "+rb.toString(), capacity-len >= 0);
        CHECKTM("Too low size to read "+std::to_string(len)+" elems: "+rb.toString(), preSize >= len);
        CHECKTM("Is empty "+rb.toString(), !rb.isEmpty());

        for(int i=0; i<len; i++) {
            SharedType svI = rb.get();
            CHECKTM("Empty at read #"+std::to_string(i+1)+": "+rb.toString(), svI!=nullptr);
            CHECKM("Wrong value at read #"+std::to_string(i+1)+": "+rb.toString(), startValue+i, svI->intValue());
        }

        CHECKM("Invalid size "+rb.toString(), preSize-len, rb.getSize());
        CHECKTM("Invalid free slots after reading "+std::to_string(len)+": "+rb.toString(), rb.getFreeSlots()>= len);
        CHECKTM("Is full "+rb.toString(), !rb.isFull());
    }

    void writeTestImpl(Ringbuffer<SharedType> &rb, int capacity, int len, int startValue) {
        int preSize = rb.getSize();

        CHECKM("Wrong capacity "+rb.toString(), capacity, rb.capacity());
        CHECKTM("Too low capacity to write "+std::to_string(len)+" elems: "+rb.toString(), capacity-len >= 0);
        CHECKTM("Too low size to write "+std::to_string(len)+" elems: "+rb.toString(), preSize+len <= capacity);
        CHECKTM("Is full "+rb.toString(), !rb.isFull());

        for(int i=0; i<len; i++) {
            std::string m = "Buffer is full at put #"+std::to_string(i)+": "+rb.toString();
            CHECKTM(m, rb.put( SharedType( new Integer(startValue+i) ) ) );
        }

        CHECKM("Invalid size "+rb.toString(), preSize+len, rb.getSize());
        CHECKTM("Is empty "+rb.toString(), !rb.isEmpty());
    }

    void moveGetPutImpl(Ringbuffer<SharedType> &rb, int pos) {
        CHECKTM("RB is empty "+rb.toString(), !rb.isEmpty());
        for(int i=0; i<pos; i++) {
            CHECKM("MoveFull.get failed "+rb.toString(), i, rb.get()->intValue());
            CHECKTM("MoveFull.put failed "+rb.toString(), rb.put( SharedType( new Integer(i) ) ) );
        }
    }

    void movePutGetImpl(Ringbuffer<SharedType> &rb, int pos) {
        CHECKTM("RB is full "+rb.toString(), !rb.isFull());
        for(int i=0; i<pos; i++) {
            CHECKTM("MoveEmpty.put failed "+rb.toString(), rb.put( SharedType( new Integer(600+i) ) ) );
            CHECKM("MoveEmpty.get failed "+rb.toString(), 600+i, rb.get()->intValue());
        }
    }

    void test01_FullRead() {
        int capacity = 11;
        std::vector<SharedType> source = createIntArray(capacity, 0);
        std::shared_ptr<SharedTypeRingbuffer> rb = createFull(source);
        fprintf(stderr, "test01_FullRead: Created / %s\n", rb->toString().c_str());
        CHECKM("Not full size "+rb->toString(), capacity, rb->getSize());
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, true, capacity, capacity, 0);
        fprintf(stderr, "test01_FullRead: PostRead / %s\n", rb->toString().c_str());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
    }

    void test02_EmptyWrite() {
        int capacity = 11;
        std::shared_ptr<Ringbuffer<SharedType>> rb = createEmpty(capacity);
        fprintf(stderr, "test01_EmptyWrite: Created / %s\n", rb->toString().c_str());
        CHECKM("Not zero size "+rb->toString(), 0, rb->getSize());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        writeTestImpl(*rb, capacity, capacity, 0);
        fprintf(stderr, "test01_EmptyWrite: PostWrite / %s\n", rb->toString().c_str());
        CHECKM("Not full size "+rb->toString(), capacity, rb->getSize());
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, true, capacity, capacity, 0);
        fprintf(stderr, "test01_EmptyWrite: PostRead / %s\n", rb->toString().c_str());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
    }

    void test03_FullReadReset() {
        int capacity = 11;
        std::vector<SharedType> source = createIntArray(capacity, 0);
        std::shared_ptr<Ringbuffer<SharedType>> rb = createFull(source);
        fprintf(stderr, "test01_FullReadReset: Created / %s\n", rb->toString().c_str());
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        rb->reset(source);
        fprintf(stderr, "test01_FullReadReset: Post Reset w/ source / %s\n", rb->toString().c_str());
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, false, capacity, capacity, 0);
        fprintf(stderr, "test01_FullReadReset: Post Read / %s\n", rb->toString().c_str());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        rb->reset(source);
        fprintf(stderr, "test01_FullReadReset: Post Reset w/ source / %s\n", rb->toString().c_str());
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, false, capacity, capacity, 0);
        fprintf(stderr, "test01_FullReadReset: Post Read / %s\n", rb->toString().c_str());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
    }

    void test04_EmptyWriteClear() {
        int capacity = 11;
        std::shared_ptr<Ringbuffer<SharedType>> rb = createEmpty(capacity);
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        rb->clear();
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        writeTestImpl(*rb, capacity, capacity, 0);
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, false, capacity, capacity, 0);
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        rb->clear();
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());

        writeTestImpl(*rb, capacity, capacity, 0);
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, false, capacity, capacity, 0);
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
    }

    void test05_ReadResetMid01() {
        int capacity = 11;
        std::vector<SharedType> source = createIntArray(capacity, 0);
        std::shared_ptr<Ringbuffer<SharedType>> rb = createFull(source);
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        rb->reset(source);
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, false, capacity, 5, 0);
        CHECKTM("Is empty "+rb->toString(), !rb->isEmpty());
        CHECKTM("Is Full "+rb->toString(), !rb->isFull());

        rb->reset(source);
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, false, capacity, capacity, 0);
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
    }

    void test06_ReadResetMid02() {
        int capacity = 11;
        std::vector<SharedType> source = createIntArray(capacity, 0);
        std::shared_ptr<Ringbuffer<SharedType>> rb = createFull(source);
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        rb->reset(source);
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        moveGetPutImpl(*rb, 5);
        readTestImpl(*rb, false, capacity, 5, 5);
        CHECKTM("Is empty "+rb->toString(), !rb->isEmpty());
        CHECKTM("Is Full "+rb->toString(), !rb->isFull());

        rb->reset(source);
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        readTestImpl(*rb, false, capacity, capacity, 0);
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
    }

    void test_GrowFullImpl(int initialCapacity, int pos) {
        int growAmount = 5;
        int grownCapacity = initialCapacity+growAmount;
        std::vector<SharedType> source = createIntArray(initialCapacity, 0);
        std::shared_ptr<Ringbuffer<SharedType>> rb = createFull(source);

        for(int i=0; i<initialCapacity; i++) {
            SharedType svI = rb->get();
            CHECKTM("Empty at read #"+std::to_string(i+1)+": "+rb->toString(), svI!=nullptr);
            CHECKM("Wrong value at read #"+std::to_string(i+1)+": "+rb->toString(), (0+i)%initialCapacity, svI->intValue());
        }
        CHECKM("Not zero size "+rb->toString(), 0, rb->getSize());

        rb->reset(source);
        CHECKM("Not orig size "+rb->toString(), initialCapacity, rb->getSize());

        moveGetPutImpl(*rb, pos);
        // PRINTM("X02 "+rb->toString());
        // rb->dump(stderr, "X02");

        rb->recapacity(grownCapacity);
        CHECKM("Wrong capacity "+rb->toString(), grownCapacity, rb->capacity());
        CHECKM("Not orig size "+rb->toString(), initialCapacity, rb->getSize());
        CHECKTM("Is full "+rb->toString(), !rb->isFull());
        CHECKTM("Is empty "+rb->toString(), !rb->isEmpty());
        // PRINTM("X03 "+rb->toString());
        // rb->dump(stderr, "X03");

        for(int i=0; i<growAmount; i++) {
            CHECKTM("Buffer is full at put #"+std::to_string(i)+": "+rb->toString(), rb->put( SharedType( new Integer(100+i) ) ) );
        }
        CHECKM("Not new size "+rb->toString(), grownCapacity, rb->getSize());
        CHECKTM("Not full "+rb->toString(), rb->isFull());

        for(int i=0; i<initialCapacity; i++) {
            SharedType svI = rb->get();
            // PRINTM("X05["+std::to_string(i)+"]: "+rb->toString()+", svI-null: "+std::to_string(svI==nullptr));
            CHECKTM("Empty at read #"+std::to_string(i+1)+": "+rb->toString(), svI!=nullptr);
            CHECKM("Wrong value at read #"+std::to_string(i+1)+": "+rb->toString(), (pos+i)%initialCapacity, svI->intValue());
        }

        for(int i=0; i<growAmount; i++) {
            SharedType svI = rb->get();
            // PRINTM("X07["+std::to_string(i)+"]: "+rb->toString()+", svI-null: "+std::to_string(svI==nullptr));
            CHECKTM("Empty at read #"+std::to_string(i+1)+": "+rb->toString(), svI!=nullptr);
            CHECKM("Wrong value at read #"+std::to_string(i+1)+": "+rb->toString(), 100+i, svI->intValue());
        }

        CHECKM("Not zero size "+rb->toString(), 0, rb->getSize());
        CHECKTM("Not empty "+rb->toString(), rb->isEmpty());
        CHECKTM("Is full "+rb->toString(), !rb->isFull());
    }

  public:

    void test20_GrowFull01_Begin() {
        test_GrowFullImpl(11, 0);
    }
    void test21_GrowFull02_Begin1() {
        test_GrowFullImpl(11, 0+1);
    }
    void test22_GrowFull03_Begin2() {
        test_GrowFullImpl(11, 0+2);
    }
    void test23_GrowFull04_Begin3() {
        test_GrowFullImpl(11, 0+3);
    }
    void test24_GrowFull05_End() {
        test_GrowFullImpl(11, 11-1);
    }
    void test25_GrowFull11_End1() {
        test_GrowFullImpl(11, 11-1-1);
    }
    void test26_GrowFull12_End2() {
        test_GrowFullImpl(11, 11-1-2);
    }
    void test27_GrowFull13_End3() {
        test_GrowFullImpl(11, 11-1-3);
    }

    void test_list() override {
        test01_FullRead();
        test02_EmptyWrite();
        test03_FullReadReset();
        test04_EmptyWriteClear();
        test05_ReadResetMid01();
        test06_ReadResetMid02();

        test20_GrowFull01_Begin();
        test21_GrowFull02_Begin1();
        test22_GrowFull03_Begin2();
        test23_GrowFull04_Begin3();
        test24_GrowFull05_End();
        test25_GrowFull11_End1();
        test26_GrowFull12_End2();
        test27_GrowFull13_End3();
    }
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    Cppunit_tests test1;
    return test1.run();
}

