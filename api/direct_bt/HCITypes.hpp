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
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>

#include "UUID.hpp"
#include "BTAddress.hpp"
#include "BTTypes.hpp"
#include "HCIComm.hpp"
#include "MgmtComm.hpp"

#define JAVA_MAIN_PACKAGE "org/tinyb"
#define JAVA_HCI_PACKAGE "tinyb/hci"

namespace direct_bt {

    // *************************************************
    // *************************************************
    // *************************************************

    class HCIAdapter; // forward
    class HCIDevice; // forward

    class HCISession
    {
        friend class HCIAdapter; // top manager: adapter open/close
        friend class HCIDevice;  // local device manager: device connect/disconnect

        private:
            static std::atomic_int name_counter;
            HCIAdapter &adapter;
            HCIComm hciComm;
            std::shared_ptr<HCIDevice> connectedDevice;

            HCISession(HCIAdapter &a, const uint16_t dev_id, const uint16_t channel, const int timeoutMS=HCI_TO_SEND_REQ_POLL_MS)
            : adapter(a), hciComm(dev_id, channel, timeoutMS),
              connectedDevice(nullptr), name(name_counter.fetch_add(1))
            {}

            void connected(std::shared_ptr<HCIDevice> device) {
                connectedDevice = device;
            }

        public:
            const int name;

            ~HCISession() { disconnect(); close(); }

            const HCIAdapter &getAdapter() { return adapter; }
            uint16_t getConnectedDeviceHandle() const { return hciComm.leConnHandle(); }
            std::shared_ptr<HCIDevice> getConnectedDevice() { return connectedDevice; }

            void disconnect(const uint8_t reason=0);

            bool close();

            bool isOpen() const { return hciComm.isOpen(); }
            int dd() const { return hciComm.dd(); }
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

    class HCIObject
    {
        protected:
            std::mutex lk;
            std::atomic_bool valid;

            HCIObject() : valid(true) {}

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

    class HCIDeviceDiscoveryListener {
        public:
            virtual void deviceAdded(HCIAdapter const &a, std::shared_ptr<HCIDevice> device) = 0;
            virtual void deviceUpdated(HCIAdapter const &a, std::shared_ptr<HCIDevice> device) = 0;
            virtual ~HCIDeviceDiscoveryListener() {}
    };

    class HCIDevice : public HCIObject
    {
        friend HCIAdapter; // managing us: ctor and update(..) during discovery

        private:
            static const int to_connect_ms = 5000;

            HCIAdapter const & adapter;
            uint64_t ts_update;
            std::string name;
            int8_t rssi = 0;
            int8_t tx_power = 0;
            std::shared_ptr<ManufactureSpecificData> msd = nullptr;
            std::vector<std::shared_ptr<uuid_t>> services;

            HCIDevice(HCIAdapter const & adapter, EInfoReport const & r);

            void addService(std::shared_ptr<uuid_t> const &uuid);
            void addServices(std::vector<std::shared_ptr<uuid_t>> const & services);

            void update(EInfoReport const & data);

        public:
            const uint64_t ts_creation;
            /** Device mac address */
            const EUI48 mac;

            ~HCIDevice();

            /** Returns the managing adapter */
            HCIAdapter const & getAdapter() const { return adapter; }

            /** Returns the shares reference of this instance, managed by the adapter */
            std::shared_ptr<HCIDevice> getSharedInstance() const;

            uint64_t getCreationTimestamp() const { return ts_creation; }
            uint64_t getUpdateTimestamp() const { return ts_update; }
            uint64_t getLastUpdateAge(const uint64_t ts_now) const { return ts_now - ts_update; }
            EUI48 const & getAddress() const { return mac; }
            std::string getAddressString() const { return mac.toString(); }
            std::string const & getName() const { return name; }
            bool hasName() const { return name.length()>0; }
            int8_t getRSSI() const { return rssi; }
            int8_t getTxPower() const { return tx_power; }
            std::shared_ptr<ManufactureSpecificData> const getManufactureSpecificData() const { return msd; }

            std::vector<std::shared_ptr<uuid_t>> getServices() const { return services; }

            /** Returns index >= 0 if found, otherwise -1 */
            int findService(std::shared_ptr<uuid_t> const &uuid) const;

            std::string toString() const;

            /**
             * Creates a new connection to this device.
             * <p>
             * Returns the new device connection handle if successful, otherwise 0 is returned.
             * </p>
             * <p>
             * The device connection handle as well as this device is stored and tracked by
             * the given HCISession instance.
             * </p>
             * <p>
             * Default parameter values are chosen for using public address resolution
             * and usual connection latency, interval etc.
             * </p>
             */
            uint16_t le_connect(HCISession& s,
                    const uint8_t peer_mac_type=HCIADDR_LE_PUBLIC, const uint8_t own_mac_type=HCIADDR_LE_PUBLIC,
                    const uint16_t interval=0x0004, const uint16_t window=0x0004,
                    const uint16_t min_interval=0x000F, const uint16_t max_interval=0x000F,
                    const uint16_t latency=0x0000, const uint16_t supervision_timeout=0x0C80,
                    const uint16_t min_ce_length=0x0001, const uint16_t max_ce_length=0x0001,
                    const uint8_t initiator_filter=0);
    };

