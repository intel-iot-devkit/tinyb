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
    isConnected = false;
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

std::string const DBTDevice::getName() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTDevice*>(this)->mtx_data); // RAII-style acquire and relinquish via destructor
    return name;
}

std::shared_ptr<ManufactureSpecificData> const DBTDevice::getManufactureSpecificData() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTDevice*>(this)->mtx_data); // RAII-style acquire and relinquish via destructor
    return msd;
}

std::vector<std::shared_ptr<uuid_t>> DBTDevice::getServices() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTDevice*>(this)->mtx_data); // RAII-style acquire and relinquish via destructor
    return services;
}

std::string DBTDevice::toString(bool includeDiscoveredServices) const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTDevice*>(this)->mtx_data); // RAII-style acquire and relinquish via destructor
    const uint64_t t0 = getCurrentMilliseconds();
    std::string msdstr = nullptr != msd ? msd->toString() : "MSD[null]";
    std::string out("Device[address["+getAddressString()+", "+getBDAddressTypeString(getAddressType())+"], name['"+name+
            "'], age "+std::to_string(t0-ts_creation)+" ms, lup "+std::to_string(t0-ts_update)+
            " ms, connected "+std::to_string(isConnected)+", rssi "+std::to_string(getRSSI())+
            ", tx-power "+std::to_string(tx_power)+
            ", appearance "+uint16HexString(static_cast<uint16_t>(appearance))+" ("+AppearanceCatToString(appearance)+
            "), "+msdstr+", "+javaObjectToString()+"]");
    if(includeDiscoveredServices && services.size() > 0 ) {
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
    const std::lock_guard<std::recursive_mutex> lock(mtx_data); // RAII-style acquire and relinquish via destructor

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
        if( 0 == name.length() || data.getName().length() > name.length() ) {
            name = data.getName();
            setEIRDataTypeSet(res, EIRDataType::NAME);
        }
    }
    if( data.isSet(EIRDataType::NAME_SHORT) ) {
        if( 0 == name.length() ) {
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

EIRDataType DBTDevice::update(GenericAccess const &data, const uint64_t timestamp) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_data); // RAII-style acquire and relinquish via destructor

    EIRDataType res = EIRDataType::NONE;
    ts_update = timestamp;
    if( 0 == name.length() || data.deviceName.length() > name.length() ) {
        name = data.deviceName;
        setEIRDataTypeSet(res, EIRDataType::NAME);
    }
    if( appearance != data.appearance ) {
        appearance = data.appearance;
        setEIRDataTypeSet(res, EIRDataType::APPEARANCE);
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
                adapter.sendDeviceUpdated("getConnectionInfo", sharedInstance, getCurrentMilliseconds(), updateMask);
            }
        }
    }
    return connInfo;
}

uint16_t DBTDevice::connectLE(HCIAddressType peer_mac_type, HCIAddressType own_mac_type,
        uint16_t le_scan_interval, uint16_t le_scan_window,
        uint16_t conn_interval_min, uint16_t conn_interval_max,
        uint16_t conn_latency, uint16_t supervision_timeout)
{
    if( 0 < hciConnHandle ) {
        ERR_PRINT("DBTDevice::connectLE: Already connected: %s", toString().c_str());
        return 0;
    }

    const std::lock_guard<std::recursive_mutex> lock(adapter.mtx_hci); // RAII-style acquire and relinquish via destructor
    std::shared_ptr<HCIComm> hciComm = adapter.openHCI();
    if( nullptr == hciComm || !hciComm->isOpen() ) {
        ERR_PRINT("DBTDevice::connectLE: Opening adapter's HCIComm failed: %s", toString().c_str());
        return 0;
    }
    if( !isLEAddressType() ) {
        ERR_PRINT("DBTDevice::connectLE: Not a BDADDR_LE_PUBLIC or BDADDR_LE_RANDOM address: %s", toString().c_str());
        return 0;
    }

    {
        // Currently doing nothing, but notifying
        DBTManager & mngr = adapter.getManager();
        mngr.create_connection(adapter.dev_id, address, addressType);
    }
    HCIErrorCode status = hciComm->le_create_conn(&hciConnHandle, address,
                                                  peer_mac_type, own_mac_type,
                                                  le_scan_interval, le_scan_window, conn_interval_min, conn_interval_max,
                                                  conn_latency, supervision_timeout);
#if 0
    if( HCIErrorCode::CONNECTION_ALREADY_EXISTS == status ) {
        INFO_PRINT("DBTDevice::connectLE: Connection already exists: status 0x%2.2X (%s) on %s",
                static_cast<uint8_t>(status), getHCIErrorCodeString(status).c_str(), toString().c_str());
        std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
        if( nullptr == sharedInstance ) {
            throw InternalError("DBTDevice::connectLE: Device unknown to adapter and not tracked: "+toString(), E_FILE_LINE);
        }
        adapter.performDeviceConnected(sharedInstance, getCurrentMilliseconds());
        return 0;
    }
#endif
    if( HCIErrorCode::COMMAND_DISALLOWED == status ) {
        WARN_PRINT("DBTDevice::connectLE: Could not yet create connection: status 0x%2.2X (%s), errno %d %s on %s",
                static_cast<uint8_t>(status), getHCIErrorCodeString(status).c_str(), errno, strerror(errno), toString().c_str());
        return 0;
    }
    if ( HCIErrorCode::SUCCESS != status ) {
        ERR_PRINT("DBTDevice::connectLE: Could not create connection: status 0x%2.2X (%s), errno %d %s on %s",
                static_cast<uint8_t>(status), getHCIErrorCodeString(status).c_str(), errno, strerror(errno), toString().c_str());
        return 0;
    }

    return hciConnHandle;
}

