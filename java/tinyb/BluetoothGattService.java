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
public class BluetoothGattService extends BluetoothObject
{
    public native BluetoothType getBluetoothType();
    public native BluetoothAdapter clone();

    static BluetoothType class_type() { return BluetoothType.GATT_SERVICE; }

    /** Find a BluetoothGattCharacteristic. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattCharacteristic you are
      * waiting for
      * @parameter timeout the function will return after timeout time, a
      * value of zero means wait forever. If object is not found during this time null will be returned.
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothGattCharacteristic find(String UUID, Duration duration) {
            BluetoothManager manager = BluetoothManager.getBluetoothManager();
            return (BluetoothGattCharacteristic) manager.find(BluetoothType.GATT_CHARACTERISTIC,
                null, UUID, this, duration);
    }

    /** Find a BluetoothGattCharacteristic. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattDescriptor you are
      * waiting for
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothGattCharacteristic find(String UUID) {
            return find(UUID, Duration.ZERO);
    }

    /* D-Bus property accessors: */

    /** Get the UUID of this service
      * @return The 128 byte UUID of this service, NULL if an error occurred
      */
    public native String getUUID();

    /** Returns the device to which this service belongs to.
      * @return The device.
      */
    public native BluetoothDevice getDevice();

    /** Returns true if this service is a primary service, false if secondary.
      * @return true if this service is a primary service, false if secondary.
      */
    public native boolean getPrimary();

    /** Returns a list of BluetoothGattCharacteristics this service exposes.
      * @return A list of BluetoothGattCharacteristics exposed by this service
      */
    public native List<BluetoothGattCharacteristic> getCharacteristics();

    private native void delete();

    private BluetoothGattService(long instance)
    {
        super(instance);
    }
}
