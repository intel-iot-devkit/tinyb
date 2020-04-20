/**
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
 * Author: Andrei Vasiliu <andrei.vasiliu@intel.com>
 * Copyright (c) 2016 Intel Corporation.
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

package org.tinyb;

import java.util.List;

public interface BluetoothManager
{
    /** Find a BluetoothObject of a type matching type. If parameters name,
      * identifier and parent are not null, the returned object will have to
      * match them.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter type specify the type of the object you are
      * waiting for, NONE means anything.
      * @parameter name optionally specify the name of the object you are
      * waiting for (for Adapter or Device)
      * @parameter identifier optionally specify the identifier of the object you are
      * waiting for (UUID for GattService, GattCharacteristic or GattDescriptor, address
      * for Adapter or Device)
      * @parameter parent optionally specify the parent of the object you are
      * waiting for
      * @parameter timeoutMS the function will return after timeout time in milliseconds, a
      * value of zero means wait forever. If object is not found during this time null will be returned.
      * @return An object matching the name, identifier, parent or null if not found before
      * timeout expires or event is canceled.
      */
    public BluetoothObject find(BluetoothType type, String name, String identifier, BluetoothObject parent, long timeoutMS);


    /** Find a BluetoothObject of a type matching type. If parameters name,
      * identifier and parent are not null, the returned object will have to
      * match them.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter type specify the type of the object you are
      * waiting for, NONE means anything.
      * @parameter name optionally specify the name of the object you are
      * waiting for (for Adapter or Device)
      * @parameter identifier optionally specify the identifier of the object you are
      * waiting for (UUID for GattService, GattCharacteristic or GattDescriptor, address
      * for Adapter or Device)
      * @parameter parent optionally specify the parent of the object you are
      * waiting for
      * @return An object matching the name, identifier and parent.
      */
    public BluetoothObject  find(BluetoothType type, String name, String identifier, BluetoothObject parent);

    /** Find a BluetoothObject of type T. If parameters name, identifier and
      * parent are not null, the returned object will have to match them.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter name optionally specify the name of the object you are
      * waiting for (for Adapter or Device)
      * @parameter identifier optionally specify the identifier of the object you are
      * waiting for (UUID for GattService, GattCharacteristic or GattDescriptor, address
      * for Adapter or Device)
      * @parameter parent optionally specify the parent of the object you are
      * waiting for
      * @parameter timeoutMS the function will return after timeout time in milliseconds, a
      * value of zero means wait forever. If object is not found during this time null will be returned.
      * @return An object matching the name, identifier, parent or null if not found before
      * timeout expires or event is canceled.
      */
    public <T extends BluetoothObject>  T find(String name, String identifier, BluetoothObject parent, long timeoutMS);

    /** Find a BluetoothObject of type T. If parameters name, identifier and
      * parent are not null, the returned object will have to match them.
      * It will first check for existing objects. It will not turn on discovery
      * or connect to devices.
      * @parameter name optionally specify the name of the object you are
      * waiting for (for Adapter or Device)
      * @parameter identifier optionally specify the identifier of the object you are
      * waiting for (UUID for GattService, GattCharacteristic or GattDescriptor, address
      * for Adapter or Device)
      * @parameter parent optionally specify the parent of the object you are
      * waiting for
      * @return An object matching the name, identifier and parent.
      */
    public <T extends BluetoothObject>  T find(String name, String identifier, BluetoothObject parent);

    /** Return a BluetoothObject of a type matching type. If parameters name,
      * identifier and parent are not null, the returned object will have to
      * match them. Only objects which are already in the system will be returned.
      * @parameter type specify the type of the object you are
      * waiting for, NONE means anything.
      * @parameter name optionally specify the name of the object you are
      * waiting for (for Adapter or Device)
      * @parameter identifier optionally specify the identifier of the object you are
      * waiting for (UUID for GattService, GattCharacteristic or GattDescriptor, address
      * for Adapter or Device)
      * @parameter parent optionally specify the parent of the object you are
      * waiting for
      * @return An object matching the name, identifier, parent or null if not found.
      */
    public BluetoothObject getObject(BluetoothType type, String name,
                                String identifier, BluetoothObject parent);

    /** Return a List of BluetoothObject of a type matching type. If parameters name,
      * identifier and parent are not null, the returned object will have to
      * match them. Only objects which are already in the system will be returned.
      * @parameter type specify the type of the object you are
      * waiting for, NONE means anything.
      * @parameter name optionally specify the name of the object you are
      * waiting for (for Adapter or Device)
      * @parameter identifier optionally specify the identifier of the object you are
      * waiting for (UUID for GattService, GattCharacteristic or GattDescriptor, address
      * for Adapter or Device)
      * @parameter parent optionally specify the parent of the object you are
      * waiting for
      * @return A vector of object matching the name, identifier, parent.
      */
    public List<BluetoothObject> getObjects(BluetoothType type, String name,
                                    String identifier, BluetoothObject parent);

    /** Returns a list of BluetoothAdapters available in the system
      * @return A list of BluetoothAdapters available in the system
      */
    public List<BluetoothAdapter> getAdapters();

    /** Returns a list of discovered BluetoothDevices
      * @return A list of discovered BluetoothDevices
      */
    public List<BluetoothDevice> getDevices();

    /** Returns a list of available BluetoothGattServices
      * @return A list of available BluetoothGattServices
      */
    public List<BluetoothGattService> getServices();

    /** Sets a default adapter to use for discovery.
      * @return TRUE if the device was set
      */
    public boolean setDefaultAdapter(BluetoothAdapter adapter);

    /** Gets the default adapter to use for discovery.
      * <p>
      * System default is the last detected adapter at initialisation.
      * </p>
      * @return the used default adapter
      */
    public BluetoothAdapter getDefaultAdapter();

    /** Turns on device discovery on the default adapter if it is disabled.
      * @return TRUE if discovery was successfully enabled
      */
    public boolean startDiscovery() throws BluetoothException;

    /** Turns off device discovery on the default adapter if it is enabled.
      * @return TRUE if discovery was successfully disabled
      */
    public boolean stopDiscovery() throws BluetoothException;

    /** Returns if the discovers is running or not.
      * @return TRUE if discovery is running
      */
    public boolean getDiscovering() throws BluetoothException;

    /**
     * Release the native memory associated with this object and all related Bluetooth resources.
     * The object should not be used following a call to close
     * <p>
     * Shutdown method is intended to allow a clean Bluetooth state at program exist.
     * </p>
     */
    public void shutdown();
}
