import java.util.*;

public class BluetoothEvent
{
    private long nativeInstance;

    public native BluetoothType getType();
    public native String getName();
    public native String getIdentifier();
    public native boolean executeCallback();
    public native boolean hasCallback();

    private native void init(BluetoothType type, String name, String identifier,
                            BluetoothObject parent, BluetoothCallback cb, Object data);
    private native void delete();

    public BluetoothEvent(BluetoothType type, String name, String identifier,
                            BluetoothObject parent, BluetoothCallback cb, Object data)
    {
        init(type, name, identifier, parent, cb, data);
    }

    protected void finalize()
    {
        delete();
    }
}
