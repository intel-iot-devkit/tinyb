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

import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattDescriptor;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothType;

public class DBTGattCharacteristic extends DBTObject implements BluetoothGattCharacteristic
{
    private final BluetoothGattService service;
    private final String[] properties;
    private final String uuid;

   /* pp */ DBTGattCharacteristic(final long nativeInstance, final BluetoothGattService service, final String[] properties, final String uuid)
    {
        super(nativeInstance, uuid.hashCode());
        this.service = service;
        this.properties = properties;
        this.uuid = uuid;
    }

    @Override
    public boolean equals(final Object obj)
    {
        if (obj == null || !(obj instanceof DBTGattCharacteristic)) {
            return false;
        }
        final DBTGattCharacteristic other = (DBTGattCharacteristic)obj;
        return uuid.equals(other.uuid);
    }

    @Override
    public String getUUID() { return uuid; }

    @Override
    public BluetoothType getBluetoothType() { return class_type(); }

    static BluetoothType class_type() { return BluetoothType.GATT_CHARACTERISTIC; }

    @Override
    public BluetoothGattCharacteristic clone()
    { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public BluetoothGattDescriptor find(final String UUID, final long timeoutMS) {
        final BluetoothManager manager = DBTManager.getBluetoothManager();
        return (BluetoothGattDescriptor) manager.find(BluetoothType.GATT_DESCRIPTOR,
                null, UUID, this, timeoutMS);
    }

    @Override
    public BluetoothGattDescriptor find(final String UUID) {
        return find(UUID, 0);
    }

    @Override
    public final BluetoothGattService getService() { return service; }

    /**
     * {@inheritDoc}
     * <p>
     * Actually the BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1.1 Characteristic Properties
     * </p>
     * <p>
     * Returns string values as defined in <https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/gatt-api.txt>
     * <pre>
     * org.bluez.GattCharacteristic1 :: array{string} Flags [read-only]
     * </pre>
     * </p>
     */
    @Override
    public final String[] getFlags() { return properties; }

    /* D-Bus method calls: */

    @Override
    public native byte[] readValue() throws BluetoothException;

    @Override
    public native void enableValueNotifications(BluetoothNotification<byte[]> callback);

    @Override
    public native void disableValueNotifications();

    @Override
    public native boolean writeValue(byte[] argValue) throws BluetoothException;

    /* Native accessors: */

    @Override
    public native byte[] getValue();

    @Override
    public native boolean getNotifying();

    @Override
    public native List<BluetoothGattDescriptor> getDescriptors();

    @Override
    protected native void deleteImpl();

}
