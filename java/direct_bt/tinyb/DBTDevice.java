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

import java.lang.ref.WeakReference;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

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
import org.tinyb.HCIStatusCode;

public class DBTDevice extends DBTObject implements BluetoothDevice
{
    private static final boolean DEBUG = DBTManager.DEBUG;

    /** Device's adapter weak back-reference */
    private final WeakReference<DBTAdapter> wbr_adapter;

    private final String address;
    private final BluetoothAddressType addressType;
    private final String name;
    private final long ts_creation;
    long ts_last_discovery;
    long ts_last_update;

    private final Object userCallbackLock = new Object();

    private final long blockedNotificationRef = 0;
    private BluetoothNotification<Boolean> userBlockedNotificationsCB = null;
    private final AtomicBoolean isBlocked = new AtomicBoolean(false);

    private final long pairedNotificationRef = 0;
    private BluetoothNotification<Boolean> userPairedNotificationsCB = null;
    private final AtomicBoolean isPaired = new AtomicBoolean(false);

    private final long trustedNotificationRef = 0;
    private BluetoothNotification<Boolean> userTrustedNotificationsCB = null;
    private final AtomicBoolean isTrusted = new AtomicBoolean(false);

    private BluetoothNotification<Boolean> userConnectedNotificationsCB = null;
    private final AtomicBoolean isConnected = new AtomicBoolean(false);

    private BluetoothNotification<Short> userRSSINotificationsCB = null;
    private BluetoothNotification<Map<Short, byte[]> > userManufDataNotificationsCB = null;
    private BluetoothNotification<Boolean> userServicesResolvedNotificationsCB = null;
    private final AtomicBoolean servicesResolved = new AtomicBoolean(false);
    private short appearance = 0;

    final AdapterStatusListener statusListener = new AdapterStatusListener() {
        @Override
        public void deviceUpdated(final BluetoothDevice device, final EIRDataTypeSet updateMask, final long timestamp) {
            final boolean rssiUpdated = updateMask.isSet( EIRDataTypeSet.DataType.RSSI );
            final boolean mdUpdated = updateMask.isSet( EIRDataTypeSet.DataType.MANUF_DATA );
            if( rssiUpdated || mdUpdated ) {
                synchronized(userCallbackLock) {
                    if( rssiUpdated && null != userRSSINotificationsCB ) {
                        userRSSINotificationsCB.run(getRSSI());
                    }
                    if( mdUpdated && null != userManufDataNotificationsCB ) {
                        userManufDataNotificationsCB.run(getManufacturerData());
                    }
                }
            }
        }
        @Override
        public void deviceConnected(final BluetoothDevice device, final long timestamp) {
            if( isConnected.compareAndSet(false, true) ) {
                synchronized(userCallbackLock) {
                    if( null != userConnectedNotificationsCB ) {
                        userConnectedNotificationsCB.run(Boolean.TRUE);
                    }
                    if( servicesResolved.compareAndSet(false, true) ) {
                        if( null != userServicesResolvedNotificationsCB ) {
                            userServicesResolvedNotificationsCB.run(Boolean.TRUE);
                        }
                    }
                }
            }
        }
        @Override
        public void deviceDisconnected(final BluetoothDevice device, final HCIStatusCode reason, final long timestamp) {
            if( isConnected.compareAndSet(true, false) ) {
                synchronized(userCallbackLock) {
                    if( servicesResolved.compareAndSet(true, false) ) {
                        if( null != userServicesResolvedNotificationsCB ) {
                            userServicesResolvedNotificationsCB.run(Boolean.FALSE);
                        }
                    }
                    if( null != userConnectedNotificationsCB ) {
                        userConnectedNotificationsCB.run(Boolean.FALSE);
                    }
                }
            }
        }
    };

    final private BluetoothNotification<Boolean> blockedNotificationsCB = new BluetoothNotification<Boolean>() {
        @Override
        public void run(final Boolean value) {
            if( DEBUG ) {
                System.err.println("Device.BlockedNotification: "+isBlocked+" -> "+value+" on "+DBTDevice.this.toString());
            }
            final boolean _isBlocked = value.booleanValue();
            if( isBlocked.compareAndSet(!_isBlocked, _isBlocked) ) {
                synchronized(userCallbackLock) {
                    if( null != userBlockedNotificationsCB ) {
                        userBlockedNotificationsCB.run(value);
                    }
                }
            }
        }
    };

