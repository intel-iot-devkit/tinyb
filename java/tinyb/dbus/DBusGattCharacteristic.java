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

import java.util.List;

import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattDescriptor;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothType;
import org.tinyb.GATTCharacteristicListener;

public class DBusGattCharacteristic extends DBusObject implements BluetoothGattCharacteristic
{
    @Override
    public native BluetoothType getBluetoothType();
    @Override
    public native DBusGattCharacteristic clone();

    static BluetoothType class_type() { return BluetoothType.GATT_CHARACTERISTIC; }

    @Override
    public BluetoothGattDescriptor find(final String UUID, final long timeoutMS) {
        final BluetoothManager manager = DBusManager.getBluetoothManager();
        return (BluetoothGattDescriptor) manager.find(BluetoothType.GATT_DESCRIPTOR,
                null, UUID, this, timeoutMS);
    }

    @Override
    public BluetoothGattDescriptor find(final String UUID) {
        return find(UUID, 0);
    }

    /* D-Bus method calls: */

    @Override
    public native byte[] readValue() throws BluetoothException;

    @Override
    public native void enableValueNotifications(BluetoothNotification<byte[]> callback);

    @Override
    public native void disableValueNotifications();

    @Override
    public native boolean writeValue(byte[] argValue) throws BluetoothException;

    /* D-Bus property accessors: */

    @Override
    public native String getUUID();

    @Override
    public native BluetoothGattService getService();

    @Override
    public native byte[] getValue();

    @Override
    public native boolean getNotifying();

    @Override
    public native String[] getFlags();

    @Override
    public native List<BluetoothGattDescriptor> getDescriptors();

    private native void init(DBusGattCharacteristic obj);

    private native void delete();

    private DBusGattCharacteristic(final long instance)
    {
        super(instance);
    }

    @Override
    public boolean addCharacteristicListener(final GATTCharacteristicListener listener) {
        return false; // FIXME
    }
    @Override
    public boolean configNotificationIndication(final boolean enableNotification, final boolean enableIndication, final boolean[] enabledState)
            throws IllegalStateException
    {
        return false; // FIXME
    }

    @Override
    public boolean addCharacteristicListener(final GATTCharacteristicListener listener, final boolean[] enabledState)
            throws IllegalStateException
    {
        return false; // FIXME
    }
    @Override
    public boolean removeCharacteristicListener(final GATTCharacteristicListener l, final boolean disableIndicationNotification) {
        return false; // FIXME
    }
    @Override
    public int removeAllAssociatedCharacteristicListener(final boolean disableIndicationNotification) {
        return 0; // FIXME
    }

    @Override
    public String toString() {
        return "Characteristic[uuid "+getUUID()+"]";
    }
}
