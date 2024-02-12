// function_base
class function_base
{
	jsoncons::optional<std::size_t> arg_count_;
public:
	function_base(jsoncons::optional<std::size_t> arg_count)
		: arg_count_(arg_count)
	{
	}

	jsoncons::optional<std::size_t> arity() const
	{
		return arg_count_;
	}

	virtual ~function_base() = default;

	virtual reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>&, std::error_code& ec) const = 0;

	virtual std::string to_string(std::size_t = 0) const
	{
		return std::string("to_string not implemented");
	}
};  

class abs_function : public function_base
{
public:
	abs_function()
		: function_base(1)
	{
	}

	virtual std::string to_string(std::size_t = 0) const override
	{
		return std::string("abs() function");
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		switch (arg0.type())
		{
			case jsoncons::json_type::uint64_value:
				return arg0;
			case jsoncons::json_type::int64_value:
			{
				return arg0.template as<int64_t>() >= 0 ? arg0 : *resources.create_json(std::abs(arg0.template as<int64_t>()));
			}
			case jsoncons::json_type::double_value:
			{
				return arg0.template as<double>() >= 0 ? arg0 : *resources.create_json(std::abs(arg0.template as<double>()));
			}
			default:
			{
//						auto at = arg0.type();
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
		}
	}
};

class avg_function : public function_base
{
public:
	avg_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (arg0.empty())
		{
			return resources.null_value();
		}

		double sum = 0;
		for (auto& j : arg0.array_range())
		{
			if (!j.is_number())
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			sum += j.template as<double>();
		}

		return sum == 0 ? resources.null_value() : *resources.create_json(sum/arg0.size());
	}
};

class ceil_function : public function_base
{
public:
	ceil_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		switch (arg0.type())
		{
			case jsoncons::json_type::uint64_value:
			case jsoncons::json_type::int64_value:
			{
				return *resources.create_json(arg0.template as<double>());
			}
			case jsoncons::json_type::double_value:
			{
				return *resources.create_json(std::ceil(arg0.template as<double>()));
			}
			default:
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
		}
	}
};

class contains_function : public function_base
{
public:
	contains_function()
		: function_base(2)
	{
	}
	
	virtual std::string to_string(std::size_t = 0) const override
	{
		return std::string("contains() function");
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!(args[0].is_value() && args[1].is_value()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}


		reference arg0 = args[0].value();
		reference arg1 = args[1].value();

		switch (arg0.type())
		{
			case jsoncons::json_type::array_value:
				for (auto& j : arg0.array_range())
				{
					if (j == arg1)
					{
						return resources.true_value();
					}
				}
				return resources.false_value();
			case jsoncons::json_type::string_value:
			{
				if (arg1.is_number()) {
					auto sv0 = arg0.template as<string_view_type>();
					auto sv1 = std::to_string(arg1.template as<double>());
					return sv0.find(sv1) != string_view_type::npos ? resources.true_value() : resources.false_value();
				} else if (!arg1.is_string())  {
					ec = jsonformula_errc::invalid_type;
					return resources.null_value();
				} else {
					auto sv0 = arg0.template as<string_view_type>();
					auto sv1 = arg1.template as<string_view_type>();
					return sv0.find(sv1) != string_view_type::npos ? resources.true_value() : resources.false_value();
				}
			}
			default:
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
		}
	}
};

class ends_with_function : public function_base
{
public:
	ends_with_function()
		: function_base(2)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!(args[0].is_value() && args[1].is_value()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_string())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg1 = args[1].value();
		if (!arg1.is_string())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		auto sv0 = arg0.template as<string_view_type>();
		auto sv1 = arg1.template as<string_view_type>();

		if (sv1.length() <= sv0.length() && sv1 == sv0.substr(sv0.length() - sv1.length()))
		{
			return resources.true_value();
		}
		else
		{
			return resources.false_value();
		}
	}
};

class floor_function : public function_base
{
public:
	floor_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		switch (arg0.type())
		{
			case jsoncons::json_type::uint64_value:
			case jsoncons::json_type::int64_value:
			{
				return *resources.create_json(arg0.template as<double>());
			}
			case jsoncons::json_type::double_value:
			{
				return *resources.create_json(std::floor(arg0.template as<double>()));
			}
			default:
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
		}
	}
};

class join_function : public function_base
{
public:
	join_function()
		: function_base(2)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		reference arg0 = args[0].value();
		reference arg1 = args[1].value();