uint16_t DBTDevice::connectBREDR(const uint16_t pkt_type, const uint16_t clock_offset, const uint8_t role_switch)
{
    if( 0 < hciConnHandle ) {
        ERR_PRINT("DBTDevice::connectBREDR: Already connected: %s", toString().c_str());
        return 0;
    }
    const std::lock_guard<std::recursive_mutex> lock(adapter.mtx_hci); // RAII-style acquire and relinquish via destructor
    std::shared_ptr<HCIComm> hciComm = adapter.openHCI();
    if( nullptr == hciComm || !hciComm->isOpen() ) {
        ERR_PRINT("DBTDevice::connectBREDR: Opening adapter's HCIComm failed: %s", toString().c_str());
        return 0;
    }
    if( !isBREDRAddressType() ) {
        ERR_PRINT("DBTDevice::connectBREDR: Not a BDADDR_BREDR address: %s", toString().c_str());
        return 0;
    }

    {
        // Currently doing nothing, but notifying
        DBTManager & mngr = adapter.getManager();
        mngr.create_connection(adapter.dev_id, address, addressType);
    }

    HCIErrorCode status = hciComm->create_conn(&hciConnHandle, address, pkt_type, clock_offset, role_switch);
    if ( HCIErrorCode::SUCCESS != status ) {
        ERR_PRINT("DBTDevice::connectBREDR: Could not create connection: status 0x%2.2X (%s), errno %d %s on %s",
                static_cast<uint8_t>(status), getHCIErrorCodeString(status).c_str(), errno, strerror(errno), toString().c_str());
        return 0;
    }

    return hciConnHandle;
}

uint16_t DBTDevice::connectDefault()
{
    switch( addressType ) {
        case BDAddressType::BDADDR_LE_PUBLIC:
            return connectLE(HCIAddressType::HCIADDR_LE_PUBLIC);
        case BDAddressType::BDADDR_LE_RANDOM:
            return connectLE(HCIAddressType::HCIADDR_LE_RANDOM);
        case BDAddressType::BDADDR_BREDR:
            return connectBREDR();
        default:
            ERR_PRINT("DBTDevice::connectDefault: Not a valid address type: %s", toString().c_str());
            return 0;
    }
}

void DBTDevice::notifyConnected() {
    DBG_PRINT("DBTDevice::notifyConnected: %s", toString().c_str());
    isConnected = true;
}

void DBTDevice::notifyDisconnected() {
    DBG_PRINT("DBTDevice::notifyDisconnected: %s", toString().c_str());
    try {
        // coming from manager disconnect, ensure cleaning up!
        disconnect(true /* sentFromManager */, false /* ioErrorCause */);
    } catch (std::exception &e) {
        ERR_PRINT("Exception caught on %s: %s", toString().c_str(), e.what());
    }
    isConnected = false;
}

