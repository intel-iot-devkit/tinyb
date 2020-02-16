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

#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include  <algorithm>

#include "HCITypes.hpp"

using namespace tinyb_hci;

// *************************************************
// *************************************************
// *************************************************

HCIDevice::HCIDevice(EInfoReport &r)
: ts_creation(r.getTimestamp()), mac(r.getAddress())
{
    if( !r.isSet(EInfoReport::Element::BDADDR) ) {
        throw IllegalArgumentException("HCIDevice ctor: Address not set: "+r.toString());
    }
    update(r);
}

std::string HCIDevice::getAddressString() const {
    if( valid ) {
        char sa[18];
        ba2str(&mac, sa);
        return std::string(sa);
    } else {
        return std::string();
    }
}

void HCIDevice::addService(std::shared_ptr<UUID> const &uuid)
{
    if( 0 > findService(uuid) ) {
        services.push_back(uuid);
    }
}
void HCIDevice::addServices(std::vector<std::shared_ptr<UUID>> const & services)
{
    for(int j=0; j<services.size(); j++) {
        const std::shared_ptr<UUID> uuid = services.at(j);
        addService(uuid);
    }
}

int HCIDevice::findService(std::shared_ptr<UUID> const &uuid) const
{
    auto begin = services.begin();
    auto it = std::find_if(begin, services.end(), [&](std::shared_ptr<UUID> const& p) {
        return *p == *uuid;
    });
    if ( it == std::end(services) ) {
        return -1;
    } else {
        return std::distance(begin, it);
    }
}

std::string HCIDevice::toString() const {
    const uint64_t t0 = getCurrentMilliseconds();
    std::string msdstr = nullptr != msd ? msd->toString() : "MSD[null]";
    std::string out("Device["+getAddressString()+", '"+getName()+
            "', age "+std::to_string(t0-ts_creation)+" ms, lup "+std::to_string(t0-ts_update)+" ms, rssi "+std::to_string(getRSSI())+
            ", tx-power "+std::to_string(tx_power)+", "+msdstr+"]");
    if(services.size() > 0 ) {
        out.append("\n");
        for(auto it = services.begin(); it != services.end(); it++) {
            std::shared_ptr<UUID> p = *it;
            out.append("  ").append(p->toUUID128String()).append(", ").append(std::to_string(static_cast<int>(p->type))).append(" bytes\n");
        }
    }
    return out;
}

void HCIDevice::update(EInfoReport const & data) {
    ts_update = data.getTimestamp();
    if( data.isSet(EInfoReport::Element::NAME) ) {
        if( !name.length() || data.name.length() > name.length() ) {
            name = data.name;
        }
    }
    if( data.isSet(EInfoReport::Element::NAME_SHORT) ) {
        if( !name.length() ) {
            name = data.name_short;
        }
    }
    if( data.isSet(EInfoReport::Element::RSSI) ) {
        rssi = data.rssi;
    }
    if( data.isSet(EInfoReport::Element::TX_POWER) ) {
        tx_power = data.tx_power;
    }
    if( data.isSet(EInfoReport::Element::MANUF_DATA) ) {
        msd = data.msd;
    }
    addServices(data.services);
}

ManufactureSpecificData::ManufactureSpecificData(uint16_t const company, uint8_t const * const data, int const data_len)
: company(company), companyName(std::string(bt_compidtostr(company))), data_len(data_len), data(new uint8_t[data_len]) {
    memcpy(this->data.get(), data, data_len);
}

std::string ManufactureSpecificData::toString() const {
  std::string out("MSD[");
  out.append(std::to_string(company)+" "+companyName);
  out.append(", data "+std::to_string(data_len)+" bytes]");
  return out;
}

static std::string get_string(const uint8_t *buffer, int buffer_len) {
    const int cstr_len = std::min(buffer_len, 30);
    char cstr[30+1]; // EOS
    memcpy(cstr, buffer, cstr_len);
    cstr[cstr_len] = 0; // EOS
    return std::string(cstr);
}

