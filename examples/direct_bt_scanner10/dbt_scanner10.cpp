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

static EUI48 waitForDevice = EUI48_ANY_DEVICE;

static void deviceConnectTask(std::shared_ptr<DBTDevice> device);

static void deviceProcessTask(std::shared_ptr<DBTDevice> device);

#include <pthread.h>

class DeviceTask {
    public:
        std::shared_ptr<DBTDevice> device;
        std::thread worker;

        DeviceTask(std::shared_ptr<DBTDevice> d)
        : device(d),
          worker( std::thread(::deviceProcessTask, d) )
        {
            worker.detach();
            fprintf(stderr, "DeviceTask ctor: %s\n", d->toString().c_str());
        }

        DeviceTask(const DeviceTask &o) noexcept = default;
        DeviceTask(DeviceTask &&o) noexcept = default;
        DeviceTask& operator=(const DeviceTask &o) noexcept = default;
        DeviceTask& operator=(DeviceTask &&o) noexcept = default;

        ~DeviceTask() {
            fprintf(stderr, "DeviceTask dtor: %s\n", device->toString().c_str());
        }

};

/**
 * TODO: Analyze why 'vector<DeviceTask>' regarding field 'std::thread'
 *       does _not_ work. Instance vector of DeviceTask with std::thread
 *       causes multiple occasions of SIGSEGFAULT and mutex issues!
 */
static std::vector<std::shared_ptr<DeviceTask>> deviceTasks;
static std::recursive_mutex mtx_deviceTasks;

static int getDeviceTaskCount() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_deviceTasks); // RAII-style acquire and relinquish via destructor
    return deviceTasks.size();
}
static bool isDeviceTaskInProgress(std::shared_ptr<DBTDevice> d) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_deviceTasks); // RAII-style acquire and relinquish via destructor
    for (auto it = deviceTasks.begin(); it != deviceTasks.end(); ++it) {
        if ( *d == *(*it)->device ) {
            return true;
        }
    }
    return false;
}
static bool addDeviceTask(std::shared_ptr<DBTDevice> d) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_deviceTasks); // RAII-style acquire and relinquish via destructor
    if( !isDeviceTaskInProgress(d) ) {
        deviceTasks.push_back( std::shared_ptr<DeviceTask>( new DeviceTask(d) ) );
        return true;
    }
    return false;
}
static bool removeDeviceTask(std::shared_ptr<DBTDevice> d) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_deviceTasks); // RAII-style acquire and relinquish via destructor
    for (auto it = deviceTasks.begin(); it != deviceTasks.end(); ) {
        if ( *d == *(*it)->device ) {
            it = deviceTasks.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

static std::recursive_mutex mtx_devicesProcessed;
static std::vector<EUI48> devicesProcessed;

static void addDevicesProcessed(const EUI48 &a) {
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

    void deviceFound(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) override {
        (void)timestamp;

        if( waitForDevice == EUI48_ANY_DEVICE ||
            ( waitForDevice == device->address && !isDeviceProcessed(waitForDevice) ) )
        {
            fprintf(stderr, "****** FOUND__-0: Connecting %s\n", device->toString().c_str());
            std::thread dc(::deviceConnectTask, device);
            dc.detach();
        } else {
            fprintf(stderr, "****** FOUND__-1: NOP %s\n", device->toString().c_str());
        }
    }
    void deviceUpdated(std::shared_ptr<DBTDevice> device, const uint64_t timestamp, const EIRDataType updateMask) override {
        fprintf(stderr, "****** UPDATED: %s of %s\n", eirDataMaskToString(updateMask).c_str(), device->toString().c_str());
        (void)timestamp;
    }
    void deviceConnected(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) override {
        (void)timestamp;

        if( waitForDevice == EUI48_ANY_DEVICE ||
            ( waitForDevice == device->address && !isDeviceProcessed(waitForDevice) ) )
        {
            fprintf(stderr, "****** CONNECTED-0: Processing %s\n", device->toString().c_str());
            addDeviceTask( device );
        } else {
            fprintf(stderr, "****** CONNECTED-1: NOP %s\n", device->toString().c_str());
        }
    }
    void deviceDisconnected(std::shared_ptr<DBTDevice> device, const uint64_t timestamp) override {
        fprintf(stderr, "****** DISCONNECTED: %s\n", device->toString().c_str());
        (void)timestamp;
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

static void deviceConnectTask(std::shared_ptr<DBTDevice> device) {
    fprintf(stderr, "****** Device Connector: Start %s\n", device->toString().c_str());
    device->getAdapter().stopDiscovery();
    bool res = device->connectHCIDefault();
    fprintf(stderr, "****** Device Connector: End result %d of %s\n", res, device->toString().c_str());
    if( !res && 0 == getDeviceTaskCount() ) {
        fprintf(stderr, "****** Device Connector: startDiscovery()\n");
        device->getAdapter().startDiscovery();
    }
}

static void deviceProcessTask(std::shared_ptr<DBTDevice> device) {
    fprintf(stderr, "****** Device Process: Start %s\n", device->toString().c_str());
    const uint64_t t1 = getCurrentMilliseconds();

    //
    // GATT Service Processing
    //
    std::vector<GATTServiceRef> primServices;
    std::shared_ptr<GATTHandler> gatt = device->connectGATT();
    if( nullptr == gatt ) {
        fprintf(stderr, "****** Device Process: GATT Connect failed\n");
        goto out;
    }
    primServices = device->getGATTServices(); // implicit GATT connect...
    if( primServices.size() > 0 ) {
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
                        fprintf(stderr, "  [%2.2d.%2.2d] Value: %s\n", (int)i, (int)j, value.toString().c_str());
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
    }
    device->disconnect();

out:
    addDevicesProcessed(device->getAddress());
    if( 1 >= getDeviceTaskCount() ) {
        fprintf(stderr, "****** Device Process: startDiscovery()\n");
        device->getAdapter().startDiscovery();
    }
    removeDeviceTask(device);
    fprintf(stderr, "****** Device Process: End\n");
}


int main(int argc, char *argv[])
{
    int dev_id = 0; // default
    bool waitForEnter=false;
    bool done = false;

    for(int i=1; i<argc; i++) {
        if( !strcmp("-wait", argv[i]) ) {
            waitForEnter = true;
        } else if( !strcmp("-dev_id", argv[i]) && argc > (i+1) ) {
            dev_id = atoi(argv[++i]);
        } else if( !strcmp("-mac", argv[i]) && argc > (i+1) ) {
            std::string macstr = std::string(argv[++i]);
            waitForDevice = EUI48(macstr);
        }
    }
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

    std::shared_ptr<HCIComm> hci = adapter.openHCI();
    if( nullptr == hci || !hci->isOpen() ) {
        fprintf(stderr, "Couldn't open HCI from %s\n", adapter.toString().c_str());
        exit(1);
    }

    if( !adapter.startDiscovery() ) {
        perror("Adapter start discovery failed");
        goto out;
    }

    do {
        if( waitForDevice != EUI48_ANY_DEVICE && isDeviceProcessed(waitForDevice) ) {
            fprintf(stderr, "****** WaitForDevice processed %s", waitForDevice.toString().c_str());
            done = true;
        }
        sleep(3);
    } while( !done );

out:
    adapter.closeHCI();
    return 0;
}

