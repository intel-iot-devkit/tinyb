/*
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

#ifndef ATT_PDU_TYPES_HPP_
#define ATT_PDU_TYPES_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>

#include "UUID.hpp"
#include "BTTypes.hpp"

#include "OctetTypes.hpp"

/**
 * Direct-BT provides direct Bluetooth programming without intermediate layers
 * targeting high-performance reliable Bluetooth support.
 *
 * By having least system and userspace dependencies and no communication overhead,
 * Direct-BT shall be suitable for embedded device configurations besides others.
 *
 * Direct-BT implements the following layers
 * - DBTManager for adapter management and device discovery
 *   - Using *BlueZ Kernel Manager Control Channel*
 * - HCIHandler implements native HCI handling (DBTDevice connect/disconnect w/ tracking, discovery, etc)
 * - *Basic HCI* via HCIComm for connection
 * - *ATT PDU* AttPDUMsg via L2CAP for low level packet communication
 * - *GATT Support* via GATTHandler using AttPDUMsg over L2CAPComm, providing
 *   -  GATTService
 *   -  GATTCharacteristic
 *   -  GATTDescriptor
 *
 * DBTManager still utilizes the *BlueZ Kernel Manager Control Channel*
 * adapter configuration and device discovery.
 * To remove potential side-effects and this last Linux dependency,
 * we will have DBTManager using direct HCI programming via HCIHandler in the future.
 *
 * - - - - - - - - - - - - - - -
 *
 * From a user perspective the following hierarchy is provided
 * - DBTAdapter has no or more
 *   - DBTDevice has no or more
 *     - GATTService has no or more
 *       - GATTCharacteristic has no or more
 *         - GATTDescriptor
 *
 * - - - - - - - - - - - - - - -
 *
 * Object lifecycle with all instances having a back-references to their owner
 * - DBTManager singleton instance for all
 * - DBTAdapter ownership by user
 *   - DBTDevice ownership by DBTAdapter
 *     - GATTHandler optional ownership by DBTDevice
 *
 * - GATTHandler with DBTDevice reference
 *   - GATTService ownership by GATTHandler
 *     - GATTCharacteristic ownership by GATTService
 *       - GATTDescriptor ownership by GATTCharacteristic
 *
 * - - - - - - - - - - - - - - -
 *
 * Main event listener can be attached to these objects
 * which maintain a set of unique listener instances without duplicates.
 *
 * - DBTAdapter
 *   - AdapterStatusListener
 *
 * - GATTHandler
 *   - GATTCharacteristicListener
 *
 * Other API attachment method exists for GATTCharacteristicListener,
 * however, they only exists for convenience and end up to be attached to GATTHandler.
 *
 * - - - - - - - - - - - - - - -
 *
 * BT Core Spec v5.2:  Vol 3, Part A L2CAP Spec: 7.9 PRIORITIZING DATA OVER HCI
 *
 * > In order for guaranteed channels to meet their guarantees,
 * > L2CAP should prioritize traffic over the HCI transport in devices that support HCI.
 * > Packets for Guaranteed channels should receive higher priority than packets for Best Effort channels.
 *
 * *As we have experience slower GATT communication w/o HCI connection,
 * Direct-BT enforces the HCI connection.*
 *
 * - - - - - - - - - - - - - - -
 *
 * Module ATTPDUTypes:
 *
 * - BT Core Spec v5.2: Vol 3, Part F Attribute Protocol (ATT)
 */
namespace direct_bt {

    class AttException : public RuntimeException {
        protected:
            AttException(std::string const type, std::string const m, const char* file, int line) noexcept
            : RuntimeException(type, m, file, line) {}

        public:
            AttException(std::string const m, const char* file, int line) noexcept
            : RuntimeException("AttException", m, file, line) {}
    };

    class AttOpcodeException : public AttException {
        public:
            AttOpcodeException(std::string const m, const char* file, int line) noexcept
            : AttException("AttOpcodeException", m, file, line) {}
    };

    class AttValueException : public AttException {
        public:
            AttValueException(std::string const m, const char* file, int line) noexcept
            : AttException("AttValueException", m, file, line) {}
    };

