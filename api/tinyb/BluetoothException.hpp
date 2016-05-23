#pragma once
#include <stdexcept>
#include <iostream>

using namespace tinyb;

class tinyb::BluetoothException: public std::runtime_error {

public:
    explicit BluetoothException(const std::string &msg): std::runtime_error(msg) {
    }

    explicit BluetoothException(const char *msg): std::runtime_error(msg) {
    }

    virtual ~BluetoothException() throw() {
    }

    virtual const char* what() const throw() {
        return std::runtime_error::what();
    }
};
