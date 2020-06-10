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

#include "BasicAlgos.hpp"

#include "BTIoctl.hpp"
#include "HCIIoctl.hpp"
#include "HCIComm.hpp"

#include "DBTAdapter.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
    #include <poll.h>
}

using namespace direct_bt;

bool DBTAdapter::addConnectedDevice(const std::shared_ptr<DBTDevice> & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = connectedDevices.begin(); it != connectedDevices.end(); ++it) {
        if ( *device == **it ) {
            DBG_PRINT("DBTAdapter::addConnectedDevice: Device already connected: %s", device->toString().c_str());
            return false;
        }
    }
    connectedDevices.push_back(device);
    DBG_PRINT("DBTAdapter::addConnectedDevice: Device connected: %s", device->toString().c_str());
    return true;
}

bool DBTAdapter::removeConnectedDevice(const DBTDevice & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = connectedDevices.begin(); it != connectedDevices.end(); ) {
        if ( &device == (*it).get() ) { // compare actual device address
            it = connectedDevices.erase(it);
            DBG_PRINT("DBTAdapter::removeConnectedDevice: Device disconnected: %s", device.toString().c_str());
            return true;
        } else {
            ++it;
        }
    }
    DBG_PRINT("DBTAdapter::removeConnectedDevice: Device not connected: %s", device.toString().c_str());
    return false;
}

int DBTAdapter::disconnectAllDevices(const HCIStatusCode reason) {
    std::vector<std::shared_ptr<DBTDevice>> devices(connectedDevices); // copy!
    const int count = devices.size();
    for (auto it = devices.begin(); it != devices.end(); ++it) {
        (*it)->disconnect(reason); // will erase device from list via removeConnectedDevice(..) above
    }
    return count;
}

std::shared_ptr<DBTDevice> DBTAdapter::findConnectedDevice (EUI48 const & mac) const {
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


// *************************************************
// *************************************************
// *************************************************

bool DBTAdapter::validateDevInfo() {
    currentScanType = ScanType::SCAN_TYPE_NONE;
    keepDiscoveringAlive = false;

    if( !mgmt.isOpen() || 0 > dev_id ) {
        return false;
    }

    adapterInfo = mgmt.getAdapterInfo(dev_id);
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DISCOVERING, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceDiscoveringCB));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::NEW_SETTINGS, bindMemberFunc(this, &DBTAdapter::mgmtEvNewSettingsCB));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::LOCAL_NAME_CHANGED, bindMemberFunc(this, &DBTAdapter::mgmtEvLocalNameChangedCB));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_CONNECTED, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceConnectedCB));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_DISCONNECTED, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceDisconnectedCB));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_FOUND, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceFoundCB));

    return true;
}

DBTAdapter::DBTAdapter()
: mgmt(DBTManager::get(BTMode::BT_MODE_LE)), btMode(mgmt.getBTMode()), dev_id(nullptr != mgmt.getDefaultAdapterInfo() ? 0 : -1)
{
    valid = validateDevInfo();
}

DBTAdapter::DBTAdapter(EUI48 &mac) 
: mgmt(DBTManager::get(BTMode::BT_MODE_LE)), btMode(mgmt.getBTMode()), dev_id(mgmt.findAdapterInfoIdx(mac))
{
    valid = validateDevInfo();
}

DBTAdapter::DBTAdapter(const int dev_id) 
: mgmt(DBTManager::get(BTMode::BT_MODE_LE)), btMode(mgmt.getBTMode()), dev_id(dev_id)
{
    valid = validateDevInfo();
}

DBTAdapter::~DBTAdapter() {
    DBG_PRINT("DBTAdapter::dtor: ... %p %s", this, toString().c_str());
    keepDiscoveringAlive = false;
    {
        int count = mgmt.removeMgmtEventCallback(dev_id);
        DBG_PRINT("DBTAdapter removeMgmtEventCallback(DISCOVERING): %d callbacks", count);
        (void)count;
    }
    statusListenerList.clear();

    stopDiscovery();
    disconnectAllDevices();
    closeHCI();
    removeDiscoveredDevices();
    sharedDevices.clear();

    DBG_PRINT("DBTAdapter::dtor: XXX");
}

std::shared_ptr<NameAndShortName> DBTAdapter::setLocalName(const std::string &name, const std::string &short_name) {
    return mgmt.setLocalName(dev_id, name, short_name);
}

