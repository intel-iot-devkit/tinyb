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

#include "direct_bt_tinyb_DBTNativeDownlink.h"

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "JNIMem.hpp"
#include "helper_base.hpp"
#include "helper_dbt.hpp"

#include "direct_bt/DBTTypes.hpp"

using namespace direct_bt;

void Java_direct_1bt_tinyb_DBTNativeDownlink_initNativeJavaObject(JNIEnv *env, jobject obj, jlong nativeInstance)
{
    try {
        JavaUplink *javaUplink = castInstance<JavaUplink>(nativeInstance);
        std::shared_ptr<JavaGlobalObj> jobjRef( new JavaGlobalObj(obj) );
        javaUplink->setJavaObject( jobjRef );
        JavaGlobalObj::check(javaUplink->getJavaObject(), E_FILE_LINE);
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
}

void Java_direct_1bt_tinyb_DBTNativeDownlink_clearNativeJavaObject(JNIEnv *env, jobject obj, jlong nativeInstance)
{
    (void)obj;
    try {
        JavaUplink *javaUplink = castInstance<JavaUplink>(nativeInstance);
        DBG_PRINT("Java_direct_1bt_tinyb_DBTNativeDownlink_clearNativeJavaObject %s", javaUplink->toString().c_str());
        javaUplink->setJavaObject(nullptr);
    } CATCH_EXCEPTION_AND_RAISE_JAVA(env, e)
}