    /**
     * ATT PDU Overview
     * ================
     * Handles the Attribute Protocol (ATT) using Protocol Data Unit (PDU)
     * encoded messages over L2CAP channel.
     * <p>
     * Implementation uses persistent memory w/ ownership
     * copying PDU data to allow intermediate pipe processing.
     * </p>
     * <p>
     *
     * Vol 3, Part F 2 - Protocol Overview pp
     * ---------------------------------------
     * One attribute := { UUID type; uint16_t handle; permissions for higher layer; },
     * where
     *
     * UUID is an official assigned number,
     *
     * handle uniquely references an attribute on a server for client R/W access,
     * see Vol 3, Part F 3.4.4 - 3.4.6, also 3.4.7 (notified/indicated),
     * 3.4.3 (discovery) and 3.2.5 (permissions).
     *
     * Client sends ATT requests to a server, which shall respond to all.
     *
     * A device can take client and server roles concurrently.
     *
     * One server per device, ATT handle is unique for all supported bearers.
     * For each client, server has one set of ATTs.
     * The server (and hence device) can support multiple clients.
     *
     * Services are distinguished by range of handles for each service.
     * Discovery is of these handle ranges is defined by a higher layer spec.
     *
     * ATT Protocol has notification and indication capabilities for efficient
     * ATT value promotion to client w/o reading them (Vol 3, Part F 3.3).
     *
     * All ATT Protocol requests sent over an ATT bearer.
     * Multiple ATT bearers can be established between two devices.
     * Each ATT bearer uses a separate L2CAP channel an can have different configurations.
     *
     * For LE a single ATT bearer using a fixed L2CAP channel is available ASAP after
     * ACL connection is established.
     * Additional ATT bearers can be established using L2CAP (Vol 3, Part F 3.2.11).
     * </p>
     *
     * <p>
     * Vol 3, Part F 3 - Basics and Types
     * ------------------------------------
     * ATT handle is uint16_t and valid if > 0x0000, max is 0xffff.
     * ATT handle is unique per server.
     *
     * ATT value (Vol 3, Part F 3.2.4)
     *
     * - ATT value is uint8_t array of fixed or variable length.
     *
     * - ATT values might be too large for a single PDU,
     *   hence it must be sent using multiple PDUs.
     *
     * - ATT value encoding is defined by the ATT type (UUID).
     *
     * - ATT value transmission done via request, response,
     *   notification or indication
     *
     * - ATT value variable length is implicit by PDU carrying packet (PDU parent),
     *   implying:
     *   - One ATT value per ATT request... unless ATT values have fixed length.
     *   - Only one ATT value with variable length in a request...
     *   - L2CAP preserves DGRAM boundaries
     *
     *   Some PDUs include the ATT value length, for which above limitations don't apply.
     *
     *   Maximum length of an attribute value shall be 512 bytes (Vol 3, Part F 3.2.8),
     *   spread across multiple PDUs.
     * </p>
     *
     * - BT Core Spec v5.2: Vol 3, Part A: BT Logical Link Control and Adaption Protocol (L2CAP)
     *
     * - BT Core Spec v5.2: Vol 3, Part F Attribute Protocol (ATT)
     *
     * - BT Core Spec v5.2: Vol 3, Part F 3 ATT PDUs (Protocol Data Unit)
     *
     * - BT Core Spec v5.2: Vol 3, Part F 3.3 ATT PDUs
     *
     * - BT Core Spec v5.2: Vol 3, Part F 4 Security Considerations
     *
     * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     *
     * AttPDUMsg Base Class
     * =====================
     * Attribute Protocol (ATT)'s Protocol Data Unit (PDU) message
     * Vol 3, Part F 3.3 and Vol 3, Part F 3.4
     * <p>
     * Little endian, however, ATT value endianess is defined by above layer.
     * </p>
     * <p>
     * ATT_MTU Vol 3, Part F 3.2.8
     * Maximum size of any packet sent. Higher layer spec defines the default ATT_MTU value.
     * LE L2CAP GATT ATT_MTU is 23 bytes (Vol 3, Part G 5.2.1).
     *
     * Maximum length of an attribute value shall be 512 bytes (Vol 3, Part F 3.2.8),
     * spread across multiple PDUs.
     * </p>
     * <p>
     * ATT PDU Format Vol 3, Part F 3.3.1
     * -----------------------------------
     * ```
     *   { uint8_t opcode, uint8_t param[0..ATT_MTU-X], uint8_t auth_sig[0||12] }
     * ```
     * with
     * ```
     *   opcode bits{ 0-5 method, 6 command-flag, 7 auth-sig-flag }
     * ```
     * and
     * ```
     *   X =  1 if auth-sig flag of ATT-opcode is 0, or
     *   X = 13 if auth-sig flag of ATT-opcode is 1.
     * ```
     * </p>
     */
    class AttPDUMsg
    {
        public:
            /** ATT Opcode Summary Vol 3, Part F 3.4.8 */
            enum Opcode : uint8_t {
                ATT_PDU_UNDEFINED               = 0x00, // our own pseudo opcode, indicating no ATT PDU message

                ATT_METHOD_MASK                 = 0x3F, // bits 0 .. 5
                ATT_COMMAND_FLAG                = 0x40, // bit 6 (counting from 0)
                ATT_AUTH_SIGNATURE_FLAG         = 0x80, // bit 7 (counting from 0)

                ATT_ERROR_RSP                   = 0x01,
                ATT_EXCHANGE_MTU_REQ            = 0x02,
                ATT_EXCHANGE_MTU_RSP            = 0x03,
                ATT_FIND_INFORMATION_REQ        = 0x04,
                ATT_FIND_INFORMATION_RSP        = 0x05,
                ATT_FIND_BY_TYPE_VALUE_REQ      = 0x06,
                ATT_FIND_BY_TYPE_VALUE_RSP      = 0x07,
                ATT_READ_BY_TYPE_REQ            = 0x08,
                ATT_READ_BY_TYPE_RSP            = 0x09,
                ATT_READ_REQ                    = 0x0A,
                ATT_READ_RSP                    = 0x0B,
                ATT_READ_BLOB_REQ               = 0x0C,
                ATT_READ_BLOB_RSP               = 0x0D,
                ATT_READ_MULTIPLE_REQ           = 0x0E,
                ATT_READ_MULTIPLE_RSP           = 0x0F,
                ATT_READ_BY_GROUP_TYPE_REQ      = 0x10,
                ATT_READ_BY_GROUP_TYPE_RSP      = 0x11,
                ATT_WRITE_REQ                   = 0x12,
                ATT_WRITE_RSP                   = 0x13,
                ATT_WRITE_CMD                   = ATT_WRITE_REQ + ATT_COMMAND_FLAG, // = 0x52
                ATT_PREPARE_WRITE_REQ           = 0x16,
                ATT_PREPARE_WRITE_RSP           = 0x17,
                ATT_EXECUTE_WRITE_REQ           = 0x18,
                ATT_EXECUTE_WRITE_RSP           = 0x19,

                ATT_READ_MULTIPLE_VARIABLE_REQ  = 0x20,
                ATT_READ_MULTIPLE_VARIABLE_RSP  = 0x21,
                ATT_MULTIPLE_HANDLE_VALUE_NTF   = 0x23,

                ATT_HANDLE_VALUE_NTF            = 0x1B,
                ATT_HANDLE_VALUE_IND            = 0x1D,
                ATT_HANDLE_VALUE_CFM            = 0x1E,

