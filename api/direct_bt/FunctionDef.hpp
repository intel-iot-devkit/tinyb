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

#ifndef CLASS_FUNCTION_HPP_
#define CLASS_FUNCTION_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <functional>

#include "BasicTypes.hpp"

namespace direct_bt {

    /**
     * One goal to _produce_ the member-function type instance
     * is to be class type agnostic for storing in the toolkit.
     * This is essential to utilize a function-callback API,
     * where only the provider of an instance knows about its class type.
     *
     * Further we can't utilize std::function and std::bind,
     * as std::function doesn't provide details about the
     * member-function-call identity and hence lacks of
     * the equality operator and
     * std::bind doesn't even specify a return type.
     *
     * A capturing lambda in C++-11, does produce decoration code
     * accessing the captured elements, i.e. an anonymous helper class.
     * Due to this fact, the return type is an undefined lambda specific
     * and hence we can't use it to feed the function invocation
     * into ClassFunction<> using a well specified type.
     *
     * <pre>
        template<typename R, typename C, typename... A>
        inline ClassFunction<R, A...>
        bindClassFunction(C *base, R(C::*mfunc)(A...)) {
            return ClassFunction<R, A...>(
                    (void*)base,
                    (void*)(*((void**)&mfunc)),
                    [&](A... args)->R{ (base->*mfunc)(args...); });
                     ^
                     | Capturing lambda function-pointer are undefined!
        }
        </pre>
     *
     * Hence we need to manually produce the on-the-fly invocation data type
     * to capture details on the caller's class type for the member-function-call,
     * which are then being passed to the ClassFunction<> anonymously
     * while still being able to perform certain operations
     * like equality operation for identity.
     */
    template<typename R, typename... A>
    class InvocationFunc {
        protected:
            InvocationFunc() {}

        public:
            virtual ~InvocationFunc() noexcept {}

            InvocationFunc(const InvocationFunc &o) = default;
            InvocationFunc(InvocationFunc &&o) = default;
            InvocationFunc& operator=(const InvocationFunc &o) = default;
            InvocationFunc& operator=(InvocationFunc &&o) = default;

            /** Poor man's RTTI */
            virtual int getType() const = 0;

            virtual InvocationFunc<R, A...> * clone() const = 0;

            virtual R invoke(A... args) = 0;

            virtual bool operator==(const InvocationFunc<R, A...>& rhs) const = 0;

            virtual bool operator!=(const InvocationFunc<R, A...>& rhs) const = 0;

            virtual std::string toString() const = 0;
    };

    template<typename R, typename C, typename... A>
    class NullInvocationFunc : public InvocationFunc<R, A...> {
        public:
            NullInvocationFunc() { }

            int getType() const override { return 0; }

            InvocationFunc<R, A...> clone() const override { return NullInvocationFunc(); }

            R invoke(A... args) override {
                return (R)0;
            }

            bool operator==(const InvocationFunc<R, A...>& rhs) const override
            {
                return getType() == rhs.getType();
            }

            bool operator!=(const InvocationFunc<R, A...>& rhs) const override
            {
                return !( *this == rhs );
            }

            std::string toString() const override {
                return "NullInvocation";
            }
    };

    template<typename R, typename C, typename... A>
    class ClassInvocationFunc : public InvocationFunc<R, A...> {
        private:
            C* base;
            R(C::*member)(A...);

        public:
            ClassInvocationFunc(C *_base, R(C::*_member)(A...))
            : base(_base), member(_member) {
            }

            int getType() const override { return 1; }

            InvocationFunc<R, A...> * clone() const override { return new ClassInvocationFunc(*this); }

            R invoke(A... args) override {
                return (base->*member)(args...);
            }

            bool operator==(const InvocationFunc<R, A...>& rhs) const override
            {
                if( &rhs == this ) {
                    return true;
                }
                if( getType() != rhs.getType() ) {
                    return false;
                }
                const ClassInvocationFunc<R, C, A...> * prhs = static_cast<const ClassInvocationFunc<R, C, A...>*>(&rhs);
                return base == prhs->base && member == prhs->member;
            }

            bool operator!=(const InvocationFunc<R, A...>& rhs) const override
            {
                return !( *this == rhs );
            }

