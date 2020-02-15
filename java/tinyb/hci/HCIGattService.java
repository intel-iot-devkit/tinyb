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

package tinyb.hci;

import java.util.List;

import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothType;

public class HCIGattService extends HCIObject implements BluetoothGattService
{
    private final String uuid;

   /* pp */ HCIGattService(final String uuid)
    {
        super(uuid.hashCode());
        this.uuid = uuid;
    }

    @Override
    public boolean equals(final Object obj)
    {
        if (obj == null || !(obj instanceof HCIGattService)) {
            return false;
        }
        final HCIGattService other = (HCIGattService)obj;
        return uuid.equals(other.uuid);
    }

    @Override
    public String getUUID() { return uuid; }

    @Override
    public native BluetoothType getBluetoothType();

    @Override
    public native BluetoothGattService clone();

    static BluetoothType class_type() { return BluetoothType.GATT_SERVICE; }

    @Override
    public BluetoothGattCharacteristic find(final String UUID, final long timeoutMS) {
            final BluetoothManager manager = HCIManager.getBluetoothManager();
            return (HCIGattCharacteristic) manager.find(BluetoothType.GATT_CHARACTERISTIC,
                null, UUID, this, timeoutMS);
    }

    @Override
    public BluetoothGattCharacteristic find(final String UUID) {
            return find(UUID, 0);
    }

    /* D-Bus property accessors: */

    @Override
    public native HCIDevice getDevice();

    @Override
    public native boolean getPrimary();

    @Override
    public native List<BluetoothGattCharacteristic> getCharacteristics();

    private native void delete();
}
