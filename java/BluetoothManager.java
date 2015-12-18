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
