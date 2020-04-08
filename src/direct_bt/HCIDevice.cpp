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

#include "HCIComm.hpp"
#include "HCITypes.hpp"

#include "dbt_debug.hpp"

using namespace direct_bt;

HCIDevice::HCIDevice(HCIAdapter const & a, EInfoReport const & r)
: adapter(a), ts_creation(r.getTimestamp()), mac(r.getAddress())
{
    if( !r.isSet(EInfoReport::Element::BDADDR) ) {
        throw IllegalArgumentException("HCIDevice ctor: Address not set: "+r.toString(), E_FILE_LINE);
    }
    update(r);
}

HCIDevice::~HCIDevice() {
    services.clear();
    msd = nullptr;
}

std::shared_ptr<HCIDevice> HCIDevice::getSharedInstance() const {
    const std::shared_ptr<HCIDevice> myself = adapter.findDiscoveredDevice(mac);
    if( nullptr == myself ) {
        throw InternalError("HCIDevice: Not present in HCIAdapter: "+toString(), E_FILE_LINE);
    }
    return myself;
}

void HCIDevice::addService(std::shared_ptr<uuid_t> const &uuid)
{
    if( 0 > findService(uuid) ) {
        services.push_back(uuid);
    }
}
void HCIDevice::addServices(std::vector<std::shared_ptr<uuid_t>> const & services)
{
    for(size_t j=0; j<services.size(); j++) {
        const std::shared_ptr<uuid_t> uuid = services.at(j);
        addService(uuid);
    }
}

int HCIDevice::findService(std::shared_ptr<uuid_t> const &uuid) const
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

std::string HCIDevice::toString() const {
    const uint64_t t0 = getCurrentMilliseconds();
    std::string msdstr = nullptr != msd ? msd->toString() : "MSD[null]";
    std::string out("Device["+getAddressString()+", '"+getName()+
            "', age "+std::to_string(t0-ts_creation)+" ms, lup "+std::to_string(t0-ts_update)+" ms, rssi "+std::to_string(getRSSI())+
            ", tx-power "+std::to_string(tx_power)+", "+msdstr+"]");
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

uint16_t HCIDevice::le_connect(HCISession &session,
        uint8_t peer_mac_type, uint8_t own_mac_type,
        uint16_t interval, uint16_t window,
        uint16_t min_interval, uint16_t max_interval,
        uint16_t latency, uint16_t supervision_timeout,
        uint16_t min_ce_length, uint16_t max_ce_length,
        uint8_t initiator_filter )
{
    if( !session.isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return 0;
    }
    const uint16_t handle = session.hciComm.le_create_conn(
                mac, peer_mac_type, own_mac_type,
                interval, window, min_interval, max_interval, latency, supervision_timeout, min_ce_length, max_ce_length, initiator_filter);

    if (handle <= 0) {
        perror("Could not create connection");
        return 0;
    }
    session.connected(getSharedInstance());

    return handle;
}

// *************************************************
// *************************************************
// *************************************************

/**
  ServicesResolvedNotification
  D-Bus BlueZ 
  
  src/device.c:
    gatt_client_init
      gatt_client_ready_cb
        device_svc_resolved()
          device_set_svc_refreshed() 
        register_gatt_services()
        device_svc_resolved()

  src/shared/gatt-client.c
     bt_gatt_client_new()
       gatt_client_init(.., uint16_t mtu)
         discovery_op_create(client, 0x0001, 0xffff, init_complete, NULL);
         
         Setup MTU: BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part G] page 546: 4.3.1 Exchange MTU

         bt_gatt_discover_all_primary_services(...)

     discover_all
         bt_gatt_discover_all_primary_services(...)

  src/shared/gatt-helpers.c
    bt_gatt_discover_all_primary_services
      bt_gatt_discover_primary_services
        discover_services

    bt_gatt_discover_secondary_services
      discover_services

    discover_services
      shared/gatt-helpers.c line 831: discover all primary service!  
      Protocol Data Unit – PDU (2-257 octets)

        op = new0(struct bt_gatt_request, 1);
        op->att = att;
        op->start_handle = start;
        op->end_handle = end;
        op->callback = callback;
        op->user_data = user_data;
        op->destroy = destroy;
        // set service uuid to primary or secondary
        op->service_type = primary ? GATT_PRIM_SVC_UUID : GATT_SND_SVC_UUID;

        uint8_t pdu[6];

        put_le16(start, pdu);
        put_le16(end, pdu + 2);
        put_le16(op->service_type, pdu + 4);

        op->id = bt_att_send(att, BT_ATT_OP_READ_BY_GRP_TYPE_REQ,
                        pdu, sizeof(pdu),
                        read_by_grp_type_cb,
                        bt_gatt_request_ref(op),
                        async_req_unref);



 */
void ServicesResolvedNotification() {
}