void EInfoReport::setName(const uint8_t *buffer, int buffer_len) {
    name = get_string(buffer, buffer_len);
    set(Element::NAME);
}

void EInfoReport::setShortName(const uint8_t *buffer, int buffer_len) {
    name_short = get_string(buffer, buffer_len);
    set(Element::NAME_SHORT);
}

void EInfoReport::addService(std::shared_ptr<UUID> const &uuid)
{
    auto begin = services.begin();
    auto it = std::find_if(begin, services.end(), [&](std::shared_ptr<UUID> const& p) {
        return *p == *uuid;
    });
    if ( it == std::end(services) ) {
        services.push_back(uuid);
    }
}

std::string EInfoReport::getAddressString() const {
    char sa[18];
    ba2str(&bdaddr, sa);
    return std::string(sa);
}

std::string EInfoReport::dataSetToString() const {
    std::string out("DataSet[");
    if( isSet(Element::EVT_TYPE) ) {
        out.append("EVT_TYPE, ");
    }
    if( isSet(Element::BDADDR) ) {
        out.append("BDADDR, ");
    }
    if( isSet(Element::NAME) ) {
        out.append("NAME, ");
    }
    if( isSet(Element::NAME_SHORT) ) {
        out.append("NAME_SHORT, ");
    }
    if( isSet(Element::RSSI) ) {
        out.append("RSSI, ");
    }
    if( isSet(Element::TX_POWER) ) {
        out.append("TX_POWER, ");
    }
    if( isSet(Element::MANUF_DATA) ) {
        out.append("MANUF_DATA, ");
    }
    out.append("]");
    return out;
}
std::string EInfoReport::toString() const {
    std::string msdstr = nullptr != msd ? msd->toString() : "MSD[null]";
    std::string out("ADRecord["+getAddressString()+", "+name+"/"+name_short+", "+dataSetToString()+
                    ", evt-type "+std::to_string(evt_type)+", rssi "+std::to_string(rssi)+
                    ", tx-power "+std::to_string(tx_power)+", "+msdstr+"]");
    if(services.size() > 0 ) {
        out.append("\n");
        for(auto it = services.begin(); it != services.end(); it++) {
            std::shared_ptr<UUID> p = *it;
            out.append("  ").append(p->toUUID128String()).append(", ").append(std::to_string(static_cast<int>(p->type))).append(" bytes\n");
        }
    }
    return out;
}

// *************************************************
// *************************************************
// *************************************************

int HCIAdapter::getDefaultDevId() {
    return hci_get_route(NULL);
}
int HCIAdapter::getDevId(bdaddr_t &bdaddr) {
    return hci_get_route(&bdaddr);
}
int HCIAdapter::getDevId(const std::string &hcidev) {
    return hci_devid(hcidev.c_str());
}

bool HCIAdapter::validateDevInfo() {
    memset(&dev_info, 0, sizeof(dev_info));
    if( 0 > dev_id ) {
        return false;
    }
    if( 0 > hci_devinfo(dev_id, &dev_info) ) {
        return false;
    }
    return true;
}

void HCIAdapter::sessionClosed(HCISession& s) 
{
    auto it = std::find_if(sessions.begin(), sessions.end(), [&](std::shared_ptr<HCISession> const& p) {
        return *p == s;
    });
    if ( it != std::end(sessions) ) {
        sessions.erase(it);
    }
}

HCIAdapter::HCIAdapter()
: dev_id(getDefaultDevId())
{
    valid = validateDevInfo();
}

HCIAdapter::HCIAdapter(bdaddr_t &mac) 
: dev_id(getDevId(mac))
{
    valid = validateDevInfo();
}

HCIAdapter::HCIAdapter(const std::string &hcidev)
: dev_id(getDevId(hcidev))
{
    valid = validateDevInfo();
}

HCIAdapter::HCIAdapter(const int dev_id) 
: dev_id(dev_id)
{
    valid = validateDevInfo();
}

HCIAdapter::~HCIAdapter() {
    discoveredDevices.clear();
    sessions.clear();
}

