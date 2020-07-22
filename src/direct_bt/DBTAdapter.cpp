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

int DBTAdapter::findDeviceIdx(std::vector<std::shared_ptr<DBTDevice>> & devices, EUI48 const & mac, const BDAddressType macType) {
    const size_t size = devices.size();
    for (size_t i = 0; i < size; i++) {
        std::shared_ptr<DBTDevice> & e = devices[i];
        if ( nullptr != e && mac == e->getAddress() && macType == e->getAddressType() ) {
            return i;
        }
    }
    return -1;
}

std::shared_ptr<DBTDevice> DBTAdapter::findDevice(std::vector<std::shared_ptr<DBTDevice>> & devices, EUI48 const & mac, const BDAddressType macType) {
    const size_t size = devices.size();
    for (size_t i = 0; i < size; i++) {
        std::shared_ptr<DBTDevice> & e = devices[i];
        if ( nullptr != e && mac == e->getAddress() && macType == e->getAddressType() ) {
            return e;
        }
    }
    return nullptr;
}

std::shared_ptr<DBTDevice> DBTAdapter::findDevice(std::vector<std::shared_ptr<DBTDevice>> & devices, DBTDevice const & device) {
    const size_t size = devices.size();
    for (size_t i = 0; i < size; i++) {
        std::shared_ptr<DBTDevice> & e = devices[i];
        if ( nullptr != e && device == *e ) {
            return e;
        }
    }
    return nullptr;
}

bool DBTAdapter::addConnectedDevice(const std::shared_ptr<DBTDevice> & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectedDevices); // RAII-style acquire and relinquish via destructor
    if( nullptr != findDevice(connectedDevices, *device) ) {
        return false;
    }
    connectedDevices.push_back(device);
    return true;
}

bool DBTAdapter::removeConnectedDevice(const DBTDevice & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = connectedDevices.begin(); it != connectedDevices.end(); ) {
        if ( nullptr != *it && device == **it ) {
            it = connectedDevices.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

int DBTAdapter::disconnectAllDevices(const HCIStatusCode reason) {
    std::vector<std::shared_ptr<DBTDevice>> devices;
    {
        const std::lock_guard<std::recursive_mutex> lock(mtx_connectedDevices); // RAII-style acquire and relinquish via destructor
        devices = connectedDevices; // copy!
    }
    const int count = devices.size();
    for (auto it = devices.begin(); it != devices.end(); ++it) {
        if( nullptr != *it ) {
            (*it)->disconnect(reason); // will erase device from list via removeConnectedDevice(..) above
        }
    }
    return count;
}

std::shared_ptr<DBTDevice> DBTAdapter::findConnectedDevice (EUI48 const & mac, const BDAddressType macType) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectedDevices); // RAII-style acquire and relinquish via destructor
    return findDevice(connectedDevices, mac, macType);
}


// *************************************************
// *************************************************
// *************************************************

bool DBTAdapter::openHCI()
{
    HCIHandler *s = new HCIHandler(btMode, dev_id, HCIHandler::Defaults::HCI_COMMAND_REPLY_TIMEOUT);
    if( !s->isOpen() ) {
        delete s;
        ERR_PRINT("Could not open HCIHandler: %s of %s", s->toString().c_str(), toString().c_str());
        return false;
    }
    hci = std::shared_ptr<HCIHandler>( s );
    return true;
}

bool DBTAdapter::closeHCI()
{
    DBG_PRINT("DBTAdapter::closeHCI: ...");
    if( nullptr == hci || !hci->isOpen() ) {
        DBG_PRINT("DBTAdapter::closeHCI: Not open");
        return false;
    }
    hci->close();
    hci = nullptr;
    DBG_PRINT("DBTAdapter::closeHCI: XXX");
    return true;
}

