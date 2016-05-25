/*
 * Author: Petre Eftime <petre.p.eftime@intel.com>
 * Copyright (c) 2015 Intel Corporation.
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

#include <string>
#include <condition_variable>
#include <atomic>
#include <functional>
#include "BluetoothObject.hpp"
#pragma once

using namespace tinyb;

typedef std::function<void (BluetoothObject &, void *)> BluetoothCallback;

class tinyb::BluetoothEvent {
private:
    std::string *name;
    std::string *identifier;
    BluetoothObject *parent;
    BluetoothType type;
    bool execute_once;
    BluetoothCallback cb;
    void *data;
    bool canceled;

class BluetoothConditionVariable {

    friend class BluetoothEvent;

    std::condition_variable cv;
    std::mutex lock;
    BluetoothObject *result;
    std::atomic_bool triggered;
    std::atomic_uint waiting;

    BluetoothConditionVariable() : cv(), lock() {
        result = nullptr;
        waiting = 0;
        triggered = false;
    }

    BluetoothObject *wait() {
        if (result != nullptr)
            return result;

        if (!triggered) {
            std::unique_lock<std::mutex> lk(lock);
            waiting++;
            cv.wait(lk);
            waiting--;
        }

        return result;
    }

    BluetoothObject *wait_for(std::chrono::milliseconds timeout) {
        if (result != nullptr)
            return result;

        if (!triggered) {
            waiting++;
            std::unique_lock<std::mutex> lk(lock);
            cv.wait_for(lk, timeout);
            waiting--;
        }

        return result;
    }

    void notify() {
        triggered = true;
        while (waiting != 0)
            cv.notify_all();
    }

    ~BluetoothConditionVariable() {
        notify();
    }
};


BluetoothConditionVariable cv;

static void generic_callback(BluetoothObject &object, void *data);
public:

    BluetoothEvent(BluetoothType type, std::string *name, std::string *identifier,
        BluetoothObject *parent, bool execute_once = true,
        BluetoothCallback cb = nullptr, void *data = NULL);
    ~BluetoothEvent();

    BluetoothType get_type() const {
        return type;
    }

    std::string *get_name() const {
        return name;
    }

    std::string *get_identifier() const {
        return identifier;
    }

    BluetoothObject *get_parent() const {
        return parent;
    }

    bool execute_callback(BluetoothObject &object);
    bool has_callback() {
        return (cb != NULL);
    }

   BluetoothObject *get_result() {
        return cv.result;
   }

   void cancel();

   void wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());

   bool operator==(BluetoothEvent const &other);
};
