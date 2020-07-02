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
 * Bluetooth LE random address type constants.
 * <p>
 * See {@link #get(int)} for its native integer mapping.
 * </p>
 * <p>
 * BT Core Spec v5.2:  Vol 6 LE, Part B Link Layer Specification: 1.3 Device Address
 * <p>
 * BT Core Spec v5.2:  Vol 6 LE, Part B Link Layer Specification: 1.3.2 Random device Address
 * </p>
 * <p>
 * Table 1.2, address bits [47:46]
 * </p>
 * <p>
 * If {@link BluetoothAddressType} is {@link BluetoothAddressType#BDADDR_LE_RANDOM},
 * its value shall be different than {@link BLERandomAddressType#UNDEFINED}.
 * </p>
 * <p>
 * If {@link BluetoothAddressType} is not {@link BluetoothAddressType#BDADDR_LE_RANDOM},
 * its value shall be {@link BLERandomAddressType#UNDEFINED}.
 * </p>
 * @since 2.0.0
 */
public enum BLERandomAddressType {
    /** Non-resolvable private random device address 0b00 */
    UNRESOLVABLE_PRIVAT (0x00),
    /** Resolvable private random device address 0b01 */
    RESOLVABLE_PRIVAT   (0x01),
    /** Reserved for future use 0b10 */
    RESERVED            (0x02),
    /** Static public 'random' device address 0b11 */
    STATIC_PUBLIC       (0x03),
    /** Undefined */
    UNDEFINED           (0xff);

    public final int value;

    /**
     * Maps the specified integer value to a constant of {@link BLERandomAddressType}.
     * @param value the integer value to be mapped to a constant of this enum type.
     * @return the corresponding constant of this enum type, using {@link #UNDEFINED} if not supported.
     */
    public static BLERandomAddressType get(final int value) {
        switch(value) {
            case 0x00: return UNRESOLVABLE_PRIVAT;
            case 0x01: return RESOLVABLE_PRIVAT;
            case 0x02: return RESERVED;
            case 0x03: return STATIC_PUBLIC;
            default: return UNDEFINED;
        }
    }

    public static BLERandomAddressType getLERandomAddressType(final String address) {
        final String highByteS = address.substring(0, 3);
        int highByte = 0;
        try {
            highByte = Integer.valueOf(highByteS, 16);
        } catch (final NumberFormatException e) {
            return UNDEFINED;
        }
        final int high2 = ( highByte >> 6 ) & 0x03;
        return get(high2);
    }

    BLERandomAddressType(final int v) {
        value = v;
    }
}
