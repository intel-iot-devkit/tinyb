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

#ifndef HCI_TYPES_HPP_
#define HCI_TYPES_HPP_

#include <cstring>
#include <string>
#include <cstdint>

#include <mutex>

#include "BTTypes.hpp"
#include "BTIoctl.hpp"
#include "OctetTypes.hpp"
#include "HCIIoctl.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module HCITypes:
 *
 * - BT Core Spec v5.2: Vol 4, Part E Host Controller Interface (HCI): 7 HCI commands and events
 *
 */
namespace direct_bt {

    class HCIException : public RuntimeException {
        protected:
            HCIException(std::string const type, std::string const m, const char* file, int line) noexcept
            : RuntimeException(type, m, file, line) {}

        public:
            HCIException(std::string const m, const char* file, int line) noexcept
            : RuntimeException("HCIException", m, file, line) {}
    };

    class HCIPacketException : public HCIException {
        public:
            HCIPacketException(std::string const m, const char* file, int line) noexcept
            : HCIException("HCIPacketException", m, file, line) {}
    };
    class HCIOpcodeException : public HCIException {
        public:
            HCIOpcodeException(std::string const m, const char* file, int line) noexcept
            : HCIException("HCIOpcodeException", m, file, line) {}
    };

    enum class HCIConstInt : int {
        /** 3s poll timeout for complete HCI replies */
        TO_SEND_REQ_POLL_MS    = 3000,
        /** 10s le connection timeout, supervising max is 32s (v5.2 Vol 4, Part E - 7.8.12) */
        LE_CONN_TIMEOUT_MS     = 10000
    };
    inline int number(const HCIConstInt rhs) {
        return static_cast<int>(rhs);
    }

    enum class HCIConstU16 : uint16_t {
        INDEX_NONE             = 0xFFFF,
        /* Net length w/o null-termination */
        MAX_NAME_LENGTH        = 248,
        MAX_SHORT_NAME_LENGTH  =  10,
        MAX_AD_LENGTH          =  31
    };
    inline uint16_t number(const HCIConstU16 rhs) {
        return static_cast<uint16_t>(rhs);
    }

    /**
     * BT Core Spec v5.2: Vol 1, Part F Controller Error Codes: 1.3 List of Error Codes
     * <p>
     * BT Core Spec v5.2: Vol 1, Part F Controller Error Codes: 2 Error code descriptions
     * </p>
     */
    enum class HCIStatusCode : uint8_t {
        SUCCESS = 0x00,
        UNKNOWN_HCI_COMMAND = 0x01,
        UNKNOWN_CONNECTION_IDENTIFIER = 0x02,
        HARDWARE_FAILURE = 0x03,
        PAGE_TIMEOUT = 0x04,
        AUTHENTICATION_FAILURE = 0x05,
        PIN_OR_KEY_MISSING = 0x06,
        MEMORY_CAPACITY_EXCEEDED = 0x07,
        CONNECTION_TIMEOUT = 0x08,
        CONNECTION_LIMIT_EXCEEDED = 0x09,
        SYNC_DEVICE_CONNECTION_LIMIT_EXCEEDED = 0x0a,
        CONNECTION_ALREADY_EXISTS = 0x0b,
        COMMAND_DISALLOWED = 0x0c,
        CONNECTION_REJECTED_LIMITED_RESOURCES = 0x0d,
        CONNECTION_REJECTED_SECURITY = 0x0e,
        CONNECTION_REJECTED_UNACCEPTABLE_BD_ADDR = 0x0f,
        CONNECTION_ACCEPT_TIMEOUT_EXCEEDED = 0x10,
        UNSUPPORTED_FEATURE_OR_PARAM_VALUE = 0x11,
        INVALID_HCI_COMMAND_PARAMETERS = 0x12,
        REMOTE_USER_TERMINATED_CONNECTION = 0x13,
        REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES = 0x14,
        REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF = 0x15,
        CONNECTION_TERMINATED_BY_LOCAL_HOST = 0x16,
        REPEATED_ATTEMPTS = 0x17,
        PAIRING_NOT_ALLOWED = 0x18,
        UNKNOWN_LMP_PDU = 0x19,
        UNSUPPORTED_REMOTE_OR_LMP_FEATURE = 0x1a,
        SCO_OFFSET_REJECTED = 0x1b,
        SCO_INTERVAL_REJECTED = 0x1c,
        SCO_AIR_MODE_REJECTED = 0x1d,
        INVALID_LMP_OR_LL_PARAMETERS = 0x1e,
        UNSPECIFIED_ERROR = 0x1f,
        UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE = 0x20,
        ROLE_CHANGE_NOT_ALLOWED = 0x21,
        LMP_OR_LL_RESPONSE_TIMEOUT = 0x22,
        LMP_OR_LL_COLLISION = 0x23,
        LMP_PDU_NOT_ALLOWED = 0x24,
        ENCRYPTION_MODE_NOT_ACCEPTED = 0x25,
        LINK_KEY_CANNOT_BE_CHANGED = 0x26,
        REQUESTED_QOS_NOT_SUPPORTED = 0x27,
        INSTANT_PASSED = 0x28,
        PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED = 0x29,
        DIFFERENT_TRANSACTION_COLLISION = 0x2a,
        QOS_UNACCEPTABLE_PARAMETER = 0x2c,
        QOS_REJECTED = 0x2d,
        CHANNEL_ASSESSMENT_NOT_SUPPORTED = 0x2e,
        INSUFFICIENT_SECURITY = 0x2f,
        PARAMETER_OUT_OF_RANGE = 0x30,
        ROLE_SWITCH_PENDING = 0x32,
        RESERVED_SLOT_VIOLATION = 0x34,
        ROLE_SWITCH_FAILED = 0x35,
        EIR_TOO_LARGE = 0x36,
        SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST = 0x37,
        HOST_BUSY_PAIRING = 0x38,
        CONNECTION_REJECTED_NO_SUITABLE_CHANNEL = 0x39,
        CONTROLLER_BUSY = 0x3a,
        UNACCEPTABLE_CONNECTION_PARAM = 0x3b,
        ADVERTISING_TIMEOUT = 0x3c,
        CONNECTION_TERMINATED_MIC_FAILURE = 0x3d,
        CONNECTION_EST_FAILED_OR_SYNC_TIMETOUT = 0x3e,
        MAX_CONNECTION_FAILED  = 0x3f,
        COARSE_CLOCK_ADJ_REJECTED = 0x40,
        TYPE0_SUBMAP_NOT_DEFINED = 0x41,
        UNKNOWN_ADVERTISING_IDENTIFIER = 0x42,
        LIMIT_REACHED = 0x43,
        OPERATION_CANCELLED_BY_HOST = 0x44,
        PACKET_TOO_LONG = 0x45,

