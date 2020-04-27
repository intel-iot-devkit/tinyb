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
    connectedDevices.push_back(device);
}

void HCISession::disconnected(std::shared_ptr<DBTDevice> & device) {
    for (auto it = connectedDevices.begin(); it != connectedDevices.end(); ) {
        if ( *it == device ) {
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
    mgmt.addMgmtEventCallback(MgmtEvent::Opcode::DISCOVERING, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceDiscoveringCB));
    mgmt.addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_CONNECTED, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceConnectedCB));
    mgmt.addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_DISCONNECTED, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceDisconnectedCB));

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
        if( 1 != ( count = mgmt.removeMgmtEventCallback(MgmtEvent::Opcode::DISCOVERING, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceDiscoveringCB)) ) ) {
            ERR_PRINT("DBTAdapter removeMgmtEventCallback(DISCOVERING) not 1 but %d", count);
        }
    }
    deviceStatusListener = nullptr;

    scannedDevices.clear();
    discoveredDevices.clear();

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
    HCISession * s = new HCISession( *this, HCI_CHANNEL_RAW );
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
    // FIXME:
    (void)own_mac_type;
    (void)interval;
    (void)window;

    keepDiscoveringAlive = true;
    currentScanType = mgmt.startDiscovery(dev_id);
    return ScanType::SCAN_TYPE_NONE != currentScanType;
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

int DBTAdapter::findScannedDeviceIdx(EUI48 const & mac) const {
    return findDevice(scannedDevices, mac);
}

std::shared_ptr<DBTDevice> DBTAdapter::findScannedDevice (EUI48 const & mac) const {
    const int idx = findDevice(scannedDevices, mac);
    if( 0 <= idx ) {
        return scannedDevices.at(idx);
    }
    return nullptr;
}

bool DBTAdapter::addScannedDevice(std::shared_ptr<DBTDevice> const &device) {
    if( 0 > findDevice(scannedDevices, device->address) ) {
        scannedDevices.push_back(device);
        return true;
    }
    return false;
}

int DBTAdapter::findDiscoveredDeviceIdx(EUI48 const & mac) const {
    return findDevice(discoveredDevices, mac);
}

std::shared_ptr<DBTDevice> DBTAdapter::findDiscoveredDevice (EUI48 const & mac) const {
    const int idx = findDevice(discoveredDevices, mac);
    if( 0 <= idx ) {
        return discoveredDevices.at(idx);
    }
    return nullptr;
}

bool DBTAdapter::addDiscoveredDevice(std::shared_ptr<DBTDevice> const &device) {
    if( 0 > findDiscoveredDeviceIdx(device->address) ) {
        discoveredDevices.push_back(device);
        return true;
    }
    return false;
}

int DBTAdapter::removeDiscoveredDevices() {
    // also need to flush scannedDevices, old data
    scannedDevices.clear();
    int res = discoveredDevices.size();
    discoveredDevices.clear();
    return res;
}

