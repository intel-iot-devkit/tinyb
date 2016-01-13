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

public class BluetoothDevice extends BluetoothObject
{
    private long nativeInstance;

    public native BluetoothType getBluetoothType();
    public native BluetoothDevice clone();
    public native boolean disconnect();
    public native boolean connect();
    public native boolean connectProfile(String arg_UUID);
    public native boolean disconnectProfile(String arg_UUID);
    public native boolean pair();
    public native boolean cancelPairing();
    public native List<BluetoothGattService> getServices();
    public native String getAddress();
    public native String getName();
    public native String getAlias();
    public native void setAlias(String value);
    public native int getBluetoothClass();
    public native short getAppearance();
    public native String getIcon();
    public native boolean getPaired();
    public native boolean getTrusted();
    public native void setTrusted(boolean value);
    public native boolean getBlocked();
    public native void setBlocked(boolean value);
    public native boolean getLegacyPairing();
    public native short getRssi();
    public native boolean getConnected();
    public native String[] getUuids();
    public native String getModalias();
    public native BluetoothAdapter getAdapter();

    private native void delete();
    /*public BluetoothDevice()
    {
        System.out.println("mda");
    }*/
    private BluetoothDevice(long instance)
    {
        super(instance);
        nativeInstance = instance;
    }

    protected void finalize()
    {
        delete();
    }
}
