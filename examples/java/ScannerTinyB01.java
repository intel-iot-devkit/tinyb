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

import java.lang.reflect.InvocationTargetException;
import java.util.Iterator;
import java.util.List;

import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothDevice;
import org.tinyb.BluetoothDeviceDiscoveryListener;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothFactory;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattDescriptor;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothUtils;

public class ScannerTinyB01 {
    static {
        System.setProperty("org.tinyb.verbose", "true");
    }
    /** 60,000 milliseconds */
    static long TO_DISCOVER = 60000;

    public static void main(final String[] args) throws InterruptedException {
        int factory = 0;
        int mode = 0;
        boolean forever = false;
        final String mac;
        {
            String _mac = null;
            for(int i=0; i< args.length; i++) {
                final String arg = args[i];

                if( arg.equals("-mac") ) {
                    _mac = args[++i];
                } else if( arg.equals("-mode") ) {
                    mode = Integer.valueOf(args[++i]).intValue();
                } else if( arg.equals("-factory") ) {
                    factory = Integer.valueOf(args[++i]).intValue();
                } else if( arg.equals("-forever") ) {
                    forever = true;
                }
            }

            if ( null == _mac ) {
                System.err.println("Run with '-mac <device_address> [-mode <mode>] [-factory <BluetoothManager-Factory-Implementation-Class>]'");
                System.exit(-1);
            }
            mac = _mac;
        }

        final BluetoothFactory.ImplementationIdentifier implID = 0 == factory ? BluetoothFactory.DirectBTImplementationID : BluetoothFactory.DBusImplementationID;
        final BluetoothManager manager;
        {
            BluetoothManager _manager = null;
            try {
                _manager = BluetoothFactory.getBluetoothManager( implID );
            } catch (BluetoothException | NoSuchMethodException | SecurityException
                    | IllegalAccessException | IllegalArgumentException
                    | InvocationTargetException | ClassNotFoundException e) {
                System.err.println("Unable to instantiate BluetoothManager via "+implID);
                e.printStackTrace();
                System.exit(-1);
            }
            manager = _manager;
        }
        final BluetoothAdapter adapter = manager.getDefaultAdapter();
        final BluetoothDevice[] matchingDiscoveredDeviceBucket = { null };

        final BluetoothDeviceDiscoveryListener deviceDiscListener = new BluetoothDeviceDiscoveryListener() {
            @Override
            public void deviceAdded(final BluetoothAdapter adapter, final BluetoothDevice device) {
                final boolean matches = device.getAddress().equals(mac);
                System.err.println("****** ADDED__: "+device.toString()+" - match "+matches);
                System.err.println("Status HCIAdapter:");
                System.err.println(adapter.toString());

                if( matches ) {
                    synchronized(matchingDiscoveredDeviceBucket) {
                        matchingDiscoveredDeviceBucket[0] = device;
                        matchingDiscoveredDeviceBucket.notifyAll();
                    }
                }
            }

            @Override
            public void deviceUpdated(final BluetoothAdapter adapter, final BluetoothDevice device) {
                final boolean matches = device.getAddress().equals(mac);
                System.err.println("****** UPDATED: "+device.toString()+" - match "+matches);
                System.err.println("Status HCIAdapter:");
                System.err.println(adapter.toString());
            }

            @Override
            public void deviceRemoved(final BluetoothAdapter adapter, final BluetoothDevice device) {
                final boolean matches = device.getAddress().equals(mac);
                System.err.println("****** REMOVED: "+device.toString()+" - match "+matches);
                System.err.println("Status HCIAdapter:");
                System.err.println(adapter.toString());
            }
        };
        adapter.removeDevices();
        adapter.setDeviceDiscoveryListener(deviceDiscListener);

        do {
            final long t0 = BluetoothUtils.getCurrentMilliseconds();

            final boolean discoveryStarted = adapter.startDiscovery();

            System.err.println("The discovery started: " + (discoveryStarted ? "true" : "false") + " for mac "+mac+", mode "+mode);
            if( !discoveryStarted ) {
                break;
            }
            BluetoothDevice sensor = null;

            if( 0 == mode ) {
                synchronized(matchingDiscoveredDeviceBucket) {
                    while( null == matchingDiscoveredDeviceBucket[0] ) {
                        matchingDiscoveredDeviceBucket.wait(TO_DISCOVER);
                    }
                    sensor = matchingDiscoveredDeviceBucket[0];
                }
            } else if( 1 == mode ) {
                sensor = adapter.find(null, mac, TO_DISCOVER);
            } else {
                boolean timeout = false;
                while( null == sensor && !timeout ) {
                    final List<BluetoothDevice> devices = adapter.getDevices();
                    for(final Iterator<BluetoothDevice> id = devices.iterator(); id.hasNext() && !timeout; ) {
                        final BluetoothDevice d = id.next();
                        if(d.getAddress().equals(mac)) {
                            sensor = d;
                            break;
                        }
                        final long tn = BluetoothUtils.getCurrentMilliseconds();
                        timeout = ( tn - t0 ) > TO_DISCOVER;
                    }
                }
            }
            final long t1 = BluetoothUtils.getCurrentMilliseconds();
            if (sensor == null) {
                System.err.println("No sensor found within "+(t1-t0)+" ms");
                System.exit(-1);
            }
            System.err.println("Found device in "+(t1-t0)+" ms: ");
            printDevice(sensor);
            adapter.stopDiscovery(); // FIXME ????

            final BooleanNotification connectedNotification = new BooleanNotification("Connected", t1);
            final BooleanNotification servicesResolvedNotification = new BooleanNotification("ServicesResolved", t1);
            sensor.enableConnectedNotifications(connectedNotification);
            sensor.enableServicesResolvedNotifications(servicesResolvedNotification);

            final long t2 = BluetoothUtils.getCurrentMilliseconds();
            final long t3;
            if ( sensor.connect() ) {
                t3 = BluetoothUtils.getCurrentMilliseconds();
                System.err.println("Sensor connected: "+(t3-t2)+" ms, total "+(t3-t0)+" ms");
                System.err.println("Sensor connectedNotification: "+connectedNotification.getValue());
            } else {
                t3 = BluetoothUtils.getCurrentMilliseconds();
                System.out.println("Could not connect device: "+(t3-t2)+" ms, total "+(t3-t0)+" ms");
                System.exit(-1);
            }

            if( false ) {
            synchronized( servicesResolvedNotification ) {
                while( !servicesResolvedNotification.getValue() ) {
                    final long tn = BluetoothUtils.getCurrentMilliseconds();
                    if( tn - t3 > 20000 ) {
                        break; // 20s TO
                    }
                    servicesResolvedNotification.wait();
                }
            }
            final long t4;
            if ( servicesResolvedNotification.getValue() ) {
                t4 = BluetoothUtils.getCurrentMilliseconds();
                System.err.println("Sensor servicesResolved: "+(t4-t3)+" ms, total "+(t4-t0)+" ms");
            } else {
                t4 = BluetoothUtils.getCurrentMilliseconds();
                System.out.println("Could not connect device: "+(t4-t3)+" ms, total "+(t4-t0)+" ms");
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
            }

            sensor.disconnect();
            System.err.println("ScannerTinyB01 04 ...: "+adapter);
        } while( forever );
        System.err.println("ScannerTinyB01 01 stopDiscovery: "+adapter);
        adapter.stopDiscovery();
        System.err.println("ScannerTinyB01 02 close: "+adapter);
        adapter.close();
        System.err.println("ScannerTinyB01 05");
        manager.shutdown();
        System.err.println("ScannerTinyB01 XX");
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
                final long t1 = BluetoothUtils.getCurrentMilliseconds();
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
