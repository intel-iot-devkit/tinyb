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

#include <direct_bt/DirectBT.hpp>
#include <cinttypes>

extern "C" {
    #include <unistd.h>
}

using namespace direct_bt;

/***
 * This C++ direct_bt scanner code uses the high-level API like dbt_scanner00
 * and adds multithreading, i.e. one thread processes each found device found
 * as notified via the event listener.
 */

static int64_t timestamp_t0;

static bool USE_WHITELIST = false;

static EUI48 waitForDevice = EUI48_ANY_DEVICE;

static void connectDiscoveredDevice(std::shared_ptr<DBTDevice> device);

static void processConnectedDevice(std::shared_ptr<DBTDevice> device);

#include <pthread.h>

static std::vector<EUI48> devicesInProcessing;
static std::recursive_mutex mtx_devicesProcessing;

static std::recursive_mutex mtx_devicesProcessed;
static std::vector<EUI48> devicesProcessed;

static void addToDevicesProcessed(const EUI48 &a) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_devicesProcessed); // RAII-style acquire and relinquish via destructor
    devicesProcessed.push_back(a);
}
static bool isDeviceProcessed(const EUI48 & a) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_devicesProcessed); // RAII-style acquire and relinquish via destructor
    for (auto it = devicesProcessed.begin(); it != devicesProcessed.end(); ++it) {
        if ( a == *it ) {
            return true;
        }
    }
    return false;
}

static void addToDevicesProcessing(const EUI48 &a) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_devicesProcessing); // RAII-style acquire and relinquish via destructor
    devicesInProcessing.push_back(a);
}
static bool removeFromDevicesProcessing(const EUI48 &a) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_devicesProcessing); // RAII-style acquire and relinquish via destructor
    for (auto it = devicesInProcessing.begin(); it != devicesInProcessing.end(); ++it) {
        if ( a == *it ) {
            devicesInProcessing.erase(it);
            return true;
        }
    }
    return false;
}
static bool isDeviceProcessing(const EUI48 & a) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_devicesProcessing); // RAII-style acquire and relinquish via destructor
    for (auto it = devicesInProcessing.begin(); it != devicesInProcessing.end(); ++it) {
        if ( a == *it ) {
            return true;
        }
    }
    return false;
}

class MyAdapterStatusListener : public AdapterStatusListener {

    void adapterSettingsChanged(DBTAdapter const &a, const AdapterSetting oldmask, const AdapterSetting newmask,
                                const AdapterSetting changedmask, const uint64_t timestamp) override {
        fprintf(stderr, "****** SETTINGS_CHANGED: %s -> %s, changed %s\n",
                adapterSettingsToString(oldmask).c_str(),
                adapterSettingsToString(newmask).c_str(),
                adapterSettingsToString(changedmask).c_str());
        fprintf(stderr, "Status DBTAdapter:\n");
        fprintf(stderr, "%s\n", a.toString().c_str());
        (void)timestamp;
    }

    void discoveringChanged(DBTAdapter const &a, const bool enabled, const bool keepAlive, const uint64_t timestamp) override {
        fprintf(stderr, "****** DISCOVERING: enabled %d, keepAlive %d: %s\n", enabled, keepAlive, a.toString().c_str());
        (void)timestamp;
    }

    void deviceFound(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) override {
        (void)timestamp;

        if( BDAddressType::BDADDR_LE_PUBLIC != device->getAddressType()
            /** && BDAddressType::BDADDR_LE_RANDOM != device->getAddressType() */ ) {
            fprintf(stderr, "****** FOUND__-2: Skip non public LE %s\n", device->toString(true).c_str());
            return;
        }
        if( waitForDevice == EUI48_ANY_DEVICE ||
            ( waitForDevice == device->address &&
              !isDeviceProcessed(waitForDevice) &&
              !isDeviceProcessing(waitForDevice)
            ) )
        {
            fprintf(stderr, "****** FOUND__-0: Connecting %s\n", device->toString(true).c_str());
            std::thread dc(::connectDiscoveredDevice, device);
            dc.detach();
        } else {
            fprintf(stderr, "****** FOUND__-1: NOP %s\n", device->toString(true).c_str());
        }
    }

