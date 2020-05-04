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

#ifndef MGMT_TYPES_HPP_
#define MGMT_TYPES_HPP_

#include <cstring>
#include <string>
#include <cstdint>

#include <mutex>

#include "BTTypes.hpp"
#include "BTIoctl.hpp"
#include "OctetTypes.hpp"
#include "HCIComm.hpp"

namespace direct_bt {

    class MgmtException : public RuntimeException {
        protected:
            MgmtException(std::string const type, std::string const m, const char* file, int line) noexcept
            : RuntimeException(type, m, file, line) {}

        public:
            MgmtException(std::string const m, const char* file, int line) noexcept
            : RuntimeException("MgmtException", m, file, line) {}
    };

    class MgmtOpcodeException : public MgmtException {
        public:
            MgmtOpcodeException(std::string const m, const char* file, int line) noexcept
            : MgmtException("MgmtOpcodeException", m, file, line) {}
    };

    enum BTMode : uint8_t {
        BT_MODE_DUAL        = 1,
        BT_MODE_BREDR       = 2,
        BT_MODE_LE          = 3
    };

    enum MgmtConstU16 : uint16_t {
        INDEX_NONE          = 0xFFFF,
        /* Net length, guaranteed to be null-terminated */
        MAX_NAME_LENGTH        = 248+1,
        MAX_SHORT_NAME_LENGTH  =  10+1
    };

