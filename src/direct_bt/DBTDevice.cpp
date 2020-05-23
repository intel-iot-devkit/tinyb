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

#include "DBTDevice.hpp"
#include "DBTAdapter.hpp"

using namespace direct_bt;

DBTDevice::DBTDevice(DBTAdapter & a, EInfoReport const & r)
: adapter(a), ts_creation(r.getTimestamp()), address(r.getAddress()), addressType(r.getAddressType())
{
    if( !r.isSet(EIRDataType::BDADDR) ) {
        throw IllegalArgumentException("DBTDevice ctor: Address not set: "+r.toString(), E_FILE_LINE);
    }
    if( !r.isSet(EIRDataType::BDADDR_TYPE) ) {
        throw IllegalArgumentException("DBTDevice ctor: AddressType not set: "+r.toString(), E_FILE_LINE);
    }
    update(r);
}

DBTDevice::~DBTDevice() {
    DBG_PRINT("DBTDevice::dtor: ... %p %s", this, toString().c_str());
    remove();
    services.clear();
    msd = nullptr;
}

std::shared_ptr<DBTDevice> DBTDevice::getSharedInstance() const {
    return adapter.getSharedDevice(*this);
}
void DBTDevice::releaseSharedInstance() const {
    adapter.releaseSharedDevice(*this);
}

bool DBTDevice::addService(std::shared_ptr<uuid_t> const &uuid)
{
    if( 0 > findService(uuid) ) {
        services.push_back(uuid);
        return true;
    }
    return false;
}
bool DBTDevice::addServices(std::vector<std::shared_ptr<uuid_t>> const & services)
{
    bool res = false;
    for(size_t j=0; j<services.size(); j++) {
        const std::shared_ptr<uuid_t> uuid = services.at(j);
        res = addService(uuid) || res;
    }
    return res;
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
            ", tx-power "+std::to_string(tx_power)+", appearance "+uint16HexString(appearance)+", "+msdstr+", "+javaObjectToString()+"]");
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

EIRDataType DBTDevice::update(EInfoReport const & data) {
    EIRDataType res = EIRDataType::NONE;
    ts_update = data.getTimestamp();
    if( data.isSet(EIRDataType::BDADDR) ) {
        if( data.getAddress() != this->address ) {
            WARN_PRINT("DBTDevice::update:: BDADDR update not supported: %s for %s",
                    data.toString().c_str(), this->toString().c_str());
        }
    }
    if( data.isSet(EIRDataType::BDADDR_TYPE) ) {
        if( data.getAddressType() != this->addressType ) {
            WARN_PRINT("DBTDevice::update:: BDADDR_TYPE update not supported: %s for %s",
                    data.toString().c_str(), this->toString().c_str());
        }
    }
    if( data.isSet(EIRDataType::NAME) ) {
        if( !name.length() || data.getName().length() > name.length() ) {
            name = data.getName();
            setEIRDataTypeSet(res, EIRDataType::NAME);
        }
    }
    if( data.isSet(EIRDataType::NAME_SHORT) ) {
        if( !name.length() ) {
            name = data.getShortName();
            setEIRDataTypeSet(res, EIRDataType::NAME_SHORT);
        }
    }
    if( data.isSet(EIRDataType::RSSI) ) {
        if( rssi != data.getRSSI() ) {
            rssi = data.getRSSI();
            setEIRDataTypeSet(res, EIRDataType::RSSI);
        }
    }
    if( data.isSet(EIRDataType::TX_POWER) ) {
        if( tx_power != data.getTxPower() ) {
            tx_power = data.getTxPower();
            setEIRDataTypeSet(res, EIRDataType::TX_POWER);
        }
    }
    if( data.isSet(EIRDataType::APPEARANCE) ) {
        if( appearance != data.getAppearance() ) {
            appearance = data.getAppearance();
            setEIRDataTypeSet(res, EIRDataType::APPEARANCE);
        }
    }
    if( data.isSet(EIRDataType::MANUF_DATA) ) {
        if( msd != data.getManufactureSpecificData() ) {
            msd = data.getManufactureSpecificData();
            setEIRDataTypeSet(res, EIRDataType::MANUF_DATA);
        }
    }
    if( addServices( data.getServices() ) ) {
        setEIRDataTypeSet(res, EIRDataType::SERVICE_UUID);
    }
    return res;
}

