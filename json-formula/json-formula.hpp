// Copyright 2020 Daniel Parker
// Copyright 2023, Adobe
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This started as the JMESPath parser for JsonCons
// and was extended by Leonard Rosenthol (lrosenth@adobe.com)
// to support json-formula
// 
// Broke the single file into multiple files (that are all included from here)
// in order to make it easier to add more functions to the parser.

#ifndef JSONFORMULA_HPP
#define JSONFORMULA_HPP

#include <string>
#include <vector>
#include <unordered_map> // std::unordered_map
#include <memory>
#include <type_traits> // std::is_const
#include <limits> // std::numeric_limits
#include <utility> // std::move
#include <functional> // 
#include <algorithm> // std::stable_sort, std::reverse
#include <cmath> // std::abs
#include <jsoncons/json.hpp>
#include "json-formula-error.hpp"
#include "json-formula-declarations.hpp"

namespace jsonformula {

    namespace detail {
     
    #include "json-formula-resources.hpp"
    #include "json-formula-evaluator.hpp"

    } // detail

    template <class Json>
    using jsonformula_expression = typename jsonformula::detail::jsonformula_evaluator<Json,const Json&>::jsonformula_expression;

    template<class Json>
    Json search(const Json& doc, const typename Json::string_view_type& path)
    {
        jsonformula::detail::jsonformula_evaluator<Json,const Json&> evaluator;
        std::error_code ec;
        auto expr = evaluator.compile(path.data(), path.size(), ec);
        if (ec)
        {
            JSONCONS_THROW(jsonformula_error(ec, evaluator.line(), evaluator.column()));
        }
        auto result = expr.evaluate(doc, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonformula_error(ec));
        }
        return result;
    }

    template<class Json>
    Json search(const Json& doc, const typename Json::string_view_type& path, std::error_code& ec)
    {
        jsonformula::detail::jsonformula_evaluator<Json,const Json&> evaluator;
        auto expr = evaluator.compile(path.data(), path.size(), ec);
        if (ec)
        {
            return Json::null();
        }
        auto result = expr.evaluate(doc, ec);
        if (ec)
        {
            return Json::null();
        }
        return result;
    }

	template<class Json>
	Json debug_search(const Json& doc, const typename Json::string_view_type& path, std::error_code& ec)
	{
		jsonformula::detail::jsonformula_evaluator<Json,const Json&> evaluator;
		evaluator.debug();	// enable debugging!
		auto expr = evaluator.compile(path.data(), path.size(), ec);
		if (ec)
		{
			return Json::null();
		}
		auto result = expr.evaluate(doc, ec);
		if (ec)
		{
			return Json::null();
		}
		return result;
	}

    template <class Json>
    jsonformula_expression<Json> make_expression(const typename jsoncons::json::string_view_type& expr)
    {
        return jsonformula_expression<Json>::compile(expr);
    }

    template <class Json>
    jsonformula_expression<Json> make_expression(const typename jsoncons::json::string_view_type& expr,
                                              std::error_code& ec)
    {
        return jsonformula_expression<Json>::compile(expr, ec);
    }


} // namespace jsonformula

#endif
