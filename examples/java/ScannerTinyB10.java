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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import org.tinyb.AdapterSettings;
import org.tinyb.BluetoothAdapter;
import org.tinyb.BluetoothAddressType;
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
import org.tinyb.HCIWhitelistConnectType;

public class ScannerTinyB10 {
    static {
        System.setProperty("org.tinyb.verbose", "true");
    }

    static final String EUI48_ANY_DEVICE = "00:00:00:00:00:00";

    String waitForDevice = EUI48_ANY_DEVICE;

    long timestamp_t0;

    boolean USE_WHITELIST = false;
    final List<String> whitelist = new ArrayList<String>();

    boolean BLOCK_DISCOVERY = true;

    int factory = 0;

    int dev_id = 0; // default

    Collection<String> devicesTasks = Collections.synchronizedCollection(new ArrayList<>());
    Collection<String> devicesProcessed = Collections.synchronizedCollection(new ArrayList<>());

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
        }

        @Override
        public void deviceFound(final BluetoothDevice device, final long timestamp) {
            System.err.println("****** FOUND__: "+device.toString());

            if( BluetoothAddressType.BDADDR_LE_PUBLIC != device.getAddressType()
                /** && BluetoothAddressType.BDADDR_LE_RANDOM != device.getAddressType() */ ) {
                System.err.println("****** FOUND__-2: Skip non public LE "+device.toString());
                return;
            }
            if( waitForDevice.equals(EUI48_ANY_DEVICE) ||
                ( waitForDevice.equals(device.getAddress()) &&
                  !devicesProcessed.contains(waitForDevice) &&
                  !devicesTasks.contains(waitForDevice)
                ) )
            {
                System.err.println("****** FOUND__-0: Connecting "+device.toString());
                final Thread deviceConnectTask = new Thread( new Runnable() {
                    @Override
                    public void run() {
                        connectDiscoveredDevice(device);
                    }
                }, "DBT-Connect-"+device.getAddress());
                deviceConnectTask.setDaemon(true); // detach thread
                deviceConnectTask.start();
            } else {
                System.err.println("****** FOUND__-1: NOP "+device.toString());
            }
        }

        @Override
        public void deviceUpdated(final BluetoothDevice device, final long timestamp, final EIRDataTypeSet updateMask) {
            System.err.println("****** UPDATED: "+updateMask+" of "+device);
        }

        @Override
        public void deviceConnectionChanged(final BluetoothDevice device, final boolean connected, final long timestamp) {
            System.err.println("****** CONNECTION: connected "+connected+": "+device);

            if( !connected ) {
                System.err.println("****** DISCONNECTED: "+device.toString());
                return;
            }

            if( waitForDevice.equals(EUI48_ANY_DEVICE) ||
                ( waitForDevice.equals(device.getAddress()) &&
                  !devicesProcessed.contains(waitForDevice) &&
                  !devicesTasks.contains(waitForDevice)
                ) )
            {
                System.err.println("****** CONNECTED-0: Processing "+device.toString());
                final Thread deviceProcessingTask = new Thread( new Runnable() {
                    @Override
                    public void run() {
                        processConnectedDevice(device);
                    }
                }, "DBT-Process-"+device.getAddress());
                devicesTasks.add(device.getAddress());
                deviceProcessingTask.setDaemon(true); // detach thread
                deviceProcessingTask.start();
            } else {
                System.err.println("****** CONNECTED-1: NOP %s" + device.toString());
            }
        }
    };

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

    private void connectDiscoveredDevice(final BluetoothDevice device) {
        System.err.println("****** Connecting Device: Start " + device.toString());
        device.getAdapter().stopDiscovery();
        boolean res = false;
        if( !USE_WHITELIST ) {
            res = device.connect();
        }
        System.err.println("****** Connecting Device: End result "+res+" of " + device.toString());
        if( !USE_WHITELIST && ( !BLOCK_DISCOVERY || !res ) ) {
            device.getAdapter().startDiscovery( BLOCK_DISCOVERY );
        }
    }

    private void processConnectedDevice(final BluetoothDevice device) {
        // earmark device as being processed right-away
        devicesProcessed.add(device.getAddress());

        System.err.println("****** Processing Device: Start " + device.toString());
        final long t1 = BluetoothUtils.getCurrentMilliseconds();

        //
        // GATT Service Processing
        //
        final List<BluetoothGattService> primServices = device.getServices(); // implicit GATT connect...
        if( null == primServices ) {
            System.err.println("****** Processing Device: getServices() failed " + device.toString());
        } else {
            final long t5 = BluetoothUtils.getCurrentMilliseconds();
            {
                final long td15 = t5 - t1; // connected -> gatt-complete
                final long tdc5 = t5 - device.getCreationTimestamp(); // discovered to gatt-complete
                final long td05 = t5 - timestamp_t0; // adapter-init -> gatt-complete
                System.err.println(System.lineSeparator()+System.lineSeparator());
                System.err.println("GATT primary-services completed\n");
                System.err.println("  connected to gatt-complete " + td15 + " ms,"+System.lineSeparator()+
                                   "  discovered to gatt-complete " + tdc5 + " ms (connect " + (tdc5 - td15) + " ms),"+System.lineSeparator()+
                                   "  adapter-init to gatt-complete " + td05 + " ms"+System.lineSeparator());
            }
            final boolean addedCharacteristicListenerRes =
              BluetoothGattService.addCharacteristicListenerToAll(device, primServices, myCharacteristicListener);
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
            // FIXME sleep 1s for potential callbacks ..
            try {
                Thread.sleep(1000);
            } catch (final InterruptedException e) {
                e.printStackTrace();
            }
        }
        if( USE_WHITELIST || BLOCK_DISCOVERY ) {
            device.disconnect(); // will implicitly purge the GATT data, including GATTCharacteristic listener.
        } else {
            device.getAdapter().stopDiscovery();
            device.disconnect();
            device.getAdapter().startDiscovery(false);
        }

        if( !USE_WHITELIST && BLOCK_DISCOVERY ) {
            if( 1 >= devicesTasks.size() ) {
                device.getAdapter().startDiscovery( BLOCK_DISCOVERY );
            }
        }
        devicesTasks.remove(device.getAddress());
        System.err.println("****** Processing Device: End: " + device.toString());
    }

    public void runTest() {
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

        timestamp_t0 = BluetoothUtils.getCurrentMilliseconds();

        adapter.addStatusListener(statusListener, null);
        adapter.enableDiscoverableNotifications(new BooleanNotification("Discoverable", timestamp_t0));

        adapter.enableDiscoveringNotifications(new BooleanNotification("Discovering", timestamp_t0));

        adapter.enablePairableNotifications(new BooleanNotification("Pairable", timestamp_t0));

        adapter.enablePoweredNotifications(new BooleanNotification("Powered", timestamp_t0));

        if( USE_WHITELIST ) {
            for(final Iterator<String> wliter = whitelist.iterator(); wliter.hasNext(); ) {
                final String addr = wliter.next();
                final boolean res = adapter.addDeviceToWhitelist(addr, BluetoothAddressType.BDADDR_LE_PUBLIC, HCIWhitelistConnectType.HCI_AUTO_CONN_ALWAYS);
                System.err.println("Added to whitelist: res "+res+", address "+addr);
            }
        }

        boolean done = false;

        if( !USE_WHITELIST ) {
            System.err.println("****** Main: startDiscovery()\n");
            if( !adapter.startDiscovery( BLOCK_DISCOVERY ) ) {
                System.err.println("Adapter start discovery failed");
                done = true;
            }
        }

        while( !done ) {
            if( !waitForDevice.equals(EUI48_ANY_DEVICE) && devicesProcessed.contains(waitForDevice) ) {
                System.err.println("****** WaitForDevice processed "+waitForDevice);
                done = true;
            } else {
                if( !!USE_WHITELIST && !BLOCK_DISCOVERY && 0 >= devicesTasks.size() ) {
                    adapter.startDiscovery(false);
                }
            }
            try {
                Thread.sleep(5000);
            } catch (final InterruptedException e) {
                e.printStackTrace();
            }
        }

        System.err.println("ScannerTinyB10 03 close: "+adapter);
        adapter.close();
        System.err.println("ScannerTinyB10 04");
        manager.shutdown();
        System.err.println("ScannerTinyB10 XX");
    }

    public static void main(final String[] args) throws InterruptedException {
        final ScannerTinyB10 test = new ScannerTinyB10();

        boolean waitForEnter=false;
        {
            for(int i=0; i< args.length; i++) {
                final String arg = args[i];

                if( arg.equals("-wait") ) {
                    waitForEnter = true;
                } else if( arg.equals("-keepDiscovery") ) {
                    test.BLOCK_DISCOVERY = false;
                } else if( arg.equals("-dev_id") && args.length > (i+1) ) {
                    test.dev_id = Integer.valueOf(args[++i]).intValue();
                } else if( arg.equals("-mac") && args.length > (i+1) ) {
                    test.waitForDevice = args[++i];
                } else if( arg.equals("-wl") && args.length > (i+1) ) {
                    final String addr = args[++i];
                    System.err.println("Whitelist + "+addr);
                    test.whitelist.add(addr);
                    test.BLOCK_DISCOVERY = true;
                    test.USE_WHITELIST = true;
                } else if( arg.equals("-factory") && args.length > (i+1) ) {
                    test.factory = Integer.valueOf(args[++i]).intValue();
                }
            }

            System.err.println("Run with '[-dev_id <adapter-index>] [-mac <device_address>] (-wl <device_address>)* [-factory <BluetoothManager-Factory-Implementation-Class>]'");
        }

        System.err.println("USE_WHITELIST "+test.USE_WHITELIST);
        System.err.println("BLOCK_DISCOVERY "+test.BLOCK_DISCOVERY);
        System.err.println("dev_id "+test.dev_id);
        System.err.println("waitForDevice: "+test.waitForDevice);

        if( waitForEnter ) {
            System.err.println("Press ENTER to continue\n");
            try{ System.in.read();
            } catch(final Exception e) { }
        }
        test.runTest();
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
