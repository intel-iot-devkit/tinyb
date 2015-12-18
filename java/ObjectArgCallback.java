public class ObjectArgCallback extends BluetoothCallback
{
    private Object callbackArg;

    public ObjectArgCallback(BluetoothObject bObj, Object callbackArg)
    {
        this.bObj = bObj;
        this.callbackArg = callbackArg;
    }

    public void run()
    {
    }
}