        INTERNAL_FAILURE = 0xfe,
        UNKNOWN = 0xff
    };
    inline uint8_t number(const HCIStatusCode rhs) {
        return static_cast<uint8_t>(rhs);
    }
    std::string getHCIStatusCodeString(const HCIStatusCode ec);

    enum class HCIConstU8 : uint8_t {
        /** HCIPacketType::COMMAND header size including HCIPacketType */
        COMMAND_HDR_SIZE  = 1+3,
        /** HCIPacketType::ACLDATA header size including HCIPacketType */
        ACL_HDR_SIZE      = 1+4,
        /** HCIPacketType::SCODATA header size including HCIPacketType */
        SCO_HDR_SIZE      = 1+3,
        /** HCIPacketType::EVENT header size including HCIPacketType */
        EVENT_HDR_SIZE    = 1+2,
        /** Total packet size, guaranteed to be handled by adapter. */
        PACKET_MAX_SIZE   = 255
    };
    inline uint8_t number(const HCIConstU8 rhs) {
        return static_cast<uint8_t>(rhs);
    }

    enum class HCIPacketType : uint8_t {
        COMMAND = 0x01,
        ACLDATA = 0x02,
        SCODATA = 0x03,
        EVENT   = 0x04,
        DIAG    = 0xf0,
        VENDOR  = 0xff
    };
    inline uint8_t number(const HCIPacketType rhs) {
        return static_cast<uint8_t>(rhs);
    }
    std::string getHCIPacketTypeString(const HCIPacketType op);

    enum class HCIOGF : uint8_t {
        /** link control commands */
        LINK_CTL    = 0x01,
        /** link policy commands */
        LINK_POLICY = 0x02,
        /** controller baseband commands */
        BREDR_CTL   = 0x03,
        /** LE controller commands */
        LE_CTL      = 0x08
    };
    inline uint8_t number(const HCIOGF rhs) {
        return static_cast<uint8_t>(rhs);
    }
    std::string getHCIOGFString(const HCIOGF op);

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.7 Events
     */
    enum class HCIEventType : uint8_t {
        INVALID                         = 0x00,
        INQUIRY_COMPLETE                = 0x01,
        INQUIRY_RESULT                  = 0x02,
        CONN_COMPLETE                   = 0x03,
        CONN_REQUEST                    = 0x04,
        DISCONN_COMPLETE                = 0x05,
        AUTH_COMPLETE                   = 0x06,
        REMOTE_NAME                     = 0x07,
        ENCRYPT_CHANGE                  = 0x08,
        CHANGE_LINK_KEY_COMPLETE        = 0x09,
        REMOTE_FEATURES                 = 0x0b,
        REMOTE_VERSION                  = 0x0c,
        QOS_SETUP_COMPLETE              = 0x0d,
        CMD_COMPLETE                    = 0x0e,
        CMD_STATUS                      = 0x0f,
        HARDWARE_ERROR                  = 0x10,
        ROLE_CHANGE                     = 0x12,
        NUM_COMP_PKTS                   = 0x13,
        MODE_CHANGE                     = 0x14,
        PIN_CODE_REQ                    = 0x16,
        LINK_KEY_REQ                    = 0x17,
        LINK_KEY_NOTIFY                 = 0x18,
        CLOCK_OFFSET                    = 0x1c,
        PKT_TYPE_CHANGE                 = 0x1d,
        DISCONN_PHY_LINK_COMPLETE       = 0x42,
        DISCONN_LOGICAL_LINK_COMPLETE   = 0x46,
        LE_META                         = 0x3e,
        AMP_Receiver_Report             = 0x4b

