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

extern "C" {
    #include <bluetooth/bluetooth.h>
    #include <bluetooth/hci.h>
    #include <bluetooth/hci_lib.h>
}

#include "HCITypes.hpp"

using namespace tinyb_hci;

static inline const bdaddr_t* const_eui48_to_const_bdaddr_ptr(const EUI48 *p) {
    return static_cast<const bdaddr_t *>( static_cast<void *>( const_cast<EUI48 *>( p ) ) );
}

// *************************************************
// *************************************************
// *************************************************

HCIDevice::HCIDevice(HCIAdapter const & a, EInfoReport const & r)
: adapter(a), ts_creation(r.getTimestamp()), mac(r.getAddress())
{
    if( !r.isSet(EInfoReport::Element::BDADDR) ) {
        throw IllegalArgumentException("HCIDevice ctor: Address not set: "+r.toString(), E_FILE_LINE);
    }
    update(r);
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
        if( !name.length() || data.getName().length() > name.length() ) {
            name = data.getName();
        }
    }
    if( data.isSet(EInfoReport::Element::NAME_SHORT) ) {
        if( !name.length() ) {
            name = data.getShortName();
        }
    }
    if( data.isSet(EInfoReport::Element::RSSI) ) {
        rssi = data.getRSSI();
    }
    if( data.isSet(EInfoReport::Element::TX_POWER) ) {
        tx_power = data.getTxPower();
    }
    if( data.isSet(EInfoReport::Element::MANUF_DATA) ) {
        msd = data.getManufactureSpecificData();
    }
    addServices(data.getServices());
}

uint16_t HCIDevice::le_connect(HCISession &s,
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
    memcpy(&bdmac, const_eui48_to_const_bdaddr_ptr(&mac), sizeof(bdaddr_t));

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

// *************************************************
// *************************************************
// *************************************************

