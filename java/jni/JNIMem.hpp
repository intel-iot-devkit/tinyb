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


/* 
 * This class provides a lifetime-managed JNIEnv object, which attaches or
 * detaches the current thread from the JVM automatically
 */
class JNIEnvContainer {
private:    
    JNIEnv *env = nullptr;
    
public:
    /* Attaches this thread to the JVM if it is not already attached */
    JNIEnvContainer();
    /* Detaches this thread to the JVM if it is attached */
    ~JNIEnvContainer();

    /* Provides access to the local thread's JNIEnv object */
    JNIEnv *operator*();
    /* Provides access to the local thread's JNIEnv object's methods */
    JNIEnv *operator->();

    /* Attaches this thread to the JVM if it is not already attached */
    void attach();
    /* Detaches this thread to the JVM if it is attached */
    void detach();
};

/* Each thread has a local jni_env variable of JNIEnvContainer type */
extern thread_local JNIEnvContainer jni_env;

/*
 * This class provides a lifetime-managed GlobalRef variable, which is automatically
 * deleted when it goes out of scope.
 */
class JNIGlobalRef {
private:
    jobject object;

public:
    /* Creates a GlobalRef from an object passed to it */
    JNIGlobalRef(jobject object);
    /* Deletes the stored GlobalRef */
    ~JNIGlobalRef();

    /* Provides access to the stored GlobalRef */
    jobject operator*();
};

