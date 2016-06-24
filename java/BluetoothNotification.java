package tinyb;

public interface BluetoothNotification<T> {
    public void run(T value);
}
