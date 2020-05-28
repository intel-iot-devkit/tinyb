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

#ifndef DBT_ADAPTER_HPP_
#define DBT_ADAPTER_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>

#include "DBTTypes.hpp"

#include "DBTDevice.hpp"

#include "HCIComm.hpp"
#include "DBTManager.hpp"

namespace direct_bt {

    class DBTAdapter; // forward

    /**
     * {@link DBTAdapter} status listener for {@link DBTDevice} discovery events: Added, updated and removed;
     * as well as for certain {@link DBTAdapter} events.
     * <p>
     * A listener instance may be attached to a {@link DBTAdapter} via
     * {@link DBTAdapter::addStatusListener(std::shared_ptr<AdapterStatusListener>)}.
     * </p>
     * <p>
     * The listener receiver maintains a unique set of listener instances without duplicates.
     * </p>
     */
    class AdapterStatusListener {
        public:
            /**
             * Custom filter for all 'device*' notification methods,
             * which will not be called if this method returns false.
             * <p>
             * User may override this method to test whether the 'device*' methods shall be called
             * for the given device.
             * </p>
             * <p>
             * Defaults to true;
             * </p>
             */
            virtual bool matchDevice(const DBTDevice & device) {
                (void)device;
                return true;
            }

            /**
             * DBTAdapter setting(s) changed.
             * @param a the adapter which settings have changed.
             * @param oldmask the previous settings mask
             * @param newmask the new settings mask
             * @param changedmask the changes settings mask
             * @param timestamp the time in monotonic milliseconds when this event occurred. See BasicTypes::getCurrentMilliseconds().
             */
            virtual void adapterSettingsChanged(DBTAdapter const &a, const AdapterSetting oldmask, const AdapterSetting newmask,
                                                const AdapterSetting changedmask, const uint64_t timestamp) = 0;

            /**
             * DBTAdapter's discovery state has changed, i.e. enabled or disabled.
             * @param a the adapter which discovering state has changed.
             * @param enabled the new discovery state
             * @param keepAlive if {@code true}, the discovery will be re-enabled if disabled by the underlying Bluetooth implementation.
             * @param timestamp the time in monotonic milliseconds when this event occurred. See BasicTypes::getCurrentMilliseconds().
             */
            virtual void discoveringChanged(DBTAdapter const &a, const bool enabled, const bool keepAlive, const uint64_t timestamp) = 0;

            /**
             * A DBTDevice has been newly discovered.
             * @param device the found device
             * @param timestamp the time in monotonic milliseconds when this event occurred. See BasicTypes::getCurrentMilliseconds().
             */
            virtual void deviceFound(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) = 0;

            /**
             * An already discovered DBTDevice has been updated.
             * @param device the updated device
             * @param timestamp the time in monotonic milliseconds when this event occurred. See BasicTypes::getCurrentMilliseconds().
             * @param updateMask the update mask of changed data
             */
            virtual void deviceUpdated(std::shared_ptr<DBTDevice> device, const uint64_t timestamp, const EIRDataType updateMask) = 0;

            /**
             * DBTDevice's connection status has changed.
             * @param device the device which connection state has changed
             * @param connected if {@code true} the device has been connected, otherwise disconnected
             * @param timestamp the time in monotonic milliseconds when this event occurred. See BasicTypes::getCurrentMilliseconds().
             */
            virtual void deviceConnectionChanged(std::shared_ptr<DBTDevice> device, const bool connected, const uint64_t timestamp) = 0;

            virtual ~AdapterStatusListener() {}

            /**
             * Default comparison operator, merely testing for same memory reference.
             * <p>
             * Specializations may override.
             * </p>
             */
            virtual bool operator==(const AdapterStatusListener& rhs) const
            { return this == &rhs; }

            bool operator!=(const AdapterStatusListener& rhs) const
            { return !(*this == rhs); }
    };

    // *************************************************
    // *************************************************
    // *************************************************

    class DBTAdapter : public DBTObject
    {
        private:
            /** Returns index >= 0 if found, otherwise -1 */
            static int findDevice(std::vector<std::shared_ptr<DBTDevice>> const & devices, EUI48 const & mac);

