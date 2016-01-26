import tinyb.*;
import java.util.*;

public class Hellotinyb {
    private static final float SCALE_LSB = 0.03125f;

    public static void main(String[] args) throws InterruptedException {
        BluetoothManager manager = BluetoothManager.getBluetoothManager();

        if (args.length < 1) {
            System.err.println("Run with <device_address> argument");
            System.exit(-1);
        }

        boolean discoveryStarted = manager.startDiscovery();

        System.out.println("The discovery started: " + (discoveryStarted ? "true" : "false"));
        BluetoothDevice sensor = null;
        for (int i = 0; i < 15; ++i) {
            List<BluetoothDevice> list = manager.getDevices();

            for (BluetoothDevice device : list) {
                System.out.println("Address = " + device.getAddress());
                System.out.println("Name = " + device.getName());
                System.out.println("Connected = " + device.getConnected());

                if (device.getAddress().equals(args[0])) {
                    sensor = device;
                }
            }
            if (sensor != null) {
                break;
            }
            System.out.println("");
            Thread.sleep(4000);
        }

        Thread.sleep(4000);
        manager.stopDiscovery();

        if (sensor == null) {
            System.out.println("No sensor found with the provided address.");
            System.exit(-1);
        }

        Thread.sleep(4000);
        sensor.connect();
        System.out.println("Sensor with the provided address connected");

        List<BluetoothGattService> bluetoothServices = null;

        do {
            Thread.sleep(4000);
            bluetoothServices = sensor.getServices();

            for (BluetoothGattService service : bluetoothServices) {
                System.out.println("UUID: " + service.getUuid());
            }
        } while (bluetoothServices.isEmpty());
    }
}
