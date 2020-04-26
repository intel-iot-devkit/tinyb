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

#define USE_BT_MGMT 1

// *************************************************
// *************************************************
// *************************************************

std::atomic_int HCISession::name_counter(0);

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

    adapterInfo = mgmt.getAdapter(dev_id);
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
: mgmt(DBTManager::get(BTMode::BT_MODE_LE)), dev_id(mgmt.getDefaultAdapterIdx())
{
    valid = validateDevInfo();
}

DBTAdapter::DBTAdapter(EUI48 &mac) 
: mgmt(DBTManager::get(BTMode::BT_MODE_LE)), dev_id(mgmt.findAdapterIdx(mac))
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
        perror("Could not open device");
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
#ifdef USE_BT_MGMT
    // FIXME:
    (void)own_mac_type;
    (void)interval;
    (void)window;

    keepDiscoveringAlive = true;
    currentScanType = mgmt.startDiscovery(dev_id);
    return ScanType::SCAN_TYPE_NONE != currentScanType;
#else
    if( nullptr == session || !session->isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return false;
    }
    if( !session->hciComm.le_enable_scan(own_mac_type, interval, window) ) {
        perror("Start scanning failed");
        return false;
    }
    return true;
#endif
}

void DBTAdapter::stopDiscovery() {
#ifdef USE_BT_MGMT
    DBG_PRINT("DBTAdapter::stopDiscovery: ...");
    keepDiscoveringAlive = false;
    if( mgmt.stopDiscovery(dev_id, currentScanType) ) {
        currentScanType = ScanType::SCAN_TYPE_NONE;
    }
#else
    if( nullptr == session || !session->isOpen() ) {
        DBG_PRINT("DBTAdapter::stopDiscovery: Not open");
        return;
    }
    DBG_PRINT("DBTAdapter::stopDiscovery: ...");
    session->hciComm.le_disable_scan();
#endif
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

int DBTAdapter::discoverDevices(const int waitForDeviceCount,
                                const EUI48 &waitForDevice,
                                const int timeoutMS,
                                const uint32_t ad_type_req)
{
#ifdef USE_BT_MGMT
    return discoverDevicesMgmt(waitForDeviceCount, waitForDevice, timeoutMS, ad_type_req);
#else
    return discoverDevicesHCI(waitForDeviceCount, waitForDevice, timeoutMS, ad_type_req);
#endif
}

int DBTAdapter::discoverDevicesHCI(const int waitForDeviceCount,
                                   const EUI48 &waitForDevice,
                                   const int timeoutMS,
                                   const uint32_t ad_type_req)
{
    if( nullptr == session || !session->isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return false;
    }
    const std::lock_guard<std::recursive_mutex> lock(session->mutex()); // RAII-style acquire and relinquish via destructor

    hci_ufilter nf, of;
    socklen_t olen;
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    int bytes_left = -1;
    int err;

    olen = sizeof(of);
    if (getsockopt(session->dd(), SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
        perror("Could not get socket options");
        return false;
    }

    HCIComm::filter_clear(&nf);
    HCIComm::filter_set_ptype(HCI_EVENT_PKT, &nf);
    HCIComm::filter_set_event(HCI_EV_LE_META, &nf);

    if (setsockopt(session->dd(), SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
        perror("Could not set socket options");
        return false;
    }
    const int64_t t0 = getCurrentMilliseconds();
    const uint32_t ad_req = static_cast<uint32_t>(EInfoReport::Element::BDADDR) |
                            static_cast<uint32_t>(EInfoReport::Element::RSSI) |
                            ad_type_req;
    bool done = false;
    int64_t t1;
    int matchedDeviceCount = 0, loop=0;

    while ( !done && ( ( t1 = getCurrentMilliseconds() ) - t0 ) < timeoutMS ) {
        uint8_t hci_type;
        hci_event_hdr *ehdr;
        hci_ev_le_meta *meta;
        loop++;

        if( timeoutMS ) {
            struct pollfd p;
            int n;

            p.fd = session->dd(); p.events = POLLIN;
            while ((n = poll(&p, 1, timeoutMS)) < 0) {
                if (errno == EAGAIN || errno == EINTR ) {
                    // cont temp unavail or interruption
                    continue;
                }
                goto errout;
            }
            if (!n) {
                // A timeout is not considered an error for discovery.
                // errno = ETIMEDOUT;
                // goto errout;
                goto done;
            }
        }

        while ((bytes_left = read(session->dd(), buf, sizeof(buf))) < 0) {
            if (errno == EAGAIN || errno == EINTR ) {
                // cont temp unavail or interruption
                continue;
            }
            goto errout;
        }

        if( bytes_left < HCI_TYPE_LEN + (int)sizeof(hci_event_hdr) + (int)sizeof(hci_ev_le_meta) ) {
            // not enough data ..
            continue;
        }
        hci_type = buf[0]; // sizeof HCI_TYPE_LEN

        ehdr = (hci_event_hdr*)(void*) ( buf + HCI_TYPE_LEN ); // sizeof hci_event_hdr

        bytes_left -= (HCI_TYPE_LEN + HCI_EVENT_HDR_SIZE);
        meta = (hci_ev_le_meta*)(void *) ( buf + ( HCI_TYPE_LEN + (int)sizeof(hci_event_hdr) ) ); // sizeof hci_ev_le_meta

        if( bytes_left < ehdr->plen ) {
            // not enough data ..
            fprintf(stderr, "DBTAdapter::discovery[%d]: Warning: Incomplete type 0x%.2X, event 0x%.2X, subevent 0x%.2X, remaining %d bytes < plen %d!\n",
                    loop-1, hci_type, ehdr->evt, meta->subevent, bytes_left, ehdr->plen);
            continue;
        } else {
            DBG_PRINT("DBTAdapter::discovery[%d]: Complete type 0x%.2X, event 0x%.2X, subevent 0x%.2X, remaining %d bytes >= plen %d",
                    loop-1, hci_type, ehdr->evt, meta->subevent, bytes_left, ehdr->plen);
        }

        // HCI_LE_Advertising_Report == 0x3E == HCI_EV_LE_META
        //        0x3E                                                           0x02
        if ( HCI_Event_Types::LE_Advertising_Report != ehdr->evt || meta->subevent != HCI_EV_LE_ADVERTISING_REPORT ) {
            continue; // next ..
        }
        bytes_left -= sizeof(hci_ev_le_meta);

        std::vector<std::shared_ptr<EInfoReport>> ad_reports = EInfoReport::read_ad_reports(((uint8_t*)(void *)meta)+1, bytes_left);

        const int num_reports = ad_reports.size();

        for(int i = 0; i < num_reports && i < 0x19; i++) {
            std::shared_ptr<EInfoReport> ad_report = ad_reports.at(i);
            const bool matches = ( ad_req == ( ad_req & ad_report->getDataSet() ) ) &&
                                 ( EUI48_ANY_DEVICE == waitForDevice || ad_report->getAddress() == waitForDevice );
            if( matches ) {
                matchedDeviceCount++;
                if( 0 < waitForDeviceCount && waitForDeviceCount <= matchedDeviceCount ) {
                    done = true;
                }
            }
            DBG_PRINT("DBTAdapter::discovery[%d] %d/%d: matches %d, waitForDevice %s, ad_req %s, matchCount %d/%d, done %d",
                    loop-1, i, num_reports, matches, waitForDevice.toString().c_str(), EInfoReport::dataSetToString(ad_req).c_str(),
                    matchedDeviceCount, waitForDeviceCount, done);
            DBG_PRINT("DBTAdapter::discovery[%d] %d/%d: %s", loop-1, i, num_reports, ad_report->toString().c_str());

            int idx = findDevice(scannedDevices, ad_report->getAddress());
            std::shared_ptr<DBTDevice> dev;
            if( 0 > idx ) {
                // new device
                dev = std::shared_ptr<DBTDevice>(new DBTDevice(*this, *ad_report));
                scannedDevices.push_back(dev);
            } else {
                // existing device
                dev = scannedDevices.at(idx);
                dev->update(*ad_report);
            }
            if( matches ) {
                if( addDiscoveredDevice(dev) ) {
                    // new matching
                    if( nullptr != deviceStatusListener ) {
                        deviceStatusListener->deviceFound(*this, dev, ad_report->getTimestamp());
                    }
                } else {
                    // update
                    if( nullptr != deviceStatusListener ) {
                        deviceStatusListener->deviceUpdated(*this, dev, ad_report->getTimestamp());
                    }
                }
            }
        }

    }
done:
    setsockopt(session->dd(), SOL_HCI, HCI_FILTER, &of, sizeof(of));
    return matchedDeviceCount;

errout:
    err = errno;
    setsockopt(session->dd(), SOL_HCI, HCI_FILTER, &of, sizeof(of));
    errno = err;
    return -1;
}

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
int DBTAdapter::discoverDevicesMgmt(const int waitForDeviceCount,
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
