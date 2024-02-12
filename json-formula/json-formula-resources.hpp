// dynamic_resources

#define JSONFORMULA_STRING_CONSTANT(CharT, Str) jsoncons::string_constant_of_type<CharT>(Str, JSONCONS_WIDEN(Str))

template<class Json, class JsonReference>
class dynamic_resources
{
	typedef typename Json::char_type char_type;
	typedef typename Json::char_traits_type char_traits_type;
	typedef std::basic_string<char_type,char_traits_type> string_type;
	typedef typename Json::string_view_type string_view_type;
	typedef JsonReference reference;
	using pointer = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
	typedef typename Json::const_pointer const_pointer;

	std::vector<std::unique_ptr<Json>> temp_storage_;

public:
	~dynamic_resources()
	{
	}

	reference number_type_name() 
	{
		static Json number_type_name(JSONFORMULA_STRING_CONSTANT(char_type, "number"));

		return number_type_name;
	}

	reference boolean_type_name()
	{
		static Json boolean_type_name(JSONFORMULA_STRING_CONSTANT(char_type, "boolean"));

		return boolean_type_name;
	}

	reference string_type_name()
	{
		static Json string_type_name(JSONFORMULA_STRING_CONSTANT(char_type, "string"));

		return string_type_name;
	}

	reference object_type_name()
	{
		static Json object_type_name(JSONFORMULA_STRING_CONSTANT(char_type, "object"));

		return object_type_name;
	}

	reference array_type_name()
	{
		static Json array_type_name(JSONFORMULA_STRING_CONSTANT(char_type, "array"));

		return array_type_name;
	}

	reference null_type_name()
	{
		static Json null_type_name(JSONFORMULA_STRING_CONSTANT(char_type, "null"));

		return null_type_name;
	}

	reference true_value() const
	{
		static const Json true_value(true, jsoncons::semantic_tag::none);
		return true_value;
	}

	reference false_value() const
	{
		static const Json false_value(false, jsoncons::semantic_tag::none);
		return false_value;
	}

	reference null_value() const
	{
		static const Json null_value(jsoncons::null_type(), jsoncons::semantic_tag::none);
		return null_value;
	}

	template <typename... Args>
	Json* create_json(Args&& ... args)
	{
		auto temp = jsoncons::make_unique<Json>(std::forward<Args>(args)...);
		Json* ptr = temp.get();
		temp_storage_.emplace_back(std::move(temp));
		return ptr;
	}
};

