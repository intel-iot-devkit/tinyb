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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import org.tinyb.AdapterSettings;
import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothAddressType;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattDescriptor;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothObject;
import org.tinyb.BluetoothType;
import org.tinyb.EIRDataTypeSet;
import org.tinyb.HCIStatusCode;
import org.tinyb.HCIWhitelistConnectType;
import org.tinyb.AdapterStatusListener;
import org.tinyb.TransportType;

public class DBTAdapter extends DBTObject implements BluetoothAdapter
{
    private static final boolean DEBUG = DBTManager.DEBUG;

    private static AtomicInteger globThreadID = new AtomicInteger(0);
    private static int discoverTimeoutMS = 100;

    private final String address;
    private final String name;

    private final Object discoveryLock = new Object();
    private final Object discoveredDevicesLock = new Object();
    private final Object userCallbackLock = new Object();

    private BluetoothNotification<Boolean> userDiscoverableNotificationCB = null;
    private final AtomicBoolean isDiscoverable = new AtomicBoolean(false);
    private BluetoothNotification<Boolean> userDiscoveringNotificationCB = null;
    private final AtomicBoolean isDiscovering = new AtomicBoolean(false);

    private BluetoothNotification<Boolean> userPoweredNotificationCB = null;
    private final AtomicBoolean isPowered = new AtomicBoolean(false);
    private BluetoothNotification<Boolean> userPairableNotificationCB = null;
    private final AtomicBoolean isPairable = new AtomicBoolean(false);

    private final List<BluetoothDevice> discoveredDevices = new ArrayList<BluetoothDevice>();

    /* pp */ DBTAdapter(final long nativeInstance, final String address, final String name)
    {
        super(nativeInstance, compHash(address, name));
        this.address = address;
        this.name = name;
        addStatusListener(this.statusListener, null);
    }

    @Override
    public synchronized void close() {
        if( !isValid() ) {
            return;
        }
        stopDiscovery();

        final List<BluetoothDevice> devices = getDevices();
        for(final Iterator<BluetoothDevice> id = devices.iterator(); id.hasNext(); ) {
            final BluetoothDevice d = id.next();
            d.close();
        }

        removeAllStatusListener();
        disableDiscoverableNotifications();
        disableDiscoveringNotifications();
        disablePairableNotifications();
        disablePoweredNotifications();

        removeDevicesImpl();
        discoveredDevices.clear();

        super.close();
    }

    @Override
    public boolean equals(final Object obj)
    {
        if (obj == null || !(obj instanceof DBTAdapter)) {
            return false;
        }
        final DBTAdapter other = (DBTAdapter)obj;
        return address.equals(other.address) && name.equals(other.name);
    }

    @Override
    public String getAddress() { return address; }

    @Override
    public String getName() { return name; }

    @Override
    public BluetoothType getBluetoothType() { return class_type(); }

    static BluetoothType class_type() { return BluetoothType.ADAPTER; }

    @Override
    public BluetoothDevice find(final String name, final String address, final long timeoutMS) {
        return (DBTDevice) findInCache(name, address, BluetoothType.DEVICE);
    }

    @Override
    public BluetoothDevice find(final String name, final String address) {
        return find(name, address, 0);
    }

    @Override
    public native boolean isDeviceWhitelisted(final String address);

    @Override
    public boolean addDeviceToWhitelist(final String address, final BluetoothAddressType address_type,
                                        final HCIWhitelistConnectType ctype,
                                        final short conn_interval_min, final short conn_interval_max,
                                        final short conn_latency, final short timeout) {
        return addDeviceToWhitelist(address, address_type.value, ctype.value,
                                    conn_interval_min, conn_interval_max, conn_latency, timeout);
    }
    private native boolean addDeviceToWhitelist(final String address, final int address_type, final int ctype,
                                        final short conn_interval_min, final short conn_interval_max,
                                        final short conn_latency, final short timeout);

    @Override
    public boolean addDeviceToWhitelist(final String address, final BluetoothAddressType address_type,
                                        final HCIWhitelistConnectType ctype) {
        return addDeviceToWhitelist(address, address_type.value, ctype.value);
    }
    private native boolean addDeviceToWhitelist(final String address, final int address_type, final int ctype);

    @Override
    public boolean removeDeviceFromWhitelist(final String address, final BluetoothAddressType address_type) {
        return removeDeviceFromWhitelist(address, address_type.value);
    }
    private native boolean removeDeviceFromWhitelist(final String address, final int address_type);

