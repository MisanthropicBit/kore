#ifndef KORE_VM_HPP
#define KORE_VM_HPP

#include "targets/bytecode/codegen/bytecode.hpp"
#include "targets/bytecode/vm/heap.hpp"
#include "targets/bytecode/vm/value_type.hpp"
#include "targets/bytecode/vm/config.hpp"
#include "targets/bytecode/register.hpp"
#include "targets/bytecode/module.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace kore {
    namespace vm {
        struct CallFrame {
            CallFrame(CompiledObject* obj);

            CallFrame(
                int ret_count,
                int reg_count,
                const bytecode_type* code,
                std::size_t size
            );

            CallFrame(
                int ret_count,
                int shift,
                std::size_t old_fp,
                std::size_t old_pc,
                const CompiledObject* obj
            );

            int ret_count;
            int reg_count;
            int shift;
            std::size_t old_fp;
            std::size_t old_pc;

            // Pointer to instructions currently being executed
            const bytecode_type* code;
            std::size_t size;
        };

        struct Context {
            std::size_t pc = 0; // Program counter
            std::size_t sp = 0; // Stack pointer
            std::size_t fp = 0; // Frame pointer for current call frame
            int shift = 0;

            Module* _current_module;

            void reset();
            void restore(const CallFrame& call_frame);

            /* void set_from_module(const Module& module) { */
            /*     reset(); */
            /*     code = module.main_object().code(0); */
            /* } */
        };

        /// Kore's register-based virtual machine
        class Vm final {
            public:
                Vm();
                ~Vm();

                /// Run the code in a sized array
                void run(const bytecode_type* code, std::size_t size);

                /// Run the code in a vector
                void run(const std::vector<bytecode_type>& code);

                /// Run a compiled object
                /* void run(const CompiledObject* obj) */

                void run_path(const fs::path& path);
                void run_module(Module& module);
                void run_compiled_object(CompiledObject* obj);

                void dump_registers(std::ostream& os);

                /// Set a return value from builtin function
                void set_return_value(const Value& value);

                void vm_fatal_error(const std::string& message);

            private:
                bool _running = false;
                bool _errored = false;
                Context _context;
                Value _registers[KORE_VM_MAX_REGISTERS];
                std::vector<CallFrame> _call_frames;

                static const std::string log_group;

                // Saved for dumping the main function's registers
                int main_call_frame_reg_count;

                // Map of loaded modules
                std::unordered_map<std::string, Module> _modules;

                // Vector of loaded functions from all loaded modules for quick access
                std::vector<CompiledObject*> _loaded_functions;

                // Table of global values
                std::vector<Value> _globals;

                // The virtual machine's heap
                Heap _heap;

            private:
                /// Load all functions from a module
                void load_functions_from_module(const Module& module);

                /// Allocate space for globals
                void allocate_globals(const Module& module);

                /// Reserve local stack space for a called function
                bool allocate_local_stack(const CompiledObject* const obj);

                /// Deallocate local stack space for a call frame
                void deallocate_local_stack(const CallFrame& call_frame);

                void vm_error(const std::string& message);
                void vm_error_unknown_opcode(Bytecode opcode);

                /// Get a function by its index
                CompiledObject* get_function(int func_index);

                /// Get the current call frame
                CallFrame& current_frame();

                /// Push an i32 value onto a call frame's stack
                inline void push_i32(i32 value);

                /// Push a Value onto a call frame's stack
                inline void push(Value& value);

                /// Push the value of a register onto the current call frame
                inline void push_register(Reg reg);

                /// When in a function call, get the caller of that function
                inline const CallFrame& get_caller();

                void push_call_frame(CallFrame call_frame);

                int push_function_arguments(bytecode_type instruction, std::size_t old_fp);

                /// Push a new call frame
                void do_function_call(
                    bytecode_type instruction,
                    const FunctionValue& callable
                );

                void do_builtin_function_call(
                    bytecode_type instruction,
                    const FunctionValue& callable
                );

                /// Pop the current call frame
                void do_function_return(bytecode_type instruction);

                /// Pop a value from a call frame's stack
                inline Value pop();

                /// Get the position of the top of the stack
                inline int top();

                /// Move a value from a source register to a destination register
                inline void move(Reg src_reg, Reg dst_reg);

                void add_module(Module& module);

                void dump_call_stack(std::ostream& os);
        };
    }
}

#endif // KORE_VM_HPP