    void deviceUpdated(std::shared_ptr<DBTDevice> device, const uint64_t timestamp, const EIRDataType updateMask) override {
        fprintf(stderr, "****** UPDATED: %s of %s\n", eirDataMaskToString(updateMask).c_str(), device->toString(true).c_str());
        (void)timestamp;
    }

    void deviceConnectionChanged(std::shared_ptr<DBTDevice> device, const bool connected, const uint64_t timestamp) override {
        (void)timestamp;

        if( !connected ) {
            fprintf(stderr, "****** DISCONNECTED: %s\n", device->toString(true).c_str());
            return;
        }

        if( waitForDevice == EUI48_ANY_DEVICE ||
            ( waitForDevice == device->address &&
              !isDeviceProcessed(waitForDevice) &&
              !isDeviceProcessing(waitForDevice)
            ) )
        {
            fprintf(stderr, "****** CONNECTED-0: Processing %s\n", device->toString(true).c_str());
            addToDevicesProcessing(device->getAddress());
            std::thread dc(::processConnectedDevice, device);
            dc.detach();
        } else {
            fprintf(stderr, "****** CONNECTED-1: NOP %s\n", device->toString(true).c_str());
        }
    }

    std::string toString() const override {
        return "MyAdapterStatusListener[this "+aptrHexString(this)+"]";
    }
};

static const uuid16_t _TEMPERATURE_MEASUREMENT(GattCharacteristicType::TEMPERATURE_MEASUREMENT);

class MyGATTEventListener : public SpecificGATTCharacteristicListener {
  public:

    MyGATTEventListener(const GATTCharacteristic * characteristicMatch)
    : SpecificGATTCharacteristicListener(characteristicMatch) {}

    void notificationReceived(GATTCharacteristicRef charDecl, std::shared_ptr<TROOctets> charValue, const uint64_t timestamp) override {
        const std::shared_ptr<DBTDevice> dev = charDecl->getDevice();
        const int64_t tR = getCurrentMilliseconds();
        fprintf(stderr, "****** GATT Notify (td %" PRIu64 " ms, dev-discovered %" PRIu64 " ms): From %s\n",
                (tR-timestamp), (tR-dev->ts_creation), dev->toString().c_str());
        if( nullptr != charDecl ) {
            fprintf(stderr, "****** decl %s\n", charDecl->toString().c_str());
        }
        fprintf(stderr, "****** rawv %s\n", charValue->toString().c_str());
    }

    void indicationReceived(GATTCharacteristicRef charDecl,
                            std::shared_ptr<TROOctets> charValue, const uint64_t timestamp,
                            const bool confirmationSent) override
    {
        const std::shared_ptr<DBTDevice> dev = charDecl->getDevice();
        const int64_t tR = getCurrentMilliseconds();
        fprintf(stderr, "****** GATT Indication (confirmed %d, td(msg %" PRIu64 " ms, dev-discovered %" PRIu64 " ms): From %s\n",
                confirmationSent, (tR-timestamp), (tR-dev->ts_creation), dev->toString().c_str());
        if( nullptr != charDecl ) {
            fprintf(stderr, "****** decl %s\n", charDecl->toString().c_str());
            if( _TEMPERATURE_MEASUREMENT == *charDecl->value_type ) {
                std::shared_ptr<TemperatureMeasurementCharateristic> temp = TemperatureMeasurementCharateristic::get(*charValue);
                if( nullptr != temp ) {
                    fprintf(stderr, "****** valu %s\n", temp->toString().c_str());
                }
            }
        }
        fprintf(stderr, "****** rawv %s\n", charValue->toString().c_str());
    }
};

