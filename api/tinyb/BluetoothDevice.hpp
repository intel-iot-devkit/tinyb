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
#include "BluetoothGattService.hpp"
#include "BluetoothManager.hpp"
#include <cstdint>
#include <vector>
#include <functional>
#include <map>

/* Forward declaration of types */
struct _Object;
typedef struct _Object Object;
struct _Device1;
typedef struct _Device1 Device1;

/**
  * Provides access to Bluetooth devices. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/device-api.txt
  */
class tinyb::BluetoothDevice: public BluetoothObject
{

friend class tinyb::BluetoothManager;
friend class tinyb::BluetoothEventManager;
friend class tinyb::BluetoothAdapter;
friend class tinyb::BluetoothGattService;
friend class tinyb::BluetoothNotificationHandler;

private:
    Device1 *object;

protected:
    BluetoothDevice(Device1 *object);

    static std::unique_ptr<BluetoothDevice> make(Object *object,
        BluetoothType type = BluetoothType::DEVICE,
        std::string *name = nullptr,
        std::string *identifier = nullptr,
        BluetoothObject *parent = nullptr);

    std::function<void(int16_t)> rssi_callback;
    std::function<void(bool)> trusted_callback;
    std::function<void(bool)> paired_callback;
    std::function<void(bool)> connected_callback;
    std::function<void(bool)> blocked_callback;
    std::function<void(std::map<uint16_t, std::vector<uint8_t>> &)> mfg_callback;
    std::function<void(std::map<std::string, std::vector<uint8_t>> &)> service_callback;
    std::function<void(bool)> services_resolved_callback;

public:

    static std::string java_class() {
        return std::string(JAVA_PACKAGE "/BluetoothDevice");
    }
    static BluetoothType class_type() { return BluetoothType::DEVICE; }

    virtual std::string get_java_class() const;
    virtual std::string get_class_name() const;
    virtual std::string get_object_path() const;
    virtual BluetoothType get_bluetooth_type() const;

    BluetoothDevice(const BluetoothDevice &object);
    ~BluetoothDevice();
    virtual BluetoothDevice *clone() const;

