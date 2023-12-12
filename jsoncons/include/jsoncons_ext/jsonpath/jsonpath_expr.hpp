// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONPATH_JSONPATH_EXPR_HPP
#define JSONCONS_JSONPATH_JSONPATH_EXPR_HPP

#include <string>
#include <vector>
#include <memory>
#include <type_traits> // std::is_const
#include <limits> // std::numeric_limits
#include <utility> // std::move
#include <regex>
#include <algorithm> // std::reverse
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>
#include <jsoncons_ext/jsonpath/expression.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_selector.hpp>

namespace jsoncons { 
namespace jsonpath {

    template <class Json>
    struct jsonpath_traits
    {
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = typename Json::string_view_type;
        using element_type = Json;
        using value_type = typename std::remove_cv<Json>::type;
        using reference = Json&;
        using const_reference = const Json&;
        using pointer = typename std::conditional<std::is_const<typename std::remove_reference<reference>::type>::value, typename Json::const_pointer, typename Json::pointer>::type;
        using allocator_type = typename value_type::allocator_type;
        using evaluator_type = typename jsoncons::jsonpath::detail::jsonpath_evaluator<value_type, reference>;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using path_expression_type = jsoncons::jsonpath::detail::path_expression<value_type,reference>;
        using path_pointer = const path_node_type*;
    };

    template <class Json,class TempAllocator=std::allocator<char>>
    class jsonpath_expr
    {
    public:
        using jsonpath_traits_type = jsoncons::jsonpath::jsonpath_traits<Json>;

        using allocator_type = typename jsonpath_traits_type::allocator_type;
        using evaluator_type = typename jsonpath_traits_type::evaluator_type;
        using char_type = typename jsonpath_traits_type::char_type;
        using string_type = typename jsonpath_traits_type::string_type;
        using string_view_type = typename jsonpath_traits_type::string_view_type;
        using value_type = typename jsonpath_traits_type::value_type;
        using reference = typename jsonpath_traits_type::reference;
        using const_reference = typename jsonpath_traits_type::const_reference;
        using path_expression_type = typename jsonpath_traits_type::path_expression_type;
        using path_node_type = typename jsonpath_traits_type::path_node_type;
    private:
        allocator_type alloc_;
        std::unique_ptr<jsoncons::jsonpath::detail::static_resources<value_type,reference>> static_resources_;
        path_expression_type expr_;
    public:
        jsonpath_expr(const allocator_set<allocator_type,TempAllocator>& alloc_set,
            std::unique_ptr<jsoncons::jsonpath::detail::static_resources<value_type,reference>>&& resources,
            path_expression_type&& expr)
            : alloc_(alloc_set.get_allocator()),
              static_resources_(std::move(resources)), 
              expr_(std::move(expr))
        {
        }

        jsonpath_expr(const jsonpath_expr&) = delete;
        jsonpath_expr(jsonpath_expr&&) = default;

        jsonpath_expr& operator=(const jsonpath_expr&) = delete;
        jsonpath_expr& operator=(jsonpath_expr&&) = default;

        template <class BinaryCallback>
        typename std::enable_if<extension_traits::is_binary_function_object<BinaryCallback,const path_node_type&,const_reference>::value,void>::type
        select(reference instance, BinaryCallback callback, result_options options = result_options()) const
        {
            jsoncons::jsonpath::detail::dynamic_resources<value_type,reference> resources{alloc_};
            expr_.evaluate(resources, instance, path_node_type{}, instance, callback, options);
        }

        value_type evaluate(reference instance, result_options options = result_options()) const
        {
            if ((options & result_options::path) == result_options::path)
            {
                jsoncons::jsonpath::detail::dynamic_resources<value_type,reference> resources{alloc_};

                value_type result(json_array_arg, semantic_tag::none, alloc_);
                auto callback = [&result](const path_node_type& p, reference)
                {
                    result.emplace_back(to_basic_string(p));
                };
                expr_.evaluate(resources, instance, path_node_type(), instance, callback, options);
                return result;
            }
            else
            {
                jsoncons::jsonpath::detail::dynamic_resources<value_type,reference> resources{alloc_};
                return expr_.evaluate(resources, instance, path_node_type(), instance, options);
            }
        }

        template <class BinaryCallback>
        typename std::enable_if<extension_traits::is_binary_function_object<BinaryCallback,const path_node_type&,value_type&>::value,void>::type
        update(reference instance, BinaryCallback callback) const
        {
            jsoncons::jsonpath::detail::dynamic_resources<value_type,reference> resources{alloc_};
            expr_.evaluate_with_replacement(resources, instance, path_node_type{}, instance, callback);
        }

        std::vector<basic_json_location<char_type>> select_paths(reference instance) const
        {
            std::vector<basic_json_location<char_type>> result;

            result_options options = result_options::path | result_options::nodups | result_options::sort;

            auto callback = [&result](const path_node_type& path, const_reference)
            {
                result.emplace_back(to_json_location(path));
            };

            jsoncons::jsonpath::detail::dynamic_resources<value_type,reference> resources{alloc_};
            expr_.evaluate(resources, instance, path_node_type(), instance, callback, options);

            return result;
        }
    };

    template <class Json>
    auto compile_jsonpath(const typename Json::string_view_type& path,
        const jsoncons::jsonpath::custom_functions<typename jsonpath_traits<Json>::value_type>& funcs = jsoncons::jsonpath::custom_functions<typename jsonpath_traits<Json>::value_type>())
    {
        using jsonpath_traits_type = jsoncons::jsonpath::jsonpath_traits<Json>;

        using value_type = typename jsonpath_traits_type::value_type;
        using reference = typename jsonpath_traits_type::reference;
        using evaluator_type = typename jsonpath_traits_type::evaluator_type;

        auto static_resources = jsoncons::make_unique<jsoncons::jsonpath::detail::static_resources<value_type,reference>>(funcs);
        evaluator_type evaluator;
        auto expr = evaluator.compile(*static_resources, path);

        return jsoncons::jsonpath::jsonpath_expr<Json>(jsoncons::combine_allocators(), std::move(static_resources), std::move(expr));
    }

    template <class Json, class TempAllocator>
    auto compile_jsonpath(const allocator_set<typename Json::allocator_type,TempAllocator>& alloc_set,
        const typename Json::string_view_type& path,
        const jsoncons::jsonpath::custom_functions<typename jsonpath_traits<Json>::value_type>& funcs, std::error_code& ec)
    {
        using jsonpath_traits_type = jsoncons::jsonpath::jsonpath_traits<Json>;

        using value_type = typename jsonpath_traits_type::value_type;
        using reference = typename jsonpath_traits_type::reference;
        using evaluator_type = typename jsonpath_traits_type::evaluator_type;

        auto static_resources = jsoncons::make_unique<jsoncons::jsonpath::detail::static_resources<value_type,reference>>(funcs, 
            alloc_set.get_allocator());
        evaluator_type evaluator{alloc_set.get_allocator()};
        auto expr = evaluator.compile(*static_resources, path, ec);

        return jsoncons::jsonpath::jsonpath_expr<value_type>(alloc_set, std::move(static_resources), std::move(expr));
    }

} // namespace jsonpath
} // namespace jsoncons

#endif