            std::string toString() const override {
                // hack to convert member pointer to void *: '*((void**)&member)'
                return "ClassInvocation "+uint64HexString((uint64_t)base)+"->"+aptrHexString( *((void**)&member) );
            }
    };

    template<typename R, typename... A>
    class PlainInvocationFunc : public InvocationFunc<R, A...> {
        private:
            R(*function)(A...);

        public:
            PlainInvocationFunc(R(*_function)(A...))
            : function(_function) {
            }

            int getType() const override { return 2; }

            InvocationFunc<R, A...> * clone() const override { return new PlainInvocationFunc(*this); }

            R invoke(A... args) override {
                return (*function)(args...);
            }

            bool operator==(const InvocationFunc<R, A...>& rhs) const override
            {
                if( &rhs == this ) {
                    return true;
                }
                if( getType() != rhs.getType() ) {
                    return false;
                }
                const PlainInvocationFunc<R, A...> * prhs = static_cast<const PlainInvocationFunc<R, A...>*>(&rhs);
                return function == prhs->function;
            }

            bool operator!=(const InvocationFunc<R, A...>& rhs) const override
            {
                return !( *this == rhs );
            }

            std::string toString() const override {
                // hack to convert function pointer to void *: '*((void**)&function)'
                return "PlainInvocation "+aptrHexString( *((void**)&function) );
            }
    };

    template<typename R, typename I, typename... A>
    class CaptureInvocationFunc : public InvocationFunc<R, A...> {
        private:
            I data;
            R(*function)(I&, A...);
            bool dataIsIdentity;

        public:
            /** Utilizes copy-ctor from 'const I& _data' */
            CaptureInvocationFunc(const I& _data, R(*_function)(I&, A...), bool dataIsIdentity)
            : data(_data), function(_function), dataIsIdentity(dataIsIdentity) {
            }

            /** Utilizes move-ctor from moved 'I&& _data' */
            CaptureInvocationFunc(I&& _data, R(*_function)(I&, A...), bool dataIsIdentity)
            : data(std::move(_data)), function(_function), dataIsIdentity(dataIsIdentity) {
            }

            int getType() const override { return 3; }

            InvocationFunc<R, A...> * clone() const override { return new CaptureInvocationFunc(*this); }

            R invoke(A... args) override {
                return (*function)(data, args...);
            }

            bool operator==(const InvocationFunc<R, A...>& rhs) const override
            {
                if( &rhs == this ) {
                    return true;
                }
                if( getType() != rhs.getType() ) {
                    return false;
                }
                const CaptureInvocationFunc<R, I, A...> * prhs = static_cast<const CaptureInvocationFunc<R, I, A...>*>(&rhs);
                return dataIsIdentity == prhs->dataIsIdentity && function == prhs->function && ( !dataIsIdentity || data == prhs->data );
            }

            bool operator!=(const InvocationFunc<R, A...>& rhs) const override
            {
                return !( *this == rhs );
            }

            std::string toString() const override {
                // hack to convert function pointer to void *: '*((void**)&function)'
                return "CaptureInvocation "+aptrHexString( *((void**)&function) );
            }
    };

    template<typename R, typename... A>
    class StdInvocationFunc : public InvocationFunc<R, A...> {
        private:
            uint64_t id;
            std::function<R(A...)> function;

        public:
            StdInvocationFunc(uint64_t _id, std::function<R(A...)> _function)
            : id(_id), function(_function) {
            }
            StdInvocationFunc(uint64_t _id)
            : id(_id), function() {
            }

            int getType() const override { return 10; }

            InvocationFunc<R, A...> * clone() const override { return new StdInvocationFunc(*this); }

            R invoke(A... args) override {
                return function(args...);
            }

            bool operator==(const InvocationFunc<R, A...>& rhs) const override
            {
                if( &rhs == this ) {
                    return true;
                }
                if( getType() != rhs.getType() ) {
                    return false;
                }
                const StdInvocationFunc<R, A...> * prhs = static_cast<const StdInvocationFunc<R, A...>*>(&rhs);
                return id == prhs->id;
            }