std::string DBTAdapter::toString() const {
    std::string out("Adapter["+getAddressString()+", '"+getName()+"', id="+std::to_string(dev_id)+", "+javaObjectToString()+"]");
    if(discoveredDevices.size() > 0 ) {
        out.append("\n");
        for(auto it = discoveredDevices.begin(); it != discoveredDevices.end(); it++) {
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
void DBTAdapter::startDiscoveryBackground() { startDiscovery(); }

bool DBTAdapter::mgmtEvDeviceFoundCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventCB:DeviceFound: %s", e->toString().c_str());
    const MgmtEvtDeviceFound &event = *static_cast<const MgmtEvtDeviceFound *>(e.get());
    if( event.getDevID() == dev_id ) {
        std::unique_lock<std::mutex> lockRead(mtxEventRead); // RAII-style acquire and relinquish via destructor
        eventReceived = e;
        cvEventRead.notify_all(); // notify waiting getter
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

int DBTAdapter::discoverDevices(const int waitForDeviceCount,
                                const EUI48 &waitForDevice,
                                const int timeoutMS,
                                const uint32_t ad_type_req)
{
    mgmt.addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_FOUND, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceFoundCB));
    const int64_t t0 = getCurrentMilliseconds();
    const uint32_t ad_req = static_cast<uint32_t>(EInfoReport::Element::BDADDR) |
                            static_cast<uint32_t>(EInfoReport::Element::RSSI) |
                            ad_type_req;
    bool done = false;
    int64_t t1;
    int matchedDeviceCount = 0, loop=0;

    while ( !done && ( ( t1 = getCurrentMilliseconds() ) - t0 ) < timeoutMS ) {
        EInfoReport ad_report;
        {
            std::shared_ptr<MgmtEvent> deviceEvent = nullptr; // holding the resource
            const MgmtEvtDeviceFound * deviceFoundEvent = nullptr; // convenient target type access
            {
                std::unique_lock<std::mutex> lockRead(mtxEventRead); // RAII-style acquire and relinquish via destructor
                while( nullptr == deviceEvent ) { // FIXME deadlock, waiting forever!
                    cvEventRead.wait(lockRead);
                    if( nullptr != eventReceived && MgmtEvent::Opcode::DEVICE_FOUND == eventReceived->getOpcode() ) {
                        deviceEvent.swap(eventReceived); // take over eventReceived
                        deviceFoundEvent = static_cast<const MgmtEvtDeviceFound *>(deviceEvent.get());
                    }
                }
            }
            loop++;

            ad_report.setSource(EInfoReport::Source::EIR);
            ad_report.setTimestamp(deviceFoundEvent->getTimestamp());
            // ad_report.setEvtType(0); ??
            ad_report.setAddressType(deviceFoundEvent->getAddressType());
            ad_report.setAddress( deviceFoundEvent->getAddress() );
            ad_report.setRSSI( deviceFoundEvent->getRSSI() );
            ad_report.read_data(deviceFoundEvent->getData(), deviceFoundEvent->getDataSize());
        }

        const bool matches = ( ad_req == ( ad_req & ad_report.getDataSet() ) ) &&
                             ( EUI48_ANY_DEVICE == waitForDevice || ad_report.getAddress() == waitForDevice );
        if( matches ) {
            matchedDeviceCount++;
            if( 0 < waitForDeviceCount && waitForDeviceCount <= matchedDeviceCount ) {
                done = true;
            }
        }
        DBG_PRINT("DBTAdapter::discovery[%d]: matches %d, waitForDevice %s, ad_req %s, matchCount %d/%d, done %d",
                loop-1, matches, waitForDevice.toString().c_str(), EInfoReport::dataSetToString(ad_req).c_str(),
                matchedDeviceCount, waitForDeviceCount, done);
        DBG_PRINT("DBTAdapter::discovery[%d]: %s", loop-1, ad_report.toString().c_str());

        int idx = findDevice(scannedDevices, ad_report.getAddress());
        std::shared_ptr<DBTDevice> dev;
        if( 0 > idx ) {
            // new device
            dev = std::shared_ptr<DBTDevice>(new DBTDevice(*this, ad_report));
            scannedDevices.push_back(dev);
        } else {
            // existing device
            dev = scannedDevices.at(idx);
            dev->update(ad_report);
        }
        if( matches ) {
            if( addDiscoveredDevice(dev) ) {
                // new matching
                if( nullptr != deviceStatusListener ) {
                    deviceStatusListener->deviceFound(*this, dev, ad_report.getTimestamp());
                }
            } else {
                // update
                if( nullptr != deviceStatusListener ) {
                    deviceStatusListener->deviceUpdated(*this, dev, ad_report.getTimestamp());
                }
            }
        }
    }
    {
        int count;
        if( 1 != ( count = mgmt.removeMgmtEventCallback(MgmtEvent::Opcode::DEVICE_FOUND, bindClassFunction(this, &DBTAdapter::mgmtEvDeviceFoundCB)) ) ) {
            throw InternalError("DBTAdapter removeMgmtEventCallback(DEVICE_FOUND) not 1 but "+std::to_string(count), E_FILE_LINE);
        }
    }
    return matchedDeviceCount;
}
