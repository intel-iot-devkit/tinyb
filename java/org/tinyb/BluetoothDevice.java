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
 *
 */
package org.tinyb;

import java.util.List;
import java.util.Map;

/**
  * Provides access to Bluetooth adapters. Follows the BlueZ adapter API
  * available at: http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/device-api.txt
  */
public interface BluetoothDevice extends BluetoothObject
{
    @Override
    public BluetoothDevice clone();

    /** Find a BluetoothGattService. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattService you are
      * waiting for
      * @parameter timeoutMS the function will return after timeout time in milliseconds, a
      * value of zero means wait forever. If object is not found during this time null will be returned.
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    BluetoothGattService find(String UUID, long timeoutMS);

    /** Find a BluetoothGattService. If parameter UUID is not null,
      * the returned object will have to match it.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter UUID optionally specify the UUID of the BluetoothGattService you are
      * waiting for
      * @return An object matching the UUID or null if not found before
      * timeout expires or event is canceled.
      */
    BluetoothGattService find(String UUID);

    /* D-Bus method calls: */
    /** The connection to this device is removed, removing all connected
      * profiles.
      * @return TRUE if the device disconnected
      */
    boolean disconnect() throws BluetoothException;

    /** A connection to this device is established, connecting each profile
      * flagged as auto-connectable.
      * @return TRUE if the device connected
      * @see #connect(short, short, short, short, short, short)
      */
    boolean connect() throws BluetoothException;

    /**
     * A connection to this device is established, see {@link #connect()}.
     * <p>
     * The given LE connection parameter will be used instead of the Bluetooth implementation defaults,
     * if this device is of type {@link BluetoothAddressType#BDADDR_LE_PUBLIC} or {@link BluetoothAddressType#BDADDR_LE_RANDOM}.
     * </p>
     * <p>
     * Set window to the same value as the interval, enables continuous scanning.
     * </p>
     *
     * @param interval default value 0x0004
     * @param window default value 0x0004
     * @param min_interval default value 0x000F
     * @param max_interval default value 0x000F
     * @param latency default value 0x0000
     * @param timeout default value 0x0C80
     * @return {@code true} if successful, otherwise {@code false}.
     *
     * @see #connect()
     * @since 2.0.0
     */
    public boolean connect(final short interval, final short window,
                           final short min_interval, final short max_interval,
                           final short latency, final short timeout);


    /** Connects a specific profile available on the device, given by UUID
      * @param arg_UUID The UUID of the profile to be connected
      * @return TRUE if the profile connected successfully
      */
    boolean connectProfile(String arg_UUID) throws BluetoothException;

    /** Disconnects a specific profile available on the device, given by UUID
      * @param arg_UUID The UUID of the profile to be disconnected
      * @return TRUE if the profile disconnected successfully
      */
    boolean disconnectProfile(String arg_UUID) throws BluetoothException;

    /** A connection to this device is established, and the device is then
      * paired.
      * @return TRUE if the device connected and paired
      */
    boolean pair() throws BluetoothException;

    /** Remove this device from the system (like an unpair).
      * @return TRUE if the device has been removed
      * @throws BluetoothException
      */
    boolean remove() throws BluetoothException;

    /** Cancels an initiated pairing operation
      * @return TRUE if the paring is cancelled successfully
      */
    boolean cancelPairing() throws BluetoothException;

    /** Returns a list of BluetoothGattServices available on this device.
      * @return A list of BluetoothGattServices available on this device,
      * NULL if an error occurred
      */
    List<BluetoothGattService> getServices();

    /* D-Bus property accessors: */
    /** Returns the hardware address of this device.
      * @return The hardware address of this device.
      */
    String getAddress();

    /** Returns the remote friendly name of this device.
      * @return The remote friendly name of this device, or NULL if not set.
      */
    String getName();

    /** Returns an alternative friendly name of this device.
      * @return The alternative friendly name of this device, or NULL if not set.
      */
    String getAlias();

    /** Sets an alternative friendly name of this device.
      */
    void setAlias(String value);

    /** Returns the Bluetooth class of the device.
      * @return The Bluetooth class of the device.
      */
    int getBluetoothClass();

    /** Returns the appearance of the device, as found by GAP service.
      * @return The appearance of the device, as found by GAP service.
      */
    short getAppearance();

    /** Returns the proposed icon name of the device.
      * @return The proposed icon name, or NULL if not set.
      */
    String getIcon();

    /** Returns the paired state the device.
      * @return The paired state of the device.
      */
    boolean getPaired();

    /**
     * Enables notifications for the paired property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the paired
     * property.
     */
    void enablePairedNotifications(BluetoothNotification<Boolean> callback);

