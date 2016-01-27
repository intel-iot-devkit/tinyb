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
#include "BluetoothManager.hpp"
#include <vector>

/* Forward declaration of types */
struct _Object;
typedef struct _Object Object;
struct _GattService1;
typedef struct _GattService1 GattService1;

/**
  * Provides access to Bluetooth GATT characteristic. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/gatt-api.txt
  */
class tinyb::BluetoothGattService: public BluetoothObject
{

friend class tinyb::BluetoothManager;
friend class tinyb::BluetoothEventManager;
friend class tinyb::BluetoothDevice;
friend class tinyb::BluetoothGattCharacteristic;

private:
    GattService1 *object;

protected:
    BluetoothGattService(GattService1 *object);

    static std::unique_ptr<BluetoothGattService> make(Object *object,
        BluetoothType type = BluetoothType::GATT_SERVICE,
        std::string *name = nullptr,
        std::string *identifier = nullptr,
        BluetoothObject *parent = nullptr);

public:

    static std::string java_class() {
        return std::string(JAVA_PACKAGE "/BluetoothGattService");
    }

    virtual std::string get_java_class() const;
    virtual std::string get_class_name() const;
    virtual std::string get_object_path() const;
    virtual BluetoothType get_bluetooth_type() const;

    BluetoothGattService(const BluetoothGattService &object);
    ~BluetoothGattService();
    virtual BluetoothGattService *clone() const;

    /* D-Bus property accessors: */

    /** Get the UUID of this service
      * @return The 128 byte UUID of this service, NULL if an error occurred
      */
    std::string get_uuid ();

    /** Returns the device to which this service belongs to.
      * @return The device.
      */
    BluetoothDevice get_device ();

    /** Returns true if this service is a primary service, false if secondary.
      * @return true if this service is a primary service, false if secondary.
      */
    bool get_primary ();

    /** Returns a list of BluetoothGattCharacteristics this service exposes.
      * @return A list of BluetoothGattCharacteristics exposed by this service
      */
    std::vector<std::unique_ptr<BluetoothGattCharacteristic>> get_characteristics ();

};
