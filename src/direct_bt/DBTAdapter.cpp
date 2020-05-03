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

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <cstdio>

#include <algorithm>

#define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "BTIoctl.hpp"
#include "HCIIoctl.hpp"
#include "HCIComm.hpp"
#include "DBTTypes.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
    #include <poll.h>
}

using namespace direct_bt;

// *************************************************
// *************************************************
// *************************************************

std::atomic_int HCISession::name_counter(0);

HCISession::HCISession(DBTAdapter &a, const uint16_t channel, const int timeoutMS)
: adapter(&a), hciComm(a.dev_id, channel, timeoutMS),
  name(name_counter.fetch_add(1))
{}

void HCISession::connected(std::shared_ptr<DBTDevice> & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = connectedDevices.begin(); it != connectedDevices.end(); ++it) {
        if ( *device == **it ) {
            return; // already connected
        }
    }
    connectedDevices.push_back(device);
}

void HCISession::disconnected(std::shared_ptr<DBTDevice> & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = connectedDevices.begin(); it != connectedDevices.end(); ) {
        if ( **it == *device ) {
            it = connectedDevices.erase(it);
        } else {
            ++it;
        }
    }
}

int HCISession::disconnectAllDevices(const uint8_t reason) {
    int count = 0;
    std::vector<std::shared_ptr<DBTDevice>> devices(connectedDevices); // copy!
    for (auto it = devices.begin(); it != devices.end(); ++it) {
        (*it)->disconnect(reason); // will erase device from list via disconnected above
        ++count;
    }
    return count;
}

std::shared_ptr<DBTDevice> HCISession::findConnectedDevice (EUI48 const & mac) const {
    std::vector<std::shared_ptr<DBTDevice>> devices(connectedDevices); // copy!
    for (auto it = devices.begin(); it != devices.end(); ) {
        if ( mac == (*it)->getAddress() ) {
            return *it;
        } else {
            ++it;
        }
    }
    return nullptr;
}

bool HCISession::close() 
{
    DBG_PRINT("HCISession::close: ...");
    if( nullptr == adapter ) {
        throw InternalError("HCISession::close(): Adapter reference is null: "+toString(), E_FILE_LINE);
    }
    if( !hciComm.isOpen() ) {
        DBG_PRINT("HCISession::close: Not open");
        return false;
    }
    disconnectAllDevices();
    adapter->sessionClosing();
    hciComm.close();
    DBG_PRINT("HCISession::close: XXX");
    return true;
}

void HCISession::shutdown() {
    DBG_PRINT("HCISession::shutdown(has-adapter %d): ...", nullptr!=adapter);

    // close()
    if( !hciComm.isOpen() ) {
        DBG_PRINT("HCISession::shutdown: Not open");
    } else {
        disconnectAllDevices();
        hciComm.close();
    }

    DBG_PRINT("HCISession::shutdown(has-adapter %d): XXX", nullptr!=adapter);
    adapter=nullptr;
}

HCISession::~HCISession() {
    DBG_PRINT("HCISession::dtor ...");
    if( nullptr != adapter ) {
        close();
        adapter = nullptr;
    }
    DBG_PRINT("HCISession::dtor XXX");
}

std::string HCISession::toString() const {
    return "HCISession[name "+std::to_string(name)+
            ", open "+std::to_string(isOpen())+
            ", "+std::to_string(this->connectedDevices.size())+" connected LE devices]";
}

// *************************************************
// *************************************************
// *************************************************

bool DBTAdapter::validateDevInfo() {
    if( !mgmt.isOpen() || 0 > dev_id ) {
        return false;
    }

    adapterInfo = mgmt.getAdapterInfo(dev_id);
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DISCOVERING, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceDiscoveringCB));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_CONNECTED, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceConnectedCB));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_DISCONNECTED, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceDisconnectedCB));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_FOUND, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceFoundCB));

    return true;
}

void DBTAdapter::sessionClosing()
{
    DBG_PRINT("DBTAdapter::sessionClosing(own-session %d): ...", nullptr!=session);
    if( nullptr != session ) {
        // FIXME: stopDiscovery();
        session = nullptr;
    }
    DBG_PRINT("DBTAdapter::sessionClosing(own-session %d): XXX", nullptr!=session);
}

DBTAdapter::DBTAdapter()
: mgmt(DBTManager::get(BTMode::BT_MODE_LE)), dev_id(nullptr != mgmt.getDefaultAdapterInfo() ? 0 : -1)
{
    valid = validateDevInfo();
}

DBTAdapter::DBTAdapter(EUI48 &mac) 
: mgmt(DBTManager::get(BTMode::BT_MODE_LE)), dev_id(mgmt.findAdapterInfoIdx(mac))
{
    valid = validateDevInfo();
}

