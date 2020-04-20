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
import org.tinyb.BluetoothDeviceDiscoveryListener;
import org.tinyb.TransportType;

public class Adapter extends DBTObject implements BluetoothAdapter
{
    private final String address;
    private final String name;
    private final DiscoveryThread discoveryThread;

    /* pp */ Adapter(final long nativeInstance, final String address, final String name)
    {
        super(nativeInstance, compHash(address, name));
        this.address = address;
        this.name = name;
        this.discoveryThread = new DiscoveryThread();
        this.discoveryThread.start(null);
        initImpl(this.discoveryThread.deviceDiscoveryListener);
    }

    @Override
    public synchronized void close() {
        discoveryThread.stop();
        super.close();
    }

    @Override
    public boolean equals(final Object obj)
    {
        if (obj == null || !(obj instanceof Device)) {
            return false;
        }
        final Adapter other = (Adapter)obj;
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
        final BluetoothManager manager = Manager.getBluetoothManager();
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
        synchronized(discoveryThread.discoveredDevicesLock) {
            final int count = discoveryThread.discoveredDevices.size();
            if( count > 0 ) {
                out.append("\n");
                for(final Iterator<BluetoothDevice> iter=discoveryThread.discoveredDevices.iterator(); iter.hasNext(); ) {
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

    private native void initImpl(final BluetoothDeviceDiscoveryListener l);

    @Override
    protected native void deleteImpl();

    /* discovery */

    @Override
    public boolean startDiscovery() throws BluetoothException {
        return discoveryThread.doDiscovery(true);
    }
    private native boolean startDiscoveryImpl() throws BluetoothException;

    @Override
    public boolean stopDiscovery() throws BluetoothException {
        return discoveryThread.doDiscovery(false);
    }
    private native boolean stopDiscoveryImpl() throws BluetoothException;

    @Override
    public List<BluetoothDevice> getDevices() {
        return discoveryThread.getDiscoveredDevices();
    }

    @Override
    public int removeDevices() throws BluetoothException {
        final int cj = discoveryThread.removeDiscoveredDevices();
        final int cn = removeDevicesImpl();
        if( cj != cn ) {
            throw new InternalError("Inconsistent discovered device count: Native "+cn+", callback "+cj);
        }
        return cn;
    }
    private native int removeDevicesImpl() throws BluetoothException;

    @Override
    public boolean getDiscovering() { return discoveryThread.running && discoveryThread.doDiscovery; }

    @Override
    public void setDeviceDiscoveryListener(final BluetoothDeviceDiscoveryListener l) {
        discoveryThread.setDeviceDiscoveryListener(l);
    }

    @Override
    public void enableDiscoveringNotifications(final BluetoothNotification<Boolean> callback) {
        discoveryThread.setDiscoveringNotificationCallback(callback);
    }

    @Override
    public void disableDiscoveringNotifications() {
        discoveryThread.setDiscoveringNotificationCallback(null);
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

    private native int discoverAnyDeviceImpl(final int timeoutMS) throws BluetoothException;
    // std::vector<std::shared_ptr<direct_bt::HCIDevice>> discoveredDevices = adapter.getDiscoveredDevices();
    private native List<BluetoothDevice> getDiscoveredDevicesImpl();

    private class DiscoveryThread implements Runnable {
        private int instanceID=-1;

        private volatile boolean running = false;
        private volatile boolean doDiscovery = false;
        private final Object stateLock = new Object();
        private volatile BluetoothDeviceDiscoveryListener userDeviceDiscoveryListener = null;
        private volatile BluetoothNotification<Boolean> discoveringNotificationCB = null;
        private List<BluetoothDevice> discoveredDevices = new ArrayList<BluetoothDevice>();
        private final Object discoveredDevicesLock = new Object();

        private final BluetoothDeviceDiscoveryListener deviceDiscoveryListener = new BluetoothDeviceDiscoveryListener() {
            @Override
            public void deviceAdded(final BluetoothAdapter a, final BluetoothDevice device) {
                final BluetoothDeviceDiscoveryListener l = userDeviceDiscoveryListener;
                System.err.println("DBTAdapter.DeviceDiscoveryListener.added: "+device+" on "+a);
                synchronized(discoveredDevicesLock) {
                    discoveredDevices.add(device);
                }
                if( null != l ) {
                    l.deviceAdded(a, device);
                }
            }

            @Override
            public void deviceUpdated(final BluetoothAdapter a, final BluetoothDevice device) {
                System.err.println("DBTAdapter.DeviceDiscoveryListener.updated: "+device+" on "+a);
                // nop on discoveredDevices
                userDeviceDiscoveryListener.deviceUpdated(a, device);
            }

            @Override
            public void deviceRemoved(final BluetoothAdapter a, final BluetoothDevice device) {
                final BluetoothDeviceDiscoveryListener l = userDeviceDiscoveryListener;
                System.err.println("DBTAdapter.DeviceDiscoveryListener.removed: "+device+" on "+a);
                synchronized(discoveredDevicesLock) {
                    discoveredDevices.remove(device);
                }
                if( null != l ) {
                    l.deviceRemoved(a, device);
                }
            }
        };

        public void start(final ThreadGroup tg) {
            synchronized( stateLock ) {
                if ( !running ) {
                    instanceID = globThreadID.addAndGet(1);
                    final Thread t = new Thread(tg, this, "Adapter-"+instanceID); // Thread name aligned w/ 'Thread-#'
                    t.setDaemon(true);
                    t.start();
                    while( !running ) {
                        try {
                            stateLock.wait();
                        } catch (final InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
        }
        public void setDeviceDiscoveryListener(final BluetoothDeviceDiscoveryListener l) {
            synchronized( stateLock ) {
                userDeviceDiscoveryListener = l;
                stateLock.notifyAll();
            }
        }
        public void setDiscoveringNotificationCallback(final BluetoothNotification<Boolean> cb) {
            synchronized( stateLock ) {
                discoveringNotificationCB = cb;
                stateLock.notifyAll();
            }
        }
        public List<BluetoothDevice> getDiscoveredDevices() {
            synchronized(discoveredDevicesLock) {
                return new ArrayList<BluetoothDevice>(discoveredDevices);
            }
        }
        public int removeDiscoveredDevices() {
            synchronized(discoveredDevicesLock) {
                final int n = discoveredDevices.size();
                discoveredDevices = new ArrayList<BluetoothDevice>();
                return n;
            }
        }
        public boolean doDiscovery(final boolean v) {
            synchronized( stateLock ) {
                if( v == doDiscovery ) {
                    return v;
                }
                final BluetoothNotification<Boolean> cb = discoveringNotificationCB;
                final boolean enable;
                if( v ) {
                    enable = startDiscoveryImpl();
                } else {
                    enable = false;
                    stopDiscoveryImpl();
                }
                doDiscovery = enable;
                if( null != cb ) {
                    cb.run(enable);
                }
                stateLock.notifyAll();
                return enable;
            }
        }
        public void stop() {
            synchronized( stateLock ) {
                stopDiscoveryImpl();
                running = false;
                doDiscovery = false;
                stateLock.notifyAll();
            }
        }

        @Override
        public void run() {
            synchronized( stateLock ) {
                running = true;
                stateLock.notifyAll();
            }
            while (running) {
                synchronized( stateLock ) {
                    while( running && !doDiscovery ) {
                        try {
                            stateLock.wait();
                        } catch (final InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
                if( !running ) {
                    break; // end loop and thread
                }

                try {
                    if( doDiscovery ) {
                        // will trigger all discovery callbacks!
                        discoverAnyDeviceImpl(discoverTimeoutMS);
                    }
                } catch (final Exception e) {
                    System.err.println(e.toString());
                    e.printStackTrace();
                    doDiscovery = false;
                }
            } // loop

            instanceID = -1;
        }
    }
    private static AtomicInteger globThreadID = new AtomicInteger(0);
    private static int discoverTimeoutMS = 100;
}