std::shared_ptr<ConnectionInfo> DBTDevice::getConnectionInfo() {
    DBTManager & mgmt = adapter.getManager();
    std::shared_ptr<ConnectionInfo> connInfo = mgmt.getConnectionInfo(adapter.dev_id, address, addressType);
    if( nullptr != connInfo ) {
        EIRDataType updateMask = EIRDataType::NONE;
        if( rssi != connInfo->getRSSI() ) {
            rssi = connInfo->getRSSI();
            setEIRDataTypeSet(updateMask, EIRDataType::RSSI);
        }
        if( tx_power != connInfo->getTxPower() ) {
            tx_power = connInfo->getTxPower();
            setEIRDataTypeSet(updateMask, EIRDataType::TX_POWER);
        }
        if( EIRDataType::NONE != updateMask ) {
            std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
            if( nullptr == sharedInstance ) {
                ERR_PRINT("DBTDevice::getConnectionInfo: Device unknown to adapter and not tracked: %s", toString().c_str());
            } else {
                adapter.sendDeviceUpdated(sharedInstance, getCurrentMilliseconds(), updateMask);
            }
        }
    }
    return connInfo;
}

uint16_t DBTDevice::le_connectHCI(HCIAddressType peer_mac_type, HCIAddressType own_mac_type,
        uint16_t interval, uint16_t window,
        uint16_t min_interval, uint16_t max_interval,
        uint16_t latency, uint16_t supervision_timeout,
        uint16_t min_ce_length, uint16_t max_ce_length,
        uint8_t initiator_filter )
{
    if( 0 < hciConnHandle ) {
        ERR_PRINT("DBTDevice::le_connect: Already connected");
        return 0;
    }
    std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
    if( nullptr == sharedInstance ) {
        throw InternalError("DBTDevice::connectGATT: Device unknown to adapter and not tracked: "+toString(), E_FILE_LINE);
    }
    std::shared_ptr<HCIComm> hciComm = adapter.getOpenHCIComm();
    if( nullptr == hciComm || !hciComm->isOpen() ) {
        ERR_PRINT("DBTDevice::le_connect: Adapter session not opened");
        return 0;
    }
    if( !isLEAddressType() ) {
        ERR_PRINT("DBTDevice::le_connect: Not a BDADDR_LE_PUBLIC or BDADDR_LE_RANDOM address: %s", toString().c_str());
        return 0;
    }

    {
        // Currently doing nothing, but notifying
        DBTManager & mngr = adapter.getManager();
        mngr.create_connection(adapter.dev_id, address, addressType);
    }

    hciConnHandle = hciComm->le_create_conn(
                        address, peer_mac_type, own_mac_type,
                        interval, window, min_interval, max_interval, latency, supervision_timeout,
                        min_ce_length, max_ce_length, initiator_filter);

    if ( 0 == hciConnHandle ) {
        ERR_PRINT("DBTDevice::le_connect: Could not create connection");
        return 0;
    }
    adapter.addConnectedDevice(sharedInstance);

    return hciConnHandle;
}

uint16_t DBTDevice::connectHCI(const uint16_t pkt_type, const uint16_t clock_offset, const uint8_t role_switch)
{
    if( 0 < hciConnHandle ) {
        ERR_PRINT("DBTDevice::connect: Already connected");
        return 0;
    }
    std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
    if( nullptr == sharedInstance ) {
        throw InternalError("DBTDevice::connectGATT: Device unknown to adapter and not tracked: "+toString(), E_FILE_LINE);
    }
    std::shared_ptr<HCIComm> hciComm = adapter.getOpenHCIComm();
    if( nullptr == hciComm || !hciComm->isOpen() ) {
        ERR_PRINT("DBTDevice::le_connect: Adapter session not opened");
        return 0;
    }
    if( !isBREDRAddressType() ) {
        ERR_PRINT("DBTDevice::connect: Not a BDADDR_BREDR address: %s", toString().c_str());
        return 0;
    }

    {
        // Currently doing nothing, but notifying
        DBTManager & mngr = adapter.getManager();
        mngr.create_connection(adapter.dev_id, address, addressType);
    }

    hciConnHandle = hciComm->create_conn(address, pkt_type, clock_offset, role_switch);

    if ( 0 == hciConnHandle ) {
        ERR_PRINT("DBTDevice::connect: Could not create connection (yet)");
        return 0;
    }
    adapter.addConnectedDevice(sharedInstance);

    return hciConnHandle;
}

