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

#ifndef JNIMEM__HPP_
#define JNIMEM__HPP_

#include <jni.h>
#include <stdexcept>

#include "BasicTypes.hpp"

extern JavaVM* vm;


/* 
 * This class provides a lifetime-managed JNIEnv object, which attaches or
 * detaches the current thread from the JVM automatically
 */
class JNIEnvContainer {
private:    
    JNIEnv *env = nullptr;
    bool needsDetach = false;
    
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
 * This class provides a lifetime-managed GlobalRef variable,
 * which is automatically deleted when it goes out of scope.
 *
 * RAII-style acquire and relinquish via destructor
 */
class JNIGlobalRef {
private:
    jobject object;

public:
    static inline void check(jobject object, const char* file, int line) {
        if( nullptr == object ) {
            throw direct_bt::RuntimeException("JNIGlobalRef::check: Null jobject", file, line);
        }
    }

    /* Creates a GlobalRef from an object passed to it */
    JNIGlobalRef(jobject object);

    JNIGlobalRef(const JNIGlobalRef &o);
    JNIGlobalRef(JNIGlobalRef &&o);

    JNIGlobalRef& operator=(const JNIGlobalRef &o);
    JNIGlobalRef& operator=(JNIGlobalRef &&o);

    /* Deletes the stored GlobalRef */
    ~JNIGlobalRef();

    /** Clears the java reference, i.e. nulling it, without deleting the global reference via JNI. */
    void clear();

    /* Provides access to the stored GlobalRef as an jobject. */
    jobject operator*() { return object; }

    /* Provides access to the stored GlobalRef as an jobject. */
    jobject getObject() const { return object; }
    /* Provides access to the stored GlobalRef as a jclass. */
    jclass getClass() const { return (jclass)object; }

    bool operator==(const JNIGlobalRef& rhs) const;

    bool operator!=(const JNIGlobalRef& rhs) const
    { return !( *this == rhs ); }
};

/*
 * This class provides a lifetime-managed 'PrimitiveArrayCritical' pinned heap,
 * which is automatically released when it goes out of scope.
 *
 * RAII-style acquire and relinquish via destructor
 */
template <typename T>
class JNICriticalArray {
public:
    enum Mode : jint {
        /** Like default 0: If 'isCopy': Update the java array data with the copy and free the copy. */
        UPDATE_AND_RELEASE = 0,

        /** Like JNI_COMMIT: If 'isCopy': Update the java array data with the copy, but do not free the copy. */
        UPDATE_NO_RELEASE = JNI_COMMIT,

        /** Like default JNI_ABORT: If 'isCopy': Do not update the java array data with the copy, but free the copy. */
        NO_UPDATE_AND_RELEASE = JNI_ABORT,
    };

private:
    JNIEnv *env;
    Mode mode = UPDATE_AND_RELEASE;
    jbyteArray jarray = nullptr;
    T* narray = nullptr;
    jboolean isCopy = false;

public:
    JNICriticalArray(JNIEnv *env) : env(env) {}

    JNICriticalArray(const JNICriticalArray &o) = delete;
    JNICriticalArray(JNICriticalArray &&o) = delete;
    JNICriticalArray& operator=(const JNICriticalArray &o) = delete;
    JNICriticalArray& operator=(JNICriticalArray &&o) = delete;

    /**
     * Release the acquired primitive array, RAII style.
     */
    ~JNICriticalArray() {
        release();
    }

    /**
     * Manual release of the acquired primitive array,
     * usually one likes to simply do this via the destructor, RAII style.
     */
    void release() {
        if( nullptr != narray ) {
            env->ReleasePrimitiveArrayCritical(jarray, narray, mode);
            this->jarray = nullptr;
            this->narray = nullptr;
            this->env = nullptr;
        }
    }

    /**
     * Acquired the primitive array.
     */
    T* get(jbyteArray jarray, Mode mode=UPDATE_AND_RELEASE) {
        if( nullptr == jarray ) {
            return nullptr;
        }
        T* narray = static_cast<T*>( env->GetPrimitiveArrayCritical(jarray, &isCopy) );
        if( nullptr != narray ) {
            this->mode = mode;
            this->jarray = jarray;
            this->narray = narray;
            return narray;
        }
        return nullptr;
    }

    /**
     * Returns true if the primitive array had been acquired
     * and the JVM utilizes a copy of the underlying java array.
     */
    bool getIsCopy() const { return isCopy; }
};

#endif /* JNIMEM__HPP_ */