void DBTAdapter::setPowered(bool value) {
    mgmt.setMode(dev_id, MgmtOpcode::SET_POWERED, value ? 1 : 0);
}

void DBTAdapter::setDiscoverable(bool value) {
    mgmt.setMode(dev_id, MgmtOpcode::SET_DISCOVERABLE, value ? 1 : 0);
}

void DBTAdapter::setBondable(bool value) {
    mgmt.setMode(dev_id, MgmtOpcode::SET_BONDABLE, value ? 1 : 0);
}

std::shared_ptr<HCIHandler> DBTAdapter::openHCI()
{
    const std::lock_guard<std::recursive_mutex> lock(mtx_hci); // RAII-style acquire and relinquish via destructor

    if( !valid ) {
        return nullptr;
    }
    if( nullptr != hci ) {
        if( hci->isOpen() ) {
            DBG_PRINT("DBTAdapter::openHCI: Already open");
            return hci;
        }
        hci = nullptr;
    }
    HCIHandler *s = new HCIHandler(btMode, dev_id, HCIHandler::Defaults::HCI_COMMAND_REPLY_TIMEOUT);
    if( !s->isOpen() ) {
        delete s;
        ERR_PRINT("Could not open HCIHandler: %s of %s", s->toString().c_str(), toString().c_str());
        return nullptr;
    }
    hci = std::shared_ptr<HCIHandler>( s );
    return hci;
}

std::shared_ptr<HCIHandler> DBTAdapter::getHCI() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTAdapter*>(this)->mtx_hci); // RAII-style acquire and relinquish via destructor
    return hci;
}

bool DBTAdapter::closeHCI()
{
    const std::lock_guard<std::recursive_mutex> lock(mtx_hci); // RAII-style acquire and relinquish via destructor

    DBG_PRINT("DBTAdapter::closeHCI: ...");
    if( nullptr == hci || !hci->isOpen() ) {
        DBG_PRINT("DBTAdapter::closeHCI: Not open");
        return false;
    }
    disconnectAllDevices(); // FIXME ????
    hci->close();
    hci = nullptr;
    DBG_PRINT("DBTAdapter::closeHCI: XXX");
    return true;
}

bool DBTAdapter::isDeviceWhitelisted(const EUI48 &address) {
    return mgmt.isDeviceWhitelisted(dev_id, address);
}

bool DBTAdapter::addDeviceToWhitelist(const EUI48 &address, const BDAddressType address_type, const HCIWhitelistConnectType ctype,
                                      const uint16_t conn_interval_min, const uint16_t conn_interval_max,
                                      const uint16_t conn_latency, const uint16_t timeout) {
    if( mgmt.isDeviceWhitelisted(dev_id, address) ) {
        ERR_PRINT("DBTAdapter::addDeviceToWhitelist: device already listed: dev_id %d, address %s", dev_id, address.toString().c_str());
        return false;
    }

    if( !mgmt.uploadConnParam(dev_id, address, address_type, conn_interval_min, conn_interval_max, conn_latency, timeout) ) {
        ERR_PRINT("DBTAdapter::addDeviceToWhitelist: uploadConnParam(dev_id %d, address %s, interval[%u..%u], latency %u, timeout %u): Failed",
                dev_id, address.toString().c_str(), conn_interval_min, conn_interval_max, conn_latency, timeout);
    }
    return mgmt.addDeviceToWhitelist(dev_id, address, address_type, ctype);
}

bool DBTAdapter::removeDeviceFromWhitelist(const EUI48 &address, const BDAddressType address_type) {
    return mgmt.removeDeviceFromWhitelist(dev_id, address, address_type);
}

bool DBTAdapter::addStatusListener(std::shared_ptr<AdapterStatusListener> l) {
    if( nullptr == l ) {
        throw IllegalArgumentException("DBTAdapterStatusListener ref is null", E_FILE_LINE);
    }
    const std::lock_guard<std::recursive_mutex> lock(mtx_statusListenerList); // RAII-style acquire and relinquish via destructor
    for(auto it = statusListenerList.begin(); it != statusListenerList.end(); ) {
        if ( **it == *l ) {
            return false; // already included
        } else {
            ++it;
        }
    }
    statusListenerList.push_back(l);
    return true;
}

