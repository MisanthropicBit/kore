#ifndef KORE_VALUE_TYPE_HPP
#define KORE_VALUE_TYPE_HPP

#include <ostream>

#include "internal_value_types.hpp"
#include "targets/bytecode/vm/values/array_value.hpp"

namespace kore {
    namespace vm {
        enum class ValueTag {
            Bool = 0,
            I32,
            I64,
            F32,
            F64,
            Array,
            /* Str, */
        };

        /// The types for the vm's runtime values implemented
        /// as a tagged union
        struct Value {
            ValueTag tag;

            union _Value {
                bool _bool;
                i32 _i32;
                i64 _i64;
                f32 _f32;
                f64 _f64;
                ArrayValue* _array;
                /* std::string _str; */
            } value;

            ~Value();

            inline bool as_bool() const {
                #if KORE_VM_DEBUG
                if (tag != ValueTag::Bool) {
                    throw std::runtime_error("Not a boolean value");
                }
                #endif

                return value._bool;
            }

            inline i32 as_i32() const {
                #if KORE_VM_DEBUG
                if (tag != ValueTag::I32) {
                    throw std::runtime_error("Not an i32 value");
                }
                #endif

                return value._i32;
            }

            inline i64 as_i64() const {
                #if KORE_VM_DEBUG
                if (tag != ValueTag::I64) {
                    throw std::runtime_error("Not an i64 value");
                }
                #endif

                return value._i64;
            }

            inline f32 as_f32() const {
                #if KORE_VM_DEBUG
                if (tag != ValueTag::F32) {
                    throw std::runtime_error("Not an f32 value");
                }
                #endif

                return value._f32;
            }

            inline f64 as_f64() const {
                #if KORE_VM_DEBUG
                if (tag != ValueTag::F64) {
                    throw std::runtime_error("Not an f64 value");
                }
                #endif

                return value._f64;
            }

            /* inline std::string as_str() const { */
            /*     #if KORE_VM_DEBUG */
            /*     if (tag != ValueTag::Str) { */
            /*         throw std::runtime_error("Not a str value"); */
            /*     } */
            /*     #endif */

            /*     return value._str; */
            /* } */

            inline ArrayValue* as_array() {
                #if KORE_VM_DEBUG
                if (tag != ValueTag::Array) {
                    throw std::runtime_error("Not an array value");
                }
                #endif

                return value._array;
            }

            static Value from_bool(bool value);
            static Value from_i32(i32 value);
            static Value from_i64(i64 value);
            static Value from_f32(f32 value);
            static Value from_f64(f64 value);
            /* static Value from_string(const std::string& str); */
            static Value allocate_array(std::size_t size);
        };

        std::ostream& operator<<(std::ostream& out, const Value& value);
    }
}

#endif // KORE_VALUE_TYPE_HPP