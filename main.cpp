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

// our user testing framework
#include "utest.h"

// use the standard user testing main
UTEST_MAIN();

static jsoncons::json eval(const jsoncons::json& j, const std::string& e, bool print=false)
{
	jsoncons::json r = jsoncons::jmespath::search(j,e);
	if ( print )
		std::cout << e << " = " << jsoncons::print(r) << "\n";
	
	return r;
}

//static std::string evalToString(const jsoncons::json& j, const std::string& e)
//{
//	jsoncons::json r = jsoncons::jmespath::search(j,e);
//	return static_cast<std::string>(r.as<std::string_view>());
//}

// tests!
// MARK: Equality
UTEST(json_formula, operators_equality) {
	jsoncons::json j;	// we don't need any actual JSON to evaluate these tests

	EXPECT_TRUE(eval(j,"`2` == `2`").as<bool>());
	EXPECT_TRUE(eval(j,"`2` = `2`").as<bool>());
	EXPECT_FALSE(eval(j,"`2` != `2`").as<bool>());
	EXPECT_FALSE(eval(j,"`2` <> `2`").as<bool>());

	EXPECT_TRUE(eval(j,"`[1,2,3]` == `[1,2,3]`").as<bool>());
	EXPECT_FALSE(eval(j,"`[1,2,3]` == `[1,2,3,4]`").as<bool>());
	EXPECT_FALSE(eval(j,"`[1,2,3]` == `[4,5,6]`").as<bool>());
	EXPECT_TRUE(eval(j,R"(`{"a":5,"b":7}` == `{"a":5,"b":7}`)").as<bool>());
	EXPECT_TRUE(eval(j,R"(`{"a":5,"b":7}` == `{"b":7,"a":5}`)").as<bool>());
	EXPECT_FALSE(eval(j,R"(`{"a":5,"b":7}` == `{"a":5,"b":7,"c":9}`)").as<bool>());
}

// MARK: Comparisons
UTEST(json_formula, operators_comparisons) {
	jsoncons::json j;	// we don't need any actual JSON to evaluate these tests

	EXPECT_TRUE(eval(j,R"('2' < '6')").as<bool>());
	EXPECT_TRUE(eval(j,R"('2' <= '6')").as<bool>());
	EXPECT_FALSE(eval(j,R"('2' > '6')").as<bool>());
	EXPECT_FALSE(eval(j,R"('2' >= '6')").as<bool>());

	EXPECT_TRUE(eval(j,R"(`2` < '6')").as<bool>());
	EXPECT_TRUE(eval(j,R"('2' < `6`)").as<bool>());

	EXPECT_TRUE(eval(j,R"(`2` <= '6')").as<bool>());
	EXPECT_TRUE(eval(j,R"('2' <= `6`)").as<bool>());

	EXPECT_FALSE(eval(j,R"(`2` > '6')").as<bool>());
	EXPECT_FALSE(eval(j,R"('2' > `6`)").as<bool>());

	EXPECT_FALSE(eval(j,R"(`2` >= '6')").as<bool>());
	EXPECT_FALSE(eval(j,R"('2' >= `6`)").as<bool>());
}

