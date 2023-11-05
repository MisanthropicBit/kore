#include "targets/bytecode/codegen/kir/module.hpp"

namespace kore {
    namespace kir {
        Module::Module(
            ModuleIndex index,
            const fs::path& path
        ) : _index(index),
            _path(path),
            _global_indices(0),
            _i32_constants(ConstantTableTag::I32),
            _i64_constants(ConstantTableTag::I64),
            _f32_constants(ConstantTableTag::F32),
            _f64_constants(ConstantTableTag::F64),
            _str_constants(ConstantTableTag::Str)
        {}

        Module::~Module() {}

        std::string Module::path() const {
            return _path;
        }

        ModuleIndex Module::index() const noexcept {
            return _index;
        }

        int Module::add_function() {
            return add_function(nullptr);
        }

        int Module::add_function(const kore::Function* function) {
            auto func_index = _functions.size();
            _functions.push_back(Function(func_index, function));

            return func_index;
        }

        Function& Module::operator[](int index) {
            return _functions.at(index);
        }

        const Function& Module::operator[](int index) const {
            return _functions.at(index);
        }

        Function& Module::main_function() {
            return _functions.back();
        }

        int Module::function_count() const noexcept {
            return _functions.size();
        }

        int Module::constants_count() const noexcept {
            return _i32_constants.size() + _i64_constants.size() + _f32_constants.size() + _f64_constants.size();
        }

        int Module::global_count() const noexcept {
            return _global_indices;
        }

        Module::iterator Module::begin() {
            return _functions.begin();
        }

        Module::iterator Module::end() {
            return _functions.end();
        }

        Module::const_iterator Module::cbegin() const {
            return _functions.cbegin();
        }

        Module::const_iterator Module::cend() const {
            return _functions.cend();
        }

        ConstantTable<i32>& Module::i32_constant_table() {
            return _i32_constants;
        }

        ConstantTable<i64>& Module::i64_constant_table() {
            return _i64_constants;
        }

        ConstantTable<f32>& Module::f32_constant_table() {
            return _f32_constants;
        }

        ConstantTable<f64>& Module::f64_constant_table() {
            return _f64_constants;
        }

        ConstantTable<std::string>& Module::str_constant_table() {
            return _str_constants;
        }

        const ConstantTable<i32>& Module::i32_constant_table() const {
            return _i32_constants;
        }

        const ConstantTable<i64>& Module::i64_constant_table() const {
            return _i64_constants;
        }

        const ConstantTable<f32>& Module::f32_constant_table() const {
            return _f32_constants;
        }

        const ConstantTable<f64>& Module::f64_constant_table() const {
            return _f64_constants;
        }

        const ConstantTable<std::string>& Module::str_constant_table() const {
            return _str_constants;
        }
    }
}