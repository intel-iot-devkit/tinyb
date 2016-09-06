#pragma once
#include<cstdint>
#include<string>
#include "BluetoothObject.hpp"

class tinyb::BluetoothUUID {
private:
    uint64_t uuid[2];

public:
    BluetoothUUID(const BluetoothUUID &other);
    BluetoothUUID(const std::string &uuid);
    BluetoothUUID(const char uuid[]);

    bool operator==(const BluetoothUUID &other);
    bool operator==(const std::string &str);
    bool operator==(const char str[]);

    BluetoothUUID operator=(const BluetoothUUID &other);
    BluetoothUUID operator=(const std::string &other);
    BluetoothUUID operator=(const char str[]);

    std::string get_string();
    std::string get_short_string();
    uint32_t get_short();
    bool is_short();
};