		if (!(args[0].is_value() && args[1].is_value()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		if (!arg0.is_string())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (!arg1.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		string_type sep = arg0.template as<string_type>();
		string_type buf;
		for (auto& j : arg1.array_range())
		{
			if (!j.is_string())
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			if (!buf.empty())
			{
				buf.append(sep);
			}
			auto sv = j.template as<string_view_type>();
			buf.append(sv.begin(), sv.end());
		}
		return *resources.create_json(buf);
	}
};

class length_function : public function_base
{
public:
	length_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();

		switch (arg0.type())
		{
			case jsoncons::json_type::object_value:
			case jsoncons::json_type::array_value:
				return *resources.create_json(arg0.size());
			case jsoncons::json_type::string_value:
			{
				auto sv0 = arg0.template as<string_view_type>();
				auto length = jsoncons::unicode_traits::count_codepoints(sv0.data(), sv0.size());
				return *resources.create_json(length);
			}
			default:
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
		}
	}
};

class max_function : public function_base
{
public:
	max_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (arg0.empty())
		{
			return resources.null_value();
		}

		bool is_number = arg0.at(0).is_number();
		bool is_string = arg0.at(0).is_string();
		if (!is_number && !is_string)
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		std::size_t index = 0;
		for (std::size_t i = 1; i < arg0.size(); ++i)
		{
			if (!(arg0.at(i).is_number() == is_number && arg0.at(i).is_string() == is_string))
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			if (arg0.at(i) > arg0.at(index))
			{
				index = i;
			}
		}

		return arg0.at(index);
	}
};

class max_by_function : public function_base
{
public:
	max_by_function()
		: function_base(2)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!(args[0].is_value() && args[1].is_expression()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (arg0.empty())
		{
			return resources.null_value();
		}

		const auto& expr = args[1].expression();

		std::error_code ec2;
		Json key1 = expr.evaluate(arg0.at(0), resources, ec2); 

		bool is_number = key1.is_number();
		bool is_string = key1.is_string();
		if (!(is_number || is_string))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		std::size_t index = 0;
		for (std::size_t i = 1; i < arg0.size(); ++i)
		{
			reference key2 = expr.evaluate(arg0.at(i), resources, ec2); 
			if (!(key2.is_number() == is_number && key2.is_string() == is_string))
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			if (key2 > key1)
			{
				key1 = key2;
				index = i;
			}
		}

		return arg0.at(index);
	}
};

class map_function : public function_base
{
public:
	map_function()
		: function_base(2)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!(args[0].is_expression() && args[1].is_value()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		const auto& expr = args[0].expression();

		reference arg0 = args[1].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		auto result = resources.create_json(jsoncons::json_array_arg);

		for (auto& item : arg0.array_range())
		{
			auto& j = expr.evaluate(item, resources, ec);
			if (ec)
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			result->emplace_back(jsoncons::json_const_pointer_arg, std::addressof(j));
		}

		return *result;
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("map_function\n");
	}
};

class min_function : public function_base
{
public:
	min_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (arg0.empty())
		{
			return resources.null_value();
		}

		bool is_number = arg0.at(0).is_number();
		bool is_string = arg0.at(0).is_string();
		if (!is_number && !is_string)
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		std::size_t index = 0;
		for (std::size_t i = 1; i < arg0.size(); ++i)
		{
			if (!(arg0.at(i).is_number() == is_number && arg0.at(i).is_string() == is_string))
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			if (arg0.at(i) < arg0.at(index))
			{
				index = i;
			}
		}

		return arg0.at(index);
	}
};

class min_by_function : public function_base
{
public:
	min_by_function()
		: function_base(2)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!(args[0].is_value() && args[1].is_expression()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (arg0.empty())
		{
			return resources.null_value();
		}

		const auto& expr = args[1].expression();

		std::error_code ec2;
		Json key1 = expr.evaluate(arg0.at(0), resources, ec2); 

		bool is_number = key1.is_number();
		bool is_string = key1.is_string();
		if (!(is_number || is_string))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		std::size_t index = 0;
		for (std::size_t i = 1; i < arg0.size(); ++i)
		{
			reference key2 = expr.evaluate(arg0.at(i), resources, ec2); 
			if (!(key2.is_number() == is_number && key2.is_string() == is_string))
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			if (key2 < key1)
			{
				key1 = key2;
				index = i;
			}
		}

		return arg0.at(index);
	}
};

class merge_function : public function_base
{
public:
	merge_function()
		: function_base(jsoncons::optional<std::size_t>())
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		if (args.empty())
		{
			ec = jsonformula_errc::invalid_arity;
			return resources.null_value();
		}

