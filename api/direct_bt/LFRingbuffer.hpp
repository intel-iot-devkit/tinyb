/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LFRINGBUFFER_HPP_
#define LFRINGBUFFER_HPP_

#include <cstring>
#include <string>
#include <cstdint>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "BasicTypes.hpp"

#include "Ringbuffer.hpp"

namespace direct_bt {


/**
 * Simple implementation of {@link Ringbuffer},
 * exposing <i>lock-free</i>
 * {@link #get() get*(..)} and {@link #put(Object) put*(..)} methods.
 * <p>
 * Implementation utilizes the <i>Always Keep One Slot Open</i>,
 * hence implementation maintains an internal array of <code>capacity</code> <i>plus one</i>!
 * </p>
 * <p>
 * Implementation is thread safe if:
 * <ul>
 *   <li>{@link #get() get*(..)} operations from multiple threads.</li>
 *   <li>{@link #put(Object) put*(..)} operations from multiple threads.</li>
 *   <li>{@link #get() get*(..)} and {@link #put(Object) put*(..)} thread may be the same.</li>
 * </ul>
 * </p>
 * <p>
 * Following methods use acquire the global multi-read and -write mutex:
 * <ul>
 *  <li>{@link #resetFull(Object[])}</li>
 *  <li>{@link #clear()}</li>
 *  <li>{@link #growEmptyBuffer(Object[])}</li>
 * </ul>
 * </p>
 * <p>
 * Characteristics:
 * <ul>
 *   <li>Read position points to the last read element.</li>
 *   <li>Write position points to the last written element.</li>
 * </ul>
 * <table border="1">
 *   <tr><td>Empty</td><td>writePos == readPos</td><td>size == 0</td></tr>
 *   <tr><td>Full</td><td>writePos == readPos - 1</td><td>size == capacity</td></tr>
 * </table>
 * </p>
 */
template <typename T, std::nullptr_t nullelem> class LFRingbuffer : public Ringbuffer<T> {
    private:
        std::mutex syncRead, syncMultiRead;
        std::mutex syncWrite, syncMultiWrite;
        std::condition_variable cvRead;
        std::condition_variable cvWrite;

        /* final */ int volatile capacityPlusOne;  // not final due to grow
        /* final */ T * volatile array; // not final due to grow
        int volatile readPos;
        int volatile writePos;
        std::atomic_int size;

        T * newArray(const int count) {
            return new T[count];
        }
        void freeArray(T * a) {
            delete[] a;
        }

        void cloneFrom(const bool allocArrayAndCapacity, const LFRingbuffer & source) {
            if( allocArrayAndCapacity ) {
                capacityPlusOne = source.capacityPlusOne;
                if( nullptr != array ) {
                    freeArray(array, true);
                }
                array = newArray(capacityPlusOne);
            } else if( capacityPlusOne != source.capacityPlusOne ) {
                throw InternalError("capacityPlusOne not equal: this "+toString()+", source "+source.toString(), E_FILE_LINE);
            }

            readPos = source.readPos;
            writePos = source.writePos;
            size = source.size;
            int localWritePos = readPos;
            for(int i=0; i<size; i++) {
                localWritePos = (localWritePos + 1) % capacityPlusOne;
                array[localWritePos] = source.array[localWritePos];
            }
            if( writePos != localWritePos ) {
                throw InternalError("copy segment error: this "+toString()+", localWritePos "+std::to_string(localWritePos)+"; source "+source.toString(), E_FILE_LINE);
            }
        }

        void resetImpl(const T * copyFrom, const int copyFromCount) /* throws IllegalArgumentException */ {
            // clear all elements, zero size
            if( 0 < size ) {
                int localReadPos = readPos;
                for(int i=0; i<size; i++) {
                    localReadPos = (localReadPos + 1) % capacityPlusOne;
                    array[localReadPos] = nullelem;
                }
                if( writePos != localReadPos ) {
                    throw InternalError("copy segment error: this "+toString()+", localReadPos "+std::to_string(localReadPos)+"; source count "+std::to_string(copyFromCount), E_FILE_LINE);
                }
                readPos = localReadPos;
                size = 0;
            }

            // fill with copyFrom elements
            if( nullptr != copyFrom && 0 < copyFromCount ) {
                if( copyFromCount > capacityPlusOne-1 ) {
                    throw IllegalArgumentException("copyFrom array length "+std::to_string(copyFromCount)+" > capacity "+toString(), E_FILE_LINE);
                }
                int localWritePos = writePos;
                for(int i=0; i<copyFromCount; i++) {
                    localWritePos = (localWritePos + 1) % capacityPlusOne;
                    array[localWritePos] = copyFrom[i];
                    size++;
                }
                writePos = localWritePos;
            }
        }

        T getImpl(const bool blocking, const bool peek, const int timeoutMS) {
            std::unique_lock<std::mutex> lockMultiRead(syncMultiRead); // RAII-style acquire and relinquish via destructor

            int localReadPos = readPos;
            if( localReadPos == writePos ) {
                if( blocking ) {
                    std::unique_lock<std::mutex> lockRead(syncRead); // RAII-style acquire and relinquish via destructor
                    while( localReadPos == writePos ) {
                        if( 0 == timeoutMS ) {
                            cvRead.wait(lockRead);
                        } else {
                            std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
                            std::cv_status s = cvRead.wait_until(lockRead, t0 + std::chrono::milliseconds(timeoutMS));
                            if( std::cv_status::timeout == s && localReadPos == writePos ) {
                                return nullelem;
                            }
                        }
                    }
                } else {
                    return nullelem;
                }
            }
            localReadPos = (localReadPos + 1) % capacityPlusOne;
            T r = array[localReadPos];
            if( !peek ) {
                array[localReadPos] = nullelem;
                {
                    std::unique_lock<std::mutex> lockWrite(syncWrite); // RAII-style acquire and relinquish via destructor
                    size--;
                    readPos = localReadPos;
                    cvWrite.notify_all(); // notify waiting putter
                }
            }
            return r;
        }

        bool putImpl(const T &e, const bool sameRef, const bool blocking, const int timeoutMS) /* throws InterruptedException */ {
            std::unique_lock<std::mutex> lockMultiWrite(syncMultiWrite); // RAII-style acquire and relinquish via destructor

            int localWritePos = writePos;
            localWritePos = (localWritePos + 1) % capacityPlusOne;
            if( localWritePos == readPos ) {
                if( blocking ) {
                    std::unique_lock<std::mutex> lockWrite(syncWrite); // RAII-style acquire and relinquish via destructor
                    while( localWritePos == readPos ) {
                        if( 0 == timeoutMS ) {
                            cvWrite.wait(lockWrite);
                        } else {
                            std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
                            std::cv_status s = cvWrite.wait_until(lockWrite, t0 + std::chrono::milliseconds(timeoutMS));
                            if( std::cv_status::timeout == s && localWritePos == readPos ) {
                                return false;
                            }
                        }
                    }
                } else {
                    return false;
                }
            }
            if( !sameRef ) {
                array[localWritePos] = e;
            }
            {
                std::unique_lock<std::mutex> lockRead(syncRead); // RAII-style acquire and relinquish via destructor
                size++;
                writePos = localWritePos;
                cvRead.notify_all(); // notify waiting getter
            }
            return true;
        }

    public:
        std::string toString() const override {
            const std::string es = isEmpty() ? ", empty" : "";
            const std::string fs = isFull() ? ", full" : "";
            return "LFRingbuffer<?>[size "+std::to_string(size)+" / "+std::to_string(capacityPlusOne-1)+
                    ", writePos "+std::to_string(writePos)+", readPos "+std::to_string(readPos)+es+fs+"]";
        }

        void dump(FILE *stream, std::string prefix) const override {
            fprintf(stream, "%s %s {\n", prefix.c_str(), toString().c_str());
            for(int i=0; i<capacityPlusOne; i++) {
                // fprintf(stream, "\t[%d]: %p\n", i, array[i].get()); // FIXME
            }
            fprintf(stream, "}\n");
        }

        /**
         * Create a full ring buffer instance w/ the given array's net capacity and content.
         * <p>
         * Example for a 10 element Integer array:
         * <pre>
         *  Integer[] source = new Integer[10];
         *  // fill source with content ..
         *  Ringbuffer<Integer> rb = new LFRingbuffer<Integer>(source);
         * </pre>
         * </p>
         * <p>
         * {@link #isFull()} returns true on the newly created full ring buffer.
         * </p>
         * <p>
         * Implementation will allocate an internal array with size of array <code>copyFrom</code> <i>plus one</i>,
         * and copy all elements from array <code>copyFrom</code> into the internal array.
         * </p>
         * @param copyFrom mandatory source array determining ring buffer's net {@link #capacity()} and initial content.
         * @throws IllegalArgumentException if <code>copyFrom</code> is <code>nullptr</code>
         */
        LFRingbuffer(const std::vector<T> & copyFrom) /* throws IllegalArgumentException */
        : capacityPlusOne(copyFrom.size() + 1), array(newArray(capacityPlusOne)),
          readPos(0), writePos(0), size(0)
        {
            resetImpl(copyFrom.data(), copyFrom.size());
        }

        LFRingbuffer(const T * copyFrom, const int copyFromSize) /* throws IllegalArgumentException */
        : capacityPlusOne(copyFromSize + 1), array(newArray(capacityPlusOne)),
          readPos(0), writePos(0), size(0)
        {
            resetImpl(copyFrom, copyFromSize);
        }

        /**
         * Create an empty ring buffer instance w/ the given net <code>capacity</code>.
         * <p>
         * Example for a 10 element Integer array:
         * <pre>
         *  Ringbuffer<Integer> rb = new LFRingbuffer<Integer>(10, Integer[].class);
         * </pre>
         * </p>
         * <p>
         * {@link #isEmpty()} returns true on the newly created empty ring buffer.
         * </p>
         * <p>
         * Implementation will allocate an internal array of size <code>capacity</code> <i>plus one</i>.
         * </p>
         * @param arrayType the array type of the created empty internal array.
         * @param capacity the initial net capacity of the ring buffer
         */
        LFRingbuffer(const int capacity)
        : capacityPlusOne(capacity + 1), array(newArray(capacityPlusOne)),
          readPos(0), writePos(0), size(0)
        { }

        ~LFRingbuffer() {
            freeArray(array);
        }

        LFRingbuffer(const LFRingbuffer &_source) noexcept
        : capacityPlusOne(_source.capacityPlusOne), array(newArray(capacityPlusOne)),
          readPos(0), writePos(0), size(0)
        {
            std::unique_lock<std::mutex> lockMultiReadS(_source.syncMultiRead);
            std::unique_lock<std::mutex> lockMultiWriteS(_source.syncMultiWrite);
            std::unique_lock<std::mutex> lockMultiRead(syncMultiRead);
            std::unique_lock<std::mutex> lockMultiWrite(syncMultiWrite);
            cloneFrom(false, _source);
        }

        LFRingbuffer& operator=(const LFRingbuffer &_source) {
            std::unique_lock<std::mutex> lockMultiReadS(_source.syncMultiRead);
            std::unique_lock<std::mutex> lockMultiWriteS(_source.syncMultiWrite);
            std::unique_lock<std::mutex> lockMultiRead(syncMultiRead);
            std::unique_lock<std::mutex> lockMultiWrite(syncMultiWrite);

            if( this == &_source ) {
                return *this;
            }
            if( capacityPlusOne != _source.capacityPlusOne ) {
                cloneFrom(true, _source);
            } else {
                resetImpl(nullptr, 0 /* empty, nothing to copy */ ); // clear
                cloneFrom(false, _source);
            }
            return *this;
        }

        LFRingbuffer(LFRingbuffer &&o) noexcept = default;
        LFRingbuffer& operator=(LFRingbuffer &&o) noexcept = default;

        int capacity() const override { return capacityPlusOne-1; }

        void clear() override {
            std::unique_lock<std::mutex> lockMultiRead(syncMultiRead); // RAII-style acquire and relinquish via destructor
            std::unique_lock<std::mutex> lockMultiWrite(syncMultiWrite); // ditto
            resetImpl(nullptr, 0 /* empty, nothing to copy */ );
        }

        void reset(const T * copyFrom, const int copyFromCount) override {
            std::unique_lock<std::mutex> lockMultiRead(syncMultiRead); // RAII-style acquire and relinquish via destructor
            std::unique_lock<std::mutex> lockMultiWrite(syncMultiWrite); // ditto
            resetImpl(copyFrom, copyFromCount);
        }

        void reset(const std::vector<T> & copyFrom) override {
            std::unique_lock<std::mutex> lockMultiRead(syncMultiRead); // RAII-style acquire and relinquish via destructor
            std::unique_lock<std::mutex> lockMultiWrite(syncMultiWrite); // ditto
            resetImpl(copyFrom.data(), copyFrom.size());
        }

        int getSize() const override { return size; }

        int getFreeSlots() const override { return capacityPlusOne - 1 - size; }

        bool isEmpty() const override { return writePos == readPos; /* 0 == size */ }

        bool isFull() const override { return ( writePos + 1 ) % capacityPlusOne == readPos ; /* capacityPlusOne - 1 == size */; }

        T get() override { return getImpl(false, false, 0); }

        T getBlocking(const int timeoutMS=0) override /* throws InterruptedException */ {
            return getImpl(true, false, timeoutMS);
        }

        T peek() override {
            return getImpl(false, true, 0);
        }

        T peekBlocking(const int timeoutMS=0) override /* throws InterruptedException */ {
            return getImpl(true, true, timeoutMS);
        }

        bool put(const T & e) override {
            return putImpl(e, false, false, 0);
        }

        bool putBlocking(const T & e, const int timeoutMS=0) override {
            return !putImpl(e, false, true, timeoutMS);
        }

        bool putSame() override {
            return putImpl(nullelem, true, false, 0);
        }

        bool putSameBlocking(const int timeoutMS=0) override {
            return putImpl(nullelem, true, true, timeoutMS);
        }

        void waitForFreeSlots(const int count) override /* throws InterruptedException */ {
            std::unique_lock<std::mutex> lockMultiWrite(syncMultiWrite); // RAII-style acquire and relinquish via destructor
            std::unique_lock<std::mutex> lockRead(syncRead); // RAII-style acquire and relinquish via destructor

            while( capacityPlusOne - 1 - size < count ) {
                cvRead.wait(lockRead);
            }
        }

        void recapacity(const int newCapacity) override {
            std::unique_lock<std::mutex> lockMultiRead(syncMultiRead);
            std::unique_lock<std::mutex> lockMultiWrite(syncMultiWrite);

            if( capacityPlusOne == newCapacity+1 ) {
                return;
            }
            if( size > newCapacity ) {
                throw IllegalArgumentException("amount "+std::to_string(newCapacity)+" < size, "+toString(), E_FILE_LINE);
            }
            if( 0 > newCapacity ) {
                throw IllegalArgumentException("amount "+std::to_string(newCapacity)+" < 0, "+toString(), E_FILE_LINE);
            }

            // save current data
            int oldCapacityPlusOne = capacityPlusOne;
            T * oldArray = array;
            int oldReadPos = readPos;

            // new blank resized array
            capacityPlusOne = newCapacity + 1;
            array = newArray(capacityPlusOne);
            readPos = 0;
            writePos = 0;
            const int _size = size.load(); // fast access

            // copy saved data
            if( nullptr != oldArray && 0 < _size ) {
                int localWritePos = writePos;
                for(int i=0; i<_size; i++) {
                    localWritePos = (localWritePos + 1) % capacityPlusOne;
                    oldReadPos = (oldReadPos + 1) % oldCapacityPlusOne;
                    array[localWritePos] = oldArray[oldReadPos];
                }
                writePos = localWritePos;
            }
            freeArray(oldArray); // and release
        }
};

} /* namespace direct_bt */

#endif /* LFRINGBUFFER_HPP_ */
