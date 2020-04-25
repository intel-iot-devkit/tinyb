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

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>

#include "UUID.hpp"
#include "BTAddress.hpp"
#include "BTTypes.hpp"
#include "HCIComm.hpp"
#include "DBTManager.hpp"
#include "JavaUplink.hpp"

#define JAVA_MAIN_PACKAGE "org/tinyb"
#define JAVA_HCI_PACKAGE "tinyb/hci"

namespace direct_bt {

    // *************************************************
    // *************************************************
    // *************************************************

    class DBTAdapter; // forward
    class DBTDevice; // forward

    /**
     * A HCI session, using an underlying {@link HCIComm} instance,
     * tracking all open LE connections.
     * <p>
     * At destruction, all remaining LE connections will be closed.
     * </p>
     */
    class HCISession
    {
        friend class DBTAdapter; // top manager: adapter open/close
        friend class DBTDevice;  // local device manager: device connect/disconnect

        private:
            static std::atomic_int name_counter;
            DBTAdapter * adapter;
            HCIComm hciComm;
            std::vector<std::shared_ptr<DBTDevice>> connectedLEDevices;

            /** Opens a new HCI session on the given BT dev_id and HCI channel. */
            HCISession(DBTAdapter &a, const uint16_t dev_id, const uint16_t channel, const int timeoutMS=HCI_TO_SEND_REQ_POLL_MS)
            : adapter(&a), hciComm(dev_id, channel, timeoutMS),
              name(name_counter.fetch_add(1))
            {}

            /** add the new {@link DBTDevice} to the list of connected LE devices */
            void connectedLE(std::shared_ptr<DBTDevice> & device);

            /** remove the {@link DBTDevice} from the list of connected LE devices */
            void disconnectedLE(std::shared_ptr<DBTDevice> & device);

            /**
             * Issues {@link #disconnectAllLEDevices()} and closes the underlying HCI session.
             * <p>
             * This shutdown hook is solely intended for adapter's destructor.
             * </p>
             */
            void shutdown();

        public:
            const int name;

            /**
             * Releases this instance after {@link #close()}.
             */
            ~HCISession();

            /** Return connected LE {@link DBTDevice}s. */
            std::vector<std::shared_ptr<DBTDevice>> getConnectedLEDevices() { return connectedLEDevices; }

            /** Disconnect all connected LE devices. Returns number of removed discovered devices. */
            int disconnectAllLEDevices(const uint8_t reason=0);

            /** Returns connected LE DBTDevice if found, otherwise nullptr */
            std::shared_ptr<DBTDevice> findConnectedLEDevice (EUI48 const & mac) const;

            /**
             * Closes this instance by {@link #disconnectAllLEDevices()}
             * and closing the underlying HCI session.
             */
            bool close();

            bool isOpen() const { return hciComm.isOpen(); }

            /** Return this HCI device descriptor, for multithreading access use {@link #dd()}. */
            int dd() const { return hciComm.dd(); }
            /** Return the recursive mutex for multithreading access of {@link #mutex()}. */
            std::recursive_mutex & mutex() { return hciComm.mutex(); }

            std::string toString() const;
    };

    inline bool operator<(const HCISession& lhs, const HCISession& rhs)
    { return lhs.name < rhs.name; }

    inline bool operator==(const HCISession& lhs, const HCISession& rhs)
    { return lhs.name == rhs.name; }

    inline bool operator!=(const HCISession& lhs, const HCISession& rhs)
    { return !(lhs == rhs); }


    // *************************************************
    // *************************************************
    // *************************************************

    class DBTObject : public JavaUplink
    {
        protected:
            std::mutex lk;
            std::atomic_bool valid;

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
            bool isValid() { return valid; }
    };

    // *************************************************
    // *************************************************
    // *************************************************

    class DBTDeviceDiscoveryListener {
        public:
            virtual void deviceAdded(DBTAdapter const &a, std::shared_ptr<DBTDevice> device) = 0;
            virtual void deviceUpdated(DBTAdapter const &a, std::shared_ptr<DBTDevice> device) = 0;
            virtual void deviceRemoved(DBTAdapter const &a, std::shared_ptr<DBTDevice> device) = 0;
            virtual ~DBTDeviceDiscoveryListener() {}
    };

    class DBTDevice : public DBTObject
    {
        friend DBTAdapter; // managing us: ctor and update(..) during discovery

        private:
            static const int to_connect_ms = 5000;

            DBTAdapter const & adapter;
            uint64_t ts_update;
            std::string name;
            int8_t rssi = 0;
            int8_t tx_power = 0;
            uint16_t leConnHandle = 0;
            std::shared_ptr<ManufactureSpecificData> msd = nullptr;
            std::vector<std::shared_ptr<uuid_t>> services;

            DBTDevice(DBTAdapter const & adapter, EInfoReport const & r);

            void addService(std::shared_ptr<uuid_t> const &uuid);
            void addServices(std::vector<std::shared_ptr<uuid_t>> const & services);