// MARK: Math
UTEST(json_formula, operators_math) {
	jsoncons::json j;	// we don't need any actual JSON to evaluate these tests

	// test integers & doubles
	EXPECT_EQ(eval(j,"`2` + `2`").as<int32_t>(), 4);
	EXPECT_EQ(eval(j,"`2` + `2.0`").as<double>(), 4.0);
	EXPECT_EQ(eval(j,"`2` - `2`").as<int32_t>(), 0);
	EXPECT_EQ(eval(j,"`2` - `2.0`").as<double>(), 0.0);
	EXPECT_EQ(eval(j,"`2` * `2`").as<int32_t>(), 4);
	EXPECT_EQ(eval(j,"`2` * `2.0`").as<double>(), 4.0);
	EXPECT_EQ(eval(j,"`2` / `2`").as<int32_t>(), 1);
	EXPECT_EQ(eval(j,"`2` / `2.0`").as<double>(), 1.0);
	
	// test string & null coercions
	EXPECT_EQ(eval(j,"`100` + '3'").as<int32_t>(), 103);
	EXPECT_EQ(eval(j,"`100.56` + '3'").as<double>(), 103.56);
	EXPECT_EQ(eval(j,"`100` + `null`").as<int32_t>(), 100);
	EXPECT_EQ(eval(j,"`100` + !`null`").as<int32_t>(), 101);
	EXPECT_EQ(eval(j,"`100` + '3' + `null`").as<int32_t>(), 103);
	EXPECT_EQ(eval(j,"`100` - '3'").as<int32_t>(), 97);
	EXPECT_EQ(eval(j,"`100.56` - '3'").as<double>(), 97.56);
	EXPECT_EQ(eval(j,"`100` - `null`").as<int32_t>(), 100);
	EXPECT_EQ(eval(j,"`100` - !`null`").as<int32_t>(), 99);
	EXPECT_EQ(eval(j,"`100` - '3' - `null`").as<int32_t>(), 97);
	EXPECT_EQ(eval(j,"`100` * '3'").as<int32_t>(), 300);
	EXPECT_EQ(eval(j,"`100.56` * '3'").as<double>(), 301.68);
	EXPECT_EQ(eval(j,"`100` * `null`").as<int32_t>(), 0);
	EXPECT_EQ(eval(j,"`100` * !`null`").as<int32_t>(), 100);
	EXPECT_EQ(eval(j,"`100` * '3' * `null`").as<int32_t>(), 0);
	EXPECT_EQ(eval(j,"`100` / '4'").as<int32_t>(), 25);
	EXPECT_EQ(eval(j,"`100.80` / '4'").as<double>(), 25.20);
	EXPECT_TRUE(eval(j,"`100` / `null`") == jsoncons::json(nullptr));
	EXPECT_EQ(eval(j,"`100` / !`null`").as<int32_t>(), 100);
	EXPECT_TRUE(eval(j,"`100` / '3' / `null`") == jsoncons::json(nullptr));

	// test the array special cases
	// Arrays shall be coerced to an array of numbers.
	EXPECT_TRUE(eval(j,R"(`[1,2,3]` + `4`)") == jsoncons::json(jsoncons::json_array_arg, {5,6,7}));
	EXPECT_TRUE(eval(j,R"(`1` + `[4,5,6,7]`)") == jsoncons::json(jsoncons::json_array_arg, {5,6,7,8}));
	EXPECT_TRUE(eval(j,R"(`[1,2,3]` + `[4,5,6,7]`)") == jsoncons::json(jsoncons::json_array_arg, {5,7,9,7}));
	EXPECT_TRUE(eval(j,R"(`[1,2,3]` - `4`)") == jsoncons::json(jsoncons::json_array_arg, {-3, -2, -1}));
	EXPECT_TRUE(eval(j,R"(`1` - `[4,5,6,7]`)") == jsoncons::json(jsoncons::json_array_arg, {-3,-4,-5,-6}));
	EXPECT_TRUE(eval(j,R"(`[1,2,3]` - `[4,5,6,7]`)") == jsoncons::json(jsoncons::json_array_arg, {-3,-3,-3,-7}));
	EXPECT_TRUE(eval(j,R"(`[1,2,3]` * `4`)") == jsoncons::json(jsoncons::json_array_arg, {4,8,12}));
	EXPECT_TRUE(eval(j,R"(`2` * `[4,5,6,7]`)") == jsoncons::json(jsoncons::json_array_arg, {8,10,12,14}));
	EXPECT_TRUE(eval(j,R"(`[1,2,3]` * `[4,5,6,7]`)") == jsoncons::json(jsoncons::json_array_arg, {4,10,18,0}));
	EXPECT_TRUE(eval(j,R"(`[1.1,2.1,3.1]` * `[4,5,6,7]`)") == jsoncons::json(jsoncons::json_array_arg, {4.4,10.5,18.6,0}));
	EXPECT_TRUE(eval(j,R"(`[10,12,14]` / `2`)") == jsoncons::json(jsoncons::json_array_arg, {5,6,7}));
	EXPECT_TRUE(eval(j,R"(`2` / `[2,4,8]`)") == jsoncons::json(jsoncons::json_array_arg, {1,0.5,0.25}));
	EXPECT_TRUE(eval(j,R"(`[10,12,14]` / `[5,6,7,8]`)") == jsoncons::json(jsoncons::json_array_arg, {2,2,2,0}));
}

