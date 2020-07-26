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

#ifndef DBT_TYPES_HPP_
#define DBT_TYPES_HPP_

#include <mutex>
#include <atomic>

#include "UUID.hpp"
#include "BTAddress.hpp"
#include "BTTypes.hpp"

#include "JavaUplink.hpp"

#define JAVA_MAIN_PACKAGE "org/tinyb"
#define JAVA_HCI_PACKAGE "tinyb/hci"

namespace direct_bt {

    class DBTAdapter; // forward
    class DBTDevice; // forward

    class DBTObject : public JavaUplink
    {
        protected:
            std::atomic_bool valid;
            std::mutex lk;

            DBTObject() : valid(true) {}

            bool lock() {
                 if (valid) {
                     lk.lock();
                     return true;
                 } else {
                     return false;
                 }
             }

             void unlock() {
                 lk.unlock();
             }

        public:
            virtual ~DBTObject() {
                valid = false;
            }

            inline bool isValid() const { return valid.load(); }

            /**
             * Throws an IllegalStateException if isValid() == false
             */
            inline void checkValid() const {
                if( !isValid() ) {
                    throw IllegalStateException("DBTObject state invalid: "+aptrHexString(this), E_FILE_LINE);
                }
            }
    };

    /**
     * mgmt_addr_info { EUI48, uint8_t type },
     * int8_t rssi,
     * int8_t tx_power,
     * int8_t max_tx_power;
     */
    class ConnectionInfo
    {
        private:
            EUI48 address;
            BDAddressType addressType;
            int8_t rssi;
            int8_t tx_power;
            int8_t max_tx_power;

        public:
            static int minimumDataSize() { return 6 + 1 + 1 + 1 + 1; }

            ConnectionInfo(const EUI48 &address, BDAddressType addressType, int8_t rssi, int8_t tx_power, int8_t max_tx_power)
            : address(address), addressType(addressType), rssi(rssi), tx_power(tx_power), max_tx_power(max_tx_power) {}

            const EUI48 getAddress() const { return address; }
            BDAddressType getAddressType() const { return addressType; }
            int8_t getRSSI() const { return rssi; }
            int8_t getTxPower() const { return tx_power; }
            int8_t getMaxTxPower() const { return max_tx_power; }

            std::string toString() const {
                return "address="+getAddress().toString()+", addressType "+getBDAddressTypeString(getAddressType())+
                       ", rssi "+std::to_string(rssi)+
                       ", tx_power[set "+std::to_string(tx_power)+", max "+std::to_string(tx_power)+"]";
            }
    };

    class NameAndShortName
    {
        friend class DBTManager; // top manager
        friend class DBTAdapter; // direct manager

        private:
            std::string name;
            std::string short_name;

        protected:
            void setName(const std::string v) { name = v; }
            void setShortName(const std::string v) { short_name = v; }

        public:
            NameAndShortName()
            : name(), short_name() {}

            NameAndShortName(const std::string & name, const std::string & short_name)
            : name(name), short_name(short_name) {}

            std::string getName() const { return name; }
            std::string getShortName() const { return short_name; }

            std::string toString() const {
                return "name '"+getName()+"', shortName '"+getShortName()+"'";
            }
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
    std::string getAdapterSettingBitString(const AdapterSetting settingBit);
    std::string getAdapterSettingsString(const AdapterSetting settingBitMask);

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
             * Sets the current_setting and returns the changed AdapterSetting bit-mask.
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
            AdapterInfo(const int dev_id, const EUI48 & address,
                        const uint8_t version, const uint16_t manufacturer,
                        const AdapterSetting supported_setting, const AdapterSetting current_setting,
                        const uint32_t dev_class, const std::string & name, const std::string & short_name)
            : dev_id(dev_id), address(address), version(version),
              manufacturer(manufacturer), supported_setting(supported_setting),
              current_setting(current_setting), dev_class(dev_class),
              name(name), short_name(short_name)
            { }

            bool isSettingSupported(const AdapterSetting setting) const {
                return setting == ( setting & supported_setting );
            }
            AdapterSetting getCurrentSetting() const { return current_setting; }
            uint32_t getDevClass() const { return dev_class; }
            std::string getName() const { return name; }
            std::string getShortName() const { return short_name; }

            std::string toString() const {
                return "Adapter[id "+std::to_string(dev_id)+", address "+address.toString()+", version "+std::to_string(version)+
                        ", manuf "+std::to_string(manufacturer)+
                        ", settings[sup "+getAdapterSettingsString(supported_setting)+", cur "+getAdapterSettingsString(current_setting)+
                        "], name '"+name+"', shortName '"+short_name+"']";
            }
    };

} // namespace direct_bt

#endif /* DBT_TYPES_HPP_ */