static void connectDiscoveredDevice(std::shared_ptr<DBTDevice> device) {
    fprintf(stderr, "****** Connecting Device: Start %s\n", device->toString().c_str());
    device->getAdapter().stopDiscovery();
    bool res = false;
    if( !USE_WHITELIST ) {
        res = device->connectDefault();
    }
    fprintf(stderr, "****** Connecting Device: End result %d of %s\n", res, device->toString().c_str());
    if( !USE_WHITELIST && 0 == devicesInProcessing.size() && !res ) {
        device->getAdapter().startDiscovery( true );
    }
}

static void processConnectedDevice(std::shared_ptr<DBTDevice> device) {
    fprintf(stderr, "****** Processing Device: Start %s\n", device->toString().c_str());
    const uint64_t t1 = getCurrentMilliseconds();
    bool success = false;

    //
    // GATT Service Processing
    //
    try {
        std::vector<GATTServiceRef> primServices = device->getGATTServices(); // implicit GATT connect...
        if( 0 == primServices.size() ) {
            goto exit;
        }

        const uint64_t t5 = getCurrentMilliseconds();
        {
            const uint64_t td15 = t5 - t1; // connected -> gatt-complete
            const uint64_t tdc5 = t5 - device->getCreationTimestamp(); // discovered to gatt-complete
            const uint64_t td05 = t5 - timestamp_t0; // adapter-init -> gatt-complete
            fprintf(stderr, "\n\n\n");
            fprintf(stderr, "GATT primary-services completed\n");
            fprintf(stderr, "  connected to gatt-complete %" PRIu64 " ms,\n"
                            "  discovered to gatt-complete %" PRIu64 " ms (connect %" PRIu64 " ms),\n"
                            "  adapter-init to gatt-complete %" PRIu64 " ms\n\n",
                            td15, tdc5, (tdc5 - td15), td05);
        }
        std::shared_ptr<GenericAccess> ga = device->getGATTGenericAccess();
        if( nullptr != ga ) {
            fprintf(stderr, "  GenericAccess: %s\n\n", ga->toString().c_str());
        }
        {
            std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
            if( nullptr != gatt && gatt->isOpen() ) {
                std::shared_ptr<DeviceInformation> di = gatt->getDeviceInformation(primServices);
                if( nullptr != di ) {
                    fprintf(stderr, "  DeviceInformation: %s\n\n", di->toString().c_str());
                }
            }
        }

        for(size_t i=0; i<primServices.size(); i++) {
            GATTService & primService = *primServices.at(i);
            fprintf(stderr, "  [%2.2d] Service %s\n", (int)i, primService.toString().c_str());
            fprintf(stderr, "  [%2.2d] Service Characteristics\n", (int)i);
            std::vector<GATTCharacteristicRef> & serviceCharacteristics = primService.characteristicList;
            for(size_t j=0; j<serviceCharacteristics.size(); j++) {
                GATTCharacteristic & serviceChar = *serviceCharacteristics.at(j);
                fprintf(stderr, "  [%2.2d.%2.2d] Decla: %s\n", (int)i, (int)j, serviceChar.toString().c_str());
                if( serviceChar.hasProperties(GATTCharacteristic::PropertyBitVal::Read) ) {
                    POctets value(GATTHandler::ClientMaxMTU, 0);
                    if( serviceChar.readValue(value) ) {
                        std::string sval = getUTF8String(value.get_ptr(), value.getSize());
                        fprintf(stderr, "  [%2.2d.%2.2d] Value: %s ('%s')\n", (int)i, (int)j, value.toString().c_str(), sval.c_str());
                    }
                }
                bool cccdEnableResult[2];
                bool cccdRet = serviceChar.configIndicationNotification(true /* enableNotification */, true /* enableIndication */, cccdEnableResult);
                fprintf(stderr, "  [%2.2d.%2.2d] Config Notification(%d), Indication(%d): Result %d\n",
                        (int)i, (int)j, cccdEnableResult[0], cccdEnableResult[1], cccdRet);
                if( cccdRet ) {
                    serviceChar.addCharacteristicListener( std::shared_ptr<GATTCharacteristicListener>( new MyGATTEventListener(&serviceChar) ) );
                }
            }
        }
        // FIXME sleep 1s for potential callbacks ..
        sleep(1);
        success = true;
    } catch ( std::exception & e ) {
        fprintf(stderr, "****** Processing Device: Exception caught for %s: %s\n", device->toString().c_str(), e.what());
    }

exit:
    device->disconnect();

    if( !USE_WHITELIST && 1 >= devicesInProcessing.size() ) {
        device->getAdapter().startDiscovery( true );
    }
    removeFromDevicesProcessing(device->getAddress());
    fprintf(stderr, "****** Processing Device: End: Success %d on %s\n", success, device->toString().c_str());
    if( success ) {
        addToDevicesProcessed(device->getAddress());
    }
}