    /**
     * Disables notifications of the paired property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    void disablePairedNotifications();

    /** Returns the trusted state the device.
      * @return The trusted state of the device.
      */
    boolean getTrusted();

    /**
     * Enables notifications for the trusted property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the trusted
     * property.
     */
    void enableTrustedNotifications(BluetoothNotification<Boolean> callback);

    /**
     * Disables notifications of the trusted property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    void disableTrustedNotifications();

    /** Sets the trusted state the device.
      */
    void setTrusted(boolean value);

    /** Returns the blocked state the device.
      * @return The blocked state of the device.
      */
    boolean getBlocked();

    /**
     * Enables notifications for the blocked property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the blocked
     * property.
     */
    void enableBlockedNotifications(BluetoothNotification<Boolean> callback);

    /**
     * Disables notifications of the blocked property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    void disableBlockedNotifications();

    /** Sets the blocked state the device.
      */
    void setBlocked(boolean value);

    /** Returns if device uses only pre-Bluetooth 2.1 pairing mechanism.
      * @return True if device uses only pre-Bluetooth 2.1 pairing mechanism.
      */
    boolean getLegacyPairing();

    /** Returns the Received Signal Strength Indicator of the device.
      * @return The Received Signal Strength Indicator of the device.
      */
    short getRSSI();

    /**
     * Enables notifications for the RSSI property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Short> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the RSSI
     * property.
     */
    void enableRSSINotifications(BluetoothNotification<Short> callback);

    /**
     * Disables notifications of the RSSI property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    void disableRSSINotifications();

    /** Returns the connected state of the device.
      * @return The connected state of the device.
      */
    boolean getConnected();

    /**
     * Enables notifications for the connected property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the connected
     * property.
     */
    void enableConnectedNotifications(BluetoothNotification<Boolean> callback);

    /**
     * Disables notifications of the connected property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    void disableConnectedNotifications();

    /** Returns the UUIDs of the device.
      * @return Array containing the UUIDs of the device, ends with NULL.
      */
    String[] getUUIDs();

    /** Returns the local ID of the adapter.
      * @return The local ID of the adapter.
      */
    String getModalias();

    /** Returns the adapter on which this device was discovered or
      * connected.
      * @return The adapter.
      */
    BluetoothAdapter getAdapter();

    /** Returns a map containing manufacturer specific advertisement data.
      * An entry has a uint16_t key and an array of bytes.
      * @return manufacturer specific advertisement data.
      */
    Map<Short, byte[]> getManufacturerData();

    /**
     * Enables notifications for the manufacturer data property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Map<Short, byte[]> > object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the manufacturer data
     * property.
     */
    void enableManufacturerDataNotifications(
            BluetoothNotification<Map<Short, byte[]>> callback);

    /**
     * Disables notifications of the manufacturer data property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    void disableManufacturerDataNotifications();

    /** Returns a map containing service advertisement data.
      * An entry has a UUID string key and an array of bytes.
      * @return service advertisement data.
      */
    Map<String, byte[]> getServiceData();

    /**
     * Enables notifications for the service data property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Map<String, byte[]> > object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the service data
     * property.
     */
    void enableServiceDataNotifications(
            BluetoothNotification<Map<String, byte[]>> callback);

    /**
     * Disables notifications of the service data property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    void disableServiceDataNotifications();

    /** Returns the transmission power level (0 means unknown).
      * @return the transmission power level (0 means unknown).
      */
    short getTxPower();

    /** Returns true if service discovery has ended.
      * @return true if the service discovery has ended.
      */
    boolean getServicesResolved();

    /**
     * Enables notifications for the services resolved property and calls run function of the
     * BluetoothNotification object.
     * @param callback A BluetoothNotification<Boolean> object. Its run function will be called
     * when a notification is issued. The run function will deliver the new value of the services resolved
     * property.
     */
    void enableServicesResolvedNotifications(
            BluetoothNotification<Boolean> callback);

    /**
     * Disables notifications of the services resolved property and unregisters the callback
     * object passed through the corresponding enable method.
     */
    void disableServicesResolvedNotifications();

    /**
     * Add the given {@link GATTCharacteristicListener} to the listener list if not already present.
     * @param listener A {@link GATTCharacteristicListener} instance, listening to all {@link BluetoothGattCharacteristic} events of this device
     * @param characteristicMatch Optional {@link BluetoothGattCharacteristic} to be matched before calling any
     *        {@link GATTCharacteristicListener} methods. Pass {@code null} for no filtering.
     * @return true if the given listener is not element of the list and has been newly added, otherwise false.
     * @since 2.0.0
     */
    public boolean addCharacteristicListener(final GATTCharacteristicListener listener, final BluetoothGattCharacteristic characteristicMatch);

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
}
