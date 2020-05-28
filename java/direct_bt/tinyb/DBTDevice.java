/**
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
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

package direct_bt.tinyb;

import java.util.List;
import java.util.Map;

import org.tinyb.AdapterStatusListener;
import org.tinyb.BluetoothAddressType;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothType;
import org.tinyb.EIRDataTypeSet;
import org.tinyb.GATTCharacteristicListener;

public class DBTDevice extends DBTObject implements BluetoothDevice
{
    private final DBTAdapter adapter;
    private final String address;
    private final BluetoothAddressType addressType;
    private final String name;
    private final long ts_creation;
    long ts_update;

    private final long connectedNotificationRef = 0;
    private boolean connected = false;
    private final Object userCallbackLock = new Object();
    private BluetoothNotification<Boolean> userConnectedNotificationsCB = null;
    private BluetoothNotification<Short> userRSSINotificationsCB = null;
    private BluetoothNotification<Map<Short, byte[]> > userManufDataNotificationsCB = null;
    private BluetoothNotification<Boolean> userServicesResolvedNotificationsCB = null;
    private boolean servicesResolved = false;
    private short appearance = 0;

    final AdapterStatusListener statusListener = new AdapterStatusListener() {
        @Override
        public void deviceUpdated(final BluetoothDevice device, final long timestamp, final EIRDataTypeSet updateMask) {
            synchronized(userCallbackLock) {
                if( updateMask.isSet( EIRDataTypeSet.DataType.RSSI ) && null != userRSSINotificationsCB ) {
                    userRSSINotificationsCB.run(getRSSI());
                }
                if( updateMask.isSet( EIRDataTypeSet.DataType.MANUF_DATA ) && null != userManufDataNotificationsCB ) {
                    userManufDataNotificationsCB.run(getManufacturerData());
                }
            }
        }
        @Override
        public void deviceConnectionChanged(final BluetoothDevice device, final boolean connected, final long timestamp) {
            DBTDevice.this.connected = connected;
            synchronized(userCallbackLock) {
                if( null != userConnectedNotificationsCB ) {
                    userConnectedNotificationsCB.run(connected);
                }
            }
        }
    };


    /* pp */ DBTDevice(final long nativeInstance, final DBTAdapter adptr,
                       final String address, final int intAddressType,
                       final String name, final long ts_creation)
    {
        super(nativeInstance, compHash(address, name));
        this.adapter = adptr;
        this.address = address;
        this.addressType = BluetoothAddressType.get(intAddressType);
        this.name = name;
        this.ts_creation = ts_creation;
        ts_update = ts_creation;
        appearance = 0;
        initImpl();
        this.adapter.addStatusListener(statusListener, this); // only for this device
    }

    @Override
    public synchronized void close() {
        if( !isValid() ) {
            return;
        }
        disconnect();

        disableConnectedNotifications();
        disableRSSINotifications();
        disableManufacturerDataNotifications();
        disableServicesResolvedNotifications();

        disableBlockedNotifications();
        disablePairedNotifications();
        disableServiceDataNotifications();
        disableTrustedNotifications();

        adapter.removeStatusListener(statusListener);
        super.close();
    }

    @Override
    public boolean equals(final Object obj)
    {
        if (obj == null || !(obj instanceof DBTDevice)) {
            return false;
        }
        final DBTDevice other = (DBTDevice)obj;
        return address.equals(other.address) && name.equals(other.name);
    }

    @Override
    public final long getCreationTimestamp() { return ts_creation; }

    @Override
    public DBTAdapter getAdapter() {
        return adapter;
    }

    @Override
    public String getAddress() { return address; }

    @Override
    public BluetoothAddressType getAddressType() { return addressType; }

    @Override
    public String getName() { return name; }

    @Override
    public BluetoothType getBluetoothType() { return class_type(); }

    static BluetoothType class_type() { return BluetoothType.DEVICE; }

    @Override
    public BluetoothGattService find(final String UUID, final long timeoutMS) {
        final BluetoothManager manager = DBTManager.getBluetoothManager();
        return (BluetoothGattService) manager.find(BluetoothType.GATT_SERVICE,
                null, UUID, this, timeoutMS);
    }

    @Override
    public BluetoothGattService find(final String UUID) {
        return find(UUID, 0);
    }

    /* Unsupported */

    @Override
    public int getBluetoothClass() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public final BluetoothDevice clone() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public boolean pair() throws BluetoothException { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public boolean cancelPairing() throws BluetoothException { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public String getAlias() { return null; } // FIXME

    @Override
    public void setAlias(final String value) { throw new UnsupportedOperationException(); } // FIXME

    /* internal */

    private native void initImpl();

    /* DBT method calls: Connection */

    @Override
    public final void enableConnectedNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userConnectedNotificationsCB = callback;
        }
    }
    @Override
    public final void disableConnectedNotifications() {
        synchronized(userCallbackLock) {
            userConnectedNotificationsCB = null;
        }
    }

    @Override
    public final boolean getConnected() { return connected; }

    @Override
    public final boolean disconnect() throws BluetoothException {
        boolean res = false;
        if( connected ) {
            res = disconnectImpl();
            if( res ) {
                // FIXME: Split up - may offload to other thread
                // Currently service resolution performed in connectImpl()!
                servicesResolved = false;
                userServicesResolvedNotificationsCB.run(Boolean.FALSE);

                connected = false;
                userConnectedNotificationsCB.run(Boolean.FALSE);
            }
        }
        return res;
    }
    private native boolean disconnectImpl() throws BluetoothException;

    @Override
    public final boolean connect() throws BluetoothException {
        boolean res = false;
        if( !connected ) {
            res = connectImpl();
            if( res ) {
                connected = true;
                userConnectedNotificationsCB.run(Boolean.TRUE);

                // FIXME: Split up - may offload to other thread
                // Currently service resolution performed in connectImpl()!
                servicesResolved = true;
                userServicesResolvedNotificationsCB.run(Boolean.TRUE);
            }
        }
        return res;
    }
    private native boolean connectImpl() throws BluetoothException;

    @Override
    public boolean connect(final short interval, final short window,
                           final short min_interval, final short max_interval,
                           final short latency, final short timeout) {
        boolean res = false;
        if( !connected ) {
            res = connectImpl(interval, window, min_interval, max_interval, latency, timeout);
            if( res ) {
                connected = true;
                userConnectedNotificationsCB.run(Boolean.TRUE);

                // FIXME: Split up - may offload to other thread
                // Currently service resolution performed in connectImpl()!
                servicesResolved = true;
                userServicesResolvedNotificationsCB.run(Boolean.TRUE);
            }
        }
        return res;
    }
    private native boolean connectImpl(final short interval, final short window,
                                       final short min_interval, final short max_interval,
                                       final short latency, final short timeout);

    /* DBT Java callbacks */

    @Override
    public final void enableRSSINotifications(final BluetoothNotification<Short> callback) {
        synchronized(userCallbackLock) {
            userRSSINotificationsCB = callback;
        }
    }

    @Override
    public final void disableRSSINotifications() {
        synchronized(userCallbackLock) {
            userRSSINotificationsCB = null;
        }
    }


    @Override
    public final void enableManufacturerDataNotifications(final BluetoothNotification<Map<Short, byte[]> > callback) {
        synchronized(userCallbackLock) {
            userManufDataNotificationsCB = callback;
        }
    }

    @Override
    public final void disableManufacturerDataNotifications() {
        synchronized(userCallbackLock) {
            userManufDataNotificationsCB = null;
        }
    }


    @Override
    public final void enableServicesResolvedNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userServicesResolvedNotificationsCB = callback;
        }
    }

    @Override
    public void disableServicesResolvedNotifications() {
        synchronized(userCallbackLock) {
            userServicesResolvedNotificationsCB = null;
        }
    }

    @Override
    public boolean getServicesResolved () { return servicesResolved; }

    @Override
    public short getAppearance() { return appearance; }

    @Override
    public final String toString() { return toStringImpl(); }

    /* DBT native callbacks */

    private native String toStringImpl();

    @Override
    public native void enableBlockedNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public void disableBlockedNotifications() { } // FIXME

    @Override
    public native void enableServiceDataNotifications(BluetoothNotification<Map<String, byte[]> > callback);

    @Override
    public void disableServiceDataNotifications() { } // FIXME

    @Override
    public native void enablePairedNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public void disablePairedNotifications() { } // FIXME

    @Override
    public native void enableTrustedNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public void disableTrustedNotifications() { } // FIXME


    /* DBT native method calls: */

    @Override
    public native boolean connectProfile(String arg_UUID) throws BluetoothException;

    @Override
    public native boolean disconnectProfile(String arg_UUID) throws BluetoothException;

     @Override
    public native boolean remove() throws BluetoothException;

    @Override
    public native List<BluetoothGattService> getServices();

    /* property accessors: */

    @Override
    public native String getIcon();

    @Override
    public native boolean getPaired();

    @Override
    public native boolean getTrusted();

    @Override
    public native void setTrusted(boolean value);

    @Override
    public native boolean getBlocked();

    @Override
    public native void setBlocked(boolean value);

    @Override
    public native boolean getLegacyPairing();

    @Override
    public native short getRSSI();

    @Override
    public native String[] getUUIDs();

    @Override
    public native String getModalias();

    @Override
    public native Map<Short, byte[]> getManufacturerData();

    @Override
    public native Map<String, byte[]> getServiceData();

    @Override
    public native short getTxPower ();

    @Override
    protected native void deleteImpl();

    @Override
    public native boolean addCharacteristicListener(final GATTCharacteristicListener listener, final BluetoothGattCharacteristic characteristicMatch);

    @Override
    public native boolean removeCharacteristicListener(final GATTCharacteristicListener l);

    @Override
    public native int removeAllCharacteristicListener();
}