bool DBTAdapter::validateDevInfo() {
    currentScanType = ScanType::SCAN_TYPE_NONE;
    keepDiscoveringAlive = false;

    if( 0 > dev_id ) {
        ERR_PRINT("DBTAdapter::validateDevInfo: Invalid negative dev_id: %s", toString().c_str());
        return false;
    }
    if( !mgmt.isOpen() ) {
        ERR_PRINT("DBTAdapter::validateDevInfo: Manager not open: %s", toString().c_str());
        return false;
    }
    if( !openHCI() ) {
        ERR_PRINT("DBTAdapter::validateDevInfo: Opening adapter's HCI failed: %s", toString().c_str());
        return false;
    }

    adapterInfo = mgmt.getAdapterInfo(dev_id);
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DISCOVERING, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceDiscoveringMgmt));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::NEW_SETTINGS, bindMemberFunc(this, &DBTAdapter::mgmtEvNewSettingsMgmt));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::LOCAL_NAME_CHANGED, bindMemberFunc(this, &DBTAdapter::mgmtEvLocalNameChangedMgmt));
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_FOUND, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceFoundMgmt));

    hci->addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_CONNECTED, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceConnectedHCI));
    hci->addMgmtEventCallback(MgmtEvent::Opcode::CONNECT_FAILED, bindMemberFunc(this, &DBTAdapter::mgmtEvConnectFailedHCI));
    hci->addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_DISCONNECTED, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceDisconnectedHCI));

#ifdef VERBOSE_ON
    mgmt.addMgmtEventCallback(dev_id, MgmtEvent::Opcode::DEVICE_DISCONNECTED, bindMemberFunc(this, &DBTAdapter::mgmtEvDeviceDisconnectedMgmt));
#endif
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
    // mute all listener first
    {
        int count = mgmt.removeMgmtEventCallback(dev_id);
        DBG_PRINT("DBTAdapter removeMgmtEventCallback(DISCOVERING): %d callbacks", count);
        (void)count;
    }
    if( nullptr != hci ) {
        hci->clearAllMgmtEventCallbacks();
    }
    statusListenerList.clear();

    // Removes all device references from the lists: connectedDevices, discoveredDevices, sharedDevices
    stopDiscovery();
    disconnectAllDevices();
    closeHCI();
    removeDiscoveredDevices();
    sharedDevices.clear();

    DBG_PRINT("DBTAdapter::dtor: XXX");
}