                ATT_SIGNED_WRITE_CMD            = ATT_WRITE_REQ + ATT_COMMAND_FLAG + ATT_AUTH_SIGNATURE_FLAG // = 0xD2
            };

            static std::string getOpcodeString(const Opcode opc);

        protected:
            void checkOpcode(const Opcode expected) const
            {
                const Opcode has = getOpcode();
                if( expected != has ) {
                    throw AttOpcodeException("Has opcode "+uint8HexString(has, true)+" "+getOpcodeString(has)+
                                     ", but expected "+uint8HexString(expected, true)+" "+getOpcodeString(expected), E_FILE_LINE);
                }
            }
            void checkOpcode(const Opcode exp1, const Opcode exp2) const
            {
                const Opcode has = getOpcode();
                if( exp1 != has && exp2 != has ) {
                    throw AttOpcodeException("Has opcode "+uint8HexString(has, true)+" "+getOpcodeString(has)+
                                     ", but expected either "+uint8HexString(exp1, true)+" "+getOpcodeString(exp1)+
                                     " or  "+uint8HexString(exp1, true)+" "+getOpcodeString(exp1), E_FILE_LINE);
                }
            }

            virtual std::string baseString() const {
                return "opcode="+uint8HexString(getOpcode(), true)+" "+getOpcodeString()+
                        ", size[total="+std::to_string(pdu.getSize())+", param "+std::to_string(getPDUParamSize())+"]";
            }
            virtual std::string valueString() const {
                return "size "+std::to_string(getPDUValueSize())+", data "
                        +bytesHexString(pdu.get_ptr(), getPDUValueOffset(), getPDUValueSize(), true /* lsbFirst */, true /* leading0X */);
            }

        public:
            /** actual received PDU */
            POctets pdu;

            /** creation timestamp in milliseconds */
            const int64_t ts_creation;

            /**
             * Return a newly created specialized instance pointer to base class.
             * <p>
             * Returned memory reference is managed by caller (delete etc)
             * </p>
             */
            static AttPDUMsg* getSpecialized(const uint8_t * buffer, int const buffer_size);

            /** Persistent memory, w/ ownership ..*/
            AttPDUMsg(const uint8_t* source, const int size)
                : pdu(source, std::max(1, size)), ts_creation(getCurrentMilliseconds())
            {
                pdu.check_range(0, getPDUMinSize());
            }

            /** Persistent memory, w/ ownership ..*/
            AttPDUMsg(const Opcode opc, const int size)
                : pdu(std::max(1, size)), ts_creation(getCurrentMilliseconds())
            {
                pdu.put_uint8(0, opc);
                pdu.check_range(0, getPDUMinSize());
            }

            AttPDUMsg(const AttPDUMsg &o) noexcept = default;
            AttPDUMsg(AttPDUMsg &&o) noexcept = default;
            AttPDUMsg& operator=(const AttPDUMsg &o) noexcept = default;
            AttPDUMsg& operator=(AttPDUMsg &&o) noexcept = default;

            virtual ~AttPDUMsg() {}

            /** ATT PDU Format Vol 3, Part F 3.3.1 */
            Opcode getOpcode() const {
                return static_cast<Opcode>(pdu.get_uint8(0));
            }
            std::string getOpcodeString() const { return getOpcodeString(getOpcode()); }

            /** ATT PDU Format Vol 3, Part F 3.3.1 */
            Opcode getOpMethod() const {
                return static_cast<Opcode>(getOpcode() & ATT_METHOD_MASK);
            }

            /** ATT PDU Format Vol 3, Part F 3.3.1 */
            bool getOpCommandFlag() const {
                return static_cast<Opcode>(getOpcode() & ATT_COMMAND_FLAG);
            }

            /** ATT PDU Format Vol 3, Part F 3.3.1 */
            bool getOpAuthSigFlag() const {
                return static_cast<Opcode>(getOpcode() & ATT_AUTH_SIGNATURE_FLAG);
            }

            /**
             * ATT PDU Format Vol 3, Part F 3.3.1
             * <p>
             * The ATT Authentication Signature size in octets.
             * </p>
             * <p>
             * This auth-signature comes at the very last of the PDU.
             * </p>
             */
            int getAuthSigSize() const {
                return getOpAuthSigFlag() ? 12 : 0;
            }

            /**
             * ATT PDU Format Vol 3, Part F 3.3.1
             * <p>
             * The ATT PDU parameter size in octets
             * less opcode (1 byte) and auth-signature (0 or 12 bytes).
             * </p>
             * <pre>
             *   param-size := pdu.size - getAuthSigSize() - 1
             * </pre>
             * <p>
             * Note that the PDU parameter include the PDU value below.
             * </p>
             * <p>
             * Note that the optional auth-signature is at the end of the PDU.
             * </p>
             */
            int getPDUParamSize() const {
                return pdu.getSize() - getAuthSigSize() - 1 /* opcode */;
            }

            /**
             * Returns the octet offset to the value segment in this PDU
             * including the mandatory opcode,
             * i.e. the number of octets until the first value octet.
             * <p>
             * Note that the ATT PDU value is part of the PDU param,
             * where it is the last segment.
             * </p>
             * <p>
             * The value offset is ATT PDU specific and may point
             * to the variable user data post handle etc within the PDU Param block.
             * </p>
             * <p>
             * Note that the opcode must be included in the implementation,
             * as it may be used to reference the value in the pdu
             * conveniently.
             * </p>
             */
            virtual int getPDUValueOffset() const { return 1; /* default: opcode */ }

            /**
             * Returns this PDU's minimum size, i.e.
             * <pre>
             *   opcode + param - value + auth_signature
             * </pre>
             * Value is excluded as it might be flexible.
             */
            int getPDUMinSize() const {
                return getPDUValueOffset() + getAuthSigSize();
            }