    enum class AdapterSetting : uint32_t {
        NONE               =          0,
        POWERED            = 0x00000001,
        CONNECTABLE        = 0x00000002,
        FAST_CONNECTABLE   = 0x00000004,
        DISCOVERABLE       = 0x00000008,
        BONDABLE           = 0x00000010,
        LINK_SECURITY      = 0x00000020,
        SSP                = 0x00000040,
        BREDR              = 0x00000080,
        HS                 = 0x00000100,
        LE                 = 0x00000200,
        ADVERTISING        = 0x00000400,
        SECURE_CONN        = 0x00000800,
        DEBUG_KEYS         = 0x00001000,
        PRIVACY            = 0x00002000,
        CONFIGURATION      = 0x00004000,
        STATIC_ADDRESS     = 0x00008000,
        PHY_CONFIGURATION  = 0x00010000
    };
    inline AdapterSetting operator ^(const AdapterSetting lhs, const AdapterSetting rhs) {
        return static_cast<AdapterSetting> ( static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs) );
    }
    inline AdapterSetting operator |(const AdapterSetting lhs, const AdapterSetting rhs) {
        return static_cast<AdapterSetting> ( static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs) );
    }
    inline AdapterSetting operator &(const AdapterSetting lhs, const AdapterSetting rhs) {
        return static_cast<AdapterSetting> ( static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs) );
    }
    inline bool operator ==(const AdapterSetting lhs, const AdapterSetting rhs) {
        return static_cast<uint32_t>(lhs) == static_cast<uint32_t>(rhs);
    }
    inline bool operator !=(const AdapterSetting lhs, const AdapterSetting rhs) {
        return !( lhs == rhs );
    }
    inline bool isAdapterSettingSet(const AdapterSetting mask, const AdapterSetting bit) { return AdapterSetting::NONE != ( mask & bit ); }
    inline void setAdapterSettingSet(AdapterSetting &mask, const AdapterSetting bit) { mask = mask | bit; }
    std::string adapterSettingBitToString(const AdapterSetting settingBit);
    std::string adapterSettingsToString(const AdapterSetting settingBitMask);

    enum MgmtConst : int {
        MGMT_HEADER_SIZE       = 6
    };

    enum MgmtStatus : uint8_t {
        SUCCESS             = 0x00,
        UNKNOWN_COMMAND     = 0x01,
        NOT_CONNECTED       = 0x02,
        FAILED              = 0x03,
        CONNECT_FAILED      = 0x04,
        AUTH_FAILED         = 0x05,
        NOT_PAIRED          = 0x06,
        NO_RESOURCES        = 0x07,
        TIMEOUT             = 0x08,
        ALREADY_CONNECTED   = 0x09,
        BUSY                = 0x0a,
        REJECTED            = 0x0b,
        NOT_SUPPORTED       = 0x0c,
        INVALID_PARAMS      = 0x0d,
        DISCONNECTED        = 0x0e,
        NOT_POWERED         = 0x0f,
        CANCELLED           = 0x10,
        INVALID_INDEX       = 0x11,
        RFKILLED            = 0x12,
        ALREADY_PAIRED      = 0x13,
        PERMISSION_DENIED   = 0x14
    };

    std::string getMgmtStatusString(const MgmtStatus opc);

    enum MgmtOpcode : uint16_t {
        READ_VERSION            = 0x0001,
        READ_COMMANDS           = 0x0002,
        READ_INDEX_LIST         = 0x0003,
        READ_INFO               = 0x0004,
        SET_POWERED             = 0x0005, // uint8_t bool
        SET_DISCOVERABLE        = 0x0006, // uint8_t bool [+ uint16_t timeout]
        SET_CONNECTABLE         = 0x0007, // uint8_t bool
        SET_FAST_CONNECTABLE    = 0x0008, // uint8_t bool
        SET_BONDABLE            = 0x0009, // uint8_t bool
        SET_LINK_SECURITY       = 0x000A,
        SET_SSP                 = 0x000B,
        SET_HS                  = 0x000C,
        SET_LE                  = 0x000D, // uint8_t bool
        SET_DEV_CLASS           = 0x000E, // uint8_t major, uint8_t minor
        SET_LOCAL_NAME          = 0x000F, // uint8_t name[MAX_NAME_LENGTH], uint8_t short_name[MAX_SHORT_NAME_LENGTH];
        ADD_UUID                = 0x0010,
        REMOVE_UUID             = 0x0011,
        LOAD_LINK_KEYS          = 0x0012,
        LOAD_LONG_TERM_KEYS     = 0x0013,
        DISCONNECT              = 0x0014,
        GET_CONNECTIONS         = 0x0015,
        PIN_CODE_REPLY          = 0x0016,
        PIN_CODE_NEG_REPLY      = 0x0017,
        SET_IO_CAPABILITY       = 0x0018,
        PAIR_DEVICE             = 0x0019,
        CANCEL_PAIR_DEVICE      = 0x001A,
        UNPAIR_DEVICE           = 0x001B,
        USER_CONFIRM_REPLY      = 0x001C,
        USER_CONFIRM_NEG_REPLY  = 0x001D,
        USER_PASSKEY_REPLY      = 0x001E,
        USER_PASSKEY_NEG_REPLY  = 0x001F,
        READ_LOCAL_OOB_DATA     = 0x0020,
        ADD_REMOTE_OOB_DATA     = 0x0021,
        REMOVE_REMOTE_OOB_DATA  = 0x0022,
        START_DISCOVERY         = 0x0023,
        STOP_DISCOVERY          = 0x0024,
        CONFIRM_NAME            = 0x0025,
        BLOCK_DEVICE            = 0x0026,
        UNBLOCK_DEVICE          = 0x0027,
        SET_DEVICE_ID           = 0x0028,
        SET_ADVERTISING         = 0x0029,
        SET_BREDR               = 0x002A,
        SET_STATIC_ADDRESS      = 0x002B,
        SET_SCAN_PARAMS         = 0x002C,
        SET_SECURE_CONN         = 0x002D,
        SET_DEBUG_KEYS          = 0x002E,
        SET_PRIVACY             = 0x002F,
        LOAD_IRKS               = 0x0030,
        GET_CONN_INFO           = 0x0031,
        GET_CLOCK_INFO          = 0x0032,
        ADD_DEVICE              = 0x0033,
        REMOVE_DEVICE           = 0x0034,
        LOAD_CONN_PARAM         = 0x0035,
        READ_UNCONF_INDEX_LIST  = 0x0036,
        READ_CONFIG_INFO        = 0x0037,
        SET_EXTERNAL_CONFIG     = 0x0038,
        SET_PUBLIC_ADDRESS      = 0x0039,
        START_SERVICE_DISCOVERY = 0x003A,
        READ_LOCAL_OOB_EXT_DATA = 0x003B,
        READ_EXT_INDEX_LIST     = 0x003C,
        READ_ADV_FEATURES       = 0x003D,
        ADD_ADVERTISING         = 0x003E,
        REMOVE_ADVERTISING      = 0x003F,
        GET_ADV_SIZE_INFO       = 0x0040,
        START_LIMITED_DISCOVERY = 0x0041,
        READ_EXT_INFO           = 0x0042,
        SET_APPEARANCE          = 0x0043,
        GET_PHY_CONFIGURATION   = 0x0044,
        SET_PHY_CONFIGURATION   = 0x0045,
        SET_BLOCKED_KEYS        = 0x0046
    };

    std::string getMgmtOpcodeString(const MgmtOpcode op);

    enum MgmtOption : uint32_t {
        EXTERNAL_CONFIG     = 0x00000001,
        PUBLIC_ADDRESS      = 0x00000002
    };

    enum ScanType : uint8_t {
        SCAN_TYPE_NONE  = 0,
        SCAN_TYPE_BREDR = 1 << BDAddressType::BDADDR_BREDR,
        SCAN_TYPE_LE    = ( 1 << BDAddressType::BDADDR_LE_PUBLIC ) | ( 1 << BDAddressType::BDADDR_LE_RANDOM ),
        SCAN_TYPE_DUAL  = SCAN_TYPE_BREDR | SCAN_TYPE_LE
    };

    class MgmtCommand
    {
        protected:
            POctets pdu;

            inline static void checkOpcode(const MgmtOpcode has, const MgmtOpcode min, const MgmtOpcode max)
            {
                if( has < min || has > max ) {
                    throw MgmtOpcodeException("Has opcode "+uint16HexString(has, true)+
                                     ", not within range ["+uint16HexString(min, true)+".."+uint16HexString(max, true)+"]", E_FILE_LINE);
                }
            }

            virtual std::string baseString() const {
                return "opcode="+uint8HexString(getOpcode(), true)+" "+getOpcodeString()+", devID "+uint16HexString(getDevID(), true);
            }
            virtual std::string valueString() const {
                const int psz = getParamSize();
                const std::string ps = psz > 0 ? bytesHexString(getParam(), 0, psz, true /* lsbFirst */, true /* leading0X */) : "";
                return "param[size "+std::to_string(getParamSize())+", data "+ps+"], tsz "+std::to_string(getTotalSize());
            }

        public:

            MgmtCommand(const MgmtOpcode opc, const uint16_t dev_id, const uint16_t param_size=0)
            : pdu(MGMT_HEADER_SIZE+param_size)
            {
                checkOpcode(opc, READ_VERSION, SET_BLOCKED_KEYS);

                pdu.put_uint16(0, opc);
                pdu.put_uint16(2, dev_id);
                pdu.put_uint16(4, param_size);
            }
            MgmtCommand(const MgmtOpcode opc, const uint16_t dev_id, const uint16_t param_size, const uint8_t* param)
            : MgmtCommand(opc, dev_id, param_size)
            {
                if( param_size > 0 ) {
                    memcpy(pdu.get_wptr(MGMT_HEADER_SIZE), param, param_size);
                }
            }
            virtual ~MgmtCommand() {}

            int getTotalSize() const { return pdu.getSize(); }

            /** Return the underlying octets read only */
            TROOctets & getPDU() { return pdu; }

            MgmtOpcode getOpcode() const { return static_cast<MgmtOpcode>( pdu.get_uint16(0) ); }
            std::string getOpcodeString() const { return getMgmtOpcodeString(getOpcode()); }
            uint16_t getDevID() const { return pdu.get_uint16(2); }
            uint16_t getParamSize() const { return pdu.get_uint16(4); }
            const uint8_t* getParam() const { return pdu.get_ptr(MGMT_HEADER_SIZE); }

            std::string toString() const {
                return "MgmtReq["+baseString()+", "+valueString()+"]";
            }
    };

    class MgmtUint8Cmd : public MgmtCommand
    {
        public:
            MgmtUint8Cmd(const MgmtOpcode opc, const uint16_t dev_id, const uint8_t data)
            : MgmtCommand(opc, dev_id, 1)
            {
                pdu.put_uint8(MGMT_HEADER_SIZE, data);
            }
    };


    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     */
    class MgmtDisconnectCmd : public MgmtCommand
    {
        public:
            MgmtDisconnectCmd(const uint16_t dev_id, const EUI48 &address, const BDAddressType addressType)
            : MgmtCommand(MgmtOpcode::DISCONNECT, dev_id, 6+1)
            {
                pdu.put_eui48(MGMT_HEADER_SIZE, address);
                pdu.put_uint8(MGMT_HEADER_SIZE+6, addressType);
            }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * uint8_t pin_len,
     * uint8_t pin_code[16]
     */
    class MgmtPinCodeReplyCmd : public MgmtCommand
    {
        public:
            MgmtPinCodeReplyCmd(const uint16_t dev_id, const EUI48 &address, const BDAddressType addressType,
                                const uint8_t pin_len, const TROOctets &pin_code)
            : MgmtCommand(MgmtOpcode::PIN_CODE_REPLY, dev_id, 6+1+1+16)
            {
                pdu.put_eui48(MGMT_HEADER_SIZE, address);
                pdu.put_uint8(MGMT_HEADER_SIZE+6, addressType);
                pdu.put_uint8(MGMT_HEADER_SIZE+7, pin_len);
                pdu.put_octets(MGMT_HEADER_SIZE+8, pin_code);
            }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     */
    class MgmtPinCodeNegativeReplyCmd : public MgmtCommand
    {
        public:
            MgmtPinCodeNegativeReplyCmd(const uint16_t dev_id, const EUI48 &address, const BDAddressType addressType)
            : MgmtCommand(MgmtOpcode::PIN_CODE_NEG_REPLY, dev_id, 6+1)
            {
                pdu.put_eui48(MGMT_HEADER_SIZE, address);
                pdu.put_uint8(MGMT_HEADER_SIZE+6, addressType);
            }
    };

    /**
     * uint16_t opcode,
     * uint16_t dev-id,
     * uint16_t param_size
     */
    class MgmtEvent
    {
        public:
            enum Opcode : uint16_t {
                CMD_COMPLETE               = 0x0001,
                CMD_STATUS                 = 0x0002,
                CONTROLLER_ERROR           = 0x0003,
                INDEX_ADDED                = 0x0004,
                INDEX_REMOVED              = 0x0005,
                NEW_SETTINGS               = 0x0006,
                CLASS_OF_DEV_CHANGED       = 0x0007,
                LOCAL_NAME_CHANGED         = 0x0008,
                NEW_LINK_KEY               = 0x0009,
                NEW_LONG_TERM_KEY          = 0x000A,
                DEVICE_CONNECTED           = 0x000B,
                DEVICE_DISCONNECTED        = 0x000C,
                CONNECT_FAILED             = 0x000D,
                PIN_CODE_REQUEST           = 0x000E,
                USER_CONFIRM_REQUEST       = 0x000F,
                USER_PASSKEY_REQUEST       = 0x0010,
                AUTH_FAILED                = 0x0011,
                DEVICE_FOUND               = 0x0012,
                DISCOVERING                = 0x0013,
                DEVICE_BLOCKED             = 0x0014,
                DEVICE_UNBLOCKED           = 0x0015,
                DEVICE_UNPAIRED            = 0x0016,
                PASSKEY_NOTIFY             = 0x0017,
                NEW_IRK                    = 0x0018,
                NEW_CSRK                   = 0x0019,
                DEVICE_WHITELIST_ADDED     = 0x001A,
                DEVICE_WHITELIST_REMOVED   = 0x001B,
                NEW_CONN_PARAM             = 0x001C,
                UNCONF_INDEX_ADDED         = 0x001D,
                UNCONF_INDEX_REMOVED       = 0x001E,
                NEW_CONFIG_OPTIONS         = 0x001F,
                EXT_INDEX_ADDED            = 0x0020,
                EXT_INDEX_REMOVED          = 0x0021,
                LOCAL_OOB_DATA_UPDATED     = 0x0022,
                ADVERTISING_ADDED          = 0x0023,
                ADVERTISING_REMOVED        = 0x0024,
                EXT_INFO_CHANGED           = 0x0025,
                PHY_CONFIGURATION_CHANGED  = 0x0026,
                MGMT_EVENT_TYPE_COUNT      = 0x0026
            };

            static std::string getOpcodeString(const Opcode opc);

        protected:
            /** actual received mgmt event */
            POctets pdu;
            uint64_t ts_creation;

            static void checkOpcode(const Opcode has, const Opcode min, const Opcode max)
            {
                if( has < min || has > max ) {
                    throw MgmtOpcodeException("Has evcode "+uint16HexString(has, true)+
                                     ", not within range ["+uint16HexString(min, true)+".."+uint16HexString(max, true)+"]", E_FILE_LINE);
                }
            }
            static void checkOpcode(const Opcode has, const Opcode exp)
            {
                if( has != exp ) {
                    throw MgmtOpcodeException("Has evcode "+uint16HexString(has, true)+
                                     ", not matching "+uint16HexString(exp, true), E_FILE_LINE);
                }
            }

            virtual std::string baseString() const {
                return "opcode="+uint8HexString(getOpcode(), true)+" "+getOpcodeString()+", devID "+uint16HexString(getDevID(), true);
            }
            virtual std::string valueString() const {
                const int d_sz = getDataSize();
                const std::string d_str = d_sz > 0 ? bytesHexString(getData(), 0, d_sz, true /* lsbFirst */, true /* leading0X */) : "";
                return "data[size "+std::to_string(d_sz)+", data "+d_str+"], tsz "+std::to_string(getTotalSize());
            }

        public:

            /**
             * Return a newly created specialized instance pointer to base class.
             * <p>
             * Returned memory reference is managed by caller (delete etc)
             * </p>
             */
            static MgmtEvent* getSpecialized(const uint8_t * buffer, int const buffer_size);

            /** Persistent memory, w/ ownership ..*/
            MgmtEvent(const uint8_t* buffer, const int buffer_len)
            : pdu(buffer, buffer_len), ts_creation(getCurrentMilliseconds())
            {
                pdu.check_range(0, MGMT_HEADER_SIZE+getParamSize());
                checkOpcode(getOpcode(), CMD_COMPLETE, PHY_CONFIGURATION_CHANGED);
            }
            MgmtEvent(const Opcode opc, const uint16_t dev_id, const uint16_t param_size=0)
            : pdu(MGMT_HEADER_SIZE+param_size), ts_creation(getCurrentMilliseconds())
            {
                // checkOpcode(opc, READ_VERSION, SET_BLOCKED_KEYS);

                pdu.put_uint16(0, opc);
                pdu.put_uint16(2, dev_id);
                pdu.put_uint16(4, param_size);
            }
            MgmtEvent(const Opcode opc, const uint16_t dev_id, const uint16_t param_size, const uint8_t* param)
            : MgmtEvent(opc, dev_id, param_size)
            {
                if( param_size > 0 ) {
                    memcpy(pdu.get_wptr(MGMT_HEADER_SIZE), param, param_size);
                }
            }
            virtual ~MgmtEvent() {}

            int getTotalSize() const { return pdu.getSize(); }

            uint64_t getTimestamp() const { return ts_creation; }
            Opcode getOpcode() const { return static_cast<Opcode>( pdu.get_uint16(0) ); }
            std::string getOpcodeString() const { return getOpcodeString(getOpcode()); }
            uint16_t getDevID() const { return pdu.get_uint16(2); }
            uint16_t getParamSize() const { return pdu.get_uint16(4); }

            virtual int getDataOffset() const { return MGMT_HEADER_SIZE; }
            virtual int getDataSize() const { return getParamSize(); }
            virtual const uint8_t* getData() const { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }

            virtual bool validate(const MgmtCommand &req) const {
                return req.getDevID() == getDevID();
            }

            std::string toString() const {
                return "MgmtEvt["+baseString()+", "+valueString()+"]";
            }
    };

    class MgmtEvtCmdComplete : public MgmtEvent
    {
        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", req-opcode="+uint8HexString(getReqOpcode(), true)+" "+getMgmtOpcodeString(getReqOpcode())+
                       ", status "+uint8HexString(getStatus(), true)+" "+getMgmtStatusString(getStatus());
            }

        public:
            static MgmtOpcode getReqOpcode(const uint8_t *data) {
                return static_cast<MgmtOpcode>( get_uint16(data, MGMT_HEADER_SIZE, true /* littleEndian */) );
            }

            MgmtEvtCmdComplete(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), CMD_COMPLETE);
            }
            MgmtOpcode getReqOpcode() const { return static_cast<MgmtOpcode>( pdu.get_uint16(MGMT_HEADER_SIZE) ); }
            MgmtStatus getStatus() const { return static_cast<MgmtStatus>( pdu.get_uint8(MGMT_HEADER_SIZE+2) ); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+3; }
            int getDataSize() const override { return getParamSize()-3; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }

            bool validate(const MgmtCommand &req) const override {
                return MgmtEvent::validate(req) && req.getOpcode() == getReqOpcode();
            }
    };

    class MgmtEvtCmdStatus : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", req-opcode="+uint8HexString(getReqOpcode(), true)+" "+getMgmtOpcodeString(getReqOpcode())+
                       ", status "+uint8HexString(getStatus(), true)+" "+getMgmtStatusString(getStatus());
            }

        public:
            MgmtEvtCmdStatus(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), CMD_STATUS);
            }
            MgmtOpcode getReqOpcode() const { return static_cast<MgmtOpcode>( pdu.get_uint16(MGMT_HEADER_SIZE) ); }
            MgmtStatus getStatus() const { return static_cast<MgmtStatus>( pdu.get_uint8(MGMT_HEADER_SIZE+2) ); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+3; }
            int getDataSize() const override { return 0; }
            const uint8_t* getData() const override { return nullptr; }

            bool validate(const MgmtCommand &req) const override {
                return MgmtEvent::validate(req) && req.getOpcode() == getReqOpcode();
            }
    };

    class MgmtEvtDiscovering : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", scan-type="+uint8HexString(getScanType(), true)+
                       ", enabled "+std::to_string(getEnabled());
            }

        public:
            MgmtEvtDiscovering(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), DISCOVERING);
            }
            ScanType getScanType() const { return static_cast<ScanType>( pdu.get_uint8(MGMT_HEADER_SIZE) ); }
            bool getEnabled() const { return 0 != pdu.get_uint8(MGMT_HEADER_SIZE+1); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+2; }
            int getDataSize() const override { return 0; }
            const uint8_t* getData() const override { return nullptr; }
    };

    /**
     * uint32_t settings
     */
    class MgmtEvtNewSettings : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", settings="+adapterSettingsToString(getSettings());
            }

        public:
            MgmtEvtNewSettings(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), NEW_SETTINGS);
            }
            AdapterSetting getSettings() const { return static_cast<AdapterSetting>( pdu.get_uint32(MGMT_HEADER_SIZE) ); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+4; }
            int getDataSize() const override { return getParamSize()-4; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * int8_t store_hint,
     * uint16_t min_interval;
     * uint16_t max_interval;
     * uint16_t latency,
     * uint16_t timeout,
     */
    class MgmtEvtNewConnectionParam : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType())+
                       ", store-hint "+std::to_string(getStoreHint())+
                       ", interval["+std::to_string(getMinInterval())+".."+std::to_string(getMaxInterval())+
                       "], latency "+std::to_string(getLatency())+", timeout "+std::to_string(getTimeout());
            }

        public:
            MgmtEvtNewConnectionParam(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), NEW_CONN_PARAM);
            }
            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            int8_t getStoreHint() const { return pdu.get_int8(MGMT_HEADER_SIZE+7); }
            uint16_t getMinInterval() const { return pdu.get_uint32(MGMT_HEADER_SIZE+8); }
            uint16_t getMaxInterval() const { return pdu.get_uint32(MGMT_HEADER_SIZE+10); }
            uint16_t getLatency() const { return pdu.get_uint16(MGMT_HEADER_SIZE+12); }
            uint16_t getTimeout() const { return pdu.get_uint16(MGMT_HEADER_SIZE+14); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+14; }
            int getDataSize() const override { return getParamSize()-14; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * int8_t rssi,
     * uint32_t flags,
     * uint16_t eir_len;
     * uint8_t *eir
     */
    class MgmtEvtDeviceFound : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType())+
                       ", rssi "+std::to_string(getRSSI())+", flags="+uint32HexString(getFlags(), true)+
                       ", eir-size "+std::to_string(getEIRSize());
            }

        public:
            MgmtEvtDeviceFound(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), DEVICE_FOUND);
            }
            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            int8_t getRSSI() const { return pdu.get_int8(MGMT_HEADER_SIZE+7); }
            uint32_t getFlags() const { return pdu.get_uint32(MGMT_HEADER_SIZE+8); }
            uint16_t getEIRSize() const { return pdu.get_uint16(MGMT_HEADER_SIZE+12); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+14; }
            int getDataSize() const override { return getParamSize()-14; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * uint32_t flags,
     * uint16_t eir_len;
     * uint8_t *eir
     */
    class MgmtEvtDeviceConnected : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType())+
                       ", flags="+uint32HexString(getFlags(), true)+
                       ", eir-size "+std::to_string(getEIRSize());
            }

        public:
            MgmtEvtDeviceConnected(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), DEVICE_CONNECTED);
            }
            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            uint32_t getFlags() const { return pdu.get_uint32(MGMT_HEADER_SIZE+7); }
            uint16_t getEIRSize() const { return pdu.get_uint16(MGMT_HEADER_SIZE+11); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+13; }
            int getDataSize() const override { return getParamSize()-13; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * uint8_t status
     */
    class MgmtEvtDeviceConnectFailed : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType())+
                       ", status "+uint8HexString(getStatus(), true)+" "+getMgmtStatusString(getStatus());
            }

        public:
            MgmtEvtDeviceConnectFailed(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), CONNECT_FAILED);
            }
            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            MgmtStatus getStatus() const { return static_cast<MgmtStatus>( pdu.get_uint8(MGMT_HEADER_SIZE+7) ); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+8; }
            int getDataSize() const override { return getParamSize()-8; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * uint8_t reason
     */
    class MgmtEvtDeviceDisconnected : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType())+
                       ", reason "+std::to_string(getReason());
            }

        public:
            MgmtEvtDeviceDisconnected(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), DEVICE_DISCONNECTED);
            }
            MgmtEvtDeviceDisconnected(const uint16_t dev_id, const EUI48 &address, const BDAddressType addressType, uint8_t reason)
            : MgmtEvent(DEVICE_DISCONNECTED, dev_id, 6+1+1)
            {
                pdu.put_eui48(MGMT_HEADER_SIZE, address);
                pdu.put_uint8(MGMT_HEADER_SIZE+6, addressType);
                pdu.put_uint8(MGMT_HEADER_SIZE+6+1, reason);
            }

            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            uint8_t getReason() const { return pdu.get_uint8(MGMT_HEADER_SIZE+7); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+8; }
            int getDataSize() const override { return getParamSize()-8; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * uint8_t secure
     */
    class MgmtEvtPinCodeRequest : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType())+
                       ", secure "+std::to_string(getSecure());
            }

        public:
            MgmtEvtPinCodeRequest(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), PIN_CODE_REQUEST);
            }
            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            uint8_t getSecure() const { return pdu.get_uint8(MGMT_HEADER_SIZE+7); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+8; }
            int getDataSize() const override { return getParamSize()-8; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * uint8_t action
     */
    class MgmtEvtDeviceWhitelistAdded : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType())+
                       ", action "+std::to_string(getAction());
            }

        public:
            MgmtEvtDeviceWhitelistAdded(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), DEVICE_WHITELIST_ADDED);
            }
            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            uint8_t getAction() const { return pdu.get_uint8(MGMT_HEADER_SIZE+7); }

            int getDataOffset() const override { return MGMT_HEADER_SIZE+8; }
            int getDataSize() const override { return getParamSize()-8; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     */
    class MgmtEvtDeviceWhitelistRemoved : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType());
            }

        public:
            MgmtEvtDeviceWhitelistRemoved(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), DEVICE_WHITELIST_REMOVED);
            }
            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            int getDataOffset() const override { return MGMT_HEADER_SIZE+7; }
            int getDataSize() const override { return getParamSize()-7; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     */
    class MgmtEvtDeviceUnpaired : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", address="+getAddress().toString()+
                       ", addressType "+getBDAddressTypeString(getAddressType());
            }

        public:
            MgmtEvtDeviceUnpaired(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), DEVICE_UNPAIRED);
            }
            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(MGMT_HEADER_SIZE)); } // mgmt_addr_info
            BDAddressType getAddressType() const { return static_cast<BDAddressType>(pdu.get_uint8(MGMT_HEADER_SIZE+6)); } // mgmt_addr_info

            int getDataOffset() const override { return MGMT_HEADER_SIZE+7; }
            int getDataSize() const override { return getParamSize()-7; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }
    };

    /**
     * uint8_t name[MGMT_MAX_NAME_LENGTH];
     * uint8_t short_name[MGMT_MAX_SHORT_NAME_LENGTH];
     */
    class MgmtEvtLocalNameChanged : public MgmtEvent
    {
        protected:
            std::string valueString() const override {
                return "name '"+getName()+"', shortName '"+getShortName()+"'";
            }

        public:
            static int getRequiredSize() { return MGMT_HEADER_SIZE + MgmtConstU16::MAX_NAME_LENGTH + MgmtConstU16::MAX_SHORT_NAME_LENGTH; }

            MgmtEvtLocalNameChanged(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), LOCAL_NAME_CHANGED);
                pdu.check_range(0, getRequiredSize());
            }

            const std::string getName() const { return std::string( (const char*)pdu.get_ptr(MGMT_HEADER_SIZE) ); }
            const std::string getShortName() const { return std::string( (const char*)pdu.get_ptr(MGMT_HEADER_SIZE+MgmtConstU16::MAX_NAME_LENGTH) ); }
    };

    class MgmtEvtAdapterInfo : public MgmtEvtCmdComplete
    {
        protected:
            std::string valueString() const override {
                return getAddress().toString()+", version "+std::to_string(getVersion())+
                        ", manuf "+std::to_string(getManufacturer())+
                        ", settings[sup "+adapterSettingsToString(getSupportedSetting())+", cur "+adapterSettingsToString(getCurrentSetting())+
                        "], name '"+getName()+"', shortName '"+getShortName()+"'";
            }

        public:
            static int getRequiredSize() { return MGMT_HEADER_SIZE + 3 + 20 + MgmtConstU16::MAX_NAME_LENGTH + MgmtConstU16::MAX_SHORT_NAME_LENGTH; }

            MgmtEvtAdapterInfo(const uint8_t* buffer, const int buffer_len)
            : MgmtEvtCmdComplete(buffer, buffer_len)
            {
                pdu.check_range(0, getRequiredSize());
            }

            const EUI48 getAddress() const { return EUI48(pdu.get_ptr(getDataOffset()+0)); }
            uint8_t getVersion() const { return pdu.get_uint8(getDataOffset()+6); }
            uint16_t getManufacturer() const { return pdu.get_uint16(getDataOffset()+7); }
            AdapterSetting getSupportedSetting() const { return static_cast<AdapterSetting>( pdu.get_uint32(getDataOffset()+9) ); }
            AdapterSetting getCurrentSetting() const { return static_cast<AdapterSetting>( pdu.get_uint32(getDataOffset()+13) ); }
            uint32_t getDevClass() const { return pdu.get_uint8(getDataOffset()+17)
                                                  | ( pdu.get_uint8(getDataOffset()+18) << 8 )
                                                  | ( pdu.get_uint8(getDataOffset()+19) << 16 ); }
            const std::string getName() const { return std::string( (const char*)pdu.get_ptr(getDataOffset()+20) ); }
            const std::string getShortName() const { return std::string( (const char*)pdu.get_ptr(getDataOffset()+20+MgmtConstU16::MAX_NAME_LENGTH) ); }
    };

    class DBTManager; // forward
    class DBTAdapter; // forward

    /** Immutable persistent adapter info */
    class AdapterInfo
    {
        friend class DBTManager; // top manager
        friend class DBTAdapter; // direct manager

        public:
            const int dev_id;
            const EUI48 address;
            const uint8_t version;
            const uint16_t manufacturer;
            const AdapterSetting supported_setting;

        private:
            AdapterSetting current_setting;
            uint32_t dev_class;
            std::string name;
            std::string short_name;

            /**
             * Sets the current_setting and returns the changed MgmtSetting bit-mask.
             */
            AdapterSetting setCurrentSetting(AdapterSetting new_setting) {
                new_setting = new_setting & supported_setting;
                AdapterSetting changes = new_setting ^ current_setting;

                if( AdapterSetting::NONE != changes ) {
                    current_setting = new_setting;
                }
                return changes;
            }
            void setDevClass(const uint32_t v) { dev_class = v; }
            void setName(const std::string v) { name = v; }
            void setShortName(const std::string v) { short_name = v; }

        public:
            AdapterInfo(const MgmtEvtAdapterInfo &s)
            : dev_id(s.getDevID()), address(s.getAddress()), version(s.getVersion()),
              manufacturer(s.getManufacturer()), supported_setting(s.getSupportedSetting()),
              current_setting(s.getCurrentSetting()), dev_class(s.getDevClass()),
              name(s.getName()), short_name(s.getShortName())
            { }

            bool isSettingSupported(const AdapterSetting setting) const {
                return setting == ( setting & supported_setting );
            }
            AdapterSetting getCurrentSetting() const { return current_setting; }
            uint32_t getDevClass() const { return dev_class; }
            const std::string getName() const { return name; }
            const std::string getShortName() const { return short_name; }

            std::string toString() const {
                return "Adapter[id "+std::to_string(dev_id)+", address "+address.toString()+", version "+std::to_string(version)+
                        ", manuf "+std::to_string(manufacturer)+
                        ", settings[sup "+adapterSettingsToString(supported_setting)+", cur "+adapterSettingsToString(current_setting)+
                        "], name '"+name+"', shortName '"+short_name+"']";
            }
    };

} // namespace direct_bt

#endif /* MGMT_TYPES_HPP_ */
