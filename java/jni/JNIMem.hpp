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

#pragma once
#include <jni.h>
#include <stdexcept>

extern JavaVM* vm;

class JNIEnvContainer {
private:    
    JNIEnv *env = nullptr;
    
public:
    JNIEnv *operator*() {
        attach();
        return env;
    }

    JNIEnv *operator->() {
        attach();
        return env;
    }

    JNIEnvContainer() {
    }

    ~JNIEnvContainer() {
        detach();
    }

    void attach() {
        if (env != nullptr)
            return;
        jint err = vm->AttachCurrentThreadAsDaemon((void **)&env, NULL);
        if (err != JNI_OK)
            throw std::runtime_error("Attach to VM failed");
    }

    void detach() {
        if (env == nullptr)
            return;
        vm->DetachCurrentThread();
        env = nullptr;
    }
};

extern thread_local JNIEnvContainer jni_env;

class JNIGlobalRef {
private:
    jobject object;

public:
    
    JNIGlobalRef(jobject object) {
        this->object = jni_env->NewGlobalRef(object);
    }

    ~JNIGlobalRef() {
        jni_env->DeleteGlobalRef(object);
    }

    jobject operator*() {
        return object;
    }
};

