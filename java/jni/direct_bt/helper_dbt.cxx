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

#include <jni.h>
#include <memory>
#include <stdexcept>
#include <vector>

#include "helper_dbt.hpp"

using namespace direct_bt;

jclass direct_bt::search_class(JNIEnv *env, JavaUplink &object)
{
    return search_class(env, object.get_java_class().c_str());
}

#if 0

jobject direct_bt::convert_vector_to_jobject(JNIEnv *env, std::vector<std::shared_ptr<JavaUplink>>& array)
{
    unsigned int array_size = array.size();

    jmethodID arraylist_add;
    jobject result = get_new_arraylist(env, array_size, &arraylist_add);

    if (0 == array_size) {
        return result;
    }

    for (unsigned int i = 0; i < array_size; ++i) {
        std::shared_ptr<JavaUplink> elem = array.at(i);
        std::shared_ptr<JNIGlobalRef> objref = elem->getJavaObject();
        if ( nullptr == objref ) {
            throw InternalError("JavaUplink element of array has no valid java-object: "+elem->toString(), E_FILE_LINE);
        }
        env->CallBooleanMethod(result, arraylist_add, objref->get());
    }
    return result;
}

#endif

static std::string jStringEmpty("");
static std::string jAddressTypePublic("public");
static std::string jAddressTypeRandom("random");

BDAddressType direct_bt::fromJavaAdressTypeToBDAddressType(JNIEnv *env, jstring jAddressType) {
    if( nullptr != jAddressType ) {
        std::string saddressType = from_jstring_to_string(env, jAddressType);
        if( jAddressTypePublic == saddressType ) {
            return BDAddressType::BDADDR_LE_PUBLIC;
        }
        if( jAddressTypeRandom == saddressType ) {
            return BDAddressType::BDADDR_LE_RANDOM;
        }
    }
    return BDAddressType::BDADDR_BREDR;
}
jstring direct_bt::fromBDAddressTypeToJavaAddressType(JNIEnv *env, BDAddressType bdAddressType) {
    switch( bdAddressType ) {
        case BDAddressType::BDADDR_LE_PUBLIC:
            return from_string_to_jstring(env, jAddressTypePublic);
        case BDAddressType::BDADDR_LE_RANDOM:
            return from_string_to_jstring(env, jAddressTypeRandom);
        case BDAddressType::BDADDR_BREDR:
            // fall through intended
        default:
            return from_string_to_jstring(env, jStringEmpty);
    }
}
