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

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>
#include <DBTTypes.hpp>

#include "BTIoctl.hpp"
#include "HCIIoctl.hpp"

#include "HCIComm.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
    #include <poll.h>
}

using namespace direct_bt;

// *************************************************
// *************************************************
// *************************************************

std::atomic_int DBTSession::name_counter(0);

void DBTSession::disconnect(const uint8_t reason) {
    connectedDevice = nullptr;

    if( !hciComm.isLEConnected() ) {
        DBG_PRINT("HCISession::disconnect: Not connected");
        return;
    }
    if( !hciComm.le_disconnect(reason) ) {
        DBG_PRINT("HCISession::disconnect: errno %d %s", errno, strerror(errno));
    }
}

bool DBTSession::close() 
{
    if( !hciComm.isOpen() ) {
        DBG_PRINT("HCISession::close: Not open");
        return false;
    }
    DBG_PRINT("HCISession::close: ...");
    adapter.sessionClosing(*this);
    hciComm.close();
    return true;
}

// *************************************************
// *************************************************
// *************************************************

bool DBTAdapter::validateDevInfo() {
    if( !mgmt.isOpen() || 0 > dev_id ) {
        return false;
    }

    adapterInfo = mgmt.getAdapter(dev_id);
    return true;
}

void DBTAdapter::sessionClosing(DBTSession& s) 
{
    stopDiscovery(s);
    session = nullptr;
}

DBTAdapter::DBTAdapter()
: mgmt(MgmtHandler::get()), dev_id(mgmt.getDefaultAdapterIdx())
{
    valid = validateDevInfo();
}

DBTAdapter::DBTAdapter(EUI48 &mac) 
: mgmt(MgmtHandler::get()), dev_id(mgmt.findAdapterIdx(mac))
{
    valid = validateDevInfo();
}

DBTAdapter::DBTAdapter(const int dev_id) 
: mgmt(MgmtHandler::get()), dev_id(dev_id)
{
    valid = validateDevInfo();
}

DBTAdapter::~DBTAdapter() {
    DBG_PRINT("HCIAdapter::dtor: %s", toString().c_str());
    deviceDiscoveryListener = nullptr;

    scannedDevices.clear();
    discoveredDevices.clear();
    session = nullptr;
}

std::shared_ptr<DBTSession> DBTAdapter::open() 
{
    if( !valid ) {
        return nullptr;
    }
    DBTSession * s = new DBTSession( *this, dev_id, HCI_CHANNEL_RAW );
    if( !s->isOpen() ) {
        delete s;
        perror("Could not open device");
        return nullptr;
    }
    session = std::shared_ptr<DBTSession>( s );
    return session;
}

std::shared_ptr<DBTDeviceDiscoveryListener> DBTAdapter::setDeviceDiscoveryListener(std::shared_ptr<DBTDeviceDiscoveryListener> l)
{
    std::shared_ptr<DBTDeviceDiscoveryListener> o = deviceDiscoveryListener;
    deviceDiscoveryListener = l;
    return o;
}

bool DBTAdapter::startDiscovery(DBTSession &session, uint8_t own_mac_type,
                                uint16_t interval, uint16_t window)
{
    if( !session.isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return false;
    }
    if( !session.hciComm.le_enable_scan(own_mac_type, interval, window) ) {
        perror("Start scanning failed");
        return false;
    }
    return true;
}

void DBTAdapter::stopDiscovery(DBTSession& session) {
    if( !session.isOpen() ) {
        DBG_PRINT("HCIAdapter::stopDiscovery: Not open");
        return;
    }
    DBG_PRINT("HCIAdapter::stopDiscovery: ...");
    session.hciComm.le_disable_scan();
}

int DBTAdapter::findDevice(std::vector<std::shared_ptr<DBTDevice>> const & devices, EUI48 const & mac) {
    auto begin = devices.begin();
    auto it = std::find_if(begin, devices.end(), [&](std::shared_ptr<DBTDevice> const& p) {
        return p->mac == mac;
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
    if( 0 > findDevice(scannedDevices, device->mac) ) {
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
    if( 0 > findDiscoveredDeviceIdx(device->mac) ) {
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

int DBTAdapter::discoverDevices(DBTSession& session,
                                const int waitForDeviceCount,
                                const EUI48 &waitForDevice,
                                const int timeoutMS,
                                const uint32_t ad_type_req)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    hci_ufilter nf, of;
    socklen_t olen;
    int bytes_left = -1;
    const int64_t t0 = getCurrentMilliseconds();
    int err;

    if( !session.isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return false;
    }

    olen = sizeof(of);
    if (getsockopt(session.dd(), SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
        perror("Could not get socket options");
        return false;
    }

    HCIComm::filter_clear(&nf);
    HCIComm::filter_set_ptype(HCI_EVENT_PKT, &nf);
    HCIComm::filter_set_event(HCI_EV_LE_META, &nf);

    if (setsockopt(session.dd(), SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
        perror("Could not set socket options");
        return false;
    }

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

            p.fd = session.dd(); p.events = POLLIN;
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

        while ((bytes_left = read(session.dd(), buf, sizeof(buf))) < 0) {
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
            fprintf(stderr, "HCIAdapter::discovery[%d]: Warning: Incomplete type 0x%.2X, event 0x%.2X, subevent 0x%.2X, remaining %d bytes < plen %d!\n",
                    loop-1, hci_type, ehdr->evt, meta->subevent, bytes_left, ehdr->plen);
            continue;
        } else {
            DBG_PRINT("HCIAdapter::discovery[%d]: Complete type 0x%.2X, event 0x%.2X, subevent 0x%.2X, remaining %d bytes >= plen %d",
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
            DBG_PRINT("HCIAdapter::discovery[%d] %d/%d: matches %d, waitForDevice %s, ad_req %s, matchCount %d/%d, done %d",
                    loop-1, i, num_reports, matches, waitForDevice.toString().c_str(), EInfoReport::dataSetToString(ad_req).c_str(),
                    matchedDeviceCount, waitForDeviceCount, done);
            DBG_PRINT("HCIAdapter::discovery[%d] %d/%d: %s", loop-1, i, num_reports, ad_report->toString().c_str());

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
                    if( nullptr != deviceDiscoveryListener ) {
                        deviceDiscoveryListener->deviceAdded(*this, dev);
                    }
                } else {
                    // update
                    if( nullptr != deviceDiscoveryListener ) {
                        deviceDiscoveryListener->deviceUpdated(*this, dev);
                    }
                }
            }
        }

    }
done:
    setsockopt(session.dd(), SOL_HCI, HCI_FILTER, &of, sizeof(of));
    return matchedDeviceCount;

errout:
    err = errno;
    setsockopt(session.dd(), SOL_HCI, HCI_FILTER, &of, sizeof(of));
    errno = err;
    return -1;
}
