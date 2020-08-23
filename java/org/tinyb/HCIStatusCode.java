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
public enum HCIStatusCode {
        SUCCESS((byte) 0x00),
        UNKNOWN_HCI_COMMAND((byte) 0x01),
        UNKNOWN_CONNECTION_IDENTIFIER((byte) 0x02),
        HARDWARE_FAILURE((byte) 0x03),
        PAGE_TIMEOUT((byte) 0x04),
        AUTHENTICATION_FAILURE((byte) 0x05),
        PIN_OR_KEY_MISSING((byte) 0x06),
        MEMORY_CAPACITY_EXCEEDED((byte) 0x07),
        CONNECTION_TIMEOUT((byte) 0x08),
        CONNECTION_LIMIT_EXCEEDED((byte) 0x09),
        SYNC_DEVICE_CONNECTION_LIMIT_EXCEEDED((byte) 0x0a),
        CONNECTION_ALREADY_EXISTS((byte) 0x0b),
        COMMAND_DISALLOWED((byte) 0x0c),
        CONNECTION_REJECTED_LIMITED_RESOURCES((byte) 0x0d),
        CONNECTION_REJECTED_SECURITY((byte) 0x0e),
        CONNECTION_REJECTED_UNACCEPTABLE_BD_ADDR((byte) 0x0f),
        CONNECTION_ACCEPT_TIMEOUT_EXCEEDED((byte) 0x10),
        UNSUPPORTED_FEATURE_OR_PARAM_VALUE((byte) 0x11),
        INVALID_HCI_COMMAND_PARAMETERS((byte) 0x12),
        REMOTE_USER_TERMINATED_CONNECTION((byte) 0x13),
        REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES((byte) 0x14),
        REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF((byte) 0x15),
        CONNECTION_TERMINATED_BY_LOCAL_HOST((byte) 0x16),
        REPEATED_ATTEMPTS((byte) 0x17),
        PAIRING_NOT_ALLOWED((byte) 0x18),
        UNKNOWN_LMP_PDU((byte) 0x19),
        UNSUPPORTED_REMOTE_OR_LMP_FEATURE((byte) 0x1a),
        SCO_OFFSET_REJECTED((byte) 0x1b),
        SCO_INTERVAL_REJECTED((byte) 0x1c),
        SCO_AIR_MODE_REJECTED((byte) 0x1d),
        INVALID_LMP_OR_LL_PARAMETERS((byte) 0x1e),
        UNSPECIFIED_ERROR((byte) 0x1f),
        UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE((byte) 0x20),
        ROLE_CHANGE_NOT_ALLOWED((byte) 0x21),
        LMP_OR_LL_RESPONSE_TIMEOUT((byte) 0x22),
        LMP_OR_LL_COLLISION((byte) 0x23),
        LMP_PDU_NOT_ALLOWED((byte) 0x24),
        ENCRYPTION_MODE_NOT_ACCEPTED((byte) 0x25),
        LINK_KEY_CANNOT_BE_CHANGED((byte) 0x26),
        REQUESTED_QOS_NOT_SUPPORTED((byte) 0x27),
        INSTANT_PASSED((byte) 0x28),
        PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED((byte) 0x29),
        DIFFERENT_TRANSACTION_COLLISION((byte) 0x2a),
        QOS_UNACCEPTABLE_PARAMETER((byte) 0x2c),
        QOS_REJECTED((byte) 0x2d),
        CHANNEL_ASSESSMENT_NOT_SUPPORTED((byte) 0x2e),
        INSUFFICIENT_SECURITY((byte) 0x2f),
        PARAMETER_OUT_OF_RANGE((byte) 0x30),
        ROLE_SWITCH_PENDING((byte) 0x32),
        RESERVED_SLOT_VIOLATION((byte) 0x34),
        ROLE_SWITCH_FAILED((byte) 0x35),
        EIR_TOO_LARGE((byte) 0x36),
        SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST((byte) 0x37),
        HOST_BUSY_PAIRING((byte) 0x38),
        CONNECTION_REJECTED_NO_SUITABLE_CHANNEL((byte) 0x39),
        CONTROLLER_BUSY((byte) 0x3a),
        UNACCEPTABLE_CONNECTION_PARAM((byte) 0x3b),
        ADVERTISING_TIMEOUT((byte) 0x3c),
        CONNECTION_TERMINATED_MIC_FAILURE((byte) 0x3d),
        CONNECTION_EST_FAILED_OR_SYNC_TIMEOUT((byte) 0x3e),
        MAX_CONNECTION_FAILED ((byte) 0x3f),
        COARSE_CLOCK_ADJ_REJECTED((byte) 0x40),
        TYPE0_SUBMAP_NOT_DEFINED((byte) 0x41),
        UNKNOWN_ADVERTISING_IDENTIFIER((byte) 0x42),
        LIMIT_REACHED((byte) 0x43),
        OPERATION_CANCELLED_BY_HOST((byte) 0x44),
        PACKET_TOO_LONG((byte) 0x45),
        INTERNAL_TIMEOUT((byte) 0xfd),
        INTERNAL_FAILURE((byte) 0xfe),
        UNKNOWN((byte) 0xff);

