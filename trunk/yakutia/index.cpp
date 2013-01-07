#include <iostream>
#include <vector>
#include <string>
#include <uuid++.hh>

#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"

using namespace std;
using namespace cgicc;

int main(int argc, char**argv)
{
	try {
		Cgicc cgi;
		const CgiEnvironment& env = cgi.getEnvironment();
		// Iterate through the vector, and print out each value
		const_cookie_iterator iter;
		string cuid;
		for(iter = env.getCookieList().begin(); 
			iter != env.getCookieList().end(); 
			++iter) {
			if( iter->getName() == "cuid" )
				cuid = iter->getValue();
		}

		if(cuid.empty()) {
			uuid id;
			id.make(UUID_MAKE_V1);
			cuid = id.string();
		}

		cout << HTTPHTMLHeader()
			.setCookie(HTTPCookie("cuid", cuid))
			<< endl;

		// Set up the HTML document
		cout << html() << head(title("cgicc example")) << endl;
		cout << body() << endl;

		cout << h1("Testing...");

		if( !cuid.empty() )
			cout << "Cookie cuid received as: " << cuid << br() << endl;

		// Print out the submitted element
		form_iterator name = cgi.getElement("name");
		if(name != cgi.getElements().end()) {
			cout << "Your name: " << **name << endl;
		}

		// Close the HTML document
		cout << body() << html() << endl;

	}

	catch(exception& e) {
		cerr << e.what() << endl;
	}

}

