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

package org.tinyb;

/**
 * {@link BluetoothAdapter} status listener for {@link BluetoothDevice} discovery events: Added, updated and removed;
 * as well as for certain {@link BluetoothAdapter} events.
 * <p>
 * A listener instance may be attached to a {@link BluetoothAdapter} via
 * {@link BluetoothAdapter#addStatusListener(BluetoothDeviceDiscoveryListener, BluetoothDevice)}.
 * </p>
 * <p>
 * You can only attach one {@link AdapterStatusListener} instance at a time,
 * i.e. you cannot attach the same instance more than once to a {@link BluetoothAdapter}.
 * <br>
 * To attach multiple instances, you need to create one instance per attachment.
 * <br>
 * This restriction is due to implementation semantics of strictly associating
 * one Java {@link AdapterStatusListener} instance to one C++ {@code AdapterStatusListener} instance.
 * The latter will be added to the native list of listeners.
 * This class's {@code nativeInstance} field links the Java instance to mentioned C++ listener.
 * </p>
 */
public abstract class AdapterStatusListener {
    @SuppressWarnings("unused")
    private long nativeInstance;

    /** A {@link BluetoothAdapter} setting has been changed. */
    public void adapterSettingsChanged(final BluetoothAdapter adapter,
                                       final AdapterSettings oldmask, final AdapterSettings newmask,
                                       final AdapterSettings changedmask, final long timestamp) {
    }
    /** A {@link BluetoothDevice} has been newly discovered. */
    public void deviceFound(final BluetoothDevice device, final long timestamp) {
    }
    /** An already discovered {@link BluetoothDevice} has been updated. */
    public void deviceUpdated(final BluetoothDevice device, final long timestamp, final EIRDataTypeSet updateMask) {
    }
    /** {@link BluetoothDevice} has been connected. */
    public void deviceConnected(final BluetoothDevice device, final long timestamp) {
    }
    /** {@link BluetoothDevice} has been disconnected. */
    public void deviceDisconnected(final BluetoothDevice device, final long timestamp) {
    }
};
