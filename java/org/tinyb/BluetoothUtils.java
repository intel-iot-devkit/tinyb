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

public class BluetoothUtils {

    /**
     * Returns current monotonic time in milliseconds.
     */
    public static native long getCurrentMilliseconds();

    /**
     * Returns a hex string representation
     *
     * @param bytes the byte array to represent
     * @param lsbFirst if true, orders LSB left -> MSB right, usual for byte streams.
     *                 Otherwise orders MSB left -> LSB right, usual for readable integer values.
     * @param leading0X if true, prepends the value identifier '0x'
     */
    public static String bytesHexString(final byte[] bytes, final boolean lsbFirst, final boolean leading0X) {
        final char[] hexChars;
        final int offset;
        if( leading0X ) {
            offset = 2;
            hexChars = new char[2 + bytes.length * 2];
            hexChars[0] = '0';
            hexChars[1] = 'x';
        } else {
            offset = 0;
            hexChars = new char[bytes.length * 2];
        }

        if( lsbFirst ) {
            // LSB left -> MSB right
            for (int j = 0; j < bytes.length; j++) {
                final int v = bytes[j] & 0xFF;
                hexChars[offset + j * 2] = HEX_ARRAY[v >>> 4];
                hexChars[offset + j * 2 + 1] = HEX_ARRAY[v & 0x0F];
            }
        } else {
            // MSB left -> LSB right
            for (int j = bytes.length-1; j >= 0; j--) {
                final int v = bytes[j] & 0xFF;
                hexChars[offset + j * 2] = HEX_ARRAY[v >>> 4];
                hexChars[offset + j * 2 + 1] = HEX_ARRAY[v & 0x0F];
            }
        }
        return new String(hexChars);
    }
    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();


    /**
     * Returns all valid consecutive UTF-8 characters within buffer
     * in the range offset -> size or until EOS.
     * <p>
     * In case a non UTF-8 character has been detected,
     * the content will be cut off and the decoding loop ends.
     * </p>
     * <p>
     * Method utilizes a finite state machine detecting variable length UTF-8 codes.
     * See <a href="http://bjoern.hoehrmann.de/utf-8/decoder/dfa/">Bjoern Hoehrmann's site</a> for details.
     * </p>
     */
    public static native String decodeUTF8String(final byte[] buffer, final int offset, final int size);

}
