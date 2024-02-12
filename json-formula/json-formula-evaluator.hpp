enum class path_state 
{
	start,
	lhs_expression,
	rhs_expression,
	sub_expression,
	expression_type,
	comparator_expression,
	function_expression,
	argument,
	expression_or_expression_type,
	quoted_string,
	raw_string,
	raw_string_escape_char,
	quoted_string_escape_char,
	escape_u1, 
	escape_u2, 
	escape_u3, 
	escape_u4, 
	escape_expect_surrogate_pair1, 
	escape_expect_surrogate_pair2, 
	escape_u5, 
	escape_u6, 
	escape_u7, 
	escape_u8, 
	literal,
	key_expr,
	val_expr,
	identifier_or_function_expr,
	unquoted_string,
	key_val_expr,
	number,
	digit,
	index_or_slice_expression,
	bracket_specifier,
	bracket_specifier_or_multi_select_list,
	filter,
	multi_select_list,
	multi_select_hash,
	rhs_slice_expression_stop,
	rhs_slice_expression_step,
	expect_rbracket,
	expect_rparen,
	expect_dot,
	expect_rbrace,
	expect_colon,
	expect_multi_select_list,
	cmp_lt_or_lte,
	cmp_eq,
	cmp_gt_or_gte,
	cmp_ne,
	expect_pipe_or_or,
	expect_and,
	// json-formula
	jf_expression,
	plus_operator,
	minus_operator,
	mult_operator,
	div_operator,
	concat_operator,
	union_operator
};


template<class Json, class JsonReference>
class jsonformula_evaluator
{
public:
	typedef typename Json::char_type char_type;
	typedef typename Json::char_traits_type char_traits_type;
	typedef std::basic_string<char_type,char_traits_type> string_type;
	typedef typename Json::string_view_type string_view_type;
	typedef JsonReference reference;
	using pointer = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
	typedef typename Json::const_pointer const_pointer;

	// expression_base
	class expression_base
	{
		std::size_t precedence_level_;
		bool is_right_associative_;
		bool is_projection_;
	public:
		expression_base(operator_kind oper, bool is_projection)
			: precedence_level_(operator_table::precedence_level(oper)), 
				is_right_associative_(operator_table::is_right_associative(oper)), 
				is_projection_(is_projection)
		{
		}

		std::size_t precedence_level() const
		{
			return precedence_level_;
		}

		bool is_right_associative() const
		{
			return is_right_associative_;
		}

		bool is_projection() const 
		{
			return is_projection_;
		}

		virtual ~expression_base() = default;

		virtual reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const = 0;

		virtual void add_expression(std::unique_ptr<expression_base>&& expressions) = 0;

		virtual std::string to_string(std::size_t = 0) const
		{
			return std::string("to_string not implemented");
		}
	};  

	// parameter

	enum class parameter_kind{value, expression};

	class parameter
	{
		parameter_kind type_;

		union
		{
			expression_base* expression_;
			pointer value_;
		};

	public:

		parameter(const parameter& other) noexcept
			: type_(other.type_)
		{
			switch (type_)
			{
				case parameter_kind::expression:
					expression_ = other.expression_;
					break;
				case parameter_kind::value:
					value_ = other.value_;
					break;
				default:
					break;
			}
		}

		parameter(reference value) noexcept
			: type_(parameter_kind::value), value_(std::addressof(value))
		{
		}

		parameter(expression_base* expression) noexcept
			: type_(parameter_kind::expression), expression_(expression)
		{
		}

		parameter& operator=(const parameter& other)
		{
			if (&other != this)
			{
				type_ = other.type_;
				switch (type_)
				{
					case parameter_kind::expression:
						expression_ = other.expression_;
						break;
					case parameter_kind::value:
						value_ = other.value_;
						break;
					default:
						break;
				}
			}
			return *this;
		}

		bool is_value() const
		{
			return type_ == parameter_kind::value;
		}

		bool is_expression() const
		{
			return type_ == parameter_kind::expression;
		}

		const Json& value() const
		{
			return *value_;
		}

		const expression_base& expression() const
		{
			return *expression_;
		}
	};

	#include "json-formula-functions.hpp"

	// operators 
	static bool is_false(reference ref)
	{
		return (ref.is_array() && ref.empty()) ||
				(ref.is_object() && ref.empty()) ||
				(ref.is_string() && ref.as_string_view().size() == 0) ||
				(ref.is_bool() && !ref.as_bool()) ||
				(ref.is_number() && ref.template as<int>()==0) ||	// json-formula also treats 0 as false
				ref.is_null();
	}

	static bool is_true(reference ref)
	{
		return !is_false(ref);
	}

	class unary_operator
	{
		std::size_t precedence_level_;
		bool is_right_associative_;

	protected:
		~unary_operator() = default; // virtual destructor not needed
	public:
		unary_operator(operator_kind oper)
			: precedence_level_(operator_table::precedence_level(oper)), 
				is_right_associative_(operator_table::is_right_associative(oper))
		{
		}

		std::size_t precedence_level() const 
		{
			return precedence_level_;
		}
		bool is_right_associative() const
		{
			return is_right_associative_;
		}

		virtual reference evaluate(reference val, dynamic_resources<Json,JsonReference>&, std::error_code& ec) const = 0;
	};