        // etc etc - incomplete
    };
    inline uint8_t number(const HCIEventType rhs) {
        return static_cast<uint8_t>(rhs);
    }
    std::string getHCIEventTypeString(const HCIEventType op);

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.7.65 LE Meta event
     */
    enum class HCIMetaEventType : uint8_t {
        INVALID                             = 0x00,
        LE_CONN_COMPLETE                    = 0x01,/**< LE_CONN_COMPLETE */
        LE_ADVERTISING_REPORT               = 0x02,/**< LE_ADVERTISING_REPORT */
        LE_CONN_UPDATE_COMPLETE             = 0x03,/**< LE_CONN_UPDATE_COMPLETE */
        LE_REMOTE_FEAT_COMPLETE             = 0x04,/**< LE_REMOTE_FEAT_COMPLETE */
        LE_LTKEY_REQUEST                    = 0x05,/**< LE_LTKEY_REQUEST */
        LE_REMOTE_CONN_PARAM_REQ            = 0x06,/**< LE_REMOTE_CONN_PARAM_REQ */
        LE_DATA_LENGTH_CHANGE               = 0x07,/**< LE_DATA_LENGTH_CHANGE */
        LE_READ_LOCAL_P256_PUBKEY_COMPLETE  = 0x08,/**< LE_READ_LOCAL_P256_PUBKEY_COMPLETE */
        LE_GENERATE_DHKEY_COMPLETE          = 0x09,/**< LE_GENERATE_DHKEY_COMPLETE */
        LE_ENHANCED_CONN_COMPLETE           = 0x0A,/**< LE_ENHANCED_CONN_COMPLETE */
        LE_DIRECT_ADV_REPORT                = 0x0B,/**< LE_DIRECT_ADV_REPORT */
        LE_PHY_UPDATE_COMPLETE              = 0x0C,/**< LE_PHY_UPDATE_COMPLETE */
        LE_EXT_ADV_REPORT                   = 0x0D,/**< LE_EXT_ADV_REPORT */
        LE_PERIODIC_ADV_SYNC_ESTABLISHED    = 0x0E,/**< LE_PERIODIC_ADV_SYNC_ESTABLISHED */
        LE_PERIODIC_ADV_REPORT              = 0x0F,/**< LE_PERIODIC_ADV_REPORT */
        LE_PERIODIC_ADV_SYNC_LOST           = 0x10,/**< LE_PERIODIC_ADV_SYNC_LOST */
        LE_SCAN_TIMEOUT                     = 0x11,/**< LE_SCAN_TIMEOUT */
        LE_ADV_SET_TERMINATED               = 0x12,/**< LE_ADV_SET_TERMINATED */
        LE_SCAN_REQ_RECEIVED                = 0x13,/**< LE_SCAN_REQ_RECEIVED */
        LE_CHANNEL_SEL_ALGO                 = 0x14,/**< LE_CHANNEL_SEL_ALGO */
        LE_CONNLESS_IQ_REPORT               = 0x15,/**< LE_CONNLESS_IQ_REPORT */
        LE_CONN_IQ_REPORT                   = 0x16,/**< LE_CONN_IQ_REPORT */
        LE_CTE_REQ_FAILED                   = 0x17,/**< LE_CTE_REQ_FAILED */
        LE_PERIODIC_ADV_SYNC_TRANSFER_RECV  = 0x18,/**< LE_PERIODIC_ADV_SYNC_TRANSFER_RECV */
        LE_CIS_ESTABLISHED                  = 0x19,/**< LE_CIS_ESTABLISHED */
        LE_CIS_REQUEST                      = 0x1A,/**< LE_CIS_REQUEST */
        LE_CREATE_BIG_COMPLETE              = 0x1B,/**< LE_CREATE_BIG_COMPLETE */
        LE_TERMINATE_BIG_COMPLETE           = 0x1C,/**< LE_TERMINATE_BIG_COMPLETE */
        LE_BIG_SYNC_ESTABLISHED             = 0x1D,/**< LE_BIG_SYNC_ESTABLISHED */
        LE_BIG_SYNC_LOST                    = 0x1E,/**< LE_BIG_SYNC_LOST */
        LE_REQUEST_PEER_SCA_COMPLETE        = 0x1F,/**< LE_REQUEST_PEER_SCA_COMPLETE */
        LE_PATH_LOSS_THRESHOLD              = 0x20,/**< LE_PATH_LOSS_THRESHOLD */
        LE_TRANSMIT_POWER_REPORTING         = 0x21,/**< LE_TRANSMIT_POWER_REPORTING */
        LE_BIGINFO_ADV_REPORT               = 0x22 /**< LE_BIGINFO_ADV_REPORT */
    };
    inline uint8_t number(const HCIMetaEventType rhs) {
        return static_cast<uint8_t>(rhs);
    }
    std::string getHCIMetaEventTypeString(const HCIMetaEventType op);

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.1 Link Controller commands
     * <p>
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.3 Controller & Baseband commands
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.4 Informational paramters
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.8 LE Controller commands
     * </p>
     */
    enum class HCIOpcode : uint16_t {
        SPECIAL                     = 0x0000,/**< SPECIAL */
        CREATE_CONN                 = 0x0405,
        DISCONNECT                  = 0x0406,
        SET_EVENT_MASK              = 0x0C01,/**< SET_EVENT_MASK */
        RESET                       = 0x0C03,
        READ_LOCAL_VERSION          = 0x1001,
        LE_SET_EVENT_MASK           = 0x2001,/**< LE_SET_EVENT_MASK */
        LE_READ_BUFFER_SIZE         = 0x2002,
        LE_READ_LOCAL_FEATURES      = 0x2003,
        LE_SET_RANDOM_ADDR          = 0x2005,
        LE_SET_ADV_PARAM            = 0x2006,
        LE_READ_ADV_TX_POWER        = 0x2007,
        LE_SET_ADV_DATA             = 0x2008,
        LE_SET_SCAN_RSP_DATA        = 0x2009,
        LE_SET_ADV_ENABLE           = 0x200a,
        LE_SET_SCAN_PARAM           = 0x200b,
        LE_SET_SCAN_ENABLE          = 0x200c,
        LE_CREATE_CONN              = 0x200d, /**< LE_CREATE_CONN */
        LE_CREATE_CONN_CANCEL       = 0x200e,
        LE_READ_WHITE_LIST_SIZE     = 0x200f,
        LE_CLEAR_WHITE_LIST         = 0x2010,
        LE_ADD_TO_WHITE_LIST        = 0x2011,
        LE_DEL_FROM_WHITE_LIST      = 0x2012,
        LE_CONN_UPDATE              = 0x2013,
        LE_READ_REMOTE_FEATURES     = 0x2016,
        LE_START_ENC                = 0x2019
        // etc etc - incomplete
    };
    inline uint16_t number(const HCIOpcode rhs) {
        return static_cast<uint16_t>(rhs);
    }
    std::string getHCIOpcodeString(const HCIOpcode op);

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 5.4 Exchange of HCI-specific information
     * <p>
     * BT Core Spec v5.2: Vol 4, Part E HCI: 5.4.1 HCI Command packet
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 4, Part E HCI: 5.4.4 HCI Event packet
     * </p>
     * <pre>
     * </pre>
     */
    class HCIPacket
    {
        protected:
            POctets pdu;

