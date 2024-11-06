#ifndef KORE_ARRAY_TYPE_HPP
#define KORE_ARRAY_TYPE_HPP

#include "types/type.hpp"

namespace kore {
    class ArrayType : public Type {
        public:
            ArrayType();
            ArrayType(const Type* element_type);
            virtual ~ArrayType();

            void set_rank(int rank);
            void increase_rank();
            int rank() const noexcept;
            const Type* element_type() const;
            const Type* unify(const Type* other_type) const override;
            const Type* unify(const ArrayType* array_type) const override;
            void unify_element_type(const Type* type);

            std::string name() const override;

            void write(AstWriter* const writer) const override;

        private:
            int _rank;
            const Type* _element_type;
    };
}

#endif // KORE_ARRAY_TYPE_HPP