    final private BluetoothNotification<Boolean> pairedNotificationsCB = new BluetoothNotification<Boolean>() {
        @Override
        public void run(final Boolean value) {
            if( DEBUG ) {
                System.err.println("Device.PairedNotification: "+isPaired+" -> "+value+" on "+DBTDevice.this.toString());
            }
            final boolean _isPaired = value.booleanValue();
            if( isPaired.compareAndSet(!_isPaired, _isPaired) ) {
                synchronized(userCallbackLock) {
                    if( null != userPairedNotificationsCB ) {
                        userPairedNotificationsCB.run(value);
                    }
                }
            }
        }
    };

    final private BluetoothNotification<Boolean> trustedNotificationsCB = new BluetoothNotification<Boolean>() {
        @Override
        public void run(final Boolean value) {
            if( DEBUG ) {
                System.err.println("Device.TrustedNotification: "+isTrusted+" -> "+value+" on "+DBTDevice.this.toString());
            }
            final boolean _isTrusted = value.booleanValue();
            if( isTrusted.compareAndSet(!_isTrusted, _isTrusted) ) {
                synchronized(userCallbackLock) {
                    if( null != userTrustedNotificationsCB ) {
                        userTrustedNotificationsCB.run(value);
                    }
                }
            }
        }
    };

