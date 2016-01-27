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

#pragma once
#include "BluetoothObject.hpp"
#include <vector>

/* Forward declaration of types */
struct _Object;
typedef struct _Object Object;
struct _GattDescriptor1;
typedef struct _GattDescriptor1 GattDescriptor1;

/**
  * Provides access to Bluetooth GATT descriptor. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/gatt-api.txt
  */
class tinyb::BluetoothGattDescriptor: public BluetoothObject
{

friend class tinyb::BluetoothGattCharacteristic;
friend class tinyb::BluetoothManager;
friend class tinyb::BluetoothEventManager;

private:
    GattDescriptor1 *object;


protected:
    BluetoothGattDescriptor(GattDescriptor1 *object);

    static std::unique_ptr<BluetoothGattDescriptor> make(Object *object,
        BluetoothType type = BluetoothType::GATT_DESCRIPTOR,
        std::string *name = nullptr,
        std::string *identifier = nullptr,
        BluetoothObject *parent = nullptr);

public:

    static std::string java_class() {
        return std::string(JAVA_PACKAGE "/BluetoothGattDescriptor");
    }

    virtual std::string get_java_class() const;
    virtual std::string get_class_name() const;
    virtual std::string get_object_path() const;
    virtual BluetoothType get_bluetooth_type() const;

    BluetoothGattDescriptor(const BluetoothGattDescriptor &object);
    ~BluetoothGattDescriptor();
    virtual BluetoothGattDescriptor *clone() const;

    /* D-Bus method calls: */
    /** Reads the value of this descriptor
      * @return A vector<uchar> containing data from this descriptor
      */
    std::vector<unsigned char> read_value (
    );

    /** Writes the value of this descriptor.
      * @param[in] arg_value The data as vector<uchar>
      * to be written packed in a GBytes struct
      * @return TRUE if value was written succesfully
      */
    bool write_value (
        const std::vector<unsigned char> &arg_value
    );

    /* D-Bus property accessors: */
    /** Get the UUID of this descriptor.
      * @return The 128 byte UUID of this descriptor, NULL if an error occurred
      */
    std::string get_uuid ();

    /** Returns the characteristic to which this descriptor belongs to.
      * @return The characteristic.
      */
    BluetoothGattCharacteristic get_characteristic ();

    /** Returns the cached value of this descriptor, if any.
      * @return The cached value of this descriptor.
      */
    std::vector<unsigned char> get_value ();

};
