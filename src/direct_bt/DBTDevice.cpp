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
: adapter(a), ts_creation(r.getTimestamp()),
  address(r.getAddress()), addressType(r.getAddressType()),
  leRandomAddressType(address.getBLERandomAddressType(addressType))
{
    ts_last_discovery = ts_creation;
    hciConnHandle = 0;
    isConnected = false;
    isConnectIssued = false;
    if( !r.isSet(EIRDataType::BDADDR) ) {
        throw IllegalArgumentException("Address not set: "+r.toString(), E_FILE_LINE);
    }
    if( !r.isSet(EIRDataType::BDADDR_TYPE) ) {
        throw IllegalArgumentException("AddressType not set: "+r.toString(), E_FILE_LINE);
    }
    update(r);

    if( BDAddressType::BDADDR_LE_RANDOM == addressType ) {
        if( BLERandomAddressType::UNDEFINED == leRandomAddressType ) {
            throw IllegalArgumentException("BDADDR_LE_RANDOM: Invalid BLERandomAddressType "+
                    getBLERandomAddressTypeString(leRandomAddressType)+": "+toString(), E_FILE_LINE);
        }
    } else {
        if( BLERandomAddressType::UNDEFINED != leRandomAddressType ) {
            throw new IllegalArgumentException("Not BDADDR_LE_RANDOM: Invalid given native BLERandomAddressType "+
                    getBLERandomAddressTypeString(leRandomAddressType)+": "+toString(), E_FILE_LINE);
        }
    }
}

DBTDevice::~DBTDevice() {
    DBG_PRINT("DBTDevice::dtor: ... %p %s", this, getAddressString().c_str());
    remove();
    advServices.clear();
    advMSD = nullptr;
    DBG_PRINT("DBTDevice::dtor: XXX %p %s", this, getAddressString().c_str());
}

std::shared_ptr<DBTDevice> DBTDevice::getSharedInstance() const {
    return adapter.getSharedDevice(*this);
}
void DBTDevice::releaseSharedInstance() const {
    adapter.removeSharedDevice(*this);
}

bool DBTDevice::addAdvService(std::shared_ptr<uuid_t> const &uuid)
{
    if( 0 > findAdvService(uuid) ) {
        advServices.push_back(uuid);
        return true;
    }
    return false;
}
bool DBTDevice::addAdvServices(std::vector<std::shared_ptr<uuid_t>> const & services)
{
    bool res = false;
    for(size_t j=0; j<services.size(); j++) {
        const std::shared_ptr<uuid_t> uuid = services.at(j);
        res = addAdvService(uuid) || res;
    }
    return res;
}

int DBTDevice::findAdvService(std::shared_ptr<uuid_t> const &uuid) const
{
    const size_t size = advServices.size();
    for (size_t i = 0; i < size; i++) {
        const std::shared_ptr<uuid_t> & e = advServices[i];
        if ( nullptr != e && *uuid == *e ) {
            return i;
        }
    }
    return -1;
}

std::string const DBTDevice::getName() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTDevice*>(this)->mtx_data); // RAII-style acquire and relinquish via destructor
    return name;
}

std::shared_ptr<ManufactureSpecificData> const DBTDevice::getManufactureSpecificData() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTDevice*>(this)->mtx_data); // RAII-style acquire and relinquish via destructor
    return advMSD;
}

std::vector<std::shared_ptr<uuid_t>> DBTDevice::getAdvertisedServices() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTDevice*>(this)->mtx_data); // RAII-style acquire and relinquish via destructor
    return advServices;
}

std::string DBTDevice::toString(bool includeDiscoveredServices) const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTDevice*>(this)->mtx_data); // RAII-style acquire and relinquish via destructor
    const uint64_t t0 = getCurrentMilliseconds();
    std::string leaddrtype;
    if( BLERandomAddressType::UNDEFINED != leRandomAddressType ) {
        leaddrtype = ", random "+getBLERandomAddressTypeString(leRandomAddressType);
    }
    std::string msdstr = nullptr != advMSD ? advMSD->toString() : "MSD[null]";
    std::string out("Device[address["+getAddressString()+", "+getBDAddressTypeString(getAddressType())+leaddrtype+"], name['"+name+
            "'], age[total "+std::to_string(t0-ts_creation)+", ldisc "+std::to_string(t0-ts_last_discovery)+", lup "+std::to_string(t0-ts_last_update)+
            "]ms, connected["+std::to_string(isConnectIssued)+"/"+std::to_string(isConnected)+", "+uint16HexString(hciConnHandle)+"], rssi "+std::to_string(getRSSI())+
            ", tx-power "+std::to_string(tx_power)+
            ", appearance "+uint16HexString(static_cast<uint16_t>(appearance))+" ("+getAppearanceCatString(appearance)+
            "), "+msdstr+", "+javaObjectToString()+"]");
    if(includeDiscoveredServices && advServices.size() > 0 ) {
        out.append("\n");
        const size_t size = advServices.size();
        for (size_t i = 0; i < size; i++) {
            const std::shared_ptr<uuid_t> & e = advServices[i];
            if( 0 < i ) {
                out.append("\n");
            }
            out.append("  ").append(e->toUUID128String()).append(", ").append(std::to_string(static_cast<int>(e->getTypeSize()))).append(" bytes");
        }
    }
    return out;
}