            DBTManager& mgmt;
            std::shared_ptr<AdapterInfo> adapterInfo;
            NameAndShortName localName;
            ScanType currentScanType = ScanType::SCAN_TYPE_NONE;

            std::shared_ptr<HCIComm> hciComm;
            std::vector<std::shared_ptr<DBTDevice>> connectedDevices;
            std::vector<std::shared_ptr<DBTDevice>> discoveredDevices; // all discovered devices
            std::vector<std::shared_ptr<DBTDevice>> sharedDevices; // all active shared devices
            std::vector<std::shared_ptr<AdapterStatusListener>> statusListenerList;
            std::recursive_mutex mtx_hci;
            std::recursive_mutex mtx_connectedDevices;
            std::recursive_mutex mtx_discoveredDevices;
            std::recursive_mutex mtx_sharedDevices;
            std::recursive_mutex mtx_statusListenerList;
            volatile bool keepDiscoveringAlive = false;

            bool validateDevInfo();

            friend std::shared_ptr<DBTDevice> DBTDevice::getSharedInstance() const;
            friend void DBTDevice::releaseSharedInstance() const;
            friend std::shared_ptr<ConnectionInfo> DBTDevice::getConnectionInfo();
            friend void DBTDevice::disconnect(const uint8_t reason);
            friend uint16_t DBTDevice::connectLE(HCIAddressType peer_mac_type, HCIAddressType own_mac_type,
                    uint16_t interval, uint16_t window,
                    uint16_t min_interval, uint16_t max_interval,
                    uint16_t latency, uint16_t supervision_timeout);
            friend uint16_t DBTDevice::connectBREDR(const uint16_t pkt_type, const uint16_t clock_offset, const uint8_t role_switch);
            friend std::vector<std::shared_ptr<GATTService>> DBTDevice::getGATTServices();

            void addConnectedDevice(const std::shared_ptr<DBTDevice> & device);
            void removeConnectedDevice(const DBTDevice & device);
            int disconnectAllDevices(const uint8_t reason=0);
            std::shared_ptr<DBTDevice> findConnectedDevice (EUI48 const & mac) const;

            bool addDiscoveredDevice(std::shared_ptr<DBTDevice> const &device);

            bool addSharedDevice(std::shared_ptr<DBTDevice> const &device);
            std::shared_ptr<DBTDevice> getSharedDevice(const DBTDevice & device);
            void releaseSharedDevice(const DBTDevice & device);
            std::shared_ptr<DBTDevice> findSharedDevice (EUI48 const & mac) const;

            bool mgmtEvDeviceDiscoveringCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvNewSettingsCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvLocalNameChangedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceFoundCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceConnectedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceDisconnectedCB(std::shared_ptr<MgmtEvent> e);

            void startDiscoveryBackground();

            void sendDeviceUpdated(std::shared_ptr<DBTDevice> device, uint64_t timestamp, EIRDataType updateMask);


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

            EUI48 const & getAddress() const { return adapterInfo->address; }
            std::string getAddressString() const { return adapterInfo->address.toString(); }

            /**
             * Returns the system name.
             */
            std::string getName() const { return adapterInfo->getName(); }

            /**
             * Returns the short system name.
             */
            std::string getShortName() const { return adapterInfo->getShortName(); }

            /**
             * Returns the local friendly name and short_name. Contains empty strings if not set.
             * <p>
             * The value is being updated via SET_LOCAL_NAME management event reply.
             * </p>
             */
            const NameAndShortName & getLocalName() const { return localName; }

            /**
             * Sets the local friendly name.
             * <p>
             * Returns the immediate SET_LOCAL_NAME reply if successful, otherwise nullptr.
             * The corresponding management event will be received separately.
             * </p>
             */
            std::shared_ptr<NameAndShortName> setLocalName(const std::string &name, const std::string &short_name);

            /**
             * Set the power state of the adapter.
             */
            void setPowered(bool value);

            /**
             * Set the discoverable state of the adapter.
             */
            void setDiscoverable(bool value);