// MARK: Concat
UTEST(json_formula, operator_concat) {
	jsoncons::json j;	// we don't need any actual JSON to evaluate these tests

	EXPECT_TRUE(eval(j,"'2' & '2'") == jsoncons::json("22"));
	EXPECT_TRUE(eval(j,"'2' & `2`") == jsoncons::json("22"));
	EXPECT_TRUE(eval(j,"'2' & `2.0`") == jsoncons::json("22.000000"));	// floating point sucks!
	EXPECT_TRUE(eval(j,"'2' & `false`") == jsoncons::json("2false"));
	EXPECT_TRUE(eval(j,"'2' & `[2]`") == jsoncons::json(jsoncons::json_array_arg, {"22"}));
	EXPECT_TRUE(eval(j,R"('2' & `[2,"foo"]`)") == jsoncons::json(jsoncons::json_array_arg, {"22", "2foo"}));
	EXPECT_TRUE(eval(j,R"(`[2,"bar",true]` & '2')") == jsoncons::json(jsoncons::json_array_arg, {"22", "bar2", "true2"}));
	EXPECT_TRUE(eval(j,R"(`["foo", 22]` & `[44, true, "x"]`)") == jsoncons::json(jsoncons::json_array_arg, {"foo44", "22true", "x"}));
	EXPECT_TRUE(eval(j,R"('2' & `{"a":2}`)") == jsoncons::json("2"));
}

// MARK: Merge
UTEST(json_formula, operator_merge) {
	jsoncons::json j;	// we don't need any actual JSON to evaluate these tests
	
	EXPECT_TRUE(eval(j,"'2' & '2'") == jsoncons::json("22"));

	EXPECT_TRUE(eval(j,"`[1,2,3]` ~ `[4,5,6]`") == jsoncons::json(jsoncons::json_array_arg, {1,2,3,4,5,6}));
	EXPECT_TRUE(eval(j,"`[1,2,3]` ~ `4`") == jsoncons::json(jsoncons::json_array_arg, {1,2,3,4}));
	EXPECT_TRUE(eval(j,"`[1,2,3]` ~ `5.0`") == jsoncons::json(jsoncons::json_array_arg, {1,2,3,5.0}));
	EXPECT_TRUE(eval(j,"`[1,2,3]` ~ 'foo'") == jsoncons::json(jsoncons::json_array_arg, {1,2,3,"foo"}));
	EXPECT_TRUE(eval(j,"`[1,2,3]` ~ `true`") == jsoncons::json(jsoncons::json_array_arg, {1,2,3,true}));
	EXPECT_TRUE(eval(j,"`[1,2,3]` ~ `null`") == jsoncons::json(jsoncons::json_array_arg, {1,2,3}));
	EXPECT_TRUE(eval(j,R"(`[1,2,3]` ~ `{"a":5,"b":7}`)") == jsoncons::json(jsoncons::json_array_arg, {1,2,3}));
}

