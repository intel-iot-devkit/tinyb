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

#include "BluetoothObject.hpp"
#include <glib.h>

using namespace tinyb;

std::string BluetoothObject::get_java_class() const
{
   return std::string(JAVA_PACKAGE "/BluetoothObject");
}

std::string BluetoothObject::get_class_name() const
{
   return std::string("BluetoothObject");
}

std::string BluetoothObject::get_object_path() const
{
   return std::string();
}

BluetoothType BluetoothObject::get_bluetooth_type() const
{
   return BluetoothType::NONE;
}

BluetoothObject *BluetoothObject::clone() const
{
    return NULL;
}

bool BluetoothObject::operator==(const BluetoothObject &other) const
{
   return (this->get_bluetooth_type() == other.get_bluetooth_type())
        && (this->get_object_path() == other.get_object_path());
}

bool BluetoothObject::operator!=(const BluetoothObject &other) const
{
   return !(*this == other);
}
