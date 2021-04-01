#include <iostream>
#include <sstream>
#include <vector>

#include "ast/parser_error_node.hpp"
#include "logging.hpp"
#include "operator.hpp"
#include "parser.hpp"

Parser::Parser() : _failed(false), _did_peek(false) {}

Parser::~Parser() {}

bool Parser::failed() const noexcept {
    return _failed;
}

Token* Parser::current_token() {
const Token* Parser::current_token() {
    return &this->_current_token;
}

const Token* Parser::peek_token() {
    if (!_did_peek) {
        _peek_token = _scanner.next_token();
        _did_peek = true;
    }

    return &this->_peek_token;
}

const Token* Parser::next_token() {
    if (_did_peek) {
        _did_peek = false;
        _current_token = _peek_token;

        return &_current_token;
    }

    do {
        _current_token = _scanner.next_token();
    } while (_current_token.category() == TokenCategory::comment);

    return &_current_token;
}

bool Parser::expect_named_identifier(const std::string& name) {
    const Token* const token = current_token();

    if (token->is_identifier() && token->value() == name) {
        next_token();

        return true;
    }

    return false;
}

bool Parser::expect_keyword(const Keyword& keyword) {
    const Token* const token = current_token();

    if (token->type() == TokenType::keyword && (token->value() == "module" || token->value() == "import")) {
        next_token();

        return true;
    }

    return false;
}

bool Parser::expect_token_type(const TokenType& token_type) {
    const Token* const token = current_token();

    if (token->type() == token_type) {
        next_token();

        return true;
    }

    return false;
}

void Parser::emit_parser_error(const char* const format, ...) {
    _failed = true;

    /* const std::string error = format_error("", _scanner.current_line(), current_token()->location()); */

    va_list args;
    va_start(args, format);
    parser_error(current_token()->location(), format, args);
    va_end(args);
}

void Parser::set_module_name(const std::string& module_name) {
    _module_name = module_name;
}

void Parser::add_statement(Statement* statement) {
    // Add the statement to the current function we are parsing if there is
    // one, otherwise add it to the top-level scope
    if (_current_function) {
        _current_function->add_statement(statement);
    } else {
        _ast->add_statement(statement);
    }
}

std::string Parser::module_name() const {
    return _module_name;
}

Expression* Parser::make_parser_error(const std::string& msg) {
    return Expression::make_parser_error(msg, current_token()->location());
}

void Parser::parse_module() {
    const Token* token = current_token();

    if (expect_keyword(Keyword::Module)) {
        token = current_token();

        if (token->is_identifier()) {
            set_module_name(token->value());
            add_statement(Statement::make_module_decl(token->value(), token->location()));
        } else {
            emit_parser_error("Module name must be an identifier");
        }
    } else {
        emit_parser_error("File must start with a module declaration");
    }

    next_token();
}

void Parser::parse_import_decl() {
    parse_import_spec();
}

void Parser::parse_import_spec() {
    std::string import_spec;
    const Token* token = current_token();
    Location import_spec_loc = token->location();

    do {
        if (token->is_identifier()) {
            import_spec += token->value();
            token = next_token();

            if (token->type() != TokenType::dot) {
                break;
            }

            token = next_token();
        } else {
            emit_parser_error("Expected an identifier after 'import' keyword");
            break;
        }
    } while (true);

    add_statement(Statement::make_import_decl(import_spec, import_spec_loc));
}

bool Parser::valid_declaration_start(const Token* const token) {
    return token->is_identifier();
}

bool Parser::valid_function_start(const Token* const token) {
    if (token->is_keyword()) {
        auto value = token->value();

        return value == "export" || value == "func";
    }

    return false;
}

void Parser::parse_declaration() {
    const Token* token = current_token();

    /* if (token->is_keyword() && token->value() == "type") { */
    /*     parse_type_alias(); */
    /*     return; */
    /* } */

    if (!token->is_identifier()) {
        return;
    }

    auto identifier = *token;
    token = next_token();

    /* if (token->is_type()) { */
    /*     emit_parser_error("Expected type after identifier '%s'", token->value().c_str()); */
    /*     return; */
    /* } */

    auto type = *token;
    token = next_token();

    if (!expect_token_type(TokenType::assign)) {
        add_statement(Statement::make_variable_decl(identifier, type));
        return;
    }

    Expression* expr = parse_expression(operator_base_precedence());

    add_statement(Statement::make_variable_assignment(identifier, type, expr));
}

/* void Parser::parse_if_statement() { */
/*     if (expect_keyword(Keyword::If)) { */

/*     } */
/* } */

/* void Parser::parse_type_alias() { */
/*     const Token* token = current_token(); */

/*     if (!token->is_identifier()) { */
/*         emit_parser_error("Expected identifier after 'type'"); */
/*         return; */
/*     } */
/* } */

void Parser::parse_toplevel() {
    parse_module();

    while (expect_keyword(Keyword::Import)) {
        // TODO: How to require newline after each import?
        parse_import_decl();
    }

    const Token* token = current_token();

    while (token->type() != TokenType::eof) {
        if (valid_declaration_start(token)) {
            parse_declaration();
        } else if (valid_function_start(token)) {
            parse_function();
        } else {
            emit_parser_error("Expected a declaration");
            break;
        }

        /* token = next_token(); */
    }
}

void Parser::parse_function() {
    bool exported = expect_keyword(Keyword::Export);
    bool is_func = expect_keyword(Keyword::Func);

    if (exported) {
        if (!is_func) {
            emit_parser_error("Expected 'func' after 'export' keyword");
            next_token();
            return;
        }
    } else if (!is_func) {
        return;
    }

    auto token = current_token();

    if (!token->is_identifier()) {
        emit_parser_error("Expected function name identifier after 'func'");
        next_token();
        return;
    }

    const Token func_name(*token);

    next_token();

    Function* func = Statement::make_function(exported, func_name);
    parse_function_signature(func);
    /* parse_type(); */

    if (!func) {
        // TODO: ?
    }

    // TODO: Can we use a local scope here instead?
    _current_function = func;
    parse_block();
    _current_function = nullptr;

    add_statement(func);
}

void Parser::parse_function_signature(Function* const func) {
    if (expect_token_type(TokenType::lparen)) {
        if (expect_token_type(TokenType::rparen)) {
            // No parameters, just return
            return;
        }

        parse_function_parameters(func);

        /* Type* return_type = parse_type(); */
    }
}

void Parser::parse_function_parameters(Function* const func) {
    const Token* token = current_token();

    if (token->is_identifier()) {
        parse_parameter_list(func);
    } else {
        emit_parser_error(
            "Expected identifier or ')' for function parameters, but got '%s'",
            token->value().c_str()
        );
    }
}

bool Parser::parse_parameter_decl(Function* const func) {
    auto token = current_token();

    if (token->type() == TokenType::identifier) {
        func->add_parameter(Expression::make_identifier(*token));
        token = next_token();

        if (token->is_type()) {
            // TODO: Parse type
            token = next_token();
        }

        if (token->type() == TokenType::comma) {
            next_token();
        } else if (token->type() == TokenType::rparen) {
            return false;
        } else {
            emit_parser_error("Unexpected token '%s' in function parameter", token->type());
            return false;
        }
    }

    return true;
}

void Parser::parse_parameter_list(Function* const func) {
    auto token = current_token();
    Location loc = token->location();

    while (!_scanner.eof() && parse_parameter_decl(func));

    if (!expect_token_type(TokenType::rparen)) {
        emit_parser_error("Expected ')' after function parameters");
    }
}

    const Token* token = current_token();
    Location loc = token->location();

    do {
        if (token->is_identifier()) {
            func->add_parameter(Expression::make_identifier(*token));

            /* Type* param_type = parse_type(); */
            next_token();
            token = current_token();

            if (token->is_type()) {
                // ...
                next_token();

                if (token->type() == TokenType::rparen) {
                    break;
                } else if (token->type() != TokenType::comma) {
                    emit_parser_error("Expected ',' after function paramter");
                    break;
                }
            } else if (token->type() == TokenType::comma) {
                next_token();
            } else if (token->type() == TokenType::rparen) {
                break;
            } else {
                emit_parser_error("Unexpected token %s", token->type());
            }

            token = next_token();
        } else {
            emit_parser_error("Expected an identifier but got '%s'", token->value().c_str());
            break;
        }
    } while (true);
}


    /* _ast->add_statement(Statement::make_function()); */
}
void Parser::parse_block() {
    if (expect_token_type(TokenType::lbrace)) {
        while (valid_declaration_start(current_token())) {
            // TODO: Add statements to the function, not the top-level AST
            parse_declaration();
        }

        if (!expect_token_type(TokenType::rbrace)) {
            emit_parser_error("Expected '}' to close block");
        }
    }
}

