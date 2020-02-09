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
package org.tinyb;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class BluetoothFactory {
    /**
     * Fully qualified factory class name for D-Bus implementation: {@value}
     * <p>
     * This value is exposed for convenience, user implementations are welcome.
     * </p>
     */
    public static final String DBusFactoryImplClassName = "tinyb.dbus.DBusManager";

    /**
     * Fully qualified factory class name for native HCI implementation: {@value}
     * <p>
     * This value is exposed for convenience, user implementations are welcome.
     * </p>
     */
    public static final String HCIFactoryImplClassName = "tinyb.hci.HCIManager";

    /**
     * Returns an initialized BluetoothManager instance using the given {@code factoryImplClass}.
     * <p>
     * The {@code factoryImplClass} must provide the static method
     * <pre>
     * public static BluetoothManager getBluetoothManager() throws BluetoothException { .. }
     * </pre>
     * </p>
     * @param factoryImplClass the factory implementation class
     * @throws BluetoothException
     * @throws NoSuchMethodException
     * @throws SecurityException
     * @throws IllegalAccessException
     * @throws IllegalArgumentException
     * @throws InvocationTargetException
     * @see #getBluetoothManager(String)
     */
    public static synchronized BluetoothManager getBluetoothManager(final Class<?> factoryImplClass)
            throws BluetoothException, NoSuchMethodException, SecurityException,
                   IllegalAccessException, IllegalArgumentException, InvocationTargetException
    {
        final Method m = factoryImplClass.getMethod("getBluetoothManager");
        return (BluetoothManager)m.invoke(null);
    }

    /**
     * Returns an initialized BluetoothManager instance using the given {@code factoryImplClass}.
     * <p>
     * The {@code factoryImplClass} must provide the static method
     * <pre>
     * public static synchronized BluetoothManager getBluetoothManager() throws BluetoothException { .. }
     * </pre>
     * </p>
     * @param factoryImplClass the fully qualified factory implementation class name
     * @throws BluetoothException
     * @throws NoSuchMethodException
     * @throws SecurityException
     * @throws IllegalAccessException
     * @throws IllegalArgumentException
     * @throws InvocationTargetException
     * @throws ClassNotFoundException
     * @see {@link #DBusFactoryImplClassName}
     * @see {@link #HCIFactoryImplClassName}
     */
    public static synchronized BluetoothManager getBluetoothManager(final String factoryImplClassName)
            throws BluetoothException, NoSuchMethodException, SecurityException,
                   IllegalAccessException, IllegalArgumentException, InvocationTargetException, ClassNotFoundException
    {
        final Class<?> factoryImpl = Class.forName(factoryImplClassName);
        return getBluetoothManager(factoryImpl);
    }

    /**
     * Returns an initialized BluetoothManager instance using a D-Bus implementation.
     * @throws BluetoothException
     */
    public static synchronized BluetoothManager getDBusBluetoothManager() throws BluetoothException
    {
        return tinyb.dbus.DBusManager.getBluetoothManager();
    }
}
