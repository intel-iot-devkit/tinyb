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
  * Provides access to Bluetooth adapters. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/adapter-api.txt
  */
public class BluetoothAdapter extends BluetoothObject
{
    public native BluetoothType getBluetoothType();
    public native BluetoothAdapter clone();

    static BluetoothType class_type() { return BluetoothType.ADAPTER; }

    /** Find a BluetoothDevice. If parameters name and address are not null,
      * the returned object will have to match them.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter name optionally specify the name of the object you are
      * waiting for
      * @parameter address optionally specify the MAC address of the device you are
      * waiting for
      * @parameter timeout the function will return after timeout time, a
      * value of zero means wait forever. If object is not found during this time null will be returned.
      * @return An object matching the name and address or null if not found before
      * timeout expires.
      */
    public BluetoothDevice find(String name, String address, Duration duration) {
            BluetoothManager manager = BluetoothManager.getBluetoothManager();
            return (BluetoothDevice) manager.find(BluetoothType.DEVICE, name, address, this, duration);
    }

    /** Find a BluetoothDevice. If parameters name and address are not null,
      * the returned object will have to match them.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter name optionally specify the name of the object you are
      * waiting for
      * @parameter address optionally specify the MAC address of the device you are
      * waiting for
      * @return An object matching the name and address.
      */
    public BluetoothDevice find(String name, String address) {
            return find(name, address, Duration.ZERO);
    }

    /* D-Bus method calls: */
    /** Turns on device discovery if it is disabled.
      * @return TRUE if discovery was successfully enabled
      */
    public native boolean startDiscovery() throws BluetoothException;

    /** Turns off device discovery if it is enabled.
      * @return TRUE if discovery was successfully disabled
      */
    public native boolean stopDiscovery() throws BluetoothException;

    /** Returns a list of BluetoothDevices visible from this adapter.
      * @return A list of BluetoothDevices visible on this adapter,
      * NULL if an error occurred
      */
    public native List<BluetoothDevice> getDevices();

    /* D-Bus property accessors: */
    /** Returns the hardware address of this adapter.
      * @return The hardware address of this adapter.
      */
    public native String getAddress();

    /** Returns the system name of this adapter.
      * @return The system name of this adapter.
      */
    public native String getName();

    /** Returns the friendly name of this adapter.
      * @return The friendly name of this adapter, or NULL if not set.
      */
    public native String getAlias();

    /** Sets the friendly name of this adapter.
      */
    public native void setAlias(String value);

    /** Returns the Bluetooth class of the adapter.
      * @return The Bluetooth class of the adapter.
      */
    public native long getBluetoothClass();

    /** Returns the power state the adapter.
      * @return The power state of the adapter.
      */
    public native boolean getPowered();

    /**
     * Enables notifications for the powered property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the powered
     * property.
     */
    public native void enablePoweredNotifications(BluetoothNotification<Boolean> callback);
    /**
     * Disables notifications of the powered property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    public native void disablePoweredNotifications();

    /** Sets the power state the adapter.
      */
    public native void setPowered(boolean value);

    /** Returns the discoverable state the adapter.
      * @return The discoverable state of the adapter.
      */
    public native boolean getDiscoverable();

    /**
     * Enables notifications for the discoverable property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the discoverable
     * property.
     */
    public native void enableDiscoverableNotifications(BluetoothNotification<Boolean> callback);
    /**
     * Disables notifications of the discoverable property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    public native void disableDiscoverableNotifications();

    /** Sets the discoverable state the adapter.
      */
    public native void setDiscoverable(boolean value);

    /** Returns the discoverable timeout the adapter.
      * @return The discoverable timeout of the adapter.
      */
    public native long getDiscoverableTimeout();

    /** Sets the discoverable timeout the adapter. A value of 0 disables
      * the timeout.
      */
    public native void setDiscoverableTimout(long value);

    /** Returns the pairable state the adapter.
      * @return The pairable state of the adapter.
      */
    public native boolean getPairable();

    /**
     * Enables notifications for the pairable property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the pairable
     * property.
     */
    public native void enablePairableNotifications(BluetoothNotification<Boolean> callback);
    /**
     * Disables notifications of the pairable property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    public native void disablePairableNotifications();

    /** Sets the discoverable state the adapter.
      */
    public native void setPairable(boolean value);

    /** Returns the timeout in seconds after which pairable state turns off
      * automatically, 0 means never.
      * @return The pairable timeout of the adapter.
      */
    public native long getPairableTimeout();

    /** Sets the timeout after which pairable state turns off automatically, 0 means never.
      */
    public native void setPairableTimeout(long value);

    /** Returns the discovering state the adapter. It can be modified through
      * start_discovery/stop_discovery functions.
      * @return The discovering state of the adapter.
      */
    public native boolean getDiscovering();

    /**
     * Enables notifications for the discovering property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the discovering
     * property.
     */
    public native void enableDiscoveringNotifications(BluetoothNotification<Boolean> callback);
    /**
     * Disables notifications of the discovering property and unregisters the discovering
     * object passed through the corresponding enable method.
     */
    public native void disableDiscoveringNotifications();

    /** Returns the UUIDs of the adapter.
      * @return Array containing the UUIDs of the adapter, ends with NULL.
      */
    public native String[] getUUIDs();

    /** Returns the local ID of the adapter.
      * @return The local ID of the adapter.
      */
    public native String getModalias();

    private native void delete();

    private BluetoothAdapter(long instance)
    {
        super(instance);
    }
}
