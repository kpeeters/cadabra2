
#include "Log.hh"
#include "Cadabra.hh"

#include <gtkmm/application.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/label.h>

// Cadabra frontend with GTK+ interface (using gtkmm). 
// Makes use of the client classes in the client_server directory.

int main(int argc, char **argv)
	{
	boost::log::add_common_attributes();

	try {
		auto application = Cadabra::create();
		const int status = application->run(argc, argv);
		return status;

		// The window has been closed; stop the compute logic and join
		// that thread waiting for it to complete.
		// FIXME: move to application
//		compute.terminate();
//		compute_thread.join();
		}
	catch(Glib::Error& er) {
		std::cerr << er.what() << std::endl;
		return -1;
		}
	}
