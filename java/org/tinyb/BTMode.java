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

/**
 * Bluetooth adapter operating mode
 * <p>
 * See {@link #get(int)} for its native integer mapping.
 * </p>
 * @since 2.0.0
 */
public enum BTMode {
    /** Zero mode, neither DUAL, BREDR nor LE. Usually an error. */
    NONE      (0),
    /** Dual Bluetooth mode, i.e. BREDR + LE. */
    DUAL      (1),
    /** BREDR only Bluetooth mode */
    BREDR     (2),
    /** LE only Bluetooth mode */
    LE        (3);

    public final int value;

    /**
     * Maps the specified name to a constant of BTMode.
     * <p>
     * Implementation simply returns {@link #valueOf(String)}.
     * This maps the constant names itself to their respective constant.
     * </p>
     * @param name the string name to be mapped to a constant of this enum type.
     * @return the corresponding constant of this enum type.
     * @throws IllegalArgumentException if the specified name can't be mapped to a constant of this enum type
     *                                  as described above.
     */
    public static BTMode get(final String name) throws IllegalArgumentException {
        return valueOf(name);
    }

    /**
     * Maps the specified integer value to a constant of {@link BTMode}.
     * @param value the integer value to be mapped to a constant of this enum type.
     * @return the corresponding constant of this enum type, using {@link #NONE} if not supported.
     */
    public static BTMode get(final int value) {
        switch(value) {
            case 0x01: return DUAL;
            case 0x02: return BREDR;
            case 0x03: return LE;
            default: return NONE;
        }
    }

    BTMode(final int v) {
        value = v;
    }
}