            /**
             * Set the bondable (aka pairable) state of the adapter.
             */
            void setBondable(bool value);

            /**
             * Returns a reference to the used singleton DBTManager instance.
             */
            DBTManager& getManager() const { return mgmt; }

            /**
             * Returns a reference to the already opened HCIComm
             * or the newly opened HCIComm instance, otherwise nullptr if no success.
             */
            std::shared_ptr<HCIComm> openHCI();

            /**
             * Returns the {@link #openHCI()} HCIComm or {@code nullptr} if closed.
             */
            std::shared_ptr<HCIComm> getHCI() const;

            /**
             * Closes the HCIComm instance
             */
            bool closeHCI();

            /**
             * Returns true, if the adapter's device is already whitelisted.
             */
            bool isDeviceWhitelisted(const EUI48 &address);

            /**
             * Add the given device to the adapter's autoconnect whitelist.
             * <p>
             * The given connection parameter will be uploaded to the kernel for the given device first.
             * </p>
             * <p>
             * Method will reject duplicate devices, in which case it should be removed first.
             * </p>
             */
            bool addDeviceToWhitelist(const EUI48 &address, const BDAddressType address_type,
                                      const HCIWhitelistConnectType ctype,
                                      const uint16_t min_interval=0x000F, const uint16_t max_interval=0x000F,
                                      const uint16_t latency=0x0000, const uint16_t timeout=0x0C80);


            /** Remove the given device from the adapter's autoconnect whitelist. */
            bool removeDeviceFromWhitelist(const EUI48 &address, const BDAddressType address_type);

            // device discovery aka device scanning

            /**
             * Add the given listener to the list if not already present.
             * <p>
             * Returns true if the given listener is not element of the list and has been newly added,
             * otherwise false.
             * </p>
             */
            bool addStatusListener(std::shared_ptr<AdapterStatusListener> l);

            /**
             * Remove the given listener from the list.
             * <p>
             * Returns true if the given listener is an element of the list and has been removed,
             * otherwise false.
             * </p>
             */
            bool removeStatusListener(std::shared_ptr<AdapterStatusListener> l);

            /**
             * Remove the given listener from the list.
             * <p>
             * Returns true if the given listener is an element of the list and has been removed,
             * otherwise false.
             * </p>
             */
            bool removeStatusListener(const AdapterStatusListener * l);

            /**
             * Remove all status listener from the list.
             * <p>
             * Returns the number of removed event listener.
             * </p>
             */
            int removeAllStatusListener();

            /**
             * Starts a new discovery session.
             * <p>
             * Returns true if successful, otherwise false;
             * </p>
             * <p>
             * if {@code keepAlive} is {@code  true}, discovery state will be re-enabled
             * in case the underlying Bluetooth implementation (BlueZ, ..) disabled it.
             * Default is {@code false}.
             * </p>
             * <p>
             * Default parameter values are chosen for using public address resolution
             * and usual discovery intervals etc.
             * </p>
             * <p>
             * This adapter's DBTManager instance is used, i.e. the management channel.
             * </p>
             * <p>
             * Also clears previous discovered devices via removeDiscoveredDevices().
             * </p>
             */
            bool startDiscovery(bool keepAlive=false, HCIAddressType own_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
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
             * Returns discovered devices from the last discovery.
             * <p>
             * Note that this list will be cleared when a new discovery is started over via startDiscovery().
             * </p>
             * <p>
             * Note that devices in this list might be no more available,
             * use 'DeviceStatusListener::deviceFound(..)' callback.
             * </p>
             */
            std::vector<std::shared_ptr<DBTDevice>> getDiscoveredDevices() const;

            /** Discards all discovered devices. Returns number of removed discovered devices. */
            int removeDiscoveredDevices();

            /** Returns shared DBTDevice if found, otherwise nullptr */
            std::shared_ptr<DBTDevice> findDiscoveredDevice (EUI48 const & mac) const;

            std::string toString() const override;
    };

} // namespace direct_bt

#endif /* DBT_ADAPTER_HPP_ */
