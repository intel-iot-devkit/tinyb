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
import org.tinyb.BLERandomAddressType;
import org.tinyb.BluetoothException;
import org.tinyb.BluetoothFactory;
import org.tinyb.BluetoothGattCharacteristic;
import org.tinyb.BluetoothGattService;
import org.tinyb.BluetoothManager;
import org.tinyb.BluetoothNotification;
import org.tinyb.BluetoothType;
import org.tinyb.BluetoothUtils;
import org.tinyb.EIRDataTypeSet;
import org.tinyb.GATTCharacteristicListener;
import org.tinyb.HCIStatusCode;
import org.tinyb.HCIWhitelistConnectType;

public class ScannerTinyB10 {
    static final String EUI48_ANY_DEVICE = "00:00:00:00:00:00";

    final List<String> waitForDevices = new ArrayList<String>();
    final boolean isDirectBT;
    final BluetoothManager manager;

    long timestamp_t0;

    int MULTI_MEASUREMENTS = 8;
    boolean KEEP_CONNECTED = true;
    boolean REMOVE_DEVICE = true;
    boolean USE_WHITELIST = false;
    final List<String> whitelist = new ArrayList<String>();
    final List<String> characteristicList = new ArrayList<String>();

    boolean SHOW_UPDATE_EVENTS = false;

    int dev_id = 0; // default

