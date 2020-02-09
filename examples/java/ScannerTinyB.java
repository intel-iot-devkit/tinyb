/**
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

import java.util.Iterator;
import java.util.List;

import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothFactory;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattDescriptor;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;

public class ScannerTinyB {
    /** 60,000 milliseconds */
    static long TO_DISCOVER = 60000;

    public static void main(final String[] args) throws InterruptedException {
        String mac = null;
        int mode = 0;

        for(int i=0; i< args.length; i++) {
            final String arg = args[i];

            if( arg.equals("-mac") ) {
                mac = args[++i];
            } else if( arg.equals("-mode") ) {
                mode = Integer.valueOf(args[++i]).intValue();
            }
        }

        if ( null == mac ) {
            System.err.println("Run with '-mac <device_address>' argument");
            System.exit(-1);
        }

        final boolean useAdapter = mode/10 > 0;
        mode = mode %10;

        /*
         * To start looking of the device, we first must initialize the TinyB library. The way of interacting with the
         * library is through the BluetoothManager. There can be only one BluetoothManager at one time, and the
         * reference to it is obtained through the getBluetoothManager method.
         */
        final BluetoothManager manager = BluetoothFactory.getDBusBluetoothManager();
        final BluetoothAdapter adapter = manager.getDefaultAdapter();

        final long t0 = System.currentTimeMillis();;

        if( useAdapter ) {
            adapter.removeDevices();
        }
        final boolean discoveryStarted = useAdapter ? adapter.startDiscovery() : manager.startDiscovery();

        System.err.println("The discovery started: " + (discoveryStarted ? "true" : "false") + " for mac "+mac+", mode "+mode+", useAdapter "+useAdapter);
        BluetoothDevice sensor = null;

        if( 0 == mode ) {
            if( useAdapter ) {
                sensor = adapter.find(null, mac, TO_DISCOVER);
            } else {
                sensor = manager.find(null, mac, null, TO_DISCOVER);
            }
        } else {
            boolean timeout = false;
            while( null == sensor && !timeout ) {
                final List<BluetoothDevice> devices = useAdapter ? adapter.getDevices() : manager.getDevices();
                for(final Iterator<BluetoothDevice> id = devices.iterator(); id.hasNext() && !timeout; ) {
                    final BluetoothDevice d = id.next();
                    if(d.getAddress().equals(mac)) {
                        sensor = d;
                        break;
                    }
                    final long tn = System.currentTimeMillis();
                    timeout = ( tn - t0 ) > TO_DISCOVER;
                }
            }
        }
        final long t1 = System.currentTimeMillis();
        if (sensor == null) {
            System.err.println("No sensor found within "+(t1-t0)+" ms");
            System.exit(-1);
        }
        System.err.println("Found device in "+(t1-t0)+" ms: ");
        printDevice(sensor);

        final BooleanNotification connectedNotification = new BooleanNotification("Connected", t1);
        final BooleanNotification servicesResolvedNotification = new BooleanNotification("ServicesResolved", t1);
        sensor.enableConnectedNotifications(connectedNotification);
        sensor.enableServicesResolvedNotifications(servicesResolvedNotification);

        final long t2;
        if ( sensor.connect() ) {
            t2 = System.currentTimeMillis();
            System.err.println("Sensor connected in "+(t2-t1)+" ms");
            System.err.println("Sensor connectedNotification: "+connectedNotification.getValue());
        } else {
            t2=0;
            System.out.println("Could not connect device.");
            System.exit(-1);
        }

        synchronized( servicesResolvedNotification ) {
            while( !servicesResolvedNotification.getValue() ) {
                final long tn = System.currentTimeMillis();
                if( tn - t2 > 20000 ) {
                    break; // 20s TO
                }
                servicesResolvedNotification.wait();
            }
        }
        final long t3;
        if ( servicesResolvedNotification.getValue() ) {
            t3 = System.currentTimeMillis();
            System.err.println("Sensor servicesResolved in "+(t3-t2)+" ms, total "+(t3-t1)+" ms");
        } else {
            t3=0;
            System.out.println("Could not connect device.");
            System.exit(-1);
        }
        // Will shut down everything .. ??
        //adapter.stopDiscovery();

        final List<BluetoothGattService> allBluetoothServices = sensor.getServices();
        if (allBluetoothServices.isEmpty()) {
            System.err.println("No BluetoothGattService found!");
            System.exit(1);
        }
        printAllServiceInfo(allBluetoothServices);

        sensor.disconnect();
    }
    private static void printDevice(final BluetoothDevice device) {
        System.err.println("Address = " + device.getAddress());
        System.err.println("  Name = " + device.getName());
        System.err.println("  Connected = " + device.getConnected());
        System.err.println();
    }
    private static void printAllServiceInfo(final List<BluetoothGattService> allBluetoothServices) {
        try {
            for (final BluetoothGattService service : allBluetoothServices) {
                System.err.println("Service UUID: " + service.getUUID());
                final List<BluetoothGattCharacteristic> v = service.getCharacteristics();
                for (final BluetoothGattCharacteristic c : v) {
                    System.err.println("    Characteristic UUID: " + c.getUUID());

                    final List<BluetoothGattDescriptor> descriptors = c.getDescriptors();

                    for (final BluetoothGattDescriptor d : descriptors) {
                        System.err.println("        Descriptor UUID: " + d.getUUID());
                    }
                    if (c.getUUID().contains("2a29-")) {
                        final byte[] tempRaw = c.readValue();
                        System.err.println("    Manufacturer: " + new String(tempRaw));
                    }

                    if (c.getUUID().contains("2a28-")) {
                        final byte[] tempRaw = c.readValue();
                        System.err.println("    Software: " + new String(tempRaw));
                    }

                    if (c.getUUID().contains("2a27-")) {
                        final byte[] tempRaw = c.readValue();
                        System.err.println("    Hardware: " + new String(tempRaw));
                    }

                    if (c.getUUID().contains("2a26-")) {
                        final byte[] tempRaw = c.readValue();
                        System.err.println("    Firmware: " + new String(tempRaw));
                    }

                    if (c.getUUID().contains("2a25-")) {
                        final byte[] tempRaw = c.readValue();
                        System.err.println("    Serial: " + new String(tempRaw));
                    }

                    if (c.getUUID().contains("2a24-")) {
                        final byte[] tempRaw = c.readValue();
                        System.err.println("    Model: " + new String(tempRaw));
                    }

                    if (c.getUUID().contains("2a23-")) {
                        final byte[] tempRaw = c.readValue();
                        System.err.println("    System ID: " + bytesToHex(tempRaw));
                    }
                }
            }
        } catch (final RuntimeException e) {
        }
    }
    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();
    public static String bytesToHex(final byte[] bytes) {
        final char[] hexChars = new char[bytes.length * 2];
        for (int j = 0; j < bytes.length; j++) {
            final int v = bytes[j] & 0xFF;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[j * 2 + 1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars);
    }
    static class BooleanNotification implements BluetoothNotification<Boolean> {
        private final long t0;
        private final String name;
        private boolean v;

        public BooleanNotification(final String name, final long t0) {
            this.t0 = t0;
            this.name = name;
            this.v = false;
        }

        @Override
        public void run(final Boolean v) {
            synchronized(this) {
                final long t1 = System.currentTimeMillis();
                this.v = v.booleanValue();
                System.out.println("#### "+name+": "+v+" in td "+(t1-t0)+" ms!");
                this.notifyAll();
            }
        }
        public boolean getValue() {
            synchronized(this) {
                return v;
            }
        }
    }
}