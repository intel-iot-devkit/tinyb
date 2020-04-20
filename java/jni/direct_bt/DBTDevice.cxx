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

#include "direct_bt_tinyb_DBTDevice.h"

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTTypes.hpp"

using namespace direct_bt;

void Java_direct_1bt_tinyb_DBTDevice_initImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        JavaGlobalObj::check(device->getJavaObject(), E_FILE_LINE);
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
}

void Java_direct_1bt_tinyb_DBTDevice_deleteImpl(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        delete device;
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
}

jshort Java_direct_1bt_tinyb_DBTDevice_getRSSI(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        return device->getRSSI();
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return 0;
}

jshort Java_direct_1bt_tinyb_DBTDevice_getTxPower(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        return device->getTxPower();
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return 0;
}

jboolean Java_direct_1bt_tinyb_DBTDevice_getConnected(JNIEnv *env, jobject obj)
{
    try {
        DBTDevice *device = getInstance<DBTDevice>(env, obj);
        (void) device;
        return JNI_TRUE; // FIXME
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
    return JNI_FALSE;
}
