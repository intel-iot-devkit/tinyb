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

#ifndef HCITYPES_HPP_
#define HCITYPES_HPP_

#pragma once
#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>

#include "UUID.hpp"
#include "DataTypes.hpp"

namespace tinyb_hci {

enum HCI_Event_Types : uint8_t {
    LE_Advertising_Report       = 0x3E
};

enum LE_Address_T : uint8_t {
    LE_PUBLIC = 0x00,
    LE_RANDOM = 0x01
};

/**
// *************************************************
// *************************************************
// *************************************************
 */

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

class HCISession; // forward
class HCIDevice; // forward
class HCIDeviceDiscoveryListener; // forward

class HCIAdapter : public HCIObject
{
friend class HCISession;

private:
    static int getDefaultDevId();
    static int getDevId(EUI48 &mac);
    static int getDevId(const std::string &hcidev);

    static const int to_send_req_poll_ms = 1000;

    /** Returns index >= 0 if found, otherwise -1 */
    static int findDevice(std::vector<std::shared_ptr<HCIDevice>> const & devices, EUI48 const & mac);

    EUI48 mac;
    std::string name;

    std::vector<std::shared_ptr<HCISession>> sessions;
    std::vector<std::shared_ptr<HCIDevice>> scannedDevices; // all devices scanned
    std::vector<std::shared_ptr<HCIDevice>> discoveredDevices; // matching all requirements for export
    std::shared_ptr<HCIDeviceDiscoveryListener> deviceDiscoveryListener = nullptr;

    bool validateDevInfo();
    void sessionClosed(HCISession& s);

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
     * @param[in] hcidev shall be 'hci[0-9]'
     */
    HCIAdapter(const std::string &hcidev);

    /** 
     * @param[in] dev_id an already identified HCI device id
     */
    HCIAdapter(const int dev_id);

    ~HCIAdapter();

    bool hasDevId() const { return 0 <= dev_id; }

    EUI48 const & getAddress() const { return mac; }
    std::string getAddressString() const { return mac.toString(); }
    std::string const & getName() const { return name; }

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
    bool startDiscovery(HCISession& s,
                        uint16_t interval=0x0004, uint16_t window=0x0004,
                        uint8_t own_mac_type=LE_Address_T::LE_PUBLIC);

    /**
     * Closes the discovery session.
     * @return true if no error, otherwise false.
     */
    bool stopDiscovery(HCISession& s);

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
                        const int timeoutMS=to_send_req_poll_ms,
                        const uint32_t ad_type_req=static_cast<uint32_t>(EInfoReport::Element::NAME));

    /** Returns discovered devices from a discovery */
    std::vector<std::shared_ptr<HCIDevice>> getDiscoveredDevices() { return discoveredDevices; }

    /** Discards all discovered devices. */
    void removeDiscoveredDevices();

    /** Returns index >= 0 if found, otherwise -1 */
    int findDiscoveredDevice(EUI48 const & mac) const;

    std::shared_ptr<HCIDevice> getDiscoveredDevice(int index) const { return discoveredDevices.at(index); }

    std::string toString() const;
};

// *************************************************
// *************************************************
// *************************************************

class HCISession
{
friend class HCIAdapter;

private:
    static std::atomic_int name_counter;
    HCIAdapter &adapter;

    /** HCI device handle, open, close etc. dd < 0 is uninitialized  */
    int _dd;

    HCISession(HCIAdapter &a, int dd) 
    : adapter(a), _dd(dd), name(name_counter.fetch_add(1))
    {}

public:
    const int name;

    ~HCISession() { close(); }

    const HCIAdapter &getAdapter() { return adapter; }

    bool close();
    bool isOpen() const { return 0 <= _dd; }
    int dd() const { return _dd; }
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

class HCIDeviceDiscoveryListener {
public:
    virtual void deviceAdded(HCIAdapter const &a, std::shared_ptr<HCIDevice> device) = 0;
    virtual void deviceUpdated(HCIAdapter const &a, std::shared_ptr<HCIDevice> device) = 0;
    virtual ~HCIDeviceDiscoveryListener() {}
};

class HCIDevice : public HCIObject
{
friend class HCIAdapter;
private:
    static const int to_connect_ms = 5000;

    HCIAdapter const & adapter;
    uint64_t ts_update;
    std::string name;
    int8_t rssi = 0;
    int8_t tx_power = 0;
    std::shared_ptr<ManufactureSpecificData> msd = nullptr;
    std::vector<std::shared_ptr<UUID>> services;

    HCIDevice(HCIAdapter const & adapter, EInfoReport const & r);

    void addService(std::shared_ptr<UUID> const &uuid);
    void addServices(std::vector<std::shared_ptr<UUID>> const & services);

    void update(EInfoReport const & data);

public:
    const uint64_t ts_creation;
    /** Device mac address */
    const EUI48 mac;

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

    std::vector<std::shared_ptr<UUID>> getServices() const { return services; }

    /** Returns index >= 0 if found, otherwise -1 */
    int findService(std::shared_ptr<UUID> const &uuid) const;

    std::string toString() const;

    /**
     * Creates a new connection to this device.
     * <p>
     * Returns the new connection handle if successful, otherwise 0 is returned.
     * </p>
     * <p>
     * Default parameter values are chosen for using public address resolution
     * and usual connection latency, interval etc.
     * </p>
     */
    uint16_t le_connect(HCISession& s,
            uint16_t interval=0x0004, uint16_t window=0x0004,
            uint16_t min_interval=0x000F, uint16_t max_interval=0x000F,
            uint16_t latency=0x0000, uint16_t supervision_timeout=0x0C80,
            uint16_t min_ce_length=0x0001, uint16_t max_ce_length=0x0001,
            uint8_t initiator_filter=0,
            uint8_t peer_mac_type=LE_Address_T::LE_PUBLIC,
            uint8_t own_mac_type=LE_Address_T::LE_PUBLIC );
};

inline bool operator<(const HCIDevice& lhs, const HCIDevice& rhs)
{ return lhs.mac < rhs.mac; }

inline bool operator==(const HCIDevice& lhs, const HCIDevice& rhs)
{ return lhs.mac == rhs.mac; }

inline bool operator!=(const HCIDevice& lhs, const HCIDevice& rhs)
{ return !(lhs == rhs); }

} // namespace tinyb_hci

#endif /* HCITYPES_HPP_ */