DBTAdapter::DBTAdapter(const int dev_id) 
: mgmt(DBTManager::get(BTMode::BT_MODE_LE)), dev_id(dev_id)
{
    valid = validateDevInfo();
}

DBTAdapter::~DBTAdapter() {
    DBG_PRINT("DBTAdapter::dtor: ... %s", toString().c_str());
    keepDiscoveringAlive = false;
    {
        int count;
        if( 4 != ( count = mgmt.removeMgmtEventCallback(dev_id) ) ) {
            ERR_PRINT("DBTAdapter removeMgmtEventCallback(DISCOVERING) not 4 but %d", count);
        }
    }
    deviceStatusListener = nullptr;

    removeDiscoveredDevices();

    if( nullptr != session ) {
        stopDiscovery();
        session->shutdown(); // force shutdown, not only via dtor as adapter EOL has been reached!
        session = nullptr;
    }
    DBG_PRINT("DBTAdapter::dtor: XXX");
}

std::shared_ptr<HCISession> DBTAdapter::open() 
{
    if( !valid ) {
        return nullptr;
    }
    HCISession * s = new HCISession( *this, HCI_CHANNEL_RAW, HCIDefaults::HCI_TO_SEND_REQ_POLL_MS);
    if( !s->isOpen() ) {
        delete s;
        ERR_PRINT("Could not open device");
        return nullptr;
    }
    session = std::shared_ptr<HCISession>( s );
    return session;
}

std::shared_ptr<DBTDeviceStatusListener> DBTAdapter::setDeviceStatusListener(std::shared_ptr<DBTDeviceStatusListener> l)
{
    std::shared_ptr<DBTDeviceStatusListener> o = deviceStatusListener;
    deviceStatusListener = l;
    return o;
}

bool DBTAdapter::startDiscovery(HCIAddressType own_mac_type,
                                uint16_t interval, uint16_t window)
{
    (void)own_mac_type;
    (void)interval;
    (void)window;

    removeDiscoveredDevices();
    keepDiscoveringAlive = true;
    currentScanType = mgmt.startDiscovery(dev_id);
    return ScanType::SCAN_TYPE_NONE != currentScanType;
}

void DBTAdapter::startDiscoveryBackground() {
    currentScanType = mgmt.startDiscovery(dev_id);
}

void DBTAdapter::stopDiscovery() {
    DBG_PRINT("DBTAdapter::stopDiscovery: ...");
    keepDiscoveringAlive = false;
    if( mgmt.stopDiscovery(dev_id, currentScanType) ) {
        currentScanType = ScanType::SCAN_TYPE_NONE;
    }
    DBG_PRINT("DBTAdapter::stopDiscovery: X");
}

int DBTAdapter::findDevice(std::vector<std::shared_ptr<DBTDevice>> const & devices, EUI48 const & mac) {
    auto begin = devices.begin();
    auto it = std::find_if(begin, devices.end(), [&](std::shared_ptr<DBTDevice> const& p) {
        return p->address == mac;
    });
    if ( it == std::end(devices) ) {
        return -1;
    } else {
        return std::distance(begin, it);
    }
}

std::shared_ptr<DBTDevice> DBTAdapter::findDiscoveredDevice (EUI48 const & mac) const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTAdapter*>(this)->mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    auto begin = discoveredDevices.begin();
    auto it = std::find_if(begin, discoveredDevices.end(), [&](std::shared_ptr<DBTDevice> const& p) {
        return p->address == mac;
    });
    if ( it == std::end(discoveredDevices) ) {
        return nullptr;
    } else {
        return *it;
    }
}

void DBTAdapter::addDiscoveredDevice(std::shared_ptr<DBTDevice> const &device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = discoveredDevices.begin(); it != discoveredDevices.end(); ++it) {
        if ( *device == **it ) {
            // already discovered, just replace
        }
    }
    discoveredDevices.push_back(device);
}

int DBTAdapter::removeDiscoveredDevices() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    int res = discoveredDevices.size();
    discoveredDevices.clear();
    return res;
}

std::vector<std::shared_ptr<DBTDevice>> DBTAdapter::getDiscoveredDevices() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTAdapter*>(this)->mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    std::vector<std::shared_ptr<DBTDevice>> res = discoveredDevices;
    return res;
}

