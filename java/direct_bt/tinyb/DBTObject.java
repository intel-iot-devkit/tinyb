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

import org.tinyb.BluetoothObject;
import org.tinyb.BluetoothType;

public abstract class DBTObject extends DBTNativeDownlink implements BluetoothObject
{
    private final int hashValue;

    /* pp */ static int compHash(final String a, final String b) {
        // 31 * x == (x << 5) - x
        final int hash = 31 + a.hashCode();
        return ((hash << 5) - hash) + b.hashCode();
    }
    /* pp */ static int compHash(final int a, final int b) {
        // 31 * x == (x << 5) - x
        final int hash = 31 + a;
        return ((hash << 5) - hash) + b;
    }

    protected DBTObject(final long nativeInstance, final int hashValue)
    {
        super(nativeInstance);
        this.hashValue = hashValue;
    }

    static BluetoothType class_type() { return BluetoothType.NONE; }

    @Override
    public abstract boolean equals(final Object obj);

    @Override
    public final int hashCode() {
        return hashValue;
    }

    @Override
    protected void finalize()
    {
        close();
    }

    @Override
    public synchronized void close() {
        delete();
    }

    @Override
    public BluetoothObject clone()
    { throw new UnsupportedOperationException(); } // FIXME

}
