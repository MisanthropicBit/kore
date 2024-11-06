#include "targets/bytecode/vm/heap.hpp"

namespace kore {
    namespace vm {
        void* Heap::allocate_raw(std::size_t size) {
            void* mem = malloc(size);

            if (!mem) {
                throw std::runtime_error("Out of memory");
            }

            return mem;
        }

        Value Heap::allocate_array(std::size_t size) {
            auto value = Value();

            // TODO: Handle allocation failure
            auto array_value = new ArrayValue{ size };
            array_value->heap_header().refcount = 1;

            value.tag = ValueTag::Array;
            value.value._array = array_value;

            return value;
        }

        void Heap::deallocate_raw(void* mem) {
            free(mem);
        }

        void Heap::deallocate_array(Value& value, bool ignore_refcount) {
            if (value.tag != ValueTag::Array) {
                throw std::runtime_error("Attempt to free non-array value");
            }

            auto array_value = value.as_array();

            if (!array_value) {
                throw std::runtime_error("Attempt to free array value that has already been freed");
            }

            // Sometimes we want to deallocate a function-local array where
            // only function-local references refer to it and the array does
            // not escape so we can just deallocate it
            if (!ignore_refcount && array_value->heap_header().refcount > 0) {
                throw std::runtime_error("Attempt to free array value with a non-zero reference count");
            }

            array_value->clear();
        }

        void Heap::deallocate_array(ArrayValue* array_value, bool ignore_refcount) {
            if (!array_value) {
                throw std::runtime_error("Attempt to free array value that has already been freed");
            }

            // Sometimes we want to deallocate a function-local array where
            // only function-local references refer to it and the array does
            // not escape so we can just deallocate it
            if (!ignore_refcount && array_value->heap_header().refcount > 0) {
                throw std::runtime_error("Attempt to free array value with a non-zero reference count");
            }

            array_value->clear();

            delete array_value;
        }
    }
}
