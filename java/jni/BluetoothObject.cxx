/*
 * Author: Andrei Vasiliu <andrei.vasiliu@intel.com>
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

#include "tinyb/BluetoothObject.hpp"

#include "tinyb_BluetoothObject.h"

#include "helper.hpp"

using namespace tinyb;

jobject Java_tinyb_BluetoothObject_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "NONE");
}

jobject Java_tinyb_BluetoothObject_clone(JNIEnv *env, jobject obj)
{
    return generic_clone<BluetoothObject>(env, obj);
}

void Java_tinyb_BluetoothObject_delete(JNIEnv *env, jobject obj)
{
    BluetoothObject *obj_b = getInstance<BluetoothObject>(env, obj);

    delete obj_b;
}

jboolean Java_tinyb_BluetoothObject_operatorEqual(JNIEnv *env, jobject obj, jobject other)
{
    if (!other)
    {
        return JNI_FALSE;
    }
    BluetoothObject *obj_b = getInstance<BluetoothObject>(env, obj);
    BluetoothObject *obj_other = getInstance<BluetoothObject>(env, other);

    return (*obj_b) == (*obj_other);
}

jstring Java_tinyb_BluetoothObject_getObjectPath(JNIEnv *env, jobject obj)
{
    BluetoothObject *obj_b = getInstance<BluetoothObject>(env, obj);

    return env->NewStringUTF(obj_b->get_object_path().c_str());
}

