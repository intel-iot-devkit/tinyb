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

#include <dbt_debug.hpp>
#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <cstdio>

#include  <algorithm>

#include "GATTDescriptor.hpp"
#include "GATTHandler.hpp"
#include "DBTDevice.hpp"

using namespace direct_bt;

const uuid16_t GATTDescriptor::TYPE_EXT_PROP(Type::CHARACTERISTIC_EXTENDED_PROPERTIES);
const uuid16_t GATTDescriptor::TYPE_USER_DESC(Type::CHARACTERISTIC_USER_DESCRIPTION);
const uuid16_t GATTDescriptor::TYPE_CCC_DESC(Type::CLIENT_CHARACTERISTIC_CONFIGURATION);

std::shared_ptr<DBTDevice> GATTDescriptor::getDevice() {
    return characteristic->service->device;
}

bool GATTDescriptor::readValue(int expectedLength) {
    std::shared_ptr<DBTDevice> device = getDevice();
    std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
    if( nullptr == gatt ) {
        throw IllegalStateException("Characteristic's device GATTHandle not connected: "+
                toString() + ", " + device->toString(), E_FILE_LINE);
    }
    return gatt->readDescriptorValue(*this, expectedLength);
}

bool GATTDescriptor::writeValue() {
    std::shared_ptr<DBTDevice> device = getDevice();
    std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
    if( nullptr == gatt ) {
        throw IllegalStateException("Characteristic's device GATTHandle not connected: "+
                toString() + ", " + device->toString(), E_FILE_LINE);
    }
    return gatt->writeDescriptorValue(*this);
}
