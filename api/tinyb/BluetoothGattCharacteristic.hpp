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
#include <string>
#include <vector>

/* Forward declaration of types */
struct _Object;
typedef struct _Object Object;
struct _GattCharacteristic1;
typedef struct _GattCharacteristic1 GattCharacteristic1;

/**
  * Provides access to Bluetooth GATT characteristic. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/gatt-api.txt
  */
class tinyb::BluetoothGattCharacteristic: public BluetoothObject
{

friend class tinyb::BluetoothGattService;
friend class tinyb::BluetoothGattDescriptor;
friend class tinyb::BluetoothManager;
friend class tinyb::BluetoothEventManager;

private:
    GattCharacteristic1 *object;

protected:
    BluetoothGattCharacteristic(GattCharacteristic1 *object);

    static std::unique_ptr<BluetoothGattCharacteristic> make(Object *object,
        BluetoothType type = BluetoothType::GATT_CHARACTERISTIC,
        std::string *name = nullptr,
        std::string *identifier = nullptr,
        BluetoothObject *parent = nullptr);
public:

    static std::string java_class() {
        return std::string(JAVA_PACKAGE "/BluetoothGattCharacteristic");
    }

    virtual std::string get_java_class() const;
    virtual std::string get_class_name() const ;
    virtual std::string get_object_path() const;
    virtual BluetoothType get_bluetooth_type() const;

    BluetoothGattCharacteristic(const BluetoothGattCharacteristic &object);
    ~BluetoothGattCharacteristic();
    virtual BluetoothGattCharacteristic *clone() const;

    /* D-Bus method calls: */
    /** Reads the value of this characteristic.
      * @return A std::vector<unsgined char> containing the value of this characteristic.
      */
    std::vector<unsigned char> read_value (
    );

    /** Writes the value of this characteristic.
      * @param[in] arg_value The data as vector<uchar>
      * to be written packed in a GBytes struct
      * @return TRUE if value was written succesfully
      */
    bool write_value (const std::vector<unsigned char> &arg_value);

    bool start_notify (
    );

    bool stop_notify (
    );

    /* D-Bus property accessors: */
    /** Get the UUID of this characteristic.
      * @return The 128 byte UUID of this characteristic, NULL if an error occurred
      */
    std::string get_uuid ();

    /** Returns the service to which this characteristic belongs to.
      * @return The service.
      */
    BluetoothGattService get_service ();

    /** Returns the cached value of this characteristic, if any.
      * @return The cached value of this characteristic.
      */
    std::vector<unsigned char> get_value ();

    /** Returns true if notification for changes of this characteristic are
      * activated.
      * @return True if notificatios are activated.
      */
    bool get_notifying ();

    /** Returns the flags this characterstic has.
      * @return A list of flags for this characteristic.
      */
    std::vector<std::string> get_flags ();

    /** Returns a list of BluetoothGattDescriptors this characteristic exposes.
      * @return A list of BluetoothGattDescriptors exposed by this characteristic
      * NULL if an error occurred
      */
    std::vector<std::unique_ptr<BluetoothGattDescriptor>> get_descriptors ();

};
