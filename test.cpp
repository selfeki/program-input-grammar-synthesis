
#include <libxml++/libxml++.h>
#include <libxml++/parsers/textreader.h>
#include <glibmm/ustring.h>

#include <iostream>
#include <stdlib.h>


int main() 
{
  Glib::ustring ustring("<a>hi</a>");
	try{
		xmlpp::TextReader reader(ustring);
	}
	catch(const std::exception& e) {
    std::cerr << "Exception caught: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}	