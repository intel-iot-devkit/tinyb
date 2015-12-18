import java.util.*;

public class BluetoothGattService extends BluetoothObject
{
    private long nativeInstance;

    public native BluetoothType getBluetoothType();
    public native BluetoothAdapter clone();
    public native String getUuid();
    public native BluetoothDevice getDevice();
    public native boolean getPrimary();
    public native List<BluetoothGattCharacteristic> getCharacteristics();

    private native void delete();

    private BluetoothGattService(long instance)
    {
        super(instance);
        nativeInstance = instance;
    }

    protected void finalize()
    {
        delete();
    }
}
