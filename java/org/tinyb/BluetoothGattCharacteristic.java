/**
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
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

package org.tinyb;

import java.util.List;

/**
  * Provides access to Bluetooth GATT characteristic. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/gatt-api.txt
  */
public interface BluetoothGattCharacteristic extends BluetoothObject
{
    @Override
    public BluetoothGattCharacteristic clone();

    /** Find a BluetoothGattDescriptor. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattDescriptor you are
      * waiting for
      * @parameter timeoutMS the function will return after timeout time in milliseconds, a
      * value of zero means wait forever. If object is not found during this time null will be returned.
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothGattDescriptor find(final String UUID, final long timeoutMS);

    /** Find a BluetoothGattDescriptor. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattDescriptor you are
      * waiting for
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothGattDescriptor find(final String UUID);

    /* D-Bus method calls: */

    /** Reads the value of this characteristic.
      * @return A std::vector<unsgined char> containing the value of this characteristic.
      */
    public byte[] readValue() throws BluetoothException;

    /**
     * Add the given {@link GATTCharacteristicListener} to the listener list if not already present.
     * @param listener A {@link GATTCharacteristicListener} instance, listening to this {@link BluetoothGattCharacteristic}'s events
     * @return true if the given listener is not element of the list and has been newly added, otherwise false.
     * @since 2.0.0
     */
    public boolean addCharacteristicListener(final GATTCharacteristicListener listener);

    /**
     * Remove the given {@link GATTCharacteristicListener} from the listener list.
     * @param listener A {@link GATTCharacteristicListener} instance
     * @return true if the given listener is an element of the list and has been removed, otherwise false.
     * @since 2.0.0
     */
    public boolean removeCharacteristicListener(final GATTCharacteristicListener l);

    /**
     * Remove all {@link GATTCharacteristicListener} from the list.
     * @return number of removed listener.
     * @since 2.0.0
     */
    public int removeAllCharacteristicListener();

    /**
     * Enables notifications for the value and calls run function of the BluetoothNotification
     * object. It enables notifications for this characteristic at BLE level.
     * @param callback A BluetoothNotification<byte[]> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the value
     * property.
     */
    public void enableValueNotifications(BluetoothNotification<byte[]> callback);

    /**
     * Disables notifications of the value and unregisters the callback object
     * passed through the corresponding enable method. It disables notications
     * at BLE level for this characteristic.
     */
    public void disableValueNotifications();

    /** Writes the value of this characteristic.
      * @param[in] arg_value The data as vector<uchar>
      * to be written packed in a GBytes struct
      * @return TRUE if value was written succesfully
      */
    public boolean writeValue(byte[] argValue) throws BluetoothException;


    /* D-Bus property accessors: */

    /** Get the UUID of this characteristic.
      * @return The 128 byte UUID of this characteristic, NULL if an error occurred
      */
    public String getUUID();

    /** Returns the service to which this characteristic belongs to.
      * @return The service.
      */
    public BluetoothGattService getService();

    /** Returns the cached value of this characteristic, if any.
      * @return The cached value of this characteristic.
      */
    public byte[] getValue();

    /** Returns true if notification for changes of this characteristic are
      * activated.
      * @return True if notificatios are activated.
      */
    public boolean getNotifying();

    /**
     * Returns the flags this characterstic has.
     * <p>
     * These flags are actually the BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1.1 Characteristic Properties
     * </p>
     * <p>
     * Returns string values as defined in <https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/gatt-api.txt>
     * <pre>
     * org.bluez.GattCharacteristic1 :: array{string} Flags [read-only]
     * </pre>
     * </p>
     * @return A list of flags for this characteristic.
     */
    public String[] getFlags();

    /** Returns a list of BluetoothGattDescriptors this characteristic exposes.
      * @return A list of BluetoothGattDescriptors exposed by this characteristic
      * NULL if an error occurred
      */
    public List<BluetoothGattDescriptor> getDescriptors();
}
