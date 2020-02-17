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

#include  <algorithm>

#include <inttypes.h>
#include <unistd.h>
#include <poll.h>

extern "C" {
    #include <bluetooth/bluetooth.h>
    #include <bluetooth/hci.h>
    #include <bluetooth/hci_lib.h>
}

#include "HCITypes.hpp"

#define VERBOSE_ON 1

#ifdef VERBOSE_ON
    #define DBG_PRINT(...) fprintf(stderr, __VA_ARGS__); fflush(stderr)
#else
    #define DBG_PRINT(...)
#endif

using namespace tinyb_hci;

static inline const bdaddr_t* const_eui48_to_const_bdaddr_ptr(const EUI48 *p) {
    return static_cast<const bdaddr_t *>( static_cast<void *>( const_cast<EUI48 *>( p ) ) );
}
static inline bdaddr_t* eui48_to_bdaddr_ptr(EUI48 *p) {
    return static_cast<bdaddr_t *>( static_cast<void *>( p ) );
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

// *************************************************
// *************************************************
// *************************************************

int HCIAdapter::getDefaultDevId() {
    return hci_get_route(NULL);
}
int HCIAdapter::getDevId(EUI48 &mac) {
    return hci_get_route( eui48_to_bdaddr_ptr( &mac ) );
}
int HCIAdapter::getDevId(const std::string &hcidev) {
    return hci_devid(hcidev.c_str());
}

bool HCIAdapter::validateDevInfo() {
    if( 0 > dev_id ) {
        return false;
    }
    struct hci_dev_info dev_info;
    bzero(&dev_info, sizeof(dev_info));
    if( 0 > hci_devinfo(dev_id, &dev_info) ) {
        return false;
    }
    memcpy(&mac, &dev_info.bdaddr, sizeof(mac));
    name = std::string(dev_info.name);
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

HCIAdapter::HCIAdapter(EUI48 &mac) 
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

bool HCIAdapter::startDiscovery(HCISession &s,
                                uint16_t interval, uint16_t window,
                                uint8_t own_mac_type)
{
    const uint8_t scan_type = 0x01;
    const uint8_t filter_type = 0;
    const uint8_t filter_policy = 0x00;
    const uint8_t filter_dup = 0x01;
    
    if( !s.isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return false;
    }
    int err = hci_le_set_scan_parameters(s.dd(), scan_type,
                        cpu_to_le(interval), cpu_to_le(window),
                        own_mac_type, filter_policy, to_send_req_poll_ms);
    if (err < 0) {
        perror("Set scan parameters failed");
        return false;
    }

    err = hci_le_set_scan_enable(s.dd(), 0x01, filter_dup, to_send_req_poll_ms);
    if (err < 0) {
        perror("Start scan failed");
        return false;
    }
    return true;
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
    return res;
}

void HCIAdapter::addDevice(std::shared_ptr<HCIDevice> const &device) {
    if( 0 > findDevice(device->mac) ) {
        discoveredDevices.push_back(device);
    }
}

int HCIAdapter::findDevice(EUI48 const & mac) const {
    auto begin = discoveredDevices.begin();
    auto it = std::find_if(begin, discoveredDevices.end(), [&](std::shared_ptr<HCIDevice> const& p) {
        return p->mac == mac;
    });
    if ( it == std::end(discoveredDevices) ) {
        return -1;
    } else {
        return std::distance(begin, it);
    }
}

uint16_t HCIAdapter::le_connect(HCISession &s, EUI48 const & peer_mac,
        uint16_t interval, uint16_t window,
        uint16_t min_interval, uint16_t max_interval,
        uint16_t latency, uint16_t supervision_timeout,
        uint16_t min_ce_length, uint16_t max_ce_length,
        uint8_t initiator_filter,
        uint8_t peer_mac_type,
        uint8_t own_mac_type )
{
    if( !s.isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return 0;
    }
    uint16_t handle = 0;
    bdaddr_t bdmac;
    memcpy(&bdmac, const_eui48_to_const_bdaddr_ptr(&peer_mac), sizeof(bdaddr_t));

    int err = hci_le_create_conn(s.dd(),
                cpu_to_le(interval), cpu_to_le(window),
                initiator_filter, peer_mac_type, bdmac, own_mac_type,
                cpu_to_le(min_interval), cpu_to_le(max_interval),
                cpu_to_le(latency), cpu_to_le(supervision_timeout),
                cpu_to_le(min_ce_length), cpu_to_le(max_ce_length),
                &handle, to_connect_ms);
    if (err < 0) {
        perror("Could not create connection");
        return 0;
    }
    return handle;
}

std::string HCIAdapter::toString() const {
    std::string out("Adapter["+getAddressString()+", "+getName()+", id="+std::to_string(dev_id)+"]");
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

int HCIAdapter::discoverDevices(HCISession& session,
                                const int waitForDeviceCount,
                                const EUI48 &waitForDevice,
                                const int timeoutMS,
                                const uint32_t ad_type_req)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    struct hci_filter nf, of;
    socklen_t olen;
    int bytes_left = -1;
    const int64_t t0 = getCurrentMilliseconds();

    if( !session.isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return false;
    }

    olen = sizeof(of);
    if (getsockopt(session.dd(), SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
        fprintf(stderr, "Could not get socket options\n");
        return false;
    }

    hci_filter_clear(&nf);
    hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
    hci_filter_set_event(EVT_LE_META_EVENT, &nf);

    if (setsockopt(session.dd(), SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
        fprintf(stderr, "Could not set socket options\n");
        return false;
    }

    const uint32_t ad_req = static_cast<uint32_t>(EInfoReport::Element::BDADDR) |
                            static_cast<uint32_t>(EInfoReport::Element::RSSI) |
                            ad_type_req;
    bool done = false;
    int64_t t1;
    int matchedDeviceCount = 0;

    while ( !done && ( ( t1 = getCurrentMilliseconds() ) - t0 ) < timeoutMS ) {
        uint8_t hci_type;
        hci_event_hdr *ehdr;
        evt_le_meta_event *meta;

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
                goto done; // timeout: exit but no error
            }
        }

        while ((bytes_left = read(session.dd(), buf, sizeof(buf))) < 0) {
            if (errno == EAGAIN || errno == EINTR ) {
                // cont temp unavail or interruption
                continue;
            }
            goto errout;
        }

        if( bytes_left < HCI_TYPE_LEN + HCI_EVENT_HDR_SIZE + EVT_LE_META_EVENT_SIZE ) {
            // not enough data ..
            continue;
        }
        hci_type = buf[0]; // sizeof HCI_TYPE_LEN

        ehdr = (hci_event_hdr*)(void*) ( buf + HCI_TYPE_LEN ); // sizeof HCI_EVENT_HDR_SIZE

        bytes_left -= (HCI_TYPE_LEN + HCI_EVENT_HDR_SIZE);
        meta = (evt_le_meta_event*)(void *) ( buf + ( HCI_TYPE_LEN + HCI_EVENT_HDR_SIZE ) ); // sizeof EVT_LE_META_EVENT_SIZE

        if( bytes_left < ehdr->plen ) {
            // not enough data ..
            fprintf(stderr, "HCIAdapter::discovery: Warning: Incomplete type 0x%.2X, event 0x%.2X, subevent 0x%.2X, remaining %d bytes < plen %d!\n",
                    hci_type, ehdr->evt, meta->subevent, bytes_left, ehdr->plen);
            continue;
        } else {
            DBG_PRINT("HCIAdapter::discovery: Complete type 0x%.2X, event 0x%.2X, subevent 0x%.2X, remaining %d bytes >= plen %d\n",
                    hci_type, ehdr->evt, meta->subevent, bytes_left, ehdr->plen);
        }

        // HCI_LE_Advertising_Report == 0x3E == EVT_LE_META_EVENT
        //        0x3E                                                           0x02
        if ( HCI_Event_Types::LE_Advertising_Report != ehdr->evt || meta->subevent != EVT_LE_ADVERTISING_REPORT ) {
            continue; // next ..
        }
        bytes_left -= EVT_LE_META_EVENT_SIZE;

        std::vector<std::shared_ptr<EInfoReport>> ad_reports = EInfoReport::read_ad_reports(meta->data, bytes_left);
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
            DBG_PRINT("AD Report %d/%d: matches %d, waitForDevice %s, ad_req %s, matchCount %d/%d, done %d\n",
                    i, num_reports, matches, waitForDevice.toString().c_str(), EInfoReport::dataSetToString(ad_req).c_str(),
                    matchedDeviceCount, waitForDeviceCount, done);
            DBG_PRINT("AD Report %d/%d: %s\n", i, num_reports, ad_report->toString().c_str());

            int idx = findDevice(ad_report->getAddress());
            std::shared_ptr<HCIDevice> dev;
            if( 0 > idx ) {
                if( matches ) {
                    // only add new devices if matching
                    dev = std::shared_ptr<HCIDevice>(new HCIDevice(*ad_report));
                    addDevice(dev);
                    if( nullptr != deviceDiscoveryListener ) {
                        deviceDiscoveryListener->deviceAdded(*this, dev);
                    }
                } else {
                    dev = nullptr;
                }
            } else {
                // always update
                dev = getDevice(idx);
                dev->update(*ad_report);
                if( nullptr != deviceDiscoveryListener ) {
                    deviceDiscoveryListener->deviceUpdated(*this, dev);
                }
            }
        }

    }
done:
    setsockopt(session.dd(), SOL_HCI, HCI_FILTER, &of, sizeof(of));
    return matchedDeviceCount;

errout:
    setsockopt(session.dd(), SOL_HCI, HCI_FILTER, &of, sizeof(of));
    return -1;
}
