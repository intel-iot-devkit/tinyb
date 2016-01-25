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
  * Provides access to Bluetooth GATT descriptor. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/gatt-api.txt
  */
public class BluetoothGattDescriptor extends BluetoothObject
{
    public long nativeInstance;

    public native BluetoothType getBluetoothType();
    public native BluetoothAdapter clone();

    /* D-Bus method calls: */
    /** Reads the value of this descriptor
      * @return A vector<uchar> containing data from this descriptor
      */
    public native byte[] readValue();

    /** Writes the value of this descriptor.
      * @param[in] arg_value The data as vector<uchar>
      * to be written packed in a GBytes struct
      * @return TRUE if value was written succesfully
      */
    public native boolean writeValue(byte[] argValue);

    /* D-Bus property accessors: */
    /** Get the UUID of this descriptor.
      * @return The 128 byte UUID of this descriptor, NULL if an error occurred
      */
    public native String getUuid();

    /** Returns the characteristic to which this descriptor belongs to.
      * @return The characteristic.
      */
    public native BluetoothGattCharacteristic getCharacteristic();

    /** Returns the cached value of this descriptor, if any.
      * @return The cached value of this descriptor.
      */
    public native byte[] getValue();

    private native void delete();

    private BluetoothGattDescriptor(long instance)
    {
        super(instance);
        nativeInstance = instance;
    }

    protected void finalize()
    {
        delete();
    }
}
