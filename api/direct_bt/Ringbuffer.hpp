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

#ifndef RINGBUFFER_HPP_
#define RINGBUFFER_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>

#include "BasicTypes.hpp"

namespace direct_bt {


/**
 * Ring buffer interface, a.k.a circular buffer.
 * <p>
 * Caller can chose whether to block until get / put is able to proceed or not.
 * </p>
 * <p>
 * Caller can chose whether to pass an empty array and clear references at get,
 * or using a preset array for circular access of same objects.
 * </p>
 * <p>
 * Synchronization and hence thread safety details belong to the implementation.
 * </p>
 */
template <class T> class Ringbuffer {
    public:
        virtual ~Ringbuffer() {}

        /** Returns a short string representation incl. size/capacity and internal r/w index (impl. dependent). */
        virtual std::string toString() const = 0;

        /** Debug functionality - Dumps the contents of the internal array. */
        virtual void dump(FILE *stream, std::string prefix) const = 0;

        /** Returns the net capacity of this ring buffer. */
        virtual int capacity() const = 0;

        /**
         * Releasing all elements by assigning <code>null</code>.
         * <p>
         * {@link #isEmpty()} will return <code>true</code> and
         * {@link #getSize()} will return <code>0</code> after calling this method.
         * </p>
         */
        virtual void clear() = 0;

        /**
         * {@link #clear()} all elements and add all <code>copyFrom</code> elements thereafter.
         * @param copyFrom Mandatory array w/ length {@link #capacity()} to be copied into the internal array.
         */
        virtual void reset(const T * copyFrom, const int copyFromCount) = 0;
        virtual void reset(const std::vector<T> & copyFrom) = 0;

        /** Returns the number of elements in this ring buffer. */
        virtual int getSize() const = 0;

        /** Returns the number of free slots available to put.  */
        virtual int getFreeSlots() const = 0;

        /** Returns true if this ring buffer is empty, otherwise false. */
        virtual bool isEmpty() const = 0;

        /** Returns true if this ring buffer is full, otherwise false. */
        virtual bool isFull() const = 0;

        /**
         * Dequeues the oldest enqueued element if available, otherwise null.
         * <p>
         * The returned ring buffer slot will be set to <code>null</code> to release the reference
         * and move ownership to the caller.
         * </p>
         * <p>
         * Method is non blocking and returns immediately;.
         * </p>
         * @return the oldest put element if available, otherwise null.
         */
        virtual T get() = 0;

        /**
         * Dequeues the oldest enqueued element.
         * <p>
         * The returned ring buffer slot will be set to <code>null</code> to release the reference
         * and move ownership to the caller.
         * </p>
         * <p>
         * <code>timeoutMS</code> defaults to zero,
         * i.e. infinitive blocking until an element available via put.<br>
         * Otherwise this methods blocks for the given milliseconds.
         * </p>
         * @return the oldest put element or <code>null</code> if timeout occurred.
         * @throws InterruptedException
         */
        virtual T getBlocking(const int timeoutMS=0) /* throws InterruptedException */ = 0;

        /**
         * Peeks the next element at the read position w/o modifying pointer, nor blocking.
         * @return <code>null</code> if empty, otherwise the element which would be read next.
         */
        virtual T peek() = 0;

        /**
         * Peeks the next element at the read position w/o modifying pointer, but with blocking.
         * <p>
         * <code>timeoutMS</code> defaults to zero,
         * i.e. infinitive blocking until an element available via put.<br>
         * Otherwise this methods blocks for the given milliseconds.
         * </p>
         * @return <code>null</code> if empty or timeout occurred, otherwise the element which would be read next.
         */
        virtual T peekBlocking(const int timeoutMS=0) /* throws InterruptedException */ = 0;

        /**
         * Enqueues the given element.
         * <p>
         * Returns true if successful, otherwise false in case buffer is full.
         * </p>
         * <p>
         * Method is non blocking and returns immediately;.
         * </p>
         */
        virtual bool put(const T & e) = 0;

        /**
         * Enqueues the given element.
         * <p>
         * <code>timeoutMS</code> defaults to zero,
         * i.e. infinitive blocking until a free slot becomes available via get.<br>
         * Otherwise this methods blocks for the given milliseconds.
         * </p>
         * <p>
         * Returns true if successful, otherwise false in case timeout occurred.
         * </p>
         */
        virtual bool putBlocking(const T & e, const int timeoutMS=0) = 0;

        /**
         * Enqueues the same element at it's write position, if not full.
         * <p>
         * Returns true if successful, otherwise false in case buffer is full.
         * </p>
         * <p>
         * Method is non blocking and returns immediately;.
         * </p>
         * @throws InterruptedException
         */
        virtual bool putSame() = 0;

        /**
         * Enqueues the same element at it's write position, if not full.
         * <p>
         * <code>timeoutMS</code> defaults to zero,
         * i.e. infinitive blocking until a free slot becomes available via get.<br>
         * Otherwise this methods blocks for the given milliseconds.
         * </p>
         * <p>
         * Returns true if successful, otherwise false in case timeout occurred.
         * </p>
         * @throws InterruptedException
         */
        virtual bool putSameBlocking(const int timeoutMS=0) = 0;

        /**
         * Blocks until at least <code>count</code> free slots become available.
         * @throws InterruptedException
         */
        virtual void waitForFreeSlots(const int count) /* throws InterruptedException */ = 0;

        /**
         * Resizes this ring buffer's capacity.
         * <p>
         * New capacity must be greater than current size.
         * </p>
         */
        virtual void recapacity(const int newCapacity) = 0;
};

} /* namespace direct_bt */

#endif /* RINGBUFFER_HPP_ */
