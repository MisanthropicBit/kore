#include <queue>

#include "targets/bytecode/codegen/bytecode_codegen2.hpp"
#include "targets/bytecode/codegen/kir/block_id.hpp"
#include "targets/bytecode/codegen/kir/instruction.hpp"
#include "targets/bytecode/register.hpp"
#include "utils/endian.hpp"

#define KORE_MAKE_INSTRUCTION0(opcode) (\
    ((opcode & OPCODE_BITMASK) << OPCODE_SHIFT)\
)

#define KORE_ADD_REG(instruction, reg, position) (\
    (instruction) | ((reg & 0xff) << (16 - position * 8))\
)

#define KORE_MAKE_INSTRUCTION(opcode, reg, value) (\
    ((opcode & OPCODE_BITMASK) << OPCODE_SHIFT) |\
    ((reg & 0xff) << REG1_SHIFT) |\
    (value & VALUE_BITMASK)\
)

#define KORE_MAKE_INSTRUCTION2(opcode, reg1, reg2) (\
    ((opcode & OPCODE_BITMASK) << OPCODE_SHIFT) |\
    ((reg1 & 0xff) << REG1_SHIFT) |\
    ((reg2 & 0xff) << REG2_SHIFT)\
)

#define KORE_MAKE_INSTRUCTION3(opcode, reg1, reg2, reg3) (\
    (opcode & OPCODE_BITMASK) << OPCODE_SHIFT |\
    (reg1 & 0xff) << REG1_SHIFT |\
    (reg2 & 0xff) << REG2_SHIFT |\
    (reg3 & 0xff)\
)

#define KORE_MAKE_VALUE_INSTRUCTION(opcode, value) (\
    ((opcode & OPCODE_BITMASK) << OPCODE_SHIFT) | (value & VALUE_BITMASK)\
)

#define KORE_MAKE_REG_VALUE_INSTRUCTION(opcode, reg, value) (\
    ((opcode & OPCODE_BITMASK) << OPCODE_SHIFT) | ((reg & 0xff) << REG1_SHIFT) | (value & VALUE_BITMASK)\
)

namespace kore {
    static constexpr int VALUE_BITMASK = 0xffff;
    static constexpr int REG1_SHIFT = 16;
    static constexpr int REG2_SHIFT = 8;
    /* static constexpr int JUMP_BITMASK = VALUE_BITMASK; */
    static constexpr int OPCODE_BITMASK = 0xff;
    static constexpr int OPCODE_SHIFT = 24;

    std::map<TypeCategory, std::map<BinOp, Bytecode>> _binop_map2 = {
        {
            TypeCategory::Integer32, {
                {BinOp::Plus,     Bytecode::AddI32},
                {BinOp::Minus,    Bytecode::SubI32},
                {BinOp::Mult,     Bytecode::MultI32},
                {BinOp::Pow,      Bytecode::PowI32},
                {BinOp::Div,      Bytecode::DivI32},
                {BinOp::Lt,       Bytecode::LtI32},
                {BinOp::Gt,       Bytecode::GtI32},
                {BinOp::Le,       Bytecode::LeI32},
                {BinOp::Ge,       Bytecode::GeI32},
                {BinOp::Equal,    Bytecode::EqI32},
                {BinOp::NotEqual, Bytecode::NeqI32},
            },
        },
        {
            TypeCategory::Integer64, {
                {BinOp::Plus,     Bytecode::AddI64},
                {BinOp::Minus,    Bytecode::SubI64},
                {BinOp::Mult,     Bytecode::MultI64},
                {BinOp::Pow,      Bytecode::PowI64},
                {BinOp::Div,      Bytecode::DivI64},
                {BinOp::Lt,       Bytecode::LtI64},
                {BinOp::Gt,       Bytecode::GtI64},
                {BinOp::Le,       Bytecode::LeI64},
                {BinOp::Ge,       Bytecode::GeI64},
                {BinOp::Equal,    Bytecode::EqI64},
                {BinOp::NotEqual, Bytecode::NeqI64},
            },
        },
        {
            TypeCategory::Float32, {
                {BinOp::Plus,     Bytecode::AddF32},
                {BinOp::Minus,    Bytecode::SubF32},
                {BinOp::Mult,     Bytecode::MultF32},
                {BinOp::Pow,      Bytecode::PowF32},
                {BinOp::Div,      Bytecode::DivF32},
                {BinOp::Lt,       Bytecode::LtF32},
                {BinOp::Gt,       Bytecode::GtF32},
                {BinOp::Le,       Bytecode::LeF32},
                {BinOp::Ge,       Bytecode::GeF32},
                {BinOp::Equal,    Bytecode::EqF32},
                {BinOp::NotEqual, Bytecode::NeqF32},
            },
        },
        {
            TypeCategory::Float64, {
                {BinOp::Plus,     Bytecode::AddF64},
                {BinOp::Minus,    Bytecode::SubF64},
                {BinOp::Mult,     Bytecode::MultF64},
                {BinOp::Pow,      Bytecode::PowF64},
                {BinOp::Div,      Bytecode::DivF64},
                {BinOp::Lt,       Bytecode::LtF64},
                {BinOp::Gt,       Bytecode::GtF64},
                {BinOp::Le,       Bytecode::LeF64},
                {BinOp::Ge,       Bytecode::GeF64},
                {BinOp::Equal,    Bytecode::EqF64},
                {BinOp::NotEqual, Bytecode::NeqF64},
            },
        },
    };