void DBTDevice::disconnect(const bool sentFromManager, const bool ioErrorCause, const HCIErrorCode reason) {
    DBG_PRINT("DBTDevice::disconnect: isConnected %d, sentFromManager %d, ioError %d, reason 0x%X (%s), gattHandler %d, hciConnHandle %d",
            isConnected.load(), sentFromManager, ioErrorCause, static_cast<uint8_t>(reason), getHCIErrorCodeString(reason).c_str(),
            (nullptr != gattHandler), (0 != hciConnHandle));
    disconnectGATT();

    const std::lock_guard<std::recursive_mutex> lock(adapter.mtx_hci); // RAII-style acquire and relinquish via destructor
    std::shared_ptr<HCIComm> hciComm = adapter.getHCI();

    if( !isConnected ) {
        DBG_PRINT("DBTDevice::disconnect: Skip disconnect: Not connected: %s", toString().c_str());
        goto exit;
    }
    isConnected = false;

    if( ioErrorCause ) {
        DBG_PRINT("DBTDevice::disconnect: Skip HCI disconnect: IO Error: %s", toString().c_str());
        goto skip_hci_disconnect;
    }

    if( 0 == hciConnHandle ) {
        DBG_PRINT("DBTDevice::disconnect: Skip HCI disconnect: HCI not connected: %s", toString().c_str());
        goto skip_hci_disconnect;
    }

    if( nullptr == hciComm || !hciComm->isOpen() ) {
        DBG_PRINT("DBTDevice::disconnect: Skip HCI disconnect: HCI not Open: %s", toString().c_str());
        goto skip_hci_disconnect;
    }

    if( !hciComm->disconnect(hciConnHandle, reason) ) {
        DBG_PRINT("DBTDevice::disconnect: handle 0x%X, errno %d %s", hciConnHandle, errno, strerror(errno));
    }

skip_hci_disconnect:
    hciConnHandle = 0;

    if( !sentFromManager ) {
        // Also issue mngr.disconnect on non-HCI connect (whitelist),
        // which will also send the DISCONNECT event.
        DBTManager & mngr = adapter.getManager();
        mngr.disconnect(ioErrorCause, adapter.dev_id, address, addressType, reason);
    }

exit:
    adapter.removeConnectedDevice(*this);
}

void DBTDevice::remove() {
    disconnect();
    releaseSharedInstance();
}

std::shared_ptr<GATTHandler> DBTDevice::connectGATT(int replyTimeoutMS) {
    std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
    if( nullptr == sharedInstance ) {
        throw InternalError("DBTDevice::connectGATT: Device unknown to adapter and not tracked: "+toString(), E_FILE_LINE);
    }

    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    if( nullptr != gattHandler ) {
        if( gattHandler->isOpen() ) {
            return gattHandler;
        }
        gattHandler = nullptr;
    }
    gattHandler = std::shared_ptr<GATTHandler>(new GATTHandler(sharedInstance, replyTimeoutMS));
    if( !gattHandler->connect() ) {
        DBG_PRINT("DBTDevice::connectGATT: Connection failed");
        gattHandler = nullptr;
    }
    return gattHandler;
}

std::shared_ptr<GATTHandler> DBTDevice::getGATTHandler() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    return gattHandler;
}

std::vector<std::shared_ptr<GATTService>> DBTDevice::getGATTServices() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    if( nullptr == gattHandler || !gattHandler->isOpen() ) {
        connectGATT();
        if( nullptr == gattHandler || !gattHandler->isOpen() ) {
            ERR_PRINT("DBTDevice::getServices: connectGATT failed");
            return std::vector<std::shared_ptr<GATTService>>();
        }
    }
    std::vector<std::shared_ptr<GATTService>> & gattServices = gattHandler->getServices(); // reference of the GATTHandler's list
    if( gattServices.size() > 0 ) { // reuse previous discovery result
        return gattServices;
    }
    gattServices = gattHandler->discoverCompletePrimaryServices(); // same reference of the GATTHandler's list
    if( gattServices.size() == 0 ) { // nothing discovered
        return gattServices;
    }
    // discovery success, retrieve and parse GenericAccess
    gattGenericAccess = gattHandler->getGenericAccess(gattServices);
    if( nullptr != gattGenericAccess ) {
        const uint64_t ts = getCurrentMilliseconds();
        EIRDataType updateMask = update(*gattGenericAccess, ts);
        DBG_PRINT("DBTDevice::getGATTServices: updated %s:\n    %s\n    -> %s",
            eirDataMaskToString(updateMask).c_str(), gattGenericAccess->toString().c_str(), toString().c_str());
        if( EIRDataType::NONE != updateMask ) {
            std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
            if( nullptr == sharedInstance ) {
                ERR_PRINT("DBTDevice::getGATTServices: Device unknown to adapter and not tracked: %s", toString().c_str());
            } else {
                adapter.sendDeviceUpdated("getGATTServices", sharedInstance, ts, updateMask);
            }
        }
    }
    return gattServices;
}

std::shared_ptr<GenericAccess> DBTDevice::getGATTGenericAccess() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    return gattGenericAccess;
}

void DBTDevice::disconnectGATT() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    if( nullptr != gattHandler ) {
        DBG_PRINT("DBTDevice::disconnectGATT: Disconnecting...");
        // interrupt GATT's L2CAP ::connect(..), avoiding prolonged hang
        gattHandler->disconnect();
        gattHandler = nullptr;
    }
}
