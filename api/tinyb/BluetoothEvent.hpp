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
#include "BluetoothObject.hpp"
#pragma once

using namespace tinyb;

typedef void (*BluetoothCallback)(BluetoothObject &, void *);

class tinyb::BluetoothEvent {
private:
    std::string *name;
    std::string *identifier;
    BluetoothObject *parent;
    BluetoothType type;
    BluetoothCallback cb;
    void *data;

static void generic_callback(BluetoothObject &object, void *data);

struct generic_callback_data {
    std::condition_variable cv;
    BluetoothObject *result;
};

public:

    BluetoothEvent(BluetoothType type, std::string *name, std::string *identifier,
        BluetoothObject *parent, BluetoothCallback cb = generic_callback,
        void *data = NULL);

    BluetoothType get_type();
    std::string get_name();
    std::string get_identifier();
    bool execute_callback();
    bool has_callback();

    bool operator==(BluetoothEvent const &other);
};
