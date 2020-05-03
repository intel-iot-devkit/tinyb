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
 * {@link BluetoothDevice} listener for the respective {@link BluetoothDevice} discovery events: Added, updated and removed.
 * <p>
 * A listener instance may be attached to a {@link BluetoothAdapter} via
 * {@link BluetoothAdapter#setDeviceStatusListener(BluetoothDeviceDiscoveryListener)}.
 * </p>
 */
public interface BluetoothDeviceStatusListener {
    /** A {@link BluetoothDevice} has been newly discovered. */
    public void deviceFound(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
    /** An already discovered {@link BluetoothDevice} has been updated. */
    public void deviceUpdated(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp, final EIRDataType updateMask);
    /** {@link BluetoothDevice} has been connected. */
    public void deviceConnected(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
    /** {@link BluetoothDevice} has been disconnected. */
    public void deviceDisconnected(final BluetoothAdapter adapter, final BluetoothDevice device, final long timestamp);
};