    Bytecode get_binop_instruction(TypeCategory type_category, BinOp binop) {
        return _binop_map2[type_category][binop];
    }

    BytecodeGenerator2::BytecodeGenerator2() {}

    BytecodeGenerator2::~BytecodeGenerator2() {}

    std::vector<std::uint8_t> BytecodeGenerator2::generate(kir::Kir& kir) {
        _buffer.clear();

        // Magic bytes + compiler and bytecode versions
        write_bytes("kore");
        write_bytes({ 1, 0, 0 });
        write_bytes({ 1, 0, 0 });

        write_be32(kir.globals_count());

        auto main_module = kir.main_module();
        write_be32(main_module.index());
        write_be32(main_module.main_function().index());
        write_be32(kir.module_count());

        for (auto& module : kir) {
            generate_for_module(module);
        }

        patch_jumps();

        return _buffer;
    }

    void BytecodeGenerator2::generate_for_module(const kir::Module& module) {
        write_be32(module.index());
        write_string(module.path());
        write_constant_table<i32>(module.i32_constant_table());
        write_be32(module.function_count());

        for (int idx = 0; idx < module.function_count(); ++idx) {
            generate_for_function(module[idx]);
        }
    }

    void BytecodeGenerator2::generate_for_function(const kir::Function& function) {
        write_string(function.name());

        auto location = function.location();
        /* write_string(location.path()); */
        write_be32(location.lnum());
        write_be32(location.start());
        write_be32(location.end());

        write_be32(function.index());
        /* write_be32(function.locals_count()); */
        write_be32(function.max_regs_used());
        write_be32(function.code_size());

        auto graph = function.graph();

        if (graph.size() == 0) {
            return;
        }

        auto& source_block = graph[kir::BasicBlock::StartBlockId];
        std::queue<kir::BlockId> block_ids;
        block_ids.push(source_block.id);

        while (block_ids.size() > 0) {
            auto block_id = block_ids.front();
            block_ids.pop();

            if (block_id != kir::BasicBlock::StartBlockId) {
                generate_for_block(graph[block_id]);
            }

            auto begin = graph.successor_begin(block_id);
            auto end = graph.successor_end(block_id);

            for (auto it = begin; it < end; ++it) {
                block_ids.push(*it);
            }
        }
    }

    void BytecodeGenerator2::generate_for_block(kir::BasicBlock& block) {
        // TODO: This only works if the basic blocks are sequentially ordered
        _block_offsets.push_back(_buffer.size());

        for (auto& instruction : block.instructions) {
            generate_for_instruction(instruction);
        }
    }

