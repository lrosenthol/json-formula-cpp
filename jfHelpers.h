static std::string GetType(const jsoncons::json& j)
{
	if ( j.is_double() )
		return "double";
	else if ( j.is_number() )
		return "number";
	else if ( j.is_bool() )
		return "bool";
	else if ( j.is_string() )
		return "string";
	else if ( j.is_array() )
		return "array";
	else if ( j.is_object() )
		return "object";
	else if ( j.is_null() )
		return "null";
	else
		return "other";
}

static jsoncons::json eval(const jsoncons::json& j, const std::string& e, bool print=false)
{
	try {
		jsoncons::json r = jsonformula::search(j,e);
		if ( print )
			std::cout << e << " = " << jsoncons::print(r) << " (" << GetType(r) << ")" << std::endl;
		
		return r;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	
	return jsoncons::json();
}

static jsoncons::json eval_debug(const jsoncons::json& j, const std::string& e)
{
	std::error_code ec;
	
	try {
		jsoncons::json r = jsonformula::debug_search(j,e,ec);
		if (ec) {
			std::cout << "Error #" << ec.value() << ": " << ec.message() << std::endl;
		} else {	// it's debug, we always print!
			std::cout << e << " = " << jsoncons::print(r) << " (" << GetType(r) << ")" << std::endl;
		}
		return r;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	
	return jsoncons::json();
}


static std::string evalToString(const jsoncons::json& j, const std::string& e)
{
	jsoncons::json r = jsonformula::search(j,e);
	return static_cast<std::string>(r.as<std::string_view>());
}

static jsoncons::json evalJP(const jsoncons::json& j, const std::string& e, bool print=false)
{
	jsoncons::json r = jsoncons::jmespath::search(j,e);
	if ( print )
		std::cout << e << " = " << jsoncons::print(r) << " (" << GetType(r) << ")" << std::endl;
	
	return r;
}
