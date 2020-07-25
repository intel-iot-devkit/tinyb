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
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
     * <p>
     * Method enables notification and/or indication for this characteristic at BLE level.
     * </p>
     * <p>
     * Implementation masks this Characteristic properties PropertyBitVal::Notify and PropertyBitVal::Indicate
     * with the respective user request parameters, hence removes unsupported requests.
     * </p>
     * <p>
     * Notification and/or indication configuration is only performed per characteristic if changed.
     * </p>
     * <p>
     * It is recommended to utilize notification over indication, as its link-layer handshake
     * and higher potential bandwidth may deliver material higher performance.
     * </p>
     * @param enableNotification
     * @param enableIndication
     * @param enabledState array of size 2, holding the resulting enabled state for notification and indication.
     * @return false if this characteristic has no PropertyBitVal::Notify or PropertyBitVal::Indication present,
     * or there is no GATTDescriptor of type ClientCharacteristicConfiguration, or if the operation has failed.
     * Otherwise returns true.
     * @throws IllegalStateException if notification or indication is set to be enabled
     * and the {@link BluetoothDevice}'s GATTHandler is null, i.e. not connected
     * @see #enableNotificationOrIndication(boolean[])
     * @since 2.0.0
     * @implNote not implemented in tinyb.dbus
     */
    public boolean configNotificationIndication(final boolean enableNotification, final boolean enableIndication, final boolean enabledState[/*2*/])
            throws IllegalStateException;

    /**
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
     * <p>
     * Method will attempt to enable notification on the BLE level, if available,
     * otherwise indication if available.
     * </p>
     * <p>
     * Notification and/or indication configuration is only performed per characteristic if changed.
     * </p>
     * <p>
     * It is recommended to utilize notification over indication, as its link-layer handshake
     * and higher potential bandwidth may deliver material higher performance.
     * </p>
     * @param enabledState array of size 2, holding the resulting enabled state for notification and indication.
     * @return false if this characteristic has no PropertyBitVal::Notify or PropertyBitVal::Indication present,
     * or there is no GATTDescriptor of type ClientCharacteristicConfiguration, or if the operation has failed.
     * Otherwise returns true.
     * @throws IllegalStateException if notification or indication is set to be enabled
     * and the {@link BluetoothDevice}'s GATTHandler is null, i.e. not connected
     * @see #configNotificationIndication(boolean, boolean, boolean[])
     * @since 2.0.0
     * @implNote not implemented in tinyb.dbus
     */
    public boolean enableNotificationOrIndication(final boolean enabledState[/*2*/])
            throws IllegalStateException;

    /**
     * Add the given {@link GATTCharacteristicListener} to the listener list if not already present.
     * <p>
     * Occurring notifications and indications, if enabled via {@link #configNotificationIndication(boolean, boolean, boolean[])}
     * or {@link #enableNotificationOrIndication(boolean[])},
     * will call the respective GATTCharacteristicListener callback method.
     * </p>
     * @param listener A {@link GATTCharacteristicListener} instance, listening to this {@link BluetoothGattCharacteristic}'s events
     * @return true if the given listener is not element of the list and has been newly added, otherwise false.
     * @throws IllegalStateException if the DBTDevice's GATTHandler is null, i.e. not connected
     * @throws IllegalStateException if the given {@link GATTCharacteristicListener} is already in use, i.e. added.
     * @see #enableNotificationOrIndication(boolean[])
     * @see #configNotificationIndication(boolean, boolean, boolean[])
     * @since 2.0.0
     * @implNote not implemented in tinyb.dbus
     */
    public boolean addCharacteristicListener(final GATTCharacteristicListener listener)
            throws IllegalStateException;

    /**
     * Add the given {@link GATTCharacteristicListener} to the listener list if not already present
     * and if enabling the notification <i>or</i> indication for this characteristic at BLE level was successful.<br>
     * Notification and/or indication configuration is only performed per characteristic if changed.
     * <p>
     * Implementation will attempt to enable notification only, if available,
     * otherwise indication if available. <br>
     * Implementation uses {@link #enableNotificationOrIndication(boolean[])} to enable one.
     * </p>
     * <p>
     * Occurring notifications and indications will call the respective {@link GATTCharacteristicListener}
     * callback method.
     * </p>
     * @param listener A {@link GATTCharacteristicListener} instance, listening to this {@link BluetoothGattCharacteristic}'s events
     * @param enabledState array of size 2, holding the resulting enabled state for notification and indication
     * using {@link #enableNotificationOrIndication(boolean[])}
     * @return true if enabling the notification and/or indication was successful
     * and if the given listener is not element of the list and has been newly added, otherwise false.
     * @throws IllegalStateException if the {@link BluetoothDevice}'s GATTHandler is null, i.e. not connected
     * @throws IllegalStateException if the given {@link GATTCharacteristicListener} is already in use, i.e. added.
     * @see #enableNotificationOrIndication(boolean[])
     * @see #configNotificationIndication(boolean, boolean, boolean[])
     * @since 2.0.0
     * @implNote not implemented in tinyb.dbus
     */
    public boolean addCharacteristicListener(final GATTCharacteristicListener listener, final boolean enabledState[/*2*/])
            throws IllegalStateException;

    /**
     * Disables the notification and/or indication for this characteristic at BLE level
     * if {@code disableIndicationNotification == true}
     * and removes the given {@link GATTCharacteristicListener} from the listener list.
     * <p>
     * If the DBTDevice's GATTHandler is null, i.e. not connected, {@code false} is being returned.
     * </p>
     *
     * @param listener A {@link GATTCharacteristicListener} instance
     * @param disableIndicationNotification if true, disables the notification and/or indication for this characteristic
     * using {@link #configNotificationIndication(boolean, boolean, boolean[])}
     * @return true if the given listener is an element of the list and has been removed, otherwise false.
     * @see #configNotificationIndication(boolean, boolean, boolean[])
     * @since 2.0.0
     * @implNote not implemented in tinyb.dbus
     */
    public boolean removeCharacteristicListener(final GATTCharacteristicListener l, final boolean disableIndicationNotification);

    /**
     * Disables the notification and/or indication for this characteristic BLE level
     * if {@code disableIndicationNotification == true}
     * and removes all {@link GATTCharacteristicListener} from the listener list,
     * which are associated with this characteristic instance.
     * <p>
     * Implementation tests all listener's {@link GATTCharacteristicListener#getAssociatedCharacteristic()}
     * to match with this characteristic instance.
     * </p>
     * <p>
     * If the DBTDevice's GATTHandler is null, i.e. not connected, {@code false} is being returned.
     * </p>
     *
     * @param disableIndicationNotification if true, disables the notification and/or indication for this characteristic
     * using {@link #configNotificationIndication(boolean, boolean, boolean[])}
     * @return number of removed listener.
     * @see #configNotificationIndication(boolean, boolean, boolean[])
     * @see BluetoothDevice#removeAllAssociatedCharacteristicListener(BluetoothGattCharacteristic)
     * @since 2.0.0
     * @implNote not implemented in tinyb.dbus
     */
    public int removeAllAssociatedCharacteristicListener(final boolean disableIndicationNotification);

    /**
     * Sets the given value BluetoothNotification to have its run function
     * receive the enabled notification and/or indication sent.
     * <p>
     * Enables notification and/or indication for this characteristic at BLE level.
     * </p.
     * @param callback A BluetoothNotification<byte[]> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the value
     * property.
     */
    public void enableValueNotifications(BluetoothNotification<byte[]> callback);

    /**
     * Disables notifications of the value and unregisters the callback object
     * passed through the corresponding enable method. It disables notifcations
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