    inline bool operator<(const HCIDevice& lhs, const HCIDevice& rhs)
    { return lhs.mac < rhs.mac; }

    inline bool operator==(const HCIDevice& lhs, const HCIDevice& rhs)
    { return lhs.mac == rhs.mac; }

    inline bool operator!=(const HCIDevice& lhs, const HCIDevice& rhs)
    { return !(lhs == rhs); }

    // *************************************************
    // *************************************************
    // *************************************************

    class HCIAdapter : public HCIObject
    {
        private:
            /** Returns index >= 0 if found, otherwise -1 */
            static int findDevice(std::vector<std::shared_ptr<HCIDevice>> const & devices, EUI48 const & mac);

            MgmtHandler& mgmt;
            std::shared_ptr<const AdapterInfo> adapterInfo;

            std::shared_ptr<HCISession> session;
            std::vector<std::shared_ptr<HCIDevice>> scannedDevices; // all devices scanned
            std::vector<std::shared_ptr<HCIDevice>> discoveredDevices; // matching all requirements for export
            std::shared_ptr<HCIDeviceDiscoveryListener> deviceDiscoveryListener = nullptr;

            bool validateDevInfo();

            friend bool HCISession::close();
            void sessionClosed(HCISession& s);

            friend std::shared_ptr<HCIDevice> HCIDevice::getSharedInstance() const;
            int findScannedDeviceIdx(EUI48 const & mac) const;
            std::shared_ptr<HCIDevice> findScannedDevice (EUI48 const & mac) const;
            bool addScannedDevice(std::shared_ptr<HCIDevice> const &device);

            bool addDiscoveredDevice(std::shared_ptr<HCIDevice> const &device);

        protected:

        public:
            const int dev_id;

            /**
             * Using the default adapter device
             */
            HCIAdapter();

            /**
             * @param[in] mac address
             */
            HCIAdapter(EUI48 &mac);

            /**
             * @param[in] dev_id an already identified HCI device id
             */
            HCIAdapter(const int dev_id);

            ~HCIAdapter();

            bool hasDevId() const { return 0 <= dev_id; }

            EUI48 const & getAddress() const { return adapterInfo->mac; }
            std::string getAddressString() const { return adapterInfo->mac.toString(); }
            std::string const & getName() const { return adapterInfo->name; }

            /**
             * Returns a reference to the newly opened session
             * if successful, otherwise nullptr is returned.
             */
            std::shared_ptr<HCISession> open();

            // device discovery aka device scanning

            /**
             * Replaces the HCIDeviceDiscoveryListener with the given instance, returning the replaced one.
             */
            std::shared_ptr<HCIDeviceDiscoveryListener> setDeviceDiscoveryListener(std::shared_ptr<HCIDeviceDiscoveryListener> l);

            /**
             * Starts a new discovery session.
             * <p>
             * Returns true if successful, otherwise false;
             * </p>
             * <p>
             * Default parameter values are chosen for using public address resolution
             * and usual discovery intervals etc.
             * </p>
             */
            bool startDiscovery(HCISession& s, uint8_t own_mac_type=HCIADDR_LE_PUBLIC,
                                uint16_t interval=0x0004, uint16_t window=0x0004);

            /**
             * Closes the discovery session.
             * @return true if no error, otherwise false.
             */
            void stopDiscovery(HCISession& s);

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
             * and guarantedd by the AD protocol.
             * </p>
             *
             * @return number of successfully scanned devices matching above criteria
             *         or -1 if an error has occurred.
             */
            int discoverDevices(HCISession& s,
                                const int waitForDeviceCount=1,
                                const EUI48 &waitForDevice=EUI48_ANY_DEVICE,
                                const int timeoutMS=HCI_TO_SEND_REQ_POLL_MS,
                                const uint32_t ad_type_req=static_cast<uint32_t>(EInfoReport::Element::NAME));

            /** Returns discovered devices from a discovery */
            std::vector<std::shared_ptr<HCIDevice>> getDiscoveredDevices() { return discoveredDevices; }

            /** Discards all discovered devices. */
            void removeDiscoveredDevices();

            /** Returns index >= 0 if found, otherwise -1 */
            int findDiscoveredDeviceIdx(EUI48 const & mac) const;

            /** Returns shared HCIDevice if found, otherwise nullptr */
            std::shared_ptr<HCIDevice> findDiscoveredDevice (EUI48 const & mac) const;

            std::shared_ptr<HCIDevice> getDiscoveredDevice(int index) const { return discoveredDevices.at(index); }

            std::string toString() const;
    };

} // namespace direct_bt

#endif /* HCITYPES_HPP_ */
