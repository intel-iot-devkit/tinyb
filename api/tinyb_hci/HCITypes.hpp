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

    EUI48 mac;
    std::string name;

    std::vector<std::shared_ptr<HCISession>> sessions;
    std::vector<std::shared_ptr<HCIDevice>> discoveredDevices;
    std::shared_ptr<HCIDeviceDiscoveryListener> deviceDiscoveryListener = nullptr;

    bool validateDevInfo();
    void sessionClosed(HCISession& s);

    void addDevice(std::shared_ptr<HCIDevice> const &device);

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
     * Returns a reference to the newly opened discovery session
     * if successful, otherwise nullptr is returned.
     */
    std::shared_ptr<HCISession> startDiscovery();

    /**
     * Closes the discovery session.
     * @return true if no error, otherwise false.
     */
    bool stopDiscovery(HCISession& s);

    /**
     * Discovery devices up until timeoutMS in milliseconds.
     * @return true if no error, otherwise false.
     */
    bool discoverDevices(HCISession& s, int timeoutMS);

    /** Returns discovered devices from a discovery */
    std::vector<std::shared_ptr<HCIDevice>> getDevices() { return discoveredDevices; }

    /** Discards all discovered devices. */
    void removeDevices() { discoveredDevices.clear(); }

    /** Returns index >= 0 if found, otherwise -1 */
    int findDevice(EUI48 const & mac) const;

    std::shared_ptr<HCIDevice> getDevice(int index) const { return discoveredDevices.at(index); }

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
    uint64_t ts_update;
    std::string name;
    uint8_t rssi = 0;
    int8_t tx_power = 0;
    std::shared_ptr<ManufactureSpecificData> msd = nullptr;
    std::vector<std::shared_ptr<UUID>> services;

    void addService(std::shared_ptr<UUID> const &uuid);
    void addServices(std::vector<std::shared_ptr<UUID>> const & services);

    void update(EInfoReport const & data);

public:
    const uint64_t ts_creation;
    /** Device mac address */
    const EUI48 mac;

    HCIDevice(EInfoReport &r);
    uint64_t getCreationTimestamp() const { return ts_creation; }
    uint64_t getUpdateTimestamp() const { return ts_update; }
    EUI48 const & getAddress() const { return mac; }
    std::string getAddressString() const { return mac.toString(); }
    std::string const & getName() const { return name; }
    uint8_t getRSSI() const { return rssi; }
    uint8_t getTxPower() const { return tx_power; }
    std::shared_ptr<ManufactureSpecificData> const getManufactureSpecificData() const { return msd; }

    std::vector<std::shared_ptr<UUID>> getServices() const { return services; }

    /** Returns index >= 0 if found, otherwise -1 */
    int findService(std::shared_ptr<UUID> const &uuid) const;

    std::string toString() const;
};

inline bool operator<(const HCIDevice& lhs, const HCIDevice& rhs)
{ return lhs.mac < rhs.mac; }

inline bool operator==(const HCIDevice& lhs, const HCIDevice& rhs)
{ return lhs.mac == rhs.mac; }

inline bool operator!=(const HCIDevice& lhs, const HCIDevice& rhs)
{ return !(lhs == rhs); }

} // namespace tinyb_hci

#endif /* HCITYPES_HPP_ */
