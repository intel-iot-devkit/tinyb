/*
 * Author: Petre Eftime <petre.p.eftime@intel.com>
 * Copyright (c) 2015 Intel Corporation.
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

#include <tinyb.hpp>

#include <vector>
#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

using namespace tinyb;



/** Converts a raw temperature read from the sensor to a Celsius value.
 * @param[in] raw_temp The temperature read from the sensor (two bytes)
 * @return The Celsius value of the temperature
 */
static float celsius_temp(uint16_t raw_temp)
{
    const float SCALE_LSB = 0.03125;
    return ((float)(raw_temp >> 2)) * SCALE_LSB;
}

std::atomic<bool> running(true);

void signal_handler(int signum)
{
    if (signum == SIGINT) {
        running = false;
    }
}

/** This program reads the temperature from a
 * TI Sensor Tag(http://www.ti.com/ww/en/wireless_connectivity/sensortag2015/?INTC=SensorTag&HQS=sensortag)
 * Pass the MAC address of the sensor as the first parameter of the program.
 */
int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Run as: " << argv[0] << " <device_address>" << std::endl;
        exit(1);
    }

    BluetoothManager *manager = nullptr;
    try {
        manager = BluetoothManager::get_bluetooth_manager();
    } catch(const std::runtime_error& e) {
        std::cerr << "Error while initializing libtinyb: " << e.what() << std::endl;
        exit(1);
    }

    /* Start the discovery of devices */
    bool ret = manager->start_discovery();
    std::cout << "Started = " << (ret ? "true" : "false") << std::endl;

    std::unique_ptr<BluetoothGattService> temperature_service;

    std::string device_mac(argv[1]);
    auto sensor_tag = manager->find<BluetoothDevice>(nullptr, &device_mac, nullptr, std::chrono::seconds(10));
    if (sensor_tag == nullptr) {
        std::cout << "Device not found" << std::endl;
        return 1;
    }

    if (sensor_tag == nullptr) {
       ret = manager->stop_discovery();
       std::cerr << "SensorTag not found after 30 seconds, exiting" << std::endl;
       return 1;
    }

    /* Connect to the device and get the list of services exposed by it */
    sensor_tag->connect();
    std::string service_uuid("f000aa00-0451-4000-b000-000000000000");
    std::cout << "Waiting for service " << service_uuid << " to be discovered" << std::endl;
    temperature_service = sensor_tag->find(&service_uuid);

    /* Stop the discovery (the device was found or timeout was over) */
    ret = manager->stop_discovery();
    std::cout << "Stopped = " << (ret ? "true" : "false") << std::endl;

    auto value_uuid = std::string("f000aa01-0451-4000-b000-000000000000");
    auto temp_value = temperature_service->find(&value_uuid);

    auto config_uuid = std::string("f000aa02-0451-4000-b000-000000000000");
    auto temp_config = temperature_service->find(&config_uuid);

    auto period_uuid = std::string("f000aa03-0451-4000-b000-000000000000");
    auto temp_period = temperature_service->find(&period_uuid);

    /* Activate the temperature measurements */
    try {
        std::vector<unsigned char> config_on {0x01};
        temp_config->write_value(config_on);
        std::signal(SIGINT, signal_handler);
    } catch (std::exception &e) {
         std::cout << "Error: " << e.what() << std::endl;
         running = false;
    }

    while (running) {
        /* Read temperature data and display it */
        try {
            std::vector<unsigned char> response = temp_value->read_value();
            unsigned char *data;
            unsigned int size = response.size();
            if (size > 0) {
                data = response.data();

                std::cout << "Raw data=[";
                for (unsigned i = 0; i < response.size(); i++)
                    std::cout << std::hex << static_cast<int>(data[i]) << ", ";
                std::cout << "] ";

                uint16_t ambient_temp, object_temp;
                object_temp = data[0] | (data[1] << 8);
                ambient_temp = data[2] | (data[3] << 8);

                std::cout << "Ambient temp: " << celsius_temp(ambient_temp) << "C ";
                std::cout << "Object temp: " << celsius_temp(object_temp) << "C ";
                std::cout << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));

        } catch (std::exception &e) {
            std::cout << "Error: " << e.what() << std::endl;
            break;
        }
    }

    /* Disconnect from the device */
    try {
        sensor_tag->disconnect();
    } catch (std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return 0;
}
