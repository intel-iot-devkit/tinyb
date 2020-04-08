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

#ifndef MGMT_COMM_HPP_
#define MGMT_COMM_HPP_

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

    enum MgmtConst : uint16_t {
        INDEX_NONE          = 0xFFFF,
        /* Net length, guaranteed to be null-terminated */
        MAX_NAME_LENGTH        = 248+1,
        MAX_SHORT_NAME_LENGTH  =  10+1
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

    std::string mgmt_get_status_string(const MgmtStatus opc);

    enum MgmtOperation : uint16_t {
        READ_VERSION            = 0x0001,
        READ_COMMANDS           = 0x0002,
        READ_INDEX_LIST         = 0x0003,
        READ_INFO               = 0x0004,
        SET_POWERED             = 0x0005,
        SET_DISCOVERABLE        = 0x0006,
        SET_CONNECTABLE         = 0x0007,
        SET_FAST_CONNECTABLE    = 0x0008,
        SET_BONDABLE            = 0x0009,
        SET_LINK_SECURITY       = 0x000A,
        SET_SSP                 = 0x000B,
        SET_HS                  = 0x000C,
        SET_LE                  = 0x000D,
        SET_DEV_CLASS           = 0x000E,
        SET_LOCAL_NAME          = 0x000F,
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

    std::string mgmt_get_operation_string(const MgmtOperation op);

    enum MgmtOption : uint32_t {
        EXTERNAL_CONFIG     = 0x00000001,
        PUBLIC_ADDRESS      = 0x00000002
    };

    class MgmtRequest
    {
        public:
            enum Opcode : uint16_t {
                READ_VERSION           = 0x0001,
                READ_COMMANDS          = 0x0002,
                READ_INDEX_LIST        = 0x0003,
                READ_INFO              = 0x0004,
                SET_POWERED            = 0x0005, // uint8_t bool
                SET_DISCOVERABLE       = 0x0006, // uint8_t bool [+ uint16_t timeout]
                SET_CONNECTABLE        = 0x0007, // uint8_t bool
                SET_FAST_CONNECTABLE   = 0x0008, // uint8_t bool
                SET_BONDABLE           = 0x0009, // uint8_t bool
                SET_LINK_SECURITY      = 0x000A,
                SET_SSP                = 0x000B,
                SET_HS                 = 0x000C,
                SET_LE                 = 0x000D, // uint8_t bool
                SET_DEV_CLASS          = 0x000E, // uint8_t major, uint8_t minor
                SET_LOCAL_NAME         = 0x000F  // uint8_t name[MAX_NAME_LENGTH], uint8_t short_name[MAX_SHORT_NAME_LENGTH];
            };

            static std::string getOpcodeString(const Opcode opc);

        protected:
            POctets pdu;

            int write(const int dd);
            int read(const int dd, uint8_t* buffer, const int capacity, const int timeoutMS);

            static void checkOpcode(const Opcode has, const Opcode min, const Opcode max)
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

            MgmtRequest(const Opcode opc, const uint16_t dev_id, const uint16_t param_size=0)
            : pdu(6+param_size)
            {
                checkOpcode(opc, READ_VERSION, SET_LOCAL_NAME);

                pdu.put_uint16(0, htobs(opc));
                pdu.put_uint16(2, htobs(dev_id));
                pdu.put_uint16(4, htobs(param_size));
            }
            MgmtRequest(const Opcode opc, const uint16_t dev_id, const uint16_t param_size, const uint8_t* param)
            : MgmtRequest(opc, dev_id, param_size)
            {
                if( param_size > 0 ) {
                    memcpy(pdu.get_wptr(6), param, param_size);
                }
            }
            virtual ~MgmtRequest() {}

            int getTotalSize() const { return pdu.getSize(); }

            Opcode getOpcode() const { return static_cast<Opcode>( btohs( pdu.get_uint16(0) ) ); }
            std::string getOpcodeString() const { return getOpcodeString(getOpcode()); }
            uint16_t getDevID() const { return btohs( pdu.get_uint16(2) ); }
            uint16_t getParamSize() const { return btohs( pdu.get_uint16(4) ); }
            const uint8_t* getParam() const { return pdu.get_ptr(6); }

            /** writes request to dd and reads result into buffer */
            int send(const int dd, uint8_t* buffer, const int capacity, const int timeoutMS);

            std::string toString() const {
                return "MgmtReq["+baseString()+", "+valueString()+"]";
            }
    };

    class MgmtModeReq : public MgmtRequest
    {
        public:
            MgmtModeReq(const Opcode opc, const uint16_t dev_id, const uint8_t mode)
            : MgmtRequest(opc, dev_id, 1)
            {
                checkOpcode(opc, SET_POWERED, SET_LE);
                pdu.put_uint8(6, mode);
            }
    };

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
                DEVICE_ADDED               = 0x001A,
                DEVICE_REMOVED             = 0x001B,
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
                PHY_CONFIGURATION_CHANGED  = 0x0026
            };

            static std::string getOpcodeString(const Opcode opc);

        protected:
            TROOctets pdu;

            static void checkOpcode(const Opcode has, const Opcode min, const Opcode max)
            {
                if( has < min || has > max ) {
                    throw MgmtOpcodeException("Has evcode "+uint16HexString(has, true)+
                                     ", not within range ["+uint16HexString(min, true)+".."+uint16HexString(max, true)+"]", E_FILE_LINE);
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
             * Returned memory reference must be deleted by caller.
             * </p>
             * <p>
             * Since we use transient passthrough memory, w/o ownership,
             * actual memory is reused and not copied.
             * Caller is responsible for the memory lifecycle.
             * </p>
             */
            static MgmtEvent* getSpecialized(const uint8_t * buffer, int const buffer_size);

            MgmtEvent(const uint8_t* buffer, const int buffer_len)
            : pdu(buffer, buffer_len)
            {
                pdu.check_range(0, 6+getParamSize());
                checkOpcode(getOpcode(), CMD_COMPLETE, PHY_CONFIGURATION_CHANGED);
            }
            virtual ~MgmtEvent() {}

            int getTotalSize() const { return pdu.getSize(); }

            Opcode getOpcode() const { return static_cast<Opcode>( btohs( pdu.get_uint16(0) ) ); }
            std::string getOpcodeString() const { return getOpcodeString(getOpcode()); }
            uint16_t getDevID() const { return btohs( pdu.get_uint16(2) ); }
            uint16_t getParamSize() const { return btohs( pdu.get_uint16(4) ); }

            virtual const int getDataOffset() const { return 6; }
            virtual const int getDataSize() const { return getParamSize(); }
            virtual const uint8_t* getData() const { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }

            virtual bool validate(const MgmtRequest &req) const {
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
                return MgmtEvent::baseString()+", req-opcode="+uint8HexString(getReqOpcode(), true)+" "+MgmtRequest::getOpcodeString(getReqOpcode())+
                       ", status "+uint8HexString(getStatus(), true)+" "+mgmt_get_status_string(getStatus());
            }

        public:
            static MgmtRequest::Opcode getReqOpcode(const uint8_t *data) {
                return static_cast<MgmtRequest::Opcode>( get_uint16(data, 6, true /* littleEndian */) );
            }

            MgmtEvtCmdComplete(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), CMD_COMPLETE, CMD_COMPLETE);
            }
            MgmtRequest::Opcode getReqOpcode() const { return static_cast<MgmtRequest::Opcode>( btohs( pdu.get_uint16(6) ) ); }
            MgmtStatus getStatus() const { return static_cast<MgmtStatus>( pdu.get_uint8(8) ); }

            const int getDataOffset() const override { return 9; }
            const int getDataSize() const override { return getParamSize()-3; }
            const uint8_t* getData() const override { return getDataSize()>0 ? pdu.get_ptr(getDataOffset()) : nullptr; }

            bool validate(const MgmtRequest &req) const override {
                return MgmtEvent::validate(req) && req.getOpcode() == getReqOpcode();
            }
    };

    class MgmtEvtCmdStatus : public MgmtEvent
    {
        public:

        protected:
            std::string baseString() const override {
                return MgmtEvent::baseString()+", req-opcode="+uint8HexString(getReqOpcode(), true)+" "+MgmtRequest::getOpcodeString(getReqOpcode())+
                       ", status "+uint8HexString(getStatus(), true)+" "+mgmt_get_status_string(getStatus());
            }

        public:
            MgmtEvtCmdStatus(const uint8_t* buffer, const int buffer_len)
            : MgmtEvent(buffer, buffer_len)
            {
                checkOpcode(getOpcode(), CMD_STATUS, CMD_STATUS);
            }
            MgmtRequest::Opcode getReqOpcode() const { return static_cast<MgmtRequest::Opcode>( btohs( pdu.get_uint16(6) ) ); }
            MgmtStatus getStatus() const { return static_cast<MgmtStatus>( pdu.get_uint8(8) ); }

            const int getDataOffset() const override { return 9; }
            const int getDataSize() const override { return 0; }
            const uint8_t* getData() const override { return nullptr; }

            bool validate(const MgmtRequest &req) const override {
                return MgmtEvent::validate(req) && req.getOpcode() == getReqOpcode();
            }
    };

    class MgmtEvtAdapterInfo : public MgmtEvtCmdComplete
    {
        protected:
            std::string valueString() const override {
                return getMAC().toString()+", version "+std::to_string(getVersion())+
                        ", manuf "+std::to_string(getManufacturer())+
                        ", settings[sup "+uint32HexString(getSupportedSetting(), true)+", cur "+uint32HexString(getCurrentSetting(), true)+
                        "], name "+getName()+", shortName "+getShortName();
            }

        public:
            static int getRequiredSize() { return 9 + 20 + MgmtConst::MAX_NAME_LENGTH + MgmtConst::MAX_SHORT_NAME_LENGTH; }

            MgmtEvtAdapterInfo(const uint8_t* buffer, const int buffer_len)
            : MgmtEvtCmdComplete(buffer, buffer_len)
            {
                pdu.check_range(0, getRequiredSize());
            }

            const EUI48 getMAC() const { return EUI48(pdu.get_ptr(getDataOffset()+0)); }
            const uint8_t getVersion() const { return pdu.get_uint8(getDataOffset()+6); }
            const uint16_t getManufacturer() const { return pdu.get_uint16(getDataOffset()+7); }
            const uint32_t getSupportedSetting() const { return pdu.get_uint32(getDataOffset()+9); }
            const uint32_t getCurrentSetting() const { return pdu.get_uint32(getDataOffset()+13); }
            const uint32_t getDevClass() const { return pdu.get_uint8(getDataOffset()+17)
                                                        | ( pdu.get_uint8(getDataOffset()+18) << 8 )
                                                        | ( pdu.get_uint8(getDataOffset()+19) << 16 ); }
            const std::string getName() const { return std::string( (const char*)pdu.get_ptr(getDataOffset()+20) ); }
            const std::string getShortName() const { return std::string( (const char*)pdu.get_ptr(getDataOffset()+20+MgmtConst::MAX_NAME_LENGTH) ); }
    };

    /** Immutable persistent adapter info */
    class AdapterInfo
    {
        public:
            const int dev_id;
            const EUI48 mac;
            const uint8_t version;
            const uint16_t manufacturer;
            const uint32_t supported_setting;
            const uint32_t current_setting;
            const uint32_t dev_class;
            const std::string name;
            const std::string short_name;

            AdapterInfo(const MgmtEvtAdapterInfo &s)
            : dev_id(s.getDevID()), mac(s.getMAC()), version(s.getVersion()), manufacturer(s.getManufacturer()),
              supported_setting(s.getSupportedSetting()), current_setting(s.getCurrentSetting()),
              dev_class(s.getDevClass()), name(s.getName()), short_name(s.getShortName())
            { }

            std::string toString() const {
                return "Adapter[id "+std::to_string(dev_id)+", mac "+mac.toString()+", version "+std::to_string(version)+
                        ", manuf "+std::to_string(manufacturer)+
                        ", settings[sup "+uint32HexString(supported_setting, true)+", cur "+uint32HexString(current_setting, true)+
                        "], name '"+name+"', shortName '"+short_name+"']";
            }
    };

    // *************************************************
    // *************************************************
    // *************************************************


    /**
     * A thread safe singleton handler of the Linux Kernel's BlueZ manager control channel.
     */
    class MgmtHandler {
        private:
            std::recursive_mutex mtx;
            const int ibuffer_size = 512;
            uint8_t ibuffer[512];
            std::vector<std::shared_ptr<const AdapterInfo>> adapters;
            HCIComm comm;

            int read(uint8_t* buffer, const int capacity, const int timeoutMS);
            int write(const uint8_t * buffer, const int length);

            MgmtHandler();
            MgmtHandler(const MgmtHandler&) = delete;
            void operator=(const MgmtHandler&) = delete;
            void close();

            bool initAdapter(const uint16_t dev_id);

        public:
            /**
             * Retrieves the singleton instance.
             * <p>
             * First call will open and initialize the bluetooth kernel.
             * </p>
             */
            static MgmtHandler& get() {
                /**
                 * Thread safe starting with C++11 6.7:
                 *
                 * If control enters the declaration concurrently while the variable is being initialized,
                 * the concurrent execution shall wait for completion of the initialization.
                 *
                 * (Magic Statics)
                 *
                 * Avoiding non-working double checked locking.
                 */
                static MgmtHandler s;
                return s;
            }
            ~MgmtHandler() { close(); }

            /** Returns true if this mgmt instance is open and hence valid, otherwise false */
            bool isOpen() const {
                return comm.isOpen();
            }
            bool setMode(const int dev_id, const MgmtModeReq::Opcode opc, const uint8_t mode);

            /**
             * In case response size check or devID and optional opcode validation fails,
             * function returns NULL.
             */
            std::shared_ptr<MgmtEvent> send(MgmtRequest &req, uint8_t* buffer, const int capacity, const int timeoutMS);

            const std::vector<std::shared_ptr<const AdapterInfo>> getAdapters() const { return adapters; }
            int getDefaultAdapterIdx() const { return adapters.size() > 0 ? 0 : -1; }
            int findAdapterIdx(const EUI48 &mac) const;
            std::shared_ptr<const AdapterInfo> getAdapter(const int idx) const;
    };

} // namespace direct_bt

#endif /* MGMT_COMM_HPP_ */