            inline static void checkPacketType(const HCIPacketType type) {
                switch(type) {
                    case HCIPacketType::COMMAND:
                    case HCIPacketType::ACLDATA:
                    case HCIPacketType::SCODATA:
                    case HCIPacketType::EVENT:
                    case HCIPacketType::DIAG:
                    case HCIPacketType::VENDOR:
                        return; // OK
                    default:
                        throw HCIPacketException("Unsupported packet type "+uint8HexString(number(type)), E_FILE_LINE);
                }
            }

        public:
            HCIPacket(const HCIPacketType type, const uint8_t total_packet_size)
            : pdu(total_packet_size)
            {
                pdu.put_uint8 (0, number(type));
            }
            HCIPacket(const uint8_t *packet_data, const uint8_t total_packet_size)
            : pdu(total_packet_size)
            {
                if( total_packet_size > 0 ) {
                    memcpy(pdu.get_wptr(), packet_data, total_packet_size);
                }
                checkPacketType(getPacketType());
            }
            virtual ~HCIPacket() {}

            int getTotalSize() const { return pdu.getSize(); }

            /** Return the underlying octets read only */
            TROOctets & getPDU() { return pdu; }

            HCIPacketType getPacketType() { return static_cast<HCIPacketType>(pdu.get_uint8(0)); }

    };

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 5.4.1 HCI Command packet
     * <p>
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.8 LE Controller Commands
     * </p>
     * <pre>
        __le16  opcode; // OCF & OGF
        __u8    plen;
     * </pre>
     */
    class HCICommand : public HCIPacket
    {
        protected:
            inline static void checkOpcode(const HCIOpcode has, const HCIOpcode min, const HCIOpcode max)
            {
                if( has < min || has > max ) {
                    throw HCIOpcodeException("Has opcode "+uint16HexString(number(has))+
                                     ", not within range ["+uint16HexString(number(min))+
                                     ".."+uint16HexString(number(max))+"]", E_FILE_LINE);
                }
            }