            /**
             * Returns the octet size of the value attributes in this PDI,
             * i.e. getPDUParamSize() - getPDUValueOffset() + 1.
             * <p>
             * Note that the opcode size of 1 octet is re-added as included in getPDUValueOffset()
             * for convenience but already taken-off in getPDUParamSize() for spec compliance!
             * </p>
             * <pre>
             *   value-size := param-size - value-offset + 1
             *   param-size := pdu.size - getAuthSigSize() - 1
             *
             *   value-size := pdu.size - getAuthSigSize() - 1 - value-offset + 1
             *   value-size := pdu.size - getAuthSigSize() - value-offset
             * </pre>
             */
            int getPDUValueSize() const { return getPDUParamSize() - getPDUValueOffset() + 1; }

            /**
             * Returns the theoretical maximum value size of a PDU.
             * <pre>
             *  ATT_MTU - getAuthSigSize() - value-offset
             * </pre>
             */
            int getMaxPDUValueSize(const int mtu) const {
                return mtu - getAuthSigSize() - getPDUValueOffset();
            }

            virtual std::string getName() const {
                return "AttPDUMsg";
            }

            virtual std::string toString() const {
                return getName()+"["+baseString()+", value["+valueString()+"]]";
            }
    };

    /**
     * Our own pseudo opcode, indicating no ATT PDU message.
     * <p>
     * ATT_PDU_UNDEFINED
     * </p>
     */
    class AttPDUUndefined: public AttPDUMsg
    {
        public:
            AttPDUUndefined(const uint8_t* source, const int length) : AttPDUMsg(source, length) {
                checkOpcode(ATT_PDU_UNDEFINED);
            }

            /** opcode */
            int getPDUValueOffset() const override { return 1; }

