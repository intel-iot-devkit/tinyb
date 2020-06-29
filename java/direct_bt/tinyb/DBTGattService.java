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

import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothType;

public class DBTGattService extends DBTObject implements BluetoothGattService
{
    /** Service's device weak back-reference */
    final WeakReference<DBTDevice> wbr_device;

    private final boolean isPrimary;
    private final String type_uuid;
    private final short handleStart;
    private final short handleEnd;
    private final List<BluetoothGattCharacteristic> characteristicList;

   /* pp */ DBTGattService(final long nativeInstance, final DBTDevice device, final boolean isPrimary,
                           final String type_uuid, final short handleStart, final short handleEnd)
    {
        super(nativeInstance, compHash(handleStart, handleEnd));
        this.wbr_device = new WeakReference<DBTDevice>(device);
        this.isPrimary = isPrimary;
        this.type_uuid = type_uuid;
        this.handleStart = handleStart;
        this.handleEnd = handleEnd;
        this.characteristicList = getCharacteristicsImpl();
    }

    @Override
    public boolean equals(final Object obj)
    {
        if (obj == null || !(obj instanceof DBTGattService)) {
            return false;
        }
        final DBTGattService other = (DBTGattService)obj;
        return handleStart == other.handleStart && handleEnd == other.handleEnd; /** unique attribute handles */
    }

    @Override
    public String getUUID() { return type_uuid; }

    @Override
    public BluetoothType getBluetoothType() { return class_type(); }

    static BluetoothType class_type() { return BluetoothType.GATT_SERVICE; }

    @Override
    public final BluetoothGattService clone()
    { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public BluetoothGattCharacteristic find(final String UUID, final long timeoutMS) {
            final BluetoothManager manager = DBTManager.getBluetoothManager();
            return (DBTGattCharacteristic) manager.find(BluetoothType.GATT_CHARACTERISTIC,
                null, UUID, this, timeoutMS);
    }

    @Override
    public BluetoothGattCharacteristic find(final String UUID) {
            return find(UUID, 0);
    }

    @Override
    public final BluetoothDevice getDevice() { return wbr_device.get(); }

    @Override
    public final boolean getPrimary() { return isPrimary; }

    @Override
    public final List<BluetoothGattCharacteristic> getCharacteristics() { return characteristicList; }

    /**
     * Returns the service start handle.
     * <p>
     * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
     * </p>
     */
    public final short getHandleStart() { return handleStart; }

    /**
     * Returns the service end handle.
     * <p>
     * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
     * </p>
     */
    public final short getHandleEnd() { return handleEnd; }

    @Override
    public final String toString() {
        if( !isValid() ) {
            return "Service" + "\u271D" + "[handles [0x"+Integer.toHexString(handleStart)+".."+Integer.toHexString(handleEnd)+"]]";
        }
        return toStringImpl();
    }

    /* Native method calls: */

    private native String toStringImpl();

    private native List<BluetoothGattCharacteristic> getCharacteristicsImpl();

    @Override
    protected native void deleteImpl(long nativeInstance);
}
