/*
 * Author: Petre Eftime <petre.p.eftime@intel.com>
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

#include "JNIMem.hpp"

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
            throw std::runtime_error("Attach to VM failed");
        }
        env = newEnv;
    } else if( JNI_OK != envRes ) {
        throw std::runtime_error("GetEnv of VM failed");
    }
    if (env==NULL) {
        throw std::runtime_error("GetEnv of VM is NULL");
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
    this->object = jni_env->NewGlobalRef(object);
}

JNIGlobalRef::~JNIGlobalRef() {
    jni_env->DeleteGlobalRef(object);
}

jobject JNIGlobalRef::operator*() {
    return object;
}