int main(int argc, char *argv[])
{
    int dev_id = 0; // default
    bool waitForEnter=false;
    bool done = false;
    std::vector<std::shared_ptr<EUI48>> whitelist;

    for(int i=1; i<argc; i++) {
        if( !strcmp("-wait", argv[i]) ) {
            waitForEnter = true;
        } else if( !strcmp("-dev_id", argv[i]) && argc > (i+1) ) {
            dev_id = atoi(argv[++i]);
        } else if( !strcmp("-mac", argv[i]) && argc > (i+1) ) {
            std::string macstr = std::string(argv[++i]);
            waitForDevice = EUI48(macstr);
        } else if( !strcmp("-wl", argv[i]) && argc > (i+1) ) {
            std::string macstr = std::string(argv[++i]);
            std::shared_ptr<EUI48> wlmac( new EUI48(macstr) );
            fprintf(stderr, "Whitelist + %s\n", wlmac->toString().c_str());
            whitelist.push_back( wlmac );
            USE_WHITELIST = true;
        }
    }
    fprintf(stderr, "USE_WHITELIST %d\n", USE_WHITELIST);
    fprintf(stderr, "dev_id %d\n", dev_id);
    fprintf(stderr, "waitForDevice: %s\n", waitForDevice.toString().c_str());

    if( waitForEnter ) {
        fprintf(stderr, "Press ENTER to continue\n");
        getchar();
    }

    timestamp_t0 = getCurrentMilliseconds();

    DBTAdapter adapter(dev_id);
    if( !adapter.hasDevId() ) {
        fprintf(stderr, "Default adapter not available.\n");
        exit(1);
    }
    if( !adapter.isValid() ) {
        fprintf(stderr, "Adapter invalid.\n");
        exit(1);
    }
    fprintf(stderr, "Using adapter: device %s, address %s: %s\n",
        adapter.getName().c_str(), adapter.getAddressString().c_str(), adapter.toString().c_str());

    adapter.addStatusListener(std::shared_ptr<AdapterStatusListener>(new MyAdapterStatusListener()));

    if( USE_WHITELIST ) {
        for (auto it = whitelist.begin(); it != whitelist.end(); ++it) {
            std::shared_ptr<EUI48> wlmac = *it;
            bool res = adapter.addDeviceToWhitelist(*wlmac, BDAddressType::BDADDR_LE_PUBLIC, HCIWhitelistConnectType::HCI_AUTO_CONN_ALWAYS);
            fprintf(stderr, "Added to whitelist: res %d, address %s\n", res, wlmac->toString().c_str());
        }
    } else {
        if( !adapter.startDiscovery( true ) ) {
            perror("Adapter start discovery failed");
            done = true;
        }
    }

    while( !done ) {
        if( waitForDevice != EUI48_ANY_DEVICE && isDeviceProcessed(waitForDevice) ) {
            fprintf(stderr, "****** WaitForDevice processed %s", waitForDevice.toString().c_str());
            done = true;
        }
        sleep(3);
    }

    return 0;
}

