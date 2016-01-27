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
#include "BluetoothAdapter.hpp"
#include "BluetoothEvent.hpp"
#include <vector>
#include <list>

class tinyb::BluetoothManager: public BluetoothObject
{
friend class BluetoothAdapter;
friend class BluetoothDevice;
friend class BluetoothGattService;
friend class BluetoothGattCharacteristic;
friend class BluetoothGattDescriptor;

private:
    BluetoothAdapter *default_adapter = NULL;
    static BluetoothManager *bluetooth_manager;
    std::list<std::shared_ptr<BluetoothEvent>> event_list;

    BluetoothManager();
    BluetoothManager(const BluetoothManager &object);

public:

    static std::string java_class() {
        return std::string(JAVA_PACKAGE "/BluetoothManager");
    }

    virtual std::string get_java_class() const;
    virtual std::string get_class_name() const;
    virtual std::string get_object_path() const;
    virtual BluetoothType get_bluetooth_type() const;

    ~BluetoothManager();
    /** Returns an instance of BluetoothManager, to be used instead of constructor.
      * @return An initialized BluetoothManager instance.
      */
    static BluetoothManager *get_bluetooth_manager();

    std::unique_ptr<BluetoothObject> get_object(BluetoothType type,
        std::string *name, std::string *identifier, BluetoothObject *parent);

    std::vector<std::unique_ptr<BluetoothObject>> get_objects(
        BluetoothType type = BluetoothType::NONE,
        std::string *name = nullptr, std::string *identifier = nullptr,
        BluetoothObject *parent = nullptr);

    /** Returns a list of BluetoothAdapters available in the system
      * @return A list of BluetoothAdapters available in the system
      */
    std::vector<std::unique_ptr<BluetoothAdapter>> get_adapters(
    );

    /** Returns a list of discovered BluetoothDevices
      * @return A list of discovered BluetoothDevices
      */
    std::vector<std::unique_ptr<BluetoothDevice>> get_devices(
    );

    /** Returns a list of available BluetoothGattServices
      * @return A list of available BluetoothGattServices
      */
    std::vector<std::unique_ptr<BluetoothGattService>> get_services(
    );

    /** Sets a default adapter to use for discovery.
      * @return TRUE if the device was set
      */
    bool set_default_adapter(
        BluetoothAdapter *adapter
    );

    /** Turns on device discovery on the default adapter if it is disabled.
      * @return TRUE if discovery was successfully enabled
      */
    bool start_discovery(
    );

    /** Turns off device discovery on the default adapter if it is enabled.
      * @return TRUE if discovery was successfully disabled
      */
    bool stop_discovery(
    );
};