		for (auto& param : args)
		{
			if (!param.is_value())
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
		}

		reference arg0 = args[0].value();
		if (!arg0.is_object())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (args.size() == 1)
		{
			return arg0;
		}

		auto result = resources.create_json(arg0);
		for (std::size_t i = 1; i < args.size(); ++i)
		{
			reference argi = args[i].value();
			if (!argi.is_object())
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			for (auto& item : argi.object_range())
			{
				result->insert_or_assign(item.key(),item.value());
			}
		}

		return *result;
	}
};

class type_function : public function_base
{
public:
	type_function()
		: function_base(1)
	{
	}

	virtual std::string to_string(std::size_t = 0) const override
	{
		return std::string("type() function");
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();

		switch (arg0.type())
		{
			case jsoncons::json_type::int64_value:
			case jsoncons::json_type::uint64_value:
			case jsoncons::json_type::double_value:
				return resources.number_type_name();
			case jsoncons::json_type::bool_value:
				return resources.boolean_type_name();
			case jsoncons::json_type::string_value:
				return resources.string_type_name();
			case jsoncons::json_type::object_value:
				return resources.object_type_name();
			case jsoncons::json_type::array_value:
				return resources.array_type_name();
			default:
				return resources.null_type_name();
				break;

		}
	}
};

class sort_function : public function_base
{
public:
	sort_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (arg0.size() <= 1)
		{
			return arg0;
		}

		bool is_number = arg0.at(0).is_number();
		bool is_string = arg0.at(0).is_string();
		if (!is_number && !is_string)
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		for (std::size_t i = 1; i < arg0.size(); ++i)
		{
			if (arg0.at(i).is_number() != is_number || arg0.at(i).is_string() != is_string)
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
		}

		auto v = resources.create_json(arg0);
		std::stable_sort((v->array_range()).begin(), (v->array_range()).end());
		return *v;
	}
};

class sort_by_function : public function_base
{
public:
	sort_by_function()
		: function_base(2)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!(args[0].is_value() && args[1].is_expression()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		if (arg0.size() <= 1)
		{
			return arg0;
		}

		const auto& expr = args[1].expression();

		auto v = resources.create_json(arg0);
		std::stable_sort((v->array_range()).begin(), (v->array_range()).end(),
			[&expr,&resources,&ec](reference lhs, reference rhs) -> bool
		{
			std::error_code ec2;
			reference key1 = expr.evaluate(lhs, resources, ec2);
			bool is_number = key1.is_number();
			bool is_string = key1.is_string();
			if (!(is_number || is_string))
			{
				ec = jsonformula_errc::invalid_type;
			}

			reference key2 = expr.evaluate(rhs, resources, ec2);
			if (!(key2.is_number() == is_number && key2.is_string() == is_string))
			{
				ec = jsonformula_errc::invalid_type;
			}
			
			return key1 < key2;
		});
		return ec ? resources.null_value() : *v;
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("sort_by_function\n");
	}
};

class keys_function final : public function_base
{
public:
	keys_function()
		: function_base(1)
	{
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("keys_function\n");
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_object())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		auto result = resources.create_json(jsoncons::json_array_arg);
		result->reserve(args.size());

		for (auto& item : arg0.object_range())
		{
			result->emplace_back(item.key());
		}
		return *result;
	}
};

class values_function final : public function_base
{
public:
	values_function()
		: function_base(1)
	{
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("values_function\n");
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_object())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		auto result = resources.create_json(jsoncons::json_array_arg);
		result->reserve(args.size());

		for (auto& item : arg0.object_range())
		{
			result->emplace_back(item.value());
		}
		return *result;
	}
};

class reverse_function final : public function_base
{
public:
	reverse_function()
		: function_base(1)
	{
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("reverse_function\n");
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		switch (arg0.type())
		{
			case jsoncons::json_type::string_value:
			{
				string_view_type sv = arg0.as_string_view();
				std::basic_string<char32_t> buf;
				jsoncons::unicode_traits::convert(sv.data(), sv.size(), buf);
				std::reverse(buf.begin(), buf.end());
				string_type s;
				jsoncons::unicode_traits::convert(buf.data(), buf.size(), s);
				return *resources.create_json(s);
			}
			case jsoncons::json_type::array_value:
			{
				auto result = resources.create_json(arg0);
				std::reverse(result->array_range().begin(),result->array_range().end());
				return *result;
			}
			default:
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
		}
	}
};