            std::string getName() const override {
                return "AttPDUUndefined";
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.1.1
     * <p>
     * ATT_ERROR_RSP (ATT Opcode 0x01)
     * </p>
     */
    class AttErrorRsp: public AttPDUMsg
    {
        public:
            enum ErrorCode : uint8_t {
                INVALID_HANDLE              = 0x01,
                NO_READ_PERM                = 0x02,
                NO_WRITE_PERM               = 0x03,
                INVALID_PDU                 = 0x04,
                INSUFF_AUTHENTICATION       = 0x05,
                UNSUPPORTED_REQUEST         = 0x06,
                INVALID_OFFSET              = 0x07,
                INSUFF_AUTHORIZATION        = 0x08,
                PREPARE_QUEUE_FULL          = 0x09,
                ATTRIBUTE_NOT_FOUND         = 0x0A,
                ATTRIBUTE_NOT_LONG          = 0x0B,
                INSUFF_ENCRYPTION_KEY_SIZE  = 0x0C,
                INVALID_ATTRIBUTE_VALUE_LEN = 0x0D,
                UNLIKELY_ERROR              = 0x0E,
                INSUFF_ENCRYPTION           = 0x0F,
                UNSUPPORTED_GROUP_TYPE      = 0x10,
                INSUFFICIENT_RESOURCES      = 0x11,
                DB_OUT_OF_SYNC              = 0x12,
                FORBIDDEN_VALUE             = 0x13
            };

            static std::string getPlainErrorString(const ErrorCode errorCode);

            AttErrorRsp(const uint8_t* source, const int length) : AttPDUMsg(source, length) {
                checkOpcode(ATT_ERROR_RSP);
            }

            /** opcode + reqOpcodeCause + handleCause + errorCode */
            int getPDUValueOffset() const override { return 1 + 1 + 2 + 1; }

            uint8_t getRequestedOpcodeCause() const {
                return pdu.get_uint8(1);
            }

            uint16_t getHandleCause() const {
                return pdu.get_uint16(2);
            }

            ErrorCode getErrorCode() const {
                return static_cast<ErrorCode>(pdu.get_uint8(4));
            }

            std::string getErrorString() const {
                const ErrorCode ec = getErrorCode();
                return uint8HexString(ec, true) + ": " + getPlainErrorString(ec);
            }

            std::string getName() const override {
                return "AttErrorRsp";
            }

        protected:
            std::string valueString() const override {
                return getErrorString();
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.2.2
     * <p>
     * ATT_EXCHANGE_MTU_REQ, ATT_EXCHANGE_MTU_RSP
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.3.1 Exchange MTU (Server configuration)
     * </p>
     */
    class AttExchangeMTU: public AttPDUMsg
    {
        private:
            uint8_t _data[1+2];

        public:
            AttExchangeMTU(const uint8_t* source, const int length) : AttPDUMsg(source, length) {
                checkOpcode(ATT_EXCHANGE_MTU_RSP);
            }

            AttExchangeMTU(const uint16_t mtuSize)
            : AttPDUMsg(ATT_EXCHANGE_MTU_REQ, 1+2)
            {
                pdu.put_uint16(1, mtuSize);
            }

            /** opcode + mtu-size */
            int getPDUValueOffset() const override { return 1+2; }

            uint16_t getMTUSize() const {
                return pdu.get_uint16(1);
            }

            std::string getName() const override {
                return "AttExchangeMTU";
            }

        protected:
            std::string valueString() const override {
                return "mtu "+std::to_string(getMTUSize());
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.4.3
     * <p>
     * ATT_READ_REQ
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.1 Read Characteristic Value
     * </p>
     */
    class AttReadReq : public AttPDUMsg
    {
        public:
            AttReadReq(const uint16_t handle)
            : AttPDUMsg(ATT_READ_REQ, 1+2)
            {
                pdu.put_uint16(1, handle);
            }

            /** opcode + handle */
            int getPDUValueOffset() const override { return 1+2; }

            uint16_t getHandle() const {
                return pdu.get_uint16( 1 );
            }

            std::string getName() const override {
                return "AttReadReq";
            }

        protected:
            std::string valueString() const override {
                return "handle "+uint16HexString(getHandle(), true);
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.4.4
     * <p>
     * ATT_READ_RSP (ATT Opcode 0x0B)
     * </p>
     * <p>
     * If expected value size exceeds getValueSize(), continue with ATT_READ_BLOB_REQ (3.4.4.5),
     * see shallReadBlobReq(..)
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.1 Read Characteristic Value
     * </p>
     */
    class AttReadRsp: public AttPDUMsg
    {
        private:
            const TOctetSlice view;

        public:
            static bool instanceOf();

            AttReadRsp(const uint8_t* source, const int length)
            : AttPDUMsg(source, length), view(pdu, getPDUValueOffset(), getPDUValueSize()) {
                checkOpcode(ATT_READ_RSP);
            }

            /** opcode */
            int getPDUValueOffset() const override { return 1; }

            uint8_t const * getValuePtr() const { return pdu.get_ptr(getPDUValueOffset()); }

            TOctetSlice const & getValue() const { return view; }

            std::string getName() const override {
                return "AttReadRsp";
            }

        protected:
            std::string valueString() const override {
                return "size "+std::to_string(getPDUValueSize())+", data "+view.toString();
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.4.5
     * <p>
     * ATT_READ_BLOB_REQ
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.3 Read Long Characteristic Value
     * </p>
     */
    class AttReadBlobReq : public AttPDUMsg
    {
        public:
            AttReadBlobReq(const uint16_t handle, const uint16_t value_offset)
            : AttPDUMsg(ATT_READ_BLOB_REQ, 1+2+2)
            {
                pdu.put_uint16(1, handle);
                pdu.put_uint16(3, value_offset);
            }

            /** opcode + handle + value_offset */
            int getPDUValueOffset() const override { return 1 + 2 + 2; }

            uint16_t getHandle() const {
                return pdu.get_uint16( 1 );
            }

            uint16_t getValueOffset() const {
                return pdu.get_uint16( 1 + 2 );
            }

            std::string getName() const override {
                return "AttReadBlobReq";
            }

        protected:
            std::string valueString() const override {
                return "handle "+uint16HexString(getHandle(), true)+", valueOffset "+uint16HexString(getValueOffset(), true);
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.4.6
     * <p>
     * ATT_READ_BLOB_RSP
     * </p>
     * <p>
     * If expected value size exceeds getValueSize(), continue with ATT_READ_BLOB_REQ (3.4.4.5),
     * see shallReadBlobReq(..)
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.3 Read Long Characteristic Value
     * </p>
     */
    class AttReadBlobRsp: public AttPDUMsg
    {
        private:
            const TOctetSlice view;

        public:
            static bool instanceOf();

            AttReadBlobRsp(const uint8_t* source, const int length)
            : AttPDUMsg(source, length), view(pdu, getPDUValueOffset(), getPDUValueSize()) {
                checkOpcode(ATT_READ_BLOB_RSP);
            }

            /** opcode */
            int getPDUValueOffset() const override { return 1; }

            uint8_t const * getValuePtr() const { return pdu.get_ptr(getPDUValueOffset()); }

            TOctetSlice const & getValue() const { return view; }

            std::string getName() const override {
                return "AttReadBlobRsp";
            }

        protected:
            std::string valueString() const override {
                return "size "+std::to_string(getPDUValueSize())+", data "+view.toString();
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.5.1
     * <p>
     * ATT_WRITE_REQ
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
     * </p>
     */
    class AttWriteReq : public AttPDUMsg
    {
        private:
            const TOctetSlice view;

        public:
            AttWriteReq(const uint16_t handle, const TROOctets & value)
            : AttPDUMsg(ATT_WRITE_REQ, 1+2+value.getSize()), view(pdu, getPDUValueOffset(), getPDUValueSize())
            {
                pdu.put_uint16(1, handle);
                for(int i=0; i<value.getSize(); i++) {
                    pdu.put_uint8(3+i, value.get_uint8(i));
                }
            }

            /** opcode + handle */
            int getPDUValueOffset() const override { return 1 + 2; }

            uint16_t getHandle() const {
                return pdu.get_uint16( 1 );
            }

            uint8_t const * getValuePtr() const { return pdu.get_ptr(getPDUValueOffset()); }

            TOctetSlice const & getValue() const { return view; }

            std::string getName() const override {
                return "AttWriteReq";
            }

        protected:
            std::string valueString() const override {
                return "handle "+uint16HexString(getHandle(), true)+", data "+view.toString();;
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.5.2
     * <p>
     * ATT_WRITE_RSP
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value
     * </p>
     */
    class AttWriteRsp : public AttPDUMsg
    {
        public:
            AttWriteRsp(const uint8_t* source, const int length)
            : AttPDUMsg(source, length) {
                checkOpcode(ATT_WRITE_RSP);
            }

            /** opcode */
            int getPDUValueOffset() const override { return 1; }

            std::string getName() const override {
                return "AttWriteRsp";
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.5.3
     * <p>
     * ATT_WRITE_CMD
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.1 Write Characteristic Value without Response
     * </p>
     */
    class AttWriteCmd : public AttPDUMsg
    {
        private:
            const TOctetSlice view;

        public:
            AttWriteCmd(const uint16_t handle, const TROOctets & value)
            : AttPDUMsg(ATT_WRITE_CMD, 1+2+value.getSize()), view(pdu, getPDUValueOffset(), getPDUValueSize())
            {
                pdu.put_uint16(1, handle);
                for(int i=0; i<value.getSize(); i++) {
                    pdu.put_uint8(3+i, value.get_uint8(i));
                }
            }

            /** opcode + handle */
            int getPDUValueOffset() const override { return 1 + 2; }

            uint16_t getHandle() const {
                return pdu.get_uint16( 1 );
            }

            uint8_t const * getValuePtr() const { return pdu.get_ptr(getPDUValueOffset()); }

            TOctetSlice const & getValue() const { return view; }

            std::string getName() const override {
                return "AttWriteCmd";
            }

        protected:
            std::string valueString() const override {
                return "handle "+uint16HexString(getHandle(), true)+", data "+view.toString();;
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.7.1 and 3.4.7.2
     * <p>
     * A received ATT_HANDLE_VALUE_NTF or ATT_HANDLE_VALUE_IND from server.
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.10 Characteristic Value Notification
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.11 Characteristic Value Indications
     * </p>
     * <p>
     * Send by server to notify or indicate an ATT value (at any time).
     * </p>
     */
    class AttHandleValueRcv: public AttPDUMsg
    {
        private:
            const TOctetSlice view;

        public:
            AttHandleValueRcv(const uint8_t* source, const int length)
            : AttPDUMsg(source, length), view(pdu, getPDUValueOffset(), getPDUValueSize()) {
                checkOpcode(ATT_HANDLE_VALUE_NTF, ATT_HANDLE_VALUE_IND);
            }

            /** opcode + handle */
            int getPDUValueOffset() const override { return 1+2; }

            uint16_t getHandle() const {
                return pdu.get_uint16(1);
            }

            uint8_t const * getValuePtr() const { return pdu.get_ptr(getPDUValueOffset()); }

            TOctetSlice const & getValue() const { return view; }

            bool isNotification() const {
                return ATT_HANDLE_VALUE_NTF == getOpcode();
            }

            bool isIndication() const {
                return ATT_HANDLE_VALUE_IND == getOpcode();
            }

            std::string getName() const override {
                return "AttHandleValueRcv";
            }

        protected:
            std::string valueString() const override {
                return "handle "+uint16HexString(getHandle(), true)+", size "+std::to_string(getPDUValueSize())+", data "+view.toString();
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.7.3
     * <p>
     * ATT_HANDLE_VALUE_CFM to server, acknowledging ATT_HANDLE_VALUE_IND
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.11 Characteristic Value Indications
     * </p>
     */
    class AttHandleValueCfm: public AttPDUMsg
    {
        private:
            uint8_t _data[1];

        public:
            AttHandleValueCfm()
            : AttPDUMsg(ATT_HANDLE_VALUE_CFM, 1)
            {
            }

            /** opcode */
            int getPDUValueOffset() const override { return 1; }

            std::string getName() const override {
                return "AttHandleValueCfm";
            }
    };

    class AttElementList : public AttPDUMsg
    {
        protected:
            AttElementList(const uint8_t* source, const int length)
            : AttPDUMsg(source, length) {}

            virtual std::string addValueString() const { return ""; }
            virtual std::string elementString(const int idx) const { (void)idx; return "not implemented"; }

            std::string valueString() const override {
                std::string res = "size "+std::to_string(getPDUValueSize())+", "+addValueString()+"elements[count "+std::to_string(getElementCount())+", "+
                        "size [total "+std::to_string(getElementTotalSize())+", value "+std::to_string(getElementValueSize())+"]: ";
                const int count = getElementCount();
                for(int i=0; i<count; i++) {
                    res += std::to_string(i)+"["+elementString(i)+"],";
                }
                res += "]";
                return res;
            }

        public:
            virtual ~AttElementList() {}

            virtual int getElementTotalSize() const = 0;
            virtual int getElementValueSize() const = 0;
            virtual int getElementCount() const = 0;

            int getElementPDUOffset(const int elementIdx) const {
                return getPDUValueOffset() + elementIdx * getElementTotalSize();
            }

            uint8_t const * getElementPtr(const int elementIdx) const {
                return pdu.get_ptr(getElementPDUOffset(elementIdx));
            }

            std::string getName() const override {
                return "AttElementList";
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.4.1
     * <p>
     * ATT_READ_BY_TYPE_REQ
     * </p>
     *
     * <p>
     * and
     * </p>
     *
     * ATT Protocol PDUs Vol 3, Part F 3.4.4.9
     * <p>
     * ATT_READ_BY_GROUP_TYPE_REQ
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1 Characteristic Declaration Attribute Value
     * </p>
     */
    class AttReadByNTypeReq : public AttPDUMsg
    {
        private:
            uuid_t::TypeSize getUUIFormat() const {
                return uuid_t::toTypeSize(this->getPDUValueSize());
            }

        public:
            AttReadByNTypeReq(const bool groupTypeReq, const uint16_t startHandle, const uint16_t endHandle, const uuid_t & uuid)
            : AttPDUMsg(groupTypeReq ? ATT_READ_BY_GROUP_TYPE_REQ : ATT_READ_BY_TYPE_REQ, 1+2+2+uuid.getTypeSize())
            {
                if( uuid.getTypeSize() != uuid_t::TypeSize::UUID16_SZ && uuid.getTypeSize()!= uuid_t::TypeSize::UUID128_SZ ) {
                    throw IllegalArgumentException("Only UUID16 and UUID128 allowed: "+uuid.toString(), E_FILE_LINE);
                }
                pdu.put_uint16(1, startHandle);
                pdu.put_uint16(3, endHandle);
                pdu.put_uuid(5, uuid);
            }

            /** opcode + handle-start + handle-end */
            int getPDUValueOffset() const override { return 1 + 2 + 2; }

            uint16_t getStartHandle() const {
                return pdu.get_uint16( 1 );
            }

            uint16_t getEndHandle() const {
                return pdu.get_uint16( 1 + 2 /* 1 handle size */ );
            }

            std::string getName() const override {
                return "AttReadByNTypeReq";
            }

            std::shared_ptr<const uuid_t> getNType() const {
                return pdu.get_uuid( getPDUValueOffset(), getUUIFormat() );
            }

        protected:
            std::string valueString() const override {
                return "handle ["+uint16HexString(getStartHandle(), true)+".."+uint16HexString(getEndHandle(), true)+
                       "], uuid "+getNType()->toString();
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.4.2
     * <p>
     * ATT_READ_BY_TYPE_RSP
     * </p>
     * <p>
     * Contains a list of elements, each comprised of handle-value pairs.
     * The handle is comprised of two octets, i.e. uint16_t.
     * <pre>
     *  element := { uint16_t handle, uint8_t value[value-size] }
     * </pre>
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1 Characteristic Declaration Attribute Value
     * </p>
     */
    class AttReadByTypeRsp: public AttElementList
    {
        public:
            /**
             * element := { uint16_t handle, uint8_t value[value-size] }
             */
            class Element {
                private:
                    const TOctetSlice view;

                public:
                    Element(const AttReadByTypeRsp & p, const int idx)
                    : view(p.pdu, p.getElementPDUOffset(idx), p.getElementTotalSize()) {}

                    uint16_t getHandle() const {
                        return view.get_uint16(0);
                    }

                    uint8_t const * getValuePtr() const {
                        return view.get_ptr(2 /* handle size */);
                    }

                    int getValueSize() const { return view.getSize() - 2 /* handle size */; }

                    std::string toString() const {
                        return "handle "+uint16HexString(getHandle(), true)+
                               ", data "+bytesHexString(getValuePtr(), 0, getValueSize(), true /* lsbFirst */, true /* leading0X */);
                    }
            };

            AttReadByTypeRsp(const uint8_t* source, const int length) : AttElementList(source, length) {
                checkOpcode(ATT_READ_BY_TYPE_RSP);

                if( getPDUValueSize() % getElementTotalSize() != 0 ) {
                    throw AttValueException("PDUReadByTypeRsp: Invalid packet size: pdu-value-size "+std::to_string(getPDUValueSize())+
                            " not multiple of element-size "+std::to_string(getElementTotalSize()), E_FILE_LINE);
                }
            }

            /** opcode + element-size */
            int getPDUValueOffset() const override { return 1 + 1; }

            /** Returns size of each element, i.e. handle-value pair. */
            int getElementTotalSize() const override {
                return pdu.get_uint8(1);
            }

            /**
             * Net element-value size, i.e. element size less handle.
             * <p>
             * element := { uint16_t handle, uint8_t value[value-size] }
             * </p>
             */
            int getElementValueSize() const override {
                return getElementTotalSize() - 2;
            }

            int getElementCount() const override {
                return getPDUValueSize()  / getElementTotalSize();
            }

            Element getElement(const int elementIdx) const {
                return Element(*this, elementIdx);
            }

            uint16_t getElementHandle(const int elementIdx) const {
                return pdu.get_uint16( getElementPDUOffset(elementIdx) );
            }

            uint8_t * getElementValuePtr(const int elementIdx) {
                return pdu.get_wptr() + getElementPDUOffset(elementIdx) + 2 /* handle size */;
            }

            std::string getName() const override {
                return "AttReadByTypeRsp";
            }

        protected:
            std::string elementString(const int idx) const override {
                return getElement(idx).toString();
            }
    };


    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.4.10
     * <p>
     * ATT_READ_BY_GROUP_TYPE_RSP
     * </p>
     * <p>
     * Contains a list of elements, each comprised of { start_handle, end_handle, value } triple.
     * Both handle are each comprised of two octets, i.e. uint16_t.
     * <pre>
     *  element := { uint16_t startHandle, uint16_t endHandle, uint8_t value[value-size] }
     * </pre>
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services
     * </p>
     */
    class AttReadByGroupTypeRsp : public AttElementList
    {
        public:
            /**
             * element := { uint16_t startHandle, uint16_t endHandle, uint8_t value[value-size] }
             */
            class Element {
                private:
                    const TOctetSlice view;

                public:
                    Element(const AttReadByGroupTypeRsp & p, const int idx)
                    : view(p.pdu, p.getElementPDUOffset(idx), p.getElementTotalSize()) {}

                    uint16_t getStartHandle() const {
                        return view.get_uint16(0);
                    }

                    uint16_t getEndHandle() const {
                        return view.get_uint16(2);
                    }

                    uint8_t const * getValuePtr() const {
                        return view.get_ptr(4 /* handle size */);
                    }

                    int getValueSize() const { return view.getSize() - 4 /* handle size */; }
            };

            AttReadByGroupTypeRsp(const uint8_t* source, const int length) : AttElementList(source, length) {
                checkOpcode(ATT_READ_BY_GROUP_TYPE_RSP);

                if( getPDUValueSize() % getElementTotalSize() != 0 ) {
                    throw AttValueException("PDUReadByGroupTypeRsp: Invalid packet size: pdu-value-size "+std::to_string(getPDUValueSize())+
                            " not multiple of element-size "+std::to_string(getElementTotalSize()), E_FILE_LINE);
                }
            }

            /** opcode + element-size */
            int getPDUValueOffset() const override { return 1 + 1; }

            /** Returns size of each element, i.e. handle-value triple. */
            int getElementTotalSize() const override {
                return pdu.get_uint8(1);
            }

            /**
             * Net element-value size, i.e. element size less handles.
             * <p>
             * element := { uint16_t startHandle, uint16_t endHandle, uint8_t value[value-size] }
             * </p>
             */
            int getElementValueSize() const override {
                return getElementTotalSize() - 4;
            }

            int getElementCount() const override {
                return getPDUValueSize()  / getElementTotalSize();
            }

            Element getElement(const int elementIdx) const {
                return Element(*this, elementIdx);
            }

            uint16_t getElementStartHandle(const int elementIdx) const {
                return pdu.get_uint16( getElementPDUOffset(elementIdx) );
            }

            uint16_t getElementEndHandle(const int elementIdx) const {
                return pdu.get_uint16( getElementPDUOffset(elementIdx) + 2 /* 1 handle size */ );
            }

            uint8_t * getElementValuePtr(const int elementIdx) {
                return pdu.get_wptr() + getElementPDUOffset(elementIdx) + 4 /* 2 handle size */;
            }

            std::string getName() const override {
                return "AttReadByGroupTypeRsp";
            }

        protected:
            std::string elementString(const int idx) const override {
                Element e = getElement(idx);
                return "handle ["+uint16HexString(e.getStartHandle(), true)+".."+uint16HexString(e.getEndHandle(), true)+
                       "], data "+bytesHexString(e.getValuePtr(), 0, e.getValueSize(), true /* lsbFirst */, true /* leading0X */);
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.3.1
     * <p>
     * ATT_FIND_INFORMATION_REQ
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.7.1 Discover All Characteristic Descriptors
     * </p>
     */
    class AttFindInfoReq : public AttPDUMsg
    {
        public:
            AttFindInfoReq(const uint16_t startHandle, const uint16_t endHandle)
            : AttPDUMsg(ATT_FIND_INFORMATION_REQ, 1+2+2)
            {
                pdu.put_uint16(1, startHandle);
                pdu.put_uint16(3, endHandle);
            }

            /** opcode + handle_start + handle_end */
            int getPDUValueOffset() const override { return 1 + 2 + 2; }

            uint16_t getStartHandle() const {
                return pdu.get_uint16( 1 );
            }

            uint16_t getEndHandle() const {
                return pdu.get_uint16( 1 + 2 );
            }

            std::string getName() const override {
                return "AttFindInfoReq";
            }

        protected:
            std::string valueString() const override {
                return "handle ["+uint16HexString(getStartHandle(), true)+".."+uint16HexString(getEndHandle(), true)+"]";
            }
    };

    /**
     * ATT Protocol PDUs Vol 3, Part F 3.4.3.2
     * <p>
     * ATT_FIND_INFORMATION_RSP
     * </p>
     * <p>
     * Contains a list of elements, each comprised of { handle, [UUID16 | UUID128] } pair.
     * The handle is comprised of two octets, i.e. uint16_t.
     * The UUID is either comprised of 2 octets for UUID16 or 16 octets for UUID128
     * depending on the given format.
     * <pre>
     *  element := { uint16_t handle, UUID value }, with a UUID of UUID16 or UUID128
     * </pre>
     * </p>
     * Used in:
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.7.1 Discover All Characteristic Descriptors
     * </p>
     */
    class AttFindInfoRsp: public AttElementList
    {
        private:
            uuid_t::TypeSize getUUIFormat() const {
                const int f = pdu.get_uint8(1);
                if( 0x01 == f ) {
                    return uuid_t::TypeSize::UUID16_SZ;
                } else if( 0x02 == f ) {
                    return uuid_t::TypeSize::UUID128_SZ;
                }
                throw AttValueException("PDUFindInfoRsp: Invalid format "+std::to_string(f)+", not UUID16 (1) or UUID128 (2)", E_FILE_LINE);
            }

        public:
            /**
             * element := { uint16_t handle, UUID value }, with a UUID of UUID16 or UUID128
             */
            class Element {
                public:
                    const uint16_t handle;
                    const std::shared_ptr<const uuid_t> uuid;

                    Element(const AttFindInfoRsp & p, const int idx)
                    : handle( p.getElementHandle(idx) ), uuid( p.getElementValue(idx) )
                    { }
            };

            AttFindInfoRsp(const uint8_t* source, const int length) : AttElementList(source, length) {
                checkOpcode(ATT_FIND_INFORMATION_RSP);
                if( getPDUValueSize() % getElementTotalSize() != 0 ) {
                    throw AttValueException("PDUFindInfoRsp: Invalid packet size: pdu-value-size "+std::to_string(getPDUValueSize())+
                            " not multiple of element-size "+std::to_string(getElementTotalSize()), E_FILE_LINE);
                }
            }

            /** opcode + format */
            int getPDUValueOffset() const override { return 1 + 1; }

            /** Returns size of each element, i.e. handle-value triple. */
            int getElementTotalSize() const override {
                return 2 + getElementValueSize();
            }

            /**
             * Net element-value size, i.e. element size less handles.
             * <p>
             * element := { uint16_t handle, UUID value }, with a UUID of UUID16 or UUID128
             * </p>
             */
            int getElementValueSize() const override {
                return getUUIFormat();
            }

            int getElementCount() const override {
                return getPDUValueSize()  / getElementTotalSize();
            }

            Element getElement(const int elementIdx) const {
                return Element(*this, elementIdx);
            }

            uint16_t getElementHandle(const int elementIdx) const {
                return pdu.get_uint16( getElementPDUOffset(elementIdx) );
            }

            std::shared_ptr<const uuid_t> getElementValue(const int elementIdx) const {
                return pdu.get_uuid( getElementPDUOffset(elementIdx) + 2, getUUIFormat() );
            }

            std::string getName() const override {
                return "AttFindInfoRsp";
            }

        protected:
            std::string addValueString() const override { return "format "+std::to_string(pdu.get_uint8(1))+", "; }

            std::string elementString(const int idx) const override {
                Element e = getElement(idx);
                return "handle "+uint16HexString(e.handle, true)+
                       ", uuid "+e.uuid.get()->toString();
            }
    };
}


#endif /* ATT_PDU_TYPES_HPP_ */
