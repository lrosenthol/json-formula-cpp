//
//  json-formula.cpp
//  json-formula
//
//  Created by Leonard Rosenthol on 11/20/23.
//

#include "json-formula.h"


namespace jsonformula {

jsoncons::json parser::eval( const std::string& s )
{
	std::string curTok;
	size_t sLoc = 0;
	while ( sLoc < s.length() ) {
		unsigned char c{static_cast<unsigned char>(s[sLoc++])};
		
		if ( c == ' ' || c == '\t' ) {
			// push current token on the stack & clear the current one
			mTokens.push_back(jsoncons::json(curTok));
			curTok.clear();
		} else if ( c == '`' ) { // found a literal, so scan for next one (handling escapes)
			bool stop = false;
			while ( sLoc < s.length() && !stop ) {
				unsigned char lc{static_cast<unsigned char>(s[sLoc++])};
				if ( lc == '\\' ) {		// escape
				} else if ( lc == '`') {	// end of literal
					jsoncons::json r = jsoncons::jmespath::search(jsoncons::json(),curTok);
					std::cout << jsoncons::print(r) << std::endl;
					mTokens.push_back(r);
					curTok.clear();
					stop = true;
				} else {
					curTok += lc;
				}
			}
		} else {
			curTok += c;
		}
	}
	if ( !curTok.empty() )	//anything left in the queue, add it!
		mTokens.push_back(jsoncons::json(curTok));

	// dump the tokens
	for ( auto& t : mTokens ) {
		std::cout << jsoncons::print(t) << std::endl;
	}
	return jsoncons::json("test");
}


jsoncons::json parser::eval(const jsoncons::json& j, const std::string& e)
{
	parser p( j );
	jsoncons::json result = p.eval(e);
	
	return jsoncons::json("test");
}


}