// MARK: JMESPath Functions
UTEST(json_formula, jmespath_functions) {
	jsoncons::json j = jsoncons::json::parse(R"({"foo": -1, "bar": "-2"})");
	
	EXPECT_EQ(eval(j,"abs(toNumber(bar))").as<int32_t>(), 2);
	EXPECT_EQ(eval(j,"abs(`-5`)").as<int32_t>(), 5);
	
	EXPECT_EQ(eval(j,"avg(`[10, 15, 20]`)").as<int32_t>(), 15);
	
	EXPECT_TRUE(eval(j,"contains('foobar', 'foo')").as<bool>());
	EXPECT_FALSE(eval(j,"contains('foobar', 'not')").as<bool>());
	EXPECT_TRUE(eval(j,"contains('foobar', 'bar')").as<bool>());
	EXPECT_FALSE(eval(j,"contains('foobar', `123`)").as<bool>());
	EXPECT_TRUE(eval(j,R"(contains(`["a", "b"]`, 'a'))").as<bool>());
	EXPECT_FALSE(eval(j,R"(contains(`["a", "b"]`, 'c'))").as<bool>());
	EXPECT_FALSE(eval(j,R"(contains(`["foo", "bar"]`, 'b'))").as<bool>());
	
	EXPECT_EQ(eval(j,"ceil(`1.001`)").as<int32_t>(), 2);
	EXPECT_EQ(eval(j,"ceil(`1.9`)").as<int32_t>(), 2);
	EXPECT_EQ(eval(j,"ceil(`1`)").as<int32_t>(), 1);
	
	EXPECT_TRUE(eval(j,"endsWith('foobarbaz', 'baz')").as<bool>());
	EXPECT_TRUE(eval(j,"endsWith('foobarbaz', 'z')").as<bool>());
	EXPECT_FALSE(eval(j,"endsWith('foobarbaz', 'foo')").as<bool>());
	
	EXPECT_EQ(eval(j,"floor(`1.001`)").as<int32_t>(), 1);
	EXPECT_EQ(eval(j,"floor(`1.9`)").as<int32_t>(), 1);
	EXPECT_EQ(eval(j,"floor(`1`)").as<int32_t>(), 1);
	
	EXPECT_TRUE(eval(j,R"(join(', ', `["a", "b"]`))") == jsoncons::json("a, b"));
	EXPECT_TRUE(eval(j,R"(join('', `["a", "b"]`))") == jsoncons::json("ab"));
	
	// Note that because JSON hashes are inheritently unordered,
	// the keys associated with the provided object obj are inheritently unordered.
	// Implementations are not required to return keys in any specific order.
	EXPECT_TRUE(eval(j,R"(keys(`{"foo": "baz", "bar": "bam"}`))") == jsoncons::json(jsoncons::json_array_arg, {"bar", "foo"}));
	EXPECT_TRUE(eval(j,R"(keys(`{}`))") == jsoncons::json(jsoncons::json_array_arg, {}));
	
	EXPECT_EQ(eval(j,"length('abc')").as<int32_t>(), 3);
	EXPECT_EQ(eval(j,R"(length(`"current"`))").as<int32_t>(), 7);
	EXPECT_EQ(eval(j,R"(length(`["a","b","c"]`))").as<int32_t>(), 3);
	EXPECT_EQ(eval(j,R"(length(`[]`))").as<int32_t>(), 0);
	EXPECT_EQ(eval(j,R"(length(`{}`))").as<int32_t>(), 0);
	EXPECT_EQ(eval(j,R"(length(`{"foo": "bar", "baz": "bam"}`))").as<int32_t>(), 2);
	
	EXPECT_TRUE(eval(j,R"(map(&foo, `[{"foo": "a"}, {"foo": "b"}, {}, [], {"foo": "f"}]`))") == jsoncons::json(jsoncons::json_array_arg, {"a", "b", nullptr, nullptr, "f"}));
	//	EXPECT_TRUE(eval(j,R"(map(&[], `[[1,2],3,[4],[5,6,7],[8,9]]`))", true) == jsoncons::json(jsoncons::json_array_arg, {1,2,3,4,5,6,7,8,9}));
	
	EXPECT_TRUE(eval(j,R"(max(`[5, 10, 15]`))") == jsoncons::json(15));
	EXPECT_TRUE(eval(j,R"(max(`["a", "b"]`))") == jsoncons::json("b"));
	
	EXPECT_TRUE(eval(j,R"(merge(`{"a": "b"}`, `{"c": "d"}`))") == jsoncons::json({{"a",jsoncons::json("b")},{"c",jsoncons::json("d")}}));
	EXPECT_TRUE(eval(j,R"(merge(`{"a": "b"}`, `{"a": "override"}`))") == jsoncons::json(jsoncons::json_object_arg, {{"a",jsoncons::json("override")}}));
	EXPECT_TRUE(eval(j,R"(merge(`{"a": "x", "b": "y"}`, `{"b": "override", "c": "z"}`))") == jsoncons::json({{"a",jsoncons::json("x")},{"b",jsoncons::json("override")},{"c",jsoncons::json("z")}}));
	
	EXPECT_TRUE(eval(j,R"(min(`[5, 10, 15]`))") == jsoncons::json(5));
	EXPECT_TRUE(eval(j,R"(min(`["a", "b"]`))") == jsoncons::json("a"));
	
	EXPECT_TRUE(eval(j,R"(reverse(`[0, 1, 2, 3, 4]`))") == jsoncons::json(jsoncons::json_array_arg, {4,3,2,1,0}));
	EXPECT_TRUE(eval(j,R"(reverse(`["a", "b", "c", 1, 2, 3]`))") == jsoncons::json(jsoncons::json_array_arg, {3,2,1,"c","b","a"}));
	EXPECT_TRUE(eval(j,R"(reverse('abcd'))") == jsoncons::json("dcba"));
	
	EXPECT_TRUE(eval(j,R"(sort(`["b","c","a"]`))") == jsoncons::json(jsoncons::json_array_arg, {"a","b","c"}));
	EXPECT_TRUE(eval(j,R"(sort(`[10, 5, 15, 1]`))") == jsoncons::json(jsoncons::json_array_arg, {1,5,10,15}));
	//	EXPECT_TRUE(eval(j,R"(sort(`[1,"c","a"]`))") == jsoncons::json(jsoncons::json_array_arg, {1,"c","a"}));
	//	EXPECT_TRUE(eval(j,R"(sort(`{"a": 1, "b": 2}`))") == jsoncons::json(nullptr));
	
	EXPECT_TRUE(eval(j,"startsWith('jack is at home', 'jack')").as<bool>());
	EXPECT_FALSE(eval(j,"startsWith('foobarbaz', 'baz')").as<bool>());
	EXPECT_TRUE(eval(j,"startsWith('foobarbaz', 'f')").as<bool>());
	EXPECT_TRUE(eval(j,"startsWith('foobarbaz', 'foo')").as<bool>());
	
	EXPECT_TRUE(eval(j,R"(sum(`[5, 10, 15]`))") == jsoncons::json(30));
	EXPECT_TRUE(eval(j,R"(sum(`[]`))") == jsoncons::json(0));
	
	EXPECT_TRUE(eval(j,R"(to_array(`["b","c","a"]`))") == jsoncons::json(jsoncons::json_array_arg, {"b","c","a"}));
	EXPECT_TRUE(eval(j,R"(to_array('b'))") == jsoncons::json(jsoncons::json_array_arg, {"b"}));
	EXPECT_TRUE(eval(j,R"(to_array(`0`))") == jsoncons::json(jsoncons::json_array_arg, {0}));
	EXPECT_TRUE(eval(j,R"(to_array(`false`))") == jsoncons::json(jsoncons::json_array_arg, {false}));
	
	EXPECT_TRUE(eval(j,R"(to_string(`["b","c","a"]`))") == jsoncons::json("[\"b\",\"c\",\"a\"]"));
	EXPECT_TRUE(eval(j,R"(to_string('b'))") == jsoncons::json("b"));
	EXPECT_TRUE(eval(j,R"(to_string(`0`))") == jsoncons::json("0"));
	EXPECT_TRUE(eval(j,R"(to_string(`false`))") == jsoncons::json("false"));
	
	EXPECT_TRUE(eval(j,R"(to_number('100'))") == jsoncons::json(100));
	EXPECT_EQ(eval(j,"to_number('211')").as<int32_t>(), 211);
	EXPECT_EQ(eval(j,"to_number('123.456')").as<double>(), 123.456);
	EXPECT_TRUE(eval(j,R"(to_number('foo'))") == jsoncons::json(nullptr));
	EXPECT_TRUE(eval(j,R"(to_number(null))") == jsoncons::json(0));
	EXPECT_TRUE(eval(j,R"(to_number(`null`))") == jsoncons::json(0));

	EXPECT_TRUE(eval(j,R"(type(`100`))") == jsoncons::json("number"));
	EXPECT_TRUE(eval(j,R"(type(`123.456`))") == jsoncons::json("number"));
	EXPECT_TRUE(eval(j,R"(type(`"abcd"`))") == jsoncons::json("string"));
	EXPECT_TRUE(eval(j,R"(type('efgh'))") == jsoncons::json("string"));
	EXPECT_TRUE(eval(j,R"(type(`false`))") == jsoncons::json("boolean"));
	EXPECT_TRUE(eval(j,R"(type(`null`))") == jsoncons::json("null"));
	EXPECT_TRUE(eval(j,R"(type(`[8,9,10]`))") == jsoncons::json("array"));
	EXPECT_TRUE(eval(j,R"(type(`{"a":"b", "foo":78}`))") == jsoncons::json("object"));
	
	// Note that because JSON hashes are inheritently unordered,
	// the values associated with the provided object obj are inheritently unordered.
	// Implementations are not required to return values in any specific order. For example, given the input:
	EXPECT_TRUE(eval(j,R"(values(`{"foo": "baz", "bar": "bam"}`))") == jsoncons::json(jsoncons::json_array_arg, {"bam", "baz"}));
}

