import java.util.*;

public class BluetoothGattDescriptor extends BluetoothObject
{
    public long nativeInstance;

    public native BluetoothType getBluetoothType();
    public native BluetoothAdapter clone();

    public native byte[] readValue();
    public native boolean writeValue(byte[] argValue);
    public native String getUuid();
    public native BluetoothGattCharacteristic getCharacteristic();
    public native byte[] getValue();

    private native void delete();

    private BluetoothGattDescriptor(long instance)
    {
        super(instance);
        nativeInstance = instance;
    }

    protected void finalize()
    {
        delete();
    }
}
