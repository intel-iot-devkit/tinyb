/*
 * Author: Henry Bruce <henry.bruce@intel.com>
 * Copyright (c) 2016 Intel Corporation.
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

/*
 * This sample looks for a device that implements the Environmental Sensing
 * Service and supports temperatures notifications. It then starts notfication
 * updates and displays samples until CRTL+C is hit.
 * Sample has been tested with the following devices:
 * - Zephyr Environmental Sensing Profile sample running on Arduino 101
 */

#include <tinyb.hpp>
#include <tinyb/BluetoothException.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <thread>
#include <csignal>
#include <condition_variable>

using namespace tinyb;

#define BT_MEAS_INTERVAL_INDEX 6

struct es_measurement {
    uint16_t reserved;
    uint8_t sampling_func;
    uint32_t meas_period;
    uint32_t update_interval;
    uint8_t application;
    uint8_t meas_uncertainty;
};


const std::string BT_UUID_ESS         = "0000181a-0000-1000-8000-00805f9b34fb";
const std::string BT_UUID_CUD         = "00002901-0000-1000-8000-00805f9b34fb";
const std::string BT_UUID_TEMPERATURE = "00002a6e-0000-1000-8000-00805f9b34fb";
const std::string BT_UUID_MEASUREMENT = "0000290c-0000-1000-8000-00805f9b34fb";
const std::string BT_NOFITY_FLAG = "notify";

std::condition_variable cv;


static void signal_handler(int signum)
{
    if (signum == SIGINT) {
        cv.notify_all();
    }
}


static void wait_ctrl_c()
{
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    std::signal(SIGINT, signal_handler);
    cv.wait(lock);
}


void data_callback(BluetoothGattCharacteristic &c, std::vector<unsigned char> &data, void *userdata)
{
    // unsigned char *data_c;
    unsigned int size = data.size();
    if (size == 2) {
        int16_t* raw_data = reinterpret_cast<int16_t*>(data.data());
        std::cout << "Raw data = " << std::hex << std::setfill('0') << std::setw(4) << *raw_data  << ". ";
        uint16_t temp = (*raw_data + 50) / 100;
        std::cout << "Temperature = " << std::dec << temp << "C " << std::endl;
    }
}


/** This program reads the temperature from a device running the Environmental Sensing Senvice.
 */
int main(int argc, char **argv)
{
    BluetoothManager *manager = nullptr;
    try {
        manager = BluetoothManager::get_bluetooth_manager();
    } catch(const std::runtime_error& e) {
        std::cerr << "Error while initializing libtinyb: " << e.what() << std::endl;
        exit(1);
    }

    std::unique_ptr<BluetoothAdapter> adapter = manager->get_default_adapter();
    BluetoothDevice *ess_device = NULL;

    // Start the discovery of devices */
    bool ret = manager->start_discovery();
    if (!ret) {
        std::cout << "Failed to start discovery" << std::endl;
        return 1;
    }
    std::cout << "Discovering BLE devices";
    int num_devices = 0;
    bool no_new_devices = false;
    std::vector<std::unique_ptr<BluetoothDevice>> list_devices;
    while (ess_device == NULL && !no_new_devices) {
        list_devices = manager->get_devices();
        std::cout << "." << std::flush;
        if (list_devices.size() > 0 && list_devices.size() == num_devices)
            no_new_devices = true;
        num_devices = list_devices.size();

        // Look for active ESS device
        for (auto it = list_devices.begin(); it != list_devices.end(); ++it) {
            if ((*it)->get_rssi() != 0) {
                auto list_uuids = (*it)->get_uuids();
                for (auto uuit = list_uuids.begin(); uuit != list_uuids.end(); ++uuit) {
                    if (*(uuit) == BT_UUID_ESS) {
                        ess_device = (*it).release();
                        break;
                    }
                }
            }
        }

        if (ess_device == NULL)
            std::this_thread::sleep_for(std::chrono::seconds(4));
    }
    ret = manager->stop_discovery();
    std::cout << std::endl;

    // Now try to connect
    if (ess_device != NULL) {
        std::cout << "Connecting to " << ess_device->get_name() << " with addr " << ess_device->get_address() << std::flush;
        try {
            if (ess_device->connect()) {
                std::cout << ". Connected" << std::endl;
            }
            else
                std::cout << ". Failed" << std::endl;
        } catch (BluetoothException& e) {
            std::cout  << std::endl << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cout << "ESS device not found." << std::endl;
        delete ess_device;
        return 1;
    }

    if (!ess_device->get_connected()) {
        delete ess_device;
        return 1;
    }

    std::cout << "Getting environmental service" << std::endl;
    std::unique_ptr<BluetoothGattService> environmental_service = ess_device->find(const_cast<std::string*>(&BT_UUID_ESS));
    std::cout << "Getting temperature characteristic" << std::endl;
    std::unique_ptr<BluetoothGattCharacteristic> temp_characteristic = environmental_service->find(const_cast<std::string*>(&BT_UUID_TEMPERATURE));

    std::vector<std::string> list_flags = temp_characteristic->get_flags();
    if (std::find(list_flags.begin(), list_flags.end(), BT_NOFITY_FLAG) != list_flags.end()) {
        std::unique_ptr<BluetoothGattDescriptor> meas = temp_characteristic->find(const_cast<std::string*>(&BT_UUID_MEASUREMENT));
        std::unique_ptr<BluetoothGattDescriptor> cud = temp_characteristic->find(const_cast<std::string*>(&BT_UUID_CUD));
        std::vector<unsigned char> name_bytes = cud->read_value();
        std::string name(reinterpret_cast<char *>(name_bytes.data()), name_bytes.size());
        std::cout << "Sensor name is '" << name << "'" << std::endl;
        std::vector<unsigned char> meas_bytes = meas->read_value();
        int notification_interval = meas_bytes[BT_MEAS_INTERVAL_INDEX];
        std::cout << "Temperature notification interval = " << notification_interval << " secs" << std::endl;
        std::cout << "Starting temperature notifications. " << std::endl;
        temp_characteristic->enable_value_notifications(data_callback, nullptr);
        wait_ctrl_c();
        temp_characteristic->disable_value_notifications();
    } else {
        std::cout << "Sensor does not support notifications" << std::endl;
    }

    /* Disconnect from the device */
    std::cout << "Disconnecting" << std::endl;
    try {
        ess_device->disconnect();
        delete ess_device;
    } catch (std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return 0;
}
