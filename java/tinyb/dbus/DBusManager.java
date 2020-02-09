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

import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothObject;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothType;

public class DBusManager implements BluetoothManager
{
    private long nativeInstance;
    private static DBusManager inst;

    static {
        try {
            System.loadLibrary("tinyb");
            System.loadLibrary("javatinyb");
        } catch (final UnsatisfiedLinkError e) {
            System.err.println("Native code library failed to load.\n" + e);
        }
    }

    private native static String getNativeAPIVersion();

    public native BluetoothType getBluetoothType();

    private native DBusObject find(int type, String name, String identifier, BluetoothObject parent, long milliseconds);

    @Override
    public DBusObject find(final BluetoothType type, final String name, final String identifier, final BluetoothObject parent, final long timeoutMS) {
        return find(type.ordinal(), name, identifier, parent, timeoutMS);
    }

    @Override
    public DBusObject find(final BluetoothType type, final String name, final String identifier, final BluetoothObject parent) {
        return find(type, name, identifier, parent, 0);
    }

    @SuppressWarnings("unchecked")
    @Override
    public <T extends BluetoothObject>  T find(final String name, final String identifier, final BluetoothObject parent, final long timeoutMS) {
        return (T) find(DBusObject.class_type().ordinal(), name, identifier, parent, timeoutMS);
    }

    @SuppressWarnings("unchecked")
    @Override
    public <T extends BluetoothObject>  T find(final String name, final String identifier, final BluetoothObject parent) {
        return (T) find(name, identifier, parent, 0);
    }

    @Override
    public BluetoothObject getObject(final BluetoothType type, final String name,
                                final String identifier, final BluetoothObject parent) {
        return getObject(type.ordinal(), name, identifier, parent);
    }
    private native BluetoothObject getObject(int type, String name, String identifier, BluetoothObject parent);

    @Override
    public List<BluetoothObject> getObjects(final BluetoothType type, final String name,
                                    final String identifier, final BluetoothObject parent) {
        return getObjects(type.ordinal(), name, identifier, parent);
    }
    private native List<BluetoothObject> getObjects(int type, String name, String identifier, BluetoothObject parent);

    @Override
    public native List<BluetoothAdapter> getAdapters();

    @Override
    public native List<BluetoothDevice> getDevices();

    @Override
    public native List<BluetoothGattService> getServices();

    @Override
    public native boolean setDefaultAdapter(BluetoothAdapter adapter);

    @Override
    public native BluetoothAdapter getDefaultAdapter();

    @Override
    public native boolean startDiscovery() throws BluetoothException;

    @Override
    public native boolean stopDiscovery() throws BluetoothException;

    @Override
    public native boolean getDiscovering() throws BluetoothException;

    private native void init() throws BluetoothException;
    private native void delete();
    private DBusManager()
    {
        init();
    }

    /** Returns an instance of BluetoothManager, to be used instead of constructor.
      * @return An initialized BluetoothManager instance.
      */
    public static synchronized BluetoothManager getBluetoothManager() throws RuntimeException, BluetoothException
    {
        if (inst == null)
        {
            final String nativeAPIVersion = getNativeAPIVersion();
            final String APIVersion = DBusManager.class.getPackage().getSpecificationVersion();
            if ( null != APIVersion && APIVersion.equals(nativeAPIVersion) == false) {
                final String[] nativeAPIVersionCode = nativeAPIVersion.split("\\D");
                final String[] APIVersionCode = APIVersion.split("\\D");
                if (APIVersionCode[0].equals(nativeAPIVersionCode[0]) == false) {
                    if (Integer.valueOf(APIVersionCode[0]) < Integer.valueOf(nativeAPIVersionCode[0]))
                        throw new RuntimeException("Java library is out of date. Please update the Java library.");
                    else throw new RuntimeException("Native library is out of date. Please update the native library.");
                }
                else if (APIVersionCode[0].equals("0") == true) {
                    if (Integer.valueOf(APIVersionCode[1]) < Integer.valueOf(nativeAPIVersionCode[1]))
                        throw new RuntimeException("Java library is out of date. Please update the Java library.");
                    else throw new RuntimeException("Native library is out of date. Please update the native library.");
                }
                else if (Integer.valueOf(APIVersionCode[1]) < Integer.valueOf(nativeAPIVersionCode[1]))
                    System.err.println("Java library is out of date. Please update the Java library.");
                else System.err.println("Native library is out of date. Please update the native library.");
            }
            inst = new DBusManager();
            inst.init();
        }
        return inst;
    }

    @Override
    protected void finalize()
    {
        delete();
    }
}
