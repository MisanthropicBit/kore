#include "backend/codegen/bytecode/bytecode.hpp"

namespace kore {
    std::string bytecode_to_string(Bytecode bytecode) {
        switch (bytecode) {
            case Noop:           return "noop";
            case Move:           return "move";
            case GloadI32:       return "gloadi32";
            case GloadI64:       return "gloadi64";
            case GloadF32:       return "gloadf32";
            case GloadF64:       return "gloadf64";
            case StoreI32Global: return "gstorei32";
            case LoadBool:       return "loadbool";
            case CloadI32:       return "cloadi32";
            case CloadI64:       return "cloadi64";
            case CloadF32:       return "cloadf32";
            case CloadF64:       return "cloadf64";
            case LloadI32:       return "lloadi32";
            case LloadI64:       return "lloadi64";
            case LloadF32:       return "lloadf32";
            case LloadF64:       return "lloadf64";
            case AddI32:         return "addi32";
            case AddI64:         return "addi64";
            case AddF32:         return "addf32";
            case AddF64:         return "addf64";
            case SubI32:         return "subi32";
            case SubI64:         return "subi64";
            case SubF32:         return "subf32";
            case SubF64:         return "subf64";
            case MultI32:        return "multi32";
            case MultI64:        return "multi64";
            case MultF32:        return "multf32";
            case MultF64:        return "multf64";
            case PowI32:         return "powi32";
            case PowI64:         return "powi64";
            case PowF32:         return "powf32";
            case PowF64:         return "powf64";
            case DivI32:         return "divi32";
            case DivI64:         return "divi64";
            case DivF32:         return "divf32";
            case DivF64:         return "divf64";
            case LtI32:          return "lti32";
            case LtI64:          return "lti64";
            case LtF32:          return "ltf32";
            case LtF64:          return "ltf64";
            case GtI32:          return "gti32";
            case GtI64:          return "gti64";
            case GtF32:          return "gtf32";
            case GtF64:          return "gtf64";
            case LeI32:          return "lei32";
            case LeI64:          return "lei64";
            case LeF32:          return "lef32";
            case LeF64:          return "lef64";
            case GeI32:          return "gei32";
            case GeI64:          return "gei64";
            case GeF32:          return "gef32";
            case GeF64:          return "gef64";
            case EqI32:          return "eqi32";
            case EqI64:          return "eqi64";
            case EqF32:          return "eqf32";
            case EqF64:          return "eqf64";
            case NeqI32:         return "neqi32";
            case NeqI64:         return "neqi64";
            case NeqF32:         return "neqf32";
            case NeqF64:         return "neqf64";
            case Jump:           return "jump";
            case JumpIf:         return "jumpif";
            case JumpIfNot:      return "jumpifnot";
            case Call:           return "call";
            case Ret:            return "return";
            case RetReg:         return "retreg";
        }
    }

    Bytecode int_to_bytecode(int opcode) {
        return static_cast<kore::Bytecode>(opcode);
    }

    std::ostream& operator<<(std::ostream& os, const Bytecode bytecode) {
        return os << static_cast<bytecode_type>(bytecode);
    }
}