    Collection<String> devicesInProcessing = Collections.synchronizedCollection(new ArrayList<>());
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
                && BLERandomAddressType.STATIC_PUBLIC != device.getBLERandomAddressType() ) {
                System.err.println("****** FOUND__-2: Skip 'non public' or 'random static public' LE "+device.toString());
                return;
            }
            if( !devicesInProcessing.contains( device.getAddress() ) &&
                ( waitForDevices.isEmpty() ||
                  ( waitForDevices.contains(device.getAddress()) &&
                    ( 0 < MULTI_MEASUREMENTS || !devicesProcessed.containsAll(waitForDevices) )
                  )
                )
              )
            {
                System.err.println("****** FOUND__-0: Connecting "+device.toString());
                {
                    final long td = BluetoothUtils.getCurrentMilliseconds() - timestamp_t0; // adapter-init -> now
                    System.err.println("PERF: adapter-init -> FOUND__-0 " + td + " ms");
                }
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
        public void deviceUpdated(final BluetoothDevice device, final EIRDataTypeSet updateMask, final long timestamp) {
            if( SHOW_UPDATE_EVENTS ) {
                System.err.println("****** UPDATED: "+updateMask+" of "+device);
            }
        }

        @Override
        public void deviceConnected(final BluetoothDevice device, final long timestamp) {
            if( !devicesInProcessing.contains( device.getAddress() ) &&
                ( waitForDevices.isEmpty() ||
                  ( waitForDevices.contains(device.getAddress()) &&
                    ( 0 < MULTI_MEASUREMENTS || !devicesProcessed.containsAll(waitForDevices) )
                  )
                )
              )
            {
                System.err.println("****** CONNECTED-0: Processing "+device.toString());
                {
                    final long td = BluetoothUtils.getCurrentMilliseconds() - timestamp_t0; // adapter-init -> now
                    System.err.println("PERF: adapter-init -> CONNECTED-0 " + td + " ms");
                }
                final Thread deviceProcessingTask = new Thread( new Runnable() {
                    @Override
                    public void run() {
                        processConnectedDevice(device);
                    }
                }, "DBT-Process-"+device.getAddress());
                devicesInProcessing.add(device.getAddress());
                deviceProcessingTask.setDaemon(true); // detach thread
                deviceProcessingTask.start();
            } else {
                System.err.println("****** CONNECTED-1: NOP " + device.toString());
            }
        }

        @Override
        public void deviceDisconnected(final BluetoothDevice device, final HCIStatusCode reason, final long timestamp) {
            System.err.println("****** DISCONNECTED: Reason "+reason+": "+device+" on "+device.getAdapter());
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
        if( !USE_WHITELIST && 0 == devicesInProcessing.size() && !res ) {
            device.getAdapter().startDiscovery( true );
        }
    }

    private void processConnectedDevice(final BluetoothDevice device) {
        System.err.println("****** Processing Device: Start " + device.toString());
        device.getAdapter().stopDiscovery(); // make sure for pending connections on failed connect*(..) command

        final long t1 = BluetoothUtils.getCurrentMilliseconds();
        boolean success = false;

        //
        // GATT Service Processing
        //
        try {
            final List<BluetoothGattService> primServices = device.getServices(); // implicit GATT connect...
            if( null == primServices || 0 == primServices.size() ) {
                // Cheating the flow, but avoiding: goto, do-while-false and lastly unreadable intendations
                // And it is an error case nonetheless ;-)
                throw new RuntimeException("Processing Device: getServices() failed " + device.toString());
            }
            final long t5 = BluetoothUtils.getCurrentMilliseconds();
            {
                final long td01 = t1 - timestamp_t0; // adapter-init -> processing-start
                final long td15 = t5 - t1; // get-gatt-services
                final long tdc5 = t5 - device.getLastDiscoveryTimestamp(); // discovered to gatt-complete
                final long td05 = t5 - timestamp_t0; // adapter-init -> gatt-complete
                System.err.println(System.lineSeparator()+System.lineSeparator());
                System.err.println("PERF: GATT primary-services completed\n");
                System.err.println("PERF:  adapter-init to processing-start " + td01 + " ms,"+System.lineSeparator()+
                                   "PERF:  get-gatt-services " + td15 + " ms,"+System.lineSeparator()+
                                   "PERF:  discovered to gatt-complete " + tdc5 + " ms (connect " + (tdc5 - td15) + " ms),"+System.lineSeparator()+
                                   "PERF:  adapter-init to gatt-complete " + td05 + " ms"+System.lineSeparator());
            }
            {
                for(final String characteristic : characteristicList) {
                    final BluetoothGattCharacteristic char0 = (BluetoothGattCharacteristic)
                            manager.find(BluetoothType.GATT_CHARACTERISTIC, null, characteristic, null);
                    final BluetoothGattCharacteristic char1 = (BluetoothGattCharacteristic)
                            manager.find(BluetoothType.GATT_CHARACTERISTIC, null, characteristic, device.getAdapter());
                    final BluetoothGattCharacteristic char2 = (BluetoothGattCharacteristic)
                            manager.find(BluetoothType.GATT_CHARACTERISTIC, null, characteristic, device);
                    System.err.println("Char UUID "+characteristic);
                    System.err.println("  over manager: "+char0);
                    System.err.println("  over adapter: "+char1);
                    System.err.println("  over device : "+char2);
                }
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
                        try {
                            final byte[] value = serviceChar.readValue();
                            final String svalue = BluetoothUtils.decodeUTF8String(value, 0, value.length);
                            System.err.printf("  [%02d.%02d] Value: %s ('%s')\n",
                                    i, j, BluetoothUtils.bytesHexString(value, true, true), svalue);
                        } catch( final Exception ex) {
                            System.err.println("Caught "+ex.getMessage());
                            ex.printStackTrace();
                        }
                    }
                }
            }
            // FIXME sleep 1s for potential callbacks ..
            try {
                Thread.sleep(1000);
            } catch (final InterruptedException e) {
                e.printStackTrace();
            }
            success = true;
        } catch (final Throwable t ) {
            System.err.println("****** Processing Device: Exception caught for " + device.toString() + ": "+t.getMessage());
            t.printStackTrace();
        }
        if( KEEP_CONNECTED ) {
            while( device.pingGATT() ) {
                System.err.println("****** Processing Device: pingGATT OK: "+device.getAddress());
                try {
                    device.getAdapter().startDiscovery( false );
                    Thread.sleep(1000);
                    device.getAdapter().stopDiscovery(); // make sure for pending connections on failed connect*(..) command
                } catch (final InterruptedException e) {
                    e.printStackTrace();
                }
            }
            System.err.println("****** Processing Device: pingGATT failed: "+device.getAddress());
        }

        System.err.println("****** Processing Device: disconnecting: "+device.getAddress());
        device.disconnect(); // will implicitly purge the GATT data, including GATTCharacteristic listener.
        while( device.getConnected() ) {
            try {
                Thread.sleep(100);
            } catch (final InterruptedException e) {
                e.printStackTrace();
            }
        }
        if( REMOVE_DEVICE ) {
            System.err.println("****** Processing Device: removing: "+device.getAddress());
            device.remove();
        }

        devicesInProcessing.remove(device.getAddress());
        if( 0 < MULTI_MEASUREMENTS ) {
            MULTI_MEASUREMENTS--;
            System.err.println("****** Processing Device: MULTI_MEASUREMENTS left "+MULTI_MEASUREMENTS+": "+device.getAddress());
        }
        System.err.println("****** Processing Device: End: Success " + success +
                           " on " + device.toString() + "; devInProc "+devicesInProcessing.size());
        if( !USE_WHITELIST && 0 == devicesInProcessing.size() ) {
            device.getAdapter().startDiscovery( true );
        }
        if( success ) {
            devicesProcessed.add(device.getAddress());
        }
    }

    public ScannerTinyB10(final String bluetoothManagerClazzName) {
        BluetoothManager _manager = null;
        final BluetoothFactory.ImplementationIdentifier implID = BluetoothFactory.getImplementationIdentifier(bluetoothManagerClazzName);
        if( null == implID ) {
            System.err.println("Unable to find BluetoothManager "+bluetoothManagerClazzName);
            System.exit(-1);
        }
        isDirectBT = BluetoothFactory.DirectBTImplementationID.equals(implID);
        System.err.println("Using BluetoothManager "+bluetoothManagerClazzName);
        System.err.println("Using Implementation "+implID+", isDirectBT "+isDirectBT);
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

    public void runTest() {
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

        boolean done = false;

        if( USE_WHITELIST ) {
            for(final Iterator<String> wliter = whitelist.iterator(); wliter.hasNext(); ) {
                final String addr = wliter.next();
                final boolean res = adapter.addDeviceToWhitelist(addr, BluetoothAddressType.BDADDR_LE_PUBLIC, HCIWhitelistConnectType.HCI_AUTO_CONN_ALWAYS);
                System.err.println("Added to whitelist: res "+res+", address "+addr);
            }
        } else {
            if( !adapter.startDiscovery( true ) ) {
                System.err.println("Adapter start discovery failed");
                done = true;
            }
        }

        while( !done ) {
            if( 0 == MULTI_MEASUREMENTS ||
                ( -1 == MULTI_MEASUREMENTS && !waitForDevices.isEmpty() && devicesProcessed.containsAll(waitForDevices) )
              )
            {
                System.err.println("****** EOL Test MULTI_MEASUREMENTS left "+MULTI_MEASUREMENTS+
                                   ", processed "+devicesProcessed.size()+"/"+waitForDevices.size());
                System.err.println("****** WaitForDevices "+Arrays.toString(waitForDevices.toArray()));
                System.err.println("****** DevicesProcessed "+Arrays.toString(devicesProcessed.toArray()));
                done = true;
            } else {
                try {
                    Thread.sleep(3000);
                } catch (final InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }

        // All implicit via destructor or shutdown hook!
        // manager.shutdown(); /* implies: adapter.close(); */
    }

    public static void main(final String[] args) throws InterruptedException {
        String bluetoothManagerClazzName = BluetoothFactory.DirectBTImplementationID.BluetoothManagerClassName;
        for(int i=0; i< args.length; i++) {
            final String arg = args[i];
            if( arg.equals("-bluetoothManager") && args.length > (i+1) ) {
                bluetoothManagerClazzName = args[++i];
            } else if( arg.equals("-debug") ) {
                System.setProperty("org.tinyb.verbose", "true");
                System.setProperty("org.tinyb.debug", "true");
            }
        }
        final ScannerTinyB10 test = new ScannerTinyB10(bluetoothManagerClazzName);

        boolean waitForEnter=false;
        {
            for(int i=0; i< args.length; i++) {
                final String arg = args[i];

                if( arg.equals("-wait") ) {
                    waitForEnter = true;
                } else if( arg.equals("-show_update_events") ) {
                    test.SHOW_UPDATE_EVENTS = true;
                } else if( arg.equals("-dev_id") && args.length > (i+1) ) {
                    test.dev_id = Integer.valueOf(args[++i]).intValue();
                } else if( arg.equals("-mac") && args.length > (i+1) ) {
                    test.waitForDevices.add(args[++i]);
                } else if( arg.equals("-wl") && args.length > (i+1) ) {
                    final String addr = args[++i];
                    System.err.println("Whitelist + "+addr);
                    test.whitelist.add(addr);
                    test.USE_WHITELIST = true;
                } else if( arg.equals("-char") && args.length > (i+1) ) {
                    test.characteristicList.add(args[++i]);
                } else if( arg.equals("-disconnect") ) {
                    test.KEEP_CONNECTED = false;
                } else if( arg.equals("-keepDevice") ) {
                    test.REMOVE_DEVICE = false;
                } else if( arg.equals("-count")  && args.length > (i+1) ) {
                    test.MULTI_MEASUREMENTS = Integer.valueOf(args[++i]).intValue();
                } else if( arg.equals("-single") ) {
                    test.MULTI_MEASUREMENTS = -1;
                }
            }
            System.err.println("Run with '[-dev_id <adapter-index>] (-mac <device_address>)* [-disconnect] [-count <number>] [-single] (-wl <device_address>)* (-char <uuid>)* [-show_update_events] [-bluetoothManager <BluetoothManager-Implementation-Class-Name>]'");
        }

        System.err.println("BluetoothManager "+bluetoothManagerClazzName);
        System.err.println("MULTI_MEASUREMENTS "+test.MULTI_MEASUREMENTS);
        System.err.println("KEEP_CONNECTED "+test.KEEP_CONNECTED);
        System.err.println("REMOVE_DEVICE "+test.REMOVE_DEVICE);
        System.err.println("USE_WHITELIST "+test.USE_WHITELIST);
        System.err.println("dev_id "+test.dev_id);
        System.err.println("waitForDevice: "+Arrays.toString(test.waitForDevices.toArray()));
        System.err.println("characteristicList: "+Arrays.toString(test.characteristicList.toArray()));

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
