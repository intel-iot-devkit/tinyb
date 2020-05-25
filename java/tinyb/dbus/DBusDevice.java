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

package tinyb.dbus;

import java.util.List;
import java.util.Map;

import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothType;
import org.tinyb.GATTCharacteristicListener;

public class DBusDevice extends DBusObject implements BluetoothDevice
{
    @Override
    public native BluetoothType getBluetoothType();
    @Override
    public native DBusDevice clone();

    static BluetoothType class_type() { return BluetoothType.DEVICE; }

    @Override
    public BluetoothGattService find(final String UUID, final long timeoutMS) {
        final BluetoothManager manager = DBusManager.getBluetoothManager();
        return (BluetoothGattService) manager.find(BluetoothType.GATT_SERVICE,
                null, UUID, this, timeoutMS);
    }

    @Override
    public BluetoothGattService find(final String UUID) {
        return find(UUID, 0);
    }

    /* D-Bus method calls: */

    @Override
    public native boolean disconnect() throws BluetoothException;

    @Override
    public native boolean connect() throws BluetoothException;

    @Override
    public boolean connect(final short interval, final short window,
                           final short min_interval, final short max_interval,
                           final short latency, final short timeout) {
        return connect(); // FIXME connection params ...
    }

    @Override
    public native boolean connectProfile(String arg_UUID) throws BluetoothException;

    @Override
    public native boolean disconnectProfile(String arg_UUID) throws BluetoothException;

    @Override
    public native boolean pair() throws BluetoothException;

     @Override
    public native boolean remove() throws BluetoothException;

    @Override
    public native boolean cancelPairing() throws BluetoothException;

    @Override
    public native List<BluetoothGattService> getServices();

    /* D-Bus property accessors: */

    @Override
    public native String getAddress();

    @Override
    public native String getName();

    @Override
    public native String getAlias();

    @Override
    public native void setAlias(String value);

    @Override
    public native int getBluetoothClass();

    @Override
    public native short getAppearance();

    @Override
    public native String getIcon();

    @Override
    public native boolean getPaired();

    @Override
    public native void enablePairedNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public native void disablePairedNotifications();

    @Override
    public native boolean getTrusted();

    @Override
    public native void enableTrustedNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public native void disableTrustedNotifications();

    @Override
    public native void setTrusted(boolean value);

    @Override
    public native boolean getBlocked();

    @Override
    public native void enableBlockedNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public native void disableBlockedNotifications();

    @Override
    public native void setBlocked(boolean value);

    @Override
    public native boolean getLegacyPairing();

    @Override
    public native short getRSSI();

    @Override
    public native void enableRSSINotifications(BluetoothNotification<Short> callback);

    @Override
    public native void disableRSSINotifications();

    @Override
    public native boolean getConnected();

    @Override
    public native void enableConnectedNotifications(BluetoothNotification<Boolean> callback);

     @Override
    public native void disableConnectedNotifications();

    @Override
    public native String[] getUUIDs();

    @Override
    public native String getModalias();

    @Override
    public native DBusAdapter getAdapter();

    @Override
    public native Map<Short, byte[]> getManufacturerData();

    @Override
    public native void enableManufacturerDataNotifications(BluetoothNotification<Map<Short, byte[]> > callback);

    @Override
    public native void disableManufacturerDataNotifications();


    @Override
    public native Map<String, byte[]> getServiceData();

    @Override
    public native void enableServiceDataNotifications(BluetoothNotification<Map<String, byte[]> > callback);

    @Override
    public native void disableServiceDataNotifications();

    @Override
    public native short getTxPower ();

    @Override
    public native boolean getServicesResolved ();

    @Override
    public native void enableServicesResolvedNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public native void disableServicesResolvedNotifications();

    @Override
    public boolean addCharacteristicListener(final GATTCharacteristicListener listener, final BluetoothGattCharacteristic characteristicMatch) {
        return false; // FIXME
    }

    @Override
    public boolean removeCharacteristicListener(final GATTCharacteristicListener l) {
        return false; // FIXME
    }

    @Override
    public int removeAllCharacteristicListener() {
        return 0; // FIXME
    }

    private native void delete();

    private DBusDevice(final long instance)
    {
        super(instance);
    }

}
