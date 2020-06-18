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

#ifndef JAVA_ACCESS_HPP_
#define JAVA_ACCESS_HPP_

#include <string>
#include <memory>

namespace direct_bt {

    #define JAVA_DBT_PACKAGE "direct_bt/tinyb/"

    /**
     * Pure virtual JavaAnonObj, hiding Java JNI details from API,
     * to be implemented by JNI module.
     * <p>
     * One implementation is JavaGlobalObj within the JNI module,
     * wrapping a JNIGlobalRef instance.
     * </p>
     */
    class JavaAnonObj {
        public:
            virtual ~JavaAnonObj() { }
            virtual std::string toString() const { return "JavaAnonObj[???]"; }

            /** Clears the java reference, i.e. nulling it, without deleting the global reference via JNI. */
            virtual void clear() = 0;
    };

    /**
     * Sharing the anonymous Java object (JavaAnonObj),
     * i.e. exposing the Java object uplink to the C++ implementation.
     */
    class JavaUplink {
        private:
            std::shared_ptr<JavaAnonObj> javaObjectRef;

        public:
            virtual std::string toString() const = 0;
            virtual std::string get_java_class() const = 0;

            std::string javaObjectToString() const { return nullptr == javaObjectRef ? "JavaAnonObj[null]" : javaObjectRef->toString(); }

            std::shared_ptr<JavaAnonObj> getJavaObject() { return javaObjectRef; }

            /** Assigns a new shared JavaAnonObj reference, replaced item might be deleted via JNI from dtor */
            void setJavaObject(std::shared_ptr<JavaAnonObj> objRef) { javaObjectRef = objRef; }

            /** Clears the java reference, i.e. nulling it, without deleting the global reference via JNI. */
            void clearJavaObject() {
                if( nullptr != javaObjectRef ) {
                    javaObjectRef->clear();
                }
            }

            virtual ~JavaUplink() {
                javaObjectRef = nullptr;
            }
    };

} /* namespace direct_bt */


#endif /* JAVA_ACCESS_HPP_ */