/* Type* Parser::parse_type() { */
/*     const Token* token = current_token(); */

/*     if (token->is_type()) { */
/*         // TODO: Return some type here */
/*     } else { */
/*         emit_parser_error( */
/*             "Expected type but got an %s with value '%s'", */
/*             token->type(), */
/*             token->value().c_str() */
/*         ); */
/*     } */
/* } */
Expression* Parser::parse_literal() {
    const Token* token = current_token();
    Expression* result = nullptr;

    switch (token->type()) {
        case TokenType::integer:
            result = Expression::make_int_literal(token->int_value(), token->location());
            break;

        case TokenType::floatp:
            result = Expression::make_float_literal(token->float_value(), token->location());
            break;

        case TokenType::character:
            result = Expression::make_char_literal(token->int_value(), token->location());
            break;

        case TokenType::string:
            result = Expression::make_string_literal(token->value(), token->location());
            break;

        case TokenType::keyword:
            if (token->value() == "true" || token->value() == "false") {
                result = Expression::make_bool_literal(token->value(), token->location());
            } else {
                result = Expression::make_parser_error(
                    "Expected literal token type, got keyword",
                    token->location()
                );
            }
            break;

        default:
            result = Expression::make_parser_error("Expected literal token type", token->location());
            break;
    }

    if (!result->is_error()) {
        next_token();
    }

    return result;
}