	class not_expression final : public unary_operator
	{
	public:
		not_expression()
			: unary_operator(operator_kind::not_op)
		{}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			return is_false(val) ? resources.true_value() : resources.false_value();
		}
	};

	class binary_operator
	{
		std::size_t precedence_level_;
		bool is_right_associative_;
	protected:
		~binary_operator() = default; // virtual destructor not needed
	public:
		binary_operator(operator_kind oper)
			: precedence_level_(operator_table::precedence_level(oper)), 
				is_right_associative_(operator_table::is_right_associative(oper))
		{
		}


		std::size_t precedence_level() const 
		{
			return precedence_level_;
		}
		bool is_right_associative() const
		{
			return is_right_associative_;
		}

		virtual reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>&, std::error_code& ec) const = 0;

		virtual std::string to_string(std::size_t indent = 0) const
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("to_string not implemented\n");
			return s;
		}
	};


	// token

	class token
	{
	public:
		token_kind type_;

		union
		{
			std::unique_ptr<expression_base> expression_;
			const unary_operator* unary_operator_;
			const binary_operator* binary_operator_;
			const function_base* function_;
			Json value_;
			string_type key_;
		};
	public:

		token(current_node_arg_t) noexcept
			: type_(token_kind::current_node)
		{
		}

		token(end_function_arg_t) noexcept
			: type_(token_kind::end_function)
		{
		}

		token(separator_arg_t) noexcept
			: type_(token_kind::separator)
		{
		}

		token(lparen_arg_t) noexcept
			: type_(token_kind::lparen)
		{
		}

		token(rparen_arg_t) noexcept
			: type_(token_kind::rparen)
		{
		}

		token(end_of_expression_arg_t) noexcept
			: type_(token_kind::end_of_expression)
		{
		}

		token(begin_multi_select_hash_arg_t) noexcept
			: type_(token_kind::begin_multi_select_hash)
		{
		}

		token(end_multi_select_hash_arg_t) noexcept
			: type_(token_kind::end_multi_select_hash)
		{
		}

		token(begin_multi_select_list_arg_t) noexcept
			: type_(token_kind::begin_multi_select_list)
		{
		}

		token(end_multi_select_list_arg_t) noexcept
			: type_(token_kind::end_multi_select_list)
		{
		}

		token(begin_filter_arg_t) noexcept
			: type_(token_kind::begin_filter)
		{
		}

		token(end_filter_arg_t) noexcept
			: type_(token_kind::end_filter)
		{
		}

		token(pipe_arg_t) noexcept
			: type_(token_kind::pipe)
		{
		}

		token(key_arg_t, const string_type& key)
			: type_(token_kind::key)
		{
			new (&key_) string_type(key);
		}

		token(std::unique_ptr<expression_base>&& expression)
			: type_(token_kind::expression)
		{
			new (&expression_) std::unique_ptr<expression_base>(std::move(expression));
		}

		token(const unary_operator* expression) noexcept
			: type_(token_kind::unary_operator),
				unary_operator_(expression)
		{
		}

		token(const binary_operator* expression) noexcept
			: type_(token_kind::binary_operator),
				binary_operator_(expression)
		{
		}

		token(const function_base* function) noexcept
			: type_(token_kind::function),
				function_(function)
		{
		}

		token(argument_arg_t) noexcept
			: type_(token_kind::argument)
		{
		}

		token(begin_expression_type_arg_t) noexcept
			: type_(token_kind::begin_expression_type)
		{
		}

		token(end_expression_type_arg_t) noexcept
			: type_(token_kind::end_expression_type)
		{
		}

		token(literal_arg_t, Json&& value) noexcept
			: type_(token_kind::literal), value_(std::move(value))
		{
		}

		token(token&& other) noexcept
		{
			construct(std::forward<token>(other));
		}

		token& operator=(token&& other)
		{
			if (&other != this)
			{
				if (type_ == other.type_)
				{
					switch (type_)
					{
						case token_kind::expression:
							expression_ = std::move(other.expression_);
							break;
						case token_kind::key:
							key_ = std::move(other.key_);
							break;
						case token_kind::unary_operator:
							unary_operator_ = other.unary_operator_;
							break;
						case token_kind::binary_operator:
							binary_operator_ = other.binary_operator_;
							break;
						case token_kind::function:
							function_ = other.function_;
							break;
						case token_kind::literal:
							value_ = std::move(other.value_);
							break;
						default:
							break;
					}
				}
				else
				{
					destroy();
					construct(std::forward<token>(other));
				}
			}
			return *this;
		}

		~token() noexcept
		{
			destroy();
		}

		token_kind type() const
		{
			return type_;
		}

		bool is_lparen() const
		{
			return type_ == token_kind::lparen; 
		}

		bool is_lbrace() const
		{
			return type_ == token_kind::begin_multi_select_hash; 
		}

		bool is_key() const
		{
			return type_ == token_kind::key; 
		}

		bool is_rparen() const
		{
			return type_ == token_kind::rparen; 
		}

		bool is_current_node() const
		{
			return type_ == token_kind::current_node; 
		}

		bool is_projection() const
		{
			return type_ == token_kind::expression && expression_->is_projection(); 
		}

		bool is_expression() const
		{
			return type_ == token_kind::expression; 
		}

		bool is_operator() const
		{
			return type_ == token_kind::unary_operator || 
					type_ == token_kind::binary_operator; 
		}

		std::size_t precedence_level() const
		{
			switch(type_)
			{
				case token_kind::unary_operator:
					return unary_operator_->precedence_level();
				case token_kind::binary_operator:
					return binary_operator_->precedence_level();
				case token_kind::expression:
					return expression_->precedence_level();
				default:
					return 0;
			}
		}

		jsoncons::optional<std::size_t> arity() const
		{
			return type_ == token_kind::function ? function_->arity() : jsoncons::optional<std::size_t>();
		}

		bool is_right_associative() const
		{
			switch(type_)
			{
				case token_kind::unary_operator:
					return unary_operator_->is_right_associative();
				case token_kind::binary_operator:
					return binary_operator_->is_right_associative();
				case token_kind::expression:
					return expression_->is_right_associative();
				default:
					return false;
			}
		}

		void construct(token&& other)
		{
			type_ = other.type_;
			switch (type_)
			{
				case token_kind::expression:
					new (&expression_) std::unique_ptr<expression_base>(std::move(other.expression_));
					break;
				case token_kind::key:
					new (&key_) string_type(std::move(other.key_));
					break;
				case token_kind::unary_operator:
					unary_operator_ = other.unary_operator_;
					break;
				case token_kind::binary_operator:
					binary_operator_ = other.binary_operator_;
					break;
				case token_kind::function:
					function_ = other.function_;
					break;
				case token_kind::literal:
					new (&value_) Json(std::move(other.value_));
					break;
				default:
					break;
			}
		}

		void destroy() noexcept 
		{
			switch(type_)
			{
				case token_kind::expression:
					expression_.~unique_ptr();
					break;
				case token_kind::key:
					key_.~basic_string();
					break;
				case token_kind::literal:
					value_.~Json();
					break;
				default:
					break;
			}
		}

		std::string to_string(std::size_t indent = 0) const
		{
			switch(type_)
			{
				case token_kind::expression:
					return expression_->to_string(indent);
					break;
				case token_kind::unary_operator:
					return std::string("unary_operator");
					break;
				case token_kind::binary_operator:
					return binary_operator_->to_string(indent);
					break;
				case token_kind::current_node:
					return std::string("current_node");
					break;
				case token_kind::end_function:
					return std::string("end_function");
					break;
				case token_kind::separator:
					return std::string("separator");
					break;
				case token_kind::literal:
					return std::string("literal");
					break;
				case token_kind::key:
					return std::string("key ") + key_;
					break;
				case token_kind::begin_multi_select_hash:
					return std::string("begin_multi_select_hash");
					break;
				case token_kind::begin_multi_select_list:
					return std::string("begin_multi_select_list");
					break;
				case token_kind::begin_filter:
					return std::string("begin_filter");
					break;
				case token_kind::pipe:
					return std::string("pipe");
					break;
				case token_kind::lparen:
					return std::string("lparen");
					break;
				case token_kind::function:
					return function_->to_string();
				case token_kind::argument:
					return std::string("argument");
					break;
				case token_kind::begin_expression_type:
					return std::string("begin_expression_type");
					break;
				case token_kind::end_expression_type:
					return std::string("end_expression_type");
					break;
				default:
					return std::string("default");
					break;
			}
		}
	};

	static pointer evaluate_tokens(reference doc, const std::vector<token>& output_stack, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec)
	{
		pointer root_ptr = std::addressof(doc);
		std::vector<parameter> stack;
		std::vector<parameter> arg_stack;
		for (std::size_t i = 0; i < output_stack.size(); ++i)
		{
			auto& t = output_stack[i];
			switch (t.type())
			{
				case token_kind::literal:
				{
					stack.emplace_back(t.value_);
					break;
				}
				case token_kind::begin_expression_type:
				{
					JSONCONS_ASSERT(i+1 < output_stack.size());
					++i;
					JSONCONS_ASSERT(output_stack[i].is_expression());
					JSONCONS_ASSERT(!stack.empty());
					stack.pop_back();
					stack.emplace_back(output_stack[i].expression_.get());
					break;
				}
				case token_kind::pipe:
				{
					JSONCONS_ASSERT(!stack.empty());
					root_ptr = std::addressof(stack.back().value());
					break;
				}
				case token_kind::current_node:
					stack.emplace_back(*root_ptr);
					break;
				case token_kind::expression:
				{
					JSONCONS_ASSERT(!stack.empty());
					pointer ptr = std::addressof(stack.back().value());
					stack.pop_back();
					auto& ref = t.expression_->evaluate(*ptr, resources, ec);
					stack.emplace_back(ref);
					break;
				}
				case token_kind::unary_operator:
				{
					JSONCONS_ASSERT(stack.size() >= 1);
					pointer ptr = std::addressof(stack.back().value());
					stack.pop_back();
					reference r = t.unary_operator_->evaluate(*ptr, resources, ec);
					stack.emplace_back(r);
					break;
				}
				case token_kind::binary_operator:
				{
					JSONCONS_ASSERT(stack.size() >= 2);
					pointer rhs = std::addressof(stack.back().value());
					stack.pop_back();
					pointer lhs = std::addressof(stack.back().value());
					stack.pop_back();
					reference r = t.binary_operator_->evaluate(*lhs,*rhs, resources, ec);
					stack.emplace_back(r);
					break;
				}
				case token_kind::argument:
				{
					JSONCONS_ASSERT(!stack.empty());
					arg_stack.push_back(std::move(stack.back()));
					stack.pop_back();
					break;
				}
				case token_kind::function:
				{
					if (t.function_->arity() && *(t.function_->arity()) != arg_stack.size())
					{
						ec = jsonformula_errc::invalid_arity;
						return std::addressof(resources.null_value());
					}

					reference r = t.function_->evaluate(arg_stack, resources, ec);
					if (ec)
					{
						return std::addressof(resources.null_value());
					}
					arg_stack.clear();
					stack.emplace_back(r);
					break;
				}
				default:
					break;
			}
		}
		JSONCONS_ASSERT(stack.size() == 1);
		return std::addressof(stack.back().value());
	}

	// Implementations

	class or_operator final : public binary_operator
	{
	public:
		or_operator()
			: binary_operator(operator_kind::or_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if (lhs.is_null() && rhs.is_null())
			{
				return resources.null_value();
			}
			if (!is_false(lhs))
			{
				return lhs;
			}
			else
			{
				return rhs;
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("or_operator");
			return s;
		}
	};

	class and_operator final : public binary_operator
	{
	public:
		and_operator()
			: binary_operator(operator_kind::and_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>&, std::error_code&) const override
		{
			if (is_true(lhs))
			{
				return rhs;
			}
			else
			{
				return lhs;
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("and_operator");
			return s;
		}
	};

	class eq_operator final : public binary_operator
	{
	public:
		eq_operator()
			: binary_operator(operator_kind::eq_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override 
		{
			return lhs == rhs ? resources.true_value() : resources.false_value();
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("eq_operator");
			return s;
		}
	};

	class ne_operator final : public binary_operator
	{
	public:
		ne_operator()
			: binary_operator(operator_kind::ne_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override 
		{
			return lhs != rhs ? resources.true_value() : resources.false_value();
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("ne_operator");
			return s;
		}
	};

	class lt_operator final : public binary_operator
	{
	public:
		lt_operator()
			: binary_operator(operator_kind::lt_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override 
		{
			if ( lhs.is_number() && rhs.is_number() ) {
				return lhs < rhs ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_string() && rhs.is_string() ) {
				return lhs < rhs ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_number() && rhs.is_string() ) {
				return lhs < std::stod(rhs.template as<std::string>()) ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_string() && rhs.is_number() ) {
				return lhs < std::to_string(rhs.template as<double>()) ? resources.true_value() : resources.false_value();
			} else {
				// for any other type comparisons, return false
				return resources.false_value();
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("lt_operator");
			return s;
		}
	};

	class lte_operator final : public binary_operator
	{
	public:
		lte_operator()
			: binary_operator(operator_kind::lte_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override 
		{
			if ( lhs.is_number() && rhs.is_number() ) {
				return lhs <= rhs ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_string() && rhs.is_string() ) {
				return lhs <= rhs ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_number() && rhs.is_string() ) {
				return lhs <= std::stod(rhs.template as<std::string>()) ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_string() && rhs.is_number() ) {
				return lhs <= std::to_string(rhs.template as<double>()) ? resources.true_value() : resources.false_value();
			} else {
				// for any other type comparisons, return false
				return resources.false_value();
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("lte_operator");
			return s;
		}
	};

	class gt_operator final : public binary_operator
	{
	public:
		gt_operator()
			: binary_operator(operator_kind::gt_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if ( lhs.is_number() && rhs.is_number() ) {
				return lhs > rhs ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_string() && rhs.is_string() ) {
				return lhs > rhs ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_number() && rhs.is_string() ) {
				return lhs > std::stod(rhs.template as<std::string>()) ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_string() && rhs.is_number() ) {
				return lhs > std::to_string(rhs.template as<double>()) ? resources.true_value() : resources.false_value();
			} else {
				// for any other type comparisons, return false
				return resources.false_value();
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("gt_operator");
			return s;
		}
	};

	class gte_operator final : public binary_operator
	{
	public:
		gte_operator()
			: binary_operator(operator_kind::gte_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if ( lhs.is_number() && rhs.is_number() ) {
				return lhs >= rhs ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_string() && rhs.is_string() ) {
				return lhs >= rhs ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_number() && rhs.is_string() ) {
				return lhs >= std::stod(rhs.template as<std::string>()) ? resources.true_value() : resources.false_value();
			} else if ( lhs.is_string() && rhs.is_number() ) {
				return lhs >= std::to_string(rhs.template as<double>()) ? resources.true_value() : resources.false_value();
			} else {
				// for any other type comparisons, return false
				return resources.false_value();
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("gte_operator");
			return s;
		}
	};

	// for json-fornula
	class plus_operator final : public binary_operator
	{
	public:
		plus_operator()
			: binary_operator(operator_kind::plus_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if ( (lhs.is_number() && rhs.is_number()) ||
					(lhs.is_null() || rhs.is_null()) ||
					(lhs.is_bool() || rhs.is_bool()) ) {
				double l = 0, r = 0;
				
				if ( lhs.type() == jsoncons::json_type::double_value ) {
					l = lhs.template as<double>();
				} else if ( lhs.type() == jsoncons::json_type::string_value ) {
					l = std::stod(lhs.template as<std::string>());
				} else if ( lhs.type() == jsoncons::json_type::bool_value ) {
					l = lhs.template as<bool>() ? 1 : 0;
				} else if ( !lhs.is_null() ) {
					l = lhs.template as<int64_t>();
				} else {
//						std::cout << pretty_print(lhs) << std::endl;
				}
				
				
				if ( rhs.type() == jsoncons::json_type::double_value ) {
					r = rhs.template as<double>();
				} else if ( rhs.type() == jsoncons::json_type::string_value ) {
					r = std::stod(rhs.template as<std::string>());
				} else if ( rhs.type() == jsoncons::json_type::bool_value ) {
					r = rhs.template as<bool>() ? 1 : 0;
				} else if ( !rhs.is_null() ){
					r = rhs.template as<int64_t>();
				} else {
//						std::cout << pretty_print(rhs) << std::endl;
				}
				
				return *resources.create_json(l + r);
			} else if ( lhs.type() == jsoncons::json_type::string_value || rhs.type() == jsoncons::json_type::string_value ) {
				double l = 0, r = 0;
				
				if ( lhs.type() == jsoncons::json_type::string_value )
					l = std::stod(lhs.template as<std::string>());
				else if ( lhs.type() != jsoncons::json_type::null_value )
					l = lhs.template as<double>();

				if ( rhs.type() == jsoncons::json_type::string_value )
					r = std::stod(rhs.template as<std::string>());
				else if ( rhs.type() != jsoncons::json_type::null_value )
					r = rhs.template as<double>();

				return *resources.create_json(l + r);
			} else if ( lhs.type() == jsoncons::json_type::array_value && rhs.type() == jsoncons::json_type::array_value ) {
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				auto numEnts = std::max( lhs.size(), rhs.size() );
				for (int64_t i=0; i<numEnts; i++) {
					double l=0, r=0;
					if ( i < lhs.size() ) l = lhs[i].template as<double>();
					if ( i < rhs.size() ) r = rhs[i].template as<double>();
					result->push_back( l + r);
				}
				return *result;
			} else if ( lhs.type() == jsoncons::json_type::array_value ) {
				double d = rhs.template as<double>();
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : lhs.array_range()) {
					result->push_back( v.template as<double>() + d );
				}
				return *result;
			} else if ( rhs.type() == jsoncons::json_type::array_value ) {
				double d = lhs.template as<double>();
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : rhs.array_range()) {
					result->push_back( v.template as<double>() + d );
				}
				return *result;
			} else {
				std::cout << pretty_print(lhs) << " " << pretty_print(rhs) << std::endl;
			}
			
			return resources.null_value();
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("plus_operator");
			return s;
		}
	};

	class minus_operator final : public binary_operator
	{
	public:
		minus_operator()
			: binary_operator(operator_kind::minus_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if ( (lhs.is_number() && rhs.is_number()) ||
					(lhs.is_null() || rhs.is_null()) ||
					(lhs.is_bool() || rhs.is_bool()) ) {
				double l = 0, r = 0;
				
				if ( lhs.type() == jsoncons::json_type::double_value ) {
					l = lhs.template as<double>();
				} else if ( lhs.type() == jsoncons::json_type::string_value ) {
					l = std::stod(lhs.template as<std::string>());
				} else if ( lhs.type() == jsoncons::json_type::bool_value ) {
					l = lhs.template as<bool>() ? 1 : 0;
				} else if ( !lhs.is_null() ) {
					l = lhs.template as<int64_t>();
				} else {
//						std::cout << pretty_print(lhs) << std::endl;
				}
				
				
				if ( rhs.type() == jsoncons::json_type::double_value ) {
					r = rhs.template as<double>();
				} else if ( rhs.type() == jsoncons::json_type::string_value ) {
					r = std::stod(rhs.template as<std::string>());
				} else if ( rhs.type() == jsoncons::json_type::bool_value ) {
					r = rhs.template as<bool>() ? 1 : 0;
				} else if ( !rhs.is_null() ){
					r = rhs.template as<int64_t>();
				} else {
//						std::cout << pretty_print(rhs) << std::endl;
				}
				
				return *resources.create_json(l - r);
			} else if ( lhs.type() == jsoncons::json_type::string_value || rhs.type() == jsoncons::json_type::string_value ) {
				double l = 0, r = 0;
				
				if ( lhs.type() == jsoncons::json_type::string_value )
					l = std::stod(lhs.template as<std::string>());
				else if ( lhs.type() != jsoncons::json_type::null_value )
					l = lhs.template as<double>();

				if ( rhs.type() == jsoncons::json_type::string_value )
					r = std::stod(rhs.template as<std::string>());
				else if ( rhs.type() != jsoncons::json_type::null_value )
					r = rhs.template as<double>();

				return *resources.create_json(l - r);
			} else if ( lhs.type() == jsoncons::json_type::array_value && rhs.type() == jsoncons::json_type::array_value ) {
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				auto numEnts = std::max( lhs.size(), rhs.size() );
				for (int64_t i=0; i<numEnts; i++) {
					double l=0, r=0;
					if ( i < lhs.size() ) l = lhs[i].template as<double>();
					if ( i < rhs.size() ) r = rhs[i].template as<double>();
					result->push_back( l - r);
				}
				return *result;
			} else if ( lhs.type() == jsoncons::json_type::array_value ) {
				double d = rhs.template as<double>();
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : lhs.array_range()) {
					result->push_back( v.template as<double>() - d );
				}
				return *result;
			} else if ( rhs.type() == jsoncons::json_type::array_value ) {
				double d = lhs.template as<double>();
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : rhs.array_range()) {
					result->push_back( d - v.template as<double>() );
				}
				return *result;
			}
			
			return resources.null_value();
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("minus_operator");
			return s;
		}
	};

	class mult_operator final : public binary_operator
	{
	public:
		mult_operator()
			: binary_operator(operator_kind::mult_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if ( (lhs.is_number() && rhs.is_number()) ||
					(lhs.is_null() || rhs.is_null()) ||
					(lhs.is_bool() || rhs.is_bool()) ) {
				double l = 0, r = 0;
				
				if ( lhs.type() == jsoncons::json_type::double_value ) {
					l = lhs.template as<double>();
				} else if ( lhs.type() == jsoncons::json_type::string_value ) {
					l = std::stod(lhs.template as<std::string>());
				} else if ( lhs.type() == jsoncons::json_type::bool_value ) {
					l = lhs.template as<bool>() ? 1 : 0;
				} else if ( !lhs.is_null() ) {
					l = lhs.template as<int64_t>();
				} else {
//						std::cout << pretty_print(lhs) << std::endl;
				}
				
				
				if ( rhs.type() == jsoncons::json_type::double_value ) {
					r = rhs.template as<double>();
				} else if ( rhs.type() == jsoncons::json_type::string_value ) {
					r = std::stod(rhs.template as<std::string>());
				} else if ( rhs.type() == jsoncons::json_type::bool_value ) {
					r = rhs.template as<bool>() ? 1 : 0;
				} else if ( !rhs.is_null() ){
					r = rhs.template as<int64_t>();
				} else {
//						std::cout << pretty_print(rhs) << std::endl;
				}
				
				return *resources.create_json(l * r);
			} else if ( lhs.type() == jsoncons::json_type::string_value || rhs.type() == jsoncons::json_type::string_value ) {
				double l = 0, r = 0;
				
				if ( lhs.type() == jsoncons::json_type::string_value )
					l = std::stod(lhs.template as<std::string>());
				else if ( lhs.type() != jsoncons::json_type::null_value )
					l = lhs.template as<double>();

				if ( rhs.type() == jsoncons::json_type::string_value )
					r = std::stod(rhs.template as<std::string>());
				else if ( rhs.type() != jsoncons::json_type::null_value )
					r = rhs.template as<double>();

				return *resources.create_json(l * r);
			} else if ( lhs.type() == jsoncons::json_type::array_value && rhs.type() == jsoncons::json_type::array_value ) {
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				auto numEnts = std::max( lhs.size(), rhs.size() );
				for (int64_t i=0; i<numEnts; i++) {
					double l=0, r=0;
					if ( i < lhs.size() ) l = lhs[i].template as<double>();
					if ( i < rhs.size() ) r = rhs[i].template as<double>();
					result->push_back( l * r);
				}
				return *result;
			} else if ( lhs.type() == jsoncons::json_type::array_value ) {
				double d = rhs.template as<double>();
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : lhs.array_range()) {
					result->push_back( v.template as<double>() * d );
				}
				return *result;
			} else if ( rhs.type() == jsoncons::json_type::array_value ) {
				double d = lhs.template as<double>();
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : rhs.array_range()) {
					result->push_back( v.template as<double>() * d );
				}
				return *result;
			}
			
			return resources.null_value();
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("multiply_operator");
			return s;
		}
	};

	class div_operator final : public binary_operator
	{
	public:
		div_operator()
			: binary_operator(operator_kind::div_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if ( (lhs.is_number() && rhs.is_number()) ||
					(lhs.is_null() || rhs.is_null()) ||
					(lhs.is_bool() || rhs.is_bool()) ) {
				double l = 0, r = 0;
				
				if ( lhs.type() == jsoncons::json_type::double_value ) {
					l = lhs.template as<double>();
				} else if ( lhs.type() == jsoncons::json_type::string_value ) {
					l = std::stod(lhs.template as<std::string>());
				} else if ( lhs.type() == jsoncons::json_type::bool_value ) {
					l = lhs.template as<bool>() ? 1 : 0;
				} else if ( !lhs.is_null() ) {
					l = lhs.template as<int64_t>();
				} else {
//						std::cout << pretty_print(lhs) << std::endl;
				}
				
				
				if ( rhs.type() == jsoncons::json_type::double_value ) {
					r = rhs.template as<double>();
				} else if ( rhs.type() == jsoncons::json_type::string_value ) {
					r = std::stod(rhs.template as<std::string>());
				} else if ( rhs.type() == jsoncons::json_type::bool_value ) {
					r = rhs.template as<bool>() ? 1 : 0;
				} else if ( !rhs.is_null() ){
					r = rhs.template as<int64_t>();
				} else {
//						std::cout << pretty_print(rhs) << std::endl;
				}
				
				if ( r == 0 ) {
					// divide by zero is bad - so return `null`
					return resources.null_value();
				} else
					return *resources.create_json(l / r);
			} else if ( lhs.type() == jsoncons::json_type::string_value || rhs.type() == jsoncons::json_type::string_value ) {
				double l = 0, r = 0;
				
				if ( lhs.type() == jsoncons::json_type::string_value )
					l = std::stod(lhs.template as<std::string>());
				else if ( lhs.type() != jsoncons::json_type::null_value )
					l = lhs.template as<double>();

				if ( rhs.type() == jsoncons::json_type::string_value )
					r = std::stod(rhs.template as<std::string>());
				else if ( rhs.type() != jsoncons::json_type::null_value )
					r = rhs.template as<double>();

				return *resources.create_json(l / r);
			} else if ( lhs.type() == jsoncons::json_type::array_value && rhs.type() == jsoncons::json_type::array_value ) {
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				auto numEnts = std::max( lhs.size(), rhs.size() );
				for (int64_t i=0; i<numEnts; i++) {
					double l=0, r=0;
					if ( i < lhs.size() ) l = lhs[i].template as<double>();
					if ( i < rhs.size() ) r = rhs[i].template as<double>();
					result->push_back( l / r);
				}
				return *result;
			} else if ( lhs.type() == jsoncons::json_type::array_value ) {
				double d = rhs.template as<double>();
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : lhs.array_range()) {
					result->push_back( v.template as<double>() / d );
				}
				return *result;
			} else if ( rhs.type() == jsoncons::json_type::array_value ) {
				double d = lhs.template as<double>();
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : rhs.array_range()) {
					result->push_back( d / v.template as<double>() );
				}
				return *result;
			}
			
			return resources.null_value();
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("divide_operator");
			return s;
		}
	};

	class concat_operator final : public binary_operator
	{
	public:
		concat_operator()
			: binary_operator(operator_kind::concat_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			// as per json-formula spec
			//	Arrays shall be coerced to an array of strings.
			if ( lhs.type() == jsoncons::json_type::array_value && rhs.type() == jsoncons::json_type::array_value ) {
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				auto numEnts = std::max( lhs.size(), rhs.size() );
				for (int64_t i=0; i<numEnts; i++) {
					std::string lhsStr, rhsStr;
					if ( i < lhs.size() ) lhsStr = to_string(lhs[i]);
					if ( i < rhs.size() ) rhsStr = to_string(rhs[i]);
					result->push_back( lhsStr + rhsStr);
				}
				return *result;
			} else if ( lhs.type() == jsoncons::json_type::array_value ) {
				std::string rhsStr{to_string(rhs)};
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : lhs.array_range()) {
					result->push_back( to_string(v) + rhsStr );
				}
				return *result;
			} else if ( rhs.type() == jsoncons::json_type::array_value ) {
				std::string lhsStr{to_string(lhs)};
				auto result = resources.create_json(jsoncons::json_array_arg);	// empty array
				for (auto& v : rhs.array_range()) {
					result->push_back( lhsStr + to_string(v) );
				}
				return *result;
			} else {
				std::string lhsStr{to_string(lhs)}, rhsStr{to_string(rhs)};
				
				if ( !lhsStr.empty() || !rhsStr.empty() )
					return *resources.create_json(lhsStr + rhsStr);
				else
					return resources.null_value();
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("concat_operator");
			return s;
		}

		std::string to_string(reference& r) const
		{
			switch ( r.type() ) {
				case jsoncons::json_type::array_value:
					// should not get here for this scenario...
					break;
				case jsoncons::json_type::uint64_value:
				case jsoncons::json_type::int64_value:
					return std::to_string(r.template as<int64_t>());
					break;
				case jsoncons::json_type::double_value:
					return std::to_string(r.template as<double>());
					break;
				case jsoncons::json_type::string_value:
					return r.template as<std::string>();
					break;
				case jsoncons::json_type::bool_value:
					return r.template as<bool>() ? "true" : "false";
					break;
				case jsoncons::json_type::null_value:
				case jsoncons::json_type::object_value:		// technically an error, but we'll treat an NOP
				case jsoncons::json_type::half_value:			// technically an error, but we'll treat an NOP
				case jsoncons::json_type::byte_string_value:	// technically an error, but we'll treat an NOP
					{
						// do nothing!
					}
					break;
			}
			
			return "";
		}

	};

	class union_operator final : public binary_operator
	{
	public:
		union_operator()
			: binary_operator(operator_kind::union_op)
		{
		}

		reference evaluate(reference lhs, reference rhs, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			auto addToUnion = [&resources](Json* result, reference r) {
				switch ( r.type() ) {
					case jsoncons::json_type::array_value:
						{
							for (auto& v : r.array_range()) {
								result->push_back(v);
							}
						}
						break;
					case jsoncons::json_type::uint64_value:
					case jsoncons::json_type::int64_value:
						{
							result->push_back(*resources.create_json(r.template as<int64_t>()));
						}
						break;
					case jsoncons::json_type::double_value:
						{
							result->push_back(*resources.create_json(r.template as<double>()));
						}
						break;
					case jsoncons::json_type::string_value:
						{
							result->push_back(*resources.create_json(r.template as<std::string>()));
						}
						break;
					case jsoncons::json_type::bool_value:
						{
							result->push_back(*resources.create_json(r.template as<bool>()));
						}
						break;
					case jsoncons::json_type::null_value:
					case jsoncons::json_type::object_value:		// technically an error, but we'll treat an NOP
					case jsoncons::json_type::half_value:			// technically an error, but we'll treat an NOP
					case jsoncons::json_type::byte_string_value:	// technically an error, but we'll treat an NOP
						{
							// do nothing!
						}
						break;
				}
			};
			
			// lhs must be an array...
			//	and then we determine what to do based on the type of rhs
			if ( lhs.type() == jsoncons::json_type::array_value ) {
				auto result = resources.create_json(lhs);	// copy it since we'll be modifying it
				addToUnion(result, rhs);
				return *result;
			} else if ( rhs.type() == jsoncons::json_type::array_value ) {
				auto result = resources.create_json(rhs);	// copy it since we'll be modifying it
				addToUnion(result, lhs);
				return *result;
			}
			
			return resources.null_value();
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("union_operator");
			return s;
		}
	};

	// basic_expression
	class basic_expression :  public expression_base
	{
	public:
		basic_expression()
			: expression_base(operator_kind::default_op, false)
		{
		}

		void add_expression(std::unique_ptr<expression_base>&&) override
		{
		}
	};

	class identifier_selector final : public basic_expression
	{
	private:
		string_type identifier_;
	public:
		identifier_selector(const string_view_type& name)
			: identifier_(name)
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if ( false /*debug_*/ ) {
				std::cout << "(" << to_string()  << "): " << pretty_print(val) << "\n";
			}
			
			if (val.is_object() && val.contains(identifier_))
			{
				return val.at(identifier_);
			}
			else 
			{
				return resources.null_value();
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("identifier_selector ");
			s.append(identifier_);
			return s;
		}
	};

	class current_node final : public basic_expression
	{
	public:
		current_node()
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>&, std::error_code&) const override
		{
			return val;
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("current_node ");
			return s;
		}
	};

	class index_selector final : public basic_expression
	{
		int64_t index_;
	public:
		index_selector(int64_t index)
			: index_(index)
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
		{
			if (!val.is_array())
			{
				return resources.null_value();
			}
			int64_t slen = static_cast<int64_t>(val.size());
			if (index_ >= 0 && index_ < slen)
			{
				std::size_t index = static_cast<std::size_t>(index_);
				return val.at(index);
			}
			else if ((slen + index_) >= 0 && (slen+index_) < slen)
			{
				std::size_t index = static_cast<std::size_t>(slen + index_);
				return val.at(index);
			}
			else
			{
				return resources.null_value();
			}
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("index_selector ");
			s.append(std::to_string(index_));
			return s;
		}
	};

	// projection_base
	class projection_base : public expression_base
	{
	protected:
		std::vector<std::unique_ptr<expression_base>> expressions_;
	public:
		projection_base(operator_kind oper)
			: expression_base(oper, true)
		{
		}

		void add_expression(std::unique_ptr<expression_base>&& expr) override
		{
			if (!expressions_.empty() && expressions_.back()->is_projection() && 
				(expr->precedence_level() < expressions_.back()->precedence_level() ||
					(expr->precedence_level() == expressions_.back()->precedence_level() && expr->is_right_associative())))
			{
				expressions_.back()->add_expression(std::move(expr));
			}
			else
			{
				expressions_.emplace_back(std::move(expr));
			}
		}

		reference apply_expressions(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const
		{
			pointer ptr = std::addressof(val);
			for (auto& expression : expressions_)
			{
				ptr = std::addressof(expression->evaluate(*ptr, resources, ec));
			}
			return *ptr;
		}
	};

	class object_projection final : public projection_base
	{
	public:
		object_projection()
			: projection_base(operator_kind::projection_op)
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
		{
			if (!val.is_object())
			{
				return resources.null_value();
			}

			auto result = resources.create_json(jsoncons::json_array_arg);
			for (auto& item : val.object_range())
			{
				// json-formula supports nulls in projections
				if (true /*!item.value().is_null()*/)
				{
					reference j = this->apply_expressions(item.value(), resources, ec);
					
					// json-formula supports nulls in projections
					if (true /*!j.is_null()*/)
					{
						result->emplace_back(jsoncons::json_const_pointer_arg, std::addressof(j));
					}
				}
			}
			return *result;
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("object_projection\n");
			for (auto& expr : this->expressions_)
			{
				std::string sss = expr->to_string(indent+2);
				s.insert(s.end(), sss.begin(), sss.end());
				s.push_back('\n');
			}
			return s;
		}
	};

	class list_projection final : public projection_base
	{
	public:
		list_projection()
			: projection_base(operator_kind::projection_op)
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
		{
			if (!val.is_array())
			{
				// json-formula allows nulls in list projections
				// but we need to special case them to return an array of null (since it's a projection!)
				if (val.is_null()) {
					auto result = resources.create_json(jsoncons::json_array_arg);
					result->emplace_back(resources.null_value());
					return *result;
				} else
					return resources.null_value();
			}

			auto result = resources.create_json(jsoncons::json_array_arg);
			for (reference item : val.array_range())
			{
				// json-formula allows nulls in list projections
				if (true /*!item.is_null()*/)
				{
					reference j = this->apply_expressions(item, resources, ec);

					// json-formula allows nulls in list projections
					if (true /*!j.is_null()*/)
					{
						result->emplace_back(jsoncons::json_const_pointer_arg, std::addressof(j));
					}
				}
			}
			return *result;
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("list_projection\n");
			for (auto& expr : this->expressions_)
			{
				std::string sss = expr->to_string(indent+2);
				s.insert(s.end(), sss.begin(), sss.end());
				s.push_back('\n');
			}
			return s;
		}
	};

	class slice_projection final : public projection_base
	{
		slice slice_;
	public:
		slice_projection(const slice& s)
			: projection_base(operator_kind::projection_op), slice_(s)
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
		{
			if (!val.is_array())
			{
				return resources.null_value();
			}

			auto start = slice_.get_start(val.size());
			auto end = slice_.get_stop(val.size());
			auto step = slice_.step();

			if (step == 0)
			{
				ec = jsonformula_errc::step_cannot_be_zero;
				return resources.null_value();
			}

			auto result = resources.create_json(jsoncons::json_array_arg);
			if (step > 0)
			{
				if (start < 0)
				{
					start = 0;
				}
				if (end > static_cast<int64_t>(val.size()))
				{
					end = val.size();
				}
				for (int64_t i = start; i < end; i += step)
				{
					reference j = this->apply_expressions(val.at(static_cast<std::size_t>(i)), resources, ec);
					
					// json-formula supports nulls in projections
					if (true /*!j.is_null()*/)
					{
						result->emplace_back(jsoncons::json_const_pointer_arg, std::addressof(j));
					}
				}
			}
			else
			{
				if (start >= static_cast<int64_t>(val.size()))
				{
					start = static_cast<int64_t>(val.size()) - 1;
				}
				if (end < -1)
				{
					end = -1;
				}
				for (int64_t i = start; i > end; i += step)
				{
					reference j = this->apply_expressions(val.at(static_cast<std::size_t>(i)), resources, ec);
					
					// json-formula supports nulls in projections
					if (true /*!j.is_null()*/)
					{
						result->emplace_back(jsoncons::json_const_pointer_arg, std::addressof(j));
					}
				}
			}

			return *result;
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("slice_projection\n");
			for (auto& expr : this->expressions_)
			{
				std::string sss = expr->to_string(indent+2);
				s.insert(s.end(), sss.begin(), sss.end());
				s.push_back('\n');
			}
			return s;
		}
	};

	class filter_expression final : public projection_base
	{
		std::vector<token> token_list_;
	public:
		filter_expression(std::vector<token>&& token_list)
			: projection_base(operator_kind::projection_op), token_list_(std::move(token_list))
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
		{
			if (!val.is_array())
			{
				return resources.null_value();
			}
			auto result = resources.create_json(jsoncons::json_array_arg);

			for (auto& item : val.array_range())
			{
				Json j(jsoncons::json_const_pointer_arg, evaluate_tokens(item, token_list_, resources, ec));
				if (is_true(j))
				{
					reference jj = this->apply_expressions(item, resources, ec);
					
					// json-formula supports nulls in filter expressions
					if (true /*!jj.is_null()*/)
					{
						result->emplace_back(jsoncons::json_const_pointer_arg, std::addressof(jj));
					}
				}
			}
			return *result;
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("filter_expression\n");
			for (auto& item : token_list_)
			{
				std::string sss = item.to_string(indent+2);
				s.insert(s.end(), sss.begin(), sss.end());
				s.push_back('\n');
			}
			return s;
		}
	};

	class flatten_projection final : public projection_base
	{
	public:
		flatten_projection()
			: projection_base(operator_kind::flatten_projection_op)
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
		{
			if (!val.is_array())
			{
				return resources.null_value();
			}

			auto result = resources.create_json(jsoncons::json_array_arg);
			for (reference current_elem : val.array_range())
			{
				if (current_elem.is_array())
				{
					for (reference elem : current_elem.array_range())
					{
						// json-formula allows nulls in projections
						if (true /*!elem.is_null()*/)
						{
							reference j = this->apply_expressions(elem, resources, ec);

							// json-formula allows nulls in projections
							if (true /*!j.is_null()*/)
							{
								result->emplace_back(jsoncons::json_const_pointer_arg, std::addressof(j));
							}
						}
					}
				}
				else
				{
					// json-formula allows nulls in projections
					if (true /*!current_elem.is_null()*/)
					{
						reference j = this->apply_expressions(current_elem, resources, ec);
						
						// json-formula allows nulls in projections
						if (true /*!j.is_null()*/)
						{
							result->emplace_back(jsoncons::json_const_pointer_arg, std::addressof(j));
						}
					}
				}
			}
			return *result;
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("flatten_projection\n");
			for (auto& expr : this->expressions_)
			{
				std::string sss = expr->to_string(indent+2);
				s.insert(s.end(), sss.begin(), sss.end());
				s.push_back('\n');
			}
			return s;
		}
	};

	class multi_select_list final : public basic_expression
	{
		std::vector<std::vector<token>> token_lists_;
	public:
		multi_select_list(std::vector<std::vector<token>>&& token_lists)
			: token_lists_(std::move(token_lists))
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
		{
//				 json-formula allows nulls in projections & lists
//                if (val.is_null())
//                {
//                    return val;
//                }

			// in json-formula, empty objects aren't allowed
			if ( token_lists_.size()==0 ) {
				ec = jsonformula_errc::invalid_object;
				return resources.null_value();
			}
			
			auto result = resources.create_json(jsoncons::json_array_arg);
			result->reserve(token_lists_.size());

			for (auto& list : token_lists_)
			{
				result->emplace_back(jsoncons::json_const_pointer_arg, evaluate_tokens(val, list, resources, ec));
			}
			return *result;
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("multi_select_list\n");
			for (auto& list : token_lists_)
			{
				for (auto& item : list)
				{
					std::string sss = item.to_string(indent+2);
					s.insert(s.end(), sss.begin(), sss.end());
					s.push_back('\n');
				}
				s.append("---\n");
			}
			return s;
		}
	};

	struct key_tokens
	{
		string_type key;
		std::vector<token> tokens;

		key_tokens(string_type&& key, std::vector<token>&& tokens) noexcept
			: key(std::move(key)), tokens(std::move(tokens))
		{
		}
	};

	class multi_select_hash final : public basic_expression
	{
	public:
		std::vector<key_tokens> key_toks_;

		multi_select_hash(std::vector<key_tokens>&& key_toks)
			: key_toks_(std::move(key_toks))
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
		{
			// in json-formula, null == empty object!
//                if (val.is_null())
//                {
//                    return val;
//                }
			
			// in json-formula, empty objects aren't allowed
			if ( key_toks_.size()==0 ) {
				ec = jsonformula_errc::invalid_object;
				return resources.null_value();
			}
			
			auto resultp = resources.create_json(jsoncons::json_object_arg);
			resultp->reserve(key_toks_.size());
			for (auto& item : key_toks_)
			{
				resultp->try_emplace(item.key, jsoncons::json_const_pointer_arg, evaluate_tokens(val, item.tokens, resources, ec));
			}

			return *resultp;
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("multi_select_list");
			return s;
		}
	};

	class function_expression final : public basic_expression
	{
	public:
		std::vector<token> toks_;

		function_expression(std::vector<token>&& toks)
			: toks_(std::move(toks))
		{
		}

		reference evaluate(reference val, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
		{
			return *evaluate_tokens(val, toks_, resources, ec);
		}

		std::string to_string(std::size_t indent = 0) const override
		{
			std::string s;
			for (std::size_t i = 0; i <= indent; ++i)
			{
				s.push_back(' ');
			}
			s.append("function_expression\n");
			for (auto& tok : toks_)
			{
				for (std::size_t i = 0; i <= indent+2; ++i)
				{
					s.push_back(' ');
				}
				std::string sss = tok.to_string(indent+2);
				s.insert(s.end(), sss.begin(), sss.end());
				s.push_back('\n');
			}
			return s;
		}
	};

	class static_resources
	{
		std::vector<std::unique_ptr<Json>> temp_storage_;

	public:

		static_resources() = default;
		static_resources(const static_resources& expr) = delete;
		static_resources& operator=(const static_resources& expr) = delete;
		static_resources(static_resources&& expr) = default;
		static_resources& operator=(static_resources&& expr) = default;

		const function_base* get_function(const string_type& name, std::error_code& ec) const
		{
			static abs_function abs_func;
			static avg_function avg_func;
			static ceil_function ceil_func;
			static contains_function contains_func;
			static ends_with_function ends_with_func;
			static floor_function floor_func;
			static join_function join_func;
			static length_function length_func;
			static max_function max_func;
			static max_by_function max_by_func;
			static map_function map_func;
			static merge_function merge_func;
			static min_function min_func;
			static min_by_function min_by_func;
			static type_function type_func;
			static sort_function sort_func;
			static sort_by_function sort_by_func;
			static keys_function keys_func;
			static values_function values_func;
			static reverse_function reverse_func;
			static starts_with_function starts_with_func;
			static const sum_function sum_func;
			static to_array_function to_array_func;
			static to_number_function to_number_func;
			static to_string_function to_string_func;
			static not_null_function not_null_func;
			static value_function value_func;

			using function_dictionary = std::unordered_map<string_type,const function_base*>;
			static const function_dictionary functions_ =
			{
				{string_type{'a','b','s'}, &abs_func},
				{string_type{'a','v','g'}, &avg_func},
				{string_type{'c','e','i', 'l'}, &ceil_func},
				{string_type{'c','o','n', 't', 'a', 'i', 'n', 's'}, &contains_func},
				{string_type{'e','n','d', 's', '_', 'w', 'i', 't', 'h'}, &ends_with_func},
				{string_type{'f','l','o', 'o', 'r'}, &floor_func},
				{string_type{'j','o','i', 'n'}, &join_func},
				{string_type{'l','e','n', 'g', 't', 'h'}, &length_func},
				{string_type{'m','a','x'}, &max_func},
				{string_type{'m','a','x','_','b','y'}, &max_by_func},
				{string_type{'m','a','p'}, &map_func},
				{string_type{'m','i','n'}, &min_func},
				{string_type{'m','i','n','_','b','y'}, &min_by_func},
				{string_type{'m','e','r', 'g', 'e'}, &merge_func},
				{string_type{'t','y','p', 'e'}, &type_func},
				{string_type{'s','o','r', 't'}, &sort_func},
				{string_type{'s','o','r', 't','_','b','y'}, &sort_by_func},
				{string_type{'k','e','y', 's'}, &keys_func},
				{string_type{'v','a','l', 'u','e','s'}, &values_func},
				{string_type{'r','e','v', 'e', 'r', 's','e'}, &reverse_func},
				{string_type{'s','t','a', 'r','t','s','_','w','i','t','h'}, &starts_with_func},
				{string_type{'s','u','m'}, &sum_func},
				{string_type{'t','o','_','a','r','r','a','y',}, &to_array_func},
				{string_type{'t','o','_', 'n', 'u', 'm','b','e','r'}, &to_number_func},
				{string_type{'t','o','_', 's', 't', 'r','i','n','g'}, &to_string_func},
				{string_type{'n','o','t', '_', 'n', 'u','l','l'}, &not_null_func},
				
				// json_formula changes & additions
				{string_type{'e','n','d', 's', 'W', 'i', 't', 'h'}, &ends_with_func},
				{string_type{'m','i','n','B','y'}, &min_by_func},
				{string_type{'n','o','t', 'N', 'u','l','l'}, &not_null_func},
				{string_type{'s','o','r', 't','B','y'}, &sort_by_func},
				{string_type{'s','t','a', 'r','t','s','W','i','t','h'}, &starts_with_func},
				{string_type{'t','o','A','r','r','a','y',}, &to_array_func},
				{string_type{'t','o','N', 'u', 'm','b','e','r'}, &to_number_func},
				{string_type{'t','o','S', 't', 'r','i','n','g'}, &to_string_func},
				{string_type{'v','a','l', 'u','e'}, &value_func},
			};
			auto it = functions_.find(name);
			if (it == functions_.end())
			{
				ec = jsonformula_errc::unknown_function;
				return nullptr;
			}
			return it->second;
		}

		const unary_operator* get_not_operator() const
		{
			static const not_expression not_oper;

			return &not_oper;
		}

		const binary_operator* get_or_operator() const
		{
			static const or_operator or_oper;

			return &or_oper;
		}

		const binary_operator* get_and_operator() const
		{
			static const and_operator and_oper;

			return &and_oper;
		}

		const binary_operator* get_eq_operator() const
		{
			static const eq_operator eq_oper;
			return &eq_oper;
		}

		const binary_operator* get_ne_operator() const
		{
			static const ne_operator ne_oper;
			return &ne_oper;
		}

		const binary_operator* get_lt_operator() const
		{
			static const lt_operator lt_oper;
			return &lt_oper;
		}

		const binary_operator* get_lte_operator() const
		{
			static const lte_operator lte_oper;
			return &lte_oper;
		}

		const binary_operator* get_gt_operator() const
		{
			static const gt_operator gt_oper;
			return &gt_oper;
		}

		const binary_operator* get_gte_operator() const
		{
			static const gte_operator gte_oper;
			return &gte_oper;
		}

		const binary_operator* get_plus_operator() const
		{
			static const plus_operator plus_oper;
			return &plus_oper;
		}
		const binary_operator* get_minus_operator() const
		{
			static const minus_operator minus_oper;
			return &minus_oper;
		}
		const binary_operator* get_mult_operator() const
		{
			static const mult_operator mult_oper;
			return &mult_oper;
		}
		const binary_operator* get_div_operator() const
		{
			static const div_operator div_oper;
			return &div_oper;
		}
		const binary_operator* get_concat_operator() const
		{
			static const concat_operator concat_oper;
			return &concat_oper;
		}
		const binary_operator* get_union_operator() const
		{
			static const union_operator union_oper;
			return &union_oper;
		}
	};

	class jsonformula_expression
	{
		static_resources resources_;
		std::vector<token> output_stack_;
	public:
		jsonformula_expression()
		{
		}

		jsonformula_expression(const jsonformula_expression& expr) = delete;
		jsonformula_expression& operator=(const jsonformula_expression& expr) = delete;

		jsonformula_expression(jsonformula_expression&& expr)
			: resources_(std::move(expr.resources_)),
				output_stack_(std::move(expr.output_stack_))
		{
		}

		jsonformula_expression(static_resources&& resources,
							std::vector<token>&& output_stack)
			: resources_(std::move(resources)), output_stack_(std::move(output_stack))
		{
		}

		Json evaluate(reference doc)
		{
			if (output_stack_.empty())
			{
				return Json::null();
			}
			std::error_code ec;
			Json result = evaluate(doc, ec);
			if (ec)
			{
				JSONCONS_THROW(jsonformula_error(ec));
			}
			return result;
		}

		Json evaluate(reference doc, std::error_code& ec)
		{
			if (output_stack_.empty())
			{
				return Json::null();
			}
			dynamic_resources<Json,JsonReference> dynamic_storage;
			return deep_copy(*evaluate_tokens(doc, output_stack_, dynamic_storage, ec));
		}

		static jsonformula_expression compile(const string_view_type& expr)
		{
			jsonformula::detail::jsonformula_evaluator<Json,const Json&> evaluator;
			std::error_code ec;
			jsonformula_expression result = evaluator.compile(expr.data(), expr.size(), ec);
			if (ec)
			{
				JSONCONS_THROW(jsonformula_error(ec, evaluator.line(), evaluator.column()));
			}
			return result;
		}

		static jsonformula_expression compile(const string_view_type& expr,
											std::error_code& ec)
		{
			jsonformula::detail::jsonformula_evaluator<Json,const Json&> evaluator;
			return evaluator.compile(expr.data(), expr.size(), ec);
		}
	};
private:
	std::size_t line_;
	std::size_t column_;
	const char_type* begin_input_;
	const char_type* end_input_;
	const char_type* p_;

	static_resources resources_;
	std::vector<path_state> state_stack_;

	std::vector<token> output_stack_;
	std::vector<token> operator_stack_;
	
	
	bool debug_;

public:
	jsonformula_evaluator()
		: line_(1), column_(1),
			begin_input_(nullptr), end_input_(nullptr),
			p_(nullptr), debug_(false)
	{
	}

	std::size_t line() const
	{
		return line_;
	}

	std::size_t column() const
	{
		return column_;
	}
	
	void debug(bool d=true) { debug_ = d; }

	jsonformula_expression compile(const char_type* path,
								std::size_t length,
								std::error_code& ec)
	{
		auto _checkBuffer = [this, &ec](string_type& buffer) {
			if (!buffer.empty()) {
				jsoncons::json_decoder<Json> decoder;
				jsoncons::basic_json_reader<char_type,jsoncons::string_source<char_type>> reader(buffer, decoder);
				std::error_code parse_ec;
				reader.read(parse_ec);
				if (parse_ec) {
					ec = jsonformula_errc::invalid_literal;
					return;
				}
				auto j = decoder.get_result();
				push_token(token(literal_arg, std::move(j)), ec);
				buffer.clear();
			}
		};
		
		push_token(current_node_arg, ec);
		if (ec) {return jsonformula_expression();}
		state_stack_.emplace_back(path_state::start);

		string_type buffer;
		uint32_t cp = 0;
		uint32_t cp2 = 0;
	
		begin_input_ = path;
		end_input_ = path + length;
		p_ = begin_input_;

		slice slic{};

		while (p_ < end_input_)
		{
			switch (state_stack_.back())
			{
				case path_state::start: 
				{
					state_stack_.back() = path_state::rhs_expression;
					state_stack_.emplace_back(path_state::lhs_expression);
					break;
				}
				case path_state::rhs_expression:
					switch(*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case '.': 
							++p_;
							++column_;
							state_stack_.emplace_back(path_state::sub_expression);
							break;
						case '|':
							++p_;
							++column_;
							state_stack_.emplace_back(path_state::lhs_expression);
							state_stack_.emplace_back(path_state::expect_pipe_or_or);
							break;
						case '&':
							++p_;
							++column_;
							state_stack_.emplace_back(path_state::lhs_expression);
							state_stack_.emplace_back(path_state::expect_and);
							break;
						case '<':
						case '>':
						case '=':
						{
							state_stack_.emplace_back(path_state::comparator_expression);
							break;
						}
						case '!':
						{
							++p_;
							++column_;
							state_stack_.emplace_back(path_state::lhs_expression);
							state_stack_.emplace_back(path_state::cmp_ne);
							break;
						}
						case ')':
						{
							// json-formula
							// check if there is anything in the buffer...
							// if so, we need to process it
							_checkBuffer(buffer);

							state_stack_.pop_back();
							break;
						}
						case '[':
							++p_;
							++column_;
							state_stack_.emplace_back(path_state::bracket_specifier);
							break;
						// json-formula
						case '+':
						case '-':
						case '*':
						case '/':
						case '~':
							state_stack_.emplace_back(path_state::jf_expression);
							break;
						default:
							if (state_stack_.size() > 1)
							{
								state_stack_.pop_back();
							}
							else
							{
								ec = jsonformula_errc::syntax_error;
								return jsonformula_expression();
							}
							break;
					}
					break;
				case path_state::comparator_expression:
					switch(*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case '<':
							++p_;
							++column_;
							state_stack_.back() = path_state::lhs_expression;
							state_stack_.emplace_back(path_state::cmp_lt_or_lte);
							break;
						case '>':
							++p_;
							++column_;
							state_stack_.back() = path_state::lhs_expression;
							state_stack_.emplace_back(path_state::cmp_gt_or_gte);
							break;
						case '=':
						{
							++p_;
							++column_;
							state_stack_.back() = path_state::lhs_expression;
							state_stack_.emplace_back(path_state::cmp_eq);
							break;
						}
						default:
							if (state_stack_.size() > 1)
							{
								state_stack_.pop_back();
							}
							else
							{
								ec = jsonformula_errc::syntax_error;
								return jsonformula_expression();
							}
							break;
					}
					break;
				case path_state::lhs_expression: 
				{
					switch (*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case '\"':
							// in json-formula, quoted strings are strings
//                                state_stack_.back() = path_state::val_expr;
							state_stack_.back() = path_state::quoted_string;
							++p_;
							++column_;
							break;
						case '\'':
							// in json-formula, single quoted/raw strings are value expression
							state_stack_.back() = path_state::val_expr;
							state_stack_.emplace_back(path_state::raw_string);
							++p_;
							++column_;
							break;
						case '`':
							state_stack_.back() = path_state::literal;
							++p_;
							++column_;
							break;
						case '{':
							push_token(begin_multi_select_hash_arg, ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::multi_select_hash;
							++p_;
							++column_;
							break;
						case '*': // wildcard
							push_token(token(jsoncons::make_unique<object_projection>()), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
						case '(':
						{
							++p_;
							++column_;
							push_token(lparen_arg, ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::expect_rparen;
							state_stack_.emplace_back(path_state::rhs_expression);
							state_stack_.emplace_back(path_state::lhs_expression);
							break;
						}
						case '!':
						{
							++p_;
							++column_;
							push_token(token(resources_.get_not_operator()), ec);
							if (ec) {return jsonformula_expression();}
							break;
						}
						case '@':
							++p_;
							++column_;
							push_token(token(jsoncons::make_unique<current_node>()), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();
							break;
						case '[': 
							state_stack_.back() = path_state::bracket_specifier_or_multi_select_list;
							++p_;
							++column_;
							break;
						default:
							if ((*p_ >= 'A' && *p_ <= 'Z') || (*p_ >= 'a' && *p_ <= 'z') || (*p_ == '_'))
							{
								state_stack_.back() = path_state::identifier_or_function_expr;
								state_stack_.emplace_back(path_state::unquoted_string);
								buffer.push_back(*p_);
								++p_;
								++column_;
							}
							else if ((*p_ >= '0' && *p_ <= '9') || (*p_ == '-') || (*p_ == '.')) //json-formula supports #'s
							{
								state_stack_.back() = path_state::number;
								buffer.push_back(*p_);
								++p_;
								++column_;
							}
							else
							{
								ec = jsonformula_errc::expected_identifier;
								return jsonformula_expression();
							}
							break;
					};
					break;
				}
				case path_state::sub_expression: 
				{
					switch (*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case '\"':
							// in json-formula, quoted strings are strings
//                                state_stack_.back() = path_state::val_expr;
							state_stack_.emplace_back(path_state::quoted_string);
							++p_;
							++column_;
							break;
						case '\'':
							// in json-formula, single quoted/raw strings are value expression
							state_stack_.back() = path_state::val_expr;
							state_stack_.emplace_back(path_state::raw_string);
							++p_;
							++column_;
							break;
						case '{':
							push_token(begin_multi_select_hash_arg, ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::multi_select_hash;
							++p_;
							++column_;
							break;
						case '*':
							push_token(token(jsoncons::make_unique<object_projection>()), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
						case '[': 
							state_stack_.back() = path_state::expect_multi_select_list;
							++p_;
							++column_;
							break;
						default:
							if ((*p_ >= 'A' && *p_ <= 'Z') || (*p_ >= 'a' && *p_ <= 'z') || (*p_ == '_'))
							{
								state_stack_.back() = path_state::identifier_or_function_expr;
								state_stack_.emplace_back(path_state::unquoted_string);
								buffer.push_back(*p_);
								++p_;
								++column_;
							}
							else
							{
								ec = jsonformula_errc::expected_identifier;
								return jsonformula_expression();
							}
							break;
					};
					break;
				}
				case path_state::key_expr:
					push_token(token(key_arg, buffer), ec);
					if (ec) {return jsonformula_expression();}
					buffer.clear(); 
					state_stack_.pop_back(); 
					break;
				case path_state::val_expr:
					push_token(token(jsoncons::make_unique<identifier_selector>(buffer)), ec);
					if (ec) {return jsonformula_expression();}
					buffer.clear();
					state_stack_.pop_back(); 
					break;
				case path_state::expression_or_expression_type:
					switch (*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case '&':
							push_token(token(begin_expression_type_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::expression_type;
							state_stack_.emplace_back(path_state::rhs_expression);
							state_stack_.emplace_back(path_state::lhs_expression);
							++p_;
							++column_;
							break;
						//json-formula supports #'s
						case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
						case '.':	// don't forget decimal places!
						case '-':	// don't forget negative numbers!
							// we need to first mark this as an argument
							state_stack_.back() = path_state::argument;
							state_stack_.emplace_back(path_state::rhs_expression);
							state_stack_.emplace_back(path_state::lhs_expression);

							// and now handle the number part...
							state_stack_.back() = path_state::number;
							buffer.push_back(*p_);
							++p_;
							++column_;
							break;
						default:
							state_stack_.back() = path_state::argument;
							state_stack_.emplace_back(path_state::rhs_expression);
							state_stack_.emplace_back(path_state::lhs_expression);
							break;
					}
					break;
				case path_state::identifier_or_function_expr:
					switch(*p_)
					{
						case '(':
						{
							auto f = resources_.get_function(buffer, ec);
							if (ec)
							{
								return jsonformula_expression();
							}
							buffer.clear();
							push_token(token(f), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::function_expression;
							state_stack_.emplace_back(path_state::expression_or_expression_type);
							++p_;
							++column_;
							break;
						}
						default:
						{
							push_token(token(jsoncons::make_unique<identifier_selector>(buffer)), ec);
							if (ec) {return jsonformula_expression();}
							buffer.clear();
							state_stack_.pop_back(); 
							break;
						}
					}
					break;

				case path_state::function_expression:
				{
					// json-formula
					// check if there is anything in the buffer...
					// if so, we need to process it
					_checkBuffer(buffer);

					switch (*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case ',':
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.emplace_back(path_state::expression_or_expression_type);
							++p_;
							++column_;
							break;
						case ')':
						{
							push_token(token(end_function_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
						}
						default:
							break;
					}
				}
					break;

				case path_state::argument:
					push_token(argument_arg, ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					break;

				case path_state::expression_type:
					push_token(end_expression_type_arg, ec);
					push_token(argument_arg, ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					break;

				case path_state::quoted_string: 
					switch (*p_)
					{
						case '\"':
						{
							// in json-formula, these should be treated as literal strings
							// except when they actually mean identifiers
							auto stack_grandparent = state_stack_[state_stack_.size()-2];
							
							if ( stack_grandparent == path_state::key_expr ||
								stack_grandparent == path_state::sub_expression ) {
								state_stack_.pop_back(); // quoted_string
								state_stack_.back() = path_state::val_expr;	// reset this to a val expression...
							} else {
								push_token(token(literal_arg, Json(buffer)), ec);
								if (ec) {return jsonformula_expression();}
								buffer.clear();

								state_stack_.pop_back(); // quoted_string
							}

							++p_;
							++column_;
						}
							break;
						case '\\':
							state_stack_.emplace_back(path_state::quoted_string_escape_char);
							++p_;
							++column_;
							break;
						default:
							buffer.push_back(*p_);
							++p_;
							++column_;
							break;
					};
					break;

				case path_state::unquoted_string: 
					switch (*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							{
								string_type dummy;
								state_stack_.pop_back(); // unquoted_string
								advance_past_space_character(ec, dummy);
							}
							break;
						default:
							if ((*p_ >= '0' && *p_ <= '9') || (*p_ >= 'A' && *p_ <= 'Z') || (*p_ >= 'a' && *p_ <= 'z') || (*p_ == '_'))
							{
								buffer.push_back(*p_);
								++p_;
								++column_;
							}
							else
							{
								state_stack_.pop_back(); // unquoted_string
							}
							break;
					};
					break;
				case path_state::raw_string_escape_char:
					switch (*p_)
					{
						case '\'':
							buffer.push_back(*p_);
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
						// json-formula also supports escaping the escape character!
						// it also supports the same set of escapes as double strings.
						case '\"':
							buffer.push_back('\"');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case '\\':
							buffer.push_back('\\');	
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
						case '/':
							buffer.push_back('/');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'b':
							buffer.push_back('\b');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'f':
							buffer.push_back('\f');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'n':
							buffer.push_back('\n');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'r':
							buffer.push_back('\r');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 't':
							buffer.push_back('\t');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'u':
							++p_;
							++column_;
							state_stack_.back() = path_state::escape_u1;
							break;
						default:
							buffer.push_back('\\');
							buffer.push_back(*p_);
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
					}
					break;
				case path_state::quoted_string_escape_char:
					switch (*p_)
					{
						case '\"':
							buffer.push_back('\"');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case '\\': 
							buffer.push_back('\\');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case '/':
							buffer.push_back('/');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'b':
							buffer.push_back('\b');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'f':
							buffer.push_back('\f');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'n':
							buffer.push_back('\n');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'r':
							buffer.push_back('\r');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 't':
							buffer.push_back('\t');
							++p_;
							++column_;
							state_stack_.pop_back();
							break;
						case 'u':
							++p_;
							++column_;
							state_stack_.back() = path_state::escape_u1;
							break;
						default:
							ec = jsonformula_errc::illegal_escaped_character;
							return jsonformula_expression();
					}
					break;
				case path_state::escape_u1:
					cp = append_to_codepoint(0, *p_, ec);
					if (ec)
					{
						return jsonformula_expression();
					}
					++p_;
					++column_;
					state_stack_.back() = path_state::escape_u2;
					break;
				case path_state::escape_u2:
					cp = append_to_codepoint(cp, *p_, ec);
					if (ec)
					{
						return jsonformula_expression();
					}
					++p_;
					++column_;
					state_stack_.back() = path_state::escape_u3;
					break;
				case path_state::escape_u3:
					cp = append_to_codepoint(cp, *p_, ec);
					if (ec)
					{
						return jsonformula_expression();
					}
					++p_;
					++column_;
					state_stack_.back() = path_state::escape_u4;
					break;
				case path_state::escape_u4:
					cp = append_to_codepoint(cp, *p_, ec);
					if (ec)
					{
						return jsonformula_expression();
					}
					if (jsoncons::unicode_traits::is_high_surrogate(cp))
					{
						++p_;
						++column_;
						state_stack_.back() = path_state::escape_expect_surrogate_pair1;
					}
					else
					{
						jsoncons::unicode_traits::convert(&cp, 1, buffer);
						++p_;
						++column_;
						state_stack_.pop_back();
					}
					break;
				case path_state::escape_expect_surrogate_pair1:
					switch (*p_)
					{
						case '\\': 
							++p_;
							++column_;
							state_stack_.back() = path_state::escape_expect_surrogate_pair2;
							break;
						default:
							ec = jsonformula_errc::invalid_codepoint;
							return jsonformula_expression();
					}
					break;
				case path_state::escape_expect_surrogate_pair2:
					switch (*p_)
					{
						case 'u': 
							++p_;
							++column_;
							state_stack_.back() = path_state::escape_u5;
							break;
						default:
							ec = jsonformula_errc::invalid_codepoint;
							return jsonformula_expression();
					}
					break;
				case path_state::escape_u5:
					cp2 = append_to_codepoint(0, *p_, ec);
					if (ec)
					{
						return jsonformula_expression();
					}
					++p_;
					++column_;
					state_stack_.back() = path_state::escape_u6;
					break;
				case path_state::escape_u6:
					cp2 = append_to_codepoint(cp2, *p_, ec);
					if (ec)
					{
						return jsonformula_expression();
					}
					++p_;
					++column_;
					state_stack_.back() = path_state::escape_u7;
					break;
				case path_state::escape_u7:
					cp2 = append_to_codepoint(cp2, *p_, ec);
					if (ec)
					{
						return jsonformula_expression();
					}
					++p_;
					++column_;
					state_stack_.back() = path_state::escape_u8;
					break;
				case path_state::escape_u8:
				{
					cp2 = append_to_codepoint(cp2, *p_, ec);
					if (ec)
					{
						return jsonformula_expression();
					}
					uint32_t codepoint = 0x10000 + ((cp & 0x3FF) << 10) + (cp2 & 0x3FF);
					jsoncons::unicode_traits::convert(&codepoint, 1, buffer);
					state_stack_.pop_back();
					++p_;
					++column_;
					break;
				}
				case path_state::raw_string: 
					switch (*p_)
					{
						case '\'':
						{
							// in json-formula, these should be treated as literal strings
							// except when they actually mean identifiers
							auto stack_grandparent = state_stack_[state_stack_.size()-2];
							
							if ( stack_grandparent == path_state::sub_expression ) {
								state_stack_.pop_back(); // raw_string
								state_stack_.back() = path_state::val_expr;	// reset this to a val expression...
							} else {
								state_stack_.pop_back(); // raw_string
							}

							++p_;
							++column_;
							break;
						}
						case '\\':
							state_stack_.emplace_back(path_state::raw_string_escape_char);
							++p_;
							++column_;
							break;
						default:
							buffer.push_back(*p_);
							++p_;
							++column_;
							break;
					};
					break;
				case path_state::literal: 
					switch (*p_)
					{
						case '`':
						{
							jsoncons::json_decoder<Json> decoder;
							jsoncons::basic_json_reader<char_type,jsoncons::string_source<char_type>> reader(buffer, decoder);
							std::error_code parse_ec;
							reader.read(parse_ec);
							if (parse_ec)
							{
								ec = jsonformula_errc::invalid_literal;
								return jsonformula_expression();
							}
							auto j = decoder.get_result();

							push_token(token(literal_arg, std::move(j)), ec);
							if (ec) {return jsonformula_expression();}
							buffer.clear();
							state_stack_.pop_back(); // json_value
							++p_;
							++column_;
							break;
						}
						case '\\':
							if (p_+1 < end_input_)
							{
								++p_;
								++column_;
								if (*p_ != '`')
								{
									buffer.push_back('\\');
								}
								buffer.push_back(*p_);
							}
							else
							{
								ec = jsonformula_errc::unexpected_end_of_input;
								return jsonformula_expression();
							}
							++p_;
							++column_;
							break;
						default:
							buffer.push_back(*p_);
							++p_;
							++column_;
							break;
					};
					break;
				case path_state::number:
					switch(*p_)
					{
						case '-':
						case '+':
						{
							bool digit_follows = false;	// assume the worst
							auto stack_grandparent = state_stack_[state_stack_.size()-2];
							
							if ( buffer[buffer.length()-1]=='e' || buffer[buffer.length()-1]=='E' ) {
								// json-formula also supports exponents, and next is a digit
								digit_follows = true;
							} else if ( stack_grandparent == path_state::index_or_slice_expression ||
										stack_grandparent == path_state::rhs_slice_expression_step ||
										stack_grandparent == path_state::rhs_slice_expression_stop ) {
								// we are in an index or slice expression
								digit_follows = true;
							}
							
							if ( digit_follows ) {
								buffer.push_back(*p_);
								state_stack_.back() = path_state::digit;
								++p_;
								++column_;
							} else {	// it's an equation/formula, so pop out of number!
								state_stack_.pop_back();
							}
						}
							break;
							
						case '.':	// don't forget decimal places!
							buffer.push_back(*p_);
							state_stack_.back() = path_state::digit;
							++p_;
							++column_;
							break;
						case 'e':	// json-formula also supports exponents
						case 'E':
							buffer.push_back(*p_);
							state_stack_.back() = path_state::number;	// keep a number since it might be fllowed by +/-
							++p_;
							++column_;
							break;
						default:
							state_stack_.back() = path_state::digit;
							break;
					}
					break;
				case path_state::digit:
					switch(*p_)
					{
						case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
						case '.':	// don't forget decimal places!
							buffer.push_back(*p_);
							++p_;
							++column_;
							break;
						default:
							state_stack_.pop_back(); // digit
							break;
					}
					break;

				case path_state::bracket_specifier:
					switch(*p_)
					{
						case '*':
							push_token(token(jsoncons::make_unique<list_projection>()), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::expect_rbracket;
							++p_;
							++column_;
							break;
						case ']': // []
							push_token(token(jsoncons::make_unique<flatten_projection>()), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back(); // bracket_specifier
							++p_;
							++column_;
							break;
						case '?':
							push_token(token(begin_filter_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::filter;
							state_stack_.emplace_back(path_state::rhs_expression);
							state_stack_.emplace_back(path_state::lhs_expression);
							++p_;
							++column_;
							break;
						case ':': // slice_expression
							state_stack_.back() = path_state::rhs_slice_expression_stop ;
							state_stack_.emplace_back(path_state::number);
							++p_;
							++column_;
							break;
						// number
						case '-':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
							state_stack_.back() = path_state::index_or_slice_expression;
							state_stack_.emplace_back(path_state::number);
							break;
						default:
							ec = jsonformula_errc::expected_index_expression;
							return jsonformula_expression();
					}
					break;
				case path_state::bracket_specifier_or_multi_select_list:
					switch(*p_)
					{
						case '*':
							if (p_+1 >= end_input_)
							{
								ec = jsonformula_errc::unexpected_end_of_input;
								return jsonformula_expression();
							}
							if (*(p_+1) == ']')
							{
								state_stack_.back() = path_state::bracket_specifier;
							}
							else
							{
								push_token(token(begin_multi_select_list_arg), ec);
								if (ec) {return jsonformula_expression();}
								state_stack_.back() = path_state::multi_select_list;
								state_stack_.emplace_back(path_state::lhs_expression);                                
							}
							break;
						case ']': // []
						case '?':
						case ':': // slice_expression
						case '-':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
							state_stack_.back() = path_state::bracket_specifier;
							break;
						default:
							push_token(token(begin_multi_select_list_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::multi_select_list;
							state_stack_.emplace_back(path_state::lhs_expression);
							break;
					}
					break;

				case path_state::expect_multi_select_list:
					switch(*p_)
					{
						case ']':
						case '?':
						case ':':
							ec = jsonformula_errc::expected_multi_select_list;
							return jsonformula_expression();
						// json-formula allows indices here...
						case '-':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
							// need to push the multi-select token on first
							// then setup the state stack
							push_token(token(begin_multi_select_list_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::multi_select_list;
							state_stack_.emplace_back(path_state::number);
							break;
						case '*':
							push_token(token(jsoncons::make_unique<list_projection>()), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::expect_rbracket;
							++p_;
							++column_;
							break;
						default:
							push_token(token(begin_multi_select_list_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::multi_select_list;
							state_stack_.emplace_back(path_state::lhs_expression);
							break;
					}
					break;

				case path_state::multi_select_hash:
					switch(*p_)
					{
						case '*':
						case ']':
						case '?':
						case ':':
						case '-':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
							break;
						// json-formula supports empty objects
						case '}':
						{
							state_stack_.pop_back();
							push_token(end_multi_select_hash_arg, ec);
							if (ec) {return jsonformula_expression();}
							++p_;
							++column_;
							break;
						}
						default:
							state_stack_.back() = path_state::key_val_expr;
							break;
					}
					break;

				case path_state::index_or_slice_expression:
					switch(*p_)
					{
						case ']':
						{
							if (buffer.empty())
							{
								push_token(token(jsoncons::make_unique<flatten_projection>()), ec);
								if (ec) {return jsonformula_expression();}
							}
							else
							{
								int64_t val{ 0 };
								auto r = jsoncons::detail::to_integer(buffer.data(), buffer.size(), val);
								if (!r)
								{
									ec = jsonformula_errc::invalid_number;
									return jsonformula_expression();
								}
								push_token(token(jsoncons::make_unique<index_selector>(val)), ec);
								if (ec) {return jsonformula_expression();}

								buffer.clear();
							}
							state_stack_.pop_back(); // bracket_specifier
							++p_;
							++column_;
							break;
						}
						case ':':
						{
							if (!buffer.empty())
							{
								int64_t val;
								auto r = jsoncons::detail::to_integer(buffer.data(), buffer.size(), val);
								if (!r)
								{
									ec = jsonformula_errc::invalid_number;
									return jsonformula_expression();
								}
								slic.start_ = val;
								buffer.clear();
							}
							state_stack_.back() = path_state::rhs_slice_expression_stop;
							state_stack_.emplace_back(path_state::number);
							++p_;
							++column_;
							break;
						}
						case ',':
						{
							// json-formula
							//	if we landed here, then we MAY have found a raw array of numbers
							//	it is also possible that it is a bogus array index...
							//	we check which, based on the output_stack
							//	if it's a raw array, reset the stack to what it should be...
							//		Push on the multi-select token
							//		Replace the current state (index or slice) with multi-select list
							//  otherwise, leave it alone!
							if ( output_stack_[output_stack_.size()-1].type() == token_kind::expression ) {
								// nothing to do...
							} else {
								push_token(token(begin_multi_select_list_arg), ec);
								if (ec) {return jsonformula_expression();}
								state_stack_.back() = path_state::multi_select_list;
							}
							
							// check if there is anything in the buffer... (there better be!)
							// which will push a literal onto the token stack
								_checkBuffer(buffer);
							
							// now push on the seperator
							// and push lhs_expression onto the state stack
							push_token(token(separator_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.emplace_back(path_state::lhs_expression);
							++p_;
							++column_;
							break;
						}
						default:
							ec = jsonformula_errc::expected_rbracket;
							return jsonformula_expression();
					}
					break;
				case path_state::rhs_slice_expression_stop :
				{
					if (!buffer.empty())
					{
						int64_t val{ 0 };
						auto r = jsoncons::detail::to_integer(buffer.data(), buffer.size(), val);
						if (!r)
						{
							ec = jsonformula_errc::invalid_number;
							return jsonformula_expression();
						}
						slic.stop_ = jsoncons::optional<int64_t>(val);
						buffer.clear();
					}
					switch(*p_)
					{
						case ']':
							push_token(token(jsoncons::make_unique<slice_projection>(slic)), ec);
							if (ec) {return jsonformula_expression();}
							slic = slice{};
							state_stack_.pop_back(); // bracket_specifier2
							++p_;
							++column_;
							break;
						case ':':
							state_stack_.back() = path_state::rhs_slice_expression_step;
							state_stack_.emplace_back(path_state::number);
							++p_;
							++column_;
							break;
						default:
							ec = jsonformula_errc::expected_rbracket;
							return jsonformula_expression();
					}
					break;
				}
				case path_state::rhs_slice_expression_step:
				{
					if (!buffer.empty())
					{
						int64_t val{ 0 };
						auto r = jsoncons::detail::to_integer(buffer.data(), buffer.size(), val);
						if (!r)
						{
							ec = jsonformula_errc::invalid_number;
							return jsonformula_expression();
						}
						if (val == 0)
						{
							ec = jsonformula_errc::step_cannot_be_zero;
							return jsonformula_expression();
						}
						slic.step_ = val;
						buffer.clear();
					}
					switch(*p_)
					{
						case ']':
							push_token(token(jsoncons::make_unique<slice_projection>(slic)), ec);
							if (ec) {return jsonformula_expression();}
							buffer.clear();
							slic = slice{};
							state_stack_.pop_back(); // rhs_slice_expression_step
							++p_;
							++column_;
							break;
						default:
							ec = jsonformula_errc::expected_rbracket;
							return jsonformula_expression();
					}
					break;
				}
				case path_state::expect_rbracket:
				{
					switch(*p_)
					{
						case ']':
							state_stack_.pop_back(); // expect_rbracket
							++p_;
							++column_;
							break;
						default:
							ec = jsonformula_errc::expected_rbracket;
							return jsonformula_expression();
					}
					break;
				}
				case path_state::expect_rparen:
					switch (*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case ')':
							++p_;
							++column_;
							push_token(rparen_arg, ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::rhs_expression;
							break;
						default:
							ec = jsonformula_errc::expected_rparen;
							return jsonformula_expression();
					}
					break;
				case path_state::key_val_expr: 
				{
					switch (*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case '\"':
							state_stack_.back() = path_state::expect_colon;
							state_stack_.emplace_back(path_state::key_expr);
							state_stack_.emplace_back(path_state::quoted_string);
							++p_;
							++column_;
							break;
						case '\'':
							state_stack_.back() = path_state::expect_colon;
							state_stack_.emplace_back(path_state::key_expr);	// json-formula support these as keys too
							state_stack_.emplace_back(path_state::raw_string);
							++p_;
							++column_;
							break;
						default:
							if ((*p_ >= 'A' && *p_ <= 'Z') || (*p_ >= 'a' && *p_ <= 'z') || (*p_ == '_'))
							{
								state_stack_.back() = path_state::expect_colon;
								state_stack_.emplace_back(path_state::key_expr);
								state_stack_.emplace_back(path_state::unquoted_string);
								buffer.push_back(*p_);
								++p_;
								++column_;
							}
							else
							{
								ec = jsonformula_errc::expected_key;
								return jsonformula_expression();
							}
							break;
					};
					break;
				}
				case path_state::cmp_lt_or_lte:
				{
					// json-formula
					// check if there is anything in the buffer...
					// if so, we need to process it
					_checkBuffer(buffer);

					switch(*p_)
					{
						case '=':
							push_token(token(resources_.get_lte_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
						// json-formula supports <> for not equal
						case '>':
							push_token(token(resources_.get_ne_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
						default:
							push_token(token(resources_.get_lt_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();
							break;
					}
					break;
				}
				case path_state::cmp_gt_or_gte:
				{
					// json-formula
					// check if there is anything in the buffer...
					// if so, we need to process it
					_checkBuffer(buffer);

					switch(*p_)
					{
						case '=':
							push_token(token(resources_.get_gte_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back(); 
							++p_;
							++column_;
							break;
						default:
							push_token(token(resources_.get_gt_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back(); 
							break;
					}
					break;
				}
				case path_state::cmp_eq:
				{
					// json-formula
					// check if there is anything in the buffer...
					// if so, we need to process it
					_checkBuffer(buffer);

					// json-formula supports both `=` and `==`
					push_token(token(resources_.get_eq_operator()), ec);
					push_token(token(current_node_arg), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					if ( *p_ == '=' ) {	// be careful not to move past something else!
						++p_;
						++column_;
					}
					break;
				}
				case path_state::cmp_ne:
				{
					// json-formula
					// check if there is anything in the buffer...
					// if so, we need to process it
					_checkBuffer(buffer);

					switch(*p_)
					{
						case '=':
							push_token(token(resources_.get_ne_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back(); 
							++p_;
							++column_;
							break;
						default:
							ec = jsonformula_errc::expected_comparator;
							return jsonformula_expression();
					}
					break;
				}
				case path_state::jf_expression:
				{
					// json-formula
					// check if there is anything in the buffer...
					// if so, we need to process it
					_checkBuffer(buffer);
					
					switch(*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case '+':
							++p_;
							++column_;
							state_stack_.back() = path_state::lhs_expression;
							state_stack_.emplace_back(path_state::plus_operator);
							break;
						case '-':
							++p_;
							++column_;
							state_stack_.back() = path_state::lhs_expression;
							state_stack_.emplace_back(path_state::minus_operator);
							break;
						case '*':
							++p_;
							++column_;
							state_stack_.back() = path_state::lhs_expression;
							state_stack_.emplace_back(path_state::mult_operator);
							break;
						case '/':
							++p_;
							++column_;
							state_stack_.back() = path_state::lhs_expression;
							state_stack_.emplace_back(path_state::div_operator);
							break;
						case '~':
							++p_;
							++column_;
							state_stack_.back() = path_state::lhs_expression;
							state_stack_.emplace_back(path_state::union_operator);
							break;
						default:
							if (state_stack_.size() > 1)
							{
								state_stack_.pop_back();
							}
							else
							{
								ec = jsonformula_errc::syntax_error;
								return jsonformula_expression();
							}
							break;
					}
					break;
				}
				case path_state::plus_operator:
				{
					push_token(token(resources_.get_plus_operator()), ec);
					push_token(token(current_node_arg), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					break;
				}
				case path_state::minus_operator:
				{
					push_token(token(resources_.get_minus_operator()), ec);
					push_token(token(current_node_arg), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					break;
				}
				case path_state::mult_operator:
				{
					push_token(token(resources_.get_mult_operator()), ec);
					push_token(token(current_node_arg), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					break;
				}
				case path_state::div_operator:
				{
					push_token(token(resources_.get_div_operator()), ec);
					push_token(token(current_node_arg), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					break;
				}
				case path_state::concat_operator:
				{
					push_token(token(resources_.get_concat_operator()), ec);
					push_token(token(current_node_arg), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					break;
				}
				case path_state::union_operator:
				{
					push_token(token(resources_.get_union_operator()), ec);
					push_token(token(current_node_arg), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back();
					break;
				}
				case path_state::expect_dot:
				{
					switch(*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case '.':
							state_stack_.pop_back(); // expect_dot
							++p_;
							++column_;
							break;
						default:
							ec = jsonformula_errc::expected_dot;
							return jsonformula_expression();
					}
					break;
				}
				case path_state::expect_pipe_or_or:
				{
					switch(*p_)
					{
						case '|':
							push_token(token(resources_.get_or_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back(); 
							++p_;
							++column_;
							break;
						default:
							push_token(token(pipe_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back(); 
							break;
					}
					break;
				}
				case path_state::expect_and:
				{
					switch(*p_)
					{
						case '&':
							push_token(token(resources_.get_and_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back(); // expect_and
							++p_;
							++column_;
							break;
						default:
							// for json-formula, this is the concat operator
//                                ec = jsonformula_errc::expected_and;
//                                return jsonformula_expression();
							push_token(token(resources_.get_concat_operator()), ec);
							push_token(token(current_node_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back(); // expect_and
							++p_;
							++column_;
							break;
					}
					break;
				}
				case path_state::multi_select_list:
				{
					switch(*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case ',':
							// json-formula
							// check if there is anything in the buffer...(better be)
							_checkBuffer(buffer);

							push_token(token(separator_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.emplace_back(path_state::lhs_expression);
							++p_;
							++column_;
							break;
						case '[':
							state_stack_.emplace_back(path_state::lhs_expression);
							break;
						case '.':
							state_stack_.emplace_back(path_state::sub_expression);
							++p_;
							++column_;
							break;
						case '|':
						{
							++p_;
							++column_;
							state_stack_.emplace_back(path_state::lhs_expression);
							state_stack_.emplace_back(path_state::expect_pipe_or_or);
							break;
						}
						case ']':
						{
							// json-formula
							// check if there is anything in the buffer...
							// if so, we need to process it
							_checkBuffer(buffer);

							push_token(token(end_multi_select_list_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();

							++p_;
							++column_;
							break;
						}
						default:
							ec = jsonformula_errc::expected_rbracket;
							return jsonformula_expression();
					}
					break;
				}
				case path_state::filter:
				{
					switch(*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case ']':
						{
							push_token(token(end_filter_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.pop_back();
							++p_;
							++column_;
							break;
						}
						default:
							ec = jsonformula_errc::expected_rbracket;
							return jsonformula_expression();
					}
					break;
				}
				case path_state::expect_rbrace:
				{
					// json-formula
					// check if there is anything in the buffer...
					// if so, we need to process it
					_checkBuffer(buffer);

					switch(*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case ',':
							push_token(token(separator_arg), ec);
							if (ec) {return jsonformula_expression();}
							state_stack_.back() = path_state::key_val_expr; 
							++p_;
							++column_;
							break;
						case '[':
						case '{':
							state_stack_.emplace_back(path_state::lhs_expression);
							break;
						case '.':
							state_stack_.emplace_back(path_state::sub_expression);
							++p_;
							++column_;
							break;
						case '}':
						{
							state_stack_.pop_back();
							push_token(end_multi_select_hash_arg, ec);
							if (ec) {return jsonformula_expression();}
							++p_;
							++column_;
							break;
						}
						// json-formula supports functions
						case '+':
						case '-':
						case '*':
						case '/':
							state_stack_.emplace_back(path_state::jf_expression);
							break;
						default:
							if ( debug_ ) { std::cout << "rbrace: " << *p_ << std::endl; }
							
							ec = jsonformula_errc::expected_rbrace;
							return jsonformula_expression();
					}
					break;
				}
				case path_state::expect_colon:
				{
					switch(*p_)
					{
						case ' ':case '\t':case '\r':case '\n':
							advance_past_space_character(ec, buffer);
							break;
						case ':':
							state_stack_.back() = path_state::expect_rbrace;
							state_stack_.emplace_back(path_state::lhs_expression);
							++p_;
							++column_;
							break;
						default:
							ec = jsonformula_errc::expected_colon;
							return jsonformula_expression();
					}
					break;
				}
			}
			
		}

		if (state_stack_.empty())
		{
			ec = jsonformula_errc::syntax_error;
			return jsonformula_expression();
		}
		while (state_stack_.size() > 1)
		{
			switch (state_stack_.back())
			{
				case path_state::rhs_expression:
					if (state_stack_.size() > 1)
					{
						state_stack_.pop_back();
					}
					else
					{
						ec = jsonformula_errc::syntax_error;
						return jsonformula_expression();
					}
					break;
				case path_state::number:	// in json-formula, we could end with a number
				case path_state::digit:	// in json-formula, we could end with a digit
					{
						jsoncons::json_decoder<Json> decoder;
						jsoncons::basic_json_reader<char_type,jsoncons::string_source<char_type>> reader(buffer, decoder);
						std::error_code parse_ec;
						reader.read(parse_ec);
						if (parse_ec)
						{
							ec = jsonformula_errc::invalid_literal;
							return jsonformula_expression();
						}
						auto j = decoder.get_result();
						push_token(token(literal_arg, std::move(j)), ec);
						if (ec) {return jsonformula_expression();}
						buffer.clear();
						state_stack_.pop_back();
					}
					break;
				case path_state::val_expr:
					push_token(token(jsoncons::make_unique<identifier_selector>(buffer)), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back(); 
					break;
				case path_state::identifier_or_function_expr:
					push_token(token(jsoncons::make_unique<identifier_selector>(buffer)), ec);
					if (ec) {return jsonformula_expression();}
					state_stack_.pop_back(); 
					break;
				case path_state::quoted_string: 	// specific error for unbalanced quotes in json-formula
				case path_state::raw_string:
					ec = jsonformula_errc::unbalanced_quotes;
					return jsonformula_expression();
					break;
				case path_state::unquoted_string:
					state_stack_.pop_back();
					break;
				default:
					ec = jsonformula_errc::syntax_error;
					return jsonformula_expression();
					break;
			}
		}

		if (!(state_stack_.size() == 1 && state_stack_.back() == path_state::rhs_expression))
		{
			ec = jsonformula_errc::unexpected_end_of_input;
			return jsonformula_expression();
		}

		state_stack_.pop_back();

		push_token(end_of_expression_arg, ec);
		if (ec) {return jsonformula_expression();}

		// debug the output stack...
		if ( debug_ ) {
			for (auto& t : output_stack_)
			{
				std::cout << t.to_string() << std::endl;
			}
		}
		
		return jsonformula_expression(std::move(resources_), std::move(output_stack_));
	}

	void advance_past_space_character(std::error_code& ec, string_type& buffer)
	{
		// json-formula
		// check if there is anything in the buffer...
		// if so, we need to process it
		if (!buffer.empty()) {
			jsoncons::json_decoder<Json> decoder;
			jsoncons::basic_json_reader<char_type,jsoncons::string_source<char_type>> reader(buffer, decoder);
			std::error_code parse_ec;
			reader.read(parse_ec);
			if (parse_ec) {
				// we had some problem with the literal, so let's force it to number & back
				auto bufD = std::stod(buffer);
				buffer = std::to_string(bufD);
				return;	// will cause it to run again with the new buffer...
			}
			auto j = decoder.get_result();
			push_token(token(literal_arg, std::move(j)), ec);
			buffer.clear();
		}

		switch (*p_)
		{
			case ' ':case '\t':
				++p_;
				++column_;
				break;
			case '\r':
				if (p_+1 >= end_input_)
				{
					ec = jsonformula_errc::unexpected_end_of_input;
					return;
				}
				if (*(p_+1) == '\n')
					++p_;
				++line_;
				column_ = 1;
				++p_;
				break;
			case '\n':
				++line_;
				column_ = 1;
				++p_;
				break;
			default:
				break;
		}
	}

	void unwind_rparen(std::error_code& ec)
	{
		auto it = operator_stack_.rbegin();
		while (it != operator_stack_.rend() && !it->is_lparen())
		{
			output_stack_.emplace_back(std::move(*it));
			++it;
		}
		if (it == operator_stack_.rend())
		{
			ec = jsonformula_errc::unbalanced_parentheses;
			return;
		}
		++it;
		operator_stack_.erase(it.base(),operator_stack_.end());
	}

	void push_token(token&& tok, std::error_code& ec)
	{
		// debug the output stack...
		if ( debug_ ) {
			std::cout << "pre-push" << std::endl;
			for (auto& t : output_stack_)
			{
				std::cout << t.to_string() << std::endl;
			}
		}

		switch (tok.type())
		{
			case token_kind::end_filter:
			{
				unwind_rparen(ec);
				std::vector<token> toks;
				auto it = output_stack_.rbegin();
				while (it != output_stack_.rend() && it->type() != token_kind::begin_filter)
				{
					toks.emplace_back(std::move(*it));
					++it;
				}
				if (it == output_stack_.rend())
				{
					ec = jsonformula_errc::unbalanced_braces;
					return;
				}
				if (toks.back().type() != token_kind::literal)
				{
					toks.emplace_back(current_node_arg);
				}
				std::reverse(toks.begin(), toks.end());
				++it;
				output_stack_.erase(it.base(),output_stack_.end());

				if (!output_stack_.empty() && output_stack_.back().is_projection() && 
					(tok.precedence_level() < output_stack_.back().precedence_level() ||
					(tok.precedence_level() == output_stack_.back().precedence_level() && tok.is_right_associative())))
				{
					output_stack_.back().expression_->add_expression(jsoncons::make_unique<filter_expression>(std::move(toks)));
				}
				else
				{
					output_stack_.emplace_back(token(jsoncons::make_unique<filter_expression>(std::move(toks))));
				}
				break;
			}
			case token_kind::end_multi_select_list:
			{
				unwind_rparen(ec);
				std::vector<std::vector<token>> vals;
				auto it = output_stack_.rbegin();
				while (it != output_stack_.rend() && it->type() != token_kind::begin_multi_select_list)
				{
					std::vector<token> toks;
					do
					{
						toks.emplace_back(std::move(*it));
						++it;
					} while (it != output_stack_.rend() && it->type() != token_kind::begin_multi_select_list && it->type() != token_kind::separator);
					if (it->type() == token_kind::separator)
					{
						++it;
					}
					if (toks.back().type() != token_kind::literal)
					{
						toks.emplace_back(current_node_arg);
					}
					std::reverse(toks.begin(), toks.end());
					vals.emplace_back(std::move(toks));
				}
				if (it == output_stack_.rend())
				{
					ec = jsonformula_errc::unbalanced_braces;
					return;
				}
				++it;
				output_stack_.erase(it.base(),output_stack_.end());
				std::reverse(vals.begin(), vals.end());
				if (!output_stack_.empty() && output_stack_.back().is_projection() && 
					(tok.precedence_level() < output_stack_.back().precedence_level() ||
					(tok.precedence_level() == output_stack_.back().precedence_level() && tok.is_right_associative())))
				{
					output_stack_.back().expression_->add_expression(jsoncons::make_unique<multi_select_list>(std::move(vals)));
				}
				else
				{
					output_stack_.emplace_back(token(jsoncons::make_unique<multi_select_list>(std::move(vals))));
				}
				break;
			}
			case token_kind::end_multi_select_hash:
			{
				unwind_rparen(ec);
				std::vector<key_tokens> key_toks;
				auto it = output_stack_.rbegin();
				while (it != output_stack_.rend() && it->type() != token_kind::begin_multi_select_hash)
				{
					std::vector<token> toks;
					do
					{
						toks.emplace_back(std::move(*it));
						++it;
					} while (it != output_stack_.rend() && it->type() != token_kind::key);
					JSONCONS_ASSERT(it->is_key());
					auto key = std::move(it->key_);
					++it;
					if (it->type() == token_kind::separator)
					{
						++it;
					}
					if (toks.back().type() != token_kind::literal)
					{
						toks.emplace_back(current_node_arg);
					}
					std::reverse(toks.begin(), toks.end());
					key_toks.emplace_back(std::move(key), std::move(toks));
				}
				if (it == output_stack_.rend())
				{
					ec = jsonformula_errc::unbalanced_braces;
					return;
				}
				std::reverse(key_toks.begin(), key_toks.end());
				++it;
				output_stack_.erase(it.base(),output_stack_.end());

				if (!output_stack_.empty() && output_stack_.back().is_projection() && 
					(tok.precedence_level() < output_stack_.back().precedence_level() ||
					(tok.precedence_level() == output_stack_.back().precedence_level() && tok.is_right_associative())))
				{
					output_stack_.back().expression_->add_expression(jsoncons::make_unique<multi_select_hash>(std::move(key_toks)));
				}
				else
				{
					output_stack_.emplace_back(token(jsoncons::make_unique<multi_select_hash>(std::move(key_toks))));
				}
				break;
			}
			case token_kind::end_expression_type:
			{
				std::vector<token> toks;
				auto it = output_stack_.rbegin();
				while (it != output_stack_.rend() && it->type() != token_kind::begin_expression_type)
				{
					toks.emplace_back(std::move(*it));
					++it;
				}
				if (it == output_stack_.rend())
				{
					JSONCONS_THROW(jsoncons::json_runtime_error<std::runtime_error>("Unbalanced braces"));
				}
				if (toks.back().type() != token_kind::literal)
				{
					toks.emplace_back(current_node_arg);
				}
				std::reverse(toks.begin(), toks.end());
				output_stack_.erase(it.base(),output_stack_.end());
				output_stack_.emplace_back(token(jsoncons::make_unique<function_expression>(std::move(toks))));
				break;
			}
			case token_kind::literal:
				if (!output_stack_.empty() && output_stack_.back().type() == token_kind::current_node)
				{
					output_stack_.back() = std::move(tok);
				}
				else
				{
					output_stack_.emplace_back(std::move(tok));
				}
				break;
			case token_kind::expression:
				if (!output_stack_.empty() && output_stack_.back().is_projection() && 
					(tok.precedence_level() < output_stack_.back().precedence_level() ||
					(tok.precedence_level() == output_stack_.back().precedence_level() && tok.is_right_associative())))
				{
					output_stack_.back().expression_->add_expression(std::move(tok.expression_));
				}
				else
				{
					output_stack_.emplace_back(std::move(tok));
				}
				break;
			case token_kind::rparen:
				{
					unwind_rparen(ec);
					break;
				}
			case token_kind::end_function:
				{
					unwind_rparen(ec);
					std::vector<token> toks;
					auto it = output_stack_.rbegin(), prev=output_stack_.rbegin();
					std::size_t arg_count = 0, tok_count=0;
					while (it != output_stack_.rend() && it->type() != token_kind::function)
					{
						if (it->type() == token_kind::argument)
						{
							++arg_count;
						}
													
						toks.emplace_back(std::move(*it));
						tok_count++;

						// json formula supports operators as params
						// swap it with the argument!
						if (it->type() == token_kind::argument &&
							prev->type() == token_kind::binary_operator) {
							std::swap(toks[tok_count-1], toks[tok_count-2]);
						}

						prev=it;
						++it;
					}
					
					// debug the toks...
					if ( debug_ ) {
						std::cout << "toks-1" << std::endl;
						for (auto& t : toks)
						{
							std::cout << t.to_string() << std::endl;
						}
					}
					
					if (it == output_stack_.rend())
					{
						ec = jsonformula_errc::unbalanced_parentheses;
						return;
					}
					if (it->arity() && arg_count != *(it->arity()))
					{
						ec = jsonformula_errc::invalid_arity;
						return;
					}
					if (toks.back().type() != token_kind::literal)
					{
						toks.emplace_back(current_node_arg);
					}
					std::reverse(toks.begin(), toks.end());
					toks.push_back(std::move(*it));
					++it;
					output_stack_.erase(it.base(),output_stack_.end());

					// debug the toks...
					if ( debug_ ) {
						std::cout << "toks-2" << std::endl;
						for (auto& t : toks)
						{
							std::cout << t.to_string() << std::endl;
						}
					}
					
					if (!output_stack_.empty() && output_stack_.back().is_projection() &&
						(tok.precedence_level() < output_stack_.back().precedence_level() ||
						(tok.precedence_level() == output_stack_.back().precedence_level() && tok.is_right_associative())))
					{
						output_stack_.back().expression_->add_expression(jsoncons::make_unique<function_expression>(std::move(toks)));
					}
					else
					{
						output_stack_.emplace_back(token(jsoncons::make_unique<function_expression>(std::move(toks))));
					}
					break;
				}
			case token_kind::end_of_expression:
				{
					auto it = operator_stack_.rbegin();
					while (it != operator_stack_.rend())
					{
						output_stack_.emplace_back(std::move(*it));
						++it;
					}
					operator_stack_.clear();
					break;
				}
			case token_kind::unary_operator:
			case token_kind::binary_operator:
			{
				if (operator_stack_.empty() || operator_stack_.back().is_lparen())
				{
					operator_stack_.emplace_back(std::move(tok));
				}
				else if (tok.precedence_level() < operator_stack_.back().precedence_level()
							|| (tok.precedence_level() == operator_stack_.back().precedence_level() && tok.is_right_associative()))
				{
					operator_stack_.emplace_back(std::move(tok));
				}
				else
				{
					auto it = operator_stack_.rbegin();
					while (it != operator_stack_.rend() && it->is_operator()
							&& (tok.precedence_level() > it->precedence_level()
							|| (tok.precedence_level() == it->precedence_level() && tok.is_right_associative())))
					{
						output_stack_.emplace_back(std::move(*it));
						++it;
					}

					operator_stack_.erase(it.base(),operator_stack_.end());
					operator_stack_.emplace_back(std::move(tok));
				}
				break;
			}
			case token_kind::separator:
			{
				unwind_rparen(ec);
				output_stack_.emplace_back(std::move(tok));
				operator_stack_.emplace_back(token(lparen_arg));
				break;
			}
			case token_kind::begin_filter:
				output_stack_.emplace_back(std::move(tok));
				operator_stack_.emplace_back(token(lparen_arg));
				break;
			case token_kind::begin_multi_select_list:
				output_stack_.emplace_back(std::move(tok));
				operator_stack_.emplace_back(token(lparen_arg));
				break;
			case token_kind::begin_multi_select_hash:
				output_stack_.emplace_back(std::move(tok));
				operator_stack_.emplace_back(token(lparen_arg));
				break;
			case token_kind::function:
				output_stack_.emplace_back(std::move(tok));
				operator_stack_.emplace_back(token(lparen_arg));
				break;
			case token_kind::current_node:
				output_stack_.emplace_back(std::move(tok));
				break;
			case token_kind::key:
			case token_kind::pipe:
			case token_kind::argument:
			case token_kind::begin_expression_type:
				output_stack_.emplace_back(std::move(tok));
				break;
			case token_kind::lparen:
				operator_stack_.emplace_back(std::move(tok));
				break;
			default:
				break;
		}

		// debug the output stack...
		if ( debug_ ) {
			std::cout << "post-push" << std::endl;
			for (auto& t : output_stack_)
			{
				std::cout << t.to_string() << std::endl;
			}
		}
	}

	uint32_t append_to_codepoint(uint32_t cp, int c, std::error_code& ec)
	{
		cp *= 16;
		if (c >= '0'  &&  c <= '9')
		{
			cp += c - '0';
		}
		else if (c >= 'a'  &&  c <= 'f')
		{
			cp += c - 'a' + 10;
		}
		else if (c >= 'A'  &&  c <= 'F')
		{
			cp += c - 'A' + 10;
		}
		else
		{
			ec = jsonformula_errc::invalid_codepoint;
		}
		return cp;
	}
};