// MARK: People test
UTEST(json_formula, expressions_people) {
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
	jsoncons::json j = jsoncons::json::parse(persons);
	
	EXPECT_TRUE(eval(j,"people[0].age") == jsoncons::json(20));
	EXPECT_TRUE(eval(j,"people[?age > `20`].[name, age]") == jsoncons::json::parse(R"([["Fred",25],["George",30]])"));
	EXPECT_TRUE(eval(j,"people[*].{name: name, tags: tags[0]}") == jsoncons::json::parse(R"([{"name":"Bob","tags":"a"},{"name":"Fred","tags":"d"},{"name":"George","tags":"g"}])"));
	
	EXPECT_TRUE(eval(j,"max_by(people, &age)") == jsoncons::json::parse(R"({"age":30,"name":"George","other":"baz","tags":["g","h","i"]})"));
	EXPECT_TRUE(eval(j,"max_by(people, &age).age") == jsoncons::json(30));
	EXPECT_TRUE(eval(j,"min_by(people, &age).age") == jsoncons::json(20));
	EXPECT_TRUE(eval(j,"sort_by(people, &age)[].age") == jsoncons::json({20, 25, 30}));
}

// MARK: Contents Test
UTEST(json_formula, expressions_contents) {
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
	jsoncons::json j = jsoncons::json::parse(contents);

	EXPECT_TRUE(eval(j,"Contents[*].Size | sum(@)") == jsoncons::json(2488));
	EXPECT_TRUE(eval(j,"sortBy(Contents, &Date)[*].{Key: Key, Size: Size}") == jsoncons::json::parse(R"([{"Key":"logs/baz","Size":329},{"Key":"logs/aa","Size":308},{"Key":"logs/qux","Size":297},{"Key":"logs/bar","Size":604},{"Key":"logs/foo","Size":647},{"Key":"logs/bb","Size":303}])"));

}

