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

#ifndef DBT_DEVICE_HPP_
#define DBT_DEVICE_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>

#include "DBTTypes.hpp"

#include "HCIIoctl.hpp"
#include "GATTHandler.hpp"

namespace direct_bt {

    // *************************************************
    // *************************************************
    // *************************************************

    class DBTAdapter; // forward

    class DBTDevice : public DBTObject
    {
        friend DBTAdapter; // managing us: ctor and update(..) during discovery

        private:
            static const int to_connect_ms = 5000;

            DBTAdapter & adapter;
            uint64_t ts_update;
            std::string name;
            int8_t rssi = 127; // The core spec defines 127 as the "not available" value
            int8_t tx_power = 127; // The core spec defines 127 as the "not available" value
            AppearanceCat appearance = AppearanceCat::UNKNOWN;
            uint16_t hciConnHandle = 0;
            std::shared_ptr<ManufactureSpecificData> msd = nullptr;
            std::vector<std::shared_ptr<uuid_t>> services;
            std::shared_ptr<GATTHandler> gattHandler = nullptr;
            std::shared_ptr<GenericAccess> gattGenericAccess = nullptr;
            std::recursive_mutex mtx_data;
            std::recursive_mutex mtx_gatt;

            DBTDevice(DBTAdapter & adapter, EInfoReport const & r);

            bool addService(std::shared_ptr<uuid_t> const &uuid);
            bool addServices(std::vector<std::shared_ptr<uuid_t>> const & services);

            EIRDataType update(EInfoReport const & data);
            EIRDataType update(GenericAccess const &data, const uint64_t timestamp);

            /** Returns the shared pointer of this instance managed by the adapter. */
            std::shared_ptr<DBTDevice> getSharedInstance() const;

            void releaseSharedInstance() const;
            void notifyDisconnected();
            void disconnect(const bool disconnectManager, const uint8_t reason=0x13 /* HCIErrorCode::REMOTE_USER_TERMINATED_CONNECTION */);

        public:
            const uint64_t ts_creation;
            /** Device mac address */
            const EUI48 address;
            const BDAddressType addressType;

            /**
             * Releases this instance after calling {@link #remove()}.
             */
            ~DBTDevice();

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTDevice");
            }

            /** Returns the managing adapter */
            DBTAdapter & getAdapter() const { return adapter; }

            /**
             * Returns the timestamp in monotonic milliseconds when this device instance has been created, either via discovery or direct connection.
             * @see BasicTypes::getCurrentMilliseconds()
             */
            uint64_t getCreationTimestamp() const { return ts_creation; }

            uint64_t getUpdateTimestamp() const { return ts_update; }
            uint64_t getLastUpdateAge(const uint64_t ts_now) const { return ts_now - ts_update; }

            EUI48 const & getAddress() const { return address; }
            std::string getAddressString() const { return address.toString(); }
            BDAddressType getAddressType() const { return addressType; }
            bool isLEAddressType() const { return BDADDR_LE_PUBLIC == addressType || BDADDR_LE_RANDOM == addressType; }
            bool isBREDRAddressType() const { return BDADDR_BREDR == addressType; }

            /** Return RSSI of device as recognized at discovery and connect. */
            int8_t getRSSI() const { return rssi; }

            /** Return Tx Power of device as recognized at discovery and connect. */
            int8_t getTxPower() const { return tx_power; }

            /** Return AppearanceCat of device as recognized at discovery, connect and GATT discovery. */
            AppearanceCat getAppearance() const { return appearance; }

            std::string const getName() const;

            /** Return shared ManufactureSpecificData as recognized at discovery, pre GATT discovery. */
            std::shared_ptr<ManufactureSpecificData> const getManufactureSpecificData() const;

            /** Return a list of services as recognized at discovery, pre GATT discovery. */
            std::vector<std::shared_ptr<uuid_t>> getServices() const;

            /** Returns index >= 0 if found, otherwise -1 */
            int findService(std::shared_ptr<uuid_t> const &uuid) const;

            std::string toString() const override { return toString(false); }

            std::string toString(bool includeDiscoveredServices) const;

            /**
             * Retrieves the current connection info for this device and returns the ConnectionInfo reference if successful,
             * otherwise returns nullptr.
             * <p>
             * Before this method returns, the internal rssi and tx_power will be updated if any changed
             * and therefore all DBTAdapterStatusListener's deviceUpdated(..) method called for notification.
             * </p>
             */
            std::shared_ptr<ConnectionInfo> getConnectionInfo();

