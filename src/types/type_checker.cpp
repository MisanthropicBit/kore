#include "type_checker.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/statements/branch.hpp"
#include "ast/statements/statement.hpp"
#include "ast/statements/variable_assignment.hpp"
#include "ast/statements/variable_declaration.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostics.hpp"
#include "errors/errors.hpp"
#include "logging/logging.hpp"
#include "targets/bytecode/vm/builtins/builtins.hpp"
#include "types/array_type.hpp"
#include "types/function_type.hpp"
#include "types/scope.hpp"
#include "utils/unused_parameter.hpp"

#include <iostream>
#include <sstream>

namespace kore {
    TypeChecker::TypeChecker(const ParsedCommandLineArgs& args) : _args(&args) {}

    TypeChecker::~TypeChecker() {}

    int TypeChecker::check(const Ast& ast) {
        _diagnostics.clear();

        for (auto const& statement : ast) {
            statement->accept(*this);
        }

        return _diagnostics.size();
    }

    void TypeChecker::trace_type_checker(
        const std::string& name,
        const Type* type1,
        const Type* type2
    ) {
        if (_args && _args->trace == TraceOption::TypeCheck) {
            std::string msg = name;
            const std::string group = "type_check";

            if (type1) {
                msg += ": " + type1->name();
            }

            if (type2) {
                msg += ", " + type2->name();
            }

            debug_group(group, "%s", msg.c_str());
        }
    }

    std::vector<Diagnostic> TypeChecker::diagnostics() {
        return _diagnostics;
    }

    void TypeChecker::push_error(DiagnosticData&& data) {
        if (_error_threshold != _NO_ERROR_THRESHOLD && static_cast<int>(_diagnostics.size()) >= _error_threshold) {
            return;
        }

        _diagnostics.emplace_back(DiagnosticType::TypeCheck, DiagnosticLevel::Error, data);
    }

    void TypeChecker::visit(ArrayExpression& array) {
        auto element_type = static_cast<const ArrayType*>(array.type())->element_type();

        // We typecheck an array expression (like [1, 2, 3]) by seeing if we can
        // unify all element types with the inferred or given type of the array
        for (int idx = 0; idx < array.size(); ++idx) {
            auto element_index_type = array[idx]->type();

            if (element_index_type->unify(element_type)->is_unknown()) {
                push_error(IncompatibleTypes{ &array, array[idx] });
            }
        }
    }

    void TypeChecker::visit(IndexExpression& index_expr) {
        // TODO: Typecheck this when it may not be an array
        auto index_expr_type = index_expr.index_expr()->type();

        // We typecheck an array index expression (like a[0]) by checking that
        // the index type is a 32- or 64-bit integer
        if (!index_expr_type->is_numeric()) {
            push_error(ArrayIndexNotNumeric{ &index_expr, index_expr.index_expr() });
        } else {
            auto category = index_expr_type->category();

            if (category != TypeCategory::Integer32 && category != TypeCategory::Integer64) {
                push_error(ArrayIndexNeedsCast{ &index_expr, index_expr.index_expr() });
            }
        }
    }

    void TypeChecker::visit(IndexExpression& array_index, ValueContext context) {
        UNUSED_PARAM(context);
        visit(array_index);
    }

    void TypeChecker::visit(Identifier& identifier) {
        auto entry = _scope_stack.find(identifier.name());

        if (entry) {
            trace_type_checker("identifier", entry->identifier->type());

            identifier.set_type(entry->identifier->type());
        } else {
            push_error(UndefinedVariable{ &identifier });
        }
    }

    void TypeChecker::visit(Identifier& identifier, ValueContext context) {
        if (context == ValueContext::RValue) {
            visit(identifier);
            return;
        }

        // Lvalue context: Check if the identifier can be assigned to or would
        // shadow an identifier with the same name in an upper scope
        auto entry = _scope_stack.find_inner(identifier.name());

        if (!entry) {
            if (_scope_stack.is_global_scope() && identifier.is_mutable()) {
                push_error(CannotDeclareMutableGlobal{ &identifier });
            } else {
                auto shadowed_entry = _scope_stack.find(identifier.name());

                if (shadowed_entry) {
                    auto shadowed_identifier = shadowed_entry->identifier;

                    if (shadowed_entry->is_global_scope()) {
                        push_error(
                            CannotAssignGlobalVariable{
                                &identifier,
                                shadowed_identifier,
                            }
                        );
                    } else {
                        push_error(
                            VariableShadows{
                                &identifier,
                                shadowed_identifier
                            }
                        );
                    }
                }

                _scope_stack.insert(&identifier);
            }
        } else {
            // Variable already exists in this scope, check that we are not
            // redeclaring a constant variable
            if (!entry->identifier->is_mutable()) {
                push_error(ConstantVariableRedeclaration{ &identifier, entry->identifier });
            }
        }
    }

