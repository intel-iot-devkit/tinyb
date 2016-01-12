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
