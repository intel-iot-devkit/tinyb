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
     * A HCI session, using an underlying {@link HCIComm} instance,
     * tracking all open connections.
     * <p>
     * At destruction, all remaining connections will be closed.
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
            std::vector<std::shared_ptr<DBTDevice>> connectedDevices;
            std::recursive_mutex mtx_connectedDevices;

            /** Opens a new HCI session on the given BT dev_id and HCI channel. */
            HCISession(DBTAdapter &a, const uint16_t channel, const int timeoutMS=HCI_TO_SEND_REQ_POLL_MS);

            /**
             * Add the new {@link DBTDevice} to the list of connected devices.
             * <p>
             * Throws an InternalError if the given device was already added to the list of connected devices.
             * Comparison is done by DBTDevice equality of address.
             * This ensures runtime consistency regarding overall DBTDevice reference tracking.
             * </p>
             */
            void connected(const std::shared_ptr<DBTDevice> & device);

            /**
             * Remove the {@link DBTDevice} from the list of connected devices.
             */
            void disconnected(const DBTDevice & device);

            /**
             * Issues {@link #disconnectAllDevices()} and closes the underlying HCI session.
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

            /** Return connected {@link DBTDevice}s. */
            std::vector<std::shared_ptr<DBTDevice>> getConnectedDevices() { return connectedDevices; }

            /** Disconnect all connected devices. Returns number of removed discovered devices. */
            int disconnectAllDevices(const uint8_t reason=0);

            /** Returns connected DBTDevice if found, otherwise nullptr */
            std::shared_ptr<DBTDevice> findConnectedDevice (EUI48 const & mac) const;

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

            virtual void adapterSettingsChanged(DBTAdapter const &a, const AdapterSetting oldmask, const AdapterSetting newmask,
                                                const AdapterSetting changedmask, const uint64_t timestamp) = 0;

            virtual void deviceFound(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) = 0;

            virtual void deviceUpdated(std::shared_ptr<DBTDevice> device, const uint64_t timestamp, const EIRDataType updateMask) = 0;

            virtual void deviceConnected(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) = 0;

            virtual void deviceDisconnected(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) = 0;

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
            volatile bool keepDiscoveringAlive = false;

            std::shared_ptr<HCISession> session;
            std::vector<std::shared_ptr<DBTDevice>> discoveredDevices; // all discovered devices
            std::vector<std::shared_ptr<DBTDevice>> sharedDevices; // all active shared devices
            std::vector<std::shared_ptr<AdapterStatusListener>> statusListenerList;
            std::recursive_mutex mtx_discoveredDevices;
            std::recursive_mutex mtx_sharedDevices;
            std::recursive_mutex mtx_statusListenerList;

            bool validateDevInfo();

            friend bool HCISession::close();
            void sessionClosing();

            friend std::shared_ptr<DBTDevice> DBTDevice::getSharedInstance() const;
            friend void DBTDevice::releaseSharedInstance() const;
            friend std::shared_ptr<ConnectionInfo> DBTDevice::getConnectionInfo();

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
             * Returns a reference to the newly opened HCI session
             * if successful, otherwise nullptr is returned.
             */
            std::shared_ptr<HCISession> open();

            /**
             * Returns the {@link #open()} session or {@code nullptr} if closed.
             */
            std::shared_ptr<HCISession> getOpenSession() const { return session; }

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
            bool startDiscovery(HCIAddressType own_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
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