std::string DBTAdapter::toString() const {
    std::string out("Adapter["+getAddressString()+", '"+getName()+"', id="+std::to_string(dev_id)+", "+javaObjectToString()+"]");
    std::vector<std::shared_ptr<DBTDevice>> devices = getDiscoveredDevices();
    if(devices.size() > 0 ) {
        out.append("\n");
        for(auto it = devices.begin(); it != devices.end(); it++) {
            std::shared_ptr<DBTDevice> p = *it;
            out.append("  ").append(p->toString()).append("\n");
        }
    }
    return out;
}

// *************************************************

bool DBTAdapter::mgmtEvDeviceDiscoveringCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventCB:DeviceDiscovering(keepDiscoveringAlive %d): %s",
            keepDiscoveringAlive, e->toString().c_str());
    const MgmtEvtDiscovering &event = *static_cast<const MgmtEvtDiscovering *>(e.get());
    if( keepDiscoveringAlive && !event.getEnabled() ) {
        std::thread bg(&DBTAdapter::startDiscoveryBackground, this);
        bg.detach();
    }
    return true;
}

bool DBTAdapter::mgmtEvDeviceConnectedCB(std::shared_ptr<MgmtEvent> e) {
    const MgmtEvtDeviceConnected &event = *static_cast<const MgmtEvtDeviceConnected *>(e.get());
    EInfoReport ad_report;
    {
        ad_report.setSource(EInfoReport::Source::EIR);
        ad_report.setTimestamp(event.getTimestamp());
        ad_report.setAddressType(event.getAddressType());
        ad_report.setAddress( event.getAddress() );
        ad_report.read_data(event.getData(), event.getDataSize());
    }
    std::shared_ptr<DBTDevice> device = session->findConnectedDevice(event.getAddress());
    if( nullptr != device ) {
        DBG_PRINT("DBTAdapter::EventCB:DeviceConnected: %s,\n    %s\n    -> %s",
                event.toString().c_str(), ad_report.toString().c_str(), device->toString().c_str());
        if( nullptr != deviceStatusListener ) {
            deviceStatusListener->deviceConnected(*this, device, event.getTimestamp());
        }
    } else {
        DBG_PRINT("DBTAdapter::EventCB:DeviceConnected: %s,\n    %s\n    -> Device not tracked",
                event.toString().c_str(), ad_report.toString().c_str());
    }
    (void)ad_report;
    return true;
}
bool DBTAdapter::mgmtEvDeviceDisconnectedCB(std::shared_ptr<MgmtEvent> e) {
    const MgmtEvtDeviceDisconnected &event = *static_cast<const MgmtEvtDeviceDisconnected *>(e.get());
    std::shared_ptr<DBTDevice> device = session->findConnectedDevice(event.getAddress());
    if( nullptr != device ) {
        DBG_PRINT("DBTAdapter::EventCB:DeviceDisconnected: %s\n    -> %s", event.toString().c_str(), device->toString().c_str());
        if( nullptr != deviceStatusListener ) {
            deviceStatusListener->deviceDisconnected(*this, device, event.getTimestamp());
        }
    } else {
        DBG_PRINT("DBTAdapter::EventCB:DeviceDisconnected: %s\n    -> Device not tracked", event.toString().c_str());
    }
    return true;
}

bool DBTAdapter::mgmtEvDeviceFoundCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventCB:DeviceFound: %s", e->toString().c_str());
    const MgmtEvtDeviceFound &deviceFoundEvent = *static_cast<const MgmtEvtDeviceFound *>(e.get());
    if( deviceFoundEvent.getDevID() != dev_id ) {
        return true;
    }
    EInfoReport ad_report;
    ad_report.setSource(EInfoReport::Source::EIR);
    ad_report.setTimestamp(deviceFoundEvent.getTimestamp());
    // ad_report.setEvtType(0); ??
    ad_report.setAddressType(deviceFoundEvent.getAddressType());
    ad_report.setAddress( deviceFoundEvent.getAddress() );
    ad_report.setRSSI( deviceFoundEvent.getRSSI() );
    ad_report.read_data(deviceFoundEvent.getData(), deviceFoundEvent.getDataSize());

    std::shared_ptr<DBTDevice> dev = findDiscoveredDevice(ad_report.getAddress());
    if( nullptr == dev ) {
        // new device
        dev = std::shared_ptr<DBTDevice>(new DBTDevice(*this, ad_report));
        addDiscoveredDevice(dev);
        if( nullptr != deviceStatusListener ) {
            deviceStatusListener->deviceFound(*this, dev, ad_report.getTimestamp());
        }
    } else {
        // existing device
        EIRDataType updateMask = dev->update(ad_report);
        if( EIRDataType::NONE != updateMask && nullptr != deviceStatusListener ) {
            deviceStatusListener->deviceUpdated(*this, dev, ad_report.getTimestamp(), updateMask);
        }
    }
    return true;
}