Expression* Parser::parse_maybe_qualified_identifier() {
    std::vector<std::string> identifier;
    const Token* token = current_token();
    Location loc = token->location();

    do {
        if (token->is_identifier()) {
            identifier.emplace_back(token->value());
            token = next_token();

            if (token->type() != TokenType::dot) {
                break;
            }

            token = next_token();
        } else {
            emit_parser_error("Expected an identifier after 'import' keyword");
            break;
        }
    } while (true);

    return Expression::make_identifier(identifier, loc);
}

Expression* Parser::parse_unary_expression() {
    Expression* expr = parse_literal();

    if (!expr->is_error()) {
        return expr;
    }

    expr = parse_maybe_qualified_identifier();

    if (!expr->is_error()) {
        return expr;
    }

    if (expect_token_type(TokenType::lparen)) {
        return parse_parenthesised_expression();
    }

    return make_parser_error("Expected an unary expression");

    /* if (token->category() != TokenCategory::op) { */
    /*     return make_parser_error("Expected a literal"); */
    /* } */

    /* auto value = token->value(); */

    /* if (token->is_op()) { */
    /*     Location location = token->location(); */
    /*     auto op = token->op(); */
    /*     this->next_token(); */

    /*     auto expr = parse_literal(); */

    /*     if (expr->is_error()) { */
    /*         return expr; */
    /*     } */

    /*     return Expression::make_unary(op, expr, location); */
    /* } */

    /* return make_parser_error("Expected unary operator"); */
}

Expression* Parser::parse_parenthesised_expression() {
    Expression* expr = parse_expression(operator_base_precedence());

    if (expect_token_type(TokenType::rparen)) {
        expr->set_parenthesised(true);
        return expr;
    }

    return make_parser_error("Expected ')' after expression");
}

Expression* Parser::parse_expression(int precedence) {
    Expression* left = parse_unary_expression();

    if (left->is_error()) {
        return left;
    }

    while (true) {
        const Token* token = current_token();

        if (token->category() != TokenCategory::bin_op) {
            return left;
        }

        int right_precedence = operator_precedence(token->value());

        if (precedence >= right_precedence) {
            // This is where precedence climbing comes in
            return left;
        }

        Location binop_location = token->location();
        std::string op = token->op();
        next_token();

        auto right = parse_expression(right_precedence);

        left = Expression::make_binary(op, left, right, binop_location);
    }

    return left;
}

/* Ast Parser::parse(const std::string& path) { */
/*     return Ast(); */
/* } */

void Parser::parse_file(const std::string& path, Ast* ast) {
    if (_scanner.open_file(path)) {
        _ast = ast;
        next_token();
        parse_toplevel();

        _ast = nullptr;

        /* return Ast(parse_toplevel()); */
    }

    /* return Ast(nullptr); */
}
