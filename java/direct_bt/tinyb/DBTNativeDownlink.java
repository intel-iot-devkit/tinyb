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

package direct_bt.tinyb;

import org.tinyb.BluetoothFactory;

public abstract class DBTNativeDownlink
{
    protected long nativeInstance;
    private boolean isValid;

    static {
        BluetoothFactory.checkInitialized();
    }

    protected DBTNativeDownlink(final long nativeInstance)
    {
        this.nativeInstance = nativeInstance;
        isValid = true;
        initNativeJavaObject(nativeInstance);
    }

    protected final boolean isValid() { return isValid; }

    @Override
    protected void finalize()
    {
        delete();
    }

    /**
     * Deletes the native instance in the following order
     * <ol>
     *   <li>Removes this java reference from the native instance</li>
     *   <li>Deletes the native instance via {@link #deleteImpl()}</li>
     *   <li>Sets the nativeInstance := 0</li>
     * </ol>
     */
    public synchronized void delete() {
        if (!isValid) {
            return;
        }
        isValid = false;
        deleteNativeJavaObject(nativeInstance);
        deleteImpl();
        nativeInstance = 0;
    }

    /**
     * Called from native JavaUplink dtor -> JavaGlobalObj dtor,
     * i.e. native instance destructed in native land.
     */
    private synchronized void notifyDeleted() {
        isValid = false;
        nativeInstance = 0;
        // System.err.println("***** notifyDeleted: "+getClass().getSimpleName()+": valid "+isValid+" -> false, handle 0x"+Long.toHexString(nativeInstance)+" -> null: "+toString());
    }

    /**
     * Deletes the native instance.
     * <p>
     * Called via {@link #delete()} and at this point this java reference
     * has been removed from the native instance.
     * </p>
     */
    protected abstract void deleteImpl();

    private native void initNativeJavaObject(final long nativeInstance);
    private native void deleteNativeJavaObject(final long nativeInstance);
}
