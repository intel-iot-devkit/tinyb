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
 * BT Core Spec v5.2: Vol 1, Part F Controller Error Codes: 1.3 List of Error Codes
 * <p>
 * BT Core Spec v5.2: Vol 1, Part F Controller Error Codes: 2 Error code descriptions
 * </p>
 * @since 2.0.0
 */
public enum HCIErrorCode {
        SUCCESS(0x00),
        UNKNOWN_HCI_COMMAND(0x01),
        UNKNOWN_CONNECTION_IDENTIFIER(0x02),
        HARDWARE_FAILURE(0x03),
        PAGE_TIMEOUT(0x04),
        AUTHENTICATION_FAILURE(0x05),
        PIN_OR_KEY_MISSING(0x06),
        MEMORY_CAPACITY_EXCEEDED(0x07),
        CONNECTION_TIMEOUT(0x08),
        CONNECTION_LIMIT_EXCEEDED(0x09),
        SYNC_DEVICE_CONNECTION_LIMIT_EXCEEDED(0x0a),
        CONNECTION_ALREADY_EXISTS(0x0b),
        COMMAND_DISALLOWED(0x0c),
        CONNECTION_REJECTED_LIMITED_RESOURCES(0x0d),
        CONNECTION_REJECTED_SECURITY(0x0e),
        CONNECTION_REJECTED_UNACCEPTABLE_BD_ADDR(0x0f),
        CONNECTION_ACCEPT_TIMEOUT_EXCEEDED(0x10),
        UNSUPPORTED_FEATURE_OR_PARAM_VALUE(0x11),
        INVALID_HCI_COMMAND_PARAMETERS(0x12),
        REMOTE_USER_TERMINATED_CONNECTION(0x13),
        REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES(0x14),
        REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF(0x15),
        CONNECTION_TERMINATED_BY_LOCAL_HOST(0x16),
        REPEATED_ATTEMPTS(0x17),
        PAIRING_NOT_ALLOWED(0x18),
        UNKNOWN_LMP_PDU(0x19),
        UNSUPPORTED_REMOTE_OR_LMP_FEATURE(0x1a),
        SCO_OFFSET_REJECTED(0x1b),
        SCO_INTERVAL_REJECTED(0x1c),
        SCO_AIR_MODE_REJECTED(0x1d),
        INVALID_LMP_OR_LL_PARAMETERS(0x1e),
        UNSPECIFIED_ERROR(0x1f),
        UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE(0x20),
        ROLE_CHANGE_NOT_ALLOWED(0x21),
        LMP_OR_LL_RESPONSE_TIMEOUT(0x22),
        LMP_OR_LL_COLLISION(0x23),
        LMP_PDU_NOT_ALLOWED(0x24),
        ENCRYPTION_MODE_NOT_ACCEPTED(0x25),
        LINK_KEY_CANNOT_BE_CHANGED(0x26),
        REQUESTED_QOS_NOT_SUPPORTED(0x27),
        INSTANT_PASSED(0x28),
        PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED(0x29),
        DIFFERENT_TRANSACTION_COLLISION(0x2a),
        QOS_UNACCEPTABLE_PARAMETER(0x2c),
        QOS_REJECTED(0x2d),
        CHANNEL_ASSESSMENT_NOT_SUPPORTED(0x2e),
        INSUFFICIENT_SECURITY(0x2f),
        PARAMETER_OUT_OF_RANGE(0x30),
        ROLE_SWITCH_PENDING(0x32),
        RESERVED_SLOT_VIOLATION(0x34),
        ROLE_SWITCH_FAILED(0x35),
        EIR_TOO_LARGE(0x36),
        SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST(0x37),
        HOST_BUSY_PAIRING(0x38),
        CONNECTION_REJECTED_NO_SUITABLE_CHANNEL(0x39),
        CONTROLLER_BUSY(0x3a),
        UNACCEPTABLE_CONNECTION_PARAM(0x3b),
        ADVERTISING_TIMEOUT(0x3c),
        CONNECTION_TERMINATED_MIC_FAILURE(0x3d),
        CONNECTION_EST_FAILED_OR_SYNC_TIMETOUT(0x3e),
        MAX_CONNECTION_FAILED (0x3f),
        COARSE_CLOCK_ADJ_REJECTED(0x40),
        TYPE0_SUBMAP_NOT_DEFINED(0x41),
        UNKNOWN_ADVERTISING_IDENTIFIER(0x42),
        LIMIT_REACHED(0x43),
        OPERATION_CANCELLED_BY_HOST(0x44),
        PACKET_TOO_LONG(0x45),
        INTERNAL_FAILURE(0xff);

    public final int value;

    /**
     * Maps the specified name to a constant of HCIErrorCode.
     * <p>
     * Implementation simply returns {@link #valueOf(String)}.
     * This maps the constant names itself to their respective constant.
     * </p>
     * @param name the string name to be mapped to a constant of this enum type.
     * @return the corresponding constant of this enum type.
     * @throws IllegalArgumentException if the specified name can't be mapped to a constant of this enum type
     *                                  as described above.
     */
    public static HCIErrorCode get(final String name) throws IllegalArgumentException {
        return valueOf(name);
    }

    /**
     * Maps the specified integer value to a constant of HCIErrorCode.
     * @param value the integer value to be mapped to a constant of this enum type.
     * @return the corresponding constant of this enum type.
     * @throws IllegalArgumentException if the specified name can't be mapped to a constant of this enum type
     *                                  as described above.
     */
    public static HCIErrorCode get(final int value) throws IllegalArgumentException {
        final HCIErrorCode[] enums = HCIErrorCode.values();
        for(int i=0; i<enums.length; i++) {
            if( value == enums[i].value ) {
                return enums[i];
            }
        }
        throw new IllegalArgumentException("Unsupported value "+value);
    }

    HCIErrorCode(final int v) {
        value = v;
    }
}