            void update(EInfoReport const & data);

        public:
            const uint64_t ts_creation;
            /** Device mac address */
            const EUI48 address;
            const BDAddressType addressType;

            /**
             * Releases this instance after {@link #le_disconnect()}.
             */
            ~DBTDevice();

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTDevice");
            }

            /** Returns the managing adapter */
            DBTAdapter const & getAdapter() const { return adapter; }

            /** Returns the shares reference of this instance, managed by the adapter */
            std::shared_ptr<DBTDevice> getSharedInstance() const;

            uint64_t getCreationTimestamp() const { return ts_creation; }
            uint64_t getUpdateTimestamp() const { return ts_update; }
            uint64_t getLastUpdateAge(const uint64_t ts_now) const { return ts_now - ts_update; }
            EUI48 const & getAddress() const { return address; }
            std::string getAddressString() const { return address.toString(); }
            BDAddressType getAddressType() const { return addressType; }
            std::string const & getName() const { return name; }
            bool hasName() const { return name.length()>0; }
            int8_t getRSSI() const { return rssi; }
            int8_t getTxPower() const { return tx_power; }
            std::shared_ptr<ManufactureSpecificData> const getManufactureSpecificData() const { return msd; }

            std::vector<std::shared_ptr<uuid_t>> getServices() const { return services; }

            /** Returns index >= 0 if found, otherwise -1 */
            int findService(std::shared_ptr<uuid_t> const &uuid) const;

            std::string toString() const override;

            /**
             * Creates a new connection to this device.
             * <p>
             * Returns the new device connection handle if successful, otherwise 0 is returned.
             * </p>
             * <p>
             * The device is tracked by the managing adapter's HCISession instance.
             * </p>
             * <p>
             * Default parameter values are chosen for using public address resolution
             * and usual connection latency, interval etc.
             * </p>
             */
            uint16_t le_connect(const HCIAddressType peer_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                const HCIAddressType own_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                const uint16_t interval=0x0004, const uint16_t window=0x0004,
                                const uint16_t min_interval=0x000F, const uint16_t max_interval=0x000F,
                                const uint16_t latency=0x0000, const uint16_t supervision_timeout=0x0C80,
                                const uint16_t min_ce_length=0x0001, const uint16_t max_ce_length=0x0001,
                                const uint8_t initiator_filter=0);

            /** Return the LE connection handle as returned by {@link #le_connect(..)}, 0 if not connectedLE. */
            uint16_t getLEConnectionHandle() const { return leConnHandle; }