void DBTAdapter::printSharedPtrListOfDevices() {
    const std::lock_guard<std::recursive_mutex> lock0(mtx_connectedDevices);
    const std::lock_guard<std::recursive_mutex> lock1(mtx_discoveredDevices);
    const std::lock_guard<std::recursive_mutex> lock2(mtx_sharedDevices);

    printSharedPtrList("SharedDevices", sharedDevices);
    printSharedPtrList("DiscoveredDevices", discoveredDevices);
    printSharedPtrList("ConnectedDevices", connectedDevices);
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

bool DBTAdapter::isDeviceWhitelisted(const EUI48 &address) {
    return mgmt.isDeviceWhitelisted(dev_id, address);
}

bool DBTAdapter::addDeviceToWhitelist(const EUI48 &address, const BDAddressType address_type, const HCIWhitelistConnectType ctype,
                                      const uint16_t conn_interval_min, const uint16_t conn_interval_max,
                                      const uint16_t conn_latency, const uint16_t timeout) {
    checkValid();
    if( mgmt.isDeviceWhitelisted(dev_id, address) ) {
        ERR_PRINT("DBTAdapter::addDeviceToWhitelist: device already listed: dev_id %d, address %s", dev_id, address.toString().c_str());
        return true;
    }

    if( !mgmt.uploadConnParam(dev_id, address, address_type, conn_interval_min, conn_interval_max, conn_latency, timeout) ) {
        ERR_PRINT("DBTAdapter::addDeviceToWhitelist: uploadConnParam(dev_id %d, address %s, interval[%u..%u], latency %u, timeout %u): Failed",
                dev_id, address.toString().c_str(), conn_interval_min, conn_interval_max, conn_latency, timeout);
    }
    return mgmt.addDeviceToWhitelist(dev_id, address, address_type, ctype);
}

bool DBTAdapter::removeDeviceFromWhitelist(const EUI48 &address, const BDAddressType address_type) {
    checkValid();
    return mgmt.removeDeviceFromWhitelist(dev_id, address, address_type);
}

bool DBTAdapter::addStatusListener(std::shared_ptr<AdapterStatusListener> l) {
    checkValid();
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
    checkValid();
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
    checkValid();
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
    checkValid();
    const std::lock_guard<std::recursive_mutex> lock(mtx_statusListenerList); // RAII-style acquire and relinquish via destructor
    int count = statusListenerList.size();
    statusListenerList.clear();
    return count;
}

bool DBTAdapter::startDiscovery(const bool keepAlive, const HCILEOwnAddressType own_mac_type,
                                const uint16_t le_scan_interval, const uint16_t le_scan_window)
{
    checkValid();
    const std::lock_guard<std::recursive_mutex> lock(mtx_discovery); // RAII-style acquire and relinquish via destructor
    if( ScanType::SCAN_TYPE_NONE != currentScanType ) {
        removeDiscoveredDevices();
        if( keepDiscoveringAlive == keepAlive ) {
            DBG_PRINT("DBTAdapter::startDiscovery: Already discovering, unchanged keepAlive %d -> %d ...", keepDiscoveringAlive.load(), keepAlive);
        } else {
            DBG_PRINT("DBTAdapter::startDiscovery: Already discovering, changed keepAlive %d -> %d ...", keepDiscoveringAlive.load(), keepAlive);
            keepDiscoveringAlive = keepAlive;
        }
        return true;
    }
    (void)own_mac_type;
    (void)le_scan_interval;
    (void)le_scan_window;

    removeDiscoveredDevices();
    DBG_PRINT("DBTAdapter::startDiscovery: Initiating discovery, keepAlive %d -> %d ...", keepDiscoveringAlive.load(), keepAlive);
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

std::shared_ptr<DBTDevice> DBTAdapter::findDiscoveredDevice (EUI48 const & mac, const BDAddressType macType) {
    const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTAdapter*>(this)->mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    return findDevice(discoveredDevices, mac, macType);
}

bool DBTAdapter::addDiscoveredDevice(std::shared_ptr<DBTDevice> const &device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    if( nullptr != findDevice(discoveredDevices, *device) ) {
        // already discovered
        return false;
    }
    discoveredDevices.push_back(device);
    return true;
}

bool DBTAdapter::removeDiscoveredDevice(const DBTDevice & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = discoveredDevices.begin(); it != discoveredDevices.end(); ) {
        if ( nullptr != *it && device == **it ) {
            it = discoveredDevices.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
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
    if( nullptr != findDevice(sharedDevices, *device) ) {
        // already shared
        return false;
    }
    sharedDevices.push_back(device);
    return true;
}

std::shared_ptr<DBTDevice> DBTAdapter::getSharedDevice(const DBTDevice & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_sharedDevices); // RAII-style acquire and relinquish via destructor
    return findDevice(sharedDevices, device);
}

void DBTAdapter::removeSharedDevice(const DBTDevice & device) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_sharedDevices); // RAII-style acquire and relinquish via destructor
    for (auto it = sharedDevices.begin(); it != sharedDevices.end(); ) {
        if ( nullptr != *it && device == **it ) {
            it = sharedDevices.erase(it);
            return; // unique set
        } else {
            ++it;
        }
    }
}

std::shared_ptr<DBTDevice> DBTAdapter::findSharedDevice (EUI48 const & mac, const BDAddressType macType) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_sharedDevices); // RAII-style acquire and relinquish via destructor
    return findDevice(sharedDevices, mac, macType);
}

std::string DBTAdapter::toString() const {
    std::string out("Adapter["+BTModeString(btMode)+", "+getAddressString()+", '"+getName()+"', id="+std::to_string(dev_id)+", "+javaObjectToString()+"]");
    std::vector<std::shared_ptr<DBTDevice>> devices = getDiscoveredDevices();
    if(devices.size() > 0 ) {
        out.append("\n");
        for(auto it = devices.begin(); it != devices.end(); it++) {
            std::shared_ptr<DBTDevice> p = *it;
            if( nullptr != p ) {
                out.append("  ").append(p->toString()).append("\n");
            }
        }
    }
    return out;
}

