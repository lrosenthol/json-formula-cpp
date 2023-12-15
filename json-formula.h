//
//  json-formula.h
//  json-formula
//
//  Created by Leonard Rosenthol on 11/20/23.
//

#ifndef json_formula_h
#define json_formula_h

#include "jsoncons/json.hpp"
#include "jsoncons_ext/jmespath/jmespath.hpp"

namespace jsonformulaX {

	enum class token_kind
	{
		current_node,
		number,
		lparen,
		rparen,
		begin_multi_select_hash,
		end_multi_select_hash,
		begin_multi_select_list,
		end_multi_select_list,
		begin_filter,
		end_filter,
		pipe,
		separator,
		key,
		literal,
		expression,
		binary_operator,
		unary_operator,
		function,
		end_function,
		argument,
		begin_expression_type,
		end_expression_type,
		end_of_expression
	};

	class parser {
		public:
			parser( const jsoncons::json& j) : mJSON( j ) {}
			jsoncons::json eval( const std::string& e );

			static jsoncons::json eval(const jsoncons::json& j, const std::string& e);
		
			// debugging
			std::vector<jsoncons::json> Tokens() const { return mTokens; }

		private:
			const jsoncons::json& 			mJSON;
			std::vector<jsoncons::json> 	mTokens;
	};

}

#endif /* json_formula_h */