    void TypeChecker::visit(VariableAssignment& assignment) {
        // Visit the right-hand side expression
        assignment.rhs()->accept(*this);

        auto declared_type = assignment.declared_type();
        auto expr_type = assignment.rhs()->type();

        trace_type_checker("assignment", declared_type, expr_type);

        // If the variable was not given an explicit type, rely on inferred
        // type instead
        if (!declared_type->is_unknown()) {
            if (declared_type->unify(expr_type)->is_unknown()) {
                push_error(CannotAssign{ &assignment });
                return;
            }
        }

        auto lhs = assignment.lhs();
        lhs->accept(*this, ValueContext::LValue);

        if (lhs->type()->unify(assignment.rhs()->type())->is_unknown()) {
            push_error(CannotAssign{ &assignment });
        }
    }

    void TypeChecker::visit(class Call& call) {
        trace_type_checker("call", call.type());

        // Find an scoped entry in the current or enclosing scopes for a
        // function definition
        auto entry = _scope_stack.find(call.name());

        if (!entry) {
            auto builtin_function = vm::get_builtin_function_by_name(call.name());

            if (!builtin_function) {
                push_error(UnknownCall{ &call });
                return;
            }

            // TODO: Typecheck builtin function calls

            if (call.arg_count() != builtin_function->arity) {
                /* push_error(errors::typing::incorrect_arg_count( */
                /*     call, */
                /*     func_type */
                /* )); */

                return;
            }

            call.set_type(builtin_function->ret_types[0]);

            return;
        }

        auto type = entry->identifier->type();

        if (type->category() != TypeCategory::Function) {
            push_error(CannotCallNonFunction{ &call, type });
            return;
        }

        auto func_type = static_cast<const FunctionType*>(type);

        if (call.arg_count() != func_type->arity()) {
            push_error(IncorrectArgumentCount{ &call, func_type });
            return;
        }

        trace_type_checker("call", func_type);

        for (int i = 0; i < call.arg_count(); ++i) {
            auto arg = call.arg(i);
            arg->accept(*this);

            auto arg_type = arg->type();
            auto param_type = func_type->parameter(i)->type();
            auto unified_type = arg_type->unify(param_type);

            if (unified_type->is_unknown()) {
                push_error(IncorrectArgumentType{ &call, arg, param_type, i });
            }
        }

        call.set_type(func_type->return_type());
    }

    void TypeChecker::visit(BinaryExpression& binexpr) {
        auto left = binexpr.left();
        auto right = binexpr.right();

        left->accept(*this);
        right->accept(*this);

        trace_type_checker("binop", left->type(), right->type());

        auto op = binexpr.op();
        auto left_type = left->type();
        auto right_type = right->type();
        auto left_type_compatible = compatible_binop_type(op, left_type);
        auto right_type_compatible = compatible_binop_type(op, right_type);

        // Check if types are compatible with the binary operation
        if (!left_type_compatible) {
            push_error(
                IncompatibleBinaryOperation{
                    left,
                    right,
                    op,
                    IncompatibleBinaryOperation::Context::Left,
                }
            );
        }

        if (!right_type_compatible) {
            push_error(
                IncompatibleBinaryOperation{
                    left,
                    right,
                    op,
                    IncompatibleBinaryOperation::Context::Right,
                }
            );
        }

        auto result_type = left_type->unify(right_type);

        // Handle type unification errors
        if (left_type_compatible && right_type_compatible) {
            if (result_type->is_unknown()) {
                push_error(IncompatibleBinaryOperation{
                    left,
                    right,
                    op,
                    IncompatibleBinaryOperation::Context::Both
                });
            } else {
                binexpr.set_type(result_type);
            }
        }
    }

    void TypeChecker::visit(Branch& branch) {
        trace_type_checker("branch");

        _scope_stack.enter();

        if (branch.condition()) {
            branch.condition()->accept(*this);
        }

        for (auto& statement : branch) {
            statement->accept(*this);
        }

        _scope_stack.leave();
    }

    void TypeChecker::visit(IfStatement& statement) {
        for (auto& branch : statement) {
            auto condition = branch->condition();

            if (condition) {
                condition->accept(*this);
            }

            for (auto& statement : *branch) {
                statement->accept(*this);
            }
        }
    }

    void TypeChecker::visit(Function& func) {
        trace_type_checker("function", func.type());

        // Enter a new function scope and add all function
        // arguments to that scope
        _scope_stack.enter_function_scope(&func);

        for (int i = 0; i < func.arity(); ++i) {
            auto parameter = func.parameter(i);
            _scope_stack.insert(parameter);
        }

        for (auto& statement : func) {
            statement->accept(*this);
        }

        _scope_stack.leave_function_scope();

        // After leaving the function scope, bind the function type
        // to the function name (not necessarily in the top-level scope
        // since we want to support nested functions)
        _scope_stack.insert(func.identifier());
    }

    void TypeChecker::visit(Return& ret) {
        trace_type_checker("return");

        auto func = _scope_stack.enclosing_function();

        if (ret.expr()) {
            auto expr = ret.expr();
            expr->accept(*this);

            if (expr->type()->unify(func->return_type())->is_unknown()) {
                push_error(ReturnTypeMismatch{ func, &ret });
                return;
            }
            push_error(VoidReturnFromNonVoidFunction{ func, &ret });
        } else {
            if (!func->return_type()->is_void()) {
            }
        }
    }
}
