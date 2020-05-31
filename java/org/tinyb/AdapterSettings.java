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
 * Bit mask of '{@link BluetoothAdapter} setting' data fields,
 * indicating a set of related data.
 *
 * @since 2.0.0
 */
public class AdapterSettings {

    /**
     * Bits representing '{@link BluetoothAdapter} setting' data fields.
     *
     * @since 2.0.0
     */
     public enum SettingType {
        NONE               (         0),
        POWERED            (0x00000001),
        CONNECTABLE        (0x00000002),
        FAST_CONNECTABLE   (0x00000004),
        DISCOVERABLE       (0x00000008),
        BONDABLE           (0x00000010),
        LINK_SECURITY      (0x00000020),
        SSP                (0x00000040),
        BREDR              (0x00000080),
        HS                 (0x00000100),
        LE                 (0x00000200),
        ADVERTISING        (0x00000400),
        SECURE_CONN        (0x00000800),
        DEBUG_KEYS         (0x00001000),
        PRIVACY            (0x00002000),
        CONFIGURATION      (0x00004000),
        STATIC_ADDRESS     (0x00008000),
        PHY_CONFIGURATION  (0x00010000);

        SettingType(final int v) { value = v; }
        public final int value;
    }

    public int mask;

    public AdapterSettings(final int v) {
        mask = v;
    }

    public boolean isSet(final SettingType bit) { return 0 != ( mask & bit.value ); }
    public void set(final SettingType bit) { mask = mask | bit.value; }

    public String toString() {
        int count = 0;
        final StringBuilder out = new StringBuilder();
        if( isSet(SettingType.POWERED) ) {
            out.append(SettingType.POWERED.name()); count++;
        }
        if( isSet(SettingType.CONNECTABLE) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.CONNECTABLE.name()); count++;
        }
        if( isSet(SettingType.FAST_CONNECTABLE) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.FAST_CONNECTABLE.name()); count++;
        }
        if( isSet(SettingType.DISCOVERABLE) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.DISCOVERABLE.name()); count++;
        }
        if( isSet(SettingType.BONDABLE) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.BONDABLE.name()); count++;
        }
        if( isSet(SettingType.LINK_SECURITY) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.LINK_SECURITY.name()); count++;
        }
        if( isSet(SettingType.SSP) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.SSP.name()); count++;
        }
        if( isSet(SettingType.BREDR) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.BREDR.name()); count++;
        }
        if( isSet(SettingType.HS) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.HS.name()); count++;
        }
        if( isSet(SettingType.LE) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.LE.name()); count++;
        }
        if( isSet(SettingType.ADVERTISING) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.ADVERTISING.name()); count++;
        }
        if( isSet(SettingType.SECURE_CONN) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.SECURE_CONN.name()); count++;
        }
        if( isSet(SettingType.DEBUG_KEYS) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.DEBUG_KEYS.name()); count++;
        }
        if( isSet(SettingType.PRIVACY) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.PRIVACY.name()); count++;
        }
        if( isSet(SettingType.CONFIGURATION) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.CONFIGURATION.name()); count++;
        }
        if( isSet(SettingType.STATIC_ADDRESS) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.STATIC_ADDRESS.name()); count++;
        }
        if( isSet(SettingType.PHY_CONFIGURATION) ) {
            if( 0 < count ) { out.append(", "); }
            out.append(SettingType.PHY_CONFIGURATION.name()); count++;
        }
        if( 1 < count ) {
            out.insert(0, "[");
            out.append("]");
        }
        return out.toString();
    }
}
