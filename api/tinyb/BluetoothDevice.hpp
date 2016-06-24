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

    /** Registers a callback which will be called when the paired property changes.
      * @param callback The callback function to be called.
      * @param device Will contain a reference to this device
      * @param paired Will contain the new value of the paired property
      * @param userdata Data provided by the user to be attached to the callback. Can be nullptr. The caller retains ownership of this data and will have to clear it after disabling this notification.
      */
    void enable_paired_notifications(
        std::function<void(BluetoothDevice &device, bool paired, void *userdata)> callback,
        void *userdata);
    /** Registers a callback which will be called when the paired property changes.
      * @param callback The callback function to be called.
      * @param paired Will contain the new value of the paired property
      */
    void enable_paired_notifications(
        std::function<void(bool paired)> callback);
    /** Unregisters the callback set with enable_paired_notifications. No more notifications will
      * be sent after this operation completes.
      */
    void disable_paired_notifications();

    /** Returns the trusted state the device.
      * @return The trusted state of the device.
      */
    bool get_trusted ();

    /** Sets the trusted state the device.
      */
    void set_trusted (bool  value);

    /** Registers a callback which will be called when the trusted property changes.
      * @param callback The callback function to be called.
      * @param device Will contain a reference to this device
      * @param trusted Will contain the new value of the trusted property
      * @param userdata Data provided by the user to be attached to the callback. Can be nullptr. The caller retains ownership of this data and will have to clear it after disabling this notification.
      */
    void enable_trusted_notifications(
        std::function<void(BluetoothDevice &device, bool trusted, void *userdata)> callback,
        void *userdata);
    void enable_trusted_notifications(
        std::function<void(bool trusted)> callback);
    /** Unregisters the callback set with enable_trusted_notifications. No more notifications will
      * be sent after this operation completes.
      */
    void disable_trusted_notifications();

    /** Returns the blocked state the device.
      * @return The blocked state of the device.
      */
    bool get_blocked ();

    /** Sets the blocked state the device.
      */
    void set_blocked (bool  value);

    /** Registers a callback which will be called when the blocked property changes.
      * @param callback The callback function to be called.
      * @param device Will contain a reference to this device
      * @param blocked Will contain the new value of the trusted property
      * @param userdata Data provided by the user to be attached to the callback. Can be nullptr.
      * The caller retains ownership of this data and might have to deallocate it after disabling this notification.
      */
    void enable_blocked_notifications(
        std::function<void(BluetoothDevice &device, bool blocked, void *userdata)> callback,
        void *userdata);
    void enable_blocked_notifications(
        std::function<void(bool blocked)> callback);
    /** Unregisters the callback set with enable_trusted_notifications. No more notifications will
      * be sent after this operation completes.
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


    /** Registers a callback which will be called when the RSSI property changes.
      * @param callback The callback function to be called.
      * @param device Will contain a reference to this device
      * @param rssi Will contain the new value of the rssi property
      * @param userdata Data provided by the user to be attached to the callback. Can be nullptr. The caller retains ownership of this data and will have to clear it after disabling this notification.
      */
    void enable_rssi_notifications(
        std::function<void(BluetoothDevice &device, int16_t rssi, void *userdata)> callback,
        void *userdata = nullptr);
   /** Registers a callback which will be called when the RSSI property changes.
      * @param callback The callback function to be called.
      * @param rssi Will contain the new value of the rssi property
      */
    void enable_rssi_notifications(
        std::function<void(int16_t rssi)> callback);
    /** Unregisters the callback set with enable_rssi_notifications. No more notifications will
      * be sent after this operation completes.
      */
    void disable_rssi_notifications();

    /** Returns the connected state of the device.
      * @return The connected state of the device.
      */
    bool get_connected ();

    /** Registers a callback which will be called when the connected property changes.
      * @param callback The callback function to be called.
      * @param device Will contain a reference to this device
      * @param connected Will contain the new value of the connected property
      * @param userdata Data provided by the user to be attached to the callback. Can be nullptr. The caller retains ownership of this data and will have to clear it after disabling this notification.
      */
    void enable_connected_notifications(
        std::function<void(BluetoothDevice &device, bool connected, void *userdata)> callback,
        void *userdata);
    /** Registers a callback which will be called when the connected property changes.
      * @param callback The callback function to be called.
      * @param connected Will contain the new value of the connected property
      */
    void enable_connected_notifications(
        std::function<void(bool connected)> callback);
    /** Unregisters the callback set with enable_connected_notifications. No more notifications will
      * be sent after this operation completes.
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
};
