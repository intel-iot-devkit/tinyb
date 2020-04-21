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

import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothType;
import org.tinyb.BluetoothUtils;

public class DBTDevice extends DBTObject implements BluetoothDevice
{
    private final DBTAdapter adapter;
    private final String address;
    private final String name;
    private final long ts_creation;
    long ts_update;

    /* pp */ DBTDevice(final long nativeInstance, final DBTAdapter adptr, final String address, final String name, final long ts_creation)
    {
        super(nativeInstance, compHash(address, name));
        this.adapter = adptr;
        this.address = address;
        this.name = name;
        this.ts_creation = ts_creation;
        ts_update = ts_creation;
        initImpl();
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
    public String getAddress() { return address; }

    @Override
    public String getName() { return name; }

    @Override
    public BluetoothType getBluetoothType() { return class_type(); }

    static BluetoothType class_type() { return BluetoothType.DEVICE; }

    @Override
    public final BluetoothDevice clone()
    { throw new UnsupportedOperationException(); } // FIXME

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

    @Override
    public String toString() {
        final StringBuilder out = new StringBuilder();
        final long t0 = BluetoothUtils.getCurrentMilliseconds();
        // std::string msdstr = nullptr != msd ? msd->toString() : "MSD[null]";
        final String msdstr = "MSD[null]";
        out.append("Device[").append(getAddress()).append(", '").append(getName())
                .append("', age ").append(t0-ts_creation).append(" ms, lup ").append(t0-ts_update).append(" ms, rssi ").append(getRSSI())
                .append(", tx-power ").append(getTxPower()).append(", ").append(msdstr).append("]");
        /**
        if(services.size() > 0 ) {
            out.append("\n");
            final int i=0;
            for(final auto it = services.begin(); it != services.end(); it++, i++) {
                if( 0 < i ) {
                    out.append("\n");
                }
                std::shared_ptr<uuid_t> p = *it;
                out.append("  ").append(p->toUUID128String()).append(", ").append(std::to_string(static_cast<int>(p->getTypeSize()))).append(" bytes");
            }
        } */
        return out.toString();
    }

    /* internal */

    private native void initImpl();

    /* DBT method calls: Connection */

    @Override
    public final void enableConnectedNotifications(final BluetoothNotification<Boolean> callback) {
        connectedNotifications = callback;
    }
    private BluetoothNotification<Boolean> connectedNotifications = null;
    private boolean connected = false;

    @Override
    public final boolean getConnected() { return connected; }
    // private native boolean getConnectedImpl();

    @Override
    public final boolean disconnect() throws BluetoothException {
        boolean res = false;
        if( connected ) {
            res = disconnectImpl();
            if( res ) {
                connectedNotifications.run(Boolean.FALSE);
                connected = false;
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
                connectedNotifications.run(Boolean.TRUE);
                connected = true;
            }
        }
        return res;
    }
    private native boolean connectImpl() throws BluetoothException;

    /* DBT method calls: */

    @Override
    public native boolean connectProfile(String arg_UUID) throws BluetoothException;

    @Override
    public native boolean disconnectProfile(String arg_UUID) throws BluetoothException;

    @Override
    public boolean pair() throws BluetoothException
    { throw new UnsupportedOperationException(); } // FIXME

     @Override
    public native boolean remove() throws BluetoothException;

    @Override
    public boolean cancelPairing() throws BluetoothException
    { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public native List<BluetoothGattService> getServices();

    /* property accessors: */

    @Override
    public String getAlias() { return null; } // FIXME

    @Override
    public void setAlias(final String value)
    { throw new UnsupportedOperationException(); } // FIXME

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
    public native void disableConnectedNotifications();

    @Override
    public native String[] getUUIDs();

    @Override
    public native String getModalias();

    @Override
    public native DBTAdapter getAdapter();

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
    public final void enableServicesResolvedNotifications(final BluetoothNotification<Boolean> callback) {
        servicesResolvedNotifications = callback;
    }
    private BluetoothNotification<Boolean> servicesResolvedNotifications = null;

    @Override
    public native void disableServicesResolvedNotifications();

    @Override
    protected native void deleteImpl();
}
