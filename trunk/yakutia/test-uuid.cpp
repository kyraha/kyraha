#include <uuid++.hh>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char**argv)
{
	uuid id;
	id.make(UUID_MAKE_V1);
	cout << "Just generated UUID: " << id.string();
	cout << endl;
	return 0;
}