class starts_with_function : public function_base
{
public:
	starts_with_function()
		: function_base(2)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!(args[0].is_value() && args[1].is_value()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_string())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg1 = args[1].value();
		if (!arg1.is_string())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		auto sv0 = arg0.template as<string_view_type>();
		auto sv1 = arg1.template as<string_view_type>();

		if (sv1.length() <= sv0.length() && sv1 == sv0.substr(0, sv1.length()))
		{
			return resources.true_value();
		}
		else
		{
			return resources.false_value();
		}
	}
};

class sum_function : public function_base
{
public:
	sum_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (!arg0.is_array())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}
		double sum = 0;
		for (auto& j : arg0.array_range())
		{
			if (!j.is_number())
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
			sum += j.template as<double>();
		}

		return *resources.create_json(sum);
	}
};

class to_array_function final : public function_base
{
public:
	to_array_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		if (arg0.is_array())
		{
			return arg0;
		}
		else
		{
			auto result = resources.create_json(jsoncons::json_array_arg);
			result->push_back(arg0);
			return *result;
		}
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("to_array_function\n");
	}
};

class to_number_function final : public function_base
{
public:
	to_number_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		switch (arg0.type())
		{
			case jsoncons::json_type::int64_value:
			case jsoncons::json_type::uint64_value:
			case jsoncons::json_type::double_value:
				return arg0;
			case jsoncons::json_type::string_value:
			{
				auto sv = arg0.as_string_view();
				uint64_t uval{ 0 };
				auto result1 = jsoncons::detail::to_integer(sv.data(), sv.length(), uval);
				if (result1)
				{
					return *resources.create_json(uval);
				}
				int64_t sval{ 0 };
				auto result2 = jsoncons::detail::to_integer(sv.data(), sv.length(), sval);
				if (result2)
				{
					return *resources.create_json(sval);
				}
				jsoncons::detail::chars_to to_double;
				try
				{
					auto s = arg0.as_string();
					double d = to_double(s.c_str(), s.length());
					return *resources.create_json(d);
				}
				catch (const std::exception&)
				{
					return resources.null_value();
				}
			}
			case jsoncons::json_type::null_value:	// json-formula addition
				return *resources.create_json(0);

			default:
				return resources.null_value();
		}
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("to_number_function\n");
	}
};

class to_string_function final : public function_base
{
public:
	to_string_function()
		: function_base(1)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		if (!args[0].is_value())
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		return *resources.create_json(arg0.template as<string_type>());
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("to_string() function");
	}
};

class not_null_function final : public function_base
{
public:
	not_null_function()
		: function_base(jsoncons::optional<std::size_t>())
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code&) const override
	{
		for (auto& param : args)
		{
			if (param.is_value() && !param.value().is_null())
			{
				return param.value();
			}
		}
		return resources.null_value();
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("not_null_function");
	}
};

class value_function final : public function_base
{
public:
	value_function()
		: function_base(2)
	{
	}

	reference evaluate(std::vector<parameter>& args, dynamic_resources<Json,JsonReference>& resources, std::error_code& ec) const override
	{
		JSONCONS_ASSERT(args.size() == *this->arity());

		// we need two params
		if (!(args[0].is_value() && args[1].is_value()))
		{
			ec = jsonformula_errc::invalid_type;
			return resources.null_value();
		}

		reference arg0 = args[0].value();
		reference arg1 = args[1].value();

		switch (arg0.type())
		{
			case jsoncons::json_type::array_value:
			{
				int64_t idx = -1;	// start with a bogus value
				auto a1t = arg1.type();
				if ( a1t == jsoncons::json_type::int64_value ||
						a1t == jsoncons::json_type::uint64_value ||
						a1t == jsoncons::json_type::double_value ) {
					idx = arg1.template as<int64_t>();
				} else if ( a1t == jsoncons::json_type::string_value ) {
					auto sv = arg1.as_string_view();
					jsoncons::detail::to_integer(sv.data(), sv.length(), idx);
				}
				if ( idx >=0 )
					return arg0[idx];
				else
					return resources.null_value();
			}
			case jsoncons::json_type::object_value:
			{
//						auto a1t = arg1.type();
				if ( arg1.is_string() ) {
					return arg0[arg1.as_string_view()];
				} else
					return resources.null_value();
			}
			default:
			{
				ec = jsonformula_errc::invalid_type;
				return resources.null_value();
			}
		}

		// return empty/null
		return resources.null_value();
	}

	std::string to_string(std::size_t = 0) const override
	{
		return std::string("value_function");
	}
};

