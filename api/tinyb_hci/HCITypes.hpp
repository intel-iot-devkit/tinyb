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
class HCIDeviceDiscoveryListener; // forward

class HCIAdapter : public HCIObject
{
friend class HCISession;

private:
    static int getDefaultDevId();
    static int getDevId(bdaddr_t &bdaddr);
    static int getDevId(const std::string &hcidev);

    static const int to_send_req_poll_ms = 1000;

    struct hci_dev_info dev_info;
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

class ManufactureSpecificData
{
public:
	uint16_t const company;
	std::string const companyName;
	int const data_len;
	std::shared_ptr<uint8_t> const data;

	ManufactureSpecificData()
	: company(0), companyName(), data_len(0), data(nullptr) {}

	ManufactureSpecificData(uint16_t const company, uint8_t const * const data, int const data_len);

	std::string getCompanyString() const;
	std::string toString() const;
};

/**
 * Collection of device information, e.g. advertising data (AD)
 * or explicit 'extended inquiry response' (EIR).
 * <p>
 * AD as well as EIR information is passed in little endian order
 * in the same fashion data block:
 * <pre>
 * a -> {
 * 			uint8_t len
 * 			uint8_t type
 * 			uint8_t data[len-1];
 * 	    }
 * b -> next block = a + 1 + len;
 * </pre>
 * </p>
 */
class EInfoReport
{
friend class HCIDevice;

public:
	enum class Element : uint32_t {
		EVT_TYPE    = (1 << 0),
		BDADDR_TYPE = (1 << 1),
		BDADDR      = (1 << 2),
		NAME        = (1 << 3),
		NAME_SHORT  = (1 << 4),
		RSSI        = (1 << 5),
		TX_POWER    = (1 << 6),
		MANUF_DATA  = (1 << 7)
	};

private:
	uint64_t timestamp = 0;
	uint32_t data_set = 0;

	uint8_t evt_type = 0;
	uint8_t bdaddr_type = 0;
	bdaddr_t bdaddr;

	std::string name;
	std::string name_short;
	uint8_t rssi = 0;
	int8_t tx_power = 0;
	std::shared_ptr<ManufactureSpecificData> msd = nullptr;
	std::vector<std::shared_ptr<UUID>> services;

    void set(Element bit) { data_set |= static_cast<uint32_t>(bit); }

public:
    void setTimestamp(uint64_t ts) { timestamp = ts; }
	uint64_t getTimestamp() const { return timestamp; }
    bool isSet(Element bit) const { return 0 != (data_set & static_cast<uint32_t>(bit)); }

    void setEvtType(uint8_t et) { evt_type = et; set(Element::EVT_TYPE); }
    void setAddressType(uint8_t at) { bdaddr_type = at; set(Element::BDADDR_TYPE); }
    void setAddress(bdaddr_t const *a) { bacpy( &bdaddr, a ); set(Element::BDADDR); }
	void setName(const uint8_t *buffer, int buffer_len);
	void setShortName(const uint8_t *buffer, int buffer_len);
    void setRSSI(uint8_t v) { rssi = v; set(Element::RSSI); }
    void setTxPower(int8_t v) { tx_power = v; set(Element::TX_POWER); }
    void setManufactureSpecificData(uint16_t const company, uint8_t const * const data, int const data_len) {
    	msd = std::shared_ptr<ManufactureSpecificData>(new ManufactureSpecificData(company, data, data_len));
    	set(Element::MANUF_DATA);
    }

    void addService(std::shared_ptr<UUID> const &uuid);

    bdaddr_t const & getAddress() { return bdaddr; }
    std::string const & getName() { return name; }

    std::string getAddressString() const;
    std::string dataSetToString() const;
    std::string toString() const;
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
    const bdaddr_t mac;

    HCIDevice(EInfoReport &r);
    uint64_t getCreationTimestamp() { return ts_creation; }
    uint64_t getUpdateTimestamp() { return ts_update; }
    bdaddr_t const & getAddress() const { return mac; }
    std::string getAddressString() const;
    std::string getName() const { return name; }
    uint8_t getRSSI() const { return rssi; }
    uint8_t getTxPower() const { return tx_power; }
    std::shared_ptr<ManufactureSpecificData> const getManufactureSpecificData() const { return msd; }

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