    std::unique_ptr<BluetoothGattService> find(
        std::string *identifier,
        std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
    {

        BluetoothManager *manager = BluetoothManager::get_bluetooth_manager();
        return manager->find<BluetoothGattService>(nullptr, identifier, this, timeout);
    }

    /* D-Bus method calls: */

    /** The connection to this device is removed, removing all connected
      * profiles.
      * @return TRUE if the device disconnected
      */
    bool disconnect (
    );

    /** A connection to this device is established, connecting each profile
      * flagged as auto-connectable.
      * @return TRUE if the device connected
      */
    bool connect (
    );

    /** Connects a specific profile available on the device, given by UUID
      * @param arg_UUID The UUID of the profile to be connected
      * @return TRUE if the profile connected successfully
      */
    bool connect_profile (
        const std::string &arg_UUID
    );

    /** Disconnects a specific profile available on the device, given by UUID
      * @param arg_UUID The UUID of the profile to be disconnected
      * @return TRUE if the profile disconnected successfully
      */
    bool disconnect_profile (
        const std::string &arg_UUID
    );

    /** A connection to this device is established, and the device is then
      * paired.
      * @return TRUE if the device connected and paired
      */
    bool pair (
    );
	
	/** Remove the current device (like an unpair).
      * @return true if the device has been removed from the system.
      */
    bool remove_device(
	);

    /** Cancels an initiated pairing operation
      * @return TRUE if the paring is cancelled successfully
      */
    bool cancel_pairing (
    );

    /** Returns a list of BluetoothGattServices available on this device.
      * @return A list of BluetoothGattServices available on this device,
      * NULL if an error occurred
      */
    std::vector<std::unique_ptr<BluetoothGattService>> get_services (
    );

    /* D-Bus property accessors: */
    /** Returns the hardware address of this device.
      * @return The hardware address of this device.
      */
    std::string get_address ();

    /** Returns the remote friendly name of this device.
      * @return The remote friendly name of this device, or NULL if not set.
      */
    std::string get_name ();

    /** Returns an alternative friendly name of this device.
      * @return The alternative friendly name of this device, or NULL if not set.
      */
    std::string get_alias ();

    /** Sets an alternative friendly name of this device.
      */
    void set_alias (const std::string &value);

    /** Returns the Bluetooth class of the device.
      * @return The Bluetooth class of the device.
      */
    unsigned int get_class ();

    /** Returns the appearance of the device, as found by GAP service.
      * @return The appearance of the device, as found by GAP service.
      */
    uint16_t get_appearance ();

    /** Returns the proposed icon name of the device.
      * @return The proposed icon name, or NULL if not set.
      */
    std::unique_ptr<std::string> get_icon ();

    /** Returns the paired state the device.
      * @return The paired state of the device.
      */
    bool get_paired ();

    /**
     * Enables notifications for changes of the paired status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous paired callback, if any was installed.
     * @param callback A function of the form void(BluetoothDevice&, bool, void *), where
     * BluetoothDevice& is the device for which the callback was set, bool will contain the
     * new value of the paired property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_paired_notifications(
        std::function<void(BluetoothDevice &device, bool paired, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the paired status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous paired callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the paired property
     */
    void enable_paired_notifications(
        std::function<void(bool paired)> callback);
    /**
     * Disables notifications for changes of the paired status of the device
     * and uninstalls any callback.
     */
    void disable_paired_notifications();

    /** Returns the trusted state the device.
      * @return The trusted state of the device.
      */
    bool get_trusted ();

    /** Sets the trusted state the device.
      */
    void set_trusted (bool  value);

    /**
     * Enables notifications for changes of the trusted status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous trusted callback, if any was installed.
     * @param callback A function of the form void(BluetoothDevice&, bool, void *), where
     * BluetoothDevice& is the device for which the callback was set, bool will contain the
     * new value of the trusted property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_trusted_notifications(
        std::function<void(BluetoothDevice &device, bool trusted, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the trusted status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous trusted callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the trusted property
     */
    void enable_trusted_notifications(
        std::function<void(bool trusted)> callback);
    /**
     * Disables notifications for changes of the trusted status of the device
     * and uninstalls any callback.
     */
    void disable_trusted_notifications();

    /** Returns the blocked state the device.
      * @return The blocked state of the device.
      */
    bool get_blocked ();

    /** Sets the blocked state the device.
      */
    void set_blocked (bool  value);

    /**
     * Enables notifications for changes of the blocked status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous blocked callback, if any was installed.
     * @param callback A function of the form void(BluetoothDevice&, bool, void *), where
     * BluetoothDevice& is the device for which the callback was set, bool will contain the
     * new value of the blocked property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_blocked_notifications(
        std::function<void(BluetoothDevice &device, bool blocked, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the blocked status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous blocked callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the blocked property
     */
    void enable_blocked_notifications(
        std::function<void(bool blocked)> callback);
    /**
     * Disables notifications for changes of the blocked status of the device
     * and uninstalls any callback.
     */
    void disable_blocked_notifications();

    /** Returns if device uses only pre-Bluetooth 2.1 pairing mechanism.
      * @return True if device uses only pre-Bluetooth 2.1 pairing mechanism.
      */
    bool get_legacy_pairing ();

    /** Returns the Received Signal Strength Indicator of the device (0 means unknown).
      * @return The Received Signal Strength Indicator of the device (0 means unknown).
      */
    int16_t get_rssi ();


    /**
     * Enables notifications for changes of the RSSI value of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous RSSI callback, if any was installed.
     * @param callback A function of the form void(BluetoothDevice&, int16_t, void *), where
     * BluetoothDevice& is the device for which the callback was set, bool will contain the
     * new value of the RSSI property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_rssi_notifications(
        std::function<void(BluetoothDevice &device, int16_t rssi, void *userdata)> callback,
        void *userdata = nullptr);
    /**
     * Enables notifications for changes of the RSSI value of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous RSSI callback, if any was installed.
     * @param callback A function of the form void(int16_t), where
     * bool will contain the new value of the RSSI property
     */
    void enable_rssi_notifications(
        std::function<void(int16_t rssi)> callback);
    /**
     * Disables notifications for changes of the RSSI value of the device
     * and uninstalls any callback.
     */
    void disable_rssi_notifications();

    /** Returns the connected state of the device.
      * @return The connected state of the device.
      */
    bool get_connected ();

    /**
     * Enables notifications for changes of the connected status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous connected callback, if any was installed.
     * @param callback A function of the form void(BluetoothDevice&, bool, void *), where
     * BluetoothDevice& is the device for which the callback was set, bool will contain the
     * new value of the connected property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_connected_notifications(
        std::function<void(BluetoothDevice &device, bool connected, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the connected status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous connected callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the connected property
     */
    void enable_connected_notifications(
        std::function<void(bool connected)> callback);
    /**
     * Disables notifications for changes of the connected status of the device
     * and uninstalls any callback.
     */
    void disable_connected_notifications();

    /** Returns the UUIDs of the device.
      * @return Array containing the UUIDs of the device, ends with NULL.
      */
    std::vector<std::string> get_uuids ();

    /** Returns the local ID of the adapter, or nullptr.
      * @return The local ID of the adapter, or nullptr.
      */
    std::unique_ptr<std::string> get_modalias ();

    /** Returns the adapter on which this device was discovered or
      * connected.
      * @return The adapter.
      */
    BluetoothAdapter get_adapter ();

    /** Returns a map containing manufacturer specific advertisement data.
      * An entry has a uint16_t key and an array of bytes.
      * @return manufacturer specific advertisement data.
      */
    std::map<uint16_t, std::vector<uint8_t>> get_manufacturer_data();
    /**
     * Enables notifications for changes of the manufacturer data of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous connected callback, if any was installed.
     * @param callback A function of the form void(BluetoothDevice&, bool, void *), where
     * BluetoothDevice& is the device for which the callback was set, bool will contain the
     * new value of the connected property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_manufacturer_data_notifications(
        std::function<void(BluetoothDevice &device, std::map<uint16_t, std::vector<uint8_t>> &mfgdata, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes in the manufacturer data of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous connected callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the connected property
     */
    void enable_manufacturer_data_notifications(
        std::function<void(std::map<uint16_t, std::vector<uint8_t>> &mfgdata)> callback);
    /**
     * Disables notifications for changes in the manufacturer data of the device
     * and uninstalls any callback.
     */
    void disable_manufacturer_data_notifications();


    /** Returns a map containing service advertisement data.
      * An entry has a UUID string key and an array of bytes.
      * @return service advertisement data.
      */
    std::map<std::string, std::vector<uint8_t>> get_service_data();
    /**
     * Enables notifications for changes of the service data of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous connected callback, if any was installed.
     * @param callback A function of the form void(BluetoothDevice&, bool, void *), where
     * BluetoothDevice& is the device for which the callback was set, bool will contain the
     * new value of the connected property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_service_data_notifications(
        std::function<void(BluetoothDevice &device, std::map<std::string, std::vector<uint8_t>> &servicedata, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes in the manufacturer data of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous connected callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the connected property
     */
    void enable_service_data_notifications(
        std::function<void(std::map<std::string, std::vector<uint8_t>> &servicedata)> callback);
    /**
     * Disables notifications for changes in the service data of the device
     * and uninstalls any callback.
     */
    void disable_service_data_notifications();

     /** Returns the transmission power level (0 means unknown).
      * @return the transmission power level (0 means unknown).
      */
    int16_t get_tx_power ();

     /** Returns true if service discovery has ended.
      * @return true if the service discovery has ended.
      */
    bool get_services_resolved ();
    /**
     * Enables notifications for changes of the services resolved status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous services resolved callback, if any was installed.
     * @param callback A function of the form void(BluetoothDevice&, bool, void *), where
     * BluetoothDevice& is the device for which the callback was set, bool will contain the
     * new value of the services resolved property and void * contains optional, user set data
     * @param userdata The data which will be delivered to the callback when it is triggered.
     * Memory must be managed by user.
     */
    void enable_services_resolved_notifications(
        std::function<void(BluetoothDevice &device, bool services_resolved, void *userdata)> callback,
        void *userdata);
    /**
     * Enables notifications for changes of the services resolved status of the device
     * and triggers the callback when the value changes.
     * Uninstalls the previous services resolved callback, if any was installed.
     * @param callback A function of the form void(bool), where
     * bool will contain the new value of the services resolved property
     */
    void enable_services_resolved_notifications(
        std::function<void(bool connec)> callback);
    /**
     * Disables notifications for changes of the services resolved status of the device
     * and uninstalls any callback.
     */
    void disable_services_resolved_notifications();

};
