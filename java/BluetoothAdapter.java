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