    void BytecodeGenerator2::generate_for_instruction(kir::Instruction& instruction) {
        switch (instruction.type()) {
            case kir::InstructionType::LoadBool: {
                bool value = instruction.expr_as<BoolExpression>()->bool_value();

                write_be32(
                    KORE_MAKE_INSTRUCTION(
                        Bytecode::LoadBool,
                        instruction.reg1(),
                        value
                    )
                );
                break;
            }

            case kir::InstructionType::LoadInteger: {
                write_be32(
                    KORE_MAKE_INSTRUCTION(
                        Bytecode::CloadI32,
                        instruction.reg1(),
                        instruction.value()
                    )
                );
                break;
            }

            case kir::InstructionType::LoadGlobal: {
                write_be32(
                    KORE_MAKE_REG_VALUE_INSTRUCTION(
                        Bytecode::Gload,
                        instruction.reg1(),
                        instruction.value()
                    )
                );
                break;
            }

            case kir::InstructionType::Move: {
                write_be32(
                    KORE_MAKE_INSTRUCTION2(
                        Bytecode::Move,
                        instruction.reg1(),
                        instruction.reg2()
                    )
                );
                break;
            }

            case kir::InstructionType::Binop: {
                auto binexpr = instruction.expr_as<BinaryExpression>();
                auto opcode = get_binop_instruction(
                    binexpr->type()->category(),
                    binexpr->op()
                );

                write_be32(
                    KORE_MAKE_INSTRUCTION3(
                        opcode,
                        instruction.reg1(),
                        instruction.reg2(),
                        instruction.reg3()
                    )
                );
                break;
            }

            case kir::InstructionType::Branch: {
                // We write the basic block IDs directly in the jump offsets
                // now and patch them later when we know the instruction
                // offsets of all basic blocks
                write_be32(
                    KORE_MAKE_INSTRUCTION(
                        Bytecode::JumpIfNot,
                        instruction.reg1(),
                        instruction.bb1()
                    )
                );
                save_patch_location();

                write_be32(
                    KORE_MAKE_VALUE_INSTRUCTION(Bytecode::Jump, instruction.bb2())
                );

                save_patch_location();
                break;
            }

            case kir::InstructionType::AllocateArray: {
                write_be32(
                    KORE_MAKE_INSTRUCTION(
                        Bytecode::AllocArray,
                        instruction.reg1(),
                        instruction.value()
                    )
                );
                break;
            }

            case kir::InstructionType::BuiltinFunctionCall: {
                write_be32(
                    KORE_MAKE_INSTRUCTION3(
                        Bytecode::BuiltinCall,
                        instruction.reg1(),
                        instruction.reg2(),
                        instruction.reg3()
                    )
                );
                break;
            }

            case kir::InstructionType::Call: {
                // TODO: This should probably be a register containing the function index instead
                auto func_index = 0;

                auto arg_count = instruction.value();
                auto registers = instruction.registers();
                auto ret_count = registers.size() - arg_count;

                // TODO: Use return count of called function here (what about
                // variadic functions?)
                std::vector<std::uint8_t> bytes{
                    Bytecode::Call,
                    static_cast<std::uint8_t>(func_index),
                    static_cast<std::uint8_t>(arg_count),
                    static_cast<std::uint8_t>(ret_count),
                };

                if (arg_count > 0) {
                    bytes.insert(
                        bytes.end(),
                        registers.cbegin(),
                        registers.cbegin() + arg_count
                    );
                }

                if (ret_count > 0) {
                    bytes.insert(
                        bytes.end(),
                        registers.cbegin() + arg_count,
                        registers.cend()
                    );
                }

                write_bytes(bytes);
                break;
            }

            case kir::InstructionType::LoadFloat: {
                break;
            }

            case kir::InstructionType::Raw: {
                auto registers = instruction.registers();
                auto ins = KORE_MAKE_INSTRUCTION0(instruction.opcode());

                for (size_t i = 0; i < registers.size(); ++i) {
                    ins = KORE_ADD_REG(ins, registers[i], i);
                }

                write_be32(ins);

                break;
            }
        }
    }

    void BytecodeGenerator2::save_patch_location() {
        _patch_locations.push_back(_buffer.size() - 1);
    }

    void BytecodeGenerator2::patch_jumps() {
        for (auto location : _patch_locations) {
            auto instruction = _buffer[location];
            auto bb_offset = GET_VALUE(instruction);

            SET_VALUE(instruction, _block_offsets[bb_offset]);
        }
    }

    void BytecodeGenerator2::write_bytes(const std::string& str) {
        _buffer.insert(_buffer.end(), str.cbegin(), str.cend());
    }

    void BytecodeGenerator2::write_bytes(std::vector<std::uint8_t>& bytes) {
        _buffer.insert(_buffer.end(), bytes.cbegin(), bytes.cend());
    }

    void BytecodeGenerator2::write_bytes(std::initializer_list<std::uint8_t> bytes) {
        _buffer.insert(_buffer.end(), bytes);
    }

    void BytecodeGenerator2::write_string(const std::string& str) {
        if (str.empty()) {
            throw std::runtime_error("Attempt to write empty string");
        }

        if (str.size() >= std::numeric_limits<short>::max()) {
            // TODO: Move to common errors
            throw std::runtime_error("Can only write strings less than");
        }

        write_be32(str.size());

        // TODO: Will this work for utf-8 strings?
        // Do not include the null-terminator
        write_bytes(str);
    }

    void BytecodeGenerator2::write_raw(std::uint32_t value) {
        _buffer.insert(
            _buffer.end(),
            {
                static_cast<std::uint8_t>((value >> 24) & 0xff),
                static_cast<std::uint8_t>((value >> 16) & 0xff),
                static_cast<std::uint8_t>((value >> 8) & 0xff),
                static_cast<std::uint8_t>(value & 0xff)
            }
        );
    }

    void BytecodeGenerator2::write_be32(std::uint32_t value) {
        ::kore::write_be32(value, _buffer);
    }
}

#undef KORE_MAKE_INSTRUCTION
#undef KORE_MAKE_INSTRUCTION2
#undef KORE_MAKE_INSTRUCTION3
#undef KORE_MAKE_VALUE_INSTRUCTION
#undef KORE_MAKE_REG_VALUE_INSTRUCTION