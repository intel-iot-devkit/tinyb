/*
 * Author: Petre Eftime <petre.p.eftime@intel.com>
 * Copyright (c) 2016 Intel Corporation.
 *
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

#include <cstdio>

#include "JNIMem.hpp"

// #define VERBOSE_ON 1
#ifdef VERBOSE_ON
    #define DBG_PRINT(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); }
#else
    #define DBG_PRINT(...)
#endif

JavaVM* vm;
thread_local JNIEnvContainer jni_env;

jint JNI_OnLoad(JavaVM *initVM, void *reserved) {
    (void)reserved; // warning
    vm = initVM;
    return JNI_VERSION_1_8;
}

JNIEnv *JNIEnvContainer::operator*() {
    attach();
    return env;
}

JNIEnv *JNIEnvContainer::operator->() {
    attach();
    return env;
}

JNIEnvContainer::JNIEnvContainer() {}

JNIEnvContainer::~JNIEnvContainer() {
    detach();
}

void JNIEnvContainer::attach() {
    if (env != nullptr) {
        return;
    }
    JNIEnv *newEnv = nullptr;
    int envRes;

    envRes = vm->GetEnv((void **) &env, JNI_VERSION_1_8) ;
    if( JNI_EDETACHED == envRes ) {
        envRes = vm->AttachCurrentThreadAsDaemon((void**) &newEnv, NULL);
        if( JNI_OK != envRes ) {
            throw direct_bt::RuntimeException("Attach to VM failed", E_FILE_LINE);
        }
        env = newEnv;
    } else if( JNI_OK != envRes ) {
        throw direct_bt::RuntimeException("GetEnv of VM failed", E_FILE_LINE);
    }
    if (env==NULL) {
        throw direct_bt::RuntimeException("GetEnv of VM is NULL", E_FILE_LINE);
    }
    needsDetach = NULL != newEnv;
}

void JNIEnvContainer::detach() {
    if (env == nullptr) {
        return;
    }
    if( needsDetach ) {
        vm->DetachCurrentThread();
    }
    env = nullptr;
    needsDetach = false;
}

JNIGlobalRef::JNIGlobalRef(jobject object) {
    if( nullptr == object ) {
        throw direct_bt::RuntimeException("JNIGlobalRef ctor null jobject", E_FILE_LINE);
    }
    this->object = jni_env->NewGlobalRef(object);
    DBG_PRINT("JNIGlobalRef::def_ctor %p -> %p", object, this->object);
}

JNIGlobalRef::JNIGlobalRef(const JNIGlobalRef &o) {
    if( nullptr == o.object ) {
        throw direct_bt::RuntimeException("Other JNIGlobalRef jobject is null", E_FILE_LINE);
    }
    object = jni_env->NewGlobalRef(o.object);
    DBG_PRINT("JNIGlobalRef::copy_ctor %p -> %p", o.object, object);
}
JNIGlobalRef::JNIGlobalRef(JNIGlobalRef &&o)
: object(o.object) {
    DBG_PRINT("JNIGlobalRef::move_ctor %p (nulled) -> %p", o.object, object);
    o.object = nullptr;
}
JNIGlobalRef& JNIGlobalRef::operator=(const JNIGlobalRef &o) {
    if( &o == this ) {
        return *this;
    }
    JNIEnv * env = *jni_env;
    if( nullptr != object ) { // always
        env->DeleteGlobalRef(object);
    }
    if( nullptr == o.object ) {
        throw direct_bt::RuntimeException("Other JNIGlobalRef jobject is null", E_FILE_LINE);
    }
    object = jni_env->NewGlobalRef(o.object);
    DBG_PRINT("JNIGlobalRef::copy_assign %p -> %p", o.object, object);
    return *this;
}
JNIGlobalRef& JNIGlobalRef::operator=(JNIGlobalRef &&o) {
    object = o.object;
    DBG_PRINT("JNIGlobalRef::move_assign %p (nulled) -> %p", o.object, object);
    o.object = nullptr;
    return *this;
}

JNIGlobalRef::~JNIGlobalRef() {
    try {
        JNIEnv * env = *jni_env;
        if( nullptr == env ) {
            throw direct_bt::RuntimeException("JNIGlobalRef dtor null JNIEnv", E_FILE_LINE);
        }
        DBG_PRINT("JNIGlobalRef::dtor %p", object);
        if( nullptr != object ) {
            // due to move ctor and assignment, we accept nullptr object
            env->DeleteGlobalRef(object);
        }
    } catch (std::exception &e) {
        fprintf(stderr, "JNIGlobalRef dtor: Caught %s\n", e.what());
    }
}

bool JNIGlobalRef::operator==(const JNIGlobalRef& rhs) const {
    if( &rhs == this ) {
        DBG_PRINT("JNIGlobalRef::== true: %p == %p (ptr)", object, rhs.object);
        return true;
    }
    bool res = JNI_TRUE == jni_env->IsSameObject(object, rhs.object);
    DBG_PRINT("JNIGlobalRef::== %d: %p == %p (IsSameObject)", res, object, rhs.object);
    return res;
}
