#include "ast/ast_writer.hpp"
#include "types/array_type.hpp"
#include "types/unknown_type.hpp"

namespace kore {
    ArrayType::ArrayType()
        : _element_type(Type::unknown()) {
    }

    ArrayType::ArrayType(const Type* element_type)
        : Type(TypeCategory::Array),
        _rank(1),
        _element_type(std::move(element_type)) {
    }

    ArrayType::~ArrayType() {}

    void ArrayType::set_rank(int rank) {
        _rank = rank;
    }

    void ArrayType::increase_rank() {
        ++_rank;
    }

    int ArrayType::rank() const noexcept {
        return _rank;
    }

    const Type* ArrayType::element_type() const {
        return _element_type;
    }

    const Type* ArrayType::unify(const Type* other_type) const {
        switch (other_type->category()) {
            case TypeCategory::Array:
                return other_type->unify(this);

            default:
                return Type::unknown();
        }
    }

    const Type* ArrayType::unify(const ArrayType* array_type) const {
        return _element_type->unify(array_type->_element_type);
    }

    void ArrayType::unify_element_type(const Type* type) {
        _element_type = _element_type->unify(type);
    }

    std::string ArrayType::name() const {
        std::string name = _element_type->name();

        for (int i = 0; i < rank(); ++i) {
            name += "[]";
        }

        return name;
    }

    void ArrayType::write(AstWriter* const writer) const {
        /* _element_type->write(writer); */

        for (int i = 0; i < _rank; ++i) {
            writer->write("[]");
        }
    }
}