// *************************************************

bool DBTAdapter::mgmtEvDeviceDiscoveringMgmt(std::shared_ptr<MgmtEvent> e) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_discovery); // RAII-style acquire and relinquish via destructor
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

bool DBTAdapter::mgmtEvNewSettingsMgmt(std::shared_ptr<MgmtEvent> e) {
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

bool DBTAdapter::mgmtEvLocalNameChangedMgmt(std::shared_ptr<MgmtEvent> e) {
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

bool DBTAdapter::mgmtEvDeviceConnectedHCI(std::shared_ptr<MgmtEvent> e) {
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
    std::shared_ptr<DBTDevice> device = findConnectedDevice(event.getAddress(), event.getAddressType());
    if( nullptr == device ) {
        device = findDiscoveredDevice(event.getAddress(), event.getAddressType());
        new_connect = nullptr != device ? 1 : 0;
    }
    if( nullptr == device ) {
        device = findSharedDevice(event.getAddress(), event.getAddressType());
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
    if( 2 <= new_connect ) {
        device->ts_last_discovery = ad_report.getTimestamp();
    }
    if( 0 == new_connect ) {
        WARN_PRINT("DBTAdapter::EventHCI:DeviceConnected(dev_id %d, already connected, updated %s): %s,\n    %s\n    -> %s",
            dev_id, getEIRDataMaskString(updateMask).c_str(), event.toString().c_str(), ad_report.toString().c_str(), device->toString().c_str());
    } else {
        DBG_PRINT("DBTAdapter::EventHCI:DeviceConnected(dev_id %d, new_connect %d, updated %s): %s,\n    %s\n    -> %s",
            dev_id, new_connect, getEIRDataMaskString(updateMask).c_str(), event.toString().c_str(), ad_report.toString().c_str(), device->toString().c_str());
    }

    device->notifyConnected(event.getHCIHandle());

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
            ERR_PRINT("DBTAdapter::EventHCI:DeviceConnected-CBs %d/%zd: %s of %s: Caught exception %s",
                    i+1, statusListenerList.size(),
                    l->toString().c_str(), device->toString().c_str(), e.what());
        }
        i++;
    });
    return true;
}

bool DBTAdapter::mgmtEvConnectFailedHCI(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventHCI:ConnectFailed: %s", e->toString().c_str());
    const MgmtEvtDeviceConnectFailed &event = *static_cast<const MgmtEvtDeviceConnectFailed *>(e.get());
    std::shared_ptr<DBTDevice> device = findConnectedDevice(event.getAddress(), event.getAddressType());
    if( nullptr != device ) {
        DBG_PRINT("DBTAdapter::EventHCI:ConnectFailed(dev_id %d): %s\n    -> %s",
            dev_id, event.toString().c_str(), device->toString().c_str());
        device->notifyDisconnected();
        removeConnectedDevice(*device);

        int i=0;
        for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
            try {
                if( l->matchDevice(*device) ) {
                    l->deviceDisconnected(device, event.getHCIStatus(), event.getTimestamp());
                }
            } catch (std::exception &e) {
                ERR_PRINT("DBTAdapter::EventHCI:DeviceDisconnected-CBs %d/%zd: %s of %s: Caught exception %s",
                        i+1, statusListenerList.size(),
                        l->toString().c_str(), device->toString().c_str(), e.what());
            }
            i++;
        });
        removeDiscoveredDevice(*device); // ensure device will cause a deviceFound event after disconnect
    } else {
        DBG_PRINT("DBTAdapter::EventHCI:DeviceDisconnected(dev_id %d): %s\n    -> Device not tracked",
            dev_id, event.toString().c_str());
    }
    return true;
}