            virtual std::string baseString() const {
                return "opcode="+uint16HexString(number(getOpcode()))+" "+getOpcodeString();
            }
            virtual std::string valueString() const {
                const int psz = getParamSize();
                const std::string ps = psz > 0 ? bytesHexString(getParam(), 0, psz, true /* lsbFirst */, true /* leading0X */) : "";
                return "param[size "+std::to_string(getParamSize())+", data "+ps+"], tsz "+std::to_string(getTotalSize());
            }

        public:

            /** Enabling manual construction of command without given value. */
            HCICommand(const HCIOpcode opc, const uint8_t param_size)
            : HCIPacket(HCIPacketType::COMMAND, number(HCIConstU8::COMMAND_HDR_SIZE)+param_size)
            {
                checkOpcode(opc, HCIOpcode::SPECIAL, HCIOpcode::LE_START_ENC);

                pdu.put_uint16(1, static_cast<uint16_t>(opc));
                pdu.put_uint8(3, param_size);
            }

            /** Enabling manual construction of command with given value.  */
            HCICommand(const HCIOpcode opc, const uint8_t* param, const uint16_t param_size)
            : HCICommand(opc, param_size)
            {
                if( param_size > 0 ) {
                    memcpy(pdu.get_wptr(number(HCIConstU8::COMMAND_HDR_SIZE)), param, param_size);
                }
            }

            virtual ~HCICommand() {}

            HCIOpcode getOpcode() const { return static_cast<HCIOpcode>( pdu.get_uint16(1) ); }
            std::string getOpcodeString() const { return getHCIOpcodeString(getOpcode()); }
            uint8_t getParamSize() const { return pdu.get_uint8(3); }
            const uint8_t* getParam() const { return pdu.get_ptr(number(HCIConstU8::COMMAND_HDR_SIZE)); }

            std::string toString() const {
                return "HCICommand["+baseString()+", "+valueString()+"]";
            }
    };

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.1.6 Disconnect command
     * <pre>
        Size 3
        __le16   handle;
        __u8     reason;
     * </pre>
     */
    class HCIDisconnectCmd : public HCICommand
    {
        public:
            HCIDisconnectCmd(const uint16_t handle, HCIStatusCode reason)
            : HCICommand(HCIOpcode::DISCONNECT, 3)
            {
                pdu.put_uint16(number(HCIConstU8::COMMAND_HDR_SIZE),handle);
                pdu.put_uint8(number(HCIConstU8::COMMAND_HDR_SIZE)+2, number(reason));
            }
    };

    /**
     * Generic HCICommand wrapper for any HCI IOCTL structure
     * @tparam hcistruct the template typename, e.g. 'hci_cp_create_conn' for 'struct hci_cp_create_conn'
     */
    template<typename hcistruct>
    class HCIStructCommand : public HCICommand
    {
        public:
            /** Enabling manual construction of command without given value. */
            HCIStructCommand(const HCIOpcode opc)
            : HCICommand(opc, sizeof(hcistruct))
            { }

            /** Enabling manual construction of command with given value.  */
            HCIStructCommand(const HCIOpcode opc, const hcistruct &cp)
            : HCICommand(opc, (const uint8_t *)(&cp), sizeof(hcistruct))
            { }

            const hcistruct * getStruct() const { return (const hcistruct *)(getParam()); }
            hcistruct * getWStruct() { return (hcistruct *)( pdu.get_wptr( number(HCIConstU8::COMMAND_HDR_SIZE) ) ); }
    };

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 5.4.4 HCI Event packet
     * <p>
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.7 Events
     * </p>
     * <pre>
        __u8    evt;
        __u8    plen;
     * </pre>
     */
    class HCIEvent : public HCIPacket
    {
        protected:
            uint64_t ts_creation;

            inline static void checkEventType(const HCIEventType has, const HCIEventType min, const HCIEventType max)
            {
                if( has < min || has > max ) {
                    throw HCIOpcodeException("Has evcode "+uint8HexString(number(has))+
                                     ", not within range ["+uint8HexString(number(min))+
                                     ".."+uint8HexString(number(max))+"]", E_FILE_LINE);
                }
            }
            inline static void checkEventType(const HCIEventType has, const HCIEventType exp)
            {
                if( has != exp ) {
                    throw HCIOpcodeException("Has evcode "+uint8HexString(number(has))+
                                     ", not matching "+uint8HexString(number(exp)), E_FILE_LINE);
                }
            }

