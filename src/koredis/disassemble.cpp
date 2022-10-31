#include <array>

#include "disassemble.hpp"
#include "disassemble_error.hpp"
#include "utils/endian.hpp"

// TODO: Add checks for stream reads
namespace koredis {
    std::array<char, 4> bytecode_magic {'k', 'o', 'r', 'e'};

    std::string read_string(std::istream& is) {
        auto size = kore::read_be16(is);

        // NOTE: Use std::byte if we end up using C++17
        std::vector<char> bytes(size);

        is.read(bytes.data(), size);

        //data.resize(is.gcount());

        return std::string(bytes.cbegin(), bytes.cend());
    } 

    void read_magic(std::istream& is) {
        std::array<char, 4> magic_header;

        is.read(magic_header.data(), 4);

        if (magic_header != bytecode_magic) {
            throw DisassembleError("not a kore bytecode file", is.tellg());
        }
    }

    std::array<char, 3> read_compiler_version(std::istream& is) {
        std::array<char, 3> compiler_version;

        is.read(compiler_version.data(), 3);

        return compiler_version;
    }

    std::array<char, 3> read_bytecode_version(std::istream& is) {
        std::array<char, 3> bytecode_version;

        is.read(bytecode_version.data(), 3);

        return bytecode_version;
    }

    void disassemble_constant_table(std::ifstream& ifs, kore::Module& module) {
        /* auto constant_table_tag = kore::read_be32(ifs); */
        auto constant_table_size = kore::read_be32(ifs);

        for (decltype(constant_table_size) i = 0; i < constant_table_size; ++i) {
            // TODO: Fix signedness of values
            module.add_i32_constant(kore::read_be32(ifs));
        }
    }

    void disassemble_constant_tables(std::ifstream& ifs, kore::Module& module) {
        disassemble_constant_table(ifs, module);
    }

    void disassemble_function(std::ifstream& ifs, kore::Module& module) {
        std::string name = read_string(ifs);

        // TODO: Why do we do this?
        /* if (name != "main") { */
            auto lnum = kore::read_be32(ifs);
            auto start = kore::read_be32(ifs);
            auto end = kore::read_be32(ifs);
        /* } */

        auto locals_count = kore::read_be32(ifs);
        auto reg_count = kore::read_be32(ifs);
        auto code_size = kore::read_be32(ifs);
        std::vector<kore::bytecode_type> instructions;

        for (decltype(code_size) i = 0; i < code_size; ++i) {
            instructions.push_back(kore::read_be32(ifs));
        }

        module.add_function(name, lnum, start, end, locals_count, reg_count, instructions);
    }

    void disassemble_functions(std::ifstream& ifs, kore::Module& module) {
        std::uint32_t function_count = kore::read_be32(ifs);

        for (std::uint32_t i = 0; i < function_count; ++i) {
            disassemble_function(ifs, module);
        }
    }

    kore::Module disassemble_module_from_path(const std::string& path) {
        std::ifstream ifs{path};

        if (!ifs.is_open()) {
            throw DisassembleError("failed to open file");
        }

        // Read magic header
        read_magic(ifs);

        // Read compiler/bytecode versions
        kore::Module module;
        module.set_compiler_version(read_compiler_version(ifs));
        module.set_bytecode_version(read_bytecode_version(ifs));

        // Read constant tables
        disassemble_constant_tables(ifs, module);

        // Read functions
        disassemble_functions(ifs, module);

        return module;
    }
}