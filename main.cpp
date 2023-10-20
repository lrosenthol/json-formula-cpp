//
//  main.cpp
//  json-formula
//
//  Created by Leonard Rosenthol on 10/19/23.
//

#include <iostream>

// jsoncons
#include "jsoncons/json.hpp"
#include "jsoncons/json_error.hpp"
#include "jsoncons_ext/jmespath/jmespath.hpp"


static void eval(const jsoncons::json& j, const std::string& e)
{
	jsoncons::json r = jsoncons::jmespath::search(j,e);
	std::cout << e << " = " << jsoncons::print(r) << "\n";

}

int main(int argc, const char * argv[]) {
	std::cout << "Hello, json-formula!\n";
	
	try {
		std::string persons = R"(
		   {
			 "people": [
			{
			  "age": 20,
			  "other": "foo",
			  "tags": ["a", "b", "c"],
			  "name": "Bob"
			},
			{
			  "age": 25,
			  "other": "bar",
			  "tags": ["d", "e", "f"],
			  "name": "Fred"
			},
			{
			  "age": 30,
			  "other": "baz",
			  "tags": ["g", "h", "i"],
			  "name": "George"
			}
			 ]
		   }
		  )";
		// turn the text into a jsoncons::json structure
		jsoncons::json j = jsoncons::json::parse(persons);
		
		// examples from https://jmespath.org/examples.html
		jsoncons::json p1 = jsoncons::jmespath::search(j,"people[0].age");
		std::cout << "p1 = " << jsoncons::pretty_print(p1) << "\n";
		
		jsoncons::json p2 = jsoncons::jmespath::search(j,"people[?age > `20`].[name, age]");
		std::cout << "p2 = " << jsoncons::pretty_print(p2) << "\n";
		
		jsoncons::json p3 = jsoncons::jmespath::search(j,"people[*].{name: name, tags: tags[0]}");
		std::cout << "p3 = " << jsoncons::pretty_print(p3) << "\n";
	}
	catch (const jsoncons::json_exception& e)
	{
		std::cout << "Error:" << e.what() << std::endl;
	}

	try {
		std::string contents = R"(
		   {
			 "Contents": [
			{
			  "Date": "2014-12-21T05:18:08.000Z",
			  "Key": "logs/bb",
			  "Size": 303
			},
			{
			  "Date": "2014-12-20T05:19:10.000Z",
			  "Key": "logs/aa",
			  "Size": 308
			},
			{
			  "Date": "2014-12-20T05:19:12.000Z",
			  "Key": "logs/qux",
			  "Size": 297
			},
			{
			  "Date": "2014-11-20T05:22:23.000Z",
			  "Key": "logs/baz",
			  "Size": 329
			},
			{
			  "Date": "2014-12-20T05:25:24.000Z",
			  "Key": "logs/bar",
			  "Size": 604
			},
			{
			  "Date": "2014-12-20T05:27:12.000Z",
			  "Key": "logs/foo",
			  "Size": 647
			}
			 ]
		   }
		)";
		jsoncons::json j2 = jsoncons::json::parse(contents);
		
		jsoncons::json p4 = jsoncons::jmespath::search(j2,"sortBy(Contents, &Date)[*].{Key: Key, Size: Size}");
		std::cout << "p4 = " << jsoncons::pretty_print(p4) << "\n";

		jsoncons::json p5 = jsoncons::jmespath::search(j2,"Contents[*].Size | sum(@)");
		std::cout << "p5 = " << jsoncons::pretty_print(p5) << "\n";
	}
	catch (const jsoncons::json_exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}
	
	// built-in functions
	try {
		jsoncons::json j = jsoncons::json::parse(R"({"foo": -1, "bar": "-2"})");
		
		eval(j,"abs(toNumber(bar))");
		eval(j,"abs(`-5`)");

		eval(j,"startsWith('jack is at home', 'jack')");

		eval(j,"`2` == `2`");
		eval(j,"`2` = `2`");
		eval(j,"`2` != `2`");
		eval(j,"`2` <> `2`");
		eval(j,"`2` + `2`");
		eval(j,"`2` + `2.0`");
		eval(j,"`2` - `2`");
		eval(j,"`2` - `2.0`");
		eval(j,"`2` * `2`");
		eval(j,"`2` * `2.0`");
		eval(j,"`2` / `2`");
		eval(j,"`2` / `2.0`");
		eval(j,"'2' & '2'");

		eval(j,"`[1,2,3]` == `[1,2,3]`");
		eval(j,"`[1,2,3]` == `[1,2,3,4]`");
		eval(j,"`[1,2,3]` == `[4,5,6]`");
		eval(j,R"(`{"a":5,"b":7}` == `{"a":5,"b":7}`)");
		eval(j,R"(`{"a":5,"b":7}` == `{"b":7,"a":5}`)");
		eval(j,R"(`{"a":5,"b":7}` == `{"a":5,"b":7,"c":9}`)");

		eval(j,"`[1,2,3]` ~ `[4,5,6]`");
		eval(j,"`[1,2,3]` ~ `4`");
		eval(j,"`[1,2,3]` ~ `5.0`");
		eval(j,"`[1,2,3]` ~ 'foo'");
		eval(j,"`[1,2,3]` ~ `true`");
		eval(j,"`[1,2,3]` ~ `null`");
		eval(j,R"(`[1,2,3]` ~ `{"a":5,"b":7}`)");
	}
	catch (const jsoncons::json_exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}

	return 0;
}
