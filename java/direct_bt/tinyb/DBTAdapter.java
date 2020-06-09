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
import java.util.concurrent.atomic.AtomicInteger;

import org.tinyb.AdapterSettings;
import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothAddressType;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
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

    private final Object stateLock = new Object();
    private final Object discoveredDevicesLock = new Object();

    private BluetoothNotification<Boolean> discoverableNotification = null;
    private volatile boolean isDiscoverable = false;
    private BluetoothNotification<Boolean> discoveringNotification = null;
    private volatile boolean isDiscovering = false;

    private BluetoothNotification<Boolean> poweredNotification = null;
    private volatile boolean isPowered = false;
    private BluetoothNotification<Boolean> pairableNotification = null;
    private volatile boolean isPairable = false;

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

        for(final Iterator<BluetoothDevice> id = discoveredDevices.iterator(); id.hasNext(); ) {
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
        if (obj == null || !(obj instanceof DBTDevice)) {
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
        final BluetoothManager manager = DBTManager.getBluetoothManager();
        return (BluetoothDevice) manager.find(BluetoothType.DEVICE, name, address, this, timeoutMS);
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
    public boolean getPowered() { return isPowered; }

    @Override
    public synchronized void enablePoweredNotifications(final BluetoothNotification<Boolean> callback) {
        poweredNotification = callback;
    }

    @Override
    public synchronized void disablePoweredNotifications() {
        poweredNotification = null;
    }

    @Override
    public boolean getDiscoverable() { return isDiscoverable; }

    @Override
    public synchronized void enableDiscoverableNotifications(final BluetoothNotification<Boolean> callback) {
        discoverableNotification = callback;
    }

    @Override
    public synchronized void disableDiscoverableNotifications() {
        discoverableNotification = null;
    }

    @Override
    public boolean getDiscovering() { return isDiscovering; }

    @Override
    public synchronized void enableDiscoveringNotifications(final BluetoothNotification<Boolean> callback) {
        discoveringNotification = callback;
    }

    @Override
    public synchronized void disableDiscoveringNotifications() {
        discoveringNotification = null;
    }

    @Override
    public boolean getPairable() { return isPairable; }

    @Override
    public synchronized void enablePairableNotifications(final BluetoothNotification<Boolean> callback) {
        pairableNotification = callback;
    }

    @Override
    public synchronized void disablePairableNotifications() {
        pairableNotification = null;
    }

    @Override
    public String toString() { return toStringImpl(); }

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
    protected native void deleteImpl();

    /* discovery */

    @Override
    public boolean startDiscovery() throws BluetoothException {
        return startDiscovery(true);
    }

    @Override
    public synchronized boolean startDiscovery(final boolean keepAlive) throws BluetoothException {
        removeDevices();
        final boolean res = startDiscoveryImpl(keepAlive);
        isDiscovering = res;
        return res;
    }
    private native boolean startDiscoveryImpl(boolean keepAlive) throws BluetoothException;

    @Override
    public synchronized boolean stopDiscovery() throws BluetoothException {
        if( isDiscovering ) {
            isDiscovering = false;
            return stopDiscoveryImpl();
        }
        return false;
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
            System.err.println("DBTAdapter::removeDevices: Inconsistent discovered device count: Native "+cn+", callback "+cj);
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
                    isPowered = newmask.isSet(AdapterSettings.SettingType.POWERED);
                    final BluetoothNotification<Boolean> _poweredNotification = poweredNotification;
                    if( null != _poweredNotification ) {
                        _poweredNotification.run(isPowered);
                    }
                }
            }
            {
                if( changedmask.isSet(AdapterSettings.SettingType.DISCOVERABLE) ) {
                    isDiscoverable = newmask.isSet(AdapterSettings.SettingType.DISCOVERABLE);
                    final BluetoothNotification<Boolean> _discoverableNotification = discoverableNotification;
                    if( null != _discoverableNotification ) {
                        _discoverableNotification.run( isDiscoverable );
                    }
                }
            }
            {
                if( changedmask.isSet(AdapterSettings.SettingType.BONDABLE) ) {
                    isPairable = newmask.isSet(AdapterSettings.SettingType.BONDABLE);
                    final BluetoothNotification<Boolean> _pairableNotification = pairableNotification;
                    if( null != _pairableNotification ) {
                        _pairableNotification.run( isPairable );
                    }
                }
            }
        }
        @Override
        public void discoveringChanged(final BluetoothAdapter adapter, final boolean enabled, final boolean keepAlive, final long timestamp) {
            if( DEBUG ) {
                System.err.println("Adapter.StatusListener.DISCOVERING: enabled "+enabled+", keepAlive "+keepAlive+" on "+adapter);
            }
            if( isDiscovering != enabled ) {
                isDiscovering = enabled;
                if( null != discoveringNotification ) {
                    discoveringNotification.run(enabled);
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
}