    /* Unsupported */

    @Override
    public long getBluetoothClass() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public final BluetoothAdapter clone() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public String getInterfaceName() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public long getDiscoverableTimeout() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public void setDiscoverableTimout(final long value) { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public long getPairableTimeout() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public void setPairableTimeout(final long value) { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public String getModalias() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public String[] getUUIDs() { throw new UnsupportedOperationException(); } // FIXME


    /* Java callbacks */

    @Override
    public boolean getPowered() { return isPowered.get(); }

    @Override
    public void enablePoweredNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userPoweredNotificationCB = callback;
        }
    }

    @Override
    public void disablePoweredNotifications() {
        synchronized(userCallbackLock) {
            userPoweredNotificationCB = null;
        }
    }

    @Override
    public boolean getDiscoverable() { return isDiscoverable.get(); }

    @Override
    public void enableDiscoverableNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userDiscoverableNotificationCB = callback;
        }
    }

    @Override
    public void disableDiscoverableNotifications() {
        synchronized(userCallbackLock) {
            userDiscoverableNotificationCB = null;
        }
    }

    @Override
    public boolean getDiscovering() {
        return isDiscovering.get();
    }

    @Override
    public void enableDiscoveringNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userDiscoveringNotificationCB = callback;
        }
    }

    @Override
    public void disableDiscoveringNotifications() {
        synchronized(userCallbackLock) {
            userDiscoveringNotificationCB = null;
        }
    }

    @Override
    public boolean getPairable() { return isPairable.get(); }

    @Override
    public void enablePairableNotifications(final BluetoothNotification<Boolean> callback) {
        synchronized(userCallbackLock) {
            userPairableNotificationCB = callback;
        }
    }

    @Override
    public void disablePairableNotifications() {
        synchronized(userCallbackLock) {
            userPairableNotificationCB = null;
        }
    }

    @Override
    public String toString() {
        if( !isValid() ) {
            return "Adapter" + "\u271D" + "["+address+", '"+name+"']";
        }
        return toStringImpl();
    }

    /* Native callbacks */

    /* Native functionality / properties */

    private native String toStringImpl();

    @Override
    public native void setPowered(boolean value);

    @Override
    public native String getAlias();

    @Override
    public native void setAlias(final String value);

    @Override
    public native void setDiscoverable(boolean value);

    @Override
    public native BluetoothDevice connectDevice(String address, String addressType);

    @Override
    public native void setPairable(boolean value);

    /* internal */

    @Override
    protected native void deleteImpl(long nativeInstance);

    /* discovery */

    @Override
    public boolean startDiscovery() throws BluetoothException {
        return startDiscovery(true);
    }

    @Override
    public boolean startDiscovery(final boolean keepAlive) throws BluetoothException {
        synchronized( discoveryLock ) {
            // Ignoring 'isDiscovering', as native implementation also handles change of 'keepAlive'.
            // The discoveredDevices shall always get cleared.
            removeDevices();
            return startDiscoveryImpl(keepAlive); // event callbacks will be generated by implementation
        }
    }
    private native boolean startDiscoveryImpl(boolean keepAlive) throws BluetoothException;

    @Override
    public boolean stopDiscovery() throws BluetoothException {
        synchronized( discoveryLock ) {
            if( isDiscovering.get() ) {
                return stopDiscoveryImpl();  // event callbacks will be generated by implementation
            }
            return true;
        }
    }
    private native boolean stopDiscoveryImpl() throws BluetoothException;

    @Override
    public List<BluetoothDevice> getDevices() {
        synchronized(discoveredDevicesLock) {
            return new ArrayList<BluetoothDevice>(discoveredDevices);
        }
    }
    // std::vector<std::shared_ptr<direct_bt::HCIDevice>> discoveredDevices = adapter.getDiscoveredDevices();
    private native List<BluetoothDevice> getDiscoveredDevicesImpl();

    @Override
    public int removeDevices() throws BluetoothException {
        final int cj = removeDiscoveredDevices();
        final int cn = removeDevicesImpl();
        if( cj != cn ) {
            if( DEBUG ) {
                System.err.println("DBTAdapter::removeDevices: Inconsistent discovered device count: Native "+cn+", callback "+cj);
            }
        }
        return cn;
    }
    private native int removeDevicesImpl() throws BluetoothException;
    private int removeDiscoveredDevices() {
        synchronized(discoveredDevicesLock) {
            final int n = discoveredDevices.size();
            discoveredDevices.clear();
            return n;
        }
    }
    /* pp */ boolean removeDiscoveredDevice(final BluetoothDevice device) {
        synchronized(discoveredDevicesLock) {
            return discoveredDevices.remove(device);
        }
    }

    @Override
    public native boolean addStatusListener(final AdapterStatusListener l, final BluetoothDevice deviceMatch);

    @Override
    public native boolean removeStatusListener(final AdapterStatusListener l);

    @Override
    public native int removeAllStatusListener();

    @Override
    public void setDiscoveryFilter(final List<UUID> uuids, final int rssi, final int pathloss, final TransportType transportType) {
        final List<String> uuidsFmt = new ArrayList<>(uuids.size());
        for (final UUID uuid : uuids) {
            uuidsFmt.add(uuid.toString());
        }
        setDiscoveryFilter(uuidsFmt, rssi, pathloss, transportType.ordinal());
    }

    @SuppressWarnings("unchecked")
    public void setRssiDiscoveryFilter(final int rssi) {
        setDiscoveryFilter(Collections.EMPTY_LIST, rssi, 0, TransportType.AUTO);
    }

    private native void setDiscoveryFilter(List<String> uuids, int rssi, int pathloss, int transportType);

    ////////////////////////////////////

    private final AdapterStatusListener statusListener = new AdapterStatusListener() {
        @Override
        public void adapterSettingsChanged(final BluetoothAdapter a, final AdapterSettings oldmask, final AdapterSettings newmask,
                                           final AdapterSettings changedmask, final long timestamp) {
            if( DEBUG ) {
                System.err.println("Adapter.StatusListener.SETTINGS_CHANGED: "+oldmask+" -> "+newmask+", changed "+changedmask+" on "+a);
            }
            {
                if( changedmask.isSet(AdapterSettings.SettingType.POWERED) ) {
                    final boolean _isPowered = newmask.isSet(AdapterSettings.SettingType.POWERED);
                    if( isPowered.compareAndSet(!_isPowered, _isPowered) ) {
                        synchronized(userCallbackLock) {
                            if( null != userPoweredNotificationCB ) {
                                userPoweredNotificationCB.run(_isPowered);
                            }
                        }
                    }
                }
            }
            {
                if( changedmask.isSet(AdapterSettings.SettingType.DISCOVERABLE) ) {
                    final boolean _isDiscoverable = newmask.isSet(AdapterSettings.SettingType.DISCOVERABLE);
                    if( isDiscoverable.compareAndSet(!_isDiscoverable, _isDiscoverable) ) {
                        synchronized(userCallbackLock) {
                            if( null != userDiscoverableNotificationCB ) {
                                userDiscoverableNotificationCB.run( _isDiscoverable );
                            }
                        }
                    }
                }
            }
            {
                if( changedmask.isSet(AdapterSettings.SettingType.BONDABLE) ) {
                    final boolean _isPairable = newmask.isSet(AdapterSettings.SettingType.BONDABLE);
                    if( isPairable.compareAndSet(!_isPairable, _isPairable) ) {
                        synchronized(userCallbackLock) {
                            if( null != userPairableNotificationCB ) {
                                userPairableNotificationCB.run( _isPairable );
                            }
                        }
                    }
                }
            }
        }
        @Override
        public void discoveringChanged(final BluetoothAdapter adapter, final boolean enabled, final boolean keepAlive, final long timestamp) {
            if( DEBUG ) {
                System.err.println("Adapter.StatusListener.DISCOVERING: enabled "+enabled+", keepAlive "+keepAlive+" on "+adapter);
            }
            if( !enabled && keepAlive ) {
                // Don't update isDiscovering:=false and don't notify user IF keepAlive!
                return;
            }
            if( isDiscovering.compareAndSet(!enabled, enabled) ) {
                synchronized(userCallbackLock) {
                    if( null != userDiscoveringNotificationCB ) {
                        userDiscoveringNotificationCB.run(enabled);
                    }
                }
            }
        }
        @Override
        public void deviceFound(final BluetoothDevice device, final long timestamp) {
            if( DEBUG ) {
                System.err.println("Adapter.StatusListener.FOUND: "+device+" on "+device.getAdapter());
            }
            synchronized(discoveredDevicesLock) {
                discoveredDevices.add(device);
            }
        }

        @Override
        public void deviceUpdated(final BluetoothDevice device, final EIRDataTypeSet updateMask, final long timestamp) {
            if( DEBUG ) {
                System.err.println("Adapter.StatusListener.UPDATED: "+updateMask+" of "+device+" on "+device.getAdapter());
            }
            // nop on discoveredDevices
        }

        @Override
        public void deviceConnected(final BluetoothDevice device, final long timestamp) {
            if( DEBUG ) {
                System.err.println("Adapter.StatusListener.CONNECTED: "+device+" on "+device.getAdapter());
            }
        }

        @Override
        public void deviceDisconnected(final BluetoothDevice device, final HCIStatusCode reason, final long timestamp) {
            if( DEBUG ) {
                System.err.println("Adapter.StatusListener.DISCONNECTED: Reason "+reason+": "+device+" on "+device.getAdapter());
            }
        }
    };

    /**
     * Returns the matching {@link DBTObject} from the internal cache if found,
     * otherwise {@code null}.
     * <p>
     * The returned {@link DBTObject} may be of type
     * <ul>
     *   <li>{@link DBTDevice}</li>
     *   <li>{@link DBTGattService}</li>
     *   <li>{@link DBTGattCharacteristic}</li>
     *   <li>{@link DBTGattDescriptor}</li>
     * </ul>
     * or alternatively in {@link BluetoothObject} space
     * <ul>
     *   <li>{@link BluetoothType#DEVICE} -> {@link BluetoothDevice}</li>
     *   <li>{@link BluetoothType#GATT_SERVICE} -> {@link BluetoothGattService}</li>
     *   <li>{@link BluetoothType#GATT_CHARACTERISTIC} -> {@link BluetoothGattCharacteristic}</li>
     *   <li>{@link BluetoothType#GATT_DESCRIPTOR} -> {@link BluetoothGattDescriptor}</li>
     * </ul>
     * </p>
     * @param name name of the desired {@link BluetoothType#DEVICE device}.
     * Maybe {@code null}.
     * @param identifier EUI48 address of the desired {@link BluetoothType#DEVICE device}
     * or UUID of the desired {@link BluetoothType#GATT_SERVICE service},
     * {@link BluetoothType#GATT_CHARACTERISTIC characteristic} or {@link BluetoothType#GATT_DESCRIPTOR descriptor} to be found.
     * Maybe {@code null}, in which case the first object of the desired type is being returned - if existing.
     * @param type specify the type of the object to be found, either
     * {@link BluetoothType#DEVICE device},
     * {@link BluetoothType#GATT_SERVICE service}, {@link BluetoothType#GATT_CHARACTERISTIC characteristic}
     * or {@link BluetoothType#GATT_DESCRIPTOR descriptor}.
     * {@link BluetoothType#NONE none} means anything.
     */
    /* pp */ DBTObject findInCache(final String name, final String identifier, final BluetoothType type) {
        final boolean anyType = BluetoothType.NONE == type;
        final boolean deviceType = BluetoothType.DEVICE == type;
        final boolean serviceType = BluetoothType.GATT_SERVICE == type;
        final boolean charType = BluetoothType.GATT_CHARACTERISTIC== type;
        final boolean descType = BluetoothType.GATT_DESCRIPTOR == type;

        if( !anyType && !deviceType && !serviceType && !charType && !descType ) {
            return null;
        }
        synchronized(discoveredDevicesLock) {
            if( null == name && null == identifier && ( anyType || deviceType ) ) {
                // special case for 1st valid device
                if( discoveredDevices.size() > 0 ) {
                    return (DBTDevice) discoveredDevices.get(0);
                }
                return null; // no device
            }
            for(int devIdx = discoveredDevices.size() - 1; devIdx >= 0; devIdx-- ) {
                final DBTDevice device = (DBTDevice) discoveredDevices.get(devIdx);
                if( ( anyType || deviceType ) ) {
                    if( null != name && null != identifier &&
                        device.getName().equals(name) &&
                        device.getAddress().equals(identifier)
                      )
                    {
                        return device;
                    }
                    if( null != identifier &&
                        device.getAddress().equals(identifier)
                      )
                    {
                        return device;
                    }
                    if( null != name &&
                        device.getName().equals(name)
                      )
                    {
                        return device;
                    }
                }
                if( anyType || serviceType || charType || descType ) {
                    final DBTObject dbtObj = device.findInCache(identifier, type);
                    if( null != dbtObj ) {
                        return dbtObj;
                    }
                }
            }
            return null;
        }
    }
}