EIRDataType DBTDevice::update(EInfoReport const & data) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_data); // RAII-style acquire and relinquish via destructor

    EIRDataType res = EIRDataType::NONE;
    ts_last_update = data.getTimestamp();
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
        if( advMSD != data.getManufactureSpecificData() ) {
            advMSD = data.getManufactureSpecificData();
            setEIRDataTypeSet(res, EIRDataType::MANUF_DATA);
        }
    }
    if( addAdvServices( data.getServices() ) ) {
        setEIRDataTypeSet(res, EIRDataType::SERVICE_UUID);
    }
    return res;
}

EIRDataType DBTDevice::update(GenericAccess const &data, const uint64_t timestamp) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_data); // RAII-style acquire and relinquish via destructor

    EIRDataType res = EIRDataType::NONE;
    ts_last_update = timestamp;
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

bool DBTDevice::connectLE(uint16_t le_scan_interval, uint16_t le_scan_window,
                          uint16_t conn_interval_min, uint16_t conn_interval_max,
                          uint16_t conn_latency, uint16_t supervision_timeout)
{
    const std::lock_guard<std::recursive_mutex> lock_conn(mtx_connect); // RAII-style acquire and relinquish via destructor

    HCILEOwnAddressType hci_own_mac_type;
    HCILEPeerAddressType hci_peer_mac_type;

    switch( addressType ) {
        case BDAddressType::BDADDR_LE_PUBLIC:
            hci_peer_mac_type = HCILEPeerAddressType::PUBLIC;
            hci_own_mac_type = HCILEOwnAddressType::PUBLIC;
            break;
        case BDAddressType::BDADDR_LE_RANDOM: {
                switch( leRandomAddressType ) {
                    case BLERandomAddressType::UNRESOLVABLE_PRIVAT:
                        hci_peer_mac_type = HCILEPeerAddressType::RANDOM;
                        hci_own_mac_type = HCILEOwnAddressType::RANDOM;
                        ERR_PRINT("LE Random address type '%s' not supported yet: %s",
                                getBLERandomAddressTypeString(leRandomAddressType).c_str(), toString().c_str());
                        return false;
                    case BLERandomAddressType::RESOLVABLE_PRIVAT:
                        hci_peer_mac_type = HCILEPeerAddressType::PUBLIC_IDENTITY;
                        hci_own_mac_type = HCILEOwnAddressType::RESOLVABLE_OR_PUBLIC;
                        ERR_PRINT("LE Random address type '%s' not supported yet: %s",
                                getBLERandomAddressTypeString(leRandomAddressType).c_str(), toString().c_str());
                        return false;
                    case BLERandomAddressType::STATIC_PUBLIC:
                        // FIXME: This only works for a static random address not changing at all,
                        // i.e. between power-cycles - hence a temporary hack.
                        // We need to use 'resolving list' and/or LE Set Privacy Mode (HCI) for all devices.
                        hci_peer_mac_type = HCILEPeerAddressType::RANDOM;
                        hci_own_mac_type = HCILEOwnAddressType::PUBLIC;
                        break;
                    default: {
                        ERR_PRINT("Can't connectLE to LE Random address type '%s': %s",
                                getBLERandomAddressTypeString(leRandomAddressType).c_str(), toString().c_str());
                        return false;
                    }
                }
            } break;
        default: {
                ERR_PRINT("Can't connectLE to address type '%s': %s", getBDAddressTypeString(addressType).c_str(), toString().c_str());
                return false;
            }
    }

    if( isConnected ) {
        ERR_PRINT("DBTDevice::connectLE: Already connected: %s", toString().c_str());
        return false;
    }

    std::shared_ptr<HCIHandler> hci = adapter.getHCI();
    if( nullptr == hci || !hci->isOpen() ) {
        ERR_PRINT("DBTDevice::connectLE: Adapter's HCI not open: %s", toString().c_str());
        return false;
    }

    HCIStatusCode status = hci->le_create_conn(address,
                                              hci_peer_mac_type, hci_own_mac_type,
                                              le_scan_interval, le_scan_window, conn_interval_min, conn_interval_max,
                                              conn_latency, supervision_timeout);
    isConnectIssued = true;
#if 0
    if( HCIStatusCode::CONNECTION_ALREADY_EXISTS == status ) {
        INFO_PRINT("DBTDevice::connectLE: Connection already exists: status 0x%2.2X (%s) on %s",
                static_cast<uint8_t>(status), getHCIStatusCodeString(status).c_str(), toString().c_str());
        std::shared_ptr<DBTDevice> sharedInstance = getSharedInstance();
        if( nullptr == sharedInstance ) {
            throw InternalError("DBTDevice::connectLE: Device unknown to adapter and not tracked: "+toString(), E_FILE_LINE);
        }
        adapter.performDeviceConnected(sharedInstance, getCurrentMilliseconds());
        return 0;
    }
#endif
    if( HCIStatusCode::COMMAND_DISALLOWED == status ) {
        WARN_PRINT("DBTDevice::connectLE: Could not yet create connection: status 0x%2.2X (%s), errno %d, hci-atype[peer %s, own %s] %s on %s",
                static_cast<uint8_t>(status), getHCIStatusCodeString(status).c_str(), errno, strerror(errno),
                getHCILEPeerAddressTypeString(hci_peer_mac_type).c_str(),
                getHCILEOwnAddressTypeString(hci_own_mac_type).c_str(),
                toString().c_str());
        return false;
    }
    if ( HCIStatusCode::SUCCESS != status ) {
        ERR_PRINT("DBTDevice::connectLE: Could not create connection: status 0x%2.2X (%s), errno %d %s, hci-atype[peer %s, own %s] on %s",
                static_cast<uint8_t>(status), getHCIStatusCodeString(status).c_str(), errno, strerror(errno),
                getHCILEPeerAddressTypeString(hci_peer_mac_type).c_str(),
                getHCILEOwnAddressTypeString(hci_own_mac_type).c_str(),
                toString().c_str());
        return false;
    }
    return true;
}

