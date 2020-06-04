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
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

import org.tinyb.AdapterSettings;
import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothDevice;
import org.tinyb.AdapterStatusListener;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothFactory;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothUtils;
import org.tinyb.EIRDataTypeSet;
import org.tinyb.GATTCharacteristicListener;
import org.tinyb.HCIStatusCode;

public class ScannerTinyB01 {
    static {
        System.setProperty("org.tinyb.verbose", "true");
    }
    /** 10,000 milliseconds */
    static long TO_DISCOVER = 10000;

    static final String EUI48_ANY_DEVICE = "00:00:00:00:00:00";
    static String waitForDevice = EUI48_ANY_DEVICE;

    public static void main(final String[] args) throws InterruptedException {
        final boolean waitForEnter=false;
        long t0_discovery = TO_DISCOVER;
        int factory = 0;
        int dev_id = 0; // default
        int mode = 0;
        boolean forever = false;
        {
            for(int i=0; i< args.length; i++) {
                final String arg = args[i];

                if( arg.equals("-dev_id") && args.length > (i+1) ) {
                    dev_id = Integer.valueOf(args[++i]).intValue();
                } else if( arg.equals("-mac") && args.length > (i+1) ) {
                    waitForDevice = args[++i];
                } else if( arg.equals("-mode") && args.length > (i+1) ) {
                    mode = Integer.valueOf(args[++i]).intValue();
                } else if( arg.equals("-factory") && args.length > (i+1) ) {
                    factory = Integer.valueOf(args[++i]).intValue();
                } else if( arg.equals("-t0_discovery") && args.length > (i+1) ) {
                    t0_discovery = Long.valueOf(args[++i]).longValue();
                } else if( arg.equals("-forever") ) {
                    forever = true;
                }
            }

            System.err.println("Run with '[-dev_id <adapter-index>] -mac <device_address> [-mode <mode>] [-factory <BluetoothManager-Factory-Implementation-Class>]'");
            if ( EUI48_ANY_DEVICE.equals(waitForDevice) ) {
                System.exit(-1);
            }
        }

        System.err.println("dev_id "+dev_id);
        System.err.println("waitForDevice: "+waitForDevice);

        if( waitForEnter ) {
            System.err.println("Press ENTER to continue\n");
            try{ System.in.read();
            } catch(final Exception e) { }
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
        final BluetoothAdapter adapter;
        {
            final List<BluetoothAdapter> adapters = manager.getAdapters();
            for(int i=0; i < adapters.size(); i++) {
                System.err.println("Adapter["+i+"]: "+adapters.get(i));
            }
            if( adapters.size() <= dev_id ) {
                System.err.println("No adapter dev_id "+dev_id+" available, adapter count "+adapters.size());
                System.exit(-1);
            }
            adapter = adapters.get(dev_id);
        }

        final BluetoothDevice[] matchingDiscoveredDeviceBucket = { null };

        final AdapterStatusListener statusListener = new AdapterStatusListener() {
            @Override
            public void adapterSettingsChanged(final BluetoothAdapter adapter, final AdapterSettings oldmask,
                                               final AdapterSettings newmask, final AdapterSettings changedmask, final long timestamp) {
                System.err.println("****** SETTINGS: "+oldmask+" -> "+newmask+", changed "+changedmask);
                System.err.println("Status Adapter:");
                System.err.println(adapter.toString());
            }

            @Override
            public void discoveringChanged(final BluetoothAdapter adapter, final boolean enabled, final boolean keepAlive, final long timestamp) {
                System.err.println("****** DISCOVERING: enabled "+enabled+", keepAlive "+keepAlive+" on "+adapter);
                System.err.println("Status Adapter:");
                System.err.println(adapter.toString());
            }

            @Override
            public void deviceFound(final BluetoothDevice device, final long timestamp) {
                final boolean matches = device.getAddress().equals(waitForDevice);
                System.err.println("****** FOUND__: "+device.toString()+" - match "+matches);
                System.err.println("Status Adapter:");
                System.err.println(device.getAdapter().toString());

                if( matches ) {
                    synchronized(matchingDiscoveredDeviceBucket) {
                        matchingDiscoveredDeviceBucket[0] = device;
                        matchingDiscoveredDeviceBucket.notifyAll();
                    }
                }
            }

            @Override
            public void deviceUpdated(final BluetoothDevice device, final EIRDataTypeSet updateMask, final long timestamp) {
                final boolean matches = device.getAddress().equals(waitForDevice);
                System.err.println("****** UPDATED: "+updateMask+" of "+device+" - match "+matches);
            }

            @Override
            public void deviceConnected(final BluetoothDevice device, final long timestamp) {
                final boolean matches = device.getAddress().equals(waitForDevice);
                System.err.println("****** CONNECTED: "+device+" - matches "+matches);
            }

            @Override
            public void deviceDisconnected(final BluetoothDevice device, final HCIStatusCode reason, final long timestamp) {
                System.err.println("****** DISCONNECTED: Reason "+reason+": "+device+" on "+device.getAdapter());
            }
        };
        adapter.addStatusListener(statusListener, null);

        final long timestamp_t0 = BluetoothUtils.getCurrentMilliseconds();

        adapter.enableDiscoverableNotifications(new BooleanNotification("Discoverable", timestamp_t0));

        adapter.enableDiscoveringNotifications(new BooleanNotification("Discovering", timestamp_t0));

        adapter.enablePairableNotifications(new BooleanNotification("Pairable", timestamp_t0));

        adapter.enablePoweredNotifications(new BooleanNotification("Powered", timestamp_t0));

        final GATTCharacteristicListener myCharacteristicListener = new GATTCharacteristicListener() {
            @Override
            public void notificationReceived(final BluetoothGattCharacteristic charDecl,
                                             final byte[] value, final long timestamp) {
                System.err.println("****** GATT notificationReceived: "+charDecl+
                                   ", value "+BluetoothUtils.bytesHexString(value, true, true));
            }

            @Override
            public void indicationReceived(final BluetoothGattCharacteristic charDecl,
                                           final byte[] value, final long timestamp, final boolean confirmationSent) {
                System.err.println("****** GATT indicationReceived: "+charDecl+
                                   ", value "+BluetoothUtils.bytesHexString(value, true, true));
            }
        };

        int loop = 0;
        try {
            do {
                loop++;
                System.err.println("****** Loop "+loop);

                final long t0 = BluetoothUtils.getCurrentMilliseconds();

                final boolean discoveryStarted = adapter.startDiscovery(true);

                System.err.println("The discovery started: " + (discoveryStarted ? "true" : "false") + " for mac "+waitForDevice+", mode "+mode);
                if( !discoveryStarted ) {
                    break;
                }
                BluetoothDevice sensor = null;

                if( 0 == mode ) {
                    synchronized(matchingDiscoveredDeviceBucket) {
                        boolean timeout = false;
                        while( !timeout && null == matchingDiscoveredDeviceBucket[0] ) {
                            matchingDiscoveredDeviceBucket.wait(t0_discovery);
                            final long tn = BluetoothUtils.getCurrentMilliseconds();
                            timeout = ( tn - t0 ) > t0_discovery;
                        }
                        sensor = matchingDiscoveredDeviceBucket[0];
                        matchingDiscoveredDeviceBucket[0] = null;
                    }
                } else if( 1 == mode ) {
                    sensor = adapter.find(null, waitForDevice, t0_discovery);
                } else {
                    boolean timeout = false;
                    while( null == sensor && !timeout ) {
                        final List<BluetoothDevice> devices = adapter.getDevices();
                        for(final Iterator<BluetoothDevice> id = devices.iterator(); id.hasNext() && !timeout; ) {
                            final BluetoothDevice d = id.next();
                            if(d.getAddress().equals(waitForDevice)) {
                                sensor = d;
                                break;
                            }
                            final long tn = BluetoothUtils.getCurrentMilliseconds();
                            timeout = ( tn - t0 ) > t0_discovery;
                        }
                    }
                }
                final long t1 = BluetoothUtils.getCurrentMilliseconds();
                if (sensor == null) {
                    System.err.println("No sensor found within "+(t1-t0)+" ms");
                    continue; // forever loop
                }
                System.err.println("Found device in "+(t1-t0)+" ms: ");
                printDevice(sensor);
                adapter.stopDiscovery();

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
                    // we tolerate the failed immediate connect, as it might happen at a later time
                }

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

                final List<BluetoothGattService> primServices = sensor.getServices();
                if ( null == primServices || primServices.isEmpty() ) {
                    System.err.println("No BluetoothGattService found!");
                } else {
                    final boolean addedCharacteristicListenerRes =
                      BluetoothGattService.addCharacteristicListenerToAll(sensor, primServices, myCharacteristicListener);
                    System.err.println("Added GATTCharacteristicListener: "+addedCharacteristicListenerRes);

                    int i=0, j=0;
                    for(final Iterator<BluetoothGattService> srvIter = primServices.iterator(); srvIter.hasNext(); i++) {
                        final BluetoothGattService primService = srvIter.next();
                        System.err.printf("  [%02d] Service %s\n", i, primService.toString());
                        System.err.printf("  [%02d] Service Characteristics\n", i);
                        final List<BluetoothGattCharacteristic> serviceCharacteristics = primService.getCharacteristics();
                        for(final Iterator<BluetoothGattCharacteristic> charIter = serviceCharacteristics.iterator(); charIter.hasNext(); j++) {
                            final BluetoothGattCharacteristic serviceChar = charIter.next();
                            System.err.printf("  [%02d.%02d] Decla: %s\n", i, j, serviceChar.toString());
                            final List<String> properties = Arrays.asList(serviceChar.getFlags());
                            if( properties.contains("read") ) {
                                final byte[] value = serviceChar.readValue();
                                final String svalue = BluetoothUtils.getUTF8String(value, 0, value.length);
                                System.err.printf("  [%02d.%02d] Value: %s ('%s')\n",
                                        i, j, BluetoothUtils.bytesHexString(value, true, true), svalue);
                            }
                        }
                    }
                    Thread.sleep(1000); // FIXME: Wait for notifications

                    final boolean remRes = BluetoothGattService.removeCharacteristicListenerFromAll(sensor, primServices, myCharacteristicListener);
                    System.err.println("Removed GATTCharacteristicListener: "+remRes);
                }
                sensor.disconnect();
                System.err.println("ScannerTinyB01 04 ...: "+adapter);
            } while( forever );
        } catch (final Throwable t) {
            System.err.println("Caught: "+t.getMessage());
            t.printStackTrace();
        }

        System.err.println("ScannerTinyB01 02 clear listener etc .. ");
        adapter.removeStatusListener(statusListener);
        adapter.disableDiscoverableNotifications();
        adapter.disableDiscoveringNotifications();
        adapter.disablePairableNotifications();
        adapter.disablePoweredNotifications();

        System.err.println("ScannerTinyB01 03 close: "+adapter);
        adapter.close();
        System.err.println("ScannerTinyB01 04");
        manager.shutdown();
        System.err.println("ScannerTinyB01 XX");
    }
    private static void printDevice(final BluetoothDevice device) {
        System.err.println("Address = " + device.getAddress());
        System.err.println("  Name = " + device.getName());
        System.err.println("  Connected = " + device.getConnected());
        System.err.println();
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
                System.out.println("###### "+name+": "+v+" in td "+(t1-t0)+" ms!");
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
