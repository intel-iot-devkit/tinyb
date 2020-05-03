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

import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothType;
import org.tinyb.EIRDataType;
import org.tinyb.BluetoothDeviceStatusListener;
import org.tinyb.TransportType;

public class DBTAdapter extends DBTObject implements BluetoothAdapter
{
    private final String address;
    private final String name;

    /* pp */ DBTAdapter(final long nativeInstance, final String address, final String name)
    {
        super(nativeInstance, compHash(address, name));
        this.address = address;
        this.name = name;
        initImpl(this.deviceDiscoveryListener);
    }

    @Override
    public synchronized void close() {
        stopDiscovery();
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

    public String getInterfaceName() {
        throw new UnsupportedOperationException(); // FIXME
    }

    @Override
    public BluetoothType getBluetoothType() { return class_type(); }

    static BluetoothType class_type() { return BluetoothType.ADAPTER; }

    @Override
    public final BluetoothAdapter clone()
    { throw new UnsupportedOperationException(); } // FIXME

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
    public String toString() {
        final StringBuilder out = new StringBuilder();
        out.append("Adapter[").append(getAddress()).append(", '").append(getName()).append("', id=").append("]");
        synchronized(discoveredDevicesLock) {
            final int count = discoveredDevices.size();
            if( count > 0 ) {
                out.append("\n");
                for(final Iterator<BluetoothDevice> iter=discoveredDevices.iterator(); iter.hasNext(); ) {
                    final BluetoothDevice device = iter.next();
                    out.append("  ").append(device.toString()).append("\n");
                }
            }
        }
        return out.toString();
    }

    /* property accessors: */

    @Override
    public String getAlias() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public void setAlias(final String value) { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public long getBluetoothClass() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public native boolean getPowered();

    @Override
    public native void enablePoweredNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public native void disablePoweredNotifications();

    @Override
    public native void setPowered(boolean value);

    @Override
    public native boolean getDiscoverable();

    @Override
    public native void enableDiscoverableNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public native void disableDiscoverableNotifications();

    @Override
    public native void setDiscoverable(boolean value);

    @Override
    public native long getDiscoverableTimeout();

    @Override
    public native void setDiscoverableTimout(long value);

    @Override
    public native BluetoothDevice connectDevice(String address, String addressType);

    @Override
    public native boolean getPairable();

    @Override
    public native void enablePairableNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public native void disablePairableNotifications();

    @Override
    public native void setPairable(boolean value);

    @Override
    public native long getPairableTimeout();

    @Override
    public native void setPairableTimeout(long value);

    @Override
    public native String[] getUUIDs();

    @Override
    public native String getModalias();

    /* internal */

    private native void initImpl(final BluetoothDeviceStatusListener l);

    private synchronized void open() {
        if( !isOpen ) {
            isOpen = openImpl();
        }
    }
    private native boolean openImpl();
    boolean isOpen = false;

    @Override
    protected native void deleteImpl();

    /* discovery */

    @Override
    public synchronized boolean startDiscovery() throws BluetoothException {
        open();
        removeDevices();
        final boolean res = startDiscoveryImpl();
        isDiscovering = res;
        synchronized( stateLock ) {
            if( null != discoveringNotificationCB ) {
                discoveringNotificationCB.run(res);
            }
            stateLock.notifyAll();
        }
        return res;
    }
    private native boolean startDiscoveryImpl() throws BluetoothException;

    @Override
    public synchronized boolean stopDiscovery() throws BluetoothException {
        open();
        isDiscovering = false;
        final boolean res = stopDiscoveryImpl();
        synchronized( stateLock ) {
            if( null != discoveringNotificationCB ) {
                discoveringNotificationCB.run(false);
            }
            stateLock.notifyAll();
        }
        return res;
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
        open();
        final int cj = removeDiscoveredDevices();
        final int cn = removeDevicesImpl();
        if( cj != cn ) {
            throw new InternalError("Inconsistent discovered device count: Native "+cn+", callback "+cj);
        }
        return cn;
    }
    private native int removeDevicesImpl() throws BluetoothException;
    private int removeDiscoveredDevices() {
        synchronized(discoveredDevicesLock) {
            final int n = discoveredDevices.size();
            discoveredDevices = new ArrayList<BluetoothDevice>();
            return n;
        }
    }

    @Override
    public boolean getDiscovering() { return isDiscovering; }

    @Override
    public void setDeviceStatusListener(final BluetoothDeviceStatusListener l) {
        synchronized( stateLock ) {
            userDeviceStatusListener = l;
            stateLock.notifyAll();
        }
    }

    @Override
    public void enableDiscoveringNotifications(final BluetoothNotification<Boolean> callback) {
        setDiscoveringNotificationCallback(callback);
    }

    @Override
    public void disableDiscoveringNotifications() {
        setDiscoveringNotificationCallback(null);
    }

    @Override
    public void setDiscoveryFilter(final List<UUID> uuids, final int rssi, final int pathloss, final TransportType transportType) {
        final List<String> uuidsFmt = new ArrayList<>(uuids.size());
        for (final UUID uuid : uuids) {
            uuidsFmt.add(uuid.toString());
        }
        setDiscoveryFilter(uuidsFmt, rssi, pathloss, transportType.ordinal());
    }

    public void setRssiDiscoveryFilter(final int rssi) {
        setDiscoveryFilter(Collections.EMPTY_LIST, rssi, 0, TransportType.AUTO);
    }

    private native void setDiscoveryFilter(List<String> uuids, int rssi, int pathloss, int transportType);

    ////////////////////////////////////

    private final Object stateLock = new Object();
    private volatile BluetoothDeviceStatusListener userDeviceStatusListener = null;
    private volatile BluetoothNotification<Boolean> discoveringNotificationCB = null;
    private List<BluetoothDevice> discoveredDevices = new ArrayList<BluetoothDevice>();
    private final Object discoveredDevicesLock = new Object();
    private volatile boolean isDiscovering = false;

    private final BluetoothDeviceStatusListener deviceDiscoveryListener = new BluetoothDeviceStatusListener() {
        @Override
        public void deviceFound(final BluetoothAdapter a, final BluetoothDevice device, final long timestamp) {
            final BluetoothDeviceStatusListener l = userDeviceStatusListener;
            System.err.println("DBTAdapter.DeviceStatusListener.found: "+device+" on "+a);
            synchronized(discoveredDevicesLock) {
                discoveredDevices.add(device);
            }
            if( null != l ) {
                l.deviceFound(a, device, timestamp);
            }
        }

        @Override
        public void deviceUpdated(final BluetoothAdapter a, final BluetoothDevice device, final long timestamp, final EIRDataType updateMask) {
            System.err.println("DBTAdapter.DeviceStatusListener.updated: "+updateMask+" of "+device+" on "+a);
            // nop on discoveredDevices
            userDeviceStatusListener.deviceUpdated(a, device, timestamp, updateMask);
        }

        @Override
        public void deviceConnected(final BluetoothAdapter a, final BluetoothDevice device, final long timestamp) {
            final BluetoothDeviceStatusListener l = userDeviceStatusListener;
            System.err.println("DBTAdapter.DeviceStatusListener.connected: "+device+" on "+a);
            if( null != l ) {
                l.deviceConnected(a, device, timestamp);
            }
        }
        @Override
        public void deviceDisconnected(final BluetoothAdapter a, final BluetoothDevice device, final long timestamp) {
            final BluetoothDeviceStatusListener l = userDeviceStatusListener;
            System.err.println("DBTAdapter.DeviceStatusListener.disconnected: "+device+" on "+a);
            if( null != l ) {
                l.deviceDisconnected(a, device, timestamp);
            }
        }
    };
    private void setDiscoveringNotificationCallback(final BluetoothNotification<Boolean> cb) {
        synchronized( stateLock ) {
            discoveringNotificationCB = cb;
            stateLock.notifyAll();
        }
    }
    private static AtomicInteger globThreadID = new AtomicInteger(0);
    private static int discoverTimeoutMS = 100;
}