bool DBTAdapter::removeStatusListener(std::shared_ptr<AdapterStatusListener> l) {
    if( nullptr == l ) {
        throw IllegalArgumentException("DBTAdapterStatusListener ref is null", E_FILE_LINE);
    }
    const std::lock_guard<std::recursive_mutex> lock(mtx_statusListenerList); // RAII-style acquire and relinquish via destructor
    for(auto it = statusListenerList.begin(); it != statusListenerList.end(); ) {
        if ( **it == *l ) {
            it = statusListenerList.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

bool DBTAdapter::removeStatusListener(const AdapterStatusListener * l) {
    if( nullptr == l ) {
        throw IllegalArgumentException("DBTAdapterStatusListener ref is null", E_FILE_LINE);
    }
    const std::lock_guard<std::recursive_mutex> lock(mtx_statusListenerList); // RAII-style acquire and relinquish via destructor
    for(auto it = statusListenerList.begin(); it != statusListenerList.end(); ) {
        if ( **it == *l ) {
            it = statusListenerList.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

int DBTAdapter::removeAllStatusListener() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_statusListenerList); // RAII-style acquire and relinquish via destructor
    int count = statusListenerList.size();
    statusListenerList.clear();
    return count;
}

bool DBTAdapter::startDiscovery(const bool keepAlive, const HCIAddressType own_mac_type,
                                const uint16_t le_scan_interval, const uint16_t le_scan_window)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx_discovery); // RAII-style acquire and relinquish via destructor
    if( ScanType::SCAN_TYPE_NONE != currentScanType ) {
        return true;
    }
    (void)own_mac_type;
    (void)le_scan_interval;
    (void)le_scan_window;

    DBG_PRINT("DBTAdapter::startDiscovery: keepAlive %d ...", keepAlive);
    removeDiscoveredDevices();
    keepDiscoveringAlive = keepAlive;
    currentScanType = mgmt.startDiscovery(dev_id);
    return ScanType::SCAN_TYPE_NONE != currentScanType;
}

void DBTAdapter::startDiscoveryBackground() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discovery); // RAII-style acquire and relinquish via destructor
    currentScanType = mgmt.startDiscovery(dev_id);
}

void DBTAdapter::stopDiscovery() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discovery); // RAII-style acquire and relinquish via destructor
    keepDiscoveringAlive = false;
    if( ScanType::SCAN_TYPE_NONE == currentScanType ) {
        return;
    }
    DBG_PRINT("DBTAdapter::stopDiscovery: ...");
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

std::shared_ptr<DBTDevice> DBTAdapter::findDiscoveredDevice (EUI48 const & mac) const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTAdapter*>(this)->mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    auto begin = discoveredDevices.begin();
    auto it = std::find_if(begin, discoveredDevices.end(), [&](std::shared_ptr<DBTDevice> const& p) {
        return p->address == mac;
    });
    if ( it == std::end(discoveredDevices) ) {
        return nullptr;
    } else {
        return *it;
    }
}

bool DBTAdapter::addDiscoveredDevice(std::shared_ptr<DBTDevice> const &device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = discoveredDevices.begin(); it != discoveredDevices.end(); ++it) {
        if ( *device == **it ) {
            // already discovered
            return false;
        }
    }
    discoveredDevices.push_back(device);
    return true;
}

int DBTAdapter::removeDiscoveredDevices() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    int res = discoveredDevices.size();
    discoveredDevices.clear();
    return res;
}

std::vector<std::shared_ptr<DBTDevice>> DBTAdapter::getDiscoveredDevices() const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTAdapter*>(this)->mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    std::vector<std::shared_ptr<DBTDevice>> res = discoveredDevices;
    return res;
}

bool DBTAdapter::addSharedDevice(std::shared_ptr<DBTDevice> const &device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_sharedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = sharedDevices.begin(); it != sharedDevices.end(); ++it) {
        if ( *device == **it ) {
            // already shared
            return false;
        }
    }
    sharedDevices.push_back(device);
    return true;
}

std::shared_ptr<DBTDevice> DBTAdapter::getSharedDevice(const DBTDevice & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_sharedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = sharedDevices.begin(); it != sharedDevices.end(); ++it) {
        if ( &device == (*it).get() ) { // compare actual device address
            return *it; // done
        }
    }
    return nullptr;
}

