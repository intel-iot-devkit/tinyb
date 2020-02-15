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
#include <memory>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <vector>
#include <functional>
#include <map>

#include "HCIUtil.hpp"
#include "UUID.hpp"

#define JAVA_MAIN_PACKAGE "org/tinyb"
#define JAVA_DBUS_PACKAGE "tinyb/hci"

extern "C" {
    #include <bluetooth/bluetooth.h>
    #include <bluetooth/hci.h>
    #include <bluetooth/hci_lib.h>
}

namespace tinyb_hci {

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

class HCIAdapter : public HCIObject
{
friend class HCISession;

private:
    static int getDefaultDevId();
    static int getDevId(bdaddr_t &bdaddr);
    static int getDevId(const std::string &hcidev);

    const int to_send_req_poll_ms = 1000;
    struct hci_dev_info dev_info;
    std::vector<std::shared_ptr<HCISession>> sessions;
    std::vector<std::shared_ptr<HCIDevice>> discoveredDevices;

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
    HCIAdapter(bdaddr_t &mac);

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

    std::string getAddress() const;
    std::string getName() const;

    /** 
     * Returns a reference to the newly opened session
     * if successful, otherwise nullptr is returned.
     */
    std::shared_ptr<HCISession> open();

    // device discovery aka device scanning 

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
    int findDevice(bdaddr_t const & mac) const;

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

class HCIDevice : public HCIObject
{
friend class HCIAdapter;
private:
	std::string name;
    uint8_t rssi;
    std::vector<std::shared_ptr<UUID>> services;

    void setLastRSSI(uint8_t newRSSI) { rssi=newRSSI; }

    void addService(std::shared_ptr<UUID> const &uuid);

    void setName(std::string &n) { name = n; }

public:
    /** Device mac address */
    const bdaddr_t mac;

    HCIDevice(const bdaddr_t &mac, const std::string &name, uint8_t rssi);
    std::string getAddress() const;
    std::string getName() const { return name; }
    uint8_t getLastRSSI() const { return rssi; }
    std::vector<std::shared_ptr<UUID>> getServices() const { return services; }

    /** Returns index >= 0 if found, otherwise -1 */
    int findService(std::shared_ptr<UUID> const &uuid) const;

    std::string toString() const;
};

inline bool operator<(const HCIDevice& lhs, const HCIDevice& rhs)
{ return bacmp(&lhs.mac, &rhs.mac)<0; }

inline bool operator==(const HCIDevice& lhs, const HCIDevice& rhs)
{ return !bacmp(&lhs.mac, &rhs.mac); }

inline bool operator!=(const HCIDevice& lhs, const HCIDevice& rhs)
{ return !(lhs == rhs); }

} // namespace tinyb_hci

#endif /* HCITYPES_HPP_ */
