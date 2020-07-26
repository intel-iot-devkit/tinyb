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

import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothFactory;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattDescriptor;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothObject;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothType;

public class DBTManager implements BluetoothManager
{
    protected static final boolean DEBUG = BluetoothFactory.DEBUG;
    protected static final boolean VERBOSE = BluetoothFactory.VERBOSE;

    private static volatile boolean isJVMShuttingDown = false;
    private static final List<Runnable> userShutdownHooks = new ArrayList<Runnable>();
    private static boolean unifyUUID128Bit = true;
    private static int DefaultAdapterIndex = 0;

    static {
        AccessController.doPrivileged(new PrivilegedAction<Object>() {
            @Override
            public Object run() {
                Runtime.getRuntime().addShutdownHook(
                    new Thread(null, new Runnable() {
                                @Override
                                public void run() {
                                    DBTManager.shutdownImpl(true);
                                } }, "DBTManager_ShutdownHook" ) ) ;
                return null;
            } } ) ;
        {
            final String v = System.getProperty("org.tinyb.default_adapter", "0");
            DefaultAdapterIndex = Integer.valueOf(v);
        }

    }

    private static synchronized void shutdownImpl(final boolean _isJVMShuttingDown) {
        isJVMShuttingDown = _isJVMShuttingDown;
        if(DEBUG) {
            System.err.println("DBTManager.shutdown() START: JVM Shutdown "+isJVMShuttingDown+", on thread "+Thread.currentThread().getName());
        }
        synchronized(userShutdownHooks) {
            final int cshCount = userShutdownHooks.size();
            for(int i=0; i < cshCount; i++) {
                try {
                    if( DEBUG ) {
                        System.err.println("DBTManager.shutdown - userShutdownHook #"+(i+1)+"/"+cshCount);
                    }
                    userShutdownHooks.get(i).run();
                } catch(final Throwable t) {
                    System.err.println("DBTManager.shutdown: Caught "+t.getClass().getName()+" during userShutdownHook #"+(i+1)+"/"+cshCount);
                    if( DEBUG ) {
                        t.printStackTrace();
                    }
                }
            }
            userShutdownHooks.clear();
        }
        if(DEBUG) {
            System.err.println("DBTManager.shutdown(): Post userShutdownHook");
        }

        try {
            final BluetoothManager mgmt = getBluetoothManager();
            mgmt.shutdown();
        } catch(final Throwable t) {
            System.err.println("DBTManager.shutdown: Caught "+t.getClass().getName()+" during DBTManager.shutdown()");
            if( DEBUG ) {
                t.printStackTrace();
            }
        }

        if(DEBUG) {
            System.err.println(Thread.currentThread().getName()+" - DBTManager.shutdown() END JVM Shutdown "+isJVMShuttingDown);
        }
    }

    /** Returns true if the JVM is shutting down, otherwise false. */
    public static final boolean isJVMShuttingDown() { return isJVMShuttingDown; }

    /**
     * Add a shutdown hook to be performed at JVM shutdown before shutting down DBTManager instance.
     *
     * @param head if true add runnable at the start, otherwise at the end
     * @param runnable runnable to be added.
     */
    public static void addShutdownHook(final boolean head, final Runnable runnable) {
        synchronized( userShutdownHooks ) {
            if( !userShutdownHooks.contains( runnable ) ) {
                if( head ) {
                    userShutdownHooks.add(0, runnable);
                } else {
                    userShutdownHooks.add( runnable );
                }
            }
        }
    }

    /**
     * Returns whether uuid128_t consolidation is enabled
     * for native uuid16_t and uuid32_t values before string conversion.
     * @see #setUnifyUUID128Bit(boolean)
     */
    public static boolean getUnifyUUID128Bit() { return unifyUUID128Bit; }

    /**
     * Enables or disables uuid128_t consolidation
     * for native uuid16_t and uuid32_t values before string conversion.
     * <p>
     * Default is {@code true}, as this represent compatibility with original TinyB D-Bus behavior.
     * </p>
     * <p>
     * If desired, this value should be set once before the first call of {@link #getBluetoothManager()}!
     * </p>
     * @see #getUnifyUUID128Bit()
     */
    public static void setUnifyUUID128Bit(final boolean v) { unifyUUID128Bit=v; }

    private long nativeInstance;
    private static DBTManager inst;
    private final List<BluetoothAdapter> adapters = new ArrayList<BluetoothAdapter>();
    private int defaultAdapterIndex = 0;

    public BluetoothType getBluetoothType() { return BluetoothType.NONE; }

    @Override
    public DBTObject find(final BluetoothType type, final String name, final String identifier, final BluetoothObject parent, final long timeoutMS) {
        return findInCache((DBTObject)parent, type, name, identifier);
    }

    @Override
    public DBTObject find(final BluetoothType type, final String name, final String identifier, final BluetoothObject parent) {
        return find(type, name, identifier, parent, 0);
    }