    public final byte value;

    /**
     * Maps the specified name to a constant of HCIStatusCode.
     * <p>
     * Implementation simply returns {@link #valueOf(String)}.
     * This maps the constant names itself to their respective constant.
     * </p>
     * @param name the string name to be mapped to a constant of this enum type.
     * @return the corresponding constant of this enum type.
     * @throws IllegalArgumentException if the specified name can't be mapped to a constant of this enum type
     *                                  as described above.
     */
    public static HCIStatusCode get(final String name) throws IllegalArgumentException {
        return valueOf(name);
    }

    /**
     * Maps the specified integer value to a constant of HCIStatusCode.
     * @param value the integer value to be mapped to a constant of this enum type.
     * @return the corresponding constant of this enum type.
     * @throws IllegalArgumentException if the specified name can't be mapped to a constant of this enum type
     *                                  as described above.
     */
    public static HCIStatusCode get(final byte value) throws IllegalArgumentException {
        switch( value ) {
            case (byte) 0x00: return SUCCESS;
            case (byte) 0x01: return UNKNOWN_HCI_COMMAND;
            case (byte) 0x02: return UNKNOWN_CONNECTION_IDENTIFIER;
            case (byte) 0x03: return HARDWARE_FAILURE;
            case (byte) 0x04: return PAGE_TIMEOUT;
            case (byte) 0x05: return AUTHENTICATION_FAILURE;
            case (byte) 0x06: return PIN_OR_KEY_MISSING;
            case (byte) 0x07: return MEMORY_CAPACITY_EXCEEDED;
            case (byte) 0x08: return CONNECTION_TIMEOUT;
            case (byte) 0x09: return CONNECTION_LIMIT_EXCEEDED;
            case (byte) 0x0a: return SYNC_DEVICE_CONNECTION_LIMIT_EXCEEDED;
            case (byte) 0x0b: return CONNECTION_ALREADY_EXISTS;
            case (byte) 0x0c: return COMMAND_DISALLOWED;
            case (byte) 0x0d: return CONNECTION_REJECTED_LIMITED_RESOURCES;
            case (byte) 0x0e: return CONNECTION_REJECTED_SECURITY;
            case (byte) 0x0f: return CONNECTION_REJECTED_UNACCEPTABLE_BD_ADDR;
            case (byte) 0x10: return CONNECTION_ACCEPT_TIMEOUT_EXCEEDED;
            case (byte) 0x11: return UNSUPPORTED_FEATURE_OR_PARAM_VALUE;
            case (byte) 0x12: return INVALID_HCI_COMMAND_PARAMETERS;
            case (byte) 0x13: return REMOTE_USER_TERMINATED_CONNECTION;
            case (byte) 0x14: return REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES;
            case (byte) 0x15: return REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF;
            case (byte) 0x16: return CONNECTION_TERMINATED_BY_LOCAL_HOST;
            case (byte) 0x17: return REPEATED_ATTEMPTS;
            case (byte) 0x18: return PAIRING_NOT_ALLOWED;
            case (byte) 0x19: return UNKNOWN_LMP_PDU;
            case (byte) 0x1a: return UNSUPPORTED_REMOTE_OR_LMP_FEATURE;
            case (byte) 0x1b: return SCO_OFFSET_REJECTED;
            case (byte) 0x1c: return SCO_INTERVAL_REJECTED;
            case (byte) 0x1d: return SCO_AIR_MODE_REJECTED;
            case (byte) 0x1e: return INVALID_LMP_OR_LL_PARAMETERS;
            case (byte) 0x1f: return UNSPECIFIED_ERROR;
            case (byte) 0x20: return UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE;
            case (byte) 0x21: return ROLE_CHANGE_NOT_ALLOWED;
            case (byte) 0x22: return LMP_OR_LL_RESPONSE_TIMEOUT;
            case (byte) 0x23: return LMP_OR_LL_COLLISION;
            case (byte) 0x24: return LMP_PDU_NOT_ALLOWED;
            case (byte) 0x25: return ENCRYPTION_MODE_NOT_ACCEPTED;
            case (byte) 0x26: return LINK_KEY_CANNOT_BE_CHANGED;
            case (byte) 0x27: return REQUESTED_QOS_NOT_SUPPORTED;
            case (byte) 0x28: return INSTANT_PASSED;
            case (byte) 0x29: return PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED;
            case (byte) 0x2a: return DIFFERENT_TRANSACTION_COLLISION;
            case (byte) 0x2c: return QOS_UNACCEPTABLE_PARAMETER;
            case (byte) 0x2d: return QOS_REJECTED;
            case (byte) 0x2e: return CHANNEL_ASSESSMENT_NOT_SUPPORTED;
            case (byte) 0x2f: return INSUFFICIENT_SECURITY;
            case (byte) 0x30: return PARAMETER_OUT_OF_RANGE;
            case (byte) 0x32: return ROLE_SWITCH_PENDING;
            case (byte) 0x34: return RESERVED_SLOT_VIOLATION;
            case (byte) 0x35: return ROLE_SWITCH_FAILED;
            case (byte) 0x36: return EIR_TOO_LARGE;
            case (byte) 0x37: return SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST;
            case (byte) 0x38: return HOST_BUSY_PAIRING;
            case (byte) 0x39: return CONNECTION_REJECTED_NO_SUITABLE_CHANNEL;
            case (byte) 0x3a: return CONTROLLER_BUSY;
            case (byte) 0x3b: return UNACCEPTABLE_CONNECTION_PARAM;
            case (byte) 0x3c: return ADVERTISING_TIMEOUT;
            case (byte) 0x3d: return CONNECTION_TERMINATED_MIC_FAILURE;
            case (byte) 0x3e: return CONNECTION_EST_FAILED_OR_SYNC_TIMEOUT;
            case (byte) 0x3f: return MAX_CONNECTION_FAILED;
            case (byte) 0x40: return COARSE_CLOCK_ADJ_REJECTED;
            case (byte) 0x41: return TYPE0_SUBMAP_NOT_DEFINED;
            case (byte) 0x42: return UNKNOWN_ADVERTISING_IDENTIFIER;
            case (byte) 0x43: return LIMIT_REACHED;
            case (byte) 0x44: return OPERATION_CANCELLED_BY_HOST;
            case (byte) 0x45: return PACKET_TOO_LONG;
            case (byte) 0xfd: return INTERNAL_TIMEOUT;
            case (byte) 0xfe: return INTERNAL_FAILURE;
            case (byte) 0xff: return UNKNOWN;
        }
        throw new IllegalArgumentException("Unsupported value "+value);
    }

    HCIStatusCode(final byte v) {
        value = v;
    }
}