            /**
             * Disconnect the device's LE connection.
             * <p>
             * The device is removed from the managing adapter's HCISession instance.
             * </p>
             */
            void le_disconnect(const uint8_t reason=0);
    };

    inline bool operator<(const DBTDevice& lhs, const DBTDevice& rhs)
    { return lhs.address < rhs.address; }

    inline bool operator==(const DBTDevice& lhs, const DBTDevice& rhs)
    { return lhs.address == rhs.address; }

    inline bool operator!=(const DBTDevice& lhs, const DBTDevice& rhs)
    { return !(lhs == rhs); }

    // *************************************************
    // *************************************************
    // *************************************************

    class DBTAdapter : public DBTObject
    {
        private:
            /** Returns index >= 0 if found, otherwise -1 */
            static int findDevice(std::vector<std::shared_ptr<DBTDevice>> const & devices, EUI48 const & mac);

            DBTManager& mgmt;
            std::shared_ptr<const AdapterInfo> adapterInfo;
            ScanType currentScanType = ScanType::SCAN_TYPE_NONE;
            volatile bool keepDiscoveringAlive = false;

            std::shared_ptr<HCISession> session;
            std::vector<std::shared_ptr<DBTDevice>> scannedDevices; // all devices scanned
            std::vector<std::shared_ptr<DBTDevice>> discoveredDevices; // matching all requirements for export
            std::shared_ptr<DBTDeviceDiscoveryListener> deviceDiscoveryListener = nullptr;

            bool validateDevInfo();

            friend bool HCISession::close();
            void sessionClosing();

            friend std::shared_ptr<DBTDevice> DBTDevice::getSharedInstance() const;
            int findScannedDeviceIdx(EUI48 const & mac) const;
            std::shared_ptr<DBTDevice> findScannedDevice (EUI48 const & mac) const;
            bool addScannedDevice(std::shared_ptr<DBTDevice> const &device);

            bool addDiscoveredDevice(std::shared_ptr<DBTDevice> const &device);

            std::mutex mtxEventRead;
            std::condition_variable cvEventRead;
            std::shared_ptr<MgmtEvent> eventReceived = nullptr;
            bool mgmtEvDeviceDiscoveringCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceFoundCB(std::shared_ptr<MgmtEvent> e);

            int discoverDevicesHCI(const int waitForDeviceCount, const EUI48 &waitForDevice,
                                   const int timeoutMS, const uint32_t ad_type_req);
            int discoverDevicesMgmt(const int waitForDeviceCount, const EUI48 &waitForDevice,
                                    const int timeoutMS, const uint32_t ad_type_req);
        public:
            const int dev_id;

            /**
             * Using the default adapter device
             */
            DBTAdapter();

            /**
             * @param[in] mac address
             */
            DBTAdapter(EUI48 &mac);

            /**
             * @param[in] dev_id an already identified HCI device id
             */
            DBTAdapter(const int dev_id);

            /**
             * Releases this instance after HCISession shutdown().
             */
            ~DBTAdapter();

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTAdapter");
            }

            bool hasDevId() const { return 0 <= dev_id; }

            EUI48 const & getAddress() const { return adapterInfo->mac; }
            std::string getAddressString() const { return adapterInfo->mac.toString(); }
            std::string const & getName() const { return adapterInfo->name; }

            /**
             * Returns a reference to the used singleton DBTManager instance.
             */
            DBTManager& getManager() const { return mgmt; }

            /**
             * Returns a reference to the newly opened session
             * if successful, otherwise nullptr is returned.
             */
            std::shared_ptr<HCISession> open();

            /**
             * Returns the {@link #open()} session or {@code nullptr} if closed.
             */
            std::shared_ptr<HCISession> getOpenSession() const { return session; }

            // device discovery aka device scanning

            /**
             * Replaces the DBTDeviceDiscoveryListener with the given instance, returning the replaced one.
             */
            std::shared_ptr<DBTDeviceDiscoveryListener> setDeviceDiscoveryListener(std::shared_ptr<DBTDeviceDiscoveryListener> l);

            /**
             * Starts a new discovery session.
             * <p>
             * Returns true if successful, otherwise false;
             * </p>
             * <p>
             * Default parameter values are chosen for using public address resolution
             * and usual discovery intervals etc.
             * </p>
             * <p>
             * This adapter's DBTManager instance is used, i.e. the management channel.
             * </p>
             */
            bool startDiscovery(uint8_t own_mac_type=HCIADDR_LE_PUBLIC,
                                uint16_t interval=0x0004, uint16_t window=0x0004);

            /**
             * Closes the discovery session.
             * <p>
             * This adapter's DBTManager instance is used, i.e. the management channel.
             * </p>
             * @return true if no error, otherwise false.
             */
            void stopDiscovery();

            /**
             * Discovery devices up until 'timeoutMS' in milliseconds
             * or until 'waitForDeviceCount' and 'waitForDevice'
             * devices matching 'ad_type_req' criteria has been
             * reached.
             *
             * <p>
             * 'waitForDeviceCount' is the number of successfully
             * scanned devices matching 'ad_type_req'
             * before returning if 'timeoutMS' hasn't been reached.
             * <br>
             * Default value is '1', i.e. wait for only one device.
             * A value of <= 0 denotes infinitive, here 'timeoutMS'
             * will end the discovery process.
             * </p>
             *
             * <p>
             * 'waitForDevice' is a EUI48 denoting a specific device
             * to wait for.
             * <br>
             * Default value is 'EUI48_ANY_DEVICE', i.e. wait for any device.
             * </p>
             *
             * <p>
             * 'ad_type_req' is a bitmask of 'EInfoReport::Element'
             * denoting required data to be received before
             * adding or updating the devices in the discovered list.
             * <br>
             * Default value is: 'EInfoReport::Element::NAME',
             * while 'EInfoReport::Element::BDADDR|EInfoReport::Element::RSSI' is implicit
             * and guaranteed by the AD protocol.
             * </p>
             * <p>
             * This adapter's DBTManager instance is used, i.e. the management channel.
             * </p>
             *
             * @return number of successfully scanned devices matching above criteria
             *         or -1 if an error has occurred.
             */
            int discoverDevices(const int waitForDeviceCount=1,
                                const EUI48 &waitForDevice=EUI48_ANY_DEVICE,
                                const int timeoutMS=HCI_TO_SEND_REQ_POLL_MS,
                                const uint32_t ad_type_req=static_cast<uint32_t>(EInfoReport::Element::NAME));

            /** Returns discovered devices from a discovery */
            std::vector<std::shared_ptr<DBTDevice>> getDiscoveredDevices() { return discoveredDevices; }

            /** Discards all discovered devices. Returns number of removed discovered devices. */
            int removeDiscoveredDevices();

            /** Returns index >= 0 if found, otherwise -1 */
            int findDiscoveredDeviceIdx(EUI48 const & mac) const;

            /** Returns shared DBTDevice if found, otherwise nullptr */
            std::shared_ptr<DBTDevice> findDiscoveredDevice (EUI48 const & mac) const;

            std::shared_ptr<DBTDevice> getDiscoveredDevice(int index) const { return discoveredDevices.at(index); }

            std::string toString() const override;
    };

} // namespace direct_bt

#endif /* DBT_TYPES_HPP_ */
