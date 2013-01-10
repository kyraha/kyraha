#include <iostream>
#include <vector>
#include <string>
#include <uuid++.hh>

#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"

using namespace std;
using namespace cgicc;

class Session : public Cgicc {
	public:
		static const char* COOKIE_id() {return "cuid";};
		string	id;
		HTTPContentHeader* header;

		Session();
		virtual ~Session();

};

Session::~Session()
{
	delete header;
}

Session::Session() : Cgicc(), header(new HTTPHTMLHeader)
{
	const CgiEnvironment& env = getEnvironment();
	// Iterate through the vector, and print out each value
	const_cookie_iterator iter;
	for(iter = env.getCookieList().begin(); 
		iter != env.getCookieList().end(); 
		++iter) {
		if( iter->getName() == COOKIE_id() )
			id = iter->getValue();
	}

	if(id.empty()) {
		uuid guid;
		guid.make(UUID_MAKE_V1);
		id = guid.string();
	}

}

int main(int argc, char**argv)
{
	try {
		Session sess;

		cout << sess.header
			->setCookie(HTTPCookie(Session::COOKIE_id(), sess.id))
			<< endl;

		// Set up the HTML document
		cout << html() << head(title("example")) << endl;
		cout << body() << endl;

		cout << h1("Testing...");

		if( !sess.id.empty() )
			cout << "Cookie received is: " << sess.id << br() << endl;

		// Print out the submitted element
		form_iterator name = sess.getElement("name");
		if(name != sess.getElements().end()) {
			cout << "Your name: " << **name << endl;
		}

		// Close the HTML document
		cout << body() << html() << endl;

	}

	catch(exception& e) {
		cerr << e.what() << endl;
	}

}