bool DBTAdapter::mgmtEvDeviceDisconnectedHCI(std::shared_ptr<MgmtEvent> e) {
    const MgmtEvtDeviceDisconnected &event = *static_cast<const MgmtEvtDeviceDisconnected *>(e.get());
    std::shared_ptr<DBTDevice> device = findConnectedDevice(event.getAddress(), event.getAddressType());
    if( nullptr != device ) {
        if( device->getConnectionHandle() != event.getHCIHandle() ) {
            INFO_PRINT("DBTAdapter::EventHCI:DeviceDisconnected(dev_id %d): ConnHandle mismatch %s\n    -> %s",
                dev_id, event.toString().c_str(), device->toString().c_str());
            return true;
        }
        DBG_PRINT("DBTAdapter::EventHCI:DeviceDisconnected(dev_id %d): %s\n    -> %s",
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
                ERR_PRINT("DBTAdapter::EventHCI:DeviceDisconnected-CBs %d/%zd: %s of %s: Caught exception %s",
                        i+1, statusListenerList.size(),
                        l->toString().c_str(), device->toString().c_str(), e.what());
            }
            i++;
        });
        removeDiscoveredDevice(*device); // ensure device will cause a deviceFound event after disconnect
    } else {
        INFO_PRINT("DBTAdapter::EventHCI:DeviceDisconnected(dev_id %d): %s\n    -> Device not tracked",
            dev_id, event.toString().c_str());
    }
    return true;
}

bool DBTAdapter::mgmtEvDeviceDisconnectedMgmt(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBTAdapter::EventCB:DeviceDisconnected: %s", e->toString().c_str());
    const MgmtEvtDeviceDisconnected &event = *static_cast<const MgmtEvtDeviceDisconnected *>(e.get());
    (void)event;
    return true;
}

bool DBTAdapter::mgmtEvDeviceFoundMgmt(std::shared_ptr<MgmtEvent> e) {
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

    // std::shared_ptr<DBTDevice> dev = findDiscoveredDevice(ad_report.getAddress());
    std::shared_ptr<DBTDevice> dev;
    {
        const std::lock_guard<std::recursive_mutex> lock(const_cast<DBTAdapter*>(this)->mtx_discoveredDevices); // RAII-style acquire and relinquish via destructor
        dev = findDiscoveredDevice(ad_report.getAddress(), ad_report.getAddressType());
    }
    if( nullptr != dev ) {
        //
        // existing device
        //
        EIRDataType updateMask = dev->update(ad_report);
        INFO_PRINT("DBTAdapter::EventCB:DeviceFound.1: Old Discovered %s", dev->getAddressString().c_str());
        if( EIRDataType::NONE != updateMask ) {
            sendDeviceUpdated("DiscoveredDeviceFound", dev, ad_report.getTimestamp(), updateMask);
        }
        return true;
    }

    dev = findSharedDevice(ad_report.getAddress(), ad_report.getAddressType());
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
        DBG_PRINT("DBTAdapter::EventCB:DeviceFound.2: Old Shared %s", dev->getAddressString().c_str());

        int i=0;
        for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
            try {
                if( l->matchDevice(*dev) ) {
                    l->deviceFound(dev, ad_report.getTimestamp());
                }
            } catch (std::exception &e) {
                ERR_PRINT("DBTAdapter::EventCB:DeviceFound-CBs %d/%zd: %s of %s: Caught exception %s",
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
    DBG_PRINT("DBTAdapter::EventCB:DeviceFound.3: All New %s", dev->getAddressString().c_str());

    int i=0;
    for_each_idx_mtx(mtx_statusListenerList, statusListenerList, [&](std::shared_ptr<AdapterStatusListener> &l) {
        try {
            if( l->matchDevice(*dev) ) {
                l->deviceFound(dev, ad_report.getTimestamp());
            }
        } catch (std::exception &e) {
            ERR_PRINT("DBTAdapter::EventCB:DeviceFound-CBs %d/%zd: %s of %s: Caught exception %s",
                    i+1, statusListenerList.size(),
                    l->toString().c_str(), dev->toString().c_str(), e.what());
        }
        i++;
    });

    return true;
}