    @SuppressWarnings("unchecked")
    @Override
    public <T extends BluetoothObject>  T find(final String name, final String identifier, final BluetoothObject parent, final long timeoutMS) {
        // Due to generic type erasure, we cannot determine the matching BluetoothType for the return parameter,
        // hence this orig TinyB API method is rather misleading than useful.
        throw new UnsupportedOperationException("Generic return type 'find' won't be implemented.");
        // return (T) find(BluetoothType.NONE, name, identifier, parent, timeoutMS);
    }

    @SuppressWarnings("unchecked")
    @Override
    public <T extends BluetoothObject>  T find(final String name, final String identifier, final BluetoothObject parent) {
        // Due to generic type erasure, we cannot determine the matching BluetoothType for the return parameter,
        // hence this orig TinyB API method is rather misleading than useful.
        throw new UnsupportedOperationException("Generic return type 'find' won't be implemented.");
        // return (T) find(BluetoothType.NONE, name, identifier, parent, 0);
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

    /**
     * {@inheritDoc}
     * <p>
     * This call is a quite expensive service query, see below.
     * </p>
     * <p>
     * This implementation only fetches all {@link BluetoothGattService} from all {@link BluetoothDevice}s
     * from the {@link #getDefaultAdapter()}.
     * </p>
     * <p>
     * This implementation does not {@link BluetoothAdapter#startDiscovery() start} an explicit discovery,
     * but previous {@link BluetoothAdapter#getDevices() discovered devices} are being queried.
     * </p>
     * <p>
     * In case a {@link BluetoothDevice} hasn't detected any service yet,
     * i.e. {@link BluetoothDevice#getServicesResolved()} and {@link BluetoothDevice#getConnected()} is false,
     * a new {@link BluetoothDevice#connect() connect/disconnect} cycle is performed.
     * </p>
     */
    @Override
    public List<BluetoothGattService> getServices() {
        final List<BluetoothGattService> res = new ArrayList<BluetoothGattService>();
        for(final Iterator<BluetoothAdapter> iterA=adapters.iterator(); iterA.hasNext(); ) {
            final BluetoothAdapter adapter = iterA.next();
            final List<BluetoothDevice> devices = adapter.getDevices();
            for(final Iterator<BluetoothDevice> iterD=devices.iterator(); iterD.hasNext(); ) {
                final BluetoothDevice device = iterD.next();
                if( !device.getServicesResolved() && !device.getConnected() ) {
                    if( device.connect() ) {
                        if( !device.getServicesResolved() ) {
                            throw new InternalError("Device connected but services not resolved");
                        }
                        device.disconnect();
                    }
                }
                final List<BluetoothGattService> devServices = device.getServices();
                res.addAll(devServices);
            }
        }
        return res;
    }

    @Override
    public boolean setDefaultAdapter(final BluetoothAdapter adapter) {
        final int idx = adapters.indexOf(adapter);
        if( 0 <= idx ) {
            defaultAdapterIndex = idx;
            return true;
        }
        return false;
    }


    @Override
    public BluetoothAdapter getDefaultAdapter() { return adapters.get(defaultAdapterIndex); }

    @Override
    public boolean startDiscovery() throws BluetoothException { return startDiscovery(true); }

    @Override
    public boolean startDiscovery(final boolean keepAlive) throws BluetoothException { return getDefaultAdapter().startDiscovery(keepAlive); }

    @Override
    public boolean stopDiscovery() throws BluetoothException { return getDefaultAdapter().stopDiscovery(); }

    @Override
    public boolean getDiscovering() throws BluetoothException { return getDefaultAdapter().getDiscovering(); }

    private native List<BluetoothAdapter> getAdapterListImpl();

    private native void initImpl(final boolean unifyUUID128Bit) throws BluetoothException;
    private native void deleteImpl(long nativeInstance);
    private DBTManager()
    {
        initImpl(unifyUUID128Bit);
        try {
            adapters.addAll(getAdapterListImpl());
        } catch (final BluetoothException be) {
            be.printStackTrace();
        }
        if( adapters.size() >= DefaultAdapterIndex+1 ) {
            defaultAdapterIndex = DefaultAdapterIndex;
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
        for(final Iterator<BluetoothAdapter> ia= adapters.iterator(); ia.hasNext(); ) {
            final DBTAdapter a = (DBTAdapter)ia.next();
            a.close();
        }
        adapters.clear();
        deleteImpl(nativeInstance);
    }

    /**
     * Returns the matching {@link DBTObject} from the internal cache if found,
     * otherwise {@code null}.
     * <p>
     * The returned {@link DBTObject} may be of type
     * <ul>
     *   <li>{@link DBTAdapter}</li>
     *   <li>{@link DBTDevice}</li>
     *   <li>{@link DBTGattService}</li>
     *   <li>{@link DBTGattCharacteristic}</li>
     *   <li>{@link DBTGattDescriptor}</li>
     * </ul>
     * or alternatively in {@link BluetoothObject} space
     * <ul>
     *   <li>{@link BluetoothType#ADAPTER} -> {@link BluetoothAdapter}</li>
     *   <li>{@link BluetoothType#DEVICE} -> {@link BluetoothDevice}</li>
     *   <li>{@link BluetoothType#GATT_SERVICE} -> {@link BluetoothGattService}</li>
     *   <li>{@link BluetoothType#GATT_CHARACTERISTIC} -> {@link BluetoothGattCharacteristic}</li>
     *   <li>{@link BluetoothType#GATT_DESCRIPTOR} -> {@link BluetoothGattDescriptor}</li>
     * </ul>
     * </p>
     * @param name name of the desired {@link BluetoothType#ADAPTER adapter} or {@link BluetoothType#DEVICE device}.
     * Maybe {@code null}.
     * @param identifier EUI48 address of the desired {@link BluetoothType#ADAPTER adapter} or {@link BluetoothType#DEVICE device}
     * or UUID of the desired {@link BluetoothType#GATT_SERVICE service},
     * {@link BluetoothType#GATT_CHARACTERISTIC characteristic} or {@link BluetoothType#GATT_DESCRIPTOR descriptor} to be found.
     * Maybe {@code null}, in which case the first object of the desired type is being returned - if existing.
     * @param type specify the type of the object to be found, either
     * {@link BluetoothType#ADAPTER adapter}, {@link BluetoothType#DEVICE device},
     * {@link BluetoothType#GATT_SERVICE service}, {@link BluetoothType#GATT_CHARACTERISTIC characteristic}
     * or {@link BluetoothType#GATT_DESCRIPTOR descriptor}.
     * {@link BluetoothType#NONE none} means anything.
     */
    /* pp */ DBTObject findInCache(final String name, final String identifier, final BluetoothType type) {
        final boolean anyType = BluetoothType.NONE == type;
        final boolean adapterType = BluetoothType.ADAPTER == type;

        if( null == name && null == identifier && ( anyType || adapterType ) ) {
            // special case for 1st valid adapter
            if( adapters.size() > 0 ) {
                return (DBTAdapter) adapters.get(0);
            }
            return null; // no adapter
        }
        for(int devIdx = adapters.size() - 1; devIdx >= 0; devIdx-- ) {
            final DBTAdapter adapter = (DBTAdapter) adapters.get(devIdx);
            if( ( anyType || adapterType ) ) {
                if( null != name && null != identifier &&
                    adapter.getName().equals(name) &&
                    adapter.getAddress().equals(identifier)
                  )
                {
                    return adapter;
                }
                if( null != identifier &&
                    adapter.getAddress().equals(identifier)
                  )
                {
                    return adapter;
                }
                if( null != name &&
                    adapter.getName().equals(name)
                  )
                {
                    return adapter;
                }
            }
            final DBTObject dbtObj = adapter.findInCache(name, identifier, type);
            if( null != dbtObj ) {
                return dbtObj;
            }
        }
        return null;
    }

    /* pp */ DBTObject findInCache(final DBTObject parent, final BluetoothType type, final String name, final String identifier) {
        if( null == parent ) {
            return findInCache(name, identifier, type);
        }
        final boolean anyType = BluetoothType.NONE == type;
        final boolean deviceType = BluetoothType.DEVICE == type;
        final boolean serviceType = BluetoothType.GATT_SERVICE == type;
        final boolean charType = BluetoothType.GATT_CHARACTERISTIC== type;
        final boolean descType = BluetoothType.GATT_DESCRIPTOR == type;

        final BluetoothType parentType = parent.getBluetoothType();

        if( BluetoothType.ADAPTER == parentType &&
            ( anyType || deviceType || serviceType || charType || descType )
          )
        {
            return ((DBTAdapter) parent).findInCache(name, identifier, type);
        }
        if( BluetoothType.DEVICE == parentType &&
            ( anyType || serviceType || charType || descType )
          )
        {
            return ((DBTDevice) parent).findInCache(identifier, type);
        }
        if( BluetoothType.GATT_SERVICE == parentType &&
            ( anyType || charType || descType )
          )
        {
            final DBTGattService service = (DBTGattService) parent;
            if( !service.checkServiceCache() ) {
                return null;
            }
            return service.findInCache(identifier, type);
        }
        if( BluetoothType.GATT_CHARACTERISTIC == parentType &&
            ( anyType || descType )
          )
        {
            final DBTGattCharacteristic characteristic = (DBTGattCharacteristic) parent;
            if( !characteristic.checkServiceCache() ) {
                return null;
            }
            return characteristic.findInCache(identifier, type);
        }
        return null;
    }

}
