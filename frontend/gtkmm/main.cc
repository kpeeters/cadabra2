
#include "Client.hh"
#include "NotebookWindow.hh"
#include "Netbits.hh"

#include <gtkmm/application.h>

// Cadabra frontend with GTK+ interface (using gtkmm). 
// Makes use of the client classes in the client_server directory.


int main(int argc, char **argv)
	{
	// Create the ui material.
	Glib::RefPtr<Gtk::Application> app =
		Gtk::Application::create(argc, argv, "com.phi-sci.cadabra.cadabra-gtk");
  	cadabra::NotebookWindow nw;

	// Create and start the network thread.
	Netbits  cdb(nw);
	std::thread client_thread(&Netbits::run, std::ref(cdb));

	// Start the ui in the main thread.
	app->run(nw);
	client_thread.join();
	}
