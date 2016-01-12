public class ObjectArrayArgCallback extends BluetoothCallback
{
    private Object[] callbackArg;

    public ObjectArrayArgCallback(BluetoothObject bObj, Object[] callbackArg)
    {
        this.bObj = bObj;
        this.callbackArg = callbackArg;
    }

    public void run()
    {
    }
}