            virtual std::string baseString() const {
                return "event="+uint8HexString(number(getEventType()))+" "+getEventTypeString();
            }
            virtual std::string valueString() const {
                const int d_sz = getParamSize();
                const std::string d_str = d_sz > 0 ? bytesHexString(getParam(), 0, d_sz, true /* lsbFirst */, true /* leading0X */) : "";
                return "data[size "+std::to_string(d_sz)+", data "+d_str+"], tsz "+std::to_string(getTotalSize());
            }

        public:

            /**
             * Return a newly created specialized instance pointer to base class.
             * <p>
             * Returned memory reference is managed by caller (delete etc)
             * </p>
             */
            static HCIEvent* getSpecialized(const uint8_t * buffer, int const buffer_size);

            /** Persistent memory, w/ ownership ..*/
            HCIEvent(const uint8_t* buffer, const int buffer_len)
            : HCIPacket(buffer, buffer_len), ts_creation(getCurrentMilliseconds())
            {
                checkEventType(getEventType(), HCIEventType::INQUIRY_COMPLETE, HCIEventType::AMP_Receiver_Report);
                pdu.check_range(0, number(HCIConstU8::EVENT_HDR_SIZE)+getParamSize());
            }

            /** Enabling manual construction of event without given value.  */
            HCIEvent(const HCIEventType evt, const uint16_t param_size=0)
            : HCIPacket(HCIPacketType::EVENT, number(HCIConstU8::EVENT_HDR_SIZE)+param_size), ts_creation(getCurrentMilliseconds())
            {
                checkEventType(evt, HCIEventType::INQUIRY_COMPLETE, HCIEventType::AMP_Receiver_Report);
                pdu.put_uint8(1, number(evt));
                pdu.put_uint8(2, param_size);
            }

            /** Enabling manual construction of event with given value.  */
            HCIEvent(const HCIEventType evt, const uint8_t* param, const uint16_t param_size)
            : HCIEvent(evt, param_size)
            {
                if( param_size > 0 ) {
                    memcpy(pdu.get_wptr(number(HCIConstU8::EVENT_HDR_SIZE)), param, param_size);
                }
            }

            virtual ~HCIEvent() {}

            uint64_t getTimestamp() const { return ts_creation; }

            HCIEventType getEventType() const { return static_cast<HCIEventType>( pdu.get_uint8(1) ); }
            std::string getEventTypeString() const { return getHCIEventTypeString(getEventType()); }
            bool isEvent(HCIEventType t) const { return t == getEventType(); }

            /**
             * The meta subevent type
             */
            virtual HCIMetaEventType getMetaEventType() const { return HCIMetaEventType::INVALID; }
            std::string getMetaEventTypeString() const { return getHCIMetaEventTypeString(getMetaEventType()); }
            bool isMetaEvent(HCIMetaEventType t) const { return t == getMetaEventType(); }

            uint8_t getParamSize() const { return pdu.get_uint8(2); }
            const uint8_t* getParam() const { return pdu.get_ptr(number(HCIConstU8::EVENT_HDR_SIZE)); }

            virtual bool validate(const HCICommand & cmd) const { (void)cmd; return true; }

            std::string toString() const {
                return "HCIEvent["+baseString()+", "+valueString()+"]";
            }
    };

    /**
     * Generic HCIEvent wrapper for any HCI IOCTL 'command complete' alike event struct having a HCIStatusCode uint8_t status field.
     * @tparam hcistruct the template typename, e.g. 'hci_ev_conn_complete' for 'struct hci_ev_conn_complete'
     */
    template<typename hcistruct>
    class HCIStructCmdCompleteEvt : public HCIEvent
    {
        public:
            /** Passing through preset buffer of this type */
            HCIStructCmdCompleteEvt(const uint8_t* buffer, const int buffer_len)
            : HCIEvent(buffer, buffer_len)
            {
                pdu.check_range(0, number(HCIConstU8::EVENT_HDR_SIZE)+sizeof(hcistruct));
            }

            /** Enabling manual construction of event without given value. */
            HCIStructCmdCompleteEvt(const HCIEventType ec)
            : HCIEvent(ec, sizeof(hcistruct))
            { }

            /** Enabling manual construction of event with given value.  */
            HCIStructCmdCompleteEvt(const HCIEventType ec, const hcistruct &data)
            : HCIEvent(ec, (const uint8_t *)(&data), sizeof(hcistruct))
            { }

            bool isTypeAndSizeValid(const HCIEventType ec) const {
                return isEvent(ec) &&
                       pdu.is_range_valid(0, number(HCIConstU8::EVENT_HDR_SIZE)+sizeof(hcistruct));
            }
            const hcistruct * getStruct() const { return (const hcistruct *)(getParam()); }
            HCIStatusCode getStatus() const { return static_cast<HCIStatusCode>( getStruct()->status ); }

