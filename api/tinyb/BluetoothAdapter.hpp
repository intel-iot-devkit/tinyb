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
struct _Adapter1;
typedef struct _Adapter1 Adapter1;

/**
  * Provides access to Bluetooth adapters. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/adapter-api.txt
  */
class tinyb::BluetoothAdapter: public BluetoothObject
{

friend class tinyb::BluetoothManager;
friend class tinyb::BluetoothEventManager;
friend class tinyb::BluetoothDevice;

private:
    Adapter1 *object;
    /** Removes a device from the list of devices available on this adapter.
      * @param[in] arg_device The path of the device on DBus
      * @return TRUE if device was successfully removed
      */
    bool remove_device (
        const std::string &arg_device
    );


protected:
    BluetoothAdapter(Adapter1 *object);

   static std::unique_ptr<BluetoothAdapter> make(Object *object,
        BluetoothType type = BluetoothType::ADAPTER,
        std::string *name = nullptr,
        std::string *identifier = nullptr,
        BluetoothObject *parent = nullptr);
public:

    static std::string java_class() {
        return std::string(JAVA_PACKAGE "/BluetoothAdapter");
    }

    virtual std::string get_java_class() const;
    virtual std::string get_class_name() const;
    virtual std::string get_object_path() const;
    virtual BluetoothType get_bluetooth_type() const;

    BluetoothAdapter(const BluetoothAdapter &object);
    ~BluetoothAdapter();
    virtual BluetoothAdapter *clone() const;

    /* D-Bus method calls: */

    /** Turns on device discovery if it is disabled.
      * @return TRUE if discovery was successfully enabled
      */
    bool start_discovery (
    );

    /** Turns off device discovery if it is enabled.
      * @return TRUE if discovery was successfully disabled
      */
    bool stop_discovery (
    );


    /** Returns a list of BluetoothDevices visible from this adapter.
      * @return A list of BluetoothDevices visible on this adapter,
      * NULL if an error occurred
      */
    std::vector<std::unique_ptr<BluetoothDevice>> get_devices (
    );

    /* D-Bus property accessors: */
    /** Returns the hardware address of this adapter.
      * @return The hardware address of this adapter.
      */
    std::string get_address ();

    /** Returns the system name of this adapter.
      * @return The system name of this adapter.
      */
    std::string get_name ();

    /** Returns the friendly name of this adapter.
      * @return The friendly name of this adapter, or NULL if not set.
      */
    std::string get_alias ();

    /** Sets the friendly name of this adapter.
      */
    void set_alias (const std::string  &value);

    /** Returns the Bluetooth class of the adapter.
      * @return The Bluetooth class of the adapter.
      */
    unsigned int get_class ();

    /** Returns the power state the adapter.
      * @return The power state of the adapter.
      */
    bool get_powered ();

    /** Sets the power state the adapter.
      */
    void set_powered (bool  value);

    /** Returns the discoverable state the adapter.
      * @return The discoverable state of the adapter.
      */
    bool get_discoverable ();

    /** Sets the discoverable state the adapter.
      */
    void set_discoverable (bool  value);

    /** Returns the discoverable timeout the adapter.
      * @return The discoverable timeout of the adapter.
      */
    unsigned int get_discoverable_timeout ();

    /** Sets the discoverable timeout the adapter. A value of 0 disables
      * the timeout.
      */
    void set_discoverable_timeout (unsigned int  value);

    /** Returns the pairable state the adapter.
      * @return The pairable state of the adapter.
      */
    bool get_pairable ();

    /** Sets the discoverable state the adapter.
      */
    void set_pairable (bool  value);

    /** Returns the timeout in seconds after which pairable state turns off
      * automatically, 0 means never.
      * @return The pairable timeout of the adapter.
      */
    unsigned int get_pairable_timeout ();

    /** Sets the timeout after which pairable state turns off automatically, 0 means never.
      */
    void set_pairable_timeout (unsigned int  value);

    /** Returns the discovering state the adapter. It can be modified through
      * start_discovery/stop_discovery functions.
      * @return The discovering state of the adapter.
      */
    bool get_discovering ();

    /** Returns the UUIDs of the adapter.
      * @return Array containing the UUIDs of the adapter, ends with NULL.
      */
    std::vector<std::string> get_uuids ();

    /** Returns the local ID of the adapter.
      * @return The local ID of the adapter.
      */
    std::string get_modalias ();

};
