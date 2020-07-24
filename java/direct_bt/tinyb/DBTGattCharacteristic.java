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
import java.util.Arrays;
import java.util.List;

import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattDescriptor;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothObject;
import org.tinyb.BluetoothType;
import org.tinyb.BluetoothUtils;
import org.tinyb.GATTCharacteristicListener;

public class DBTGattCharacteristic extends DBTObject implements BluetoothGattCharacteristic
{
    private static final boolean DEBUG = DBTManager.DEBUG;

    /** Characteristics's service weak back-reference */
    final WeakReference<DBTGattService> wbr_service;

    /**
     * Characteristic Handle of this instance.
     * <p>
     * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
     * </p>
     */
    private final short handle;

    /* Characteristics Property */
    private final String[] properties;
    private final boolean hasNotify;
    private final boolean hasIndicate;

    /* Characteristics Value Type UUID */
    private final String value_type_uuid;

    /**
     * Characteristics Value Handle.
     * <p>
     * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
     * </p>
     */
    private final short value_handle;

    /* Optional Client Characteristic Configuration index within descriptorList */
    private final int clientCharacteristicsConfigIndex;

    /* pp */ final List<BluetoothGattDescriptor> descriptorList;

    private byte[] cachedValue = null;
    private BluetoothNotification<byte[]> valueNotificationCB = null;

    private boolean updateCachedValue(final byte[] value, final boolean notify) {
        boolean valueChanged = false;
        if( null == cachedValue || cachedValue.length != value.length ) {
            cachedValue = new byte[value.length];
            valueChanged = true;
        } else if( !Arrays.equals(value, cachedValue) ) {
            valueChanged = true;
        }
        if( valueChanged ) {
            System.arraycopy(value, 0, cachedValue, 0, value.length);
            if( notify && null != valueNotificationCB ) {
                valueNotificationCB.run(cachedValue);
            }
        }
        return valueChanged;
    }

   /* pp */ DBTGattCharacteristic(final long nativeInstance, final DBTGattService service,
                                  final short handle, final String[] properties,
                                  final boolean hasNotify, final boolean hasIndicate,
                                  final String value_type_uuid, final short value_handle,
                                  final int clientCharacteristicsConfigIndex)
    {
        super(nativeInstance, handle /* hash */);
        this.wbr_service = new WeakReference<DBTGattService>(service);
        this.handle = handle;

        this.properties = properties;
        this.hasNotify = hasNotify;
        this.hasIndicate = hasIndicate;
        this.value_type_uuid = value_type_uuid;
        this.value_handle = value_handle;
        this.clientCharacteristicsConfigIndex = clientCharacteristicsConfigIndex;
        this.descriptorList = getDescriptorsImpl();

        if( hasNotify || hasIndicate ) {
            // This characteristicListener serves TinyB 'enableValueNotification(..)' and 'getValue()' (cached value)
            // backwards compatibility only!
            final GATTCharacteristicListener characteristicListener = new GATTCharacteristicListener(this) {
                @Override
                public void notificationReceived(final BluetoothGattCharacteristic charDecl, final byte[] value, final long timestamp) {
                    final DBTGattCharacteristic cd = (DBTGattCharacteristic)charDecl;
                    if( !cd.equals(DBTGattCharacteristic.this) ) {
                        throw new InternalError("Filtered GATTCharacteristicListener.notificationReceived: Wrong Characteristic: Got "+charDecl+
                                                ", expected "+DBTGattCharacteristic.this.toString());
                    }
                    final boolean valueChanged = updateCachedValue(value, true);
                    if( DEBUG ) {
                        System.err.println("GATTCharacteristicListener.notificationReceived: "+charDecl+
                                           ", value[changed "+valueChanged+", data "+BluetoothUtils.bytesHexString(value, true, true)+"]");
                    }
                }
                @Override
                public void indicationReceived(final BluetoothGattCharacteristic charDecl, final byte[] value, final long timestamp,
                                               final boolean confirmationSent) {
                    final DBTGattCharacteristic cd = (DBTGattCharacteristic)charDecl;
                    if( !cd.equals(DBTGattCharacteristic.this) ) {
                        throw new InternalError("Filtered GATTCharacteristicListener.indicationReceived: Wrong Characteristic: Got "+charDecl+
                                                ", expected "+DBTGattCharacteristic.this.toString());
                    }
                    final boolean valueChanged = updateCachedValue(value, true);
                    if( DEBUG ) {
                        System.err.println("GATTCharacteristicListener.indicationReceived: "+charDecl+
                                           ", value[changed "+valueChanged+", data "+BluetoothUtils.bytesHexString(value, true, true)+"]");
                    }
                }
            };
            this.addCharacteristicListener(characteristicListener); // silent, don't enable native GATT ourselves
        }
    }

