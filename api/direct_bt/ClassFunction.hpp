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

            virtual R invoke(A... args) const = 0;

            virtual bool operator==(const InvocationFunc<R, A...>& rhs) const = 0;

            virtual bool operator!=(const InvocationFunc<R, A...>& rhs) const = 0;

            virtual std::string toString() const = 0;
    };

    template<typename R, typename C, typename... A>
    class InvocationFuncDef : public InvocationFunc<R, A...> {
        private:
            C* base;
            R(C::*member)(A...);

        public:
            InvocationFuncDef(C *_base, R(C::*_member)(A...))
            : base(_base), member(_member) {
            }

            R invoke(A... args) const override {
                return (base->*member)(args...);
            }

            bool operator==(const InvocationFunc<R, A...>& rhs) const override
            {
                const InvocationFuncDef<R, C, A...> * prhs = static_cast<const InvocationFuncDef<R, C, A...>*>(&rhs);
                return base == prhs->base && member == prhs->member;
            }

            bool operator!=(const InvocationFunc<R, A...>& rhs) const override
            {
                const InvocationFuncDef<R, C, A...> * prhs = static_cast<const InvocationFuncDef<R, C, A...>*>(&rhs);
                return base != prhs->base || member != prhs->member;
            }

            std::string toString() const override {
                // hack to convert member pointer to void *: '*((void**)&member)'
                return uint64HexString((uint64_t)base)+"->"+aptrHexString( *((void**)&member) );
            }
    };

    template<typename R, typename... A>
    class ClassFunction {
        private:
            std::shared_ptr<InvocationFunc<R, A...>> func;

        public:
            ClassFunction(std::shared_ptr<InvocationFunc<R, A...>> _func)
            : func(_func) { }

            ClassFunction(const ClassFunction &o) = default;
            ClassFunction(ClassFunction &&o) = default;
            ClassFunction& operator=(const ClassFunction &o) = default;
            ClassFunction& operator=(ClassFunction &&o) = default;

            bool operator==(const ClassFunction<R, A...>& rhs) const
            { return *func == *rhs.func; }

            bool operator!=(const ClassFunction<R, A...>& rhs) const
            { return *func != *rhs.func; }

            std::string toString() const {
                return "ClassFunction["+func->toString()+"]";
            }

            R invoke(A... args) const {
                return func->invoke(args...);
            }
    };

    template<typename R, typename C, typename... A>
    inline ClassFunction<R, A...>
    bindClassFunction(C *base, R(C::*mfunc)(A...)) {
        return ClassFunction<R, A...>(
                std::shared_ptr<InvocationFunc<R, A...>>( new InvocationFuncDef<R, C, A...>(base, mfunc) )
               );
    }

} // namespace direct_bt

#endif /* CLASS_FUNCTION_HPP_ */