void DBTAdapter::releaseSharedDevice(const DBTDevice & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_sharedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = sharedDevices.begin(); it != sharedDevices.end(); ++it) {
        if ( &device == (*it).get() ) { // compare actual device address
            sharedDevices.erase(it);
            return; // unique set
        }
    }
}

std::shared_ptr<DBTDevice> DBTAdapter::findSharedDevice (EUI48 const & mac) const {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTAdapter*>(this)->mtx_sharedDevices); // RAII-style acquire and relinquish via destructor
    auto begin = sharedDevices.begin();
    auto it = std::find_if(begin, sharedDevices.end(), [&](std::shared_ptr<DBTDevice> const& p) {
        return p->address == mac;
    });
    if ( it == std::end(sharedDevices) ) {
        return nullptr;
    } else {
        return *it;
    }
}

std::string DBTAdapter::toString() const {
    std::string out("Adapter["+BTModeString(btMode)+", "+getAddressString()+", '"+getName()+"', id="+std::to_string(dev_id)+", "+javaObjectToString()+"]");
    std::vector<std::shared_ptr<DBTDevice>> devices = getDiscoveredDevices();
    if(devices.size() > 0 ) {
        out.append("\n");
        for(auto it = devices.begin(); it != devices.end(); it++) {
            std::shared_ptr<DBTDevice> p = *it;
            out.append("  ").append(p->toString()).append("\n");
        }
    }
    return out;
}

// *************************************************

bool DBTAdapter::mgmtEvDeviceDiscoveringCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventCB:DeviceDiscovering(dev_id %d, keepDiscoveringAlive %d): %s",
        dev_id, keepDiscoveringAlive.load(), e->toString().c_str());
    const MgmtEvtDiscovering &event = *static_cast<const MgmtEvtDiscovering *>(e.get());
    const bool enabled = event.getEnabled();
    if( !enabled && !keepDiscoveringAlive ) {
        // Only update currentScanType:=false IF keepAlive==false!
        currentScanType = ScanType::SCAN_TYPE_NONE;
    }
    int i=0;
    for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
        try {
            l->discoveringChanged(*this, enabled, keepDiscoveringAlive, event.getTimestamp());
        } catch (std::exception &e) {
            ERR_PRINT("DBTAdapter::EventCB:DeviceDiscovering-CBs %d/%zd: %s of %s: Caught exception %s",
                    i+1, statusListenerList.size(),
                    l->toString().c_str(), toString().c_str(), e.what());
        }
        i++;
    });
    if( keepDiscoveringAlive && !enabled ) {
        std::thread bg(&DBTAdapter::startDiscoveryBackground, this);
        bg.detach();
    }
    return true;
}

bool DBTAdapter::mgmtEvNewSettingsCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventCB:NewSettings: %s", e->toString().c_str());
    const MgmtEvtNewSettings &event = *static_cast<const MgmtEvtNewSettings *>(e.get());
    AdapterSetting old_setting = adapterInfo->getCurrentSetting();
    AdapterSetting changes = adapterInfo->setCurrentSetting(event.getSettings());
    DBG_PRINT("DBTAdapter::EventCB:NewSettings: %s -> %s, changes %s",
            getAdapterSettingsString(old_setting).c_str(),
            getAdapterSettingsString(adapterInfo->getCurrentSetting()).c_str(),
            getAdapterSettingsString(changes).c_str() );

    int i=0;
    for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
        try {
            l->adapterSettingsChanged(*this, old_setting, adapterInfo->getCurrentSetting(), changes, event.getTimestamp());
        } catch (std::exception &e) {
            ERR_PRINT("DBTAdapter::EventCB:NewSettings-CBs %d/%zd: %s of %s: Caught exception %s",
                    i+1, statusListenerList.size(),
                    l->toString().c_str(), toString().c_str(), e.what());
        }
        i++;
    });

    return true;
}

