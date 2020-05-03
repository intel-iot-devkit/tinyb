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

public enum EIRDataType {
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

        EIRDataType(final int v) { mask = v; }
        public static EIRDataType create(final int v) {
            final EIRDataType r = NONE;
            r.mask = v;
            return r;
        }

        public int mask;
        public boolean isSet(final EIRDataType bit) { return 0 != ( mask & bit.mask ); }
        public void set(final EIRDataType bit) { mask = mask | bit.mask; }

        public String toString() {
            int count = 0;
            final StringBuilder out = new StringBuilder();
            if( isSet(EVT_TYPE) ) {
                out.append(EVT_TYPE.name()); count++;
            }
            if( isSet(BDADDR_TYPE) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(BDADDR_TYPE.name()); count++;
            }
            if( isSet(BDADDR) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(BDADDR.name()); count++;
            }
            if( isSet(FLAGS) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(FLAGS.name()); count++;
            }
            if( isSet(NAME) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(NAME.name()); count++;
            }
            if( isSet(NAME_SHORT) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(NAME_SHORT.name()); count++;
            }
            if( isSet(RSSI) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(RSSI.name()); count++;
            }
            if( isSet(TX_POWER) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(TX_POWER.name()); count++;
            }
            if( isSet(MANUF_DATA) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(MANUF_DATA.name()); count++;
            }
            if( isSet(DEVICE_CLASS) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(DEVICE_CLASS.name()); count++;
            }
            if( isSet(APPEARANCE) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(APPEARANCE.name()); count++;
            }
            if( isSet(HASH) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(HASH.name()); count++;
            }
            if( isSet(RANDOMIZER) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(RANDOMIZER.name()); count++;
            }
            if( isSet(DEVICE_ID) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(DEVICE_ID.name()); count++;
            }
            if( isSet(SERVICE_UUID) ) {
                if( 0 < count ) { out.append(", "); }
                out.append(SERVICE_UUID.name()); count++;
            }
            if( 1 < count ) {
                out.insert(0, "[");
                out.append("]");
            }
            return out.toString();
        }
}