// MARK: misc
UTEST(json_formula, misc) {
	jsoncons::json j = jsoncons::json::parse(R"({"a":"b", "c":100})");
	
	EXPECT_TRUE(eval(j,"[`3`]") == jsoncons::json(jsoncons::json_array_arg, {3}));
	EXPECT_TRUE(eval(j,"`[3]`") == jsoncons::json(jsoncons::json_array_arg, {3}));
	EXPECT_TRUE(eval(j,"[3]") == jsoncons::json(nullptr));
//	EXPECT_TRUE(eval(j,"[`2+3`]", true) == jsoncons::json(jsoncons::json_array_arg, {5}));
	EXPECT_TRUE(eval(j,"[length('123')]") == jsoncons::json(jsoncons::json_array_arg, {3}));
	EXPECT_TRUE(eval(j,"[]") == jsoncons::json(nullptr));
	EXPECT_TRUE(eval(j,"[[]]") == jsoncons::json(jsoncons::json_array_arg, {nullptr}));
//	EXPECT_TRUE(eval(j,"{}", true) == jsoncons::json(nullptr));
}


// MARK: Hacking
UTEST(json_formula, hacking) {
	jsoncons::json j = jsoncons::json::parse(R"({"a":"b", "c":100})");
	
	EXPECT_TRUE(eval(j,"10 + 1", true) == jsoncons::json(200));
}
