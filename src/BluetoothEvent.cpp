#include "generated-code.h"
#include "BluetoothEvent.hpp"
#include "BluetoothManager.hpp"

void BluetoothEvent::generic_callback(BluetoothObject &object, void *data)
{

   if (data == nullptr)
        return;

   BluetoothConditionVariable *generic_data = static_cast<BluetoothConditionVariable *>(data);

   generic_data->result = object.clone();
   generic_data->notify();
}

BluetoothEvent::BluetoothEvent(BluetoothType type, std::string *name,
    std::string *identifier, BluetoothObject *parent, bool execute_once,
    BluetoothCallback cb, void *data)
{
    canceled = false;
    this->type = type;
    if (name != nullptr)
    	this->name = new std::string(*name);
    else
        this->name = nullptr;

    if (identifier != nullptr)
        this->identifier = new std::string(*identifier);
    else
        this->identifier = nullptr;

    if (parent != nullptr)
        this->parent = parent->clone();
    else
        this->parent = nullptr;

    this->execute_once = execute_once;
    this->cb = cb;

    if (cb == generic_callback)
        this->data = static_cast<void *>(&cv);
    else
        this->data = data;
}

bool BluetoothEvent::execute_callback(BluetoothObject &object)
{
    if (has_callback()) {
        cb(object, data);
        cv.notify();
        return execute_once;
    }

    return true;
}

void BluetoothEvent::wait(std::chrono::milliseconds timeout)
{
    if (!canceled && execute_once == true) {
        if (timeout == std::chrono::milliseconds::zero())
            cv.wait();
        else
            cv.wait_for(timeout);
    }
}

void BluetoothEvent::cancel()
{
    BluetoothManager *manager = BluetoothManager::get_bluetooth_manager();
    manager->remove_event(std::shared_ptr<BluetoothEvent>(this));

    cv.notify();
}

BluetoothEvent::~BluetoothEvent()
{
    if (name != nullptr)
        delete name;
    if (identifier != nullptr)
        delete identifier;
    if (parent != nullptr)
        delete parent;
}
