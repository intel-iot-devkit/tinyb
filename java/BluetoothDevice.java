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

import java.util.*;

/**
  * Provides access to Bluetooth devices. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/device-api.txt
  */
public class BluetoothDevice extends BluetoothObject
{
    private long nativeInstance;

    public native BluetoothType getBluetoothType();
    public native BluetoothDevice clone();

    /* D-Bus method calls: */
    /** The connection to this device is removed, removing all connected
      * profiles.
      * @return TRUE if the device disconnected
      */
    public native boolean disconnect();

    /** A connection to this device is established, connecting each profile
      * flagged as auto-connectable.
      * @return TRUE if the device connected
      */
    public native boolean connect();

     /** Connects a specific profile available on the device, given by UUID
      * @param arg_UUID The UUID of the profile to be connected
      * @return TRUE if the profile connected successfully
      */
    public native boolean connectProfile(String arg_UUID);

    /** Disconnects a specific profile available on the device, given by UUID
      * @param arg_UUID The UUID of the profile to be disconnected
      * @return TRUE if the profile disconnected successfully
      */
    public native boolean disconnectProfile(String arg_UUID);

    /** A connection to this device is established, and the device is then
      * paired.
      * @return TRUE if the device connected and paired
      */
    public native boolean pair();

    /** Cancels an initiated pairing operation
      * @return TRUE if the paring is cancelled successfully
      */
    public native boolean cancelPairing();

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

    /** Returns the trusted state the device.
      * @return The trusted state of the device.
      */
    public native boolean getTrusted();

    /** Sets the trusted state the device.
      */
    public native void setTrusted(boolean value);

    /** Returns the blocked state the device.
      * @return The blocked state of the device.
      */
    public native boolean getBlocked();

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
    public native short getRssi();

    /** Returns the connected state of the device.
      * @return The connected state of the device.
      */
    public native boolean getConnected();

    /** Returns the UUIDs of the device.
      * @return Array containing the UUIDs of the device, ends with NULL.
      */
    public native String[] getUuids();

    /** Returns the local ID of the adapter.
      * @return The local ID of the adapter.
      */
    public native String getModalias();

    /** Returns the adapter on which this device was discovered or
      * connected.
      * @return The adapter.
      */
    public native BluetoothAdapter getAdapter();

    private native void delete();

    private BluetoothDevice(long instance)
    {
        super(instance);
        nativeInstance = instance;
    }

    protected void finalize()
    {
        delete();
    }
}
