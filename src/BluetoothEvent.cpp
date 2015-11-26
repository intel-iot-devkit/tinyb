#include "BluetoothEvent.hpp"

void BluetoothEvent::generic_callback(BluetoothObject &object, void *data)
{
}

BluetoothEvent::BluetoothEvent(BluetoothType type, std::string *name,
    std::string *identifier, BluetoothObject *parent,
    BluetoothCallback cb, void *data)
{
    this->type = type;
    if (name != NULL)
    	this->name = new std::string(*name);
    else
        this->name = NULL;

    if (identifier != NULL)
        this->identifier = new std::string(*identifier);
    else
        this->identifier = NULL;

    if (parent != NULL)
        this->parent = parent->clone();
    else
        this->parent = NULL;

    this->cb = cb;
    this->data = data;
}
