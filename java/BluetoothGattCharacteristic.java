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
  * Provides access to Bluetooth GATT characteristic. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/gatt-api.txt
  */
public class BluetoothGattCharacteristic extends BluetoothObject
{
    public native BluetoothType getBluetoothType();
    public native BluetoothGattCharacteristic clone();

    static BluetoothType class_type() { return BluetoothType.GATT_CHARACTERISTIC; }

    /** Find a BluetoothGattDescriptor. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattDescriptor you are
      * waiting for
      * @parameter timeout the function will return after timeout time, a
      * value of zero means wait forever. If object is not found during this time null will be returned.
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothGattDescriptor find(String UUID, Duration duration) {
            BluetoothManager manager = BluetoothManager.getBluetoothManager();
            return (BluetoothGattDescriptor) manager.find(BluetoothType.GATT_DESCRIPTOR,
                null, UUID, this, duration);
    }

    /** Find a BluetoothGattDescriptor. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattDescriptor you are
      * waiting for
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothGattDescriptor find(String UUID) {
            return find(UUID, Duration.ZERO);
    }

    /* D-Bus method calls: */
    /** Reads the value of this characteristic.
      * @return A std::vector<unsgined char> containing the value of this characteristic.
      */
    public native byte[] readValue() throws BluetoothException;

    /**
     * Enables notifications for the value and calls run function of the BluetoothNotification
     * object. It enables notifications for this characteristic at BLE level.
     * @param callback A BluetoothNotification<byte[]> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the value
     * property.
     */
    public native void enableValueNotifications(BluetoothNotification<byte[]> callback);
    /**
     * Disables notifications of the value and unregisters the callback object
     * passed through the corresponding enable method. It disables notications
     * at BLE level for this characteristic.
     */
    public native void disableValueNotifications();

    /** Writes the value of this characteristic.
      * @param[in] arg_value The data as vector<uchar>
      * to be written packed in a GBytes struct
      * @return TRUE if value was written succesfully
      */
    public native boolean writeValue(byte[] argValue) throws BluetoothException;


    /* D-Bus property accessors: */
    /** Get the UUID of this characteristic.
      * @return The 128 byte UUID of this characteristic, NULL if an error occurred
      */
    public native String getUUID();

    /** Returns the service to which this characteristic belongs to.
      * @return The service.
      */
    public native BluetoothGattService getService();

    /** Returns the cached value of this characteristic, if any.
      * @return The cached value of this characteristic.
      */
    public native byte[] getValue();

    /** Returns true if notification for changes of this characteristic are
      * activated.
      * @return True if notificatios are activated.
      */
    public native boolean getNotifying();

    /** Returns the flags this characterstic has.
      * @return A list of flags for this characteristic.
      */
    public native String[] getFlags();

    /** Returns a list of BluetoothGattDescriptors this characteristic exposes.
      * @return A list of BluetoothGattDescriptors exposed by this characteristic
      * NULL if an error occurred
      */
    public native List<BluetoothGattDescriptor> getDescriptors();

    private native void init(BluetoothGattCharacteristic obj);

    private native void delete();

    private BluetoothGattCharacteristic(long instance)
    {
        super(instance);
    }
}