bool DBTAdapter::mgmtEvLocalNameChangedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventCB:LocalNameChanged: %s", e->toString().c_str());
    const MgmtEvtLocalNameChanged &event = *static_cast<const MgmtEvtLocalNameChanged *>(e.get());
    std::string old_name = localName.getName();
    std::string old_shortName = localName.getShortName();
    bool nameChanged = old_name != event.getName();
    bool shortNameChanged = old_shortName != event.getShortName();
    if( nameChanged ) {
        localName.setName(event.getName());
    }
    if( shortNameChanged ) {
        localName.setShortName(event.getShortName());
    }
    DBG_PRINT("DBTAdapter::EventCB:LocalNameChanged: Local name: %d: '%s' -> '%s'; short_name: %d: '%s' -> '%s'",
            nameChanged, old_name.c_str(), localName.getName().c_str(),
            shortNameChanged, old_shortName.c_str(), localName.getShortName().c_str());
    (void)nameChanged;
    (void)shortNameChanged;
    return true;
}

void DBTAdapter::sendDeviceUpdated(std::string cause, std::shared_ptr<DBTDevice> device, uint64_t timestamp, EIRDataType updateMask) {
    int i=0;
    for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
        try {
            if( l->matchDevice(*device) ) {
                l->deviceUpdated(device, updateMask, timestamp);
            }
        } catch (std::exception &e) {
            ERR_PRINT("DBTAdapter::sendDeviceUpdated-CBs (%s) %d/%zd: %s of %s: Caught exception %s",
                    cause.c_str(), i+1, statusListenerList.size(),
                    l->toString().c_str(), device->toString().c_str(), e.what());
        }
        i++;
    });
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
    int new_connect = 0;
    std::shared_ptr<DBTDevice> device = findConnectedDevice(event.getAddress());
    if( nullptr == device ) {
        device = findDiscoveredDevice(event.getAddress());
        new_connect = nullptr != device ? 1 : 0;
    }
    if( nullptr == device ) {
        device = findSharedDevice(event.getAddress());
        if( nullptr != device ) {
            addDiscoveredDevice(device);
            new_connect = 2;
        }
    }
    if( nullptr == device ) {
        // a whitelist auto-connect w/o previous discovery
        device = std::shared_ptr<DBTDevice>(new DBTDevice(*this, ad_report));
        addDiscoveredDevice(device);
        addSharedDevice(device);
        new_connect = 3;
    }

    EIRDataType updateMask = device->update(ad_report);
    if( addConnectedDevice(device) ) { // track device, if not done yet
        if( 0 == new_connect ) {
            new_connect = 4; // unknown reason...
        }
    }
    if( 0 < new_connect ) {
        device->ts_last_discovery = ad_report.getTimestamp();
    }
    DBG_PRINT("DBTAdapter::EventCB:DeviceConnected(dev_id %d, new_connect %d, updated %s): %s,\n    %s\n    -> %s",
        dev_id, new_connect, getEIRDataMaskString(updateMask).c_str(), event.toString().c_str(), ad_report.toString().c_str(), device->toString().c_str());
    if( EIRDataType::NONE != updateMask || 0 < new_connect ) {
        device->notifyConnected();
        int i=0;
        for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
            try {
                if( l->matchDevice(*device) ) {
                    if( EIRDataType::NONE != updateMask ) {
                        l->deviceUpdated(device, updateMask, ad_report.getTimestamp());
                    }
                    if( 0 < new_connect ) {
                        l->deviceConnected(device, event.getTimestamp());
                    }
                }
            } catch (std::exception &e) {
                ERR_PRINT("DBTAdapter::EventCB:DeviceConnected-CBs %d/%zd: %s of %s: Caught exception %s",
                        i+1, statusListenerList.size(),
                        l->toString().c_str(), device->toString().c_str(), e.what());
            }
            i++;
        });
    }
    return true;
}

void DBTAdapter::performDeviceConnected(std::shared_ptr<DBTDevice> device, uint64_t timestamp) {
    DBG_PRINT("DBTAdapter::performDeviceConnected(dev_id %d): %s", dev_id, device->toString().c_str());
    addConnectedDevice(device); // track it
    device->notifyConnected();
    int i=0;
    for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
        try {
            if( l->matchDevice(*device) ) {
                l->deviceConnected(device, timestamp);
            }
        } catch (std::exception &e) {
            ERR_PRINT("DBTAdapter::performDeviceConnected-CBs %d/%zd: %s of %s: Caught exception %s",
                    i+1, statusListenerList.size(),
                    l->toString().c_str(), device->toString().c_str(), e.what());
        }
        i++;
    });
}