bool DBTDevice::connectBREDR(const uint16_t pkt_type, const uint16_t clock_offset, const uint8_t role_switch)
{
    const std::lock_guard<std::recursive_mutex> lock_conn(mtx_connect); // RAII-style acquire and relinquish via destructor

    if( isConnected ) {
        ERR_PRINT("DBTDevice::connectBREDR: Already connected: %s", toString().c_str());
        return false;
    }
    std::shared_ptr<HCIHandler> hci = adapter.getHCI();
    if( nullptr == hci || !hci->isOpen() ) {
        ERR_PRINT("DBTDevice::connectBREDR: Adapter's HCI not open: %s", toString().c_str());
        return false;
    }
    if( !isBREDRAddressType() ) {
        ERR_PRINT("DBTDevice::connectBREDR: Not a BDADDR_BREDR address: %s", toString().c_str());
        return false;
    }

    HCIStatusCode status = hci->create_conn(address, pkt_type, clock_offset, role_switch);
    isConnectIssued = true;
    if ( HCIStatusCode::SUCCESS != status ) {
        ERR_PRINT("DBTDevice::connectBREDR: Could not create connection: status 0x%2.2X (%s), errno %d %s on %s",
                static_cast<uint8_t>(status), getHCIStatusCodeString(status).c_str(), errno, strerror(errno), toString().c_str());
        return false;
    }
    return true;
}

bool DBTDevice::connectDefault()
{
    switch( addressType ) {
        case BDAddressType::BDADDR_LE_PUBLIC:
            /* fall through intended */
        case BDAddressType::BDADDR_LE_RANDOM:
            return connectLE();
        case BDAddressType::BDADDR_BREDR:
            return connectBREDR();
        default:
            ERR_PRINT("DBTDevice::connectDefault: Not a valid address type: %s", toString().c_str());
            return false;
    }
}

void DBTDevice::notifyConnected(const uint16_t handle) {
    DBG_PRINT("DBTDevice::notifyConnected: handle %s, %s", uint16HexString(handle).c_str(), toString().c_str());
    isConnected = true;
    hciConnHandle = handle;
}

void DBTDevice::notifyDisconnected() {
    DBG_PRINT("DBTDevice::notifyDisconnected: %s", toString().c_str());
    try {
        // coming from disconnect callback, ensure cleaning up!
        disconnect(true /* fromDisconnectCB */, false /* ioErrorCause */);
    } catch (std::exception &e) {
        ERR_PRINT("Exception caught on %s: %s", toString().c_str(), e.what());
    }
    isConnected = false;
    hciConnHandle = 0;
}

