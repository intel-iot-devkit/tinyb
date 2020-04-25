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

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "HCIComm.hpp"
#include "DBTTypes.hpp"

using namespace direct_bt;

#define USE_BT_MGMT 1

DBTDevice::DBTDevice(DBTAdapter const & a, EInfoReport const & r)
: adapter(a), ts_creation(r.getTimestamp()), address(r.getAddress()), addressType(r.getAddressType())
{
    if( !r.isSet(EInfoReport::Element::BDADDR) ) {
        throw IllegalArgumentException("HCIDevice ctor: Address not set: "+r.toString(), E_FILE_LINE);
    }
    update(r);
}

DBTDevice::~DBTDevice() {
    le_disconnect();
    services.clear();
    msd = nullptr;
}

std::shared_ptr<DBTDevice> DBTDevice::getSharedInstance() const {
    const std::shared_ptr<DBTDevice> myself = adapter.findDiscoveredDevice(address);
    if( nullptr == myself ) {
        throw InternalError("HCIDevice: Not present in HCIAdapter: "+toString(), E_FILE_LINE);
    }
    return myself;
}

void DBTDevice::addService(std::shared_ptr<uuid_t> const &uuid)
{
    if( 0 > findService(uuid) ) {
        services.push_back(uuid);
    }
}
void DBTDevice::addServices(std::vector<std::shared_ptr<uuid_t>> const & services)
{
    for(size_t j=0; j<services.size(); j++) {
        const std::shared_ptr<uuid_t> uuid = services.at(j);
        addService(uuid);
    }
}

int DBTDevice::findService(std::shared_ptr<uuid_t> const &uuid) const
{
    auto begin = services.begin();
    auto it = std::find_if(begin, services.end(), [&](std::shared_ptr<uuid_t> const& p) {
        return *p == *uuid;
    });
    if ( it == std::end(services) ) {
        return -1;
    } else {
        return std::distance(begin, it);
    }
}

std::string DBTDevice::toString() const {
    const uint64_t t0 = getCurrentMilliseconds();
    std::string msdstr = nullptr != msd ? msd->toString() : "MSD[null]";
    std::string out("Device[address["+getAddressString()+", "+getBDAddressTypeString(getAddressType())+"], name['"+getName()+
            "'], age "+std::to_string(t0-ts_creation)+" ms, lup "+std::to_string(t0-ts_update)+" ms, rssi "+std::to_string(getRSSI())+
            ", tx-power "+std::to_string(tx_power)+", "+msdstr+", "+javaObjectToString()+"]");
    if(services.size() > 0 ) {
        out.append("\n");
        int i=0;
        for(auto it = services.begin(); it != services.end(); it++, i++) {
            if( 0 < i ) {
                out.append("\n");
            }
            std::shared_ptr<uuid_t> p = *it;
            out.append("  ").append(p->toUUID128String()).append(", ").append(std::to_string(static_cast<int>(p->getTypeSize()))).append(" bytes");
        }
    }
    return out;
}

void DBTDevice::update(EInfoReport const & data) {
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

uint16_t DBTDevice::le_connect(HCIAddressType peer_mac_type, HCIAddressType own_mac_type,
        uint16_t interval, uint16_t window,
        uint16_t min_interval, uint16_t max_interval,
        uint16_t latency, uint16_t supervision_timeout,
        uint16_t min_ce_length, uint16_t max_ce_length,
        uint8_t initiator_filter )
{
    if( 0 < leConnHandle ) {
        ERR_PRINT("DBTDevice::connect: Already connected");
        return 0;
    }
#ifdef USE_BT_MGMT

    DBTManager & mngr = adapter.getManager();
    mngr.create_connection(adapter.dev_id, address, addressType); // A NOP

#endif
    std::shared_ptr<HCISession> session = adapter.getOpenSession();
    if( nullptr == session || !session->isOpen() ) {
        ERR_PRINT("DBTDevice::connect: Not opened");
        return 0;
    }

    leConnHandle = session->hciComm.le_create_conn(
                        address, peer_mac_type, own_mac_type,
                        interval, window, min_interval, max_interval, latency, supervision_timeout,
                        min_ce_length, max_ce_length, initiator_filter);

    if ( 0 == leConnHandle ) {
        ERR_PRINT("DBTDevice::disconnect: Could not create connection: errno %d %s", errno, strerror(errno));
        return 0;
    }
    std::shared_ptr<DBTDevice> thisDevice = getSharedInstance();
    session->connectedLE(thisDevice);

    return leConnHandle;
}

void DBTDevice::le_disconnect(const uint8_t reason) {
    if( 0 == leConnHandle ) {
        DBG_PRINT("DBTDevice::disconnect: Not connected");
        return;
    }

    std::shared_ptr<HCISession> session = adapter.getOpenSession();
    if( nullptr == session || !session->isOpen() ) {
        DBG_PRINT("DBTDevice::disconnect: Not opened");
        return;
    }

    const uint16_t _leConnHandle = leConnHandle;
    leConnHandle = 0;
    if( !session->hciComm.le_disconnect(_leConnHandle, reason) ) {
        DBG_PRINT("DBTDevice::disconnect: handle 0x%X, errno %d %s", _leConnHandle, errno, strerror(errno));
    }

#ifdef USE_BT_MGMT
    DBTManager & mngr = adapter.getManager();
    mngr.disconnect(adapter.dev_id, address, addressType); // actual disconnect cmd
#endif

    std::shared_ptr<DBTDevice> thisDevice = getSharedInstance();
    session->disconnectedLE(thisDevice);
}

