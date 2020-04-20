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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;

import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothDeviceDiscoveryListener;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothType;
import org.tinyb.TransportType;

public class DBusAdapter extends DBusObject implements BluetoothAdapter
{
    @Override
    public native BluetoothType getBluetoothType();

    @Override
    public native BluetoothAdapter clone();

    static BluetoothType class_type() { return BluetoothType.ADAPTER; }

    @Override
    public BluetoothDevice find(final String name, final String address, final long timeoutMS) {
            final BluetoothManager manager = DBusManager.getBluetoothManager();
            return (BluetoothDevice) manager.find(BluetoothType.DEVICE, name, address, this, timeoutMS);
    }

    @Override
    public BluetoothDevice find(final String name, final String address) {
            return find(name, address, 0);
    }

    /* D-Bus method calls: */

    @Override
    public native boolean startDiscovery() throws BluetoothException;

    @Override
    public native boolean stopDiscovery() throws BluetoothException;

    @Override
    public native List<BluetoothDevice> getDevices();

    @Override
    public native int removeDevices() throws BluetoothException;

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
    public native long getBluetoothClass();

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
    public native boolean getDiscovering();

    @Override
    public void setDeviceDiscoveryListener(final BluetoothDeviceDiscoveryListener l) {
        throw new UnsupportedOperationException(); // FIXME
    }

    @Override
    public native void enableDiscoveringNotifications(BluetoothNotification<Boolean> callback);

    @Override
    public native void disableDiscoveringNotifications();

    @Override
    public native String[] getUUIDs();

    @Override
    public native String getModalias();

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

    @Override
    public String getInterfaceName() {
        final String[] path = getObjectPath().split("/");
        return path[path.length-1];
    }

    private native void delete();

    private native void setDiscoveryFilter(List<String> uuids, int rssi, int pathloss, int transportType);

    private DBusAdapter(final long instance)
    {
        super(instance);
    }
}
