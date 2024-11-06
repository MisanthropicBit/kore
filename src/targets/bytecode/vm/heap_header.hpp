#ifndef KORE_HEAP_HEADER_HPP
#define KORE_HEAP_HEADER_HPP

namespace kore {
    namespace vm {
        struct HeapHeader {
            unsigned int refcount;
        };
    }
}

#endif // KORE_HEAP_HEADER_HPP
