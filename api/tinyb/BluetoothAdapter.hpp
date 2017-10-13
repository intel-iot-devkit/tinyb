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
#include "BluetoothUUID.hpp"
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
friend class tinyb::BluetoothNotificationHandler;

private:
    Adapter1 *object;

protected:
    BluetoothAdapter(Adapter1 *object);

   static std::unique_ptr<BluetoothAdapter> make(Object *object,
        BluetoothType type = BluetoothType::ADAPTER,
        std::string *name = nullptr,
        std::string *identifier = nullptr,
        BluetoothObject *parent = nullptr);

    std::function<void(bool)> powered_callback;
    std::function<void(bool)> discoverable_callback;
    std::function<void(bool)> pairable_callback;
    std::function<void(bool)> discovering_callback;

public:

    static std::string java_class() {
        return std::string(JAVA_PACKAGE "/BluetoothAdapter");
    }
    static BluetoothType class_type() { return BluetoothType::ADAPTER; }

    virtual std::string get_java_class() const;
    virtual std::string get_class_name() const;
    virtual std::string get_object_path() const;
    virtual BluetoothType get_bluetooth_type() const;

    BluetoothAdapter(const BluetoothAdapter &object);
    ~BluetoothAdapter();
    virtual BluetoothAdapter *clone() const;

    std::unique_ptr<BluetoothDevice> find(std::string *name,
        std::string *identifier,
        std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
    {

        BluetoothManager *manager = BluetoothManager::get_bluetooth_manager();
        return manager->find<BluetoothDevice>(name, identifier, this, timeout);
    }

    /* D-Bus method calls: */

    /** Removes a device from the list of devices available on this adapter.
      * @param[in] arg_device The path of the device on DBus
      * @return TRUE if device was successfully removed
      */
    bool remove_device (
        const std::string &arg_device
    );
    
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

    /** Sets the device discovery filter for the caller. If all fields are empty,
      * filter is removed.
      * @param uuids Vector of UUIDs to filter on
      * @param rssi RSSI low bounded threshold
      * @param pathloss Pathloss threshold value
      * @param transport Type of scan to run
      */
    bool set_discovery_filter (std::vector<BluetoothUUID> uuids,
    int16_t rssi, uint16_t pathloss, const TransportType &transport);

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
      * @return The friendly name of this adapter.
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

    /**
     * Enables notifications for changes of the powered status of the adapter
     * and triggers the callback when the value changes.
     * Uninstalls the previous powered callback, if any was installed.
     * @param callback A function of the form void(BluetoothAdapter&, bool, void *), where
     * BluetoothAdapter& is the adapter for which the callback was set, bool will contain the
     * new value of the powered property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_powered_notifications(
        std::function<void(BluetoothAdapter &adapter, bool powered, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the powered status of the adapter
     * and triggers the callback when the value changes.
     * Uninstalls the previous powered callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the powered property
     */
    void enable_powered_notifications(std::function<void(bool powered)> callback);
    /**
     * Disables notifications for changes of the powered status of the adapter
     * and uninstalls any callback.
     */
    void disable_powered_notifications();

    /** Returns the discoverable state the adapter.
      * @return The discoverable state of the adapter.
      */
    bool get_discoverable ();

    /** Sets the discoverable state the adapter.
      */
    void set_discoverable (bool  value);

    /**
     * Enables notifications for changes of the discoverable status of the adapter
     * and triggers the callback when the value changes.
     * Uninstalls the previous discoverable callback, if any was installed.
     * @param callback A function of the form void(BluetoothAdapter&, bool, void *), where
     * BluetoothAdapter& is the adapter for which the callback was set, bool will contain the
     * new value of the discoverable property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_discoverable_notifications(
        std::function<void(BluetoothAdapter &adapter, bool discoverable, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the discoverable status of the adapter
     * and triggers the callback when the value changes.
     * Uninstalls the previous discoverable callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the discoverable property
     */
    void enable_discoverable_notifications(std::function<void(bool discoverable)> callback);
    /**
     * Disables notifications for changes of the discoverable status of the adapter and uninstalls any callback;
     */
    void disable_discoverable_notifications();


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

    /**
     * Enables notifications for changes of the pairable status of the adapter
     * and triggers the callback when the value changes.
     * Uninstalls the previous pairable callback, if any was installed.
     * @param callback A function of the form void(BluetoothAdapter&, bool, void *), where
     * BluetoothAdapter& is the adapter for which the callback was set, bool will contain the
     * new value of the pairable property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
   void enable_pairable_notifications(
        std::function<void(BluetoothAdapter &adapter, bool pairable, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the pairable status of the adapter
     * and triggers the callback when the value changes.
     * Uninstalls the previous pairable callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the pairable property
     */
    void enable_pairable_notifications(std::function<void(bool pairable)> callback);
    /**
     * Disables notifications for changes of the pairable status of the adapter and uninstalls any callback;
     */
    void disable_pairable_notifications();

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

    /**
     * Enables notifications for changes of the discovering status of the adapter
     * and triggers the callback when the value changes.
     * Uninstalls the previous discovering callback, if any was installed.
     * @param callback A function of the form void(BluetoothAdapter&, bool, void *), where
     * BluetoothAdapter& is the adapter for which the callback was set, bool will contain the
     * new value of the discovering property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_discovering_notifications(
        std::function<void(BluetoothAdapter &adapter, bool discovering, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the discovering status of the adapter
     * and triggers the callback when the value changes.
     * Uninstalls the previous discovering callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the discovering property
     */
    void enable_discovering_notifications(std::function<void(bool discovering)> callback);
    /**
     * Disables notifications for changes of the discovering status of the adapter and uninstalls any callback;
     */
    void disable_discovering_notifications();

    /** Returns the UUIDs of the adapter.
      * @return Array containing the UUIDs of the adapter, ends with NULL.
      */
    std::vector<std::string> get_uuids ();

    /** Returns the local ID of the adapter.
      * @return The local ID of the adapter.
      */
    std::unique_ptr<std::string> get_modalias ();

};