            hcistruct * getWStruct() { return (hcistruct *)( pdu.get_wptr(number(HCIConstU8::EVENT_HDR_SIZE)) ); }
    };


    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.7.5 Disconnection Complete event
     * <p>
     * Size 4
        __u8     status;
        __le16   handle;
        __u8     reason;
     * </p>
     */
    class HCIDisconnectionCompleteEvent : public HCIEvent
    {
        protected:
            std::string baseString() const override {
                return HCIEvent::baseString()+
                        ", status "+uint8HexString(static_cast<uint8_t>(getStatus()), true)+" "+getHCIStatusCodeString(getStatus())+
                        ", handle "+std::to_string(getHandle())+
                        ", reason "+uint8HexString(static_cast<uint8_t>(getReason()), true)+" "+getHCIStatusCodeString(getReason());
            }

        public:
            HCIDisconnectionCompleteEvent(const uint8_t* buffer, const int buffer_len)
            : HCIEvent(buffer, buffer_len)
            {
                checkEventType(getEventType(), HCIEventType::DISCONN_COMPLETE);
                pdu.check_range(0, number(HCIConstU8::EVENT_HDR_SIZE)+4);
            }

            HCIStatusCode getStatus() const { return static_cast<HCIStatusCode>( pdu.get_uint8(number(HCIConstU8::EVENT_HDR_SIZE)) ); }
            uint16_t getHandle() const { return pdu.get_uint16(number(HCIConstU8::EVENT_HDR_SIZE)+1); }
            HCIStatusCode getReason() const { return static_cast<HCIStatusCode>( pdu.get_uint8(number(HCIConstU8::EVENT_HDR_SIZE)+3) ); }

            bool validate(const HCICommand & cmd) const override {
                return cmd.getOpcode() == HCIOpcode::DISCONNECT;
            }
    };

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.7.14 Command Complete event
     * <p>
     * Size 3 + return size
        __u8     ncmd;
        __le16   opcode;
        Return_Paramters of variable length, usually with '__u8 status' first.
     * </p>
     */
    class HCICommandCompleteEvent : public HCIEvent
    {
        protected:
            std::string baseString() const override {
                return HCIEvent::baseString()+", opcode="+uint16HexString(static_cast<uint16_t>(getOpcode()))+
                        " "+getHCIOpcodeString(getOpcode())+
                        ", ncmd "+std::to_string(getNumCommandPackets());
            }

        public:
            HCICommandCompleteEvent(const uint8_t* buffer, const int buffer_len)
            : HCIEvent(buffer, buffer_len)
            {
                checkEventType(getEventType(), HCIEventType::CMD_COMPLETE);
                pdu.check_range(0, number(HCIConstU8::EVENT_HDR_SIZE)+3);
            }

            /**
             * The Number of HCI Command packets which are allowed to be sent to the Controller from the Host.
             * <p>
             * Range: 0 to 255
             * </p>
             */
            uint8_t getNumCommandPackets() const { return pdu.get_uint8(number(HCIConstU8::EVENT_HDR_SIZE)+0); }

            /**
             * The associated command
             */
            HCIOpcode getOpcode() const { return static_cast<HCIOpcode>( pdu.get_uint16(number(HCIConstU8::EVENT_HDR_SIZE)+1) ); }

            uint8_t getReturnParamSize() const { return getParamSize() - 3; }
            const uint8_t* getReturnParam() const { return pdu.get_ptr(number(HCIConstU8::EVENT_HDR_SIZE)+3); }
            HCIStatusCode getReturnStatus(const int returnParamOffset=0) const {
                const uint8_t returnParamSize = getReturnParamSize();
                if( returnParamSize < returnParamOffset + 1 /* status size */ ) {
                    return HCIStatusCode::UNKNOWN;
                }
                return static_cast<HCIStatusCode>( pdu.get_uint8(number(HCIConstU8::EVENT_HDR_SIZE) + 3 + returnParamOffset) );
            }

            bool validate(const HCICommand & cmd) const override {
                return cmd.getOpcode() == getOpcode();
            }
    };

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.7.15 Command Status event
     * <p>
     * Size 4
        __u8     status;
        __u8     ncmd;
        __le16   opcode;
     * </p>
     */
    class HCICommandStatusEvent : public HCIEvent
    {
        protected:
            std::string baseString() const override {
                return HCIEvent::baseString()+", opcode="+uint16HexString(static_cast<uint16_t>(getOpcode()))+
                        " "+getHCIOpcodeString(getOpcode())+
                        ", ncmd "+std::to_string(getNumCommandPackets())+
                        ", status "+uint8HexString(static_cast<uint8_t>(getStatus()), true)+" "+getHCIStatusCodeString(getStatus());
            }

        public:
            HCICommandStatusEvent(const uint8_t* buffer, const int buffer_len)
            : HCIEvent(buffer, buffer_len)
            {
                checkEventType(getEventType(), HCIEventType::CMD_STATUS);
                pdu.check_range(0, number(HCIConstU8::EVENT_HDR_SIZE)+4);
            }

