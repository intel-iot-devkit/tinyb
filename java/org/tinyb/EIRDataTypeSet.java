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
 * Bit mask of 'Extended Inquiry Response' (EIR) data fields,
 * indicating a set of related data.
 *
 * @since 2.0.0
 */
public class EIRDataTypeSet {

    /**
     * Bits representing 'Extended Inquiry Response' (EIR) data fields.
     *
     * @since 2.0.0
     */
    public enum DataType {
        NONE         (     0),
        EVT_TYPE     (1 << 0),
        BDADDR_TYPE  (1 << 1),
        BDADDR       (1 << 2),
        FLAGS        (1 << 3),
        NAME         (1 << 4),
        NAME_SHORT   (1 << 5),
        RSSI         (1 << 6),
        TX_POWER     (1 << 7),
        MANUF_DATA   (1 << 8),
        DEVICE_CLASS (1 << 9),
        APPEARANCE   (1 << 10),
        HASH         (1 << 11),
        RANDOMIZER   (1 << 12),
        DEVICE_ID    (1 << 13),
        SERVICE_UUID (1 << 30);

        DataType(final int v) { value = v; }
        public final int value;
    }

    public int mask;

    public EIRDataTypeSet(final int v) {
        mask = v;
    }

    public boolean isSet(final DataType bit) { return 0 != ( mask & bit.value ); }
    public void set(final DataType bit) { mask = mask | bit.value; }

    public String toString() {
        int count = 0;
        final StringBuilder out = new StringBuilder();
        if( isSet(DataType.EVT_TYPE) ) {
            out.append(DataType.EVT_TYPE.name()); count++;
        }
        if( isSet(DataType.BDADDR_TYPE) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.BDADDR_TYPE.name()); count++;
        }
        if( isSet(DataType.BDADDR) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.BDADDR.name()); count++;
        }
        if( isSet(DataType.FLAGS) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.FLAGS.name()); count++;
        }
        if( isSet(DataType.NAME) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.NAME.name()); count++;
        }
        if( isSet(DataType.NAME_SHORT) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.NAME_SHORT.name()); count++;
        }
        if( isSet(DataType.RSSI) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.RSSI.name()); count++;
        }
        if( isSet(DataType.TX_POWER) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.TX_POWER.name()); count++;
        }
        if( isSet(DataType.MANUF_DATA) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.MANUF_DATA.name()); count++;
        }
        if( isSet(DataType.DEVICE_CLASS) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.DEVICE_CLASS.name()); count++;
        }
        if( isSet(DataType.APPEARANCE) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.APPEARANCE.name()); count++;
        }
        if( isSet(DataType.HASH) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.HASH.name()); count++;
        }
        if( isSet(DataType.RANDOMIZER) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.RANDOMIZER.name()); count++;
        }
        if( isSet(DataType.DEVICE_ID) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.DEVICE_ID.name()); count++;
        }
        if( isSet(DataType.SERVICE_UUID) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(DataType.SERVICE_UUID.name()); count++;
        }
        if( 1 < count ) {
            out.insert(0, "[");
            out.append("]");
        }
        return out.toString();
    }
}