uint16_t DBTDevice::connectHCIDefault()
{
    switch( addressType ) {
        case BDAddressType::BDADDR_LE_PUBLIC:
            return le_connectHCI(HCIAddressType::HCIADDR_LE_PUBLIC);
        case BDAddressType::BDADDR_LE_RANDOM:
            return le_connectHCI(HCIAddressType::HCIADDR_LE_RANDOM);
        case BDAddressType::BDADDR_BREDR:
            return connectHCI();
        default:
            ERR_PRINT("DBTDevice::defaultConnect: Not a valid address type: %s", toString().c_str());
            return 0;
    }
}

void DBTDevice::disconnect(const uint8_t reason) {
    disconnectGATT();

    std::shared_ptr<HCIComm> hciComm = adapter.getOpenHCIComm();

    if( 0 == hciConnHandle ) {
        DBG_PRINT("DBTDevice::disconnect: Not connected");
        goto errout;
    }

    if( nullptr == hciComm || !hciComm->isOpen() ) {
        DBG_PRINT("DBTDevice::disconnect: Session not opened");
        goto errout;
    }

    {
        const uint16_t _connHandle = hciConnHandle;
        hciConnHandle = 0;
        if( !hciComm->disconnect(_connHandle, reason) ) {
            DBG_PRINT("DBTDevice::disconnect: handle 0x%X, errno %d %s", _connHandle, errno, strerror(errno));
        }
    }

    {
        // Actually issuing DISCONNECT post HCI
        DBTManager & mngr = adapter.getManager();
        mngr.disconnect(adapter.dev_id, address, addressType, reason);
    }

errout:
    adapter.removeConnectedDevice(*this);
}

void DBTDevice::remove() {
    disconnect(0);
    releaseSharedInstance();
}

std::shared_ptr<GATTHandler> DBTDevice::connectGATT(int timeoutMS) {
    std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
    if( nullptr == sharedInstance ) {
        throw InternalError("DBTDevice::connectGATT: Device unknown to adapter and not tracked: "+toString(), E_FILE_LINE);
    }

    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    if( nullptr != gattHandler ) {
        return gattHandler;
    }
    std::shared_ptr<GATTHandler> _gattHandler = std::shared_ptr<GATTHandler>(new GATTHandler(sharedInstance, timeoutMS));
    if( _gattHandler->connect() ) {
        gattHandler = _gattHandler;
    } else {
        DBG_PRINT("DBTDevice::connectGATT: Connection failed");
        _gattHandler = nullptr;
    }
    return gattHandler;
}

std::shared_ptr<GATTHandler> DBTDevice::getGATTHandler() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    return gattHandler;
}

std::vector<std::shared_ptr<GATTService>> DBTDevice::getServices() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
    if( nullptr == sharedInstance ) {
        ERR_PRINT("DBTDevice::getServices: Device unknown to adapter and not tracked: %s", toString().c_str());
        return std::vector<std::shared_ptr<GATTService>>();
    }
    if( nullptr == gattHandler ) {
        connectGATT();
        if( nullptr == gattHandler ) {
            ERR_PRINT("DBTDevice::getServices: connectGATT failed");
            return std::vector<std::shared_ptr<GATTService>>();
        }
    }
    return gattHandler->discoverCompletePrimaryServices();
}

void DBTDevice::disconnectGATT() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    if( nullptr != gattHandler ) {
        DBG_PRINT("DBTDevice::disconnectGATT: Disconnecting...");
        gattHandler->disconnect();
        gattHandler = nullptr;
    }
}