            HCIStatusCode getStatus() const { return static_cast<HCIStatusCode>( pdu.get_uint8(number(HCIConstU8::EVENT_HDR_SIZE)) ); }

            /**
             * The Number of HCI Command packets which are allowed to be sent to the Controller from the Host.
             * <p>
             * Range: 0 to 255
             * </p>
             */
            uint8_t getNumCommandPackets() const { return pdu.get_uint8(number(HCIConstU8::EVENT_HDR_SIZE)+1); }

            /**
             * The associated command
             */
            HCIOpcode getOpcode() const { return static_cast<HCIOpcode>( pdu.get_uint16(number(HCIConstU8::EVENT_HDR_SIZE)+1+1) ); }

            bool validate(const HCICommand & cmd) const override {
                return cmd.getOpcode() == getOpcode();
            }
    };

    /**
     * BT Core Spec v5.2: Vol 4, Part E HCI: 7.7.65 LE Meta event
     * <p>
     * Size 1
        __u8     subevent;
     * </p>
     */
    class HCIMetaEvent : public HCIEvent
    {
        protected:
            static void checkMetaType(const HCIMetaEventType has, const HCIMetaEventType exp)
            {
                if( has != exp ) {
                    throw HCIOpcodeException("Has meta "+uint8HexString(number(has))+
                                     ", not matching "+uint8HexString(number(exp)), E_FILE_LINE);
                }
            }

            virtual std::string baseString() const override {
                return "event="+uint8HexString(number(getMetaEventType()))+" "+getMetaEventTypeString()+" (le-meta)";
            }

        public:
            /** Passing through preset buffer of this type */
            HCIMetaEvent(const uint8_t* buffer, const int buffer_len)
            : HCIEvent(buffer, buffer_len)
            {
                checkEventType(getEventType(), HCIEventType::LE_META);
                pdu.check_range(0, number(HCIConstU8::EVENT_HDR_SIZE)+1);
            }

            /** Enabling manual construction of event without given value. */
            HCIMetaEvent(const HCIMetaEventType mc, const int meta_param_size)
            : HCIEvent(HCIEventType::LE_META, 1+meta_param_size)
            {
                pdu.put_uint8(number(HCIConstU8::EVENT_HDR_SIZE), number(mc));
            }

            /** Enabling manual construction of event with given value.  */
            HCIMetaEvent(const HCIMetaEventType mc, const uint8_t * meta_param, const int meta_param_size)
            : HCIMetaEvent(mc, meta_param_size)
            {
                if( meta_param_size > 0 ) {
                    memcpy(pdu.get_wptr(number(HCIConstU8::EVENT_HDR_SIZE)+1), meta_param, meta_param_size);
                }
            }

            HCIMetaEventType getMetaEventType() const override { return static_cast<HCIMetaEventType>( pdu.get_uint8(number(HCIConstU8::EVENT_HDR_SIZE)) ); }
    };

    /**
     * Generic HCIMetaEvent wrapper for any HCI IOCTL 'command complete' alike meta event struct having a HCIStatusCode uint8_t status field.
     * @tparam hcistruct the template typename, e.g. 'hci_ev_le_conn_complete' for 'struct hci_ev_le_conn_complete'
     */
    template<typename hcistruct>
    class HCIStructCmdCompleteMetaEvt : public HCIMetaEvent
    {
        public:
            /** Passing through preset buffer of this type */
            HCIStructCmdCompleteMetaEvt(const uint8_t* buffer, const int buffer_len)
            : HCIMetaEvent(buffer, buffer_len)
            {
                pdu.check_range(0, number(HCIConstU8::EVENT_HDR_SIZE)+1+sizeof(hcistruct));
            }

            /** Enabling manual construction of event without given value. */
            HCIStructCmdCompleteMetaEvt(const HCIMetaEventType mc)
            : HCIMetaEvent(mc, sizeof(hcistruct))
            { }

            /** Enabling manual construction of event with given value.  */
            HCIStructCmdCompleteMetaEvt(const HCIMetaEventType mc, const hcistruct &data)
            : HCIMetaEvent(mc, (const uint8_t *)(&data), sizeof(hcistruct))
            { }

            bool isTypeAndSizeValid(const HCIMetaEventType mc) const {
                return isMetaEvent(mc) &&
                       pdu.is_range_valid(0, number(HCIConstU8::EVENT_HDR_SIZE)+1+sizeof(hcistruct));
            }
            const hcistruct * getStruct() const { return (const hcistruct *)( pdu.get_ptr(number(HCIConstU8::EVENT_HDR_SIZE)+1) ); }
            HCIStatusCode getStatus() const { return static_cast<HCIStatusCode>( getStruct()->status ); }

            hcistruct * getWStruct() { return (hcistruct *)( pdu.get_wptr(number(HCIConstU8::EVENT_HDR_SIZE)+1) ); }
    };

} // namespace direct_bt

#endif /* HCI_TYPES_HPP_ */