            bool operator!=(const InvocationFunc<R, A...>& rhs) const override
            {
                return !( *this == rhs );
            }

            std::string toString() const override {
                return "StdInvocation "+uint64HexString( id );
            }
    };

    template<typename R, typename... A>
    class FunctionDef {
        private:
            std::shared_ptr<InvocationFunc<R, A...>> func;

        public:
            /**
             * Constructs an instance with a null function.
             */
            FunctionDef()
            : func(std::shared_ptr<InvocationFunc<R, A...>>( new NullInvocationFunc<R, A...>() ))
            {};

            /**
             * Constructs an instance using the shared InvocationFunc<R, A...> function.
             */
            FunctionDef(std::shared_ptr<InvocationFunc<R, A...>> _func)
            : func(_func) { }

            /**
             * Constructs an instance by wrapping the given naked InvocationFunc<R, A...> function pointer
             * in a shared_ptr and taking ownership.
             * <p>
             * A convenience method.
             * </p.
             */
            FunctionDef(InvocationFunc<R, A...> * _funcPtr)
            : func(std::shared_ptr<InvocationFunc<R, A...>>(_funcPtr)) { }

            FunctionDef(const FunctionDef &o) = default;
            FunctionDef(FunctionDef &&o) = default;
            FunctionDef& operator=(const FunctionDef &o) = default;
            FunctionDef& operator=(FunctionDef &&o) = default;

            bool operator==(const FunctionDef<R, A...>& rhs) const
            { return *func == *rhs.func; }

            bool operator!=(const FunctionDef<R, A...>& rhs) const
            { return *func != *rhs.func; }

            /** Returns the shared InvocationFunc<R, A...> function */
            std::shared_ptr<InvocationFunc<R, A...>> getFunction() { return func; }

            /** Returns a new instance of the held InvocationFunc<R, A...> function. */
            InvocationFunc<R, A...> * cloneFunction() const { return func->clone(); }

            std::string toString() const {
                return "FunctionDef["+func->toString()+"]";
            }

            R invoke(A... args) {
                return func->invoke(args...);
            }
    };

    template<typename R, typename C, typename... A>
    inline FunctionDef<R, A...>
    bindMemberFunc(C *base, R(C::*mfunc)(A...)) {
        return FunctionDef<R, A...>( new ClassInvocationFunc<R, C, A...>(base, mfunc) );
    }

    template<typename R, typename... A>
    inline FunctionDef<R, A...>
    bindPlainFunc(R(*func)(A...)) {
        return FunctionDef<R, A...>( new PlainInvocationFunc<R, A...>(func) );
    }

    /**
     * <code>const I& data</code> will be copied into the InvocationFunc<..> specialization
     * and hence captured by copy.
     * <p>
     * The function call will have the reference of the copied data being passed for efficiency.
     * </p>
     */
    template<typename R, typename I, typename... A>
    inline FunctionDef<R, A...>
    bindCaptureFunc(const I& data, R(*func)(I&, A...), bool dataIsIdentity=true) {
        return FunctionDef<R, A...>( new CaptureInvocationFunc<R, I, A...>(data, func, dataIsIdentity) );
    }

    /**
     * <code>I&& data</code> will be moved into the InvocationFunc<..> specialization.
     * <p>
     * The function call will have the reference of the copied data being passed for efficiency.
     * </p>
     */
    template<typename R, typename I, typename... A>
    inline FunctionDef<R, A...>
    bindCaptureFunc(I&& data, R(*func)(I&, A...), bool dataIsIdentity=true) {
        return FunctionDef<R, A...>( new CaptureInvocationFunc<R, I, A...>(std::move(data), func, dataIsIdentity) );
    }

    template<typename R, typename... A>
    inline FunctionDef<R, A...>
    bindStdFunc(uint64_t id, std::function<R(A...)> func) {
        return FunctionDef<R, A...>( new StdInvocationFunc<R, A...>(id, func) );
    }
    template<typename R, typename... A>
    inline FunctionDef<R, A...>
    bindStdFunc(uint64_t id) {
        return FunctionDef<R, A...>( new StdInvocationFunc<R, A...>(id) );
    }

} // namespace direct_bt

#endif /* CLASS_FUNCTION_HPP_ */
