/*
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

import java.util.*;

public class BluetoothManager
{
    private long nativeInstance;
    private static BluetoothManager inst;

    public native BluetoothType getBluetoothType();
    public native BluetoothObject getObject(BluetoothType type, String name,
                                    String identifier, BluetoothObject parent);
    public native List<BluetoothObject> getObjects(BluetoothType type, String name,
                                    String identifier, BluetoothObject parent);
    public native List<BluetoothAdapter> getAdapters();
    public native List<BluetoothDevice> getDevices();
    public native List<BluetoothGattService> getServices();
    public native boolean setDefaultAdapter(BluetoothAdapter adapter);
    public native boolean startDiscovery();
    public native boolean stopDiscovery();

    private native void init();
    private native void delete();
    private BluetoothManager()
    {
        init();
    }
    public static synchronized BluetoothManager getBluetoothManager()
    {
        if (inst == null)
        {
            inst = new BluetoothManager();
            inst.init();
        }
        return inst;
    }

    protected void finalize()
    {
        delete();
    }
}
