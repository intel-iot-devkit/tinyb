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

#include <memory>
#pragma once

#define JAVA_PACKAGE "tinyb"

namespace tinyb {
enum class BluetoothType {
    NONE,
    ADAPTER,
    DEVICE,
    GATT_SERVICE,
    GATT_CHARACTERISTIC,
    GATT_DESCRIPTOR
};

    class BluetoothEvent;
    class BluetoothEventManager;
    class BluetoothObject;
    class BluetoothManager;
    class BluetoothAdapter;
    class BluetoothDevice;
    class BluetoothGattService;
    class BluetoothGattCharacteristic;
    class BluetoothGattDescriptor;
}

class tinyb::BluetoothObject
{
public:

    static std::string java_class() {
        return std::string(JAVA_PACKAGE "/BluetoothObject");
    }

    /** Returns the complete Java class of this object
      * @return A std::string containing the java class of this object
      */
    virtual std::string get_java_class() const;

    /** Returns the class name of this object
      * @return A std::string containing the class name of this object
      */
    virtual std::string get_class_name() const;

    /** Returns the DBus object path of this object
      * @return A std::string containing the DBus object path of this object
      */
    virtual std::string get_object_path() const;

    /** Returns the BluetoothType of this object
      * @return The BluetoothType of this object
      */
    virtual BluetoothType get_bluetooth_type() const;

    virtual ~BluetoothObject() { };


    /** Returns a raw pointer to a clone of the object
      * @return A raw pointer to a clone of the object
      */
    virtual BluetoothObject *clone() const;

    /** Returns true if this object and the other point to the same DBus Object
      * @return True if this object and the other point to the same DBus Object
      */
    virtual bool operator==(const BluetoothObject &other) const;
};
