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
 * {@link BluetoothAdapter#addStatusListener(AdapterStatusListener, BluetoothDevice)}.
 * </p>
 * <p>
 * One {@link AdapterStatusListener} instance can only be attached to a listener receiver once at a time,
 * i.e. you cannot attach the same instance more than once to a {@link BluetoothAdapter}.
 * <br>
 * To attach multiple listener, one instance per attachment must be created.
 * <br>
 * This restriction is due to implementation semantics of strictly associating
 * one Java {@link AdapterStatusListener} instance to one C++ {@code AdapterStatusListener} instance.
 * The latter will be added to the native list of listeners.
 * This class's {@code nativeInstance} field links the Java instance to mentioned C++ listener.
 * <br>
 * Since the listener receiver maintains a unique set of listener instances without duplicates,
 * this restriction is more esoteric.
 * </p>
 * @since 2.0.0
 */
public abstract class AdapterStatusListener {
    @SuppressWarnings("unused")
    private long nativeInstance;

    /**
     * {@link BluetoothAdapter} setting(s) changed.
     * @param adapter the adapter which settings have changed.
     * @param oldmask the previous settings mask
     * @param newmask the new settings mask
     * @param changedmask the changes settings mask
     * @param timestamp the time in monotonic milliseconds when this event occurred. See {@link BluetoothUtils#getCurrentMilliseconds()}.
     */
    public void adapterSettingsChanged(final BluetoothAdapter adapter,
                                       final AdapterSettings oldmask, final AdapterSettings newmask,
                                       final AdapterSettings changedmask, final long timestamp) { }

    /**
     * {@link BluetoothAdapter}'s discovery state has changed, i.e. enabled or disabled.
     * @param adapter the adapter which discovering state has changed.
     * @param enabled the new discovery state
     * @param keepAlive if {@code true}, the discovery will be re-enabled if disabled by the underlying Bluetooth implementation.
     * @param timestamp the time in monotonic milliseconds when this event occurred. See {@link BluetoothUtils#getCurrentMilliseconds()}.
     */
    public void discoveringChanged(final BluetoothAdapter adapter, final boolean enabled, final boolean keepAlive, final long timestamp) { }

    /**
     * A {@link BluetoothDevice} has been newly discovered.
     * @param device the found device
     * @param timestamp the time in monotonic milliseconds when this event occurred. See {@link BluetoothUtils#getCurrentMilliseconds()}.
     */
    public void deviceFound(final BluetoothDevice device, final long timestamp) { }

    /**
     * An already discovered {@link BluetoothDevice} has been updated.
     * @param device the updated device
     * @param updateMask the update mask of changed data
     * @param timestamp the time in monotonic milliseconds when this event occurred. See {@link BluetoothUtils#getCurrentMilliseconds()}.
     */
    public void deviceUpdated(final BluetoothDevice device, final EIRDataTypeSet updateMask, final long timestamp) { }

    /**
     * {@link BluetoothDevice} got connected.
     * @param device the device which connection state has changed
     * @param timestamp the time in monotonic milliseconds when this event occurred. See {@link BluetoothUtils#getCurrentMilliseconds()}.
     */
    public void deviceConnected(final BluetoothDevice device, final long timestamp) { }

    /**
     * {@link BluetoothDevice} got disconnected.
     * @param device the device which connection state has changed
     * @param reason the {@link HCIErrorCode} reason for disconnection
     * @param timestamp the time in monotonic milliseconds when this event occurred. See {@link BluetoothUtils#getCurrentMilliseconds()}.
     */
    public void deviceDisconnected(final BluetoothDevice device, final HCIErrorCode reason, final long timestamp) { }
};
