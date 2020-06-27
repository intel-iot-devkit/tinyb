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

#ifndef HELPER_DBT_HPP_
#define HELPER_DBT_HPP_

#include "JNIMem.hpp"
#include "helper_base.hpp"

#include "direct_bt/JavaUplink.hpp"
#include "direct_bt/BasicTypes.hpp"
#include "direct_bt/BTAddress.hpp"

namespace direct_bt {

    class DirectBTJNISettings {
        private:
            bool unifyUUID128Bit = true;

        public:
            /**
             * Enables or disables uuid128_t consolidation
             * for native uuid16_t and uuid32_t values before string conversion.
             * <p>
             * Default is {@code true}, as this represent compatibility with original TinyB D-Bus behavior.
             * </p>
             */
            bool getUnifyUUID128Bit() { return unifyUUID128Bit; }
            void setUnifyUUID128Bit(bool v) { unifyUUID128Bit = v; }
    };
    extern DirectBTJNISettings directBTJNISettings;

    /**
     * Implementation for JavaAnonObj,
     * by simply wrapping a JNIGlobalRef instance.
     */
    class JavaGlobalObj : public JavaAnonObj {
        private:
            JNIGlobalRef javaObjectRef;
            jmethodID  mNotifyDeleted;

        public:
            static inline void check(const std::shared_ptr<JavaAnonObj> & shref, const char* file, int line) {
                if( nullptr == shref ) {
                    throw direct_bt::RuntimeException("JavaGlobalObj::check: Null shared-JavaAnonObj", file, line);
                }
                const jobject obj = static_cast<const JavaGlobalObj*>(shref.get())->getObject();
                if( nullptr == obj ) {
                    throw direct_bt::RuntimeException("JavaGlobalObj::check: Null object", file, line);
                }
            }
            static bool isValid(const std::shared_ptr<JavaAnonObj> & shref) {
                if( nullptr == shref ) {
                    return false;
                }
                const jobject obj = static_cast<const JavaGlobalObj*>(shref.get())->getObject();
                if( nullptr == obj ) {
                    return false;
                }
                return true;
            }
            JavaGlobalObj(jobject obj, jmethodID mNotifyDeleted)
            : javaObjectRef(obj), mNotifyDeleted(mNotifyDeleted) { }

            JavaGlobalObj(const JavaGlobalObj &o) noexcept = default;
            JavaGlobalObj(JavaGlobalObj &&o) noexcept = default;
            JavaGlobalObj& operator=(const JavaGlobalObj &o) noexcept = default;
            JavaGlobalObj& operator=(JavaGlobalObj &&o) noexcept = default;

            virtual ~JavaGlobalObj();

            std::string toString() const override {
                const uint64_t ref = (uint64_t)(void*)javaObjectRef.getObject();
                return "JavaGlobalObj["+uint64HexString(ref, true)+"]";
            }

            /** Clears the java reference, i.e. nulling it, without deleting the global reference via JNI. */
            void clear() override { javaObjectRef.clear(); }

            JNIGlobalRef & getJavaObject() { return javaObjectRef; }

            /* Provides access to the stored GlobalRef as an jobject. */
            jobject getObject() const { return javaObjectRef.getObject(); }
            /* Provides access to the stored GlobalRef as a jclass. */
            jclass getClass() const { return javaObjectRef.getClass(); }

            /* Provides access to the stored GlobalRef as an jobject. */
            static jobject GetObject(const std::shared_ptr<JavaAnonObj> & shref) {
                return static_cast<JavaGlobalObj*>(shref.get())->getObject();
            }

            /* Provides access to the stored GlobalRef as a jclass. */
            static jclass GetClass(const std::shared_ptr<JavaAnonObj> & shref) {
                return static_cast<JavaGlobalObj*>(shref.get())->getClass();
            }
    };

    jclass search_class(JNIEnv *env, JavaUplink &object);

    template <typename T>
    jobject convert_vector_sharedptr_to_jarraylist(JNIEnv *env, std::vector<std::shared_ptr<T>>& array)
    {
        unsigned int array_size = array.size();

        jmethodID arraylist_add;
        jobject result = get_new_arraylist(env, array_size, &arraylist_add);

        if (0 == array_size) {
            return result;
        }

        for (unsigned int i = 0; i < array_size; ++i) {
            std::shared_ptr<T> elem = array[i];
            std::shared_ptr<JavaAnonObj> objref = elem->getJavaObject();
            if ( nullptr == objref ) {
                throw InternalError("JavaUplink element of array has no valid java-object: "+elem->toString(), E_FILE_LINE);
            }
            env->CallBooleanMethod(result, arraylist_add, JavaGlobalObj::GetObject(objref));
        }
        return result;
    }

    BDAddressType fromJavaAdressTypeToBDAddressType(JNIEnv *env, jstring jAddressType);
    jstring fromBDAddressTypeToJavaAddressType(JNIEnv *env, BDAddressType bdAddressType);

} // namespace direct_bt

#endif /* HELPER_DBT_HPP_ */