    @Override
    public synchronized void close() {
        if( !isValid() ) {
            return;
        }
        removeAllAssociatedCharacteristicListener(true);
        super.close();
    }

    @Override
    public boolean equals(final Object obj)
    {
        if (obj == null || !(obj instanceof DBTGattCharacteristic)) {
            return false;
        }
        final DBTGattCharacteristic other = (DBTGattCharacteristic)obj;
        return handle == other.handle; /** unique attribute handles */
    }

    @Override
    public String getUUID() { return value_type_uuid; }

    @Override
    public BluetoothType getBluetoothType() { return class_type(); }

    static BluetoothType class_type() { return BluetoothType.GATT_CHARACTERISTIC; }

    @Override
    public BluetoothGattCharacteristic clone()
    { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public BluetoothGattDescriptor find(final String UUID, final long timeoutMS) {
        if( !checkServiceCache() ) {
            return null;
        }
        return (DBTGattDescriptor) findInCache(UUID, BluetoothType.GATT_DESCRIPTOR);
    }

    @Override
    public BluetoothGattDescriptor find(final String UUID) {
        return find(UUID, 0);
    }

    @Override
    public final BluetoothGattService getService() { return wbr_service.get(); }

    @Override
    public final String[] getFlags() { return properties; }

    @Override
    public final byte[] getValue() { return cachedValue; }

    @Override
    public final byte[] readValue() throws BluetoothException {
        final byte[] value = readValueImpl();
        updateCachedValue(value, true);
        return cachedValue;
    }

    @Override
    public final boolean writeValue(final byte[] value) throws BluetoothException {
        final boolean res = writeValueImpl(value);
        if( res ) {
            updateCachedValue(value, false);
        }
        return res;
    }

    @Override
    public final List<BluetoothGattDescriptor> getDescriptors() { return descriptorList; }

    @Override
    public final synchronized boolean configNotificationIndication(final boolean enableNotification, final boolean enableIndication, final boolean enabledState[/*2*/])
            throws IllegalStateException
    {
        if( hasNotify || hasIndicate ) {
            final boolean res = configNotificationIndicationImpl(enableNotification, enableIndication, enabledState);
            if( DEBUG ) {
                System.err.println("GATTCharacteristicListener.configNotificationIndication: "+res+", enableResult "+Arrays.toString(enabledState));
            }
            return res;
        } else {
            enabledState[0] = false;
            enabledState[1] = false;
            if( DEBUG ) {
                System.err.println("GATTCharacteristicListener.configNotificationIndication: FALSE*: hasNotify "+hasNotify+", hasIndicate "+hasIndicate);
            }
            return false;
        }
    }
    private native boolean configNotificationIndicationImpl(boolean enableNotification, boolean enableIndication, final boolean enabledState[/*2*/])
            throws IllegalStateException;

    @Override
    public final boolean addCharacteristicListener(final GATTCharacteristicListener listener) {
        return getService().getDevice().addCharacteristicListener(listener);
    }

    @Override
    public final boolean addCharacteristicListener(final GATTCharacteristicListener listener, final boolean enabledState[/*2*/]) {
        if( !configNotificationIndication(true /* enableNotification */, true /* enableIndication */, enabledState) ) {
            return false;
        }
        return getService().getDevice().addCharacteristicListener(listener);
    }

    @Override
    public final boolean removeCharacteristicListener(final GATTCharacteristicListener l, final boolean disableIndicationNotification) {
        if( disableIndicationNotification ) {
            configNotificationIndication(false /* enableNotification */, false /* enableIndication */, new boolean[2]);
        }
        return getService().getDevice().removeCharacteristicListener(l);
    }

    @Override
    public final int removeAllAssociatedCharacteristicListener(final boolean disableIndicationNotification) {
        if( disableIndicationNotification ) {
            configNotificationIndication(false /* enableNotification */, false /* enableIndication */, new boolean[2]);
        }
        valueNotificationCB = null;
        return getService().getDevice().removeAllAssociatedCharacteristicListener(this);
    }

    @Override
    public final synchronized void enableValueNotifications(final BluetoothNotification<byte[]> callback) {
        if( !configNotificationIndication(true /* enableNotification */, true /* enableIndication */, new boolean[2]) ) {
            valueNotificationCB = null;
        } else {
            valueNotificationCB = callback;
        }
    }

    @Override
    public final synchronized void disableValueNotifications() {
        configNotificationIndication(false /* enableNotification */, false /* enableIndication */, new boolean[2]);
        valueNotificationCB = null;
    }

    @Override
    public final boolean getNotifying() {
        return null != valueNotificationCB;
    }

    /**
     * Characteristic Handle of this instance.
     * <p>
     * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
     * </p>
     */
    public final short getHandle() { return handle; }

    /**
     * Returns Characteristics Value Handle.
     * <p>
     * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
     * </p>
     */
    public final short getValueHandle() { return value_handle; }

    /** Returns optional Client Characteristic Configuration index within descriptorList */
    public final int getClientCharacteristicsConfigIndex() { return clientCharacteristicsConfigIndex; }

    @Override
    public final String toString() {
        if( !isValid() ) {
            return "Characteristic" + "\u271D" + "[uuid "+getUUID()+", handle 0x"+Integer.toHexString(handle)+"]";
        }
        return toStringImpl();
    }

    /* Native method calls: */

    private native String toStringImpl();

    private native byte[] readValueImpl() throws BluetoothException;

    private native boolean writeValueImpl(byte[] argValue) throws BluetoothException;

    private native List<BluetoothGattDescriptor> getDescriptorsImpl();

    @Override
    protected native void deleteImpl(long nativeInstance);

    /* local functionality */

    /* pp */ boolean checkServiceCache() {
        final DBTGattService service = wbr_service.get();
        if( null == service ) {
            return false;
        }
        final DBTDevice device = service.wbr_device.get();
        return null != device && device.checkServiceCache(false);
    }

    /**
     * Returns the matching {@link DBTObject} from the internal cache if found,
     * otherwise {@code null}.
     * <p>
     * The returned {@link DBTObject} may be of type
     * <ul>
     *   <li>{@link DBTGattDescriptor}</li>
     * </ul>
     * or alternatively in {@link BluetoothObject} space
     * <ul>
     *   <li>{@link BluetoothType#GATT_DESCRIPTOR} -> {@link BluetoothGattDescriptor}</li>
     * </ul>
     * </p>
     * @param uuid UUID of the desired
     * {@link BluetoothType#GATT_DESCRIPTOR descriptor} to be found.
     * Maybe {@code null}, in which case the first object of the desired type is being returned - if existing.
     * @param type specify the type of the object to be found, a {@link BluetoothType#GATT_DESCRIPTOR descriptor}.
     * {@link BluetoothType#NONE none} means anything.
     */
    /* pp */ DBTObject findInCache(final String uuid, final BluetoothType type) {
        final boolean anyType = BluetoothType.NONE == type;
        final boolean descType = BluetoothType.GATT_DESCRIPTOR == type;

        if( !anyType && !descType ) {
            return null;
        }
        final int size = descriptorList.size();
        for(int i = 0; i < size; i++ ) {
            final DBTGattDescriptor descr = (DBTGattDescriptor) descriptorList.get(i);
            if( null == uuid || descr.getUUID().equals(uuid) ) {
                return descr;
            }
        }
        return null;
    }
}