    /* pp */ DBTDevice(final long nativeInstance, final DBTAdapter adptr,
                       final String address, final int intAddressType,
                       final String name, final long ts_creation)
    {
        super(nativeInstance, compHash(address, name));
        this.wbr_adapter = new WeakReference<DBTAdapter>(adptr);
        this.address = address;
        this.addressType = BluetoothAddressType.get(intAddressType);
        this.name = name;
        this.ts_creation = ts_creation;
        ts_last_discovery = ts_creation;
        ts_last_update = ts_creation;
        appearance = 0;
        initImpl();
        adptr.addStatusListener(statusListener, this); // only for this device
        enableBlockedNotificationsImpl(blockedNotificationsCB);
        enablePairedNotificationsImpl(pairedNotificationsCB);
        // FIXME enableTrustedNotificationsImpl(trustedNotificationsCB);
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
        disableBlockedNotificationsImpl();
        disablePairedNotifications();
        disablePairedNotificationsImpl();
        disableServiceDataNotifications();
        disableTrustedNotifications();
        // FIXME disableTrustedNotificationsImpl();

        final DBTAdapter a = getAdapter();
        if( null != a ) {
            a.removeStatusListener(statusListener);
            a.removeDiscoveredDevice(this);
        }
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
    public final long getLastDiscoveryTimestamp() { return ts_last_discovery; }

    @Override
    public final long getLastUpdateTimestamp() { return ts_last_update; }

    @Override
    public DBTAdapter getAdapter() { return wbr_adapter.get(); }

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
    public final boolean getConnected() { return isConnected.get(); }

    @Override
    public final boolean disconnect() throws BluetoothException {
        if( isConnected.get() ) {
            return disconnectImpl(); // event callbacks will be generated by implementation
        }
        return false;
    }
    private native boolean disconnectImpl() throws BluetoothException;

    @Override
    public final boolean connect() throws BluetoothException {
        if( !isConnected.get() ) {
            return connectImpl(); // event callbacks will be generated by implementation
        }
        return false;
    }
    private native boolean connectImpl() throws BluetoothException;

    @Override
    public boolean connect(final short le_scan_interval, final short le_scan_window,
                           final short conn_interval_min, final short conn_interval_max,
                           final short conn_latency, final short timeout) {
        if( !isConnected.get() ) {
            // event callbacks will be generated by implementation
            return connectImpl(le_scan_interval, le_scan_window, conn_interval_min, conn_interval_max, conn_latency, timeout);
        }
        return false;
    }
    private native boolean connectImpl(final short le_scan_interval, final short le_scan_window,
                                       final short conn_interval_min, final short conn_interval_max,
                                       final short conn_latency, final short timeout);

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
    public boolean getServicesResolved () { return servicesResolved.get(); }

    @Override
    public short getAppearance() { return appearance; }

    @Override
    public void enableBlockedNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userBlockedNotificationsCB = callback;
        }
    }
    @Override
    public void disableBlockedNotifications() {
        synchronized(userCallbackLock) {
            userBlockedNotificationsCB = null;
        }
    }
    @Override
    public boolean getBlocked() { return isBlocked.get(); }

    @Override
    public void setBlocked(final boolean value) {
        setBlockedImpl(value);
    }

    @Override
    public void enableServiceDataNotifications(final BluetoothNotification<Map<String, byte[]> > callback) {
        // FIXME: Isn't this GATTCharacteristic data notification/indication? Then map it or drop!
    }

    @Override
    public Map<String, byte[]> getServiceData() {
        return null; // FIXME
    }

    @Override
    public void disableServiceDataNotifications() {
        // FIXME
    }

    @Override
    public void enablePairedNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userPairedNotificationsCB = callback;
        }
    }

    @Override
    public void disablePairedNotifications() {
        synchronized(userCallbackLock) {
            userPairedNotificationsCB = null;
        }
    }

    @Override
    public boolean getPaired() { return isPaired.get(); }

    @Override
    public void enableTrustedNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userTrustedNotificationsCB = callback;
        }
    }

    @Override
    public void disableTrustedNotifications() {
        synchronized(userCallbackLock) {
            userTrustedNotificationsCB = null;
        }
    }

    @Override
    public boolean getTrusted() { return isTrusted.get(); }

    @Override
    public void setTrusted(final boolean value) {
        setTrustedImpl(value);
    }

    @Override
    public native boolean getLegacyPairing();

    @Override
    public final String toString() {
        if( !isValid() ) {
            // Device✝[0x112233445566, 'adevice']
            return "Device" + "\u271D" + "["+address+", '"+name+"']";
        }
        return toStringImpl();
    }

    /* DBT native callbacks */

    private native String toStringImpl();

    private native void enableBlockedNotificationsImpl(BluetoothNotification<Boolean> callback);
    private native void disableBlockedNotificationsImpl();
    private native void setBlockedImpl(final boolean value);

    // FIXME: Figure out paired:=true, as currently we only attach to unpaired
    private native void enablePairedNotificationsImpl(BluetoothNotification<Boolean> callback);
    private native void disablePairedNotificationsImpl();

    /**
     * FIXME: How to implement trusted ?
     *
    private native void enableTrustedNotificationsImpl(BluetoothNotification<Boolean> callback);
    private native void disableTrustedNotificationsImpl();
     */
    private native void setTrustedImpl(boolean value);

    /* DBT native method calls: */

    @Override
    public native boolean connectProfile(String arg_UUID) throws BluetoothException;

    @Override
    public native boolean disconnectProfile(String arg_UUID) throws BluetoothException;

    /**
     * {@inheritDoc}
     * <p>
     * Disconnects this device via disconnect(..) and
     * removes its shared reference from the Adapter altogether.
     * </p>
     * <p>
     * This method shall be issued to ensure no device reference will
     * be leaked in a long lived adapter,
     * as only the connected-devices are removed at disconnect
     * and the discovered-devices removed with a new discovery.
     * </p>
     * <p>
     * After calling this method, the device shall no more being used.
     * </p>
     * <p>
     * This method is automatically called native @ destructor {@link #deleteImpl(long)}.
     * </p>
     */
    @Override
    public final boolean remove() throws BluetoothException {
        close(); // -> super.close() -> delete() -> deleteImpl() -> DBTDevice::remove()
        // return removeImpl();
        return true;
    }
    private native boolean removeImpl() throws BluetoothException;

    @Override
    public List<BluetoothGattService> getServices() {
        try {
            return getServicesImpl();
        } catch (final Throwable t) {
            if(DEBUG) {
                System.err.println("Caught "+t.getMessage()+" on thread "+Thread.currentThread().toString());
                t.printStackTrace();
            }
        }
        return null;
    }
    private native List<BluetoothGattService> getServicesImpl();

    @Override
    public boolean pingGATT() {
        try {
            return pingGATTImpl();
        } catch (final Throwable t) {
            if(DEBUG) {
                System.err.println("Caught "+t.getMessage()+" on thread "+Thread.currentThread().toString());
                t.printStackTrace();
            }
        }
        return false;
    }
    private native boolean pingGATTImpl();

    /* property accessors: */

    @Override
    public native String getIcon();

    @Override
    public native short getRSSI();

    @Override
    public native String[] getUUIDs();

    @Override
    public native String getModalias();

    @Override
    public native Map<Short, byte[]> getManufacturerData();

    @Override
    public native short getTxPower ();

    /**
     * {@inheritDoc}
     * <p>
     * Native implementation calls DBTDevice::remove()
     * </p>
     */
    @Override
    protected native void deleteImpl();

    @Override
    public native boolean addCharacteristicListener(final GATTCharacteristicListener listener, final BluetoothGattCharacteristic characteristicMatch);

    @Override
    public native boolean removeCharacteristicListener(final GATTCharacteristicListener l);

    @Override
    public native int removeAllCharacteristicListener();
}
