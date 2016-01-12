import java.util.*;

public class BluetoothObject
{
    long nativeInstance;

    public native BluetoothType getBluetoothType();
    public native BluetoothObject clone();

    private native void delete();
    private native boolean operatorEqual(BluetoothObject obj);

    protected BluetoothObject(long instance)
    {
        nativeInstance = instance;
    }

    protected void finalize()
    {
        delete();
    }

    public boolean equals(BluetoothObject obj)
    {
        if (obj == null)
        {
            return false;
        }
        return operatorEqual(obj);
    }
}
