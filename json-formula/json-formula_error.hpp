/// Copyright 2020 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Extended by Leonard Rosenthol (lrosenth@adobe.com)
//	to implement JSON Formula

#ifndef JSONCONS_JSONFORMULA_JSONFORMULA_ERROR_HPP
#define JSONCONS_JSONFORMULA_JSONFORMULA_ERROR_HPP

#include <jsoncons/json_exception.hpp>
#include <system_error>

namespace jsoncons { namespace jsonformula {

    class jsonformula_error : public std::system_error, public virtual json_exception
    {
        std::size_t line_number_;
        std::size_t column_number_;
        mutable std::string what_;
    public:
        jsonformula_error(std::error_code ec)
            : std::system_error(ec), line_number_(0), column_number_(0)
        {
        }
        jsonformula_error(std::error_code ec, const std::string& what_arg)
            : std::system_error(ec, what_arg), line_number_(0), column_number_(0)
        {
        }
        jsonformula_error(std::error_code ec, std::size_t position)
            : std::system_error(ec), line_number_(0), column_number_(position)
        {
        }
        jsonformula_error(std::error_code ec, std::size_t line, std::size_t column)
            : std::system_error(ec), line_number_(line), column_number_(column)
        {
        }
        jsonformula_error(const jsonformula_error& other) = default;

        jsonformula_error(jsonformula_error&& other) = default;

        const char* what() const noexcept override
        {
            if (what_.empty())
            {
                JSONCONS_TRY
                {
                    what_.append(std::system_error::what());
                    if (line_number_ != 0 && column_number_ != 0)
                    {
                        what_.append(" at line ");
                        what_.append(std::to_string(line_number_));
                        what_.append(" and column ");
                        what_.append(std::to_string(column_number_));
                    }
                    else if (column_number_ != 0)
                    {
                        what_.append(" at position ");
                        what_.append(std::to_string(column_number_));
                    }
                    return what_.c_str();
                }
                JSONCONS_CATCH(...)
                {
                    return std::system_error::what();
                }
            }
            else
            {
                return what_.c_str();
            }
        }

        std::size_t line() const noexcept
        {
            return line_number_;
        }

        std::size_t column() const noexcept
        {
            return column_number_;
        }
    };

enum class jsonformula_errc
{
    success = 0,
    expected_identifier,
    expected_index,
    expected_A_Za_Z_,
    expected_rbracket,
    expected_rparen,
    expected_rbrace,
    expected_colon,
    expected_dot,
    expected_or,
    expected_and,
    expected_multi_select_list,
    invalid_number,
    invalid_literal,
    expected_comparator,
    expected_key,
    invalid_argument,
    unknown_function,
    invalid_type,
    unexpected_end_of_input,
    step_cannot_be_zero,
    syntax_error,
    invalid_codepoint,
    illegal_escaped_character,
    unbalanced_parentheses,
    unbalanced_braces,
    invalid_arity,
    identifier_not_found,
    expected_index_expression,
    unknown_error 
};

class jsonformula_error_category_impl
   : public std::error_category
{
public:
    const char* name() const noexcept override
    {
        return "json-formula";
    }
    std::string message(int ev) const override
    {
        switch (static_cast<jsonformula_errc>(ev))
        {
            case jsonformula_errc::expected_identifier:
                return "Expected identifier";
            case jsonformula_errc::expected_index:
                return "Expected index";
            case jsonformula_errc::expected_A_Za_Z_:
                return "Expected A-Z, a-z, or _";
            case jsonformula_errc::expected_rbracket:
                return "Expected ]";
            case jsonformula_errc::expected_rparen:
                return "Expected )";
            case jsonformula_errc::expected_rbrace:
                return "Expected }";
            case jsonformula_errc::expected_colon:
                return "Expected :";
            case jsonformula_errc::expected_dot:
                return "Expected \".\"";
            case jsonformula_errc::expected_or:
                return "Expected \"||\"";
            case jsonformula_errc::expected_and:
                return "Expected \"&&\"";
            case jsonformula_errc::expected_multi_select_list:
                return "Expected multi-select-list";
            case jsonformula_errc::invalid_number:
                return "Invalid number";
            case jsonformula_errc::invalid_literal:
                return "Invalid literal";
            case jsonformula_errc::expected_comparator:
                return "Expected <, <=, ==, >=, > or !=";
            case jsonformula_errc::expected_key:
                return "Expected key";
            case jsonformula_errc::invalid_argument:
                return "Invalid argument type";
            case jsonformula_errc::unknown_function:
                return "Unknown function";
            case jsonformula_errc::invalid_type:
                return "Invalid type";
            case jsonformula_errc::unexpected_end_of_input:
                return "Unexpected end of json-formula input";
            case jsonformula_errc::step_cannot_be_zero:
                return "Slice step cannot be zero";
            case jsonformula_errc::syntax_error:
                return "Syntax error";
            case jsonformula_errc::invalid_codepoint:
                return "Invalid codepoint";
            case jsonformula_errc::illegal_escaped_character:
                return "Illegal escaped character";
            case jsonformula_errc::unbalanced_parentheses:
                return "Unbalanced parentheses";
            case jsonformula_errc::unbalanced_braces:
                return "Unbalanced braces";
            case jsonformula_errc::invalid_arity:
                return "Function called with wrong number of arguments";
            case jsonformula_errc::identifier_not_found:
                return "Identifier not found";
            case jsonformula_errc::expected_index_expression:
                return "Expected index expression";
            case jsonformula_errc::unknown_error:
            default:
                return "Unknown json-formula parser error";
        }
    }
};

inline
const std::error_category& jsonformula_error_category()
{
  static jsonformula_error_category_impl instance;
  return instance;
}

inline 
std::error_code make_error_code(jsonformula_errc result)
{
    return std::error_code(static_cast<int>(result),jsonformula_error_category());
}

}}

namespace std {
    template<>
    struct is_error_code_enum<jsoncons::jsonformula::jsonformula_errc> : public true_type
    {
    };
}

#endif
