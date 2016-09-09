/*
 * Author: Andrei Vasiliu <andrei.vasiliu@intel.com>
 * Copyright (c) 2016 Intel Corporation.
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

package tinyb;

import java.util.*;
import java.time.Duration;

/**
  * Provides access to Bluetooth devices. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/device-api.txt
  */
public class BluetoothDevice extends BluetoothObject
{
    public native BluetoothType getBluetoothType();
    public native BluetoothDevice clone();

    static BluetoothType class_type() { return BluetoothType.DEVICE; }

    /** Find a BluetoothGattService. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattService you are
      * waiting for
      * @parameter timeout the function will return after timeout time, a
      * value of zero means wait forever. If object is not found during this time null will be returned.
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothGattService find(String UUID, Duration duration) {
            BluetoothManager manager = BluetoothManager.getBluetoothManager();
            return (BluetoothGattService) manager.find(BluetoothType.GATT_SERVICE,
                null, UUID, this, duration);
    }

    /** Find a BluetoothGattService. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattService you are
      * waiting for
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothGattService find(String UUID) {
            return find(UUID, Duration.ZERO);
    }

    /* D-Bus method calls: */
    /** The connection to this device is removed, removing all connected
      * profiles.
      * @return TRUE if the device disconnected
      */
    public native boolean disconnect() throws BluetoothException;

    /** A connection to this device is established, connecting each profile
      * flagged as auto-connectable.
      * @return TRUE if the device connected
      */
    public native boolean connect() throws BluetoothException;

     /** Connects a specific profile available on the device, given by UUID
      * @param arg_UUID The UUID of the profile to be connected
      * @return TRUE if the profile connected successfully
      */
    public native boolean connectProfile(String arg_UUID) throws BluetoothException;

    /** Disconnects a specific profile available on the device, given by UUID
      * @param arg_UUID The UUID of the profile to be disconnected
      * @return TRUE if the profile disconnected successfully
      */
    public native boolean disconnectProfile(String arg_UUID) throws BluetoothException;

    /** A connection to this device is established, and the device is then
      * paired.
      * @return TRUE if the device connected and paired
      */
    public native boolean pair() throws BluetoothException;

    /** Cancels an initiated pairing operation
      * @return TRUE if the paring is cancelled successfully
      */
    public native boolean cancelPairing() throws BluetoothException;

    /** Returns a list of BluetoothGattServices available on this device.
      * @return A list of BluetoothGattServices available on this device,
      * NULL if an error occurred
      */
    public native List<BluetoothGattService> getServices();

    /* D-Bus property accessors: */
    /** Returns the hardware address of this device.
      * @return The hardware address of this device.
      */
    public native String getAddress();

    /** Returns the remote friendly name of this device.
      * @return The remote friendly name of this device, or NULL if not set.
      */
    public native String getName();

    /** Returns an alternative friendly name of this device.
      * @return The alternative friendly name of this device, or NULL if not set.
      */
    public native String getAlias();

    /** Sets an alternative friendly name of this device.
      */
    public native void setAlias(String value);

    /** Returns the Bluetooth class of the device.
      * @return The Bluetooth class of the device.
      */
    public native int getBluetoothClass();

    /** Returns the appearance of the device, as found by GAP service.
      * @return The appearance of the device, as found by GAP service.
      */
    public native short getAppearance();

    /** Returns the proposed icon name of the device.
      * @return The proposed icon name, or NULL if not set.
      */
    public native String getIcon();

    /** Returns the paired state the device.
      * @return The paired state of the device.
      */
    public native boolean getPaired();

    /**
     * Enables notifications for the paired property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the paired
     * property.
     */
    public native void enablePairedNotifications(BluetoothNotification<Boolean> callback);
    /**
     * Disables notifications of the paired property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    public native void disablePairedNotifications();

    /** Returns the trusted state the device.
      * @return The trusted state of the device.
      */
    public native boolean getTrusted();

    /**
     * Enables notifications for the trusted property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the trusted
     * property.
     */
    public native void enableTrustedNotifications(BluetoothNotification<Boolean> callback);
    /**
     * Disables notifications of the trusted property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    public native void disableTrustedNotifications();

    /** Sets the trusted state the device.
      */
    public native void setTrusted(boolean value);

    /** Returns the blocked state the device.
      * @return The blocked state of the device.
      */
    public native boolean getBlocked();

    /**
     * Enables notifications for the blocked property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the blocked
     * property.
     */
    public native void enableBlockedNotifications(BluetoothNotification<Boolean> callback);
    /**
     * Disables notifications of the blocked property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    public native void disableBlockedNotifications();

    /** Sets the blocked state the device.
      */
    public native void setBlocked(boolean value);

    /** Returns if device uses only pre-Bluetooth 2.1 pairing mechanism.
      * @return True if device uses only pre-Bluetooth 2.1 pairing mechanism.
      */
    public native boolean getLegacyPairing();

    /** Returns the Received Signal Strength Indicator of the device.
      * @return The Received Signal Strength Indicator of the device.
      */
    public native short getRSSI();

    /**
     * Enables notifications for the RSSI property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Short> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the RSSI
     * property.
     */
    public native void enableRSSINotifications(BluetoothNotification<Short> callback);
    /**
     * Disables notifications of the RSSI property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    public native void disableRSSINotifications();

    /** Returns the connected state of the device.
      * @return The connected state of the device.
      */
    public native boolean getConnected();

    /**
     * Enables notifications for the connected property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the connected
     * property.
     */
    public native void enableConnectedNotifications(BluetoothNotification<Boolean> callback);
    /**
     * Disables notifications of the connected property and unregisters the callback
     * object passed through the corresponding enable method.
     */
     public native void disableConnectedNotifications();

    /** Returns the UUIDs of the device.
      * @return Array containing the UUIDs of the device, ends with NULL.
      */
    public native String[] getUUIDs();

    /** Returns the local ID of the adapter.
      * @return The local ID of the adapter.
      */
    public native String getModalias();

    /** Returns the adapter on which this device was discovered or
      * connected.
      * @return The adapter.
      */
    public native BluetoothAdapter getAdapter();

    /** Returns a map containing manufacturer specific advertisement data.
      * An entry has a uint16_t key and an array of bytes.
      * @return manufacturer specific advertisement data.
      */
    public native Map<Short, byte[]> getManufacturerData();

    /** Returns a map containing service advertisement data.
      * An entry has a UUID string key and an array of bytes.
      * @return service advertisement data.
      */
    public native Map<String, byte[]> getServiceData();

     /** Returns the transmission power level (0 means unknown).
      * @return the transmission power level (0 means unknown).
      */
    public native short getTxPower ();

     /** Returns true if service discovery has ended.
      * @return true if the service discovery has ended.
      */
    public native boolean getServicesResolved ();

    private native void delete();

    private BluetoothDevice(long instance)
    {
        super(instance);
    }
}
