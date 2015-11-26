#include <string>
#include <condition_variable>
#include "BluetoothObject.hpp"
#pragma once

using namespace tinyb;

typedef void (*BluetoothCallback)(BluetoothObject &, void *);

class tinyb::BluetoothEvent {
private:
    std::string *name;
    std::string *identifier;
    BluetoothObject *parent;
    BluetoothType type;
    BluetoothCallback cb;
    void *data;

static void generic_callback(BluetoothObject &object, void *data);

struct generic_callback_data {
    std::condition_variable cv;
    BluetoothObject *result;
};

public:
    
    BluetoothEvent(BluetoothType type, std::string *name, std::string *identifier,
        BluetoothObject *parent, BluetoothCallback cb = generic_callback,
        void *data = NULL);

    BluetoothType get_type();
    std::string get_name();
    std::string get_identifier();
    bool execute_callback();
    bool has_callback();

    bool operator==(BluetoothEvent const &other);
};