bool DBTAdapter::mgmtEvDeviceDisconnectedCB(std::shared_ptr<MgmtEvent> e) {
    const MgmtEvtDeviceDisconnected &event = *static_cast<const MgmtEvtDeviceDisconnected *>(e.get());
    std::shared_ptr<DBTDevice> device = findConnectedDevice(event.getAddress());
    if( nullptr != device ) {
        DBG_PRINT("DBTAdapter::EventCB:DeviceDisconnected(dev_id %d): %s\n    -> %s",
            dev_id, event.toString().c_str(), device->toString().c_str());
        device->notifyDisconnected();
        removeConnectedDevice(*device);

        int i=0;
        for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
            try {
                if( l->matchDevice(*device) ) {
                    l->deviceDisconnected(device, event.getHCIReason(), event.getTimestamp());
                }
            } catch (std::exception &e) {
                ERR_PRINT("DBTAdapter::EventCB:DeviceDisconnected-CBs %d/%zd: %s of %s: Caught exception %s",
                        i+1, statusListenerList.size(),
                        l->toString().c_str(), device->toString().c_str(), e.what());
            }
            i++;
        });
    } else {
        DBG_PRINT("DBTAdapter::EventCB:DeviceDisconnected(dev_id %d): %s\n    -> Device not tracked",
            dev_id, event.toString().c_str());
    }
    return true;
}

bool DBTAdapter::mgmtEvDeviceFoundCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventCB:DeviceFound(dev_id %d): %s", dev_id, e->toString().c_str());
    const MgmtEvtDeviceFound &deviceFoundEvent = *static_cast<const MgmtEvtDeviceFound *>(e.get());

    EInfoReport ad_report;
    ad_report.setSource(EInfoReport::Source::EIR);
    ad_report.setTimestamp(deviceFoundEvent.getTimestamp());
    // ad_report.setEvtType(0); ??
    ad_report.setAddressType(deviceFoundEvent.getAddressType());
    ad_report.setAddress( deviceFoundEvent.getAddress() );
    ad_report.setRSSI( deviceFoundEvent.getRSSI() );
    ad_report.read_data(deviceFoundEvent.getData(), deviceFoundEvent.getDataSize());

    std::shared_ptr<DBTDevice> dev = findDiscoveredDevice(ad_report.getAddress());
    if( nullptr != dev ) {
        //
        // existing device
        //
        EIRDataType updateMask = dev->update(ad_report);
        if( EIRDataType::NONE != updateMask ) {
            sendDeviceUpdated("DiscoveredDeviceFound", dev, ad_report.getTimestamp(), updateMask);
        }
        return true;
    }

    dev = findSharedDevice(ad_report.getAddress());
    if( nullptr != dev ) {
        //
        // active shared device, but flushed from discovered devices
        // - update device
        // - issue deviceFound, allowing receivers to recognize the re-discovered device
        // - issue deviceUpdate if data has changed, allowing receivers to act upon
        //
        EIRDataType updateMask = dev->update(ad_report);
        addDiscoveredDevice(dev); // re-add to discovered devices!
        dev->ts_last_discovery = ad_report.getTimestamp();

        int i=0;
        for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
            try {
                if( l->matchDevice(*dev) ) {
                    l->deviceFound(dev, ad_report.getTimestamp());
                }
            } catch (std::exception &e) {
                ERR_PRINT("DBTAdapter::EventCB:SharedDeviceFound-CBs %d/%zd: %s of %s: Caught exception %s",
                        i+1, statusListenerList.size(),
                        l->toString().c_str(), dev->toString().c_str(), e.what());
            }
            i++;
        });
        if( EIRDataType::NONE != updateMask ) {
            sendDeviceUpdated("SharedDeviceFound", dev, ad_report.getTimestamp(), updateMask);
        }
        return true;
    }

    //
    // new device
    //
    dev = std::shared_ptr<DBTDevice>(new DBTDevice(*this, ad_report));
    addDiscoveredDevice(dev);
    addSharedDevice(dev);

    int i=0;
    for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
        try {
            if( l->matchDevice(*dev) ) {
                l->deviceFound(dev, ad_report.getTimestamp());
            }
        } catch (std::exception &e) {
            ERR_PRINT("DBTAdapter::EventCB:NewDeviceFound-CBs %d/%zd: %s of %s: Caught exception %s",
                    i+1, statusListenerList.size(),
                    l->toString().c_str(), dev->toString().c_str(), e.what());
        }
        i++;
    });

    return true;
}
