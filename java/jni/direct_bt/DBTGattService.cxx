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

#include "direct_bt_tinyb_DBTGattService.h"

#define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTDevice.hpp"
#include "direct_bt/DBTAdapter.hpp"

using namespace direct_bt;

void Java_direct_1bt_tinyb_DBTGattService_deleteImpl(JNIEnv *env, jobject obj) {
    try {
        GATTService *service = getInstance<GATTService>(env, obj);
        JavaGlobalObj::check(service->getJavaObject(), E_FILE_LINE);
        (void)service;
        // No delete: Service instance owned by DBTDevice
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
}

static const std::string _characteristicClazzCtorArgs("(JLorg/tinyb/BluetoothGattService;[Ljava/lang/String;Ljava/lang/String;)V");

jobject Java_direct_1bt_tinyb_DBTGattService_getCharacteristics(JNIEnv *env, jobject obj) {
    try {
        GATTService *service = getInstance<GATTService>(env, obj);
        JavaGlobalObj::check(service->getJavaObject(), E_FILE_LINE);

        std::vector<std::shared_ptr<GATTCharacteristic>> & characteristics = service->characteristicList;

        // DBTGattCharacteristic(final long nativeInstance, final BluetoothGattService service, final String[] properties, final String uuid)

        std::function<jobject(JNIEnv*, jclass, jmethodID, GATTCharacteristic *)> ctor_char =
                [](JNIEnv *env, jclass clazz, jmethodID clazz_ctor, GATTCharacteristic *characteristic)->jobject {
                    // prepare adapter ctor
                    JavaGlobalObj::check(characteristic->service->getJavaObject(), E_FILE_LINE);
                    jobject jservice = JavaGlobalObj::GetObject(characteristic->service->getJavaObject());

                    std::vector<std::unique_ptr<std::string>> props = GATTCharacteristic::getPropertiesStringList(characteristic->properties);
                    unsigned int props_size = props.size();

                    jclass string_class = search_class(env, "Ljava/lang/String;");
                    jobjectArray jproperties = env->NewObjectArray(props_size, string_class, 0);
                    if( java_exception_check(env, E_FILE_LINE) ) { return nullptr; }

                    for (unsigned int i = 0; i < props_size; ++i) {
                        jobject elem = from_string_to_jstring(env, *props[i].get());
                        env->SetObjectArrayElement(jproperties, i, elem);
                    }
                    if( java_exception_check(env, E_FILE_LINE) ) { return nullptr; }

                    const jstring uuid = from_string_to_jstring(env, characteristic->value_type->toString());
                    if( java_exception_check(env, E_FILE_LINE) ) { return nullptr; }

                    jobject jchar = env->NewObject(clazz, clazz_ctor, (jlong)characteristic, jservice, jproperties, uuid);
                    if( java_exception_check(env, E_FILE_LINE) ) { return nullptr; }
                    JNIGlobalRef::check(jchar, E_FILE_LINE);
                    std::shared_ptr<JavaAnonObj> jCharRef = characteristic->getJavaObject();
                    JavaGlobalObj::check(jCharRef, E_FILE_LINE);

                    return JavaGlobalObj::GetObject(jCharRef);
                };
        return convert_vector_sharedptr_to_jarraylist<GATTCharacteristic>(env, characteristics, _characteristicClazzCtorArgs.c_str(), ctor_char);
    } catch(...) {
        rethrow_and_raise_java_exception(env);
    }
    return nullptr;
}