std::string HCIAdapter::getAddress() const {
    if( valid ) {
        char sa[18];
        ba2str(&dev_info.bdaddr, sa);
        return std::string(sa);
    } else {
        return std::string();
    }
}

std::string HCIAdapter::getName() const {
    if( valid ) {
        return std::string(dev_info.name);
    } else {
        return std::string();
    }
}

std::shared_ptr<HCISession> HCIAdapter::open() 
{
    if( !valid ) {
        return nullptr;
    }
    int dd = hci_open_dev(dev_id);
    if( 0 > dd ) {
        perror("Could not open device");
        return nullptr;
    }
    std::shared_ptr<HCISession> s(new HCISession(*this, dd));
    sessions.push_back(s);
    return s;
}

std::shared_ptr<HCIDeviceDiscoveryListener> HCIAdapter::setDeviceDiscoveryListener(std::shared_ptr<HCIDeviceDiscoveryListener> l)
{
    std::shared_ptr<HCIDeviceDiscoveryListener> o = deviceDiscoveryListener;
    deviceDiscoveryListener = l;
    return o;
}

std::shared_ptr<HCISession> HCIAdapter::startDiscovery() {
    const uint8_t own_type = LE_PUBLIC_ADDRESS;
    const uint8_t scan_type = 0x01;
    const uint8_t filter_type = 0;
    const uint8_t filter_policy = 0x00;
    const uint16_t interval = htobs(0x0010);
    const uint16_t window = htobs(0x0010);
    const uint8_t filter_dup = 0x01;
    
    std::shared_ptr<HCISession> session = open();
    if( nullptr == session ) {
        return nullptr;
    }

    if( !session->isOpen() ) {
        fprintf(stderr, "New session not open\n");
        return nullptr;
    }
    int err = hci_le_set_scan_parameters(session->dd(), scan_type, interval, window,
                        own_type, filter_policy, to_send_req_poll_ms);
    if (err < 0) {
        perror("Set scan parameters failed");
        session->close();
        return nullptr;
    }

    err = hci_le_set_scan_enable(session->dd(), 0x01, filter_dup, to_send_req_poll_ms);
    if (err < 0) {
        perror("Start scan failed");
        session->close();
        return nullptr;
    }
    return session;
}

bool HCIAdapter::stopDiscovery(HCISession& session) {
    const uint8_t filter_dup = 0x01;

    if( !session.isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return false;
    }
    bool res;
    if( 0 > hci_le_set_scan_enable(session.dd(), 0x00, filter_dup, to_send_req_poll_ms) ) {
        perror("Stop scan failed");
        res = false;
    } else {
        res = true;
    }
    session.close();
    return res;
}

void HCIAdapter::addDevice(std::shared_ptr<HCIDevice> const &device) {
    if( 0 > findDevice(device->mac) ) {
        discoveredDevices.push_back(device);
    }
}

int HCIAdapter::findDevice(bdaddr_t const & mac) const {
    auto begin = discoveredDevices.begin();
    auto it = std::find_if(begin, discoveredDevices.end(), [&](std::shared_ptr<HCIDevice> const& p) {
        return !bacmp(&p->mac, &mac);
    });
    if ( it == std::end(discoveredDevices) ) {
        return -1;
    } else {
        return std::distance(begin, it);
    }
}

std::string HCIAdapter::toString() const {
    std::string out("Adapter["+getAddress()+", "+getName()+", id="+std::to_string(dev_id)+"]");
    if(discoveredDevices.size() > 0 ) {
        out.append("\n");
        for(auto it = discoveredDevices.begin(); it != discoveredDevices.end(); it++) {
            std::shared_ptr<HCIDevice> p = *it;
            out.append("  ").append(p->toString()).append("\n");
        }
    }
    return out;
}

// *************************************************
// *************************************************
// *************************************************

std::atomic_int HCISession::name_counter(0);

bool HCISession::close() 
{
    if( 0 > _dd ) {
        return false;
    }
    hci_close_dev(_dd);
    _dd = -1;
    adapter.sessionClosed(*this);
    return true;
}

