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

public class BluetoothAdapter extends BluetoothObject
{
    private long nativeInstance;

    public native BluetoothType getBluetoothType();
    public native BluetoothAdapter clone();
    public native boolean startDiscovery();
    public native boolean stopDiscovery();
    public native List<BluetoothDevice> getDevices();
    public native String getAddress();
    public native String getName();
    public native String getAlias();
    public native void setAlias(String value);
    public native long getBluetoothClass();
    public native boolean getPowered();
    public native void setPowered(boolean value);
    public native boolean getDiscoverable();
    public native void setDiscoverable(boolean value);
    public native long getDiscoverableTimeout();
    public native void setDiscoverableTimout(long value);
    public native boolean getPairable();
    public native void setPairable(boolean value);
    public native long getPairableTimeout();
    public native void setPairableTimeout(long value);
    public native boolean getDiscovering();
    public native String[] getUuids();
    public native String getModalias();

    private native void delete();

    private BluetoothAdapter(long instance)
    {
        super(instance);
        nativeInstance = instance;
    }

    protected void finalize()
    {
        delete();
    }
}
