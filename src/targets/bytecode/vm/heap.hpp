#ifndef KORE_HEAP
#define KORE_HEAP

#include <cstddef>

#include "targets/bytecode/vm/value_type.hpp"

namespace kore {
    namespace vm {
        // An abstraction of kore's vm heap. Currently uses the system/host
        // language allocator (malloc/free)
        class Heap {
            public:
                Heap();
                virtual ~Heap();

                void* allocate_raw(size_t size);
                Value allocate_array(size_t size);
                /* FunctionValue allocate_closure(); */

                void deallocate_raw(void* mem);
                void deallocate_array(Value& array_value, bool ignore_refcount);
                void deallocate_array(ArrayValue* array_value, bool ignore_refcount);
                /* FunctionValue deallocate_closure(); */
        };
    }
}

#endif // KORE_HEAP
