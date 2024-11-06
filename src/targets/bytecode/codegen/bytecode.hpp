#ifndef KORE_BYTECODE_HPP
#define KORE_BYTECODE_HPP

#include <cstdint>
#include <ostream>

namespace kore {
    /// Bytecodes in kore are 32-bit integer with a
    /// 1, 2 or 3 operand layout:
    ///
    /// 1 operand layout:
    ///
    /// Used for opcodes that use a single register.
    ///
    ///    8 bits           16 bits           8 bits
    /// +----------+-----------------------+-----------+
    /// |  opcode  |         unused        |  operand  |
    /// +----------+-----------------------+-----------+
    ///
    /// 2 operand layout:
    ///
    /// Used for loads where operand 1 is the target register
    /// and operand 2 is a constant index or an immediate value.
    ///
    ///    8 bits     8 bits           16 bits
    /// +----------+-----------+-----------------------+
    /// |  opcode  | operand 1 |      operand 2        |
    /// +----------+-----------+-----------------------+
    ///
    /// 3 operand layout:
    ///
    /// Used for opcodes operating on 3 registers such as the
    /// AddI32 instruction that adds the 32-bit integers in
    /// the registers of operand 2 and 3 and stores the result
    /// in the register of operand 1. For example,
    ///
    /// AddI32 r1 r2 r3
    ///
    ///    8 bits     8 bits      8 bits      8 bits
    /// +----------+-----------+-----------+-----------+
    /// |  opcode  | operand 1 | operand 2 | operand 3 |
    /// +----------+-----------+-----------+-----------+
    // TODO: Change this name to 'instruction'
    using bytecode_type = std::uint32_t;

    /// For example the following statement
    ///
    /// a = b + c
    ///
    /// where a, b, and c are 32-bit integers, a is stored
    /// in register 1 (r1), b in r2, / and c in r3, is compiled as
    ///
    /// addi32 r1 r2 r3

    enum Bytecode {
        Noop,
        Move,

        // Loads/stores
        Gload,
        Gstore,
        LoadBool,
        Cload,
        LoadFunction,

        // Load locals (TODO: Remove?)
        LloadI32,
        LloadI64,
        LloadF32,
        LloadF64,

        // Arithmetic opcodes
        AddI32,
        AddI64,
        AddF32,
        AddF64,
        SubI32,
        SubI64,
        SubF32,
        SubF64,
        MultI32,
        MultI64,
        MultF32,
        MultF64,
        PowI32,
        PowI64,
        PowF32,
        PowF64,
        DivI32,
        DivI64,
        DivF32,
        DivF64,

        // Comparison opcodes
        LtI32,
        LtI64,
        LtF32,
        LtF64,
        GtI32,
        GtI64,
        GtF32,
        GtF64,
        LeI32,
        LeI64,
        LeF32,
        LeF64,
        GeI32,
        GeI64,
        GeF32,
        GeF64,
        EqI32,
        EqI64,
        EqF32,
        EqF64,
        NeqI32,
        NeqI64,
        NeqF32,
        NeqF64,

        // Arrays
        ArrayAlloc,
        ArrayFree,
        ArrayGet,
        ArraySet,

        // Memory operations
        RefInc,
        RefDec,
        Destroy,
        Free,

        // Control flow
        Jump,
        JumpIf,
        JumpIfNot,

        // Function calls
        Call,
        Ret
    };

    std::string bytecode_to_string(Bytecode bytecode);

    Bytecode int_to_bytecode(int opcode);

    bool is_variable_length_opcode(Bytecode bytecode);
    bool is_variable_length_instruction(bytecode_type instruction);

    std::ostream& operator<<(std::ostream& os, const Bytecode bytecode);
}

#endif // KORE_BYTECODE_HPP