            /**
             * Establish a HCI BDADDR_LE_PUBLIC or BDADDR_LE_RANDOM connection to this device.
             * <p>
             * If this device's addressType is not BDADDR_LE_PUBLIC or BDADDR_LE_RANDOM, 0 is being returned.
             * </p>
             * <p>
             * Returns the new connection handle or 0 if not successful.
             * </p>
             * <p>
             * The device is tracked by the managing adapter.
             * </p>
             * <p>
             * Default parameter values are chosen for using public address resolution
             * and usual connection latency, interval etc.
             * </p>
             * <p>
             * Set window to the same value as the interval, enables continuous scanning.
             * </p>
             */
            uint16_t connectLE(const HCIAddressType peer_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                               const HCIAddressType own_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                               const uint16_t interval=0x0004, const uint16_t window=0x0004,
                               const uint16_t min_interval=0x000F, const uint16_t max_interval=0x000F,
                               const uint16_t latency=0x0000, const uint16_t supervision_timeout=0x0C80);

            /**
             * Establish a HCI BDADDR_BREDR connection to this device.
             * <p>
             * If this device's addressType is not BDADDR_BREDR, 0 is being returned.
             * </p>
             * <p>
             * Returns the new connection handle or 0 if not successful.
             * </p>
             * <p>
             * The device is tracked by the managing adapter.
             * </p>
             */
            uint16_t connectBREDR(const uint16_t pkt_type=HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5,
                                  const uint16_t clock_offset=0x0000, const uint8_t role_switch=0x01);

            /**
             * Establish a default HCI connection to this device, using certain default parameter.
             * <p>
             * Depending on this device's addressType,
             * either a BREDR (BDADDR_BREDR) or LE (BDADDR_LE_PUBLIC, BDADDR_LE_RANDOM) connection is attempted.
             * </p>
             * <p>
             * Returns the new connection handle or 0 if not successful.
             * </p>
             * <p>
             * The device is tracked by the managing adapter.
             * </p>
             */
            uint16_t connectDefault();


            /** Return the HCI connection handle to the LE or BREDR peer, 0 if not connected. */
            uint16_t getConnectionHandle() const { return hciConnHandle; }

            /**
             * Disconnect the LE or BREDR peer's GATT and HCI connection.
             * <p>
             * The device will be removed from the managing adapter's connected devices.
             * </p>
             * <p>
             * An open GATTHandler will also be closed via disconnectGATT()
             * </p>
             */
            void disconnect(const uint8_t reason=0x13 /* HCIErrorCode::REMOTE_USER_TERMINATED_CONNECTION */) {
                disconnect(true /* disconnectManager */, reason);
            }

            /**
             * Disconnects this device via disconnect(..) and
             * removes its shared reference from the Adapter, not the discovered devices.
             * <p>
             * This method shall be issued to ensure no device reference will
             * be leaked in a long lived adapter, as only the discovered devices
             * are being flushed with a new discovery.
             * </p>
             * <p>
             * After calling this method, the device shall no more being used.
             * </p>
             */
            void remove();

            /**
             * Returns a newly established GATT connection or an already open GATT connection.
             * <p>
             * The HCI connectLE(..) or connectBREDR(..) shall be performed first,
             * to produce best performance. See {@link #connectDefault()}.
             * </p>
             * <p>
             * The returned GATTHandler is managed by this device instance
             * and closed @ disconnect() or explicitly @ disconnectGATT().
             * May return nullptr if not connected or failure.
             * </p>
             */
            std::shared_ptr<GATTHandler> connectGATT(int timeoutMS=GATTHandler::Defaults::L2CAP_READER_THREAD_POLL_TIMEOUT);

            /** Returns already opened GATTHandler, see connectGATT(..) and disconnectGATT(). */
            std::shared_ptr<GATTHandler> getGATTHandler();

            /**
             * Returns a list of shared GATTService available on this device if successful,
             * otherwise returns an empty list.
             * <p>
             * If this method has been called for the first time or no services has been detected yet,
             * a list of GATTService will be discovered.
             * <br>
             * In case no GATT connection has been established yet or disconnectGATT() has been called thereafter,
             * connectGATT(..) will be performed.
             * </p>
             */
            std::vector<std::shared_ptr<GATTService>> getGATTServices();

            /** Returns the shared GenericAccess instance, retrieved by {@link #getGATTServices()} or nullptr if not available. */
            std::shared_ptr<GenericAccess> getGATTGenericAccess();

            /**
             * Explicit disconnecting an open GATTHandler, which is usually performed via disconnect()
             * <p>
             * Implementation will also discard the GATTHandler reference.
             * </p>
             */
            void disconnectGATT();
    };

    inline bool operator<(const DBTDevice& lhs, const DBTDevice& rhs)
    { return lhs.address < rhs.address; }

    inline bool operator==(const DBTDevice& lhs, const DBTDevice& rhs)
    { return lhs.address == rhs.address; }

    inline bool operator!=(const DBTDevice& lhs, const DBTDevice& rhs)
    { return !(lhs == rhs); }

} // namespace direct_bt

#endif /* DBT_DEVICE_HPP_ */