bool DBTDevice::disconnect(const bool fromDisconnectCB, const bool ioErrorCause, const HCIStatusCode reason) {
    const std::lock_guard<std::recursive_mutex> lock_conn(mtx_connect); // RAII-style acquire and relinquish via destructor

    INFO_PRINT("DBTDevice::disconnect: Start: isConnected %d/%d, fromDisconnectCB %d, ioError %d, reason 0x%X (%s), gattHandler %d, hciConnHandle %s",
            isConnectIssued.load(), isConnected.load(), fromDisconnectCB, ioErrorCause,
            static_cast<uint8_t>(reason), getHCIStatusCodeString(reason).c_str(),
            (nullptr != gattHandler), uint16HexString(hciConnHandle).c_str());
    disconnectGATT();

    bool res = false;

    std::shared_ptr<HCIHandler> hci = adapter.getHCI();

    if( !isConnected || !isConnectIssued ) {
        goto exit;
    }
    isConnectIssued = false;

    if( fromDisconnectCB || 0 == hciConnHandle ) {
        goto exit;
    }

    if( nullptr == hci || !hci->isOpen() ) {
        DBG_PRINT("DBTDevice::disconnect: Skip disconnect: HCI not Open: %s", toString().c_str());
        goto exit;
    }

    if( HCIStatusCode::SUCCESS != hci->disconnect(ioErrorCause, hciConnHandle.load(), address, addressType, reason) ) {
        DBG_PRINT("DBTDevice::disconnect: handle 0x%X, errno %d %s", hciConnHandle.load(), errno, strerror(errno));
    } else {
        res = true;
    }

exit:
    INFO_PRINT("DBTDevice::disconnect: End: isConnected %d/%d, fromDisconnectCB %d, ioError %d",
               isConnectIssued.load(), isConnected.load(), fromDisconnectCB, ioErrorCause);
    return res;
}

void DBTDevice::remove() {
    disconnect(false /* fromDisconnectCB */, false /* ioErrorCause */, HCIStatusCode::REMOTE_USER_TERMINATED_CONNECTION);
    adapter.removeConnectedDevice(*this); // usually done in DBTAdapter::mgmtEvDeviceDisconnectedHCI
    adapter.removeDiscoveredDevice(*this); // usually done in DBTAdapter::mgmtEvDeviceDisconnectedHCI
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

    if( !isConnected ) {
        ERR_PRINT("DBTDevice::connectGATT: Device not connected: %s", toString().c_str());
        return nullptr;
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
    try {
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
                getEIRDataMaskString(updateMask).c_str(), gattGenericAccess->toString().c_str(), toString().c_str());
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
    } catch (std::exception &e) {
        WARN_PRINT("DBTDevice::getGATTServices: Caught exception: '%s' on %s", e.what(), toString().c_str());
    }
    return std::vector<std::shared_ptr<GATTService>>();
}

std::shared_ptr<GATTService> DBTDevice::findGATTService(std::shared_ptr<uuid_t> const &uuid) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    const std::vector<std::shared_ptr<GATTService>> & gattServices = getGATTServices(); // reference of the GATTHandler's list
    const size_t size = gattServices.size();
    for (size_t i = 0; i < size; i++) {
        const std::shared_ptr<GATTService> & e = gattServices[i];
        if ( nullptr != e && *uuid == *(e->type) ) {
            return e;
        }
    }
    return nullptr;
}

bool DBTDevice::pingGATT() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    try {
        if( nullptr == gattHandler || !gattHandler->isOpen() ) {
            connectGATT();
            if( nullptr == gattHandler || !gattHandler->isOpen() ) {
                ERR_PRINT("DBTDevice::getServices: connectGATT failed");
                return false;
            }
        }
        std::vector<std::shared_ptr<GATTService>> & gattServices = gattHandler->getServices(); // reference of the GATTHandler's list
        if( gattServices.size() == 0 ) { // discover services
            gattServices = gattHandler->discoverCompletePrimaryServices(); // same reference of the GATTHandler's list
            if( gattServices.size() == 0 ) { // nothing discovered
                return false;
            }
        }
        return gattHandler->ping();
    } catch (std::exception &e) {
        INFO_PRINT("DBTDevice::pingGATT: Potential disconnect, exception: '%s' on %s", e.what(), toString().c_str());
    }
    return false;
}

std::shared_ptr<GenericAccess> DBTDevice::getGATTGenericAccess() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    return gattGenericAccess;
}

void DBTDevice::disconnectGATT() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_gatt); // RAII-style acquire and relinquish via destructor
    INFO_PRINT("DBTDevice::disconnectGATT: Start: gattHandle %d", (nullptr!=gattHandler));
    if( nullptr != gattHandler ) {
        // interrupt GATT's L2CAP ::connect(..), avoiding prolonged hang
        gattHandler->disconnect(false /* disconnectDevice */, false /* ioErrorCause */);
        gattHandler = nullptr;
    }
    INFO_PRINT("DBTDevice::disconnectGATT: End");
}
