
#include "Cadabra.hh"

#include <gtkmm/application.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/label.h>

// Cadabra frontend with GTK+ interface (using gtkmm). 
// Makes use of the client classes in the client_server directory.

int main(int argc, char **argv)
	{
	try {
		auto application = Cadabra::create(argc, argv);
		const int status = application->run();
		return status;
		}
	catch(Glib::Error& er) {
		std::cerr << er.what() << std::endl;
		return -1;
		}
	}
