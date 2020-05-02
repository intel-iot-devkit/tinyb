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

import java.util.ArrayList;
import java.util.List;

import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothObject;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothType;

public class DBTManager implements BluetoothManager
{
    private long nativeInstance;
    private static DBTManager inst;
    private final List<BluetoothAdapter> adapters = new ArrayList<BluetoothAdapter>();

    public BluetoothType getBluetoothType() { return BluetoothType.NONE; }

    private DBTObject find(final int type, final String name, final String identifier, final BluetoothObject parent, final long milliseconds)
    { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public DBTObject find(final BluetoothType type, final String name, final String identifier, final BluetoothObject parent, final long timeoutMS) {
        return find(type.ordinal(), name, identifier, parent, timeoutMS);
    }

    @Override
    public DBTObject find(final BluetoothType type, final String name, final String identifier, final BluetoothObject parent) {
        return find(type, name, identifier, parent, 0);
    }

    @SuppressWarnings("unchecked")
    @Override
    public <T extends BluetoothObject>  T find(final String name, final String identifier, final BluetoothObject parent, final long timeoutMS) {
        return (T) find(DBTObject.class_type().ordinal(), name, identifier, parent, timeoutMS);
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
    private BluetoothObject getObject(final int type, final String name, final String identifier, final BluetoothObject parent)
    { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public List<BluetoothObject> getObjects(final BluetoothType type, final String name,
                                    final String identifier, final BluetoothObject parent) {
        return getObjects(type.ordinal(), name, identifier, parent);
    }
    private List<BluetoothObject> getObjects(final int type, final String name, final String identifier, final BluetoothObject parent)
    { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public List<BluetoothAdapter> getAdapters() { return adapters; }

    @Override
    public List<BluetoothDevice> getDevices() { return getDefaultAdapter().getDevices(); }

    @Override
    public List<BluetoothGattService> getServices() { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public boolean setDefaultAdapter(final BluetoothAdapter adapter) { throw new UnsupportedOperationException(); } // FIXME

    @Override
    public BluetoothAdapter getDefaultAdapter() { return adapters.get(0); }

    @Override
    public boolean startDiscovery() throws BluetoothException { return getDefaultAdapter().startDiscovery(); }

    @Override
    public boolean stopDiscovery() throws BluetoothException { return getDefaultAdapter().stopDiscovery(); }

    @Override
    public boolean getDiscovering() throws BluetoothException { return getDefaultAdapter().getDiscovering(); }

    /**
     * Returns an opened default adapter instance!
     * @throws BluetoothException in case adapter is invalid or could not have been opened.
     */
    private native DBTAdapter getDefaultAdapterImpl() throws BluetoothException;
    private native List<BluetoothAdapter> getAdapterListImpl();

    private native void initImpl() throws BluetoothException;
    private native void deleteImpl();
    private DBTManager()
    {
        initImpl();
        try {
            adapters.add(getDefaultAdapterImpl());
        } catch (final BluetoothException be) {
            be.printStackTrace();
        }
    }

    /** Returns an instance of BluetoothManager, to be used instead of constructor.
      * @return An initialized BluetoothManager instance.
      */
    public static synchronized BluetoothManager getBluetoothManager() throws RuntimeException, BluetoothException
    {
        if (inst == null)
        {
            inst = new DBTManager();
        }
        return inst;
    }

    @Override
    protected void finalize() {
        shutdown();
    }

    @Override
    public void shutdown() {
        adapters.clear();
        deleteImpl();
    }

}
